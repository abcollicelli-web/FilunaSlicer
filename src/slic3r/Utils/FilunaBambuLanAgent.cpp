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

#include <nlohmann/json.hpp>

#include "FilunaMqttClient.hpp"
#include "FilunaBambuEmmc.hpp"
#include "FilunaFtps.hpp"
#include "FilunaRede.hpp"

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

std::string FilunaBambuLanAgent::project_file_command(const PrintParams& p, const std::string& remote_name, const std::string& url)
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
    print["url"]            = url.empty() ? "file:///sdcard/" + remote_name : url;
    print["md5"]            = url.empty() ? p.ftp_file_md5 : std::string("from_sd_card"); // memória interna: o literal, como o Bambu Studio
    if (!url.empty())
        print["file"] = remote_name;
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
        // Já procurando (a tela pediu a busca de novo, ou a Bambu voltou a ser a marca ativa): varre a rede agora,
        // sem esperar a próxima rodada de 20 s.
        if (m_discovering.exchange(true)) {
            m_varrer_agora = true;
            return true;
        }
        m_discovery = std::thread([this] { discovery_loop(); });
        return true;
    }
    m_discovering = false;
    if (m_discovery.joinable())
        m_discovery.join();
    return true;
}

std::string FilunaBambuLanAgent::modelo_pela_serie(const std::string& serie)
{
    static const std::pair<const char*, const char*> tabela[] = {
        {"00M", "BL-P001"}, {"00W", "BL-P002"}, {"03W", "C13"}, {"01S", "C11"}, {"01P", "C12"},
        {"030", "N1"},      {"039", "N2S"},     {"094", "O1D"}, {"22E", "N7"},  {"26A", "N9"},
    };
    for (const auto& [prefixo, modelo] : tabela)
        if (serie.rfind(prefixo, 0) == 0)
            return modelo;
    return std::string();
}

void FilunaBambuLanAgent::anunciar(const std::string& js)
{
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

void FilunaBambuLanAgent::varrer_rede()
{
    for (const std::string& ip : Filuna::portas_abertas(Filuna::enderecos_locais(), 8883, std::chrono::milliseconds(900))) {
        if (!m_discovering)
            return;
        // Já identificada (série lida antes): anuncia de novo, sem refazer o TLS. Sem o SSDP (firewall, rede
        // pública) a varredura é o único anúncio; se ela anunciasse cada IP uma vez só, a impressora excluída e
        // procurada de novo, ou achada enquanto outra marca estava ativa (anúncio sem destino), não voltava mais.
        std::string conhecida;
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            for (const auto& [serial, js] : m_seen)
                if (nlohmann::json::parse(js, nullptr, false).value("dev_ip", "") == ip)
                    conhecida = js;
        }
        if (!conhecida.empty()) {
            anunciar(conhecida);
            continue;
        }
        const std::string serie = Filuna::cn_do_certificado(ip, 8883);
        if (serie.size() < 10 || serie.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != std::string::npos)
            continue;
        nlohmann::json j;
        j["dev_id"]       = serie;
        j["dev_ip"]       = ip;
        j["dev_type"]     = modelo_pela_serie(serie);
        j["dev_name"]     = serie;
        j["dev_signal"]   = "0";
        j["connect_type"] = "lan";
        j["bind_state"]   = "free";
        j["sec_link"]     = "secure";
        j["ssdp_version"] = "v1";
        BOOST_LOG_TRIVIAL(info) << "FilunaBambuLanAgent: achada pela porta 8883 " << serie << " em " << ip;
        anunciar(j.dump());
    }
}

void FilunaBambuLanAgent::discovery_loop()
{
    namespace net = boost::asio;
    using udp     = net::ip::udp;
    net::io_context              ioc;
    std::unique_ptr<udp::socket> sock;
    try {
        sock = std::make_unique<udp::socket>(ioc);
        sock->open(udp::v4());
        sock->set_option(udp::socket::reuse_address(true));
        sock->bind(udp::endpoint(udp::v4(), m_ports.ssdp));
        sock->non_blocking(true);
    } catch (const std::exception& e) {
        // porta 2021 ocupada (ex.: Bambu Studio aberto): segue só com a varredura da porta 8883
        BOOST_LOG_TRIVIAL(warning) << "FilunaBambuLanAgent: anúncio SSDP indisponível (" << e.what() << "); procurando pela porta 8883";
        sock.reset();
    }
    char buf[2048];
    auto ultima_varredura = std::chrono::steady_clock::now() - std::chrono::seconds(15); // primeira em ~5 s
    while (m_discovering) {
        if (m_varrer_agora.exchange(false) || std::chrono::steady_clock::now() - ultima_varredura > std::chrono::seconds(20)) {
            varrer_rede();
            ultima_varredura = std::chrono::steady_clock::now();
        }
        if (!sock) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        udp::endpoint             from;
        boost::system::error_code ec;
        const std::size_t         n = sock->receive_from(net::buffer(buf), from, 0, ec);
        if (ec == net::error::would_block) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (ec) {
            BOOST_LOG_TRIVIAL(warning) << "FilunaBambuLanAgent: anúncio SSDP parou: " << ec.message();
            sock.reset();
            continue;
        }
        const std::string js = ssdp_to_discovery_json(std::string(buf, n), from.address().to_string());
        if (!js.empty())
            anunciar(js);
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
    // O envio mora em FilunaFtps.hpp: prazo em cada etapa e o fechamento que a A1 aceita (ensaiado no emulador).
    FilunaFtps::Envio e;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        e.ip    = p.dev_ip.empty() ? m_dev_ip : p.dev_ip;
        e.senha = p.password.empty() ? m_access_code : p.password;
    }
    e.porta         = m_ports.ftps;
    e.tls           = m_ports.tls;
    e.usuario       = p.username.empty() ? std::string("bblp") : p.username;
    e.arquivo_local = local_path;
    e.nome_remoto   = remote_name;
    if (update_fn)
        e.progresso = [&update_fn](int pct) { update_fn(PrintingStageUpload, pct, ""); };
    if (cancel_fn)
        e.cancelado = [&cancel_fn] { return cancel_fn(); };
    e.registro = [ip = e.ip](const std::string& etapa) { BOOST_LOG_TRIVIAL(info) << "Filuna Bambu FTPS " << ip << ": " << etapa; };
    error      = FilunaFtps::enviar(e);
    if (error.empty())
        return true;
    error = "falha no envio FTPS: " + error;
    BOOST_LOG_TRIVIAL(error) << "Filuna Bambu FTPS " << e.ip << ": " << error;
    return false;
}

bool FilunaBambuLanAgent::upload_emmc(const PrintParams& p, const std::string& local_path, const std::string& remote_name,
                                      OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn, std::string& error)
{
    FilunaBambuEmmc::Envio e;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        e.ip     = p.dev_ip.empty() ? m_dev_ip : p.dev_ip;
        e.codigo = p.password.empty() ? m_access_code : p.password;
    }
    e.arquivo_local = local_path;
    e.nome_remoto   = remote_name;
    if (update_fn)
        e.progresso = [&update_fn](int pct) { update_fn(PrintingStageUpload, pct, ""); };
    if (cancel_fn)
        e.cancelado = [&cancel_fn] { return cancel_fn(); };
    e.registro = [ip = e.ip](const std::string& etapa) { BOOST_LOG_TRIVIAL(info) << "Filuna Bambu memória interna " << ip << ": " << etapa; };
    error      = FilunaBambuEmmc::enviar(e);
    if (error.empty())
        return true;
    error = "falha no envio para a memória interna: " + error;
    BOOST_LOG_TRIVIAL(error) << "Filuna Bambu memória interna " << e.ip << ": " << error;
    return false;
}

int FilunaBambuLanAgent::start_local_print(PrintParams params, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn)
{
    const std::string local  = params.filename;
    const std::string remote = sanitize(params.dst_file.empty() ? boost::filesystem::path(local).filename().string() : params.dst_file);
    if (update_fn)
        update_fn(PrintingStageCreate, 0, "");
    // Cartão (FTPS 990) ou memória interna (porta 6000). Começa pelo que o programa indicou (impressora sem cartão que
    // imprime da memória interna: try_emmc_print) e, se falhar, tenta o outro: a Farm e a impressão de teste não sabem
    // se há cartão.
    std::string nome_emmc = remote;
    if (!boost::iends_with(nome_emmc, ".gcode.3mf") && boost::iends_with(nome_emmc, ".3mf"))
        nome_emmc = nome_emmc.substr(0, nome_emmc.size() - 4) + ".gcode.3mf";
    const bool  pode_emmc = boost::iends_with(nome_emmc, ".gcode.3mf");
    std::string error, outro;
    bool        emmc = params.try_emmc_print && pode_emmc;
    bool        ok   = emmc ? upload_emmc(params, local, nome_emmc, update_fn, cancel_fn, error)
                            : upload_ftps(params, local, remote, update_fn, cancel_fn, error);
    if (!ok && pode_emmc && !(cancel_fn && cancel_fn())) {
        emmc = !emmc;
        ok   = emmc ? upload_emmc(params, local, nome_emmc, update_fn, cancel_fn, outro)
                    : upload_ftps(params, local, remote, update_fn, cancel_fn, outro);
        if (!ok)
            error += "; " + outro;
    }
    if (!ok) {
        if (update_fn)
            update_fn(PrintingStageERROR, BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED, error);
        return BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED;
    }
    if (update_fn)
        update_fn(PrintingStageSending, 0, "");
    const std::string cmd = emmc ? project_file_command(params, nome_emmc, FilunaBambuEmmc::endereco(nome_emmc)) : project_file_command(params, remote);
    if (send_message_to_printer(params.dev_id, cmd, 0, 0) != BAMBU_NETWORK_SUCCESS) {
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
