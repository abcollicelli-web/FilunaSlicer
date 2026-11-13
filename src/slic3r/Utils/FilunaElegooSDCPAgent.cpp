#include "FilunaElegooSDCPAgent.hpp"

#include <chrono>
#include <deque>
#include <fstream>
#include <random>
#include <vector>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/post.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>
#include <openssl/evp.h>

namespace Slic3r {

namespace net       = boost::asio;
namespace beast     = boost::beast;
namespace http      = boost::beast::http;
namespace websocket = boost::beast::websocket;
using json          = nlohmann::json;

namespace {

std::string hexid(int bytes)
{
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    static const char*                  hex = "0123456789abcdef";
    std::string                         s;
    for (int i = 0; i < bytes * 2; ++i)
        s.push_back(hex[rng() % 16]);
    return s;
}

std::string md5_hex(const std::string& data)
{
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int  n = 0;
    EVP_Digest(data.data(), data.size(), md, &n, EVP_md5(), nullptr);
    static const char* hex = "0123456789abcdef";
    std::string        o;
    for (unsigned i = 0; i < n; ++i)
        o.push_back(hex[md[i] >> 4]), o.push_back(hex[md[i] & 15]);
    return o;
}

} // namespace

struct FilunaElegooSDCPAgent::Ws
{
    net::io_context                       ioc;
    websocket::stream<beast::tcp_stream>  ws{ioc};
    beast::flat_buffer                    buf;
    std::deque<std::string>               tx;
    std::function<void(const std::string&)> on_text;
    std::function<void(const std::string&)> on_closed;
    bool                                  closing = false;

    void read()
    {
        ws.async_read(buf, [this](beast::error_code ec, std::size_t) {
            if (ec) {
                if (!closing && on_closed)
                    on_closed(ec.message());
                return;
            }
            const std::string t = beast::buffers_to_string(buf.data());
            buf.consume(buf.size());
            if (on_text)
                on_text(t);
            read();
        });
    }
    void write_next()
    {
        if (tx.empty())
            return;
        ws.async_write(net::buffer(tx.front()), [this](beast::error_code ec, std::size_t) {
            if (ec)
                return;
            tx.pop_front();
            write_next();
        });
    }
    void send(std::string s)
    {
        const bool idle = tx.empty();
        tx.push_back(std::move(s));
        if (idle)
            write_next();
    }
};

AgentInfo FilunaElegooSDCPAgent::get_agent_info_static()
{
    return AgentInfo{FILUNA_ELEGOO_SDCP_AGENT_ID, "Elegoo Centauri Carbon (SDCP)", "1.0.0", "Protocolo aberto SDCP v3: UDP 3000, WebSocket 3030, HTTP de envio"};
}

FilunaElegooSDCPAgent::FilunaElegooSDCPAgent(std::string) {}
FilunaElegooSDCPAgent::~FilunaElegooSDCPAgent()
{
    start_discovery(false, false);
    disconnect_printer();
}

json FilunaElegooSDCPAgent::to_push_status(const json& msg)
{
    const json st = msg.contains("Status") ? msg["Status"] : msg;
    const json pi = st.value("PrintInfo", json::object());
    const int  ps = pi.value("Status", 0);
    bool       ativo = false;
    for (const auto& c : st.value("CurrentStatus", json::array()))
        ativo |= c.is_number_integer() && c.get<int>() == 1;
    std::string gs = "IDLE";
    if (ps == 5 || ps == 6)
        gs = "PAUSE";
    else if (ps == 7 || ps == 8)
        gs = "FAILED";
    else if (ps == 9)
        gs = "FINISH";
    else if (ps != 0 || ativo)
        gs = "RUNNING";
    json p{{"command", "push_status"}, {"sequence_id", "0"}, {"gcode_state", gs},
           {"mc_percent", pi.value("Progress", 0)}, {"layer_num", pi.value("CurrentLayer", 0)}, {"total_layer_num", pi.value("TotalLayer", 0)},
           {"subtask_name", pi.value("Filename", std::string())}, {"gcode_file", pi.value("Filename", std::string())},
           {"mc_remaining_time", std::max(0, (pi.value("TotalTicks", 0) - pi.value("CurrentTicks", 0)) / 60000)},
           {"nozzle_temper", st.value("TempOfNozzle", 0.0)}, {"nozzle_target_temper", st.value("TempTargetNozzle", 0.0)},
           {"bed_temper", st.value("TempOfHotbed", 0.0)}, {"bed_target_temper", st.value("TempTargetHotbed", 0.0)}};
    return json{{"print", p}};
}

std::string FilunaElegooSDCPAgent::discovery_to_json(const std::string& reply, const std::string& ip)
{
    json r = json::parse(reply, nullptr, false);
    if (!r.is_object() || !r.contains("Data") || !r["Data"].is_object())
        return {};
    const json& d = r["Data"];
    if (d.value("MainboardID", std::string()).empty())
        return {};
    json j{{"dev_id", d.value("MainboardID", std::string())}, {"dev_ip", d.value("MainboardIP", ip)},
           {"dev_name", d.value("Name", d.value("MachineName", std::string("Elegoo")))}, {"dev_type", d.value("MachineName", std::string())},
           {"dev_signal", "0"}, {"connect_type", "lan"}, {"bind_state", "free"}, {"sec_link", "secure"},
           {"ssdp_version", d.value("ProtocolVersion", std::string("V3.0.0"))}};
    return j.dump();
}

void FilunaElegooSDCPAgent::dispatch(const std::string& payload)
{
    OnMessageFn   local, cloud;
    QueueOnMainFn q;
    std::string   dev;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        local = m_on_local_message, cloud = m_on_message, q = m_queue_on_main, dev = m_mainboard;
    }
    auto c = [local, cloud, dev, payload] {
        if (local)
            local(dev, payload);
        else if (cloud)
            cloud(dev, payload);
    };
    q ? q(c) : c();
}

void FilunaElegooSDCPAgent::dispatch_connect(int state, const std::string& msg)
{
    OnLocalConnectedFn fn;
    QueueOnMainFn      q;
    std::string        dev;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        fn = m_on_local_connect, q = m_queue_on_main, dev = m_mainboard;
    }
    if (!fn)
        return;
    auto c = [fn, state, dev, msg] { fn(state, dev, msg); };
    q ? q(c) : c();
}

bool FilunaElegooSDCPAgent::send_cmd(int cmd, const json& data)
{
    std::lock_guard<std::recursive_mutex> l(m_mutex);
    if (!m_ws || !m_running)
        return false;
    const auto ts = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    json       req{{"Id", hexid(16)},
                   {"Data", {{"Cmd", cmd}, {"Data", data}, {"RequestID", hexid(16)}, {"MainboardID", m_mainboard}, {"TimeStamp", ts}, {"From", 0}}},
                   {"Topic", "sdcp/request/" + m_mainboard}};
    net::post(m_ws->ioc, [w = m_ws.get(), s = req.dump()]() mutable { w->send(std::move(s)); });
    return true;
}

int FilunaElegooSDCPAgent::send_message(std::string d, std::string j, int q, int f) { return send_message_to_printer(std::move(d), std::move(j), q, f); }

int FilunaElegooSDCPAgent::connect_printer(const PrinterConnectionParams& p)
{
    disconnect_printer();
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        m_ip = p.host, m_mainboard = p.dev_id, m_status = json::object();
        if (m_mainboard.empty())
            for (const auto& [id, js] : m_seen)
                if (json::parse(js, nullptr, false).value("dev_ip", "") == p.host)
                    m_mainboard = id;
        if (!p.port.empty())
            m_ports.ws = m_ports.http = static_cast<unsigned short>(std::stoi(p.port));
    }
    if (m_mainboard.empty()) {
        dispatch_connect(ConnectStatusFailed, "MainboardID não descoberto: faça a busca na rede primeiro.");
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    auto w = std::make_unique<Ws>();
    try {
        net::ip::tcp::resolver res(w->ioc);
        beast::get_lowest_layer(w->ws).expires_after(std::chrono::seconds(10));
        beast::get_lowest_layer(w->ws).connect(res.resolve(m_ip, std::to_string(m_ports.ws)));
        beast::get_lowest_layer(w->ws).expires_never();
        w->ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
        w->ws.handshake(m_ip + ":" + std::to_string(m_ports.ws), "/websocket");
    } catch (const std::exception& e) {
        dispatch_connect(ConnectStatusFailed, std::string("WebSocket SDCP: ") + e.what());
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    w->on_text = [this](const std::string& t) {
        json j = json::parse(t, nullptr, false);
        if (!j.is_object())
            return;
        const std::string topic = j.value("Topic", std::string());
        if (topic.rfind("sdcp/status/", 0) == 0 && j.contains("Status")) {
            json snap;
            {
                std::lock_guard<std::recursive_mutex> l(m_mutex);
                m_status.merge_patch(j["Status"]);
                snap = m_status;
            }
            dispatch(to_push_status(snap).dump());
        }
    };
    w->on_closed = [this](const std::string& why) { dispatch_connect(ConnectStatusLost, why); };
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        m_ws = std::move(w);
        if (m_selected.empty())
            m_selected = m_mainboard;
    }
    m_running = true;
    m_ws->read();
    m_reader = std::thread([w = m_ws.get()] {
        try {
            w->ioc.run();
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(warning) << "FilunaElegooSDCPAgent: " << e.what();
        }
    });
    send_cmd(1, json::object()); // atributos
    send_cmd(0, json::object()); // estado
    dispatch_connect(ConnectStatusOk, "");
    if (m_on_connected)
        m_on_connected("sdcp/status/" + m_mainboard);
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaElegooSDCPAgent::disconnect_printer()
{
    std::unique_ptr<Ws> w;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        w = std::move(m_ws);
    }
    m_running = false;
    if (w) {
        net::post(w->ioc, [p = w.get()] {
            p->closing = true;
            beast::error_code ec;
            beast::get_lowest_layer(p->ws).socket().close(ec);
            p->ioc.stop();
        });
        if (m_reader.joinable())
            m_reader.join();
    }
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaElegooSDCPAgent::send_message_to_printer(std::string, std::string json_str, int, int)
{
    json cmd = json::parse(json_str, nullptr, false);
    if (!cmd.is_object())
        return BAMBU_NETWORK_ERR_INVALID_HANDLE;
    if (cmd.contains("pushing"))
        return send_cmd(0, json::object()) ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_CONNECT_FAILED;
    const std::string c = cmd.contains("print") ? cmd["print"].value("command", std::string()) : std::string();
    int               n = c == "pause" ? 129 : c == "stop" ? 130 : c == "resume" ? 131 : -1;
    if (n < 0)
        return ORCA_NETWORK_ERR_CMD_NOT_SUPPORTED;
    return send_cmd(n, json::object()) ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_CONNECT_FAILED;
}

bool FilunaElegooSDCPAgent::upload(const std::string& local, const std::string& remote, OnUpdateStatusFn u, std::string& error)
{
    std::ifstream in(local, std::ios::binary);
    if (!in) {
        error = "não foi possível abrir " + local;
        return false;
    }
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const std::string md5 = md5_hex(data), uuid = hexid(16);
    std::string       ip;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        ip = m_ip;
    }
    const std::size_t chunk = 1024 * 1024;
    int               last  = -1;
    try {
        net::io_context        ioc;
        net::ip::tcp::resolver res(ioc);
        const auto             ends = res.resolve(ip, std::to_string(m_ports.http));
        for (std::size_t off = 0; off < data.size() || (data.empty() && off == 0); off += chunk) {
            const std::string part = data.substr(off, chunk);
            const std::string b    = "----FilunaSDCP" + uuid;
            auto field = [&](const std::string& k, const std::string& v) {
                return "--" + b + "\r\nContent-Disposition: form-data; name=\"" + k + "\"\r\n\r\n" + v + "\r\n";
            };
            std::string body = field("S-File-MD5", md5) + field("Check", "1") + field("Offset", std::to_string(off)) + field("Uuid", uuid) +
                               field("TotalSize", std::to_string(data.size())) + "--" + b +
                               "\r\nContent-Disposition: form-data; name=\"File\"; filename=\"" + remote +
                               "\"\r\nContent-Type: application/octet-stream\r\n\r\n" + part + "\r\n--" + b + "--\r\n";
            beast::tcp_stream s(ioc);
            s.expires_after(std::chrono::seconds(120));
            s.connect(ends);
            http::request<http::string_body> req{http::verb::post, "/uploadFile/upload", 11};
            req.set(http::field::host, ip);
            req.set(http::field::content_type, "multipart/form-data; boundary=" + b);
            req.body() = std::move(body);
            req.prepare_payload();
            http::write(s, req);
            beast::flat_buffer                fb;
            http::response<http::string_body> r;
            http::read(s, fb, r);
            json j = json::parse(r.body(), nullptr, false);
            if (r.result_int() != 200 || !j.is_object() || !j.value("success", false)) {
                error = "a impressora recusou o pedaço do arquivo: " + r.body();
                return false;
            }
            const int pct = data.empty() ? 100 : int(std::min(data.size(), off + chunk) * 100 / data.size());
            if (u && pct != last)
                u(PrintingStageUpload, last = pct, "");
            if (data.empty())
                break;
        }
    } catch (const std::exception& e) {
        error = std::string("falha no envio: ") + e.what();
        return false;
    }
    return true;
}

int FilunaElegooSDCPAgent::start_local_print(PrintParams p, OnUpdateStatusFn u, WasCancelledFn)
{
    const std::string remote = p.dst_file.empty() ? boost::filesystem::path(p.filename).filename().string() : p.dst_file;
    std::string       err;
    if (u)
        u(PrintingStageCreate, 0, "");
    if (!upload(p.filename, remote, u, err)) {
        if (u)
            u(PrintingStageERROR, BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED, err);
        return BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED;
    }
    const bool ok = send_cmd(128, json{{"Filename", "/local/" + remote}, {"StartLayer", 0}, {"Calibration_switch", p.task_bed_leveling ? 1 : 0},
                                       {"PrintPlatformType", 0}, {"Tlp_Switch", p.task_record_timelapse ? 1 : 0}});
    if (u)
        u(ok ? PrintingStageFinished : PrintingStageERROR, ok ? 0 : BAMBU_NETWORK_ERR_CONNECT_FAILED, "");
    return ok ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_CONNECT_FAILED;
}
int FilunaElegooSDCPAgent::start_print(PrintParams p, OnUpdateStatusFn u, WasCancelledFn c, OnWaitFn) { return start_local_print(std::move(p), std::move(u), std::move(c)); }
int FilunaElegooSDCPAgent::start_local_print_with_record(PrintParams p, OnUpdateStatusFn u, WasCancelledFn c, OnWaitFn) { return start_local_print(std::move(p), std::move(u), std::move(c)); }
int FilunaElegooSDCPAgent::start_send_gcode_to_sdcard(PrintParams p, OnUpdateStatusFn u, WasCancelledFn, OnWaitFn)
{
    std::string err;
    const bool  ok = upload(p.filename, p.dst_file.empty() ? boost::filesystem::path(p.filename).filename().string() : p.dst_file, u, err);
    if (u)
        u(ok ? PrintingStageFinished : PrintingStageERROR, ok ? 0 : BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED, err);
    return ok ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED;
}
int FilunaElegooSDCPAgent::start_sdcard_print(PrintParams p, OnUpdateStatusFn u, WasCancelledFn)
{
    const bool ok = send_cmd(128, json{{"Filename", "/local/" + (p.dst_file.empty() ? p.ftp_file : p.dst_file)}, {"StartLayer", 0}});
    if (u)
        u(ok ? PrintingStageFinished : PrintingStageERROR, 0, "");
    return ok ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_CONNECT_FAILED;
}

bool FilunaElegooSDCPAgent::start_discovery(bool start, bool)
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
            udp::socket     s(ioc, udp::endpoint(udp::v4(), 0));
            s.set_option(net::socket_base::broadcast(true));
            s.non_blocking(true);
            auto last = std::chrono::steady_clock::time_point{};
            char buf[2048];
            while (m_discovering) {
                if (std::chrono::steady_clock::now() - last > std::chrono::seconds(2)) {
                    boost::system::error_code ec;
                    for (auto a : {net::ip::address_v4::broadcast(), net::ip::address_v4::loopback()})
                        s.send_to(net::buffer(std::string("M99999")), udp::endpoint(a, m_ports.discovery), 0, ec);
                    last = std::chrono::steady_clock::now();
                }
                udp::endpoint             from;
                boost::system::error_code ec;
                const std::size_t         n = s.receive_from(net::buffer(buf), from, 0, ec);
                if (ec == net::error::would_block) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                const std::string js = ec ? std::string() : discovery_to_json(std::string(buf, n), from.address().to_string());
                if (js.empty())
                    continue;
                OnMsgArrivedFn fn;
                {
                    std::lock_guard<std::recursive_mutex> l(m_mutex);
                    m_seen[json::parse(js).value("dev_id", "")] = js;
                    fn                                         = m_on_ssdp;
                }
                if (fn)
                    fn(js);
            }
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(warning) << "FilunaElegooSDCPAgent: descoberta parou: " << e.what();
        }
    });
    return true;
}

int FilunaElegooSDCPAgent::bind_detect(std::string dev_ip, std::string, detectResult& d)
{
    std::lock_guard<std::recursive_mutex> l(m_mutex);
    for (const auto& [id, js] : m_seen) {
        json j = json::parse(js, nullptr, false);
        if (j.value("dev_ip", "") == dev_ip) {
            d.dev_id = id, d.dev_name = j.value("dev_name", id), d.model_id = j.value("dev_type", "");
            d.connect_type = "lan", d.bind_state = "free", d.result_msg = "ok";
            return BAMBU_NETWORK_SUCCESS;
        }
    }
    d.result_msg = "impressora não respondeu à descoberta";
    return BAMBU_NETWORK_ERR_CONNECT_FAILED;
}

std::string FilunaElegooSDCPAgent::get_user_selected_machine() { std::lock_guard<std::recursive_mutex> l(m_mutex); return m_selected; }
int FilunaElegooSDCPAgent::set_user_selected_machine(std::string d) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_selected = std::move(d); return 0; }
int FilunaElegooSDCPAgent::set_on_ssdp_msg_fn(OnMsgArrivedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_ssdp = std::move(fn); return 0; }
int FilunaElegooSDCPAgent::set_on_printer_connected_fn(OnPrinterConnectedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_connected = std::move(fn); return 0; }
int FilunaElegooSDCPAgent::set_on_message_fn(OnMessageFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_message = std::move(fn); return 0; }
int FilunaElegooSDCPAgent::set_on_local_connect_fn(OnLocalConnectedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_local_connect = std::move(fn); return 0; }
int FilunaElegooSDCPAgent::set_on_local_message_fn(OnMessageFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_local_message = std::move(fn); return 0; }
int FilunaElegooSDCPAgent::set_queue_on_main_fn(QueueOnMainFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_queue_on_main = std::move(fn); return 0; }

} // namespace Slic3r
