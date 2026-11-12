#include "FilunaBambuLanAgent.hpp"

#include <algorithm>
#include <cstdio>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>
#include <fstream>
#include <stdexcept>
#include <vector>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/write.hpp>
#include <nlohmann/json.hpp>

#include "FilunaMqttClient.hpp"

namespace Slic3r {

namespace {

std::string header(const std::string& datagram, const std::string& name)
{
    std::istringstream in(datagram);
    std::string        line;
    while (std::getline(in, line)) {
        const auto colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string k = line.substr(0, colon);
        boost::trim(k);
        if (boost::iequals(k, name)) {
            std::string v = line.substr(colon + 1);
            boost::trim(v);
            return v;
        }
    }
    return {};
}

std::string sanitize(std::string name)
{
    for (char& c : name)
        if (c == '/' || c == '\\' || c == ':' || c == '"' || c == '\'' || static_cast<unsigned char>(c) < 32)
            c = '_';
    return name.empty() ? std::string("filuna.3mf") : name;
}

} // namespace

AgentInfo FilunaBambuLanAgent::get_agent_info_static()
{
    return AgentInfo{FILUNA_BAMBU_LAN_AGENT_ID, "Bambu Lab (LAN aberto)", "1.0.0",
                     "Modo LAN da Bambu Lab sem plug-in fechado: MQTT/TLS 8883, FTPS 990, SSDP 2021"};
}

FilunaBambuLanAgent::FilunaBambuLanAgent(std::string) {}

FilunaBambuLanAgent::~FilunaBambuLanAgent()
{
    start_discovery(false, false);
    disconnect_printer();
}

std::string FilunaBambuLanAgent::ssdp_to_discovery_json(const std::string& d, const std::string& sender_ip)
{
    // Exemplo do anúncio: NOTIFY * HTTP/1.1 / Location: 192.168.1.20 / NT: urn:bambulab-com:device:3dprinter:1
    // USN: 01P00A000000000 / DevModel.bambu.com: C11 / DevName.bambu.com: P1P / DevConnect.bambu.com: lan
    // DevBind.bambu.com: free / Devseclink.bambu.com: secure / DevVersion.bambu.com: 01.07.00.00 / DevSignal.bambu.com: -40
    if (header(d, "NT").find("bambulab") == std::string::npos && header(d, "DevModel.bambu.com").empty())
        return {};
    const std::string serial = header(d, "USN");
    if (serial.empty())
        return {};
    std::string ip = header(d, "Location");
    if (ip.empty())
        ip = sender_ip;
    nlohmann::json j;
    j["dev_id"]       = serial;
    j["dev_ip"]       = ip;
    j["dev_type"]     = header(d, "DevModel.bambu.com");
    j["dev_name"]     = header(d, "DevName.bambu.com").empty() ? serial : header(d, "DevName.bambu.com");
    j["dev_signal"]   = header(d, "DevSignal.bambu.com").empty() ? "0" : header(d, "DevSignal.bambu.com");
    j["connect_type"] = header(d, "DevConnect.bambu.com").empty() ? "lan" : header(d, "DevConnect.bambu.com");
    j["bind_state"]   = header(d, "DevBind.bambu.com").empty() ? "free" : header(d, "DevBind.bambu.com");
    j["sec_link"]     = header(d, "Devseclink.bambu.com").empty() ? "secure" : header(d, "Devseclink.bambu.com");
    j["ssdp_version"] = header(d, "DevVersion.bambu.com").empty() ? "v1" : header(d, "DevVersion.bambu.com");
    return j.dump();
}

std::string FilunaBambuLanAgent::project_file_command(const PrintParams& p, const std::string& remote_name)
{
    nlohmann::json print;
    print["sequence_id"]    = "0";
    print["command"]        = "project_file";
    print["param"]          = "Metadata/plate_" + std::to_string(std::max(1, p.plate_index)) + ".gcode";
    print["project_id"]     = "0";
    print["profile_id"]     = "0";
    print["task_id"]        = "0";
    print["subtask_id"]     = "0";
    print["subtask_name"]   = p.project_name.empty() ? remote_name : p.project_name;
    print["url"]            = "file:///sdcard/" + remote_name;
    print["md5"]            = p.ftp_file_md5;
    print["timelapse"]      = p.task_record_timelapse;
    print["bed_type"]       = p.task_bed_type.empty() ? std::string("auto") : p.task_bed_type;
    print["bed_levelling"]  = p.task_bed_leveling;
    print["bed_leveling"]   = p.task_bed_leveling;
    print["flow_cali"]      = p.task_flow_cali;
    print["vibration_cali"] = p.task_vibration_cali;
    print["layer_inspect"]  = p.task_layer_inspect;
    print["use_ams"]        = p.task_use_ams;
    nlohmann::json mapping  = nlohmann::json::parse(p.ams_mapping.empty() ? std::string("[]") : p.ams_mapping, nullptr, false);
    print["ams_mapping"]    = mapping.is_array() ? mapping : nlohmann::json::array();
    if (!p.ams_mapping2.empty()) {
        nlohmann::json m2 = nlohmann::json::parse(p.ams_mapping2, nullptr, false);
        if (m2.is_array())
            print["ams_mapping2"] = m2;
    }
    nlohmann::json cmd;
    cmd["print"] = print;
    return cmd.dump();
}

int FilunaBambuLanAgent::send_message(std::string dev_id, std::string json_str, int qos, int flag)
{
    return send_message_to_printer(std::move(dev_id), std::move(json_str), qos, flag);
}

int FilunaBambuLanAgent::connect_printer(const PrinterConnectionParams& params)
{
    disconnect_printer();
    std::string dev_id = params.dev_id;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (dev_id.empty()) // impressora escolhida pelo IP: o serial vem do anúncio SSDP
            for (const auto& [serial, js] : m_seen)
                if (nlohmann::json::parse(js, nullptr, false).value("dev_ip", "") == params.host)
                    dev_id = serial;
        m_dev_id      = dev_id;
        m_dev_ip      = params.host;
        m_access_code = params.password;
    }
    if (dev_id.empty() || params.host.empty() || params.password.empty()) {
        dispatch_local_connect(ConnectStatusFailed, dev_id, "Informe IP, número de série e código de acesso da impressora.");
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    Filuna::MqttClient::Options opt;
    opt.host      = params.host;
    opt.port      = params.port.empty() ? m_ports.mqtt : static_cast<unsigned short>(std::stoi(params.port));
    opt.tls       = m_ports.tls;
    opt.client_id = "filuna-" + dev_id;
    opt.username  = params.username.empty() ? std::string("bblp") : params.username;
    opt.password  = params.password;
    auto        client = std::make_unique<Filuna::MqttClient>();
    std::string error;
    const std::string report = "device/" + dev_id + "/report";
    const bool ok = client->connect(
        opt, {report},
        [this, dev_id](const std::string&, const std::string& payload) { dispatch_message(dev_id, payload); },
        [this, dev_id](const std::string& why) { dispatch_local_connect(ConnectStatusLost, dev_id, why); }, error);
    if (!ok) {
        dispatch_local_connect(ConnectStatusFailed, dev_id, error);
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_mqtt = std::move(client);
        if (m_selected.empty())
            m_selected = dev_id;
    }
    // relatório completo logo na conexão (estado, AMS, temperaturas)
    send_message_to_printer(dev_id, R"({"pushing":{"sequence_id":"0","command":"pushall","version":1,"push_target":1}})", 0, 0);
    dispatch_local_connect(ConnectStatusOk, dev_id, "");
    OnPrinterConnectedFn fn;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        fn = m_on_connected;
    }
    if (fn)
        fn(report);
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaBambuLanAgent::disconnect_printer()
{
    std::unique_ptr<Filuna::MqttClient> c;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        c = std::move(m_mqtt);
    }
    if (c)
        c->close();
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaBambuLanAgent::send_message_to_printer(std::string dev_id, std::string json_str, int, int)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_mqtt || !m_mqtt->connected())
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    const std::string id = dev_id.empty() ? m_dev_id : dev_id;
    return m_mqtt->publish("device/" + id + "/request", json_str) ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_CONNECT_FAILED;
}

bool FilunaBambuLanAgent::start_discovery(bool start, bool)
{
    if (start) {
        if (m_discovering.exchange(true))
            return true;
        m_discovery = std::thread([this] { discovery_loop(); });
        return true;
    }
    m_discovering = false;
    if (m_discovery.joinable())
        m_discovery.join();
    return true;
}

void FilunaBambuLanAgent::discovery_loop()
{
    namespace net = boost::asio;
    using udp     = net::ip::udp;
    try {
        net::io_context ioc;
        udp::socket     sock(ioc);
        sock.open(udp::v4());
        sock.set_option(udp::socket::reuse_address(true));
        sock.bind(udp::endpoint(udp::v4(), m_ports.ssdp));
        sock.non_blocking(true);
        char buf[2048];
        while (m_discovering) {
            udp::endpoint             from;
            boost::system::error_code ec;
            const std::size_t         n = sock.receive_from(net::buffer(buf), from, 0, ec);
            if (ec == net::error::would_block) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            if (ec)
                break;
            const std::string js = ssdp_to_discovery_json(std::string(buf, n), from.address().to_string());
            if (js.empty())
                continue;
            OnMsgArrivedFn fn;
            {
                std::lock_guard<std::recursive_mutex> lock(m_mutex);
                const std::string serial = nlohmann::json::parse(js).value("dev_id", "");
                m_seen[serial]           = js;
                fn                       = m_on_ssdp;
            }
            if (fn)
                fn(js);
        }
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "FilunaBambuLanAgent: descoberta SSDP parou: " << e.what();
    }
}

int FilunaBambuLanAgent::bind_detect(std::string dev_ip, std::string, detectResult& detect)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (const auto& [serial, js] : m_seen) {
        const auto j = nlohmann::json::parse(js, nullptr, false);
        if (j.value("dev_ip", "") == dev_ip) {
            detect.dev_id       = serial;
            detect.model_id     = j.value("dev_type", "");
            detect.dev_name     = j.value("dev_name", serial);
            detect.version      = j.value("ssdp_version", "");
            detect.connect_type = "lan";
            detect.bind_state   = j.value("bind_state", "free");
            detect.result_msg   = "ok";
            return BAMBU_NETWORK_SUCCESS;
        }
    }
    detect.result_msg = "impressora não anunciada na rede";
    return BAMBU_NETWORK_ERR_CONNECT_FAILED;
}

int         FilunaBambuLanAgent::set_server_callback(OnServerErrFn fn) { m_on_server_err = std::move(fn); return BAMBU_NETWORK_SUCCESS; }
std::string FilunaBambuLanAgent::get_user_selected_machine() { std::lock_guard<std::recursive_mutex> l(m_mutex); return m_selected; }
int FilunaBambuLanAgent::set_user_selected_machine(std::string dev_id) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_selected = std::move(dev_id); return BAMBU_NETWORK_SUCCESS; }

bool FilunaBambuLanAgent::upload_ftps(const PrintParams& p, const std::string& local_path, const std::string& remote_name,
                                      OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, std::string& error)
{
    // Cliente FTPS próprio: o libcurl 7.75 das dependências trunca envios FTPS grandes (prova da
    // sonda: 3,4 MB enviados, 2 MiB recebidos) e a impressora exige reaproveitar a sessão TLS do
    // canal de controle no canal de dados, como o ftplib do ha-bambulab faz.
    namespace net = boost::asio;
    namespace ssl = boost::asio::ssl;
    using tcp     = net::ip::tcp;
    std::string ip, code, user;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        ip   = p.dev_ip.empty() ? m_dev_ip : p.dev_ip;
        code = p.password.empty() ? m_access_code : p.password;
    }
    user = p.username.empty() ? std::string("bblp") : p.username;
    std::ifstream in(local_path, std::ios::binary);
    if (!in) {
        error = "não foi possível abrir " + local_path;
        return false;
    }
    in.seekg(0, std::ios::end);
    const std::uint64_t total = std::uint64_t(in.tellg());
    in.seekg(0);
    try {
        net::io_context ioc;
        ssl::context    ctx(ssl::context::tls_client);
        ctx.set_verify_mode(ssl::verify_none); // certificado próprio da impressora
        tcp::resolver res(ioc);
        const auto    ends = res.resolve(ip, std::to_string(m_ports.ftps));

        using Stream = ssl::stream<tcp::socket>;
        Stream ctl(ioc, ctx);
        net::connect(ctl.next_layer(), ends);
        const bool tls = m_ports.tls;
        if (tls)
            ctl.handshake(ssl::stream_base::client);
        std::string pend;
        auto read_line = [&]() -> std::string {
            for (;;) {
                const auto nl = pend.find("\r\n");
                if (nl != std::string::npos) {
                    std::string l = pend.substr(0, nl);
                    pend.erase(0, nl + 2);
                    return l;
                }
                char        b[512];
                std::size_t n = tls ? ctl.read_some(net::buffer(b)) : ctl.next_layer().read_some(net::buffer(b));
                pend.append(b, n);
            }
        };
        auto reply = [&]() -> std::string { // resposta completa (multilinha "123-" ... "123 ")
            std::string l = read_line();
            if (l.size() >= 4 && l[3] == '-') {
                const std::string code3 = l.substr(0, 3) + " ";
                std::string       m;
                while ((m = read_line()).compare(0, 4, code3) != 0) {}
                return m;
            }
            return l;
        };
        auto cmd = [&](const std::string& c) -> std::string {
            const std::string line = c + "\r\n";
            tls ? net::write(ctl, net::buffer(line)) : net::write(ctl.next_layer(), net::buffer(line));
            return reply();
        };
        auto expect = [&](const std::string& r, char first, const char* what) {
            if (r.empty() || r[0] != first)
                throw std::runtime_error(std::string(what) + ": " + r);
        };
        expect(reply(), '2', "banner");
        const std::string u = cmd("USER " + user);
        if (u[0] == '3')
            expect(cmd("PASS " + code), '2', "login recusado");
        else
            expect(u, '2', "login");
        if (tls) {
            expect(cmd("PBSZ 0"), '2', "PBSZ");
            expect(cmd("PROT P"), '2', "PROT");
        }
        expect(cmd("TYPE I"), '2', "TYPE");
        const std::string pasv = cmd("PASV");
        expect(pasv, '2', "PASV");
        const auto     l = pasv.find('('), r = pasv.find(')');
        int            h[6]{};
        if (l == std::string::npos || r == std::string::npos ||
            std::sscanf(pasv.substr(l + 1, r - l - 1).c_str(), "%d,%d,%d,%d,%d,%d", &h[0], &h[1], &h[2], &h[3], &h[4], &h[5]) != 6)
            throw std::runtime_error("resposta PASV inválida: " + pasv);
        const unsigned short dport = static_cast<unsigned short>(h[4] * 256 + h[5]);
        Stream data(ioc, ctx);
        // IP do controle (a resposta PASV pode trazer endereço interno)
        data.next_layer().connect(tcp::endpoint(ctl.next_layer().remote_endpoint().address(), dport));
        const std::string line = "STOR " + remote_name + "\r\n";
        tls ? net::write(ctl, net::buffer(line)) : net::write(ctl.next_layer(), net::buffer(line));
        if (tls) {
            SSL_SESSION* sess = SSL_get1_session(ctl.native_handle());
            if (sess) {
                SSL_set_session(data.native_handle(), sess);
                SSL_SESSION_free(sess);
            }
            data.handshake(ssl::stream_base::client);
        }
        expect(reply(), '1', "STOR");
        std::vector<char> buf(256 * 1024);
        std::uint64_t     sent = 0;
        int               last = -1;
        while (in) {
            in.read(buf.data(), std::streamsize(buf.size()));
            const std::size_t n = std::size_t(in.gcount());
            if (n == 0)
                break;
            tls ? net::write(data, net::buffer(buf.data(), n)) : net::write(data.next_layer(), net::buffer(buf.data(), n));
            sent += n;
            if (cancel_fn && cancel_fn())
                throw std::runtime_error("envio cancelado");
            const int pct = total ? int(sent * 100 / total) : 100;
            if (update_fn && pct != last)
                update_fn(PrintingStageUpload, last = pct, "");
        }
        boost::system::error_code ec;
        if (tls)
            data.shutdown(ec); // close_notify: a impressora só fecha o arquivo depois dele
        data.next_layer().shutdown(tcp::socket::shutdown_both, ec);
        data.next_layer().close(ec);
        expect(reply(), '2', "fim do envio");
        cmd("QUIT");
        if (sent != total)
            throw std::runtime_error("enviados " + std::to_string(sent) + " de " + std::to_string(total) + " bytes");
    } catch (const std::exception& e) {
        error = std::string("falha no envio FTPS: ") + e.what();
        return false;
    }
    return true;
}

int FilunaBambuLanAgent::start_local_print(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn)
{
    const std::string local  = params.filename;
    const std::string remote = sanitize(params.dst_file.empty() ? boost::filesystem::path(local).filename().string() : params.dst_file);
    if (update_fn)
        update_fn(PrintingStageCreate, 0, "");
    std::string error;
    if (!upload_ftps(params, local, remote, update_fn, cancel_fn, error)) {
        if (update_fn)
            update_fn(PrintingStageERROR, BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED, error);
        return BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED;
    }
    if (update_fn)
        update_fn(PrintingStageSending, 0, "");
    if (send_message_to_printer(params.dev_id, project_file_command(params, remote), 0, 0) != BAMBU_NETWORK_SUCCESS) {
        if (update_fn)
            update_fn(PrintingStageERROR, BAMBU_NETWORK_ERR_CONNECT_FAILED, "impressora desconectada");
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    if (update_fn)
        update_fn(PrintingStageFinished, 0, "");
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaBambuLanAgent::start_print(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, OnWaitFn)
{
    return start_local_print(std::move(params), std::move(update_fn), std::move(cancel_fn));
}

int FilunaBambuLanAgent::start_local_print_with_record(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, OnWaitFn)
{
    return start_local_print(std::move(params), std::move(update_fn), std::move(cancel_fn));
}

int FilunaBambuLanAgent::start_send_gcode_to_sdcard(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, OnWaitFn)
{
    const std::string remote = sanitize(params.dst_file.empty() ? boost::filesystem::path(params.filename).filename().string() : params.dst_file);
    std::string       error;
    if (!upload_ftps(params, params.filename, remote, update_fn, cancel_fn, error)) {
        if (update_fn)
            update_fn(PrintingStageERROR, BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED, error);
        return BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED;
    }
    if (update_fn)
        update_fn(PrintingStageFinished, 0, "");
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaBambuLanAgent::start_sdcard_print(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn)
{
    const std::string remote = sanitize(params.dst_file.empty() ? params.ftp_file : params.dst_file);
    const int         rc     = send_message_to_printer(params.dev_id, project_file_command(params, remote), 0, 0);
    if (update_fn)
        update_fn(rc == BAMBU_NETWORK_SUCCESS ? PrintingStageFinished : PrintingStageERROR, rc, "");
    return rc;
}

void FilunaBambuLanAgent::dispatch_local_connect(int state, const std::string& dev_id, const std::string& msg)
{
    OnLocalConnectedFn fn;
    QueueOnMainFn      q;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        fn = m_on_local_connect;
        q  = m_queue_on_main;
    }
    if (!fn)
        return;
    auto call = [fn, state, dev_id, msg] { fn(state, dev_id, msg); };
    q ? q(call) : call();
}

void FilunaBambuLanAgent::dispatch_message(const std::string& dev_id, const std::string& payload)
{
    OnMessageFn   local, cloud;
    QueueOnMainFn q;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        local = m_on_local_message;
        cloud = m_on_message;
        q     = m_queue_on_main;
    }
    auto call = [local, cloud, dev_id, payload] {
        if (local)
            local(dev_id, payload);
        else if (cloud)
            cloud(dev_id, payload);
    };
    q ? q(call) : call();
}

int FilunaBambuLanAgent::set_on_ssdp_msg_fn(OnMsgArrivedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_ssdp = std::move(fn); return 0; }
int FilunaBambuLanAgent::set_on_printer_connected_fn(OnPrinterConnectedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_connected = std::move(fn); return 0; }
int FilunaBambuLanAgent::set_on_subscribe_failure_fn(GetSubscribeFailureFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_sub_fail = std::move(fn); return 0; }
int FilunaBambuLanAgent::set_on_message_fn(OnMessageFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_message = std::move(fn); return 0; }
int FilunaBambuLanAgent::set_on_user_message_fn(OnMessageFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_user_message = std::move(fn); return 0; }
int FilunaBambuLanAgent::set_on_local_connect_fn(OnLocalConnectedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_local_connect = std::move(fn); return 0; }
int FilunaBambuLanAgent::set_on_local_message_fn(OnMessageFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_local_message = std::move(fn); return 0; }
int FilunaBambuLanAgent::set_queue_on_main_fn(QueueOnMainFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_queue_on_main = std::move(fn); return 0; }

} // namespace Slic3r
