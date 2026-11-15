#include "FilunaAnycubicLanAgent.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <random>
#include <sstream>
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
#include "FilunaCameraUrls.hpp"

namespace Slic3r {

namespace net   = boost::asio;
namespace beast = boost::beast;
namespace http  = boost::beast::http;
using json      = nlohmann::json;

namespace {

const char* const   PREFIXO   = "anycubic/anycubicCloud/v1";
const unsigned short PORTA_INFO = 18910;

std::string hex(const unsigned char* d, unsigned n)
{
    static const char* h = "0123456789abcdef";
    std::string        out;
    for (unsigned i = 0; i < n; ++i) {
        out.push_back(h[d[i] >> 4]);
        out.push_back(h[d[i] & 15]);
    }
    return out;
}

std::string md5_hex(const std::string& s)
{
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int  n   = 0;
    EVP_MD_CTX*   ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
    EVP_DigestUpdate(ctx, s.data(), s.size());
    EVP_DigestFinal_ex(ctx, md, &n);
    EVP_MD_CTX_free(ctx);
    return hex(md, n);
}

std::string md5_arquivo(const std::string& path, std::uint64_t& tamanho)
{
    std::ifstream in(path, std::ios::binary);
    EVP_MD_CTX*   ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
    std::vector<char> buf(1 << 16);
    tamanho = 0;
    while (in) {
        in.read(buf.data(), std::streamsize(buf.size()));
        if (in.gcount() > 0) {
            EVP_DigestUpdate(ctx, buf.data(), std::size_t(in.gcount()));
            tamanho += std::uint64_t(in.gcount());
        }
    }
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int  n = 0;
    EVP_DigestFinal_ex(ctx, md, &n);
    EVP_MD_CTX_free(ctx);
    return hex(md, n);
}

std::string base64_decodificar(const std::string& b64)
{
    std::string limpo;
    for (char c : b64)
        if (c != '\n' && c != '\r' && c != ' ')
            limpo.push_back(c);
    if (limpo.empty() || limpo.size() % 4 != 0)
        return {};
    std::string out(limpo.size() / 4 * 3, '\0');
    const int   n = EVP_DecodeBlock(reinterpret_cast<unsigned char*>(&out[0]), reinterpret_cast<const unsigned char*>(limpo.data()),
                                    int(limpo.size()));
    if (n < 0)
        return {};
    std::size_t corte = 0;
    if (limpo.size() >= 1 && limpo[limpo.size() - 1] == '=')
        ++corte;
    if (limpo.size() >= 2 && limpo[limpo.size() - 2] == '=')
        ++corte;
    out.resize(std::size_t(n) - corte);
    return out;
}

std::string aleatorio(std::size_t n, const std::string& alfabeto)
{
    static std::mt19937                        g{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> d(0, alfabeto.size() - 1);
    std::string                                s;
    for (std::size_t i = 0; i < n; ++i)
        s.push_back(alfabeto[d(g)]);
    return s;
}

long long agora_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

// Pedido HTTP simples com prazo. 'alvo' já vem com caminho e consulta. Devolve false se não houve resposta HTTP.
bool http_pedir(http::verb verbo, const std::string& host, unsigned short porta, const std::string& alvo, const std::string& corpo,
                const std::vector<std::pair<std::string, std::string>>& cabecalhos, int prazo_conexao_ms, int prazo_total_s,
                unsigned& status, std::string& resposta)
{
    try {
        net::io_context        ioc;
        net::ip::tcp::resolver res(ioc);
        const auto             ends = res.resolve(host, std::to_string(porta));
        beast::tcp_stream      s(ioc);
        s.expires_after(std::chrono::milliseconds(prazo_conexao_ms));
        s.connect(ends);
        s.expires_after(std::chrono::seconds(prazo_total_s));
        http::request<http::string_body> req{verbo, alvo, 11};
        req.set(http::field::host, host + ":" + std::to_string(porta));
        req.set(http::field::user_agent, "FilunaSlicer");
        for (const auto& [k, v] : cabecalhos)
            req.set(k, v);
        req.body() = corpo;
        req.prepare_payload();
        http::write(s, req);
        beast::flat_buffer                 b;
        http::response_parser<http::string_body> p;
        p.body_limit(8 * 1024 * 1024);
        http::read(s, b, p);
        beast::error_code ec;
        s.socket().shutdown(net::ip::tcp::socket::shutdown_both, ec);
        status   = p.get().result_int();
        resposta = p.get().body();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// "http://192.168.1.50:18910/ctrl?x=1" -> porta, caminho ("/ctrl") e consulta ("x=1"). O host é sempre o informado
// pelo usuário: o que a impressora devolve só diz ONDE nesse mesmo host.
void partes_da_url(const std::string& url, unsigned short porta_padrao, unsigned short& porta, std::string& caminho, std::string& consulta)
{
    porta = porta_padrao;
    caminho = "/";
    consulta.clear();
    std::string r = url;
    const auto  esq = r.find("://");
    if (esq != std::string::npos)
        r.erase(0, esq + 3);
    const auto barra = r.find('/');
    std::string hostporta = barra == std::string::npos ? r : r.substr(0, barra);
    std::string resto     = barra == std::string::npos ? std::string("/") : r.substr(barra);
    const auto dois = hostporta.rfind(':');
    if (dois != std::string::npos) {
        try {
            porta = static_cast<unsigned short>(std::stoi(hostporta.substr(dois + 1)));
        } catch (const std::exception&) {}
    }
    const auto q = resto.find('?');
    caminho  = q == std::string::npos ? resto : resto.substr(0, q);
    consulta = q == std::string::npos ? std::string() : resto.substr(q + 1);
    if (caminho.empty() || caminho[0] != '/')
        caminho = "/" + caminho;
}

std::string cor_rgba(const json& cor)
{
    // [R,G,B] ou [R,G,B,A] -> "RRGGBBAA"
    if (!cor.is_array() || cor.size() < 3)
        return "FFFFFFFF";
    unsigned char c[4] = {255, 255, 255, 255};
    for (std::size_t i = 0; i < 4 && i < cor.size(); ++i)
        if (cor[i].is_number())
            c[i] = static_cast<unsigned char>(std::max(0, std::min(255, cor[i].get<int>())));
    std::string h = hex(c, 4);
    std::transform(h.begin(), h.end(), h.begin(), ::toupper);
    return h;
}

double num(const json& o, const char* k)
{
    return o.is_object() && o.contains(k) && o[k].is_number() ? o[k].get<double>() : 0.0;
}

// Endereço IPv4 local usado para sair para a rede (sem enviar nada): base da varredura da sub-rede.
std::string ip_local()
{
    try {
        net::io_context       ioc;
        net::ip::udp::socket  s(ioc);
        s.connect(net::ip::udp::endpoint(net::ip::make_address("192.0.2.1"), 9));
        return s.local_endpoint().address().to_string();
    } catch (const std::exception&) {
        return {};
    }
}

} // namespace

AgentInfo FilunaAnycubicLanAgent::get_agent_info_static()
{
    return AgentInfo{FILUNA_ANYCUBIC_LAN_AGENT_ID, "Anycubic Kobra (Modo LAN de fábrica)", "1.0.0",
                     "Modo LAN da Anycubic: /info e /ctrl na porta 18910, MQTT/TLS no broker da impressora, envio HTTP"};
}

FilunaAnycubicLanAgent::FilunaAnycubicLanAgent(std::string)
{
    m_client_id = "filuna" + aleatorio(10, "abcdefghijklmnopqrstuvwxyz0123456789");
}

FilunaAnycubicLanAgent::~FilunaAnycubicLanAgent()
{
    start_discovery(false, false);
    disconnect_printer();
}

std::string FilunaAnycubicLanAgent::sign(const std::string& token, long long ts, const std::string& nonce)
{
    return md5_hex(md5_hex(token.substr(0, 16)) + std::to_string(ts) + nonce);
}

std::string FilunaAnycubicLanAgent::decrypt_ctrl(const std::string& info_b64, const std::string& token, const std::string& local_token)
{
    if (token.size() < 32)
        return {};
    const std::string cifrado = base64_decodificar(info_b64);
    if (cifrado.empty() || cifrado.size() % 16 != 0)
        return {};
    unsigned char chave[16], iv[16] = {0};
    std::copy(token.begin() + 16, token.begin() + 32, chave);
    for (std::size_t i = 0; i < 16 && i < local_token.size(); ++i)
        iv[i] = static_cast<unsigned char>(local_token[i]);
    std::string     claro(cifrado.size() + 16, '\0');
    int             n1 = 0, n2 = 0;
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    bool            ok  = EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, chave, iv) == 1 &&
              EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(&claro[0]), &n1,
                                reinterpret_cast<const unsigned char*>(cifrado.data()), int(cifrado.size())) == 1 &&
              EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(&claro[0]) + n1, &n2) == 1;
    EVP_CIPHER_CTX_free(ctx);
    if (!ok)
        return {};
    claro.resize(std::size_t(n1 + n2));
    return claro;
}

std::string FilunaAnycubicLanAgent::info_to_discovery_json(const json& info, const std::string& ip)
{
    if (!info.is_object() || !info.contains("cn") || !info["cn"].is_string() || info["cn"].get<std::string>().empty() ||
        !info.contains("modelId"))
        return {};
    const std::string modelo = info.value("modelName", std::string("Anycubic"));
    json              j;
    j["dev_id"]       = info["cn"].get<std::string>();
    j["dev_ip"]       = ip;
    j["dev_type"]     = modelo;
    j["dev_name"]     = modelo;
    j["dev_signal"]   = "0";
    j["connect_type"] = "lan";
    j["bind_state"]   = "free";
    j["sec_link"]     = "secure";
    j["ssdp_version"] = "v1";
    return j.dump();
}

json FilunaAnycubicLanAgent::to_push_status(const json& e)
{
    json p;
    p["command"]     = "push_status";
    p["sequence_id"] = "0";
    const json info  = e.value("info", json::object());
    json       temp  = info.value("temp", json::object());
    if (e.contains("tempature") && e["tempature"].is_object()) // (sic) relatório rápido de temperatura
        temp.merge_patch(e["tempature"]);
    json proj = info.contains("project") && info["project"].is_object() ? info["project"] : json::object();
    if (e.contains("print") && e["print"].is_object() && e["print"].contains("progress"))
        proj.merge_patch(e["print"]); // o progresso vem no relatório "print", mais novo que o "info"
    const std::string estado  = info.value("state", std::string("free"));
    const std::string pestado = proj.value("state", std::string());
    const int         pausa   = proj.contains("pause") && proj["pause"].is_number() ? proj["pause"].get<int>() : 0;
    std::string       gs      = "IDLE";
    if (pausa == 1 || pausa == 2 || pestado == "paused" || pestado == "pausing")
        gs = "PAUSE";
    else if (estado == "busy" && pestado != "finished" && pestado != "stoped" && pestado != "stopping")
        gs = (pestado == "preheating" || pestado == "auto_leveling" || pestado == "vibrating" || pestado == "flow_calibrating") ? "PREPARE" : "RUNNING";
    else if (pestado == "finished")
        gs = "FINISH";
    p["gcode_state"]       = gs;
    const bool ativo       = gs == "RUNNING" || gs == "PAUSE" || gs == "PREPARE";
    p["mc_percent"]        = ativo || gs == "FINISH" ? int(num(proj, "progress")) : 0;
    p["mc_remaining_time"] = ativo ? int(num(proj, "remain_time")) : 0; // já vem em minutos
    p["subtask_name"]      = ativo ? proj.value("filename", std::string()) : std::string();
    p["gcode_file"]        = p["subtask_name"];
    p["layer_num"]         = ativo ? int(num(proj, "curr_layer")) : 0;
    p["total_layer_num"]   = ativo ? int(num(proj, "total_layers")) : 0;
    p["nozzle_temper"]        = num(temp, "curr_nozzle_temp");
    p["nozzle_target_temper"] = num(temp, "target_nozzle_temp");
    p["bed_temper"]           = num(temp, "curr_hotbed_temp");
    p["bed_target_temper"]    = num(temp, "target_hotbed_temp");
    p["chamber_temper"]       = num(temp, "curr_chamber_temp");
    // ACE / ACE Pro -> unidades "ams"
    const json caixas = e.value("multiColorBox", json::object()).value("multi_color_box", json::array());
    if (caixas.is_array() && !caixas.empty()) {
        json unidades = json::array();
        int  agora    = 255;
        for (const auto& c : caixas) {
            if (!c.is_object())
                continue;
            const int id = c.contains("id") && c["id"].is_number() ? c["id"].get<int>() : 0;
            json      bandejas = json::array();
            for (const auto& s : c.value("slots", json::array())) {
                if (!s.is_object())
                    continue;
                json t;
                t["id"] = std::to_string(s.contains("index") && s["index"].is_number() ? s["index"].get<int>() : 0);
                const std::string tipo = s.value("type", std::string());
                if (!tipo.empty()) {
                    t["tray_type"]  = tipo;
                    t["tray_info_idx"] = ""; // sem este campo o DeviceManager descarta o tipo (PLA, PETG, ABS…)
                    t["tray_color"] = cor_rgba(s.value("color", json::array()));
                    {
                        // carretel com etiqueta da Anycubic traz o sku; sem etiqueta, a marca fica em branco
                        const std::string sku = s.value("sku", std::string());
                        t["tray_sub_brands"] = sku.empty() ? std::string() : "Anycubic " + sku;
                    }
                    if (s.contains("consumables_percent") && s["consumables_percent"].is_number())
                        t["remain"] = s["consumables_percent"].get<int>();
                }
                bandejas.push_back(t);
            }
            const int carregado = c.contains("loaded_slot") && c["loaded_slot"].is_number() ? c["loaded_slot"].get<int>() : -1;
            if (carregado >= 0)
                agora = id * 4 + carregado;
            json u{{"id", std::to_string(id)}, {"tray", bandejas}};
            if (c.contains("humidity") && c["humidity"].is_number())
                u["humidity_raw"] = std::to_string(c["humidity"].get<int>());
            if (c.contains("temp") && c["temp"].is_number())
                u["temp"] = std::to_string(c["temp"].get<int>());
            unidades.push_back(u);
        }
        p["ams"] = {{"ams", unidades}, {"ams_exist_bits", unidades.empty() ? "0" : "1"}, {"tray_now", std::to_string(agora)}};
    }
    return json{{"print", p}};
}

bool FilunaAnycubicLanAgent::aperto_de_mao(const std::string& ip, Sessao& s, std::string& erro)
{
    unsigned    status = 0;
    std::string corpo;
    if (!http_pedir(http::verb::get, ip, PORTA_INFO, "/info", "", {}, 4000, 8, status, corpo)) {
        erro = "A impressora não respondeu em " + ip + ". Confira se ela está ligada, na mesma rede e com o Modo LAN ativo.";
        return false;
    }
    json info = json::parse(corpo, nullptr, false);
    if (status != 200 || !info.is_object()) {
        erro = "O endereço " + ip + " respondeu, mas não como uma impressora Anycubic.";
        return false;
    }
    if (info.value("ctrlType", std::string()) == "cloud") {
        erro = "A impressora está em modo nuvem. Ative o Modo LAN na tela dela: Configurações → Rede → Modo LAN.";
        return false;
    }
    const std::string token = info.contains("token") && info["token"].is_string() ? info["token"].get<std::string>() : std::string();
    const std::string ctrl  = info.contains("ctrlInfoUrl") && info["ctrlInfoUrl"].is_string() ? info["ctrlInfoUrl"].get<std::string>() : std::string();
    if (token.size() < 32 || ctrl.empty() || !info.contains("modelId") || !info.contains("cn") || !info["cn"].is_string()) {
        erro = "Esta impressora não usa o aperto de mão do Modo LAN que o Filuna conhece (linha Kobra 3 / S1).";
        return false;
    }
    s.model_id = info["modelId"].is_string() ? info["modelId"].get<std::string>() : std::to_string(info["modelId"].get<long long>());
    s.serie    = info["cn"].get<std::string>();
    s.modelo   = info.value("modelName", std::string("Anycubic"));
    const long long   ts    = agora_ms();
    const std::string nonce = aleatorio(6, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    const std::string did   = aleatorio(32, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    unsigned short    porta;
    std::string       caminho, consulta;
    partes_da_url(ctrl, PORTA_INFO, porta, caminho, consulta);
    const std::string q = (consulta.empty() ? std::string() : consulta + "&") + "ts=" + std::to_string(ts) + "&nonce=" + nonce +
                          "&sign=" + sign(token, ts, nonce) + "&did=" + did;
    if (!http_pedir(http::verb::post, ip, porta, caminho + "?" + q, "", {}, 4000, 10, status, corpo)) {
        erro = "A impressora não respondeu ao pedido de controle local.";
        return false;
    }
    json r = json::parse(corpo, nullptr, false);
    if (!r.is_object() || !r.contains("code") || !r["code"].is_number() || r["code"].get<int>() != 200 || !r.contains("data") ||
        !r["data"].is_object() || !r["data"].contains("info") || !r["data"]["info"].is_string() || !r["data"].contains("token") ||
        !r["data"]["token"].is_string()) {
        erro = "A impressora recusou o controle local" +
               (r.is_object() && r.contains("message") && r["message"].is_string() ? ": " + r["message"].get<std::string>() : std::string(".")) ;
        return false;
    }
    const std::string claro = decrypt_ctrl(r["data"]["info"].get<std::string>(), token, r["data"]["token"].get<std::string>());
    json              d     = json::parse(claro, nullptr, false);
    if (!d.is_object() || !d.contains("broker") || !d["broker"].is_string() || !d.contains("username") || !d.contains("password") ||
        !d.contains("deviceId")) {
        erro = "Não foi possível ler as credenciais locais da impressora.";
        return false;
    }
    unsigned short porta_broker;
    std::string    c2, q2;
    partes_da_url(d["broker"].get<std::string>(), 9883, porta_broker, c2, q2);
    s.broker_host = ip;
    s.broker_port = porta_broker;
    s.usuario     = d["username"].is_string() ? d["username"].get<std::string>() : std::string();
    s.senha       = d["password"].is_string() ? d["password"].get<std::string>() : std::string();
    s.device_id   = d["deviceId"].is_string() ? d["deviceId"].get<std::string>() : std::string();
    if (s.usuario.empty() || s.device_id.empty()) {
        erro = "Credenciais locais da impressora incompletas.";
        return false;
    }
    return true;
}

bool FilunaAnycubicLanAgent::pedir(const std::string& tipo, const std::string& acao, const json& dados)
{
    std::lock_guard<std::recursive_mutex> l(m_mutex);
    if (!m_mqtt || !m_mqtt->connected())
        return false;
    json r{{"type", tipo}, {"action", acao}, {"timestamp", agora_ms()}, {"msgid", aleatorio(32, "0123456789abcdef")}, {"data", dados}};
    return m_mqtt->publish(std::string(PREFIXO) + "/web/printer/" + m_s.model_id + "/" + m_s.device_id + "/" + tipo, r.dump());
}

void FilunaAnycubicLanAgent::on_mqtt(const std::string& topic, const std::string& payload)
{
    // .../<tipo>/report
    const std::string fim = "/report";
    if (topic.size() <= fim.size() || topic.compare(topic.size() - fim.size(), fim.size(), fim) != 0)
        return;
    const std::string semfim = topic.substr(0, topic.size() - fim.size());
    const auto        barra  = semfim.rfind('/');
    if (barra == std::string::npos)
        return;
    const std::string tipo = semfim.substr(barra + 1);
    json              j    = json::parse(payload, nullptr, false);
    if (j.is_object() && tipo == "video") {
        // {"state":"initSuccess","code":200}: o vídeo ao vivo fica em http://<ip>:18088/flv (HTTP-FLV, H.264)
        const std::string st = j.value("state", std::string());
        std::string ip, serie;
        {
            std::lock_guard<std::recursive_mutex> l(m_mutex);
            ip = m_ip, serie = m_s.serie;
        }
        BOOST_LOG_TRIVIAL(info) << "FilunaAnycubicLanAgent: câmera: " << st;
        if (st == "initSuccess" && !ip.empty())
            Filuna::set_camera_url(serie, "http://" + ip + ":18088/flv");
        else if (st == "pushStopped")
            Filuna::set_camera_url(serie, std::string());
        return;
    }
    if (!j.is_object() || !j.contains("data") || !j["data"].is_object())
        return;
    const json& d = j["data"];
    json        retrato;
    if (tipo == "video")
        return; // tratado antes (data pode ser nulo)
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        if (tipo == "multiColorBox") {
            // relatórios parciais: mescla por (id da caixa, índice do slot)
            json& guardado = m_estado["multiColorBox"];
            if (!guardado.is_object())
                guardado = json::object();
            json& caixas = guardado["multi_color_box"];
            if (!caixas.is_array())
                caixas = json::array();
            for (const auto& nova : d.value("multi_color_box", json::array())) {
                if (!nova.is_object())
                    continue;
                const int id   = nova.contains("id") && nova["id"].is_number() ? nova["id"].get<int>() : 0;
                json*     alvo = nullptr;
                for (auto& c : caixas)
                    if (c.is_object() && c.value("id", 0) == id)
                        alvo = &c;
                if (alvo == nullptr) {
                    caixas.push_back(json{{"id", id}, {"slots", json::array()}});
                    alvo = &caixas.back();
                }
                for (auto it = nova.begin(); it != nova.end(); ++it)
                    if (it.key() != "slots" && !it.value().is_null())
                        (*alvo)[it.key()] = it.value();
                json& slots = (*alvo)["slots"];
                if (!slots.is_array())
                    slots = json::array();
                for (const auto& ns : nova.value("slots", json::array())) {
                    if (!ns.is_object())
                        continue;
                    const int idx = ns.contains("index") && ns["index"].is_number() ? ns["index"].get<int>() : 0;
                    bool      achou = false;
                    for (auto& s : slots)
                        if (s.is_object() && s.value("index", -1) == idx) {
                            s     = ns;
                            achou = true;
                        }
                    if (!achou)
                        slots.push_back(ns);
                }
            }
        } else if (tipo == "info" || tipo == "tempature" || tipo == "fan" || tipo == "light") {
            if (!m_estado[tipo].is_object())
                m_estado[tipo] = json::object();
            m_estado[tipo].merge_patch(d);
        } else if (tipo == "print") {
            if (d.contains("progress")) // três formatos no mesmo tópico: só o de progresso interessa
                m_estado["print"] = d;
        } else {
            return;
        }
        retrato = m_estado;
    }
    dispatch(to_push_status(retrato).dump());
}

void FilunaAnycubicLanAgent::dispatch(const std::string& payload)
{
    OnMessageFn   local, cloud;
    QueueOnMainFn q;
    std::string   dev;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        local = m_on_local_message;
        cloud = m_on_message;
        q     = m_queue_on_main;
        dev   = m_s.serie;
    }
    auto call = [local, cloud, dev, payload] {
        if (local)
            local(dev, payload);
        else if (cloud)
            cloud(dev, payload);
    };
    q ? q(call) : call();
}

void FilunaAnycubicLanAgent::dispatch_connect(int state, const std::string& msg)
{
    OnLocalConnectedFn fn;
    QueueOnMainFn      q;
    std::string        dev;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        fn  = m_on_local_connect;
        q   = m_queue_on_main;
        dev = m_s.serie;
    }
    if (!fn)
        return;
    auto call = [fn, state, dev, msg] { fn(state, dev, msg); };
    q ? q(call) : call();
}

int FilunaAnycubicLanAgent::send_message(std::string dev_id, std::string json_str, int qos, int flag)
{
    return send_message_to_printer(std::move(dev_id), std::move(json_str), qos, flag);
}

int FilunaAnycubicLanAgent::connect_printer(const PrinterConnectionParams& p)
{
    disconnect_printer();
    std::string ip = p.host;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        if (ip.empty() && !p.dev_id.empty()) {
            auto it = m_seen.find(p.dev_id);
            if (it != m_seen.end())
                ip = json::parse(it->second, nullptr, false).value("dev_ip", std::string());
        }
        m_estado = json::object();
        m_s      = Sessao();
        m_s.serie = p.dev_id; // para os avisos de falha chegarem na impressora certa
        m_ip     = ip;
    }
    if (ip.empty()) {
        dispatch_connect(ConnectStatusFailed, "Informe o IP da impressora.");
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    Sessao      s;
    std::string erro;
    if (!aperto_de_mao(ip, s, erro)) {
        BOOST_LOG_TRIVIAL(error) << "FilunaAnycubicLanAgent: " << erro;
        dispatch_connect(ConnectStatusFailed, erro);
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    if (!p.dev_id.empty())
        s.serie = p.dev_id; // o DeviceManager conhece a impressora por este id
    Filuna::MqttClient::Options o;
    o.host        = ip;
    o.port        = s.broker_port;
    o.tls         = true;
    o.verify_peer = false; // certificado próprio da impressora
    o.client_id   = m_client_id;
    o.username    = s.usuario;
    o.password    = s.senha;
    o.keepalive   = std::chrono::seconds(30);
    auto              c    = std::make_unique<Filuna::MqttClient>();
    const std::string base = std::string(PREFIXO) + "/printer/public/" + s.model_id + "/" + s.device_id;
    const bool        ok   = c->connect(o, {base + "/#"}, [this](const std::string& t, const std::string& pl) { on_mqtt(t, pl); },
                                        [this](const std::string& why) { dispatch_connect(ConnectStatusLost, why); }, erro);
    if (!ok) {
        BOOST_LOG_TRIVIAL(error) << "FilunaAnycubicLanAgent: MQTT " << ip << ":" << s.broker_port << " falhou: " << erro;
        {
            std::lock_guard<std::recursive_mutex> l(m_mutex);
            m_s = s;
        }
        dispatch_connect(ConnectStatusFailed, "A impressora recusou a conexão local (" + erro + ").");
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        m_s    = s;
        m_mqtt = std::move(c);
        if (m_selected.empty())
            m_selected = s.serie;
    }
    BOOST_LOG_TRIVIAL(info) << "FilunaAnycubicLanAgent: conectado a " << s.modelo << " em " << ip << " (modelId " << s.model_id << ")";
    pedir("info", "query", nullptr);
    pedir("tempature", "query", nullptr);
    pedir("multiColorBox", "getInfo", nullptr); // "query" não devolve a caixa; "getInfo" devolve
    // Câmera: "video/startCapture" abre http://<ip>:18088/flv (HTTP-FLV). A tela ainda não toca FLV (a página
    // local não pode buscar esse fluxo sem um repasse nativo), então a captura não é ligada aqui.
    dispatch_connect(ConnectStatusOk, "");
    if (m_on_connected)
        m_on_connected(base + "/info/report");
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaAnycubicLanAgent::disconnect_printer()
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

int FilunaAnycubicLanAgent::send_message_to_printer(std::string, std::string json_str, int, int)
{
    json cmd = json::parse(json_str, nullptr, false);
    if (!cmd.is_object())
        return BAMBU_NETWORK_ERR_INVALID_HANDLE;
    // Controles do Filuna ({"filuna": {...}}) -> print/update e light/control do Modo LAN
    if (cmd.contains("filuna") && cmd["filuna"].is_object()) {
        const json&       f  = cmd["filuna"];
        const std::string fc = f.value("command", std::string());
        const char*       chave = fc == "bico" ? "target_nozzle_temp" : fc == "mesa" ? "target_hotbed_temp" :
                                  fc == "ventilador" ? "fan_speed_pct" : fc == "ventilador_aux" ? "aux_fan_speed_pct" :
                                  fc == "ventilador_caixa" ? "box_fan_level" : fc == "velocidade" ? "print_speed_mode" : nullptr;
        bool feito = false;
        if (chave != nullptr) {
            int v = f.value("valor", 0);
            if (fc == "velocidade")
                v = std::max(1, std::min(3, v)); // 1 silencioso, 2 padrão, 3 esportivo
            feito = pedir("print", "update", json{{"taskid", "-1"}, {"settings", {{chave, v}}}});
        } else if (fc == "luz") {
            const bool on = f.value("ligar", false);
            feito = pedir("light", "control", json{{"type", 2}, {"status", on ? 1 : 0}, {"brightness", on ? 100 : 0}});
        } else if (fc == "secar") {
            // secagem na unidade de cores (ACE): multiColorBox/setDry, formato capturado ao vivo na referência
            const int  id = f.value("unidade", 0);
            const bool on = f.value("ligar", false);
            json       st = on ? json{{"status", 1}, {"target_temp", f.value("temperatura", 45)}, {"duration", f.value("minutos", 240)}} :
                                 json{{"status", 0}};
            feito = pedir("multiColorBox", "setDry", json{{"multi_color_box", json::array({json{{"id", id}, {"drying_status", st}}})}});
            pedir("multiColorBox", "getInfo", nullptr);
        } else
            return BAMBU_NETWORK_ERR_INVALID_HANDLE;
        BOOST_LOG_TRIVIAL(info) << "FilunaAnycubicLanAgent: controle " << fc << (feito ? " enviado" : " falhou");
        return feito ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    std::string c;
    if (cmd.contains("print") && cmd["print"].is_object())
        c = cmd["print"].value("command", std::string());
    else if (cmd.contains("pushing") && cmd["pushing"].is_object())
        c = cmd["pushing"].value("command", std::string());
    bool ok = false;
    if (c == "pause" || c == "resume" || c == "stop")
        ok = pedir("print", c, json{{"taskid", "-1"}});
    else if (c == "pushall") {
        ok = pedir("info", "query", nullptr);
        pedir("multiColorBox", "getInfo", nullptr);
    } else
        // comando que esta marca não tem (o programa manda vários por conta própria): ignora em silêncio.
        // Devolver "não suportado" abria a cada vez a janela "controle não suportado", que travava o programa.
        return BAMBU_NETWORK_SUCCESS;
    return ok ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_CONNECT_FAILED;
}

bool FilunaAnycubicLanAgent::start_discovery(bool start, bool)
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
        // Varre a sub-rede /24 do computador na porta 18910 e pergunta /info a quem atender.
        while (m_discovering) {
            const std::string meu = ip_local();
            const auto        pt  = meu.rfind('.');
            if (pt != std::string::npos) {
                const std::string        base = meu.substr(0, pt + 1);
                std::atomic<int>         proximo{1};
                std::vector<std::thread> trabalhadores;
                for (int w = 0; w < 32; ++w)
                    trabalhadores.emplace_back([this, base, &proximo] {
                        for (;;) {
                            const int i = proximo.fetch_add(1);
                            if (i > 254 || !m_discovering)
                                return;
                            const std::string ip = base + std::to_string(i);
                            unsigned          status = 0;
                            std::string       corpo;
                            if (!http_pedir(http::verb::get, ip, PORTA_INFO, "/info", "", {}, 350, 3, status, corpo) || status != 200)
                                continue;
                            const std::string js = info_to_discovery_json(json::parse(corpo, nullptr, false), ip);
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
                    });
                for (auto& t : trabalhadores)
                    t.join();
            }
            for (int k = 0; k < 100 && m_discovering; ++k) // nova varredura a cada ~10 s
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    return true;
}

int FilunaAnycubicLanAgent::bind_detect(std::string dev_ip, std::string, detectResult& d)
{
    unsigned    status = 0;
    std::string corpo;
    if (http_pedir(http::verb::get, dev_ip, PORTA_INFO, "/info", "", {}, 3000, 6, status, corpo) && status == 200) {
        json              info = json::parse(corpo, nullptr, false);
        const std::string js   = info_to_discovery_json(info, dev_ip);
        if (!js.empty()) {
            json j = json::parse(js);
            {
                std::lock_guard<std::recursive_mutex> l(m_mutex);
                m_seen[j.value("dev_id", "")] = js;
            }
            d.dev_id = j.value("dev_id", ""), d.model_id = j.value("dev_type", ""), d.dev_name = j.value("dev_name", "");
            d.connect_type = "lan", d.bind_state = "free";
            d.result_msg   = info.value("ctrlType", std::string()) == "cloud" ? "modo nuvem: ative o Modo LAN na impressora" : "ok";
            return BAMBU_NETWORK_SUCCESS;
        }
    }
    d.result_msg = "a impressora não respondeu na porta do Modo LAN (18910)";
    return BAMBU_NETWORK_ERR_CONNECT_FAILED;
}

std::string FilunaAnycubicLanAgent::get_user_selected_machine() { std::lock_guard<std::recursive_mutex> l(m_mutex); return m_selected; }
int FilunaAnycubicLanAgent::set_user_selected_machine(std::string d) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_selected = std::move(d); return 0; }

bool FilunaAnycubicLanAgent::enviar_arquivo(const std::string& local, const std::string& remoto, OnUpdateStatusFn update_fn,
                                            WasCancelledFn cancel_fn, std::string& erro)
{
    std::string ip;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        ip = m_ip;
    }
    unsigned    status = 0;
    std::string corpo;
    if (!http_pedir(http::verb::get, ip, PORTA_INFO, "/info", "", {}, 4000, 8, status, corpo) || status != 200) {
        erro = "a impressora não respondeu ao pedido de envio";
        return false;
    }
    json info = json::parse(corpo, nullptr, false);
    if (!info.is_object() || !info.contains("fileUploadurl") || !info["fileUploadurl"].is_string()) {
        erro = "a impressora não informou onde receber o arquivo";
        return false;
    }
    unsigned short porta;
    std::string    caminho, consulta;
    partes_da_url(info["fileUploadurl"].get<std::string>(), PORTA_INFO, porta, caminho, consulta);
    std::ifstream in(local, std::ios::binary);
    if (!in) {
        erro = "não foi possível abrir " + local;
        return false;
    }
    std::string arquivo((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (cancel_fn && cancel_fn()) {
        erro = "envio cancelado";
        return false;
    }
    const std::string limite = "----FilunaAnycubic" + aleatorio(16, "0123456789abcdef");
    std::string       form;
    form.reserve(arquivo.size() + 600);
    form += "--" + limite + "\r\nContent-Disposition: form-data; name=\"filename\"\r\n\r\n" + remoto + "\r\n";
    form += "--" + limite + "\r\nContent-Disposition: form-data; name=\"gcode\"; filename=\"" + remoto +
            "\"\r\nContent-Type: application/octet-stream\r\n\r\n";
    form += arquivo;
    form += "\r\n--" + limite + "--\r\n";
    if (update_fn)
        update_fn(PrintingStageUpload, 10, "");
    if (!http_pedir(http::verb::post, ip, porta, caminho + (consulta.empty() ? std::string() : "?" + consulta), form,
                    {{"Content-Type", "multipart/form-data; boundary=" + limite}, {"X-File-Length", std::to_string(arquivo.size())}},
                    6000, 900, status, corpo)) {
        erro = "falha no envio do arquivo para a impressora";
        return false;
    }
    json r = json::parse(corpo, nullptr, false);
    const int codigo = r.is_object() && r.contains("code") && r["code"].is_number() ? r["code"].get<int>() : 0;
    if (status != 200 || (codigo != 0 && codigo != 200)) {
        erro = "a impressora recusou o arquivo (HTTP " + std::to_string(status) + ", código " + std::to_string(codigo) + ")";
        return false;
    }
    if (update_fn)
        update_fn(PrintingStageUpload, 100, "");
    return true;
}

int FilunaAnycubicLanAgent::start_local_print(PrintParams p, OnUpdateStatusFn update_fn, WasCancelledFn cancel_fn)
{
    const std::string remoto = p.dst_file.empty() ? boost::filesystem::path(p.filename).filename().string() : p.dst_file;
    std::string       erro;
    if (update_fn)
        update_fn(PrintingStageCreate, 0, "");
    if (!enviar_arquivo(p.filename, remoto, update_fn, cancel_fn, erro)) {
        BOOST_LOG_TRIVIAL(error) << "FilunaAnycubicLanAgent: envio: " << erro;
        if (update_fn)
            update_fn(PrintingStageERROR, BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED, erro);
        return BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED;
    }
    std::uint64_t     tamanho = 0;
    const std::string md5     = md5_arquivo(p.filename, tamanho);
    // ams_mapping do Slicer: posição = filamento do projeto, valor = bandeja (-1 = carretel externo)
    json mapa = json::parse(p.ams_mapping.empty() ? std::string("[]") : p.ams_mapping, nullptr, false);
    json caixa = json::array();
    json slots_atual;
    {
        std::lock_guard<std::recursive_mutex> l(m_mutex);
        const json caixas = m_estado.value("multiColorBox", json::object()).value("multi_color_box", json::array());
        slots_atual = caixas.is_array() && !caixas.empty() && caixas[0].is_object() ? caixas[0].value("slots", json::array()) : json::array();
    }
    if (mapa.is_array())
        for (std::size_t i = 0; i < mapa.size(); ++i)
            if (mapa[i].is_number_integer() && mapa[i].get<int>() >= 0) {
                const int idx = mapa[i].get<int>() % 4;
                json      cor = json::array({255, 255, 255, 255});
                std::string material;
                for (const auto& s : slots_atual)
                    if (s.is_object() && s.value("index", -1) == idx) {
                        material = s.value("type", std::string());
                        if (s.contains("color") && s["color"].is_array() && s["color"].size() >= 3)
                            cor = json::array({s["color"][0], s["color"][1], s["color"][2], 255});
                    }
                caixa.push_back({{"paint_index", int(i)}, {"ams_index", idx}, {"paint_color", cor}, {"ams_color", cor}, {"material_type", material}});
            }
    json dados{{"taskid", aleatorio(9, "123456789")},
               {"filename", remoto},
               {"filepath", "/"},
               {"filetype", 1},
               {"filesize", tamanho},
               {"md5", md5},
               {"ams_settings", {{"use_ams", !caixa.empty()}, {"ams_box_mapping", caixa}}},
               {"task_settings",
                {{"auto_leveling", p.task_bed_leveling ? 1 : 0},
                 {"vibration_compensation", p.task_vibration_cali ? 1 : 0},
                 {"flow_calibration", p.task_flow_cali ? 1 : 0},
                 {"timelapse", {{"status", p.task_record_timelapse ? 1 : 0}}},
                 {"ai_settings", {{"status", 0}}},
                 {"dry_mode", 0}}}};
    if (update_fn)
        update_fn(PrintingStageSending, 0, "");
    if (!pedir("print", "start", dados)) {
        if (update_fn)
            update_fn(PrintingStageERROR, BAMBU_NETWORK_ERR_CONNECT_FAILED, "impressora desconectada");
        return BAMBU_NETWORK_ERR_CONNECT_FAILED;
    }
    if (update_fn)
        update_fn(PrintingStageFinished, 0, "");
    return BAMBU_NETWORK_SUCCESS;
}

int FilunaAnycubicLanAgent::start_print(PrintParams p, OnUpdateStatusFn u, WasCancelledFn c, OnWaitFn) { return start_local_print(std::move(p), std::move(u), std::move(c)); }
int FilunaAnycubicLanAgent::start_local_print_with_record(PrintParams p, OnUpdateStatusFn u, WasCancelledFn c, OnWaitFn) { return start_local_print(std::move(p), std::move(u), std::move(c)); }

int FilunaAnycubicLanAgent::start_send_gcode_to_sdcard(PrintParams p, OnUpdateStatusFn u, WasCancelledFn c, OnWaitFn)
{
    std::string       erro;
    const std::string remoto = p.dst_file.empty() ? boost::filesystem::path(p.filename).filename().string() : p.dst_file;
    const bool        ok     = enviar_arquivo(p.filename, remoto, u, c, erro);
    if (u)
        u(ok ? PrintingStageFinished : PrintingStageERROR, ok ? 0 : BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED, erro);
    return ok ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_FTP_UPLOAD_FAILED;
}

int FilunaAnycubicLanAgent::start_sdcard_print(PrintParams p, OnUpdateStatusFn u, WasCancelledFn)
{
    const std::string nome = p.dst_file.empty() ? p.ftp_file : p.dst_file;
    const bool ok = pedir("print", "start", json{{"taskid", aleatorio(9, "123456789")}, {"filename", nome}, {"filepath", "/"}, {"filetype", 1},
                                                  {"ams_settings", {{"use_ams", false}, {"ams_box_mapping", json::array()}}}});
    if (u)
        u(ok ? PrintingStageFinished : PrintingStageERROR, 0, "");
    return ok ? BAMBU_NETWORK_SUCCESS : BAMBU_NETWORK_ERR_CONNECT_FAILED;
}

int FilunaAnycubicLanAgent::set_on_ssdp_msg_fn(OnMsgArrivedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_ssdp = std::move(fn); return 0; }
int FilunaAnycubicLanAgent::set_on_printer_connected_fn(OnPrinterConnectedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_connected = std::move(fn); return 0; }
int FilunaAnycubicLanAgent::set_on_message_fn(OnMessageFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_message = std::move(fn); return 0; }
int FilunaAnycubicLanAgent::set_on_local_connect_fn(OnLocalConnectedFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_local_connect = std::move(fn); return 0; }
int FilunaAnycubicLanAgent::set_on_local_message_fn(OnMessageFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_on_local_message = std::move(fn); return 0; }
int FilunaAnycubicLanAgent::set_queue_on_main_fn(QueueOnMainFn fn) { std::lock_guard<std::recursive_mutex> l(m_mutex); m_queue_on_main = std::move(fn); return 0; }

} // namespace Slic3r
