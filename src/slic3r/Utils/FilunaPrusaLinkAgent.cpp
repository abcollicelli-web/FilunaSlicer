#include "FilunaPrusaLinkAgent.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <regex>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/filesystem/path.hpp>
#include <openssl/evp.h>

namespace Slic3r {

namespace net   = boost::asio;
namespace beast = boost::beast;
namespace http  = boost::beast::http;
using json      = nlohmann::json;

namespace {

std::string md5_hex(const std::string& s)
{
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int  n = 0;
    EVP_Digest(s.data(), s.size(), md, &n, EVP_md5(), nullptr);
    static const char* hex = "0123456789abcdef";
    std::string        out;
    for (unsigned i = 0; i < n; ++i)
        out.push_back(hex[md[i] >> 4]), out.push_back(hex[md[i] & 15]);
    return out;
}

std::string campo(const std::string& ch, const std::string& k)
{
    std::smatch m;
    if (std::regex_search(ch, m, std::regex(k + "=\"([^\"]*)\"")) || std::regex_search(ch, m, std::regex(k + "=([^,\\s]+)")))
        return m[1];
    return {};
}

std::string url_nome(const std::string& s)
{
    std::string o;
    char        b[4];
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.')
            o.push_back(char(c));
        else
            std::snprintf(b, sizeof b, "%%%02X", c), o += b;
    }
    return o;
}

} // namespace

AgentInfo FilunaPrusaLinkAgent::get_agent_info_static()
{
    return AgentInfo{FILUNA_PRUSALINK_AGENT_ID, "Prusa (PrusaLink)", "1.0.0", "API aberta PrusaLink v1: estado, envio, pausa, retomada e parada"};
}

FilunaPrusaLinkAgent::FilunaPrusaLinkAgent(std::string) {}
FilunaPrusaLinkAgent::~FilunaPrusaLinkAgent() { disconnect_printer(); }

std::string FilunaPrusaLinkAgent::digest_header(const std::string& ch, const std::string& user, const std::string& pass,
                                                const std::string& method, const std::string& uri, const std::string& cnonce, int nc)
{
    const std::string realm = campo(ch, "realm"), nonce = campo(ch, "nonce"), qop = campo(ch, "qop"), opaque = campo(ch, "opaque");
    const std::string ha1 = md5_hex(user + ":" + realm + ":" + pass), ha2 = md5_hex(method + ":" + uri);
    char              ncs[9];
    std::snprintf(ncs, sizeof ncs, "%08x", nc);
    const bool        auth_qop = qop.find("auth") != std::string::npos;
    const std::string resp     = auth_qop ? md5_hex(ha1 + ":" + nonce + ":" + ncs + ":" + cnonce + ":auth:" + ha2) : md5_hex(ha1 + ":" + nonce + ":" + ha2);
    std::string h = "Digest username=\"" + user + "\", realm=\"" + realm + "\", nonce=\"" + nonce + "\", uri=\"" + uri +
                    "\", response=\"" + resp + "\"";
    if (auth_qop)
        h += std::string(", qop=auth, nc=") + ncs + ", cnonce=\"" + cnonce + "\"";
    if (!opaque.empty())
        h += ", opaque=\"" + opaque + "\"";
    return h;
}

int FilunaPrusaLinkAgent::call(const std::string& method, const std::string& path, const std::string& body, std::string& reply,
                               const std::map<std::string, std::string>& headers)
{
    std::string ip, port, user, pass, key, ch;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        ip = m_ip, port = m_port, user = m_user, pass = m_password, key = m_api_key, ch = m_challenge;
    }
    for (int tentativa = 0; tentativa < 2; ++tentativa) {
        try {
            net::io_context        ioc;
            net::ip::tcp::resolver res(ioc);
            beast::tcp_stream      s(ioc);
            s.expires_after(std::chrono::seconds(300));
            s.connect(res.resolve(ip, port));
            http::request<http::string_body> req{http::string_to_verb(method), path, 11};
            req.set(http::field::host, ip);
            for (const auto& [k, v] : headers)
                req.set(k, v);
            if (!key.empty())
                req.set("X-Api-Key", key);
            else if (!ch.empty()) {
                int nc;
                {
                    std::lock_guard<std::recursive_mutex> l(m_mutex);
                    nc = ++m_nc;
                }
                req.set(http::field::authorization, digest_header(ch, user, pass, method, path, "f1u7a" + std::to_string(nc), nc));
            }
            req.body() = body;
            req.prepare_payload();
            http::write(s, req);
            beast::flat_buffer                b;
            http::response<http::string_body> r;
            http::read(s, b, r);
            reply = r.body();
            if (r.result_int() == 401 && key.empty() && tentativa == 0 && r.count(http::field::www_authenticate)) {
                ch = std::string(r[http::field::www_authenticate]);
                std::lock_guard<std::recursive_mutex> l(m_mutex);
                m_challenge = ch, m_nc = 0;
                continue; // repete com o desafio novo (nonce expira)
            }
            return int(r.result_int());
        } catch (const std::exception& e) {
            reply = e.what();
            return 0;
        }
    }
    return 401;
}

json FilunaPrusaLinkAgent::to_push_status(const json& s)
{
    const json pr = s.value("printer", json::object());
    const json jb = s.value("job", json::object());
    const std::string st = pr.value("state", std::string());
    std::string       gs = "IDLE";
    if (st == "PRINTING" || st == "BUSY")
        gs = "RUNNING";
    else if (st == "PAUSED")
        gs = "PAUSE";
    else if (st == "FINISHED")
        gs = "FINISH";
    else if (st == "STOPPED" || st == "ERROR")
        gs = "FAILED";
    json p{{"command", "push_status"}, {"sequence_id", "0"}, {"gcode_state", gs},
           {"mc_percent", int(jb.value("progress", 0.0) + 0.5)}, {"mc_remaining_time", jb.value("time_remaining", 0) / 60},
           {"nozzle_temper", pr.value("temp_nozzle", 0.0)}, {"nozzle_target_temper", pr.value("target_nozzle", 0.0)},
           {"bed_temper", pr.value("temp_bed", 0.0)}, {"bed_target_temper", pr.value("target_bed", 0.0)}};
    return json{{"print", p}};
}

void FilunaPrusaLinkAgent::dispatch(const std::string& payload)
{
    OnMessageFn   local, cloud;
    QueueOnMainFn q;
    std::string   dev;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        local = m_on_local_message, cloud = m_on_message, q = m_queue_on_main, dev = m_dev_id;
    }
    auto call_fn = [local, cloud, dev, payload] {
        if (local)
            local(dev, payload);
        else if (cloud)
            cloud(dev, payload);
    };
    q ? q(call_fn) : call_fn();
}

void FilunaPrusaLinkAgent::dispatch_connect(int state, const std::string& msg)
{
    OnLocalConnectedFn fn;
    QueueOnMainFn      q;
    std::string        dev;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        fn = m_on_local_connect, q = m_queue_on_main, dev = m_dev_id;
    }
    if (!fn)
        return;
    auto c = [fn, state, dev, msg] { fn(state, dev, msg); };
    q ? q(c) : c();
}

void FilunaPrusaLinkAgent::poll_loop()
{
    int falhas = 0;
    while (m_polling) {
        std::string body;
        const int   code = call("GET", "/api/v1/status", "", body);
        json        j    = json::parse(body, nullptr, false);
        if (code == 200 && j.is_object()) {
            falhas = 0;
            if (j.contains("job") && j["job"].contains("id")) {
                std::lock_guard<std::recursive_mutex> l(m_mutex);
                m_job_id = j["job"].value("id", 0);
            }
            dispatch(to_push_status(j).dump());
        } else if (++falhas == 5)
            dispatch_connect(ConnectStatusLost, "sem resposta da PrusaLink");
        for (int i = 0; i < m_poll_ms / 50 && m_polling; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int FilunaPrusaLinkAgent::send_message(std::string d, std::string j, int q, int f) { return send_message_to_printer(std::move(d), std::move(j), q, f); }

int FilunaPrusaLinkAgent::connect_printer(const PrinterConnectionParams& p)
{
    disconnect_printer();
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        m_ip = p.host, m_port = p.port.empty() ? "80" : p.port, m_dev_id = p.dev_id.empty() ? p.host : p.dev_id;
        // usuário preenchido = Digest (firmware atual); só a senha = chave de API (firmware antigo)
        if (p.username.empty())
            m_api_key = p.password, m_user.clear(), m_password.clear();
        else
            m_user = p.username, m_password = p.password, m_api_key.clear();
        m_challenge.clear();
        if (m_selected.empty())
            m_selected = m_dev_id;
    }
    std::string body;
    const int   code = call("GET", "/api/v1/status", "", body);
    if (code != 200) {
        dispatch_connect(ConnectStatusFailed, code == 401 ? "usuário ou senha da PrusaLink recusados" : "PrusaLink não respondeu: " + body);
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    dispatch_connect(ConnectStatusOk, "");
    dispatch(to_push_status(json::parse(body, nullptr, false)).dump());
    if (m_on_connected)
        m_on_connected("/api/v1/status");
    m_polling = true;
    m_poll    = std::thread([this] { poll_loop(); });
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaPrusaLinkAgent::disconnect_printer()
{
    m_polling = false;
    if (m_poll.joinable())
        m_poll.join();
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaPrusaLinkAgent::send_message_to_printer(std::string, std::string json_str, int, int)
{
    json cmd = json::parse(json_str, nullptr, false);
    if (!cmd.is_object())
        return BAMBU_NETWORK_ERR_INVALID_HANDLE;
    if (cmd.contains("pushing"))
        return BAMBU_NETWORK_SUCCESS;
    const std::string c = cmd.contains("print") ? cmd["print"].value("command", std::string()) : std::string();
    int               id;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        id = m_job_id;
    }
    std::string r;
    int         code = 0;
    const std::string job = "/api/v1/job/" + std::to_string(id);
    if (c == "pause")
        code = call("PUT", job + "/pause", "", r);
    else if (c == "resume")
        code = call("PUT", job + "/resume", "", r);
    else if (c == "stop")
        code = call("DELETE", job, "", r);
    else
        return ORCA_NETWORK_ERR_CMD_NOT_SUPPORTED;
    return code >= 200 && code < 300 ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_CONNECT_FAILED;
}

int FilunaPrusaLinkAgent::upload(const PrintParams& p, bool print, OnUpdateStatusFn u, std::string& error)
{
    std::ifstream in(p.filename, std::ios::binary);
    if (!in) {
        error = "não foi possível abrir " + p.filename;
        return BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED;
    }
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const std::string name = p.dst_file.empty() ? boost::filesystem::path(p.filename).filename().string() : p.dst_file;
    if (u)
        u(PrintingStageUpload, 0, "");
    std::string r;
    const int   code = call("PUT", "/api/v1/files/usb/" + url_nome(name), data, r,
                            {{"Content-Type", "application/octet-stream"}, {"Print-After-Upload", print ? "?1" : "?0"}, {"Overwrite", "?1"}});
    if (code != 201 && code != 204 && code != 200) {
        error = "a PrusaLink recusou o envio (HTTP " + std::to_string(code) + "): " + r;
        return BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED;
    }
    if (u)
        u(PrintingStageUpload, 100, "");
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaPrusaLinkAgent::start_local_print(PrintParams p, OnUpdateStatusFn u, WasCancelledFn)
{
    std::string err;
    const int   rc = upload(p, true, u, err);
    if (u)
        u(rc == BAMBU_NETWORK_SUCCESS ? PrintingStageFinished : PrintingStageERROR, rc, err);
    return rc;
}
int FilunaPrusaLinkAgent::start_print(PrintParams p, OnUpdateStatusFn u, WasCancelledFn c, OnWaitFn) { return start_local_print(std::move(p), std::move(u), std::move(c)); }
int FilunaPrusaLinkAgent::start_local_print_with_record(PrintParams p, OnUpdateStatusFn u, WasCancelledFn c, OnWaitFn) { return start_local_print(std::move(p), std::move(u), std::move(c)); }
int FilunaPrusaLinkAgent::start_send_gcode_to_sdcard(PrintParams p, OnUpdateStatusFn u, WasCancelledFn, OnWaitFn)
{
    std::string err;
    const int   rc = upload(p, false, u, err);
    if (u)
        u(rc == BAMBU_NETWORK_SUCCESS ? PrintingStageFinished : PrintingStageERROR, rc, err);
    return rc;
}
int FilunaPrusaLinkAgent::start_sdcard_print(PrintParams p, OnUpdateStatusFn u, WasCancelledFn)
{
    std::string r;
    const int   code = call("POST", "/api/v1/files/usb/" + url_nome(p.dst_file.empty() ? p.ftp_file : p.dst_file), "", r);
    const int   rc   = code >= 200 && code < 300 ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_CONNECT_FAILED;
    if (u)
        u(rc == BAMBU_NETWORK_SUCCESS ? PrintingStageFinished : PrintingStageERROR, rc, r);
    return rc;
}

int FilunaPrusaLinkAgent::bind_detect(std::string dev_ip, std::string, detectResult& d)
{
    d.dev_id = dev_ip, d.dev_name = dev_ip, d.connect_type = "lan", d.bind_state = "free", d.result_msg = "ok";
    return BAMBU_NETWORK_SUCCESS;
}

std::string FilunaPrusaLinkAgent::get_user_selected_machine() { std::lock_guard<std::recursive_mutex> l(m_mutex); return m_selected; }
int FilunaPrusaLinkAgent::set_user_selected_machine(std::string d) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_selected = std::move(d); return 0; }
int FilunaPrusaLinkAgent::set_on_ssdp_msg_fn(OnMsgArrivedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_ssdp = std::move(fn); return 0; }
int FilunaPrusaLinkAgent::set_on_printer_connected_fn(OnPrinterConnectedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_connected = std::move(fn); return 0; }
int FilunaPrusaLinkAgent::set_on_message_fn(OnMessageFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_message = std::move(fn); return 0; }
int FilunaPrusaLinkAgent::set_on_local_connect_fn(OnLocalConnectedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_local_connect = std::move(fn); return 0; }
int FilunaPrusaLinkAgent::set_on_local_message_fn(OnMessageFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_local_message = std::move(fn); return 0; }
int FilunaPrusaLinkAgent::set_queue_on_main_fn(QueueOnMainFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_queue_on_main = std::move(fn); return 0; }

} // namespace Slic3r
