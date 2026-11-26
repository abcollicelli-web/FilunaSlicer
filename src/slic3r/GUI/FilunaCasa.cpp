#include <boost/asio.hpp> // no Windows o boost precisa vir antes dos cabeçalhos do wxWidgets
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/crc.hpp>
#ifdef _WIN32
#include <iphlpapi.h>
#include <wlanapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wlanapi.lib")
#endif

#include "FilunaCasa.hpp"
#include "FilunaPanel.hpp"
#include "GUI_App.hpp"
#include "libslic3r/AppConfig.hpp"
#include "slic3r/Utils/FilunaMqttClient.hpp"
#include "slic3r/Utils/FilunaRede.hpp"
#include "slic3r/Utils/Http.hpp"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>

#include <boost/log/trivial.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <regex>
#include <set>
#include <thread>
#include <vector>

namespace Slic3r { namespace GUI { namespace FilunaCasa {

namespace net   = boost::asio;
namespace beast = boost::beast;
namespace ws    = boost::beast::websocket;
using udp       = net::ip::udp;
using tcp       = net::ip::tcp;
using namespace std::chrono_literals;

namespace {

bool tapo_pedido(const std::string& ip, const json& pedido, json& resultado, std::string& erro); // KLAP, mais abaixo

// ---------------------------------------------------------------- utilidades

std::string minusc(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return s;
}

bool tem(const std::string& s, const char* trecho) { return minusc(s).find(trecho) != std::string::npos; }

std::string id_seguro(const std::string& s)
{
    std::string o;
    for (unsigned char c : minusc(s))
        o += std::isalnum(c) ? char(c) : '_';
    return o.substr(0, 60);
}

// Recebe pacotes UDP até o prazo; 'f(pacote, ip_de_origem, porta_de_origem)'.
template <class F> void receber(udp::socket& s, std::chrono::milliseconds prazo, F f)
{
    boost::system::error_code ec;
    s.non_blocking(true, ec);
    const auto           fim = std::chrono::steady_clock::now() + prazo;
    std::array<char, 9216> buf{};
    while (std::chrono::steady_clock::now() < fim) {
        udp::endpoint de;
        const std::size_t n = s.receive_from(net::buffer(buf), de, 0, ec);
        if (ec) { // would_block, ou ICMP de porta fechada no Windows: espera e tenta de novo
            std::this_thread::sleep_for(15ms);
            continue;
        }
        f(std::string(buf.data(), n), de.address().to_string(), de.port());
    }
}

bool http_get(const std::string& url, std::string& corpo, long max_s = 4)
{
    bool ok = false;
    corpo.clear();
    Http::get(url)
        .timeout_connect(2)
        .timeout_max(max_s)
        .on_complete([&](std::string b, unsigned st) { corpo = std::move(b); ok = st < 400; })
        .on_error([&](std::string b, std::string erro, unsigned) { corpo = b.empty() ? erro : b; })
        .perform_sync();
    return ok;
}

std::string tag_xml(const std::string& xml, const char* nome)
{
    const std::regex  r(std::string("<") + nome + "[^>]*>([^<]*)</" + nome + ">", std::regex::icase);
    std::smatch       m;
    if (!std::regex_search(xml, m, r))
        return {};
    std::string v = m[1].str();
    for (const auto& [de, para] : std::vector<std::pair<std::string, std::string>>{{"&amp;", "&"}, {"&quot;", "\""}, {"&apos;", "'"}, {"&lt;", "<"}, {"&gt;", ">"}})
        for (std::size_t p; (p = v.find(de)) != std::string::npos;)
            v.replace(p, de.size(), para);
    return v;
}

// ---------------------------------------------------------------- coleta

// Os aparelhos achados, um por endereço. Cada busca roda numa thread; quem acha primeiro fica, a menos que a nova
// identificação seja mais específica (ex.: o SSDP diz "TV LG" e o mDNS só "Google Cast").
struct Coleta
{
    std::mutex            m;
    std::map<std::string, json> por_ip;
    int                   respostas_multicast = 0;

    static int peso(const std::string& tipo)
    {
        static const std::map<std::string, int> p = {{"roteador", 9}, {"tv_lg", 8}, {"tv_samsung", 8}, {"home_assistant", 8},
                                                     {"tomada", 7},   {"luz", 7},   {"smart_life", 6}, {"tv_android", 5},
                                                     {"voz_google", 5}, {"tv_outra", 4}};
        auto it = p.find(tipo);
        return it == p.end() ? 0 : it->second;
    }
    void por(json a)
    {
        const std::string ip = a.value("ip", std::string());
        if (ip.empty())
            return;
        std::lock_guard<std::mutex> g(m);
        auto it = por_ip.find(ip);
        const auto prot = [](const json& x) { return x.value("dados", json::object()).value("protocolo", std::string()); };
        // mesma TV anunciada pelo Google Cast e pelo app Filuna TV: vale a do app (sabe receber os avisos)
        if (it == por_ip.end() || peso(a.value("tipo", std::string())) > peso(it->second.value("tipo", std::string())) ||
            (peso(a.value("tipo", std::string())) == peso(it->second.value("tipo", std::string())) && !prot(a).empty() && prot(it->second).empty()))
            por_ip[ip] = std::move(a);
    }
    void multicast()
    {
        std::lock_guard<std::mutex> g(m);
        ++respostas_multicast;
    }
};

json aparelho(const std::string& chave, const std::string& tipo, const std::string& nome, const std::string& marca,
              const std::string& modelo, const std::string& ip, const std::string& via, json fatos = json::array())
{
    // {{"Sistema", "x"}, {"Precisa", "y"}} vira objeto no nlohmann::json; a tela espera a lista de pares [rótulo, valor]
    if (fatos.is_object()) {
        json pares = json::array();
        for (const auto& [rotulo, valor] : fatos.items())
            pares.push_back(json::array({rotulo, valor}));
        fatos = std::move(pares);
    }
    std::string estado = "configurar";
    if (tipo == "tv_samsung" || tipo == "voz_google" || tipo == "tv_outra")
        estado = "incompativel";
    return {{"chave", chave}, {"tipo", tipo}, {"nome", nome}, {"marca", marca}, {"modelo", modelo}, {"ip", ip},
            {"via", "Filuna Slicer · " + via}, {"estado", estado}, {"fatos", std::move(fatos)}, {"dados", json::object()}};
}

// ---------------------------------------------------------------- SSDP (UPnP)

void buscar_ssdp(Coleta& c)
{
    net::io_context            io;
    udp::socket                s(io);
    boost::system::error_code  ec;
    s.open(udp::v4(), ec);
    if (ec)
        return;
    s.bind(udp::endpoint(udp::v4(), 0), ec);
    s.set_option(net::ip::multicast::hops(4), ec);
    const udp::endpoint grupo(net::ip::make_address_v4("239.255.255.250"), 1900);
    for (const char* st : {"ssdp:all", "urn:lge-com:service:webos-second-screen:1", "urn:schemas-upnp-org:device:InternetGatewayDevice:1",
                           "urn:dial-multiscreen-org:service:dial:1"}) {
        const std::string msg = std::string("M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: 2\r\nST: ") + st + "\r\n\r\n";
        s.send_to(net::buffer(msg), grupo, 0, ec);
    }
    struct Resp { std::string server, location, usn; std::set<std::string> st; };
    std::map<std::string, Resp> por_ip;
    receber(s, 3500ms, [&](const std::string& pkt, const std::string& ip, unsigned short) {
        c.multicast();
        Resp& r = por_ip[ip];
        std::size_t ini = 0;
        while (ini < pkt.size()) {
            std::size_t fim = pkt.find("\r\n", ini);
            if (fim == std::string::npos)
                fim = pkt.size();
            const std::string linha = pkt.substr(ini, fim - ini);
            ini = fim + 2;
            const std::size_t dp = linha.find(':');
            if (dp == std::string::npos)
                continue;
            const std::string chave = minusc(linha.substr(0, dp));
            std::string       valor = linha.substr(dp + 1);
            valor.erase(0, valor.find_first_not_of(" \t"));
            if (chave == "server") r.server = valor;
            else if (chave == "location" && r.location.empty()) r.location = valor;
            else if (chave == "st") r.st.insert(minusc(valor));
            else if (chave == "usn" && r.usn.empty()) r.usn = valor;
        }
    });
    int buscadas = 0;
    for (auto& [ip, r] : por_ip) {
        std::string xml;
        if (!r.location.empty() && buscadas++ < 30)
            http_get(r.location, xml, 3);
        const std::string fab = tag_xml(xml, "manufacturer"), modelo = tag_xml(xml, "modelName"), nome = tag_xml(xml, "friendlyName"),
                          tipo_dev = tag_xml(xml, "deviceType");
        std::string sts;
        for (const auto& x : r.st)
            sts += x + " ";
        const std::string uuid = r.usn.substr(0, r.usn.find("::"));
        const std::string chave = uuid.empty() ? "ssdp:" + ip : uuid;
        const json        fatos_base = json::array({{"Fabricante", fab.empty() ? "não informado" : fab}});
        if (tem(sts, "webos-second-screen") || (tem(fab, "lg electronics") && (tem(modelo, "webos") || tem(r.server, "webos") || tem(tipo_dev, "mediarenderer"))))
            c.por(aparelho(chave, "tv_lg", nome.empty() ? "TV LG" : nome, "LG", modelo.empty() ? "webOS" : modelo, ip, "a TV se anunciou na rede",
                           {{"Sistema", "webOS (aceita avisos na tela)"}, {"Pareamento", "uma vez, na própria TV"}, {"Requisito", "LG Connect Apps ligado"}}));
        else if (tem(fab, "samsung") && (tem(tipo_dev, "mediarenderer") || tem(tipo_dev, "tv") || tem(r.server, "tv")))
            c.por(aparelho(chave, "tv_samsung", nome.empty() ? "TV Samsung" : nome, "Samsung", modelo.empty() ? "Tizen" : modelo, ip, "rede local",
                           {{"Sistema", "Tizen"}, {"Avisos na tela", "a Samsung não libera para outros apps"}, {"Alternativa", "Fire TV Stick ou Google TV no HDMI"}}));
        else if (tem(tipo_dev, "internetgatewaydevice") || tem(sts, "internetgatewaydevice"))
            c.por(aparelho("roteador:" + ip, "roteador", "Roteador Wi-Fi", fab.empty() ? "Roteador" : fab, modelo, ip, "gateway da rede (UPnP)"));
        else if (tem(fab, "amazon"))
            c.por(aparelho(chave, "tv_android", nome.empty() ? "Fire TV" : nome, "Amazon", modelo.empty() ? "Fire TV" : modelo, ip, "rede local",
                           {{"Sistema", "Fire TV"}, {"Precisa", "app Filuna TV instalado na TV"}, {"Mostra", "texto, capa e foto da câmera"}}));
        else if (tem(fab, "roku"))
            c.por(aparelho(chave, "tv_outra", nome.empty() ? "TV Roku" : nome, "Roku", modelo, ip, "rede local",
                           {{"Sistema", "Roku"}, {"Avisos na tela", "ainda não"}, {"Alternativa", "Fire TV Stick ou Google TV no HDMI"}}));
        else if (tem(fab, "philips") && tem(modelo, "hue"))
            c.por(aparelho(chave, "luz", nome.empty() ? "Philips Hue" : nome, "Philips Hue", modelo, ip, "rede local",
                           {{"Cores", "sim"}, {"Conta", "ponte Hue"}, {"Uso no Filuna", "pelo Home Assistant, por enquanto"}}));
        else if (tem(tipo_dev, "mediarenderer") && (tem(fab, "sony") || tem(fab, "tcl") || tem(fab, "philco") || tem(fab, "philips") ||
                                                     tem(fab, "hisense") || tem(fab, "xiaomi") || tem(fab, "aoc") || tem(fab, "multilaser")))
            c.por(aparelho(chave, "tv_android", nome.empty() ? "TV " + fab : nome, fab, modelo, ip, "rede local",
                           {{"Sistema", "Android TV ou Google TV (confira)"}, {"Precisa", "app Filuna TV instalado na TV"}, {"Mostra", "texto, capa e foto da câmera"}}));
    }
}

// ---------------------------------------------------------------- mDNS

void dns_nome(std::string& out, const std::string& nome)
{
    std::size_t ini = 0;
    while (ini < nome.size()) {
        std::size_t fim = nome.find('.', ini);
        if (fim == std::string::npos)
            fim = nome.size();
        out += char(fim - ini);
        out += nome.substr(ini, fim - ini);
        ini = fim + 1;
    }
    out += '\0';
}

// Lê um nome DNS (com compressão) a partir de 'pos'; 'pos' avança só no trecho fora dos ponteiros.
bool ler_nome(const std::string& p, std::size_t& pos, std::string& nome)
{
    nome.clear();
    std::size_t i = pos, saltos = 0;
    bool        pulou = false;
    while (i < p.size()) {
        const unsigned char n = static_cast<unsigned char>(p[i]);
        if (n == 0) {
            if (!pulou)
                pos = i + 1;
            return true;
        }
        if ((n & 0xC0) == 0xC0) {
            if (i + 1 >= p.size() || ++saltos > 20)
                return false;
            if (!pulou)
                pos = i + 2;
            pulou = true;
            i     = ((n & 0x3F) << 8) | static_cast<unsigned char>(p[i + 1]);
            continue;
        }
        if (i + 1 + n > p.size())
            return false;
        if (!nome.empty())
            nome += '.';
        nome += p.substr(i + 1, n);
        i += 1 + n;
    }
    return false;
}

struct Registro { std::string nome; unsigned tipo = 0; std::string dados; std::size_t ini = 0; };

std::vector<Registro> ler_registros(const std::string& p)
{
    std::vector<Registro> out;
    if (p.size() < 12)
        return out;
    auto u16 = [&p](std::size_t i) { return unsigned((static_cast<unsigned char>(p[i]) << 8) | static_cast<unsigned char>(p[i + 1])); };
    const unsigned qd = u16(4), total = u16(6) + u16(8) + u16(10);
    std::size_t    pos = 12;
    std::string    nome;
    for (unsigned q = 0; q < qd; ++q) {
        if (!ler_nome(p, pos, nome) || pos + 4 > p.size())
            return out;
        pos += 4;
    }
    for (unsigned r = 0; r < total; ++r) {
        Registro reg;
        if (!ler_nome(p, pos, reg.nome) || pos + 10 > p.size())
            return out;
        reg.tipo                = u16(pos);
        const unsigned tamanho  = u16(pos + 8);
        pos += 10;
        if (pos + tamanho > p.size())
            return out;
        reg.ini   = pos;
        reg.dados = p.substr(pos, tamanho);
        pos += tamanho;
        out.push_back(std::move(reg));
    }
    return out;
}

std::map<std::string, std::string> ler_txt(const std::string& d)
{
    std::map<std::string, std::string> kv;
    std::size_t                        i = 0;
    while (i < d.size()) {
        const std::size_t n = static_cast<unsigned char>(d[i]);
        const std::string s = d.substr(i + 1, n);
        i += 1 + n;
        const std::size_t eq = s.find('=');
        if (eq != std::string::npos)
            kv[minusc(s.substr(0, eq))] = s.substr(eq + 1);
    }
    return kv;
}

void buscar_mdns(Coleta& c)
{
    static const std::vector<std::string> SERVICOS = {"_googlecast._tcp.local", "_androidtvremote2._tcp.local", "_amzn-wplay._tcp.local",
                                                      "_home-assistant._tcp.local", "_shelly._tcp.local", "_http._tcp.local",
                                                      "_hue._tcp.local", "_filunatv._tcp.local"};
    std::string q;
    q += std::string("\0\0\0\0\0", 4); // id 0, flags 0
    q += char(SERVICOS.size() >> 8);
    q += char(SERVICOS.size() & 0xFF);
    q += std::string(6, '\0');
    for (const auto& sv : SERVICOS) {
        dns_nome(q, sv);
        q += std::string("\x00\x0c\x80\x01", 4); // PTR, classe IN com o bit de resposta direta (QU)
    }
    net::io_context           io;
    udp::socket               s(io);
    boost::system::error_code ec;
    s.open(udp::v4(), ec);
    if (ec)
        return;
    s.bind(udp::endpoint(udp::v4(), 0), ec);
    s.set_option(net::ip::multicast::hops(255), ec);
    const udp::endpoint grupo(net::ip::make_address_v4("224.0.0.251"), 5353);
    s.send_to(net::buffer(q), grupo, 0, ec);
    struct Inst { std::string servico, ip; std::map<std::string, std::string> txt; };
    std::map<std::string, Inst> inst;
    auto tratar = [&](const std::string& pkt, const std::string& ip, unsigned short) {
        c.multicast();
        const auto regs = ler_registros(pkt);
        for (const auto& r : regs)
            if (r.tipo == 12) { // PTR: serviço -> instância
                std::size_t pos = r.ini;
                std::string alvo;
                if (ler_nome(pkt, pos, alvo) && std::find(SERVICOS.begin(), SERVICOS.end(), minusc(r.nome)) != SERVICOS.end()) {
                    inst[alvo].servico = minusc(r.nome);
                    inst[alvo].ip      = ip;
                }
            }
        for (const auto& r : regs)
            if (r.tipo == 16 && inst.count(r.nome)) // TXT
                inst[r.nome].txt = ler_txt(r.dados);
    };
    receber(s, 1200ms, tratar);
    s.send_to(net::buffer(q), grupo, 0, ec); // segunda pergunta: aparelho que dormia no Wi-Fi acorda na primeira
    receber(s, 2300ms, tratar);

    for (const auto& par : inst) {
        const std::string& nome   = par.first;
        const Inst&        i      = par.second;
        const std::string  rotulo = nome.substr(0, nome.find("._"));
        const auto        txt    = [&i](const char* k) { auto it = i.txt.find(k); return it == i.txt.end() ? std::string() : it->second; };
        if (i.servico == "_googlecast._tcp.local") {
            const std::string md = txt("md"), fn = txt("fn").empty() ? rotulo : txt("fn");
            if (tem(md, "home") || tem(md, "nest") || tem(md, "speaker") || tem(md, "audio"))
                c.por(aparelho("cast:" + txt("id"), "voz_google", fn, "Google", md, i.ip, "anúncio na rede (mDNS)",
                               {{"Aparelho", "alto-falante Google"}, {"Avisos", "o Google ainda não abriu para outros apps"}, {"Alternativa", "Amazon Echo"}}));
            else
                c.por(aparelho("cast:" + txt("id"), "tv_android", fn, tem(md, "chromecast") ? "Google" : md, md, i.ip, "anúncio na rede (mDNS)",
                               {{"Sistema", "Google TV ou Android TV"}, {"Precisa", "app Filuna TV instalado na TV"}, {"Mostra", "texto, capa e foto da câmera"}}));
        } else if (i.servico == "_androidtvremote2._tcp.local")
            c.por(aparelho("atv:" + rotulo, "tv_android", rotulo, "Android TV", "Google TV", i.ip, "anúncio na rede (mDNS)",
                           {{"Sistema", "Android TV ou Google TV"}, {"Precisa", "app Filuna TV instalado na TV"}, {"Mostra", "texto, capa e foto da câmera"}}));
        else if (i.servico == "_amzn-wplay._tcp.local")
            c.por(aparelho("fire:" + rotulo, "tv_android", txt("n").empty() ? "Fire TV" : txt("n"), "Amazon", "Fire TV", i.ip, "anúncio na rede (mDNS)",
                           {{"Sistema", "Fire TV"}, {"Precisa", "app Filuna TV instalado na TV"}, {"Mostra", "texto, capa e foto da câmera"}}));
        else if (i.servico == "_home-assistant._tcp.local") {
            json a = aparelho("ha:" + (txt("uuid").empty() ? i.ip : txt("uuid")), "home_assistant", txt("location_name").empty() ? "Home Assistant" : txt("location_name"),
                              "Home Assistant", txt("version").empty() ? "servidor da casa" : "versão " + txt("version"), i.ip, "anúncio na rede (mDNS)",
                              {{"O que o Filuna faz", "publica todas as impressoras lá"}, {"Protocolo", "MQTT, na rede da casa"}, {"Endereço", txt("base_url")}});
            c.por(std::move(a));
        } else if (i.servico == "_shelly._tcp.local" || (i.servico == "_http._tcp.local" && tem(rotulo, "shelly"))) {
            const std::string modelo = txt("app").empty() ? rotulo.substr(0, rotulo.rfind('-')) : txt("app");
            json a = aparelho("shelly:" + minusc(rotulo), "tomada", rotulo, "Shelly", modelo, i.ip, "anúncio na rede (mDNS)",
                              {{"Conta", "não precisa (controle na rede da casa)"}, {"Geração", txt("gen").empty() ? "1" : txt("gen")}});
            a["dados"] = {{"protocolo", "shelly"}, {"geracao", txt("gen").empty() ? "1" : txt("gen")}};
            c.por(std::move(a));
        } else if (i.servico == "_hue._tcp.local") {
            json a = aparelho("hue:" + rotulo, "luz", rotulo, "Philips Hue", "ponte Hue", i.ip, "anúncio na rede (mDNS)",
                              {{"Cores", "sim, todas as luzes da ponte"}, {"Conta", "não precisa (botão da ponte)"}, {"Uso no Filuna", "piscar na cor do aviso"}});
            a["dados"] = {{"protocolo", "hue"}};
            c.por(std::move(a));
        } else if (i.servico == "_filunatv._tcp.local") {
            json a = aparelho("filunatv:" + minusc(rotulo), "tv_android", txt("nome").empty() ? rotulo : txt("nome"), "Filuna TV", txt("modelo").empty() ? "app Filuna TV" : txt("modelo"),
                              i.ip, "app Filuna TV na rede",
                              {{"App", "Filuna TV instalado"}, {"Mostra", "aviso na tela, com cor e estilo"}, {"Pareamento", "código de 6 dígitos na TV"}});
            a["dados"] = {{"protocolo", "filuna_tv"}};
            c.por(std::move(a));
        }
    }
}

// ---------------------------------------------------------------- TP-Link (Kasa 9999 e Tapo 20002)

std::string kasa_cifra(const std::string& s, bool decifrar)
{
    std::string   o = s;
    unsigned char k = 171;
    for (char& ch : o) {
        const unsigned char b = static_cast<unsigned char>(ch);
        ch                    = char(b ^ k);
        k                     = decifrar ? b : static_cast<unsigned char>(ch);
    }
    return o;
}

std::string chave_publica_rsa()
{
    std::string    pem;
    EVP_PKEY*      chave = nullptr;
    EVP_PKEY_CTX*  ctx   = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (ctx != nullptr && EVP_PKEY_keygen_init(ctx) > 0 && EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 1024) > 0 && EVP_PKEY_keygen(ctx, &chave) > 0) {
        BIO* bio = BIO_new(BIO_s_mem());
        if (bio != nullptr && PEM_write_bio_PUBKEY(bio, chave) > 0) {
            char*      dados = nullptr;
            const long n     = BIO_get_mem_data(bio, &dados);
            pem.assign(dados, static_cast<std::size_t>(n));
        }
        BIO_free(bio);
    }
    EVP_PKEY_free(chave);
    EVP_PKEY_CTX_free(ctx);
    return pem;
}

void buscar_tplink(Coleta& c)
{
    net::io_context           io;
    udp::socket               s(io);
    boost::system::error_code ec;
    s.open(udp::v4(), ec);
    if (ec)
        return;
    s.set_option(net::socket_base::broadcast(true), ec);
    s.bind(udp::endpoint(udp::v4(), 0), ec);
    const auto redes = Filuna::redes_locais();
    // Kasa: JSON cifrado com XOR encadeado
    const std::string kasa = kasa_cifra("{\"system\":{\"get_sysinfo\":{}}}", false);
    s.send_to(net::buffer(kasa), udp::endpoint(net::ip::address_v4::broadcast(), 9999), 0, ec);
    Filuna::enviar_para_rede(s, 9999, kasa, redes);
    // Tapo: cabeçalho de 16 bytes + JSON com uma chave RSA pública (a resposta da descoberta vem aberta)
    const std::string corpo = json({{"params", {{"rsa_key", chave_publica_rsa()}}}}).dump();
    std::string       tapo(16, '\0');
    std::mt19937      rng(std::random_device{}());
    const unsigned    serie = rng();
    tapo[0] = 2; tapo[1] = 0; tapo[2] = 0; tapo[3] = 1;
    tapo[4] = char((corpo.size() >> 8) & 0xFF); tapo[5] = char(corpo.size() & 0xFF);
    tapo[6] = 17; tapo[7] = 0;
    for (int b = 0; b < 4; ++b) tapo[8 + b] = char((serie >> (24 - 8 * b)) & 0xFF);
    const unsigned inicial = 0x5A6B7C8Du;
    for (int b = 0; b < 4; ++b) tapo[12 + b] = char((inicial >> (24 - 8 * b)) & 0xFF);
    tapo += corpo;
    boost::crc_32_type crc;
    crc.process_bytes(tapo.data(), tapo.size());
    const unsigned v = crc.checksum();
    for (int b = 0; b < 4; ++b) tapo[12 + b] = char((v >> (24 - 8 * b)) & 0xFF);
    s.send_to(net::buffer(tapo), udp::endpoint(net::ip::address_v4::broadcast(), 20002), 0, ec);
    Filuna::enviar_para_rede(s, 20002, tapo, redes);

    receber(s, 3500ms, [&](const std::string& pkt, const std::string& ip, unsigned short porta) {
        if (porta == 9999) {
            const json j = json::parse(kasa_cifra(pkt, true), nullptr, false);
            if (!j.is_object() || !j.contains("system"))
                return;
            const json  si     = j["system"].value("get_sysinfo", json::object());
            const std::string modelo = si.value("model", std::string()), tipo = si.value("type", si.value("mic_type", std::string()));
            const bool  luz    = tem(tipo, "bulb") || tem(modelo, "kl");
            json a = aparelho("kasa:" + si.value("deviceId", ip), luz ? "luz" : "tomada", si.value("alias", std::string("TP-Link Kasa")), "TP-Link",
                              modelo, ip, "rede local (Kasa)", {{"Linha", "Kasa"}, {"Conta", "app Kasa"}});
            a["dados"] = {{"protocolo", "kasa"}};
            c.por(std::move(a));
        } else if (porta == 20002 && pkt.size() > 16) {
            const json j = json::parse(pkt.substr(16), nullptr, false);
            if (!j.is_object())
                return;
            const json  r      = j.value("result", json::object());
            const std::string tipo = r.value("device_type", std::string()), modelo = r.value("device_model", std::string());
            const bool  tomada = tem(tipo, "plug"), luz = tem(tipo, "bulb") || tem(tipo, "light");
            if (!tomada && !luz)
                return;
            json a = aparelho("tapo:" + r.value("mac", r.value("device_id", ip)), luz ? "luz" : "tomada", "Tapo " + modelo, "TP-Link", modelo,
                              r.value("ip", ip), "rede local (Tapo)", {{"Linha", "Tapo"}, {"Conta", "app Tapo"}, {"Mede consumo", tem(modelo, "p110") || tem(modelo, "p115") ? "sim" : "confira"}});
            a["dados"] = {{"protocolo", "tapo"}};
            c.por(std::move(a));
        }
    });
}

// ---------------------------------------------------------------- Smart Life (Tuya, 6666 e 6667)

std::string tuya_decifra(const std::string& cifrado)
{
    static const unsigned char CHAVE[16] = {0x6c, 0x1e, 0xc8, 0xe2, 0xbb, 0x9b, 0xb5, 0x9a, 0xb5, 0x0b, 0x0d, 0xaf, 0x64, 0x9b, 0x41, 0x0a}; // md5("yGAdlopoPVldABfn")
    if (cifrado.empty() || cifrado.size() % 16)
        return {};
    std::string     out(cifrado.size() + 16, '\0');
    int             n1 = 0, n2 = 0;
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    bool            ok  = ctx != nullptr && EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr, CHAVE, nullptr) == 1 &&
              EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(&out[0]), &n1, reinterpret_cast<const unsigned char*>(cifrado.data()), int(cifrado.size())) == 1 &&
              EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(&out[0]) + n1, &n2) == 1;
    EVP_CIPHER_CTX_free(ctx);
    if (!ok)
        return {};
    out.resize(std::size_t(n1 + n2));
    return out;
}

void buscar_tuya(Coleta& c)
{
    net::io_context io;
    std::vector<std::unique_ptr<udp::socket>> socks;
    for (unsigned short porta : {6666, 6667}) {
        auto                      s = std::make_unique<udp::socket>(io);
        boost::system::error_code ec;
        s->open(udp::v4(), ec);
        s->set_option(net::socket_base::reuse_address(true), ec);
        s->bind(udp::endpoint(udp::v4(), porta), ec);
        if (!ec)
            socks.push_back(std::move(s));
    }
    if (socks.empty())
        return;
    // os aparelhos anunciam a cada ~5 s; escuta as duas portas alternando
    const auto fim = std::chrono::steady_clock::now() + 6000ms;
    while (std::chrono::steady_clock::now() < fim)
        for (auto& s : socks)
            receber(*s, 150ms, [&](const std::string& pkt, const std::string& ip, unsigned short) {
                json              j;
                const std::size_t ab = pkt.find('{'), fe = pkt.rfind('}');
                if (ab != std::string::npos && fe != std::string::npos && fe > ab) // protocolo 3.1: JSON aberto
                    j = json::parse(pkt.substr(ab, fe - ab + 1), nullptr, false);
                if (!j.is_object() && pkt.size() > 28)
                    for (std::size_t ini : {20u, 16u}) {
                        const std::string claro = tuya_decifra(pkt.substr(ini, pkt.size() - ini - 8));
                        const std::size_t ch    = claro.find('{');
                        if (ch != std::string::npos) {
                            j = json::parse(claro.substr(ch, claro.rfind('}') - ch + 1), nullptr, false);
                            if (j.is_object())
                                break;
                        }
                    }
                const std::string gw = j.is_object() ? j.value("gwId", std::string()) : std::string();
                json a = aparelho("tuya:" + (gw.empty() ? ip : gw), "smart_life", "Aparelho Smart Life", "Smart Life (Tuya)",
                                  j.is_object() ? "versão " + j.value("version", std::string("?")) : std::string("protocolo novo"), ip, "anúncio na rede (Smart Life)",
                                  {{"Conta", "app Smart Life (a Positivo Casa Inteligente usa a mesma)"}, {"O que é", "tomada ou luz: escolha na configuração"}});
                a["dados"] = {{"protocolo", "tuya"}, {"produto", j.is_object() ? j.value("productKey", std::string()) : std::string()}};
                c.por(std::move(a));
            });
}

// ---------------------------------------------------------------- roteador e Wi-Fi

std::string gateway()
{
#ifdef _WIN32
    MIB_IPFORWARDROW linha{};
    if (GetBestRoute(0x08080808u, 0, &linha) == NO_ERROR && linha.dwForwardNextHop != 0) {
        const auto* b = reinterpret_cast<const unsigned char*>(&linha.dwForwardNextHop);
        return std::to_string(b[0]) + "." + std::to_string(b[1]) + "." + std::to_string(b[2]) + "." + std::to_string(b[3]);
    }
#endif
    const auto redes = Filuna::redes_locais();
    return redes.empty() ? std::string() : redes.front() + "1";
}

int canal_da_frequencia(unsigned long khz)
{
    const long mhz = long(khz / 1000);
    if (mhz >= 2412 && mhz <= 2484)
        return mhz == 2484 ? 14 : int((mhz - 2407) / 5);
    if (mhz >= 5000 && mhz < 5900)
        return int((mhz - 5000) / 5);
    return 0;
}

// {wifi: bool, ssid, canal, redes: [{ssid, ghz, canal, sinal}]}
json ler_wifi()
{
    json r = {{"wifi", false}, {"redes", json::array()}};
#ifdef _WIN32
    HANDLE h   = nullptr;
    DWORD  ver = 0;
    if (WlanOpenHandle(2, nullptr, &ver, &h) != ERROR_SUCCESS)
        return r;
    PWLAN_INTERFACE_INFO_LIST lista = nullptr;
    if (WlanEnumInterfaces(h, nullptr, &lista) == ERROR_SUCCESS && lista != nullptr) {
        for (DWORD i = 0; i < lista->dwNumberOfItems; ++i) {
            const WLAN_INTERFACE_INFO& info = lista->InterfaceInfo[i];
            if (info.isState == wlan_interface_state_connected) {
                DWORD                        tam = 0;
                PWLAN_CONNECTION_ATTRIBUTES  con = nullptr;
                if (WlanQueryInterface(h, &info.InterfaceGuid, wlan_intf_opcode_current_connection, nullptr, &tam, reinterpret_cast<PVOID*>(&con), nullptr) == ERROR_SUCCESS && con != nullptr) {
                    const DOT11_SSID& s = con->wlanAssociationAttributes.dot11Ssid;
                    r["wifi"]           = true;
                    r["ssid"]           = std::string(reinterpret_cast<const char*>(s.ucSSID), s.uSSIDLength);
                    WlanFreeMemory(con);
                }
                ULONG* canal = nullptr;
                if (WlanQueryInterface(h, &info.InterfaceGuid, wlan_intf_opcode_channel_number, nullptr, &tam, reinterpret_cast<PVOID*>(&canal), nullptr) == ERROR_SUCCESS && canal != nullptr) {
                    r["canal"] = static_cast<int>(*canal);
                    WlanFreeMemory(canal);
                }
            }
            PWLAN_BSS_LIST bss = nullptr;
            if (WlanGetNetworkBssList(h, &info.InterfaceGuid, nullptr, dot11_BSS_type_any, FALSE, nullptr, &bss) == ERROR_SUCCESS && bss != nullptr) {
                for (DWORD k = 0; k < bss->dwNumberOfItems; ++k) {
                    const WLAN_BSS_ENTRY& e = bss->wlanBssEntries[k];
                    const int             c = canal_da_frequencia(e.ulChCenterFrequency);
                    r["redes"].push_back({{"ssid", std::string(reinterpret_cast<const char*>(e.dot11Ssid.ucSSID), e.dot11Ssid.uSSIDLength)},
                                          {"ghz", c > 14 ? 5 : 2.4}, {"canal", c}, {"sinal", int(e.lRssi)}});
                }
                WlanFreeMemory(bss);
            }
        }
        WlanFreeMemory(lista);
    }
    WlanCloseHandle(h, nullptr);
#endif
    return r;
}

// Checagens do roteador no formato da página /casa: [situação ("ok", "aviso", "ruim"), título, dica].
json checar_rede(const json& wifi, int respostas_multicast, int impressoras)
{
    json ch = json::array();
    if (!wifi.value("wifi", false)) {
        ch.push_back({"aviso", "Computador ligado no cabo", "O Filuna lê o Wi-Fi pelo computador. Ligue o Wi-Fi do computador uma vez para conferir as redes."});
    } else {
        const std::string ssid = wifi.value("ssid", std::string());
        bool              tem24 = false, tem5 = false, outra24 = false, vizinhas = false;
        int               canal24 = 0, sinal24 = -200;
        for (const auto& r : wifi["redes"]) {
            const bool        e24  = r.value("ghz", 0.0) < 3;
            const std::string nome = r.value("ssid", std::string());
            if (nome != ssid)
                vizinhas = true;
            if (nome == ssid) {
                (e24 ? tem24 : tem5) = true;
                if (e24 && r.value("sinal", -200) > sinal24) {
                    sinal24 = r.value("sinal", -200);
                    canal24 = r.value("canal", 0);
                }
            } else if (e24 && !ssid.empty() && !nome.empty() && minusc(nome).find(minusc(ssid.substr(0, std::min<std::size_t>(4, ssid.size())))) == 0)
                outra24 = true; // "Casa_2G" ao lado de "Casa"
        }
        if (tem24 || outra24)
            ch.push_back({"ok", "Rede de 2,4 GHz disponível", "As impressoras só usam 2,4 GHz."});
        else if (!vizinhas)
            // O Windows 11 só entrega a lista de redes com a Localização ligada; sem ela vem só a rede conectada.
            ch.push_back({"aviso", "Rede de 2,4 GHz disponível",
                          "O Windows não mostrou as outras redes Wi-Fi, então não deu para conferir a de 2,4 GHz. Ligue Configurações › Privacidade e segurança › "
                          "Localização (e \"Permitir que aplicativos da área de trabalho acessem sua localização\") e toque em Verificar de novo, ou confira no roteador."});
        else
            ch.push_back({"ruim", "Rede de 2,4 GHz disponível", "Não achamos a rede de 2,4 GHz. As impressoras só usam 2,4 GHz: ligue essa rede no roteador."});
        // sem a lista de redes não dá para saber se o nome se repete nas duas bandas
        if (tem24 && tem5)
            ch.push_back({"ruim", "Nome separado para a rede de 2,4 GHz",
                          "A rede \"" + ssid + "\" usa o mesmo nome em 2,4 e 5 GHz. Crie uma rede só 2,4 GHz (ex.: " + ssid + "_2G) e ligue as impressoras nela."});
        else if (vizinhas || tem24)
            ch.push_back({"ok", "Nome separado para a rede de 2,4 GHz", "Cada banda tem o seu nome."});
        if (tem24 && tem5)
            ch.push_back({"aviso", "Direcionamento de banda (band steering)", "Com o mesmo nome nas duas bandas, desligue o direcionamento de banda no roteador ou deixe as impressoras na rede só 2,4 GHz."});
        if (canal24 > 0)
            ch.push_back({canal24 == 1 || canal24 == 6 || canal24 == 11 ? "ok" : "aviso", "Canal 2,4 GHz sem sobreposição",
                          "Canal " + std::to_string(canal24) + (canal24 == 1 || canal24 == 6 || canal24 == 11 ? " em uso: bom." : " em uso: prefira 1, 6 ou 11.")});
    }
    if (respostas_multicast > 0)
        ch.push_back({"ok", "Descoberta na rede (multicast)", "O Filuna acha as impressoras e a TV sozinho."});
    else
        ch.push_back({"ruim", "Descoberta na rede (multicast)", "Nenhum aparelho respondeu à busca. Confira se o computador não está numa rede de visitantes e se o roteador não bloqueia multicast (IGMP)."});
    ch.push_back({"aviso", "Endereço fixo para as impressoras",
                  impressoras > 0 ? "Reserve o IP de cada impressora no roteador (reserva de DHCP): assim ela não some depois que o roteador reinicia."
                                  : "Quando ligar uma impressora, reserve o IP dela no roteador (reserva de DHCP)."});
    return ch;
}

// ---------------------------------------------------------------- busca completa

std::mutex& trava() { static std::mutex m; return m; }
json&       ultima_busca()
{
    static json j = {{"rodando", false}, {"aparelhos", json::array()}, {"rede", json::object()}};
    return j;
}

void buscar_tudo()
{
    Coleta c;
    std::vector<std::thread> t;
    t.emplace_back([&c] { buscar_ssdp(c); });
    t.emplace_back([&c] { buscar_mdns(c); });
    t.emplace_back([&c] { buscar_tplink(c); });
    t.emplace_back([&c] { buscar_tuya(c); });
    json wifi;
    t.emplace_back([&wifi] { wifi = ler_wifi(); });
    for (auto& x : t)
        x.join();

    const std::string gw = gateway();
    json lista = json::array();
    bool tem_roteador = false;
    for (auto& [ip, a] : c.por_ip) {
        if (a.value("tipo", std::string()) == "roteador" || ip == gw) {
            if (a.value("tipo", std::string()) != "roteador")
                continue; // o gateway é sempre tratado como roteador abaixo
            tem_roteador = true;
        }
        lista.push_back(a);
    }
    if (!tem_roteador && !gw.empty())
        lista.push_back(aparelho("roteador:" + gw, "roteador", "Roteador Wi-Fi", "Roteador", "", gw, "gateway da rede"));
    const json checagens = checar_rede(wifi, c.respostas_multicast, 0);
    bool       ruim      = false;
    for (const auto& x : checagens)
        ruim = ruim || x[0] == "ruim";
    for (auto& a : lista)
        if (a["tipo"] == "roteador") {
            a["checagens"] = checagens;
            a["estado"]    = ruim ? "atencao" : "pronto";
            if (ruim)
                a["alerta"] = "Tem ponto para ajustar na rede: veja o diagnóstico. Rede mal configurada derruba as impressoras do Wi-Fi.";
            json fatos = json::array();
            if (wifi.value("wifi", false))
                fatos.push_back({"Rede do computador", wifi.value("ssid", std::string())});
            fatos.push_back({"Endereço", a.value("ip", std::string())});
            a["fatos"] = fatos;
        }
    BOOST_LOG_TRIVIAL(info) << "Filuna Home: busca achou " << lista.size() << " aparelhos (" << c.respostas_multicast << " respostas multicast)";
    std::lock_guard<std::mutex> g(trava());
    ultima_busca() = {{"rodando", false}, {"aparelhos", lista}, {"rede", {{"gateway", gw}, {"wifi", wifi.value("wifi", false)}, {"ssid", wifi.value("ssid", std::string())}}},
                      {"quando", std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()}};
}

// ---------------------------------------------------------------- Shelly

bool shelly_ligar(const std::string& ip, bool ligar, std::string& erro)
{
    std::string corpo;
    if (http_get("http://" + ip + "/rpc/Switch.Set?id=0&on=" + std::string(ligar ? "true" : "false"), corpo))
        return true;
    if (http_get("http://" + ip + "/relay/0?turn=" + std::string(ligar ? "on" : "off"), corpo))
        return true;
    erro = "A tomada não respondeu em " + ip + ".";
    return false;
}

json shelly_estado(const std::string& ip)
{
    std::string corpo;
    if (http_get("http://" + ip + "/rpc/Switch.GetStatus?id=0", corpo)) {
        const json j = json::parse(corpo, nullptr, false);
        if (j.is_object())
            return {{"ok", true}, {"ligada", j.value("output", false)}, {"potencia_w", j.value("apower", 0.0)}};
    }
    if (http_get("http://" + ip + "/status", corpo)) {
        const json j = json::parse(corpo, nullptr, false);
        if (j.is_object()) {
            json r = {{"ok", true}, {"ligada", j.value("relays", json::array({json::object()}))[0].value("ison", false)}};
            if (j.contains("meters") && j["meters"].is_array() && !j["meters"].empty())
                r["potencia_w"] = j["meters"][0].value("power", 0.0);
            return r;
        }
    }
    return {{"ok", false}, {"msg", "A tomada não respondeu em " + ip + "."}};
}

// ---------------------------------------------------------------- TV LG (SSAP do webOS)

// Manifesto de registro do webOS (assinado pela LG, público no kit de apps dela). Copiado da biblioteca aiowebostv,
// licença Apache 2.0 (https://github.com/home-assistant-libs/aiowebostv).
const char* LG_REGISTRO = R"LG({"forcePairing":false,"manifest":{"appVersion":"1.1","manifestVersion":1,"permissions":["LAUNCH","LAUNCH_WEBAPP","APP_TO_APP","CLOSE","TEST_OPEN","TEST_PROTECTED","CONTROL_AUDIO","CONTROL_DISPLAY","CONTROL_INPUT_JOYSTICK","CONTROL_INPUT_MEDIA_RECORDING","CONTROL_INPUT_MEDIA_PLAYBACK","CONTROL_INPUT_TV","CONTROL_POWER","CONTROL_TV_SCREEN","READ_APP_STATUS","READ_CURRENT_CHANNEL","READ_INPUT_DEVICE_LIST","READ_NETWORK_STATE","READ_RUNNING_APPS","READ_TV_CHANNEL_LIST","WRITE_NOTIFICATION_TOAST","READ_POWER_STATE","READ_COUNTRY_INFO","CONTROL_INPUT_TEXT","CONTROL_MOUSE_AND_KEYBOARD","READ_INSTALLED_APPS","READ_SETTINGS"],"signatures":[{"signature":"eyJhbGdvcml0aG0iOiJSU0EtU0hBMjU2Iiwia2V5SWQiOiJ0ZXN0LXNpZ25pbmctY2VydCIsInNpZ25hdHVyZVZlcnNpb24iOjF9.hrVRgjCwXVvE2OOSpDZ58hR+59aFNwYDyjQgKk3auukd7pcegmE2CzPCa0bJ0ZsRAcKkCTJrWo5iDzNhMBWRyaMOv5zWSrthlf7G128qvIlpMT0YNY+n/FaOHE73uLrS/g7swl3/qH/BGFG2Hu4RlL48eb3lLKqTt2xKHdCs6Cd4RMfJPYnzgvI4BNrFUKsjkcu+WD4OO2A27Pq1n50cMchmcaXadJhGrOqH5YmHdOCj5NSHzJYrsW0HPlpuAx/ECMeIZYDh6RMqaFM2DXzdKX9NmmyqzJ3o/0lkk/N97gfVRLW5hA29yeAwaCViZNCP8iC9aO0q9fQojoa7NQnAtw==","signatureVersion":1}],"signed":{"appId":"com.lge.test","created":"20140509","localizedAppNames":{"":"LG Remote App","ko-KR":"\ub9ac\ubaa8\ucee8 \uc571","zxx-XX":"\u041b\u0413 R\u044d\u043cot\u044d A\u041f\u041f"},"localizedVendorNames":{"":"LG Electronics"},"permissions":["TEST_SECURE","CONTROL_INPUT_TEXT","CONTROL_MOUSE_AND_KEYBOARD","READ_INSTALLED_APPS","READ_LGE_SDX","READ_NOTIFICATIONS","SEARCH","WRITE_SETTINGS","WRITE_NOTIFICATION_ALERT","CONTROL_POWER","READ_CURRENT_CHANNEL","READ_RUNNING_APPS","READ_UPDATE_INFO","UPDATE_FROM_REMOTE_APP","READ_LGE_TV_INPUT_EVENTS","READ_TV_CURRENT_TIME"],"serial":"2f930e2d2cfe083771f68e4fe7bb07","vendorId":"com.lge"}},"pairingType":"PROMPT"})LG";

struct Lg
{
    bool        fim = false, ok = false, sem_porta = false;
    std::string chave, erro;
};

template <class Stream>
void lg_conversa(ws::stream<Stream>& w, const std::string& host, const std::string& registro, const std::string& aviso, Lg& r,
                 std::shared_ptr<std::function<void()>> ler, std::shared_ptr<beast::flat_buffer> buf)
{
    w.set_option(ws::stream_base::timeout::suggested(beast::role_type::client));
    w.async_handshake(host, "/", [&w, registro, aviso, &r, ler, buf](beast::error_code ec) {
        if (ec) {
            r.fim  = true;
            r.erro = "A TV não aceitou a conexão (" + ec.message() + ").";
            return;
        }
        *ler = [&w, aviso, &r, ler, buf]() {
            w.async_read(*buf, [&w, aviso, &r, ler, buf](beast::error_code ec, std::size_t) {
                if (ec) {
                    if (!r.fim) {
                        r.fim  = true;
                        r.erro = r.chave.empty() ? "A TV fechou a conexão. Aceite o pedido que aparece na tela da TV." : "A TV fechou a conexão.";
                    }
                    return;
                }
                const json m = json::parse(beast::buffers_to_string(buf->data()), nullptr, false);
                buf->consume(buf->size());
                const std::string tipo = m.is_object() ? m.value("type", std::string()) : std::string();
                if (tipo == "registered") {
                    r.chave = m.value("payload", json::object()).value("client-key", std::string());
                    if (aviso.empty()) {
                        r.ok = r.fim = true;
                        return;
                    }
                    auto req = std::make_shared<std::string>(json({{"type", "request"}, {"id", "aviso"}, {"uri", "ssap://system.notifications/createToast"},
                                                                   {"payload", {{"message", aviso}}}}).dump());
                    w.async_write(net::buffer(*req), [req, ler, &r](beast::error_code ec, std::size_t) {
                        if (ec) {
                            r.fim  = true;
                            r.erro = "Não foi possível mandar o aviso para a TV.";
                            return;
                        }
                        (*ler)();
                    });
                    return;
                }
                if (tipo == "response" && m.value("id", std::string()) == "aviso") {
                    r.ok  = m.value("payload", json::object()).value("returnValue", false);
                    r.fim = true;
                    if (!r.ok)
                        r.erro = "A TV recusou o aviso.";
                    return;
                }
                if (tipo == "error") {
                    r.fim  = true;
                    r.erro = "A TV recusou: " + m.value("error", std::string("pedido negado")) + ".";
                    return;
                }
                (*ler)(); // "response" do registro (pedido na tela) e outras mensagens: continua ouvindo
            });
        };
        w.text(true);
        auto s = std::make_shared<std::string>(registro);
        w.async_write(net::buffer(*s), [s, ler, &r](beast::error_code ec, std::size_t) {
            if (ec) {
                r.fim  = true;
                r.erro = "Não foi possível falar com a TV.";
                return;
            }
            (*ler)();
        });
    });
}

Lg lg(const std::string& ip, const std::string& chave_antiga, const std::string& aviso, int espera_s)
{
    json reg = json::parse(LG_REGISTRO, nullptr, false);
    if (!reg.is_object())
        return Lg{true, false, false, {}, "Manifesto da LG inválido."};
    if (!chave_antiga.empty())
        reg["client-key"] = chave_antiga;
    const std::string msg = json({{"type", "register"}, {"id", "registro"}, {"payload", reg}}).dump();
    boost::system::error_code ec;
    const auto                addr = net::ip::make_address(ip, ec);
    if (ec)
        return Lg{true, false, false, {}, "Endereço da TV inválido."};
    Lg r;
    for (const bool tls : {true, false}) { // webOS novo só aceita wss na 3001; o antigo, ws na 3000
        r = Lg{};
        net::io_context ioc;
        auto            ler = std::make_shared<std::function<void()>>();
        auto            buf = std::make_shared<beast::flat_buffer>();
        const tcp::endpoint ep(addr, tls ? 3001 : 3000);
        if (tls) {
            net::ssl::context ctx(net::ssl::context::tls_client);
            ctx.set_verify_mode(net::ssl::verify_none); // a TV usa certificado próprio
            ws::stream<beast::ssl_stream<beast::tcp_stream>> w(ioc, ctx);
            beast::get_lowest_layer(w).expires_after(5s);
            beast::get_lowest_layer(w).async_connect(ep, [&](beast::error_code e) {
                if (e) {
                    r.fim = r.sem_porta = true;
                    r.erro              = "A TV não respondeu em " + ip + ". Ligue a TV e confira se ela está na mesma rede.";
                    return;
                }
                beast::get_lowest_layer(w).expires_never();
                w.next_layer().async_handshake(net::ssl::stream_base::client, [&](beast::error_code e2) {
                    if (e2) {
                        r.fim = r.sem_porta = true;
                        r.erro              = "A TV não aceitou a conexão segura.";
                        return;
                    }
                    lg_conversa(w, ip + ":3001", msg, aviso, r, ler, buf);
                });
            });
            ioc.run_for(std::chrono::seconds(espera_s));
        } else {
            ws::stream<beast::tcp_stream> w(ioc);
            beast::get_lowest_layer(w).expires_after(5s);
            beast::get_lowest_layer(w).async_connect(ep, [&](beast::error_code e) {
                if (e) {
                    r.fim = r.sem_porta = true;
                    r.erro              = "A TV não respondeu em " + ip + ". Ligue a TV e confira se ela está na mesma rede.";
                    return;
                }
                beast::get_lowest_layer(w).expires_never();
                lg_conversa(w, ip + ":3000", msg, aviso, r, ler, buf);
            });
            ioc.run_for(std::chrono::seconds(espera_s));
        }
        *ler = nullptr; // desfaz o ciclo da leitura
        if (!r.fim)
            r.erro = "A TV não respondeu a tempo. Aceite o pedido que aparece na tela da TV e tente de novo.";
        if (r.ok || !r.sem_porta)
            break;
    }
    return r;
}

std::string chave_config_lg(const std::string& ip) { return "filuna_casa_lg_" + id_seguro(ip); }

// ---------------------------------------------------------------- Home Assistant

struct Ha
{
    std::mutex                          m;
    std::unique_ptr<Filuna::MqttClient> cli;
    std::string                         assinatura; // broker+usuário: muda → reconecta
    std::set<std::string>               anunciadas;
    std::chrono::steady_clock::time_point tentativa;
};
Ha& ha()
{
    static Ha h;
    return h;
}

bool ha_conectar(Filuna::MqttClient& cli, const json& cfg, std::string& erro)
{
    std::string   broker = cfg.value("broker", std::string());
    std::uint16_t porta  = 1883;
    const auto    dp     = broker.rfind(':');
    if (dp != std::string::npos) {
        porta  = static_cast<std::uint16_t>(std::atoi(broker.substr(dp + 1).c_str()));
        broker = broker.substr(0, dp);
    }
    if (broker.empty()) {
        erro = "Informe o endereço do MQTT.";
        return false;
    }
    Filuna::MqttClient::Options o;
    o.host            = broker;
    o.port            = porta ? porta : 1883;
    o.tls             = false;
    o.client_id       = "filuna-slicer-" + std::to_string(std::random_device{}() % 100000);
    o.username        = cfg.value("usuario", std::string());
    o.password        = cfg.value("senha", std::string());
    o.connect_timeout = 6s;
    return cli.connect(o, {}, [](const std::string&, const std::string&) {}, [](const std::string&) {}, erro);
}

void ha_publicar(Filuna::MqttClient& cli, std::set<std::string>& anunciadas, const json& estado, const json& cfg)
{
    for (const auto& c : estado.value("impressoras", json::array())) {
        const std::string bruto = c.value("cadastro_id", std::string()).empty() ? c.value("dev_id", std::string()) : c.value("cadastro_id", std::string());
        if (bruto.empty())
            continue;
        const std::string id = id_seguro(bruto), topo = "filuna/" + id + "/estado", nome = c.value("nome", std::string("Impressora"));
        if (!anunciadas.count(id)) {
            const json dev = {{"identifiers", {"filuna_" + id}}, {"name", nome}, {"manufacturer", "Filuna"}, {"model", c.value("modelo_nome", std::string())}};
            struct S { const char* k; const char* nome; const char* un; const char* classe; };
            for (const S& s : {S{"estado", "Estado", "", ""}, S{"progresso", "Progresso", "%", ""}, S{"restante_min", "Tempo restante", "min", "duration"},
                               S{"bico", "Bico", "°C", "temperature"}, S{"mesa", "Mesa", "°C", "temperature"}}) {
                json conf = {{"name", s.nome}, {"unique_id", "filuna_" + id + "_" + s.k}, {"state_topic", topo},
                             {"value_template", std::string("{{ value_json.") + s.k + " }}"}, {"device", dev}};
                if (*s.un)
                    conf["unit_of_measurement"] = s.un;
                if (*s.classe)
                    conf["device_class"] = s.classe;
                cli.publish("homeassistant/sensor/filuna_" + id + "/" + s.k + "/config", conf.dump(), true);
            }
            anunciadas.insert(id);
        }
        json v = {{"estado", c.value("estado", std::string())}, {"progresso", c.value("progresso", 0)}, {"restante_min", c.value("restante_s", 0) / 60}};
        if (c.contains("bico") && c["bico"].is_object())
            v["bico"] = c["bico"].value("atual", 0.0);
        if (c.contains("mesa") && c["mesa"].is_object())
            v["mesa"] = c["mesa"].value("atual", 0.0);
        if (!c.value("erro", std::string()).empty())
            v["erro"] = c.value("erro", std::string());
        cli.publish(topo, v.dump(), true);
    }
}

// ---------------------------------------------------------------- ações

void responder(const json& pedido, json r)
{
    r["pedido"] = pedido.value("pedido", std::string());
    r["acao"]   = pedido.value("acao", std::string());
    wxGetApp().CallAfter([r] { FilunaPanel::broadcast("casa.acao", r); });
}

void executar(json p, std::string chave_lg, json cfg_ha)
{
    const std::string a = p.value("acao", std::string()), ip = p.value("ip", std::string());
    try {
        if (a == "tomada.estado" || a == "tomada.testar") {
            const std::string prot = p.value("protocolo", std::string("shelly"));
            if (a == "tomada.estado") {
                responder(p, tomada_estado(prot, ip));
                return;
            }
            std::string erro;
            if (!tomada_ligar(prot, ip, false, erro)) {
                responder(p, {{"ok", false}, {"msg", erro}});
                return;
            }
            std::this_thread::sleep_for(3s);
            const bool ok = tomada_ligar(prot, ip, true, erro);
            responder(p, {{"ok", ok}, {"msg", ok ? "A tomada desligou e ligou de novo." : erro}});
            return;
        }
        if (a == "tapo.conta") { // testa a conta Tapo neste aparelho; a gravação fica em acao(), na linha da tela
            json        r;
            std::string erro;
            const bool  ok = tapo_pedido(ip, {{"method", "get_device_info"}}, r, erro);
            responder(p, {{"ok", ok}, {"msg", ok ? "Conta Tapo aceita. " + std::string(r.value("device_on", false) ? "O aparelho está ligado." : "O aparelho está desligado.") : erro}});
            return;
        }
        if (a == "hue.parear") {
            std::string erro;
            const std::string u = hue_parear(ip, erro);
            if (!u.empty())
                wxGetApp().CallAfter([ip, u] {
                    if (AppConfig* cfg = wxGetApp().app_config) {
                        cfg->set("filuna_casa_hue_" + id_seguro(ip), u);
                        cfg->save();
                    }
                });
            responder(p, {{"ok", !u.empty()}, {"msg", u.empty() ? erro : "Ponte Hue pareada. O Filuna já pode piscar as luzes."}});
            return;
        }
        if (a == "filunatv.parear") {
            std::string erro;
            const std::string t = tv_filuna_parear(ip, p.value("codigo", std::string()), erro);
            if (!t.empty())
                wxGetApp().CallAfter([ip, t] {
                    if (AppConfig* cfg = wxGetApp().app_config) {
                        cfg->set("filuna_casa_tv_" + id_seguro(ip), t);
                        cfg->save();
                    }
                });
            responder(p, {{"ok", !t.empty()}, {"msg", t.empty() ? (erro.empty() ? "Código errado. Confira os 6 dígitos na TV." : erro) : "TV pareada. Os avisos já podem aparecer nela."}});
            return;
        }
        if (a == "shelly.estado")
            responder(p, shelly_estado(ip));
        else if (a == "shelly.ligar") {
            std::string erro;
            const bool  ok = shelly_ligar(ip, p.value("ligado", true), erro);
            responder(p, {{"ok", ok}, {"msg", ok ? (p.value("ligado", true) ? "Tomada ligada." : "Tomada desligada.") : erro}});
        } else if (a == "shelly.testar") {
            std::string erro;
            const auto  t0 = std::chrono::steady_clock::now();
            if (!shelly_ligar(ip, false, erro)) {
                responder(p, {{"ok", false}, {"msg", erro}});
                return;
            }
            std::this_thread::sleep_for(3s);
            const bool ok = shelly_ligar(ip, true, erro);
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count() - 3000;
            const json st = shelly_estado(ip);
            std::string msg = ok ? "A tomada desligou e ligou de novo (respondeu em " + std::to_string(std::max<long long>(ms, 0)) + " ms)." : erro;
            if (ok && st.contains("potencia_w"))
                msg += " Consumo agora: " + std::to_string(int(st.value("potencia_w", 0.0) + 0.5)) + " W.";
            responder(p, {{"ok", ok}, {"msg", msg}});
        } else if (a == "lg.parear" || a == "lg.aviso") {
            const std::string texto = a == "lg.aviso" ? p.value("texto", std::string("Filuna: aviso de teste. Está tudo certo.")) : std::string();
            const Lg          r     = lg(ip, chave_lg, texto, a == "lg.parear" ? 70 : 20);
            if (!r.chave.empty() && r.chave != chave_lg)
                wxGetApp().CallAfter([ip, chave = r.chave] {
                    if (AppConfig* cfg = wxGetApp().app_config) {
                        cfg->set(chave_config_lg(ip), chave);
                        cfg->save();
                    }
                });
            responder(p, {{"ok", r.ok}, {"msg", r.ok ? (a == "lg.parear" ? "TV pareada. O Filuna já pode mandar avisos para ela." : "Aviso mostrado na TV.") : r.erro},
                          {"pareada", !r.chave.empty() || !chave_lg.empty()}});
        } else if (a == "mqtt.testar") {
            Filuna::MqttClient cli;
            std::string        erro;
            const bool         ok = ha_conectar(cli, p, erro);
            cli.close();
            responder(p, {{"ok", ok}, {"msg", ok ? "Conectado ao MQTT do Home Assistant." : "Não conectou: " + erro}});
        } else if (a == "rede.diagnostico") {
            buscar_tudo();
            json r;
            {
                std::lock_guard<std::mutex> g(trava());
                r = ultima_busca();
            }
            json ch = json::array();
            for (const auto& x : r["aparelhos"])
                if (x["tipo"] == "roteador")
                    ch = x.value("checagens", json::array());
            responder(p, {{"ok", true}, {"checagens", ch}, {"msg", "Diagnóstico atualizado."}});
        } else
            responder(p, {{"ok", false}, {"msg", "Ação desconhecida."}});
    } catch (const std::exception& e) {
        responder(p, {{"ok", false}, {"msg", std::string("Falhou: ") + e.what()}});
    }
}

// ---------------------------------------------------------------- comandos para o motor de avisos

bool http_post_json(const std::string& url, const json& corpo, std::string& resposta, long max_s = 5)
{
    bool ok = false;
    Http::post(url)
        .header("Content-Type", "application/json")
        .set_post_body(corpo.dump())
        .timeout_connect(2)
        .timeout_max(max_s)
        .on_complete([&](std::string b, unsigned st) { resposta = std::move(b); ok = st < 400; })
        .on_error([&](std::string b, std::string erro, unsigned) { resposta = b.empty() ? erro : b; })
        .perform_sync();
    return ok;
}

// Kasa na porta 9999 (TCP): 4 bytes de tamanho + JSON cifrado com o XOR encadeado.
bool kasa_pedido(const std::string& ip, const json& pedido, json& resposta, std::string& erro)
{
    boost::system::error_code ec;
    const auto                addr = net::ip::make_address(ip, ec);
    if (ec) {
        erro = "Endereço inválido: " + ip + ".";
        return false;
    }
    const std::string corpo = kasa_cifra(pedido.dump(), false);
    std::string       pkt(4, '\0');
    const auto        n = static_cast<std::uint32_t>(corpo.size());
    for (int b = 0; b < 4; ++b)
        pkt[b] = char((n >> (24 - 8 * b)) & 0xFF);
    pkt += corpo;

    net::io_context          io;
    tcp::socket              s(io);
    std::string              entrada;
    std::array<char, 4096>   buf{};
    bool                     fim = false;
    std::function<void()>    ler;
    ler = [&]() {
        s.async_read_some(net::buffer(buf), [&](beast::error_code e, std::size_t lido) {
            entrada.append(buf.data(), lido);
            if (entrada.size() >= 4) {
                std::uint32_t esperado = 0;
                for (int b = 0; b < 4; ++b)
                    esperado = (esperado << 8) | static_cast<unsigned char>(entrada[b]);
                if (entrada.size() >= 4 + esperado) {
                    resposta = json::parse(kasa_cifra(entrada.substr(4, esperado), true), nullptr, false);
                    fim      = true;
                    return;
                }
            }
            if (e) {
                erro = "A tomada fechou a conexão.";
                fim  = true;
                return;
            }
            ler();
        });
    };
    s.async_connect(tcp::endpoint(addr, 9999), [&](beast::error_code e) {
        if (e) {
            erro = "O aparelho não respondeu em " + ip + ".";
            fim  = true;
            return;
        }
        net::async_write(s, net::buffer(pkt), [&](beast::error_code e2, std::size_t) {
            if (e2) {
                erro = "Não foi possível mandar o comando.";
                fim  = true;
                return;
            }
            ler();
        });
    });
    io.run_for(5s);
    ler = nullptr;
    if (!fim && erro.empty())
        erro = "O aparelho não respondeu a tempo em " + ip + ".";
    return resposta.is_object() && erro.empty();
}

// "#E0261B" -> matiz (0–360), saturação e brilho (0–100), como a lâmpada Kasa pede
void hex_hsv(const std::string& hex, int& h, int& s, int& v)
{
    unsigned rgb = 0xE6007E;
    try { rgb = static_cast<unsigned>(std::stoul(hex.size() > 1 && hex[0] == '#' ? hex.substr(1) : hex, nullptr, 16)); } catch (...) {}
    const double r = ((rgb >> 16) & 0xFF) / 255.0, g = ((rgb >> 8) & 0xFF) / 255.0, b = (rgb & 0xFF) / 255.0;
    const double mx = std::max({r, g, b}), mn = std::min({r, g, b}), d = mx - mn;
    double       hh = 0;
    if (d > 0)
        hh = mx == r ? 60 * std::fmod((g - b) / d, 6.0) : mx == g ? 60 * ((b - r) / d + 2) : 60 * ((r - g) / d + 4);
    if (hh < 0)
        hh += 360;
    h = int(hh + 0.5);
    s = mx > 0 ? int(d / mx * 100 + 0.5) : 0;
    v = std::max(30, int(mx * 100 + 0.5));
}

void hex_rgb(const std::string& hex, int& r, int& g, int& b)
{
    unsigned rgb = 0xE6007E;
    try { rgb = static_cast<unsigned>(std::stoul(hex.size() > 1 && hex[0] == '#' ? hex.substr(1) : hex, nullptr, 16)); } catch (...) {}
    r = (rgb >> 16) & 0xFF;
    g = (rgb >> 8) & 0xFF;
    b = rgb & 0xFF;
}


// ---------------------------------------------------------------- Tapo (KLAP, firmwares novos), Hue e Filuna TV

std::string digest(const EVP_MD* md, const std::string& d)
{
    unsigned char out[EVP_MAX_MD_SIZE];
    unsigned int  n = 0;
    EVP_Digest(d.data(), d.size(), out, &n, md, nullptr);
    return std::string(reinterpret_cast<char*>(out), n);
}
std::string sha256s(const std::string& d) { return digest(EVP_sha256(), d); }

std::string aes128cbc(const std::string& dados, const std::string& chave, const std::string& iv, bool cifrar, bool& ok)
{
    ok                   = false;
    EVP_CIPHER_CTX* c    = EVP_CIPHER_CTX_new();
    std::string     out(dados.size() + 32, '\0');
    int             n1 = 0, n2 = 0;
    if (c != nullptr && EVP_CipherInit_ex(c, EVP_aes_128_cbc(), nullptr, reinterpret_cast<const unsigned char*>(chave.data()),
                                          reinterpret_cast<const unsigned char*>(iv.data()), cifrar ? 1 : 0) > 0 &&
        EVP_CipherUpdate(c, reinterpret_cast<unsigned char*>(&out[0]), &n1, reinterpret_cast<const unsigned char*>(dados.data()), int(dados.size())) > 0 &&
        EVP_CipherFinal_ex(c, reinterpret_cast<unsigned char*>(&out[n1]), &n2) > 0)
        ok = true;
    EVP_CIPHER_CTX_free(c);
    out.resize(ok ? std::size_t(n1 + n2) : 0);
    return out;
}

std::string be32(std::int32_t v)
{
    const auto u = static_cast<std::uint32_t>(v);
    return std::string{char((u >> 24) & 0xFF), char((u >> 16) & 0xFF), char((u >> 8) & 0xFF), char(u & 0xFF)};
}

// POST binário na porta 80 com cookie (o KLAP precisa ler o Set-Cookie, que o Http do Slic3r não devolve)
bool post_bin(const std::string& ip, const std::string& alvo, const std::string& corpo, std::string& cookie, std::string& resposta, int& status)
{
    namespace bh = beast::http;
    try {
        net::io_context         io;
        beast::tcp_stream       s(io);
        s.expires_after(6s);
        s.connect(tcp::endpoint(net::ip::make_address(ip), 80));
        bh::request<bh::string_body> req{bh::verb::post, alvo, 11};
        req.set(bh::field::host, ip);
        req.set(bh::field::content_type, "application/octet-stream");
        if (!cookie.empty())
            req.set(bh::field::cookie, cookie);
        req.body() = corpo;
        req.prepare_payload();
        bh::write(s, req);
        beast::flat_buffer               b;
        bh::response<bh::string_body>    res;
        bh::read(s, b, res);
        status   = res.result_int();
        resposta = res.body();
        auto it  = res.find(bh::field::set_cookie);
        if (it != res.end()) {
            const std::string c(it->value());
            cookie = c.substr(0, c.find(';'));
        }
        beast::error_code ec;
        s.socket().shutdown(tcp::socket::shutdown_both, ec);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::mutex& trava_tapo()
{
    static std::mutex m;
    return m;
}
std::pair<std::string, std::string>& conta_tapo()
{
    static std::pair<std::string, std::string> c;
    return c;
}

// Um pedido KLAP: handshake1/2 com a conta Tapo do cliente, depois o pedido cifrado (AES-128-CBC) e assinado.
bool tapo_pedido(const std::string& ip, const json& pedido, json& resultado, std::string& erro)
{
    std::string email, senha;
    {
        std::lock_guard<std::mutex> g(trava_tapo());
        email = conta_tapo().first;
        senha = conta_tapo().second;
    }
    if (email.empty() || senha.empty()) {
        erro = "Entre com a sua conta Tapo em Dispositivo › Casa (o Filuna guarda só neste computador).";
        return false;
    }
    std::string local(16, '\0');
    RAND_bytes(reinterpret_cast<unsigned char*>(&local[0]), 16);
    std::string cookie, r1;
    int         st = 0;
    if (!post_bin(ip, "/app/handshake1", local, cookie, r1, st) || st != 200 || r1.size() < 48) {
        erro = st == 0 ? "A tomada Tapo não respondeu em " + ip + "." : "Este Tapo não aceita o protocolo KLAP (atualize o firmware pelo app Tapo).";
        return false;
    }
    const std::string remoto = r1.substr(0, 16), hash_srv = r1.substr(16, 32);
    std::string       auth;
    for (const std::string& a : {sha256s(digest(EVP_sha1(), email) + digest(EVP_sha1(), senha)),        // KLAP v2
                                 digest(EVP_md5(), digest(EVP_md5(), email) + digest(EVP_md5(), senha))}) // KLAP v1
        if (sha256s(local + remoto + a) == hash_srv) {
            auth = a;
            break;
        }
    if (auth.empty()) {
        erro = "O Tapo recusou o e-mail ou a senha da conta Tapo.";
        return false;
    }
    std::string r2;
    if (!post_bin(ip, "/app/handshake2", sha256s(remoto + local + auth), cookie, r2, st) || st != 200) {
        erro = "O Tapo recusou a sessão.";
        return false;
    }
    const std::string chave = sha256s("lsk" + local + remoto + auth).substr(0, 16);
    const std::string ivc   = sha256s("iv" + local + remoto + auth);
    const std::string assin = sha256s("ldk" + local + remoto + auth).substr(0, 28);
    std::int32_t      seq   = 0;
    for (int b = 0; b < 4; ++b)
        seq = std::int32_t((std::uint32_t(seq) << 8) | static_cast<unsigned char>(ivc[28 + b]));
    ++seq;
    const std::string iv = ivc.substr(0, 12) + be32(seq);
    bool              ok = false;
    const std::string cif = aes128cbc(pedido.dump(), chave, iv, true, ok);
    std::string       r3;
    if (!ok || !post_bin(ip, "/app/request?seq=" + std::to_string(seq), sha256s(assin + be32(seq) + cif) + cif, cookie, r3, st) || st != 200 || r3.size() <= 32) {
        erro = "O Tapo não aceitou o pedido.";
        return false;
    }
    const json j = json::parse(aes128cbc(r3.substr(32), chave, iv, false, ok), nullptr, false);
    if (!ok || !j.is_object() || j.value("error_code", -1) != 0) {
        erro = "O Tapo respondeu com erro " + std::to_string(j.is_object() ? j.value("error_code", -1) : -1) + ".";
        return false;
    }
    resultado = j.value("result", json::object());
    return true;
}

bool http_put_json(const std::string& url, const json& corpo, std::string& resposta)
{
    bool ok = false;
    Http::put(url)
        .header("Content-Type", "application/json")
        .set_post_body(corpo.dump())
        .timeout_connect(2)
        .timeout_max(5)
        .on_complete([&](std::string b, unsigned st) { resposta = std::move(b); ok = st < 400; })
        .on_error([&](std::string b, std::string erro, unsigned) { resposta = b.empty() ? erro : b; })
        .perform_sync();
    return ok;
}

} // namespace

bool tv_lg_aviso(const std::string& ip, const std::string& chave_lg_salva, const std::string& texto, std::string& erro)
{
    if (chave_lg_salva.empty()) {
        erro = "A TV ainda não foi pareada com este computador. Abra a TV em Dispositivo › Casa e toque em Parear.";
        return false;
    }
    const Lg r = lg(ip, chave_lg_salva, texto, 20);
    if (!r.ok)
        erro = r.erro;
    return r.ok;
}

bool tapo_carregar()
{
    if (tapo_tem_conta())
        return true;
    AppConfig* cfg = wxGetApp().app_config;
    const json c   = cfg != nullptr ? json::parse(cfg->get("filuna_casa_tapo"), nullptr, false) : json();
    if (!c.is_object() || c.value("email", std::string()).empty())
        return false;
    tapo_conta(c.value("email", std::string()), c.value("senha", std::string()));
    return true;
}

std::string chave_gravada(const std::string& prefixo, const std::string& ip)
{
    AppConfig* cfg = wxGetApp().app_config;
    return cfg != nullptr ? cfg->get(prefixo + id_seguro(ip)) : std::string();
}

std::string chave_lg(const std::string& ip)
{
    AppConfig* cfg = wxGetApp().app_config;
    return cfg != nullptr ? cfg->get(chave_config_lg(ip)) : std::string();
}

bool tomada_ligar(const std::string& protocolo, const std::string& ip, bool ligar, std::string& erro)
{
    if (protocolo == "shelly")
        return shelly_ligar(ip, ligar, erro);
    if (protocolo == "kasa") {
        json r;
        if (!kasa_pedido(ip, {{"system", {{"set_relay_state", {{"state", ligar ? 1 : 0}}}}}}, r, erro))
            return false;
        const int code = r.value("system", json::object()).value("set_relay_state", json::object()).value("err_code", -1);
        if (code != 0)
            erro = "A tomada recusou o comando.";
        return code == 0;
    }
    if (protocolo == "tapo") {
        json r;
        return tapo_pedido(ip, {{"method", "set_device_info"}, {"params", {{"device_on", ligar}}}}, r, erro);
    }
    if (protocolo == "tasmota") {
        std::string corpo;
        if (http_get("http://" + ip + "/cm?cmnd=Power%20" + std::string(ligar ? "On" : "Off"), corpo))
            return true;
        erro = "A tomada não respondeu em " + ip + ".";
        return false;
    }
    erro = "Esta tomada ainda não aceita comando direto pelo Filuna.";
    return false;
}

json tomada_estado(const std::string& protocolo, const std::string& ip)
{
    if (protocolo == "shelly")
        return shelly_estado(ip);
    if (protocolo == "kasa") {
        json        r;
        std::string erro;
        if (!kasa_pedido(ip, {{"system", {{"get_sysinfo", json::object()}}}, {"emeter", {{"get_realtime", json::object()}}}}, r, erro))
            return {{"ok", false}, {"msg", erro}};
        const json si = r.value("system", json::object()).value("get_sysinfo", json::object());
        json       o  = {{"ok", true}, {"ligada", si.value("relay_state", 0) == 1}};
        const json em = r.value("emeter", json::object()).value("get_realtime", json::object());
        if (em.value("err_code", -1) == 0)
            o["potencia_w"] = em.contains("power_mw") ? em.value("power_mw", 0.0) / 1000.0 : em.value("power", 0.0);
        return o;
    }
    if (protocolo == "tapo") {
        json        r, en;
        std::string erro;
        if (!tapo_pedido(ip, {{"method", "get_device_info"}}, r, erro))
            return {{"ok", false}, {"msg", erro}};
        json o = {{"ok", true}, {"ligada", r.value("device_on", false)}};
        if (tapo_pedido(ip, {{"method", "get_energy_usage"}}, en, erro) && en.contains("current_power"))
            o["potencia_w"] = en.value("current_power", 0.0) / 1000.0;
        return o;
    }
    if (protocolo == "tasmota") {
        std::string corpo;
        if (!http_get("http://" + ip + "/cm?cmnd=Status%208", corpo) || corpo.empty())
            return {{"ok", false}, {"msg", "A tomada não respondeu em " + ip + "."}};
        std::string p1;
        http_get("http://" + ip + "/cm?cmnd=Power", p1);
        const json st = json::parse(corpo, nullptr, false), pw = json::parse(p1, nullptr, false);
        json o = {{"ok", true}, {"ligada", pw.is_object() && pw.value("POWER", std::string()) == "ON"}};
        if (st.is_object() && st.contains("StatusSNS") && st["StatusSNS"].contains("ENERGY"))
            o["potencia_w"] = st["StatusSNS"]["ENERGY"].value("Power", 0.0);
        return o;
    }
    return {{"ok", false}, {"msg", "Esta tomada ainda não aceita comando direto pelo Filuna."}};
}

bool luz_aviso(const std::string& protocolo, const std::string& ip, const std::string& cor_hex, int segundos, std::string& erro)
{
    segundos = std::clamp(segundos, 2, 30);
    if (protocolo == "kasa") {
        const char* SV = "smartlife.iot.smartbulb.lightingservice";
        json        antes;
        if (!kasa_pedido(ip, {{SV, {{"get_light_state", json::object()}}}}, antes, erro))
            return false;
        json est = antes.value(SV, json::object()).value("get_light_state", json::object());
        int  h = 0, s = 0, v = 100;
        hex_hsv(cor_hex, h, s, v);
        json r;
        for (int vez = 0; vez < 3; ++vez) { // pisca 3 vezes na cor do aviso
            kasa_pedido(ip, {{SV, {{"transition_light_state", {{"on_off", 1}, {"hue", h}, {"saturation", s}, {"brightness", 100}, {"color_temp", 0}, {"transition_period", 0}}}}}}, r, erro);
            std::this_thread::sleep_for(500ms);
            kasa_pedido(ip, {{SV, {{"transition_light_state", {{"on_off", 1}, {"brightness", 5}, {"transition_period", 0}}}}}}, r, erro);
            std::this_thread::sleep_for(300ms);
        }
        kasa_pedido(ip, {{SV, {{"transition_light_state", {{"on_off", 1}, {"hue", h}, {"saturation", s}, {"brightness", 100}, {"color_temp", 0}, {"transition_period", 0}}}}}}, r, erro);
        std::this_thread::sleep_for(std::chrono::seconds(segundos));
        // volta ao que estava (desligada continua desligada)
        json volta = {{"on_off", est.value("on_off", 1)}, {"transition_period", 500}};
        const json base = est.value("on_off", 1) ? est : est.value("dft_on_state", json::object());
        for (const char* k : {"hue", "saturation", "brightness", "color_temp"})
            if (base.contains(k))
                volta[k] = base[k];
        erro.clear();
        return kasa_pedido(ip, {{SV, {{"transition_light_state", volta}}}}, r, erro);
    }
    if (protocolo == "wled") {
        std::string corpo;
        if (!http_get("http://" + ip + "/json/state", corpo)) {
            erro = "A luz não respondeu em " + ip + ".";
            return false;
        }
        const json antes = json::parse(corpo, nullptr, false);
        int        r = 0, g = 0, b = 0;
        hex_rgb(cor_hex, r, g, b);
        std::string resp;
        // efeito 1 do WLED = piscar, na cor do aviso
        if (!http_post_json("http://" + ip + "/json/state", {{"on", true}, {"bri", 255}, {"seg", json::array({{{"col", json::array({json::array({r, g, b})})}, {"fx", 1}, {"sx", 200}}})}}, resp)) {
            erro = "A luz recusou o comando.";
            return false;
        }
        std::this_thread::sleep_for(std::chrono::seconds(segundos));
        if (antes.is_object()) {
            json volta = {{"on", antes.value("on", true)}, {"bri", antes.value("bri", 128)}};
            if (antes.contains("seg"))
                volta["seg"] = antes["seg"];
            http_post_json("http://" + ip + "/json/state", volta, resp);
        }
        return true;
    }
    if (protocolo == "tapo") {
        json antes;
        if (!tapo_pedido(ip, {{"method", "get_device_info"}}, antes, erro))
            return false;
        int h = 0, s = 0, v = 100;
        hex_hsv(cor_hex, h, s, v);
        json r;
        for (int vez = 0; vez < 3; ++vez) {
            tapo_pedido(ip, {{"method", "set_device_info"}, {"params", {{"device_on", true}, {"hue", h}, {"saturation", s}, {"brightness", 100}, {"color_temp", 0}}}}, r, erro);
            std::this_thread::sleep_for(500ms);
            tapo_pedido(ip, {{"method", "set_device_info"}, {"params", {{"brightness", 5}}}}, r, erro);
            std::this_thread::sleep_for(300ms);
        }
        tapo_pedido(ip, {{"method", "set_device_info"}, {"params", {{"device_on", true}, {"hue", h}, {"saturation", s}, {"brightness", 100}, {"color_temp", 0}}}}, r, erro);
        std::this_thread::sleep_for(std::chrono::seconds(segundos));
        json volta = {{"device_on", antes.value("device_on", true)}};
        for (const char* k : {"hue", "saturation", "brightness", "color_temp"})
            if (antes.contains(k))
                volta[k] = antes[k];
        erro.clear();
        return tapo_pedido(ip, {{"method", "set_device_info"}, {"params", volta}}, r, erro);
    }
    erro = "Esta luz ainda não aceita comando direto pelo Filuna.";
    return false;
}

void tapo_conta(const std::string& email, const std::string& senha)
{
    std::lock_guard<std::mutex> g(trava_tapo());
    conta_tapo() = {email, senha};
}

bool tapo_tem_conta()
{
    std::lock_guard<std::mutex> g(trava_tapo());
    return !conta_tapo().first.empty();
}

std::string hue_parear(const std::string& ip, std::string& erro)
{
    std::string resp;
    http_post_json("http://" + ip + "/api", {{"devicetype", "filuna#slicer"}}, resp);
    const json j = json::parse(resp, nullptr, false);
    if (j.is_array() && !j.empty() && j[0].contains("success"))
        return j[0]["success"].value("username", std::string());
    erro = j.is_array() && !j.empty() && j[0].contains("error") && j[0]["error"].value("type", 0) == 101 ?
               "Aperte o botão redondo no meio da ponte Hue e toque em Parear de novo (em até 30 segundos)." :
               "A ponte Hue não respondeu em " + ip + ".";
    return {};
}

bool hue_aviso(const std::string& ip, const std::string& usuario, const std::string& cor_hex, int segundos, std::string& erro)
{
    if (usuario.empty()) {
        erro = "Pareie a ponte Hue com este computador primeiro.";
        return false;
    }
    const std::string base = "http://" + ip + "/api/" + usuario;
    std::string       corpo;
    if (!http_get(base + "/lights", corpo)) {
        erro = "A ponte Hue não respondeu em " + ip + ".";
        return false;
    }
    const json luzes = json::parse(corpo, nullptr, false);
    if (!luzes.is_object()) {
        erro = "A ponte Hue recusou o pareamento deste computador. Pareie de novo.";
        return false;
    }
    int h = 0, s = 0, v = 100;
    hex_hsv(cor_hex, h, s, v);
    std::string r;
    http_put_json(base + "/groups/0/action", {{"on", true}, {"hue", h * 65535 / 360}, {"sat", s * 254 / 100}, {"bri", 254}, {"alert", "lselect"}}, r);
    std::this_thread::sleep_for(std::chrono::seconds(std::clamp(segundos, 2, 30)));
    for (const auto& [id, luz] : luzes.items()) { // cada luz volta como estava
        const json st = luz.value("state", json::object());
        json       volta = {{"on", st.value("on", false)}, {"alert", "none"}};
        if (st.contains("bri"))
            volta["bri"] = st["bri"];
        const std::string modo = st.value("colormode", std::string());
        if (modo == "ct" && st.contains("ct"))
            volta["ct"] = st["ct"];
        else if (modo == "xy" && st.contains("xy"))
            volta["xy"] = st["xy"];
        else if (modo == "hs") {
            volta["hue"] = st.value("hue", 0);
            volta["sat"] = st.value("sat", 0);
        }
        http_put_json(base + "/lights/" + id + "/state", volta, r);
    }
    return true;
}

std::string tv_filuna_parear(const std::string& ip, const std::string& codigo, std::string& erro)
{
    std::string resp;
    if (!http_post_json("http://" + ip + ":8787/parear", {{"codigo", codigo}, {"nome", "Filuna Slicer"}}, resp)) {
        const json e = json::parse(resp, nullptr, false);
        erro = e.is_object() && e.contains("erro") ? e.value("erro", std::string()) : "O app Filuna TV não respondeu em " + ip + ". Abra o app na TV.";
        return {};
    }
    const json j = json::parse(resp, nullptr, false);
    return j.is_object() ? j.value("token", std::string()) : std::string();
}

bool tv_filuna_aviso(const std::string& ip, const std::string& token, const json& aviso, std::string& erro)
{
    if (token.empty()) {
        erro = "Pareie a TV: abra o app Filuna TV e digite o código em Dispositivo › Casa.";
        return false;
    }
    bool        ok = false;
    std::string resp;
    Http::post("http://" + ip + ":8787/aviso")
        .header("Content-Type", "application/json")
        .header("Authorization", "Bearer " + token)
        .set_post_body(aviso.dump())
        .timeout_connect(3)
        .timeout_max(8)
        .on_complete([&](std::string b, unsigned st) { resp = std::move(b); ok = st < 400; })
        .on_error([&](std::string b, std::string e, unsigned st) { erro = st == 401 ? "A TV não reconhece mais este computador: pareie de novo." : "A TV não respondeu (" + e + ")."; })
        .perform_sync();
    return ok;
}

bool ha_evento(const json& cfg_ha, const json& aviso, std::string& erro)
{
    Filuna::MqttClient cli;
    if (!ha_conectar(cli, cfg_ha, erro))
        return false;
    // Gatilhos de automação: no Home Assistant aparecem como "Filuna · Impressão concluída" e os outros
    static const std::vector<std::pair<std::string, std::string>> TIPOS = {
        {"concluida", "Impressão concluída"}, {"erro", "Erro de impressão"}, {"pausada", "Impressão pausada"},
        {"filamento", "Filamento trocado"}, {"venda", "Nova venda"}, {"mensagem", "Aviso do Filuna"}, {"teste", "Aviso de teste"}};
    const json dev = {{"identifiers", {"filuna_casa"}}, {"name", "Filuna"}, {"manufacturer", "Filuna 3D"}, {"model", "Filuna Home"}};
    for (const auto& [t, nome] : TIPOS)
        cli.publish("homeassistant/device_automation/filuna_" + t + "/config",
                    json({{"automation_type", "trigger"}, {"topic", "filuna/evento/" + t}, {"type", "filuna"}, {"subtype", nome}, {"device", dev}}).dump(), true);
    cli.publish("filuna/evento/" + aviso.value("evento", std::string("mensagem")), aviso.dump(), false);
    cli.publish("filuna/evento", aviso.dump(), false);
    cli.close();
    return true;
}

json procurar()
{
    {
        std::lock_guard<std::mutex> g(trava());
        if (ultima_busca().value("rodando", false))
            return {{"rodando", true}};
        ultima_busca()["rodando"] = true;
    }
    std::thread([] {
        try {
            buscar_tudo();
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Filuna Home: busca falhou: " << e.what();
            std::lock_guard<std::mutex> g(trava());
            ultima_busca()["rodando"] = false;
        }
    }).detach();
    return {{"rodando", true}};
}

json resultado()
{
    std::lock_guard<std::mutex> g(trava());
    return ultima_busca();
}

json acao(const json& pedido)
{
    const std::string a = pedido.value("acao", std::string());
    if (a == "ha.configurar") { // guarda o MQTT da casa neste computador (a senha não vai para o servidor)
        if (AppConfig* cfg = wxGetApp().app_config) {
            const json ha_cfg = {{"broker", pedido.value("broker", std::string())}, {"usuario", pedido.value("usuario", std::string())},
                                 {"senha", pedido.value("senha", std::string())}, {"ligado", pedido.value("ligado", true)}};
            cfg->set("filuna_casa_ha", ha_cfg.dump());
            cfg->save();
        }
        std::lock_guard<std::mutex> g(ha().m);
        ha().assinatura.clear(); // reconecta com o novo endereço na próxima leitura
        return {{"ok", true}};
    }
    if (a == "ha.estado") {
        json ha_cfg = json::object();
        if (AppConfig* cfg = wxGetApp().app_config)
            ha_cfg = json::parse(cfg->get("filuna_casa_ha"), nullptr, false);
        if (!ha_cfg.is_object())
            ha_cfg = json::object();
        ha_cfg.erase("senha");
        std::lock_guard<std::mutex> g(ha().m);
        ha_cfg["conectado"] = ha().cli != nullptr && ha().cli->connected();
        return ha_cfg;
    }
    if (a == "tapo.conta") { // conta Tapo do cliente: fica só neste computador
        const std::string email = pedido.value("email", std::string()), senha = pedido.value("senha", std::string());
        if (!email.empty() && !senha.empty()) {
            if (AppConfig* cfg = wxGetApp().app_config) {
                cfg->set("filuna_casa_tapo", json({{"email", email}, {"senha", senha}}).dump());
                cfg->save();
            }
            tapo_conta(email, senha);
        }
    }
    if (a == "tapo.tem_conta")
        return {{"conta", tapo_carregar()}};
    if (a == "hue.pareada" || a == "filunatv.pareada") {
        AppConfig* cfg = wxGetApp().app_config;
        const std::string k = (a == "hue.pareada" ? "filuna_casa_hue_" : "filuna_casa_tv_") + id_seguro(pedido.value("ip", std::string()));
        return {{"pareada", cfg != nullptr && !cfg->get(k).empty()}};
    }
    if (a == "lg.pareada") {
        AppConfig* cfg = wxGetApp().app_config;
        return {{"pareada", cfg != nullptr && !cfg->get(chave_config_lg(pedido.value("ip", std::string()))).empty()}};
    }
    std::string chave_lg;
    if (AppConfig* cfg = wxGetApp().app_config)
        chave_lg = cfg->get(chave_config_lg(pedido.value("ip", std::string())));
    std::thread(executar, pedido, chave_lg, json::object()).detach();
    return {{"aceito", true}};
}

void publicar_home_assistant(const json& estado_impressoras)
{
    AppConfig* cfg = wxGetApp().app_config;
    if (cfg == nullptr)
        return;
    const json ha_cfg = json::parse(cfg->get("filuna_casa_ha"), nullptr, false);
    if (!ha_cfg.is_object() || !ha_cfg.value("ligado", false) || ha_cfg.value("broker", std::string()).empty())
        return;
    std::thread([ha_cfg, estado_impressoras] {
        Ha&                         h = ha();
        std::lock_guard<std::mutex> g(h.m);
        const std::string assinatura = ha_cfg.value("broker", std::string()) + "|" + ha_cfg.value("usuario", std::string()) + "|" + ha_cfg.value("senha", std::string());
        if (!h.cli || !h.cli->connected() || h.assinatura != assinatura) {
            if (std::chrono::steady_clock::now() - h.tentativa < 60s && h.assinatura == assinatura)
                return; // não insiste mais que uma vez por minuto se o MQTT está fora
            h.tentativa = std::chrono::steady_clock::now();
            h.cli       = std::make_unique<Filuna::MqttClient>();
            h.anunciadas.clear();
            h.assinatura = assinatura;
            std::string erro;
            if (!ha_conectar(*h.cli, ha_cfg, erro)) {
                BOOST_LOG_TRIVIAL(info) << "Filuna Home: MQTT do Home Assistant fora: " << erro;
                h.cli.reset();
                return;
            }
        }
        ha_publicar(*h.cli, h.anunciadas, estado_impressoras, ha_cfg);
    }).detach();
}

}}} // namespace Slic3r::GUI::FilunaCasa
