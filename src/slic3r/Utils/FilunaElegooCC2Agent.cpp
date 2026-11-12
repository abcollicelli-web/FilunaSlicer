#include "FilunaElegooCC2Agent.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <thread>
#include <vector>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>
#include <openssl/evp.h>

#include "FilunaMqttClient.hpp"

namespace Slic3r {

namespace net   = boost::asio;
namespace beast = boost::beast;
namespace http  = boost::beast::http;
using json      = nlohmann::json;

namespace {

std::string md5_file(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    EVP_MD_CTX*   ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
    std::vector<char> buf(1 << 16);
    while (in) {
        in.read(buf.data(), std::streamsize(buf.size()));
        if (in.gcount() > 0)
            EVP_DigestUpdate(ctx, buf.data(), std::size_t(in.gcount()));
    }
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int  n = 0;
    EVP_DigestFinal_ex(ctx, md, &n);
    EVP_MD_CTX_free(ctx);
    static const char* hex = "0123456789abcdef";
    std::string        out;
    for (unsigned i = 0; i < n; ++i) {
        out.push_back(hex[md[i] >> 4]);
        out.push_back(hex[md[i] & 15]);
    }
    return out;
}

std::string hex_rgba(std::string c)
{
    if (!c.empty() && c[0] == '#')
        c.erase(0, 1);
    std::transform(c.begin(), c.end(), c.begin(), ::toupper);
    if (c.size() == 6)
        c += "FF";
    return c.size() == 8 ? c : std::string("FFFFFFFF");
}

} // namespace

AgentInfo FilunaElegooCC2Agent::get_agent_info_static()
{
    return AgentInfo{FILUNA_ELEGOO_CC2_AGENT_ID, "Elegoo Centauri Carbon 2 (LAN aberto)", "1.0.0",
                     "Protocolo LAN do SDK aberto Elegoo-Link: UDP 52700, MQTT 1883, HTTP /upload"};
}

FilunaElegooCC2Agent::FilunaElegooCC2Agent(std::string)
{
    m_client_id = "filuna" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count() % 100000);
}

FilunaElegooCC2Agent::~FilunaElegooCC2Agent()
{
    start_discovery(false, false);
    disconnect_printer();
}

json FilunaElegooCC2Agent::to_push_status(const json& s)
{
    json p;
    p["command"]     = "push_status";
    p["sequence_id"] = "0";
    const auto ms    = s.value("machine_status", json::object());
    const int  st    = ms.value("status", -1);
    const int  sub   = ms.value("sub_status", 0);
    std::string gs   = "IDLE";
    if (st == 2)
        gs = (sub == 2501 || sub == 2502 || sub == 2505) ? "PAUSE" : "RUNNING";
    else if (st == 14)
        gs = "FAILED";
    else if (st == 0)
        gs = "INIT";
    p["gcode_state"] = gs;
    const auto ps    = s.value("print_status", json::object());
    p["mc_percent"]  = ps.value("progress", 0);
    p["mc_remaining_time"] = ps.value("remaining_time_sec", 0) / 60;
    p["subtask_name"]      = ps.value("filename", std::string());
    p["gcode_file"]        = ps.value("filename", std::string());
    p["layer_num"]         = ps.value("current_layer", 0);
    p["total_layer_num"]   = ps.value("total_layer", 0);
    const auto ex = s.value("extruder", json::object());
    const auto hb = s.value("heater_bed", json::object());
    p["nozzle_temper"]        = ex.value("temperature", 0.0);
    p["nozzle_target_temper"] = ex.value("target", 0.0);
    p["bed_temper"]           = hb.value("temperature", 0.0);
    p["bed_target_temper"]    = hb.value("target", 0.0);
    // Canvas (sistema de cores da CC2) -> unidades "ams" no formato do DeviceManager
    if (s.contains("canvas_info") && s["canvas_info"].is_object()) {
        json units = json::array();
        for (const auto& c : s["canvas_info"].value("canvas_list", json::array())) {
            json trays = json::array();
            for (const auto& t : c.value("tray_list", json::array())) {
                json tr;
                tr["id"]         = std::to_string(t.value("tray_id", 0));
                tr["tray_type"]  = t.value("filament_type", std::string());
                tr["tray_color"] = hex_rgba(t.value("filament_color", std::string()));
                tr["tray_sub_brands"] = t.value("brand", std::string());
                trays.push_back(tr);
            }
            units.push_back({{"id", std::to_string(c.value("canvas_id", 0))}, {"tray", trays}});
        }
        p["ams"] = {{"ams", units}, {"ams_exist_bits", units.empty() ? "0" : "1"},
                    {"tray_now", std::to_string(s["canvas_info"].value("active_tray_id", 255))}};
    }
    return json{{"print", p}};
}

json FilunaElegooCC2Agent::to_cc2_request(const json& cmd, int id)
{
    std::string c;
    if (cmd.contains("print"))
        c = cmd["print"].value("command", std::string());
    else if (cmd.contains("pushing"))
        c = cmd["pushing"].value("command", std::string());
    int method = 0;
    if (c == "pause")
        method = 1021;
    else if (c == "resume")
        method = 1023;
    else if (c == "stop")
        method = 1022;
    else if (c == "pushall")
        method = 1002;
    if (!method)
        return {};
    return json{{"id", id}, {"method", method}, {"params", json::object()}};
}

std::string FilunaElegooCC2Agent::discovery_reply_to_json(const std::string& reply, const std::string& ip)
{
    json r = json::parse(reply, nullptr, false);
    if (!r.is_object() || !r.contains("id") || !r.contains("result") || !r["result"].is_object())
        return {};
    const json& x = r["result"];
    if (x.value("sn", std::string()).empty())
        return {};
    json j;
    j["dev_id"]       = x.value("sn", std::string());
    j["dev_ip"]       = ip;
    j["dev_type"]     = x.value("machine_model", std::string("Centauri Carbon 2"));
    j["dev_name"]     = x.value("host_name", j["dev_type"].get<std::string>());
    j["dev_signal"]   = "0";
    j["connect_type"] = "lan";
    j["bind_state"]   = "free";
    j["sec_link"]     = "secure";
    j["ssdp_version"] = "v1";
    return j.dump();
}

bool FilunaElegooCC2Agent::request(int method, const json& params)
{
    std::lock_guard<std::recursive_mutex> l(m_mutex);
    if (!m_mqtt || !m_mqtt->connected())
        return false;
    json r{{"id", m_next_id++}, {"method", method}, {"params", params}};
    return m_mqtt->publish("elegoo/" + m_sn + "/" + m_client_id + "/api_request", r.dump());
}

void FilunaElegooCC2Agent::on_mqtt(const std::string& topic, const std::string& payload)
{
    json j = json::parse(payload, nullptr, false);
    if (!j.is_object())
        return;
    if (topic.size() >= 18 && topic.compare(topic.size() - 18, 18, "/register_response") == 0)
        return;
    // resposta a pedido (api_response) ou empurrão de estado (api_status, método 6000)
    json result = j.contains("result") ? j["result"] : (j.contains("params") ? j["params"] : json());
    if (!result.is_object())
        return;
    json snapshot;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        m_status.merge_patch(result);
        snapshot = m_status;
    }
    dispatch(to_push_status(snapshot).dump());
}

void FilunaElegooCC2Agent::dispatch(const std::string& payload)
{
    OnMessageFn   local, cloud;
    QueueOnMainFn q;
    std::string   dev;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        local = m_on_local_message;
        cloud = m_on_message;
        q     = m_queue_on_main;
        dev   = m_sn;
    }
    auto call = [local, cloud, dev, payload] {
        if (local)
            local(dev, payload);
        else if (cloud)
            cloud(dev, payload);
    };
    q ? q(call) : call();
}

void FilunaElegooCC2Agent::dispatch_connect(int state, const std::string& msg)
{
    OnLocalConnectedFn fn;
    QueueOnMainFn      q;
    std::string        dev;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        fn  = m_on_local_connect;
        q   = m_queue_on_main;
        dev = m_sn;
    }
    if (!fn)
        return;
    auto call = [fn, state, dev, msg] { fn(state, dev, msg); };
    q ? q(call) : call();
}

int FilunaElegooCC2Agent::send_message(std::string dev_id, std::string json_str, int qos, int flag)
{
    return send_message_to_printer(std::move(dev_id), std::move(json_str), qos, flag);
}

int FilunaElegooCC2Agent::connect_printer(const PrinterConnectionParams& p)
{
    disconnect_printer();
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        m_sn   = p.dev_id;
        m_ip   = p.host;
        m_code = p.password.empty() ? std::string("123456") : p.password; // código padrão da CC2 sem PIN
        m_status = json::object();
        if (m_sn.empty())
            for (const auto& [sn, js] : m_seen)
                if (json::parse(js, nullptr, false).value("dev_ip", "") == p.host)
                    m_sn = sn;
    }
    if (m_sn.empty() || m_ip.empty()) {
        dispatch_connect(ConnectStatusFailed, "Informe IP e número de série da impressora.");
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    Filuna::MqttClient::Options o;
    o.host      = m_ip;
    o.port      = p.port.empty() ? m_ports.mqtt : static_cast<unsigned short>(std::stoi(p.port));
    o.tls       = false;
    o.client_id = m_client_id;
    o.username  = p.username.empty() ? std::string("elegoo") : p.username;
    o.password  = m_code;
    o.keepalive = std::chrono::seconds(30);
    const std::string request_id = m_client_id; // o SDK usa um id próprio; qualquer um serve à impressora
    auto        c = std::make_unique<Filuna::MqttClient>();
    std::string err;
    const std::string base = "elegoo/" + m_sn + "/";
    const bool ok = c->connect(o, {base + m_client_id + "/api_response", base + "api_status", base + request_id + "/register_response"},
                               [this](const std::string& t, const std::string& pl) { on_mqtt(t, pl); },
                               [this](const std::string& why) { dispatch_connect(ConnectStatusLost, why); }, err);
    if (!ok) {
        dispatch_connect(ConnectStatusFailed, err);
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    c->publish(base + "api_register", json{{"client_id", m_client_id}, {"request_id", request_id}}.dump());
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        m_mqtt = std::move(c);
        if (m_selected.empty())
            m_selected = m_sn;
    }
    request(1001, json::object()); // atributos (negociação de versão)
    request(1002, json::object()); // estado
    request(2005, json::object()); // canvas (cores)
    dispatch_connect(ConnectStatusOk, "");
    if (m_on_connected)
        m_on_connected(base + "api_status");
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaElegooCC2Agent::disconnect_printer()
{
    std::unique_ptr<Filuna::MqttClient> c;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        c = std::move(m_mqtt);
    }
    if (c)
        c->close();
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaElegooCC2Agent::send_message_to_printer(std::string, std::string json_str, int, int)
{
    json cmd = json::parse(json_str, nullptr, false);
    if (!cmd.is_object())
        return BAMBU_NETWORK_ERR_INVALID_HANDLE;
    int id;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        id = m_next_id;
    }
    json r = to_cc2_request(cmd, id);
    if (r.empty())
        return ORCA_NETWORK_ERR_CMD_NOT_SUPPORTED;
    if (!request(r["method"].get<int>(), r["params"]))
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    if (r["method"].get<int>() == 1002)
        request(2005, json::object());
    return BAMBU_NETWORK_SUCCESS;
}

bool FilunaElegooCC2Agent::start_discovery(bool start, bool)
{
    if (!start) {
        m_discovering = false;
        if (m_discovery.joinable())
            m_discovery.join();
        return true;
    }
    if (m_discovering.exchange(true))
        return true;
    const unsigned short port = m_ports.discovery;
    m_discovery = std::thread([this, port] {
        using udp = net::ip::udp;
        try {
            net::io_context ioc;
            udp::socket     s(ioc, udp::endpoint(udp::v4(), 0));
            s.set_option(net::socket_base::broadcast(true));
            s.non_blocking(true);
            const std::string probe = R"({"id":0,"method":7000})";
            auto              last  = std::chrono::steady_clock::time_point{};
            char              buf[2048];
            while (m_discovering) {
                if (std::chrono::steady_clock::now() - last > std::chrono::seconds(2)) {
                    boost::system::error_code ec;
                    s.send_to(net::buffer(probe), udp::endpoint(net::ip::address_v4::broadcast(), port), 0, ec);
                    s.send_to(net::buffer(probe), udp::endpoint(net::ip::address_v4::loopback(), port), 0, ec);
                    last = std::chrono::steady_clock::now();
                }
                udp::endpoint             from;
                boost::system::error_code ec;
                const std::size_t         n = s.receive_from(net::buffer(buf), from, 0, ec);
                if (ec == net::error::would_block) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                if (ec)
                    continue;
                const std::string js = discovery_reply_to_json(std::string(buf, n), from.address().to_string());
                if (js.empty())
                    continue;
                OnMsgArrivedFn fn;
                {
                    std::lock_guard<std::recursive_mutex> l(m_mutex);
                    m_seen[json::parse(js).value("dev_id", "")] = js;
                    fn = m_on_ssdp;
                }
                if (fn)
                    fn(js);
            }
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(warning) << "FilunaElegooCC2Agent: descoberta parou: " << e.what();
        }
    });
    return true;
}

int FilunaElegooCC2Agent::bind_detect(std::string dev_ip, std::string, detectResult& d)
{
    std::lock_guard<std::recursive_mutex> l(m_mutex);
    for (const auto& [sn, js] : m_seen) {
        json j = json::parse(js, nullptr, false);
        if (j.value("dev_ip", "") == dev_ip) {
            d.dev_id = sn, d.model_id = j.value("dev_type", ""), d.dev_name = j.value("dev_name", sn);
            d.connect_type = "lan", d.bind_state = "free", d.result_msg = "ok";
            return BAMBU_NETWORK_SUCCESS;
        }
    }
    d.result_msg = "impressora não respondeu à descoberta";
    return BAMBU_NETWORK_ERR_CONNECT_FAILED;
}

std::string FilunaElegooCC2Agent::get_user_selected_machine() { std::lock_guard<std::recursive_mutex> l(m_mutex); return m_selected; }
int FilunaElegooCC2Agent::set_user_selected_machine(std::string d) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_selected = std::move(d); return 0; }

bool FilunaElegooCC2Agent::upload_http(const std::string& local, const std::string& remote, OnUpdateStatusFn update_fn,
                                       WasCancelledFn cancel_fn, std::string& error)
{
    std::ifstream in(local, std::ios::binary);
    if (!in) {
        error = "não foi possível abrir " + local;
        return false;
    }
    in.seekg(0, std::ios::end);
    const std::uint64_t total = std::uint64_t(in.tellg());
    in.seekg(0);
    const std::string md5 = md5_file(local);
    std::string ip, code;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        ip = m_ip, code = m_code;
    }
    try {
        net::io_context         ioc;
        net::ip::tcp::resolver  res(ioc);
        const auto              ends = res.resolve(ip, std::to_string(m_ports.http));
        const std::size_t       chunk = 1024 * 1024; // máximo aceito pela CC2 (SDK Elegoo-Link)
        std::vector<char>       buf(chunk);
        std::uint64_t           off = 0;
        int                     last = -1;
        while (off < total || total == 0) {
            in.read(buf.data(), std::streamsize(chunk));
            const std::size_t n = std::size_t(in.gcount());
            beast::tcp_stream s(ioc);
            s.expires_after(std::chrono::seconds(180));
            s.connect(ends);
            http::request<http::string_body> req{http::verb::put, "/upload", 11};
            req.set(http::field::host, ip);
            req.set(http::field::content_type, "application/octet-stream");
            req.set("Content-Range", "bytes " + std::to_string(off) + "-" + std::to_string(off + n - 1) + "/" + std::to_string(total));
            req.set("X-File-Name", remote);
            req.set("X-File-MD5", md5);
            req.set("X-Token", code);
            req.body().assign(buf.data(), n);
            req.prepare_payload();
            http::write(s, req);
            beast::flat_buffer                b;
            http::response<http::string_body> resp;
            http::read(s, b, resp);
            beast::error_code ec;
            s.socket().shutdown(net::ip::tcp::socket::shutdown_both, ec);
            json r = json::parse(resp.body(), nullptr, false);
            if (resp.result_int() != 200 || (r.is_object() && r.value("error_code", 0) != 0)) {
                error = "a impressora recusou o pedaço do arquivo (HTTP " + std::to_string(resp.result_int()) + "): " + resp.body();
                return false;
            }
            off += n;
            if (cancel_fn && cancel_fn()) {
                error = "envio cancelado";
                return false;
            }
            const int pct = total ? int(off * 100 / total) : 100;
            if (update_fn && pct != last)
                update_fn(PrintingStageUpload, last = pct, "");
            if (total == 0)
                break;
        }
    } catch (const std::exception& e) {
        error = std::string("falha no envio HTTP: ") + e.what();
        return false;
    }
    return true;
}

int FilunaElegooCC2Agent::start_local_print(PrintParams p, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn)
{
    const std::string remote = p.dst_file.empty() ? boost::filesystem::path(p.filename).filename().string() : p.dst_file;
    std::string       err;
    if (update_fn)
        update_fn(PrintingStageCreate, 0, "");
    if (!upload_http(p.filename, remote, update_fn, cancel_fn, err)) {
        if (update_fn)
            update_fn(PrintingStageERROR, BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED, err);
        return BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED;
    }
    json cfg{{"delay_video", p.task_record_timelapse}, {"printer_check", p.task_bed_leveling}, {"print_layout", "A"},
             {"bedlevel_force", false}};
    // ams_mapping do Slicer: posição = filamento do projeto, valor = bandeja (-1 = sem bandeja)
    json m = json::parse(p.ams_mapping.empty() ? std::string("[]") : p.ams_mapping, nullptr, false);
    json slot_map = json::array();
    if (m.is_array())
        for (std::size_t i = 0; i < m.size(); ++i)
            if (m[i].is_number_integer() && m[i].get<int>() >= 0)
                slot_map.push_back({{"t", int(i)}, {"canvas_id", m[i].get<int>() / 4}, {"tray_id", m[i].get<int>() % 4}});
    if (!slot_map.empty())
        cfg["slot_map"] = slot_map;
    if (update_fn)
        update_fn(PrintingStageSending, 0, "");
    if (!request(1020, json{{"filename", remote}, {"storage_media", "local"}, {"config", cfg}})) {
        if (update_fn)
            update_fn(PrintingStageERROR, BAMBU_NETWORK_ERR_CONNECT_FAILED, "impressora desconectada");
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    if (update_fn)
        update_fn(PrintingStageFinished, 0, "");
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaElegooCC2Agent::start_print(PrintParams p, OnUpdateStatusFn u, WasCancelledFn c, OnWaitFn) { return start_local_print(std::move(p), std::move(u), std::move(c)); }
int FilunaElegooCC2Agent::start_local_print_with_record(PrintParams p, OnUpdateStatusFn u, WasCancelledFn c, OnWaitFn) { return start_local_print(std::move(p), std::move(u), std::move(c)); }

int FilunaElegooCC2Agent::start_send_gcode_to_sdcard(PrintParams p, OnUpdateStatusFn u, WasCancelledFn c, OnWaitFn)
{
    std::string err;
    const std::string remote = p.dst_file.empty() ? boost::filesystem::path(p.filename).filename().string() : p.dst_file;
    const bool ok = upload_http(p.filename, remote, u, c, err);
    if (u)
        u(ok ? PrintingStageFinished : PrintingStageERROR, ok ? 0 : BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED, err);
    return ok ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED;
}

int FilunaElegooCC2Agent::start_sdcard_print(PrintParams p, OnUpdateStatusFn u, WasCancelledFn)
{
    const bool ok = request(1020, json{{"filename", p.dst_file.empty() ? p.ftp_file : p.dst_file}, {"storage_media", "local"}, {"config", json::object()}});
    if (u)
        u(ok ? PrintingStageFinished : PrintingStageERROR, 0, "");
    return ok ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_CONNECT_FAILED;
}

int FilunaElegooCC2Agent::set_on_ssdp_msg_fn(OnMsgArrivedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_ssdp = std::move(fn); return 0; }
int FilunaElegooCC2Agent::set_on_printer_connected_fn(OnPrinterConnectedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_connected = std::move(fn); return 0; }
int FilunaElegooCC2Agent::set_on_message_fn(OnMessageFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_message = std::move(fn); return 0; }
int FilunaElegooCC2Agent::set_on_local_connect_fn(OnLocalConnectedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_local_connect = std::move(fn); return 0; }
int FilunaElegooCC2Agent::set_on_local_message_fn(OnMessageFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_local_message = std::move(fn); return 0; }
int FilunaElegooCC2Agent::set_queue_on_main_fn(QueueOnMainFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_queue_on_main = std::move(fn); return 0; }

} // namespace Slic3r
