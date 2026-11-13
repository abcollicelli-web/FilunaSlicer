#include "FilunaFlashforgeAgent.hpp"

#include <array>
#include <chrono>
#include <fstream>
#include <sstream>
#include <vector>

#include <boost/algorithm/string/trim.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/beast/http.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>

namespace Slic3r {

namespace net   = boost::asio;
namespace beast = boost::beast;
namespace http  = boost::beast::http;
using json      = nlohmann::json;

namespace {

const std::array<unsigned char, 20> DISCOVERY_MSG = {0x77, 0x77, 0x77, 0x2e, 0x75, 0x73, 0x72, 0x22, 0x65, 0x36,
                                                     0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

std::string cstr(const char* d, std::size_t n)
{
    std::string s(d, d + n);
    if (auto p = s.find('\0'); p != std::string::npos)
        s.resize(p);
    boost::trim(s);
    return s;
}

std::string b64(const std::string& s)
{
    std::string out(beast::detail::base64::encoded_size(s.size()), '\0');
    out.resize(beast::detail::base64::encode(out.data(), s.data(), s.size()));
    return out;
}

std::string color_rgba(std::string c)
{
    if (!c.empty() && c[0] == '#')
        c.erase(0, 1);
    for (auto& ch : c)
        ch = char(::toupper(ch));
    return c.size() == 6 ? c + "FF" : (c.size() == 8 ? c : std::string("FFFFFFFF"));
}

} // namespace

AgentInfo FilunaFlashforgeAgent::get_agent_info_static()
{
    return AgentInfo{FILUNA_FLASHFORGE_AGENT_ID, "Flashforge (API local aberta)", "1.0.0",
                     "API HTTP local 8898 das Flashforge AD5M/AD5X/Creator 5, sem biblioteca fechada"};
}

FilunaFlashforgeAgent::FilunaFlashforgeAgent(std::string) {}

FilunaFlashforgeAgent::~FilunaFlashforgeAgent()
{
    start_discovery(false, false);
    disconnect_printer();
}

bool FilunaFlashforgeAgent::parse_discovery(const std::string& d, std::string& name, std::string& serial)
{
    if (d.size() < 0xC4)
        return false;
    name   = cstr(d.data(), 32);
    serial = cstr(d.data() + 0x92, 32);
    return !(name.empty() && serial.empty());
}

json FilunaFlashforgeAgent::to_push_status(const json& reply)
{
    const json d = reply.contains("detail") ? reply["detail"] : reply;
    json       p;
    p["command"]     = "push_status";
    p["sequence_id"] = "0";
    const std::string st = d.value("status", std::string());
    std::string       gs = "IDLE";
    if (st == "printing" || st == "busy" || st == "heating")
        gs = "RUNNING";
    else if (st == "paused" || st == "pausing")
        gs = "PAUSE";
    else if (st == "completed")
        gs = "FINISH";
    else if (st == "error")
        gs = "FAILED";
    else if (st == "cancel")
        gs = "FAILED";
    p["gcode_state"]          = gs;
    p["mc_percent"]           = int(d.value("printProgress", 0.0) * 100.0 + 0.5);
    p["mc_remaining_time"]    = int(d.value("estimatedTime", 0.0) / 60);
    p["subtask_name"]         = d.value("printFileName", std::string());
    p["gcode_file"]           = d.value("printFileName", std::string());
    p["layer_num"]            = d.value("printLayer", 0);
    p["total_layer_num"]      = d.value("targetPrintLayer", 0);
    p["nozzle_temper"]        = d.value("rightTemp", d.value("nozzleTemp", 0.0));
    p["nozzle_target_temper"] = d.value("rightTargetTemp", d.value("nozzleTargetTemp", 0.0));
    p["bed_temper"]           = d.value("platTemp", 0.0);
    p["bed_target_temper"]    = d.value("platTargetTemp", 0.0);
    if (d.value("hasMatlStation", false) && d.contains("matlStationInfo")) {
        json trays = json::array();
        for (const auto& s : d["matlStationInfo"].value("slotInfos", json::array())) {
            json t;
            t["id"]         = std::to_string(s.value("slotId", 1) - 1);
            t["tray_type"]  = s.value("hasFilament", false) ? s.value("materialName", std::string()) : std::string();
            t["tray_color"] = color_rgba(s.value("materialColor", std::string()));
            trays.push_back(t);
        }
        p["ams"] = {{"ams", json::array({{{"id", "0"}, {"tray", trays}}})}, {"ams_exist_bits", "1"},
                    {"tray_now", std::to_string(d["matlStationInfo"].value("currentSlot", 0) - 1)}};
    }
    if (d.contains("cameraStreamUrl"))
        p["ipcam"] = {{"rtsp_url", d.value("cameraStreamUrl", std::string())}};
    return json{{"print", p}};
}

std::string FilunaFlashforgeAgent::action_for(const json& cmd)
{
    const std::string c = cmd.contains("print") ? cmd["print"].value("command", std::string()) : std::string();
    if (c == "pause")
        return "pause";
    if (c == "resume")
        return "continue";
    if (c == "stop")
        return "cancel";
    return {};
}

bool FilunaFlashforgeAgent::post_json(const std::string& path, const json& body, json& reply, std::string& error)
{
    std::string ip;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        ip = m_ip;
    }
    try {
        net::io_context        ioc;
        net::ip::tcp::resolver res(ioc);
        beast::tcp_stream      s(ioc);
        s.expires_after(std::chrono::seconds(10));
        s.connect(res.resolve(ip, std::to_string(m_ports.http)));
        http::request<http::string_body> req{http::verb::post, "/" + path, 11};
        req.set(http::field::host, ip);
        req.set(http::field::content_type, "application/json");
        req.body() = body.dump();
        req.prepare_payload();
        http::write(s, req);
        beast::flat_buffer                b;
        http::response<http::string_body> r;
        http::read(s, b, r);
        reply = json::parse(r.body(), nullptr, false);
        if (r.result_int() != 200 || !reply.is_object() || reply.value("code", 0) != 0) {
            error = "a impressora recusou /" + path + ": " + r.body();
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        error = std::string("falha HTTP /") + path + ": " + e.what();
        return false;
    }
}

void FilunaFlashforgeAgent::dispatch(const std::string& payload)
{
    OnMessageFn   local, cloud;
    QueueOnMainFn q;
    std::string   dev;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        local = m_on_local_message, cloud = m_on_message, q = m_queue_on_main, dev = m_sn;
    }
    auto call = [local, cloud, dev, payload] {
        if (local)
            local(dev, payload);
        else if (cloud)
            cloud(dev, payload);
    };
    q ? q(call) : call();
}

void FilunaFlashforgeAgent::dispatch_connect(int state, const std::string& msg)
{
    OnLocalConnectedFn fn;
    QueueOnMainFn      q;
    std::string        dev;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        fn = m_on_local_connect, q = m_queue_on_main, dev = m_sn;
    }
    if (!fn)
        return;
    auto call = [fn, state, dev, msg] { fn(state, dev, msg); };
    q ? q(call) : call();
}

void FilunaFlashforgeAgent::poll_loop()
{
    int falhas = 0;
    while (m_polling) {
        json        r;
        std::string err, sn, code;
        {
            std::lock_guard<std::recursive_mutex> l(m_mutex);
            sn = m_sn, code = m_code;
        }
        if (post_json("detail", json{{"serialNumber", sn}, {"checkCode", code}}, r, err)) {
            falhas = 0;
            dispatch(to_push_status(r).dump());
        } else if (++falhas == 5) {
            dispatch_connect(ConnectStatusLost, err);
        }
        for (int i = 0; i < m_ports.poll_ms / 50 && m_polling; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int FilunaFlashforgeAgent::send_message(std::string d, std::string j, int q, int f) { return send_message_to_printer(std::move(d), std::move(j), q, f); }

int FilunaFlashforgeAgent::connect_printer(const PrinterConnectionParams& p)
{
    disconnect_printer();
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        m_sn = p.dev_id, m_ip = p.host, m_code = p.password;
        if (m_sn.empty())
            for (const auto& [sn, js] : m_seen)
                if (json::parse(js, nullptr, false).value("dev_ip", "") == p.host)
                    m_sn = sn;
        if (!p.port.empty())
            m_ports.http = static_cast<unsigned short>(std::stoi(p.port));
    }
    json        r;
    std::string err;
    if (m_sn.empty() || !post_json("detail", json{{"serialNumber", m_sn}, {"checkCode", m_code}}, r, err)) {
        dispatch_connect(ConnectStatusFailed, err.empty() ? std::string("Informe IP, número de série e código da impressora.") : err);
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        if (m_selected.empty())
            m_selected = m_sn;
    }
    dispatch_connect(ConnectStatusOk, "");
    dispatch(to_push_status(r).dump());
    if (m_on_connected)
        m_on_connected("detail");
    m_polling = true;
    m_poll    = std::thread([this] { poll_loop(); });
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaFlashforgeAgent::disconnect_printer()
{
    m_polling = false;
    if (m_poll.joinable())
        m_poll.join();
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaFlashforgeAgent::send_message_to_printer(std::string, std::string json_str, int, int)
{
    json cmd = json::parse(json_str, nullptr, false);
    if (!cmd.is_object())
        return BAMBU_NETWORK_ERR_INVALID_HANDLE;
    if (cmd.contains("pushing"))
        return BAMBU_NETWORK_SUCCESS; // o estado já chega pela consulta periódica
    const std::string action = action_for(cmd);
    if (action.empty())
        return ORCA_NETWORK_ERR_CMD_NOT_SUPPORTED;
    std::string sn, code;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        sn = m_sn, code = m_code;
    }
    json        r;
    std::string err;
    const json  body{{"serialNumber", sn}, {"checkCode", code},
                     {"payload", {{"cmd", "jobCtl_cmd"}, {"args", {{"jobID", ""}, {"action", action}}}}}};
    return post_json("control", body, r, err) ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_CONNECT_FAILED;
}

bool FilunaFlashforgeAgent::upload(const PrintParams& p, bool print_now, OnUpdateStatusFn update_fn, std::string& error)
{
    std::ifstream in(p.filename, std::ios::binary);
    if (!in) {
        error = "não foi possível abrir " + p.filename;
        return false;
    }
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const std::string name = p.dst_file.empty() ? boost::filesystem::path(p.filename).filename().string() : p.dst_file;
    json maps = json::array();
    json m    = json::parse(p.ams_mapping.empty() ? std::string("[]") : p.ams_mapping, nullptr, false);
    if (m.is_array())
        for (std::size_t i = 0; i < m.size(); ++i)
            if (m[i].is_number_integer() && m[i].get<int>() >= 0)
                maps.push_back({{"toolId", int(i)}, {"slotId", m[i].get<int>() + 1}});
    std::string sn, code, ip;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        sn = m_sn, code = m_code, ip = m_ip;
    }
    const std::string boundary = "----FilunaFlashforge7d3a";
    std::string       body     = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"gcodeFile\"; filename=\"" + name +
                       "\"\r\nContent-Type: application/octet-stream\r\n\r\n" + data + "\r\n--" + boundary + "--\r\n";
    try {
        net::io_context        ioc;
        net::ip::tcp::resolver res(ioc);
        beast::tcp_stream      s(ioc);
        s.expires_after(std::chrono::seconds(300));
        s.connect(res.resolve(ip, std::to_string(m_ports.http)));
        http::request<http::string_body> req{http::verb::post, "/uploadGcode", 11};
        req.set(http::field::host, ip);
        req.set(http::field::content_type, "multipart/form-data; boundary=" + boundary);
        req.set("serialNumber", sn);
        req.set("checkCode", code);
        req.set("fileSize", std::to_string(data.size()));
        req.set("printNow", print_now ? "true" : "false");
        req.set("levelingBeforePrint", p.task_bed_leveling ? "true" : "false");
        req.set("flowCalibration", "false");
        req.set("firstLayerInspection", "false");
        req.set("timeLapseVideo", p.task_record_timelapse ? "true" : "false");
        req.set("useMatlStation", (p.task_use_ams && !maps.empty()) ? "true" : "false");
        req.set("gcodeToolCnt", std::to_string(std::max<std::size_t>(1, maps.size())));
        req.set("materialMappings", b64(maps.dump()));
        req.body() = std::move(body);
        req.prepare_payload();
        if (update_fn)
            update_fn(PrintingStageUpload, 0, "");
        http::write(s, req);
        if (update_fn)
            update_fn(PrintingStageUpload, 100, "");
        beast::flat_buffer                b;
        http::response<http::string_body> r;
        http::read(s, b, r);
        json j = json::parse(r.body(), nullptr, false);
        if (r.result_int() != 200 || !j.is_object() || j.value("code", 0) != 0) {
            error = "a impressora recusou o envio: " + r.body();
            return false;
        }
    } catch (const std::exception& e) {
        error = std::string("falha no envio: ") + e.what();
        return false;
    }
    return true;
}

int FilunaFlashforgeAgent::start_local_print(PrintParams p, OnUpdateStatusFn u, WasCancelledFn)
{
    std::string err;
    if (u)
        u(PrintingStageCreate, 0, "");
    if (!upload(p, true, u, err)) {
        if (u)
            u(PrintingStageERROR, BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED, err);
        return BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED;
    }
    if (u)
        u(PrintingStageFinished, 0, "");
    return BAMBU_NETWORK_SUCCESS;
}
int FilunaFlashforgeAgent::start_print(PrintParams p, OnUpdateStatusFn u, WasCancelledFn c, OnWaitFn) { return start_local_print(std::move(p), std::move(u), std::move(c)); }
int FilunaFlashforgeAgent::start_local_print_with_record(PrintParams p, OnUpdateStatusFn u, WasCancelledFn c, OnWaitFn) { return start_local_print(std::move(p), std::move(u), std::move(c)); }
int FilunaFlashforgeAgent::start_send_gcode_to_sdcard(PrintParams p, OnUpdateStatusFn u, WasCancelledFn, OnWaitFn)
{
    std::string err;
    const bool  ok = upload(p, false, u, err);
    if (u)
        u(ok ? PrintingStageFinished : PrintingStageERROR, ok ? 0 : BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED, err);
    return ok ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED;
}
int FilunaFlashforgeAgent::start_sdcard_print(PrintParams, OnUpdateStatusFn u, WasCancelledFn)
{
    if (u)
        u(PrintingStageERROR, ORCA_NETWORK_ERR_CMD_NOT_SUPPORTED, "imprimir arquivo já guardado não é exposto pela API local");
    return ORCA_NETWORK_ERR_CMD_NOT_SUPPORTED;
}

bool FilunaFlashforgeAgent::start_discovery(bool start, bool)
{
    if (!start) {
        m_discovering = false;
        if (m_discovery.joinable())
            m_discovery.join();
        return true;
    }
    if (m_discovering.exchange(true))
        return true;
    m_discovery = std::thread([this] {
        using udp = net::ip::udp;
        try {
            net::io_context ioc;
            udp::socket     s(ioc);
            s.open(udp::v4());
            s.set_option(net::socket_base::reuse_address(true));
            s.set_option(net::socket_base::broadcast(true));
            s.bind(udp::endpoint(udp::v4(), m_ports.listen));
            s.non_blocking(true);
            auto last = std::chrono::steady_clock::time_point{};
            char buf[1024];
            while (m_discovering) {
                if (std::chrono::steady_clock::now() - last > std::chrono::seconds(2)) {
                    boost::system::error_code ec;
                    for (auto a : {net::ip::address_v4::broadcast(), net::ip::address_v4::loopback()})
                        s.send_to(net::buffer(DISCOVERY_MSG), udp::endpoint(a, m_ports.discovery), 0, ec);
                    last = std::chrono::steady_clock::now();
                }
                udp::endpoint             from;
                boost::system::error_code ec;
                const std::size_t         n = s.receive_from(net::buffer(buf), from, 0, ec);
                if (ec == net::error::would_block) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                std::string name, serial;
                if (ec || !parse_discovery(std::string(buf, n), name, serial))
                    continue;
                json j{{"dev_id", serial}, {"dev_ip", from.address().to_string()}, {"dev_name", name}, {"dev_type", name},
                       {"dev_signal", "0"}, {"connect_type", "lan"}, {"bind_state", "free"}, {"sec_link", "secure"}, {"ssdp_version", "v1"}};
                OnMsgArrivedFn fn;
                {
                    std::lock_guard<std::recursive_mutex> l(m_mutex);
                    m_seen[serial] = j.dump();
                    fn             = m_on_ssdp;
                }
                if (fn)
                    fn(j.dump());
            }
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(warning) << "FilunaFlashforgeAgent: descoberta parou: " << e.what();
        }
    });
    return true;
}

int FilunaFlashforgeAgent::bind_detect(std::string dev_ip, std::string, detectResult& d)
{
    std::lock_guard<std::recursive_mutex> l(m_mutex);
    for (const auto& [sn, js] : m_seen) {
        json j = json::parse(js, nullptr, false);
        if (j.value("dev_ip", "") == dev_ip) {
            d.dev_id = sn, d.dev_name = j.value("dev_name", sn), d.model_id = j.value("dev_type", "");
            d.connect_type = "lan", d.bind_state = "free", d.result_msg = "ok";
            return BAMBU_NETWORK_SUCCESS;
        }
    }
    d.result_msg = "impressora não respondeu à descoberta";
    return BAMBU_NETWORK_ERR_CONNECT_FAILED;
}

std::string FilunaFlashforgeAgent::get_user_selected_machine() { std::lock_guard<std::recursive_mutex> l(m_mutex); return m_selected; }
int FilunaFlashforgeAgent::set_user_selected_machine(std::string d) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_selected = std::move(d); return 0; }
int FilunaFlashforgeAgent::set_on_ssdp_msg_fn(OnMsgArrivedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_ssdp = std::move(fn); return 0; }
int FilunaFlashforgeAgent::set_on_printer_connected_fn(OnPrinterConnectedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_connected = std::move(fn); return 0; }
int FilunaFlashforgeAgent::set_on_message_fn(OnMessageFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_message = std::move(fn); return 0; }
int FilunaFlashforgeAgent::set_on_local_connect_fn(OnLocalConnectedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_local_connect = std::move(fn); return 0; }
int FilunaFlashforgeAgent::set_on_local_message_fn(OnMessageFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_local_message = std::move(fn); return 0; }
int FilunaFlashforgeAgent::set_queue_on_main_fn(QueueOnMainFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_queue_on_main = std::move(fn); return 0; }

} // namespace Slic3r
