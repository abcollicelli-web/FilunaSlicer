#include <boost/asio.hpp> // no Windows o boost precisa vir antes dos cabeçalhos do wxWidgets
#include "FilunaDispositivos.hpp"

#include "FilunaAtualizacao.hpp"
#include "FilunaDados.hpp"
#include "FilunaPanel.hpp"
#include "GUI_App.hpp"
#include "HMS.hpp"
#include "MainFrame.hpp"
#include "Plater.hpp"
#include "Tab.hpp"
#include "DeviceManager.hpp"
#include "DeviceCore/DevManager.h"
#include "DeviceCore/DevBed.h"
#include "DeviceCore/DevExtruderSystem.h"
#include "DeviceCore/DevFilaSystem.h"
#include "DeviceCore/DevFan.h"
#include "DeviceCore/DevLamp.h"
#include "DeviceCore/DevDefs.h"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/Filuna/FilunaDb.hpp"
#include "slic3r/Utils/Http.hpp"
#include "slic3r/Utils/NetworkAgent.hpp"
#include "slic3r/Utils/NetworkAgentFactory.hpp"
#include "slic3r/Utils/FilunaCameraUrls.hpp"
#include "slic3r/Utils/FilunaCameraRelay.hpp"
#include "slic3r/Utils/FilunaRede.hpp"

#include <boost/algorithm/string.hpp>
#include <wx/notifmsg.h>
#include <wx/timer.h>
#include "NotificationManager.hpp"
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>
#include <vector>

namespace Slic3r { namespace GUI { namespace FilunaDispositivos {

namespace {

const char* const SITE = "https://filuna3d.com";

// módulo do catálogo -> fabricante dos perfis do Orca (resources/profiles/<Vendor>.json)
const std::map<std::string, std::string> VENDOR = {
    {"anycubic", "Anycubic"}, {"bambu", "BBL"},   {"creality", "Creality"},   {"elegoo", "Elegoo"},
    {"flashforge", "Flashforge"}, {"qidi", "Qidi"}, {"prusa", "Prusa"},     {"snapmaker", "Snapmaker"},
};

// módulo -> agente de rede da marca (ids em NetworkAgentFactory::register_all_agents)
const std::map<std::string, std::string> AGENTE = {
    {"anycubic", "filuna_anycubic_lan"}, // Modo LAN de fábrica (porta 18910 + MQTT/TLS); Moonraker só existe com Rinkhals
    {"bambu", "filuna_bambu_lan"},      // LAN aberto (MQTT/FTPS), sem plug-in fechado
    {"creality", "crealityprint"},
    {"elegoo", "filuna_elegoo_cc2"},    // Centauri Carbon 2; CC/Neptune: SDCP (ver agente_da_marca)
    {"flashforge", "filuna_flashforge_lan"},
    {"qidi", "qidi"},
    {"prusa", "filuna_prusalink"},
    {"snapmaker", "snapmaker"},
};

// Controles que cada motor de rede sabe executar na impressora (a tela só mostra os que existem).
// CC2: métodos do SDK Elegoo-Link (1026 home, 1027 mover, 1028 temperatura, 1029 luz, 1030 ventiladores, 1031
// velocidade, 2001/2002 carregar/descarregar pelo Canvas, 1007 parada). Anycubic Modo LAN: print/update e light/control.
const std::map<std::string, std::vector<std::string>> CONTROLES = {
    {"filuna_elegoo_cc2", {"bico", "mesa", "home", "mover", "luz", "ventilador", "ventilador_aux", "ventilador_caixa", "velocidade",
                           "carregar", "descarregar", "emergencia", "arquivos"}},
    {"filuna_anycubic_lan", {"bico", "mesa", "home", "mover", "luz", "ventilador", "ventilador_aux", "ventilador_caixa", "velocidade",
                             "carregar", "descarregar", "secar", "arquivos"}},
    // Klipper/Moonraker (Snapmaker U1, Qidi, Creality K): G-code pelo Moonraker (ver gcode_moonraker)
    {"snapmaker", {"bico", "mesa", "home", "mover", "ventilador", "velocidade", "emergencia"}},
    {"qidi", {"bico", "mesa", "home", "mover", "ventilador", "velocidade", "emergencia"}},
    {"crealityprint", {"bico", "mesa", "home", "mover", "ventilador", "velocidade", "emergencia"}},
    {"moonraker", {"bico", "mesa", "home", "mover", "ventilador", "velocidade", "emergencia"}},
};

// Controle da tela -> G-code do Klipper (o agente Moonraker executa "gcode_line" pelo /printer/gcode/script).
std::string gcode_moonraker(const nlohmann::json& f)
{
    const std::string tipo = f.value("command", std::string());
    if (tipo == "bico")
        return "M104 S" + std::to_string(f.value("valor", 0));
    if (tipo == "mesa")
        return "M140 S" + std::to_string(f.value("valor", 0));
    if (tipo == "home") {
        std::string g = "G28";
        for (char e : f.value("eixos", std::string("xyz")))
            g += std::string(" ") + char(std::toupper(static_cast<unsigned char>(e)));
        return f.value("eixos", std::string("xyz")) == "xyz" ? std::string("G28") : g;
    }
    if (tipo == "mover") {
        const std::string e = f.value("eixo", std::string("x"));
        std::ostringstream g;
        g << "G91\nG1 " << char(std::toupper(static_cast<unsigned char>(e[0]))) << f.value("distancia", 0.0) << " F" << (e == "z" ? 600 : 3000)
          << "\nG90";
        return g.str();
    }
    if (tipo == "ventilador")
        return "M106 S" + std::to_string(f.value("valor", 0) * 255 / 100);
    if (tipo == "velocidade") { // 0 silencioso, 1 padrão, 2 esporte, 3 turbo (mesma escala da Bambu)
        static const int pct[] = {50, 100, 124, 166};
        return "M220 S" + std::to_string(pct[std::max(0, std::min(3, f.value("valor", 1)))]);
    }
    if (tipo == "emergencia")
        return "M112";
    return std::string();
}

// Marcas sem tabela própria acima falam os comandos padrão do programa (os mesmos do painel nativo): Bambu
// (LAN ou conta), Snapmaker, QIDI, Creality, Klipper/Moonraker. Para elas os controles saem pelos comandos do
// MachineObject, numa regra só — marca nova que fale o padrão já nasce com controles.
const std::vector<std::string> CONTROLES_PADRAO = {"bico", "mesa", "home", "mover", "luz", "ventilador", "ventilador_aux",
                                                   "ventilador_caixa", "velocidade"};

const std::vector<std::string>& controles_do_agente(const std::string& agente)
{
    auto it = CONTROLES.find(agente);
    return it != CONTROLES.end() ? it->second : CONTROLES_PADRAO;
}

// Controles que o protocolo da marca só aceita com uma impressão em andamento. Parada, a tela não os oferece
// (e diz por quê) em vez de mostrar um botão que a impressora ignora.
// Anycubic Modo LAN: a velocidade vai por print/update, que só vale durante o trabalho. Parada, a impressora aceita
// aquecimento (tempature/set), ventiladores (fan/setSpeed), eixos (axis/move) e carregar/descarregar (feedFilament).

const std::map<std::string, std::vector<std::string>> SO_IMPRIMINDO = {
    {"filuna_anycubic_lan", {"velocidade"}},
};

std::string minusculas(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

// "Anycubic Kobra 3 V2 0.4 nozzle" -> começa por "Anycubic Kobra 3 V2"
bool nome_cobre_modelo(const std::string& nome_perfil, const std::string& modelo)
{
    return nome_perfil == modelo || nome_perfil.rfind(modelo + " ", 0) == 0;
}

// Catálogo e base de perfis nem sempre escrevem a marca igual ("Q1 Pro 0.4 nozzle" × "Qidi Q1 Pro",
// "Original Prusa MINI" × "Prusa MINI"): compara sem o prefixo da marca e sem diferença de maiúsculas.
std::string sem_marca(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    for (const char* p : {"original prusa ", "prusa ", "qidi ", "original "})
        if (s.rfind(p, 0) == 0) {
            s.erase(0, std::string(p).size());
            break;
        }
    return s;
}

bool casa_modelo(const std::string& nome_perfil, const std::string& modelo)
{
    return nome_cobre_modelo(nome_perfil, modelo) || nome_cobre_modelo(sem_marca(nome_perfil), sem_marca(modelo));
}

const json* modulo_do_catalogo(const json& cat, const std::string& id, json& guarda)
{
    for (const auto& m : cat.value("modulos", json::array()))
        if (m.value("id", std::string()) == id) {
            guarda = m;
            return &guarda;
        }
    return nullptr;
}

// Modelos homologados do módulo: pela política (Anycubic em modo Orca) ou pelos perfis do pacote.
std::vector<std::string> homologados(const json& mod)
{
    std::vector<std::string> out;
    if (mod.contains("politica_motor"))
        for (const auto& n : mod["politica_motor"].value("impressoras_disponiveis", json::array()))
            if (n.is_string())
                out.push_back(n.get<std::string>());
    for (const auto& c : mod.value("componentes", json::array()))
        for (const auto& n : c.value("impressoras", json::array()))
            if (n.is_string())
                out.push_back(n.get<std::string>());
    return out;
}

std::vector<std::string> indisponiveis(const json& mod)
{
    std::vector<std::string> out;
    if (mod.contains("politica_motor"))
        for (const auto& n : mod["politica_motor"].value("impressoras_indisponiveis", json::array()))
            if (n.is_string())
                out.push_back(n.get<std::string>());
    return out;
}

std::string mensagem_indisponivel(const json& mod)
{
    if (mod.contains("politica_motor"))
        return mod["politica_motor"].value("mensagem_indisponivel",
                                           std::string("Impressora não disponível: este modelo ainda não tem perfil homologado no Filuna."));
    return "Impressora não disponível: este modelo ainda não foi homologado no Filuna.";
}

const VendorProfile* vendor_profile(const std::string& vendor_id)
{
    PresetBundle* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return nullptr;
    auto it = bundle->vendors.find(vendor_id);
    return it == bundle->vendors.end() ? nullptr : &it->second;
}

// Modelos da base de perfis (resources/profiles/<Vendor>.json), presentes em TODA instalação.
// bundle->vendors só tem fabricantes já instalados pelo usuário — num Filuna recém-instalado está
// vazio e o assistente mostrava "0 modelo(s)" para tudo (beta 14/11). A base é a fonte da lista.
struct ModeloBase
{
    std::string              id, nome, familia;
    std::vector<std::string> bicos;
};

std::vector<ModeloBase> modelos_da_base(const std::string& vendor_id)
{
    std::vector<ModeloBase> out;
    const boost::filesystem::path dir = boost::filesystem::path(resources_dir()) / "profiles";
    boost::filesystem::ifstream   f(dir / (vendor_id + ".json"), std::ios::binary);
    if (!f.good())
        return out;
    json v = json::parse(f, nullptr, false);
    if (v.is_discarded())
        return out;
    for (const auto& e : v.value("machine_model_list", json::array())) {
        ModeloBase m;
        m.nome = e.value("name", std::string());
        if (m.nome.empty())
            continue;
        const std::string sub = e.value("sub_path", std::string());
        if (!sub.empty()) {
            boost::filesystem::ifstream fs(dir / vendor_id / sub, std::ios::binary);
            if (fs.good()) {
                json d = json::parse(fs, nullptr, false);
                if (!d.is_discarded()) {
                    m.id      = d.value("model_id", std::string());
                    m.familia = d.value("family", vendor_id);
                    std::string       nd = d.value("nozzle_diameter", std::string());
                    std::stringstream ss(nd);
                    std::string       b;
                    while (std::getline(ss, b, ';'))
                        if (!b.empty())
                            m.bicos.push_back(b);
                }
            }
        }
        out.push_back(std::move(m));
    }
    return out;
}

// Caminho relativo à página (resources/web/filuna/) para a foto oficial do modelo (cover do Orca).
std::string cover(const std::string& vendor_id, const std::string& model_id, const std::string& nome = std::string())
{
    // Cada marca nomeia a foto de um jeito: pelo model_id (Anycubic, Bambu…) ou pelo nome do modelo (Elegoo:
    // "Elegoo Centauri Carbon 2_cover.png", model_id "Elegoo-CC2"). Tenta os dois, para todas as marcas.
    for (const std::string& base : {model_id, nome}) {
        if (base.empty())
            continue;
        const boost::filesystem::path p = boost::filesystem::path(resources_dir()) / "profiles" / vendor_id / (base + "_cover.png");
        if (boost::filesystem::exists(p)) {
            std::string rel = "../../profiles/" + vendor_id + "/" + base + "_cover.png";
            boost::replace_all(rel, " ", "%20");
            return rel;
        }
    }
    return std::string();
}

// Preset de máquina do sistema para o modelo (bico padrão 0.4 quando houver).
const Preset* preset_do_modelo(const std::string& vendor_id, const std::string& modelo)
{
    PresetBundle* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return nullptr;
    const Preset* melhor = nullptr;
    for (const Preset& p : bundle->printers) {
        if (!p.is_system || p.vendor == nullptr || p.vendor->id != vendor_id)
            continue;
        const std::string pm = p.config.has("printer_model") ? p.config.opt_string("printer_model") : std::string();
        if (pm != modelo && !nome_cobre_modelo(p.name, modelo))
            continue;
        if (melhor == nullptr || p.name.find("0.4") != std::string::npos)
            melhor = &p;
        if (p.name.find("0.4") != std::string::npos)
            break;
    }
    return melhor;
}

std::string estado_legivel(MachineObject* o)
{
    if (o == nullptr)
        return "desligada";
    if (!o->is_online())
        return "desligada";
    if (o->print_status == "FAILED")
        return "falhou";
    if (o->is_in_printing_pause())
        return "pausada";
    if (o->is_in_printing())
        return "imprimindo";
    if (o->print_status == "FINISH")
        return "concluida";
    return "livre";
}

// Entradas em que o filamento foi trocado depois de conectar (dev_id -> {"unidade:slot"}). Fica até o cliente
// responder a pergunta do estoque na tela de Dispositivos ("dispositivos.troca_vista").
std::map<std::string, std::set<std::string>>& trocados()
{
    static std::map<std::string, std::set<std::string>> t;
    return t;
}

json cores_de(MachineObject* o)
{
    json cores = json::array();
    auto fila = o->GetFilaSystem();
    if (!fila)
        return cores;
    for (auto& [ams_id, ams] : fila->GetAmsList()) {
        if (ams == nullptr)
            continue;
        for (auto& [tray_id, tray] : ams->GetTrays()) {
            if (tray == nullptr)
                continue;
            cores.push_back({{"unidade", ams_id}, {"slot", tray_id}, {"cor", tray->color}, {"material", tray->m_fila_type}, {"marca", tray->sub_brands},
                             {"vazio", tray->color.empty()}});
        }
    }
    return cores;
}

json card(MachineObject* o)
{
    json c = {{"dev_id", o->get_dev_id()}, {"nome", o->get_dev_name()}, {"ip", o->get_dev_ip()}, {"modelo", o->printer_type},
              {"modelo_nome", std::string(o->get_printer_type_display_str().ToUTF8().data())},
              {"ligada", o->is_online()},      {"estado", estado_legivel(o)},
              {"conexao", o->dev_connection_type}, {"agente", o->printer_agent_id},
              {"progresso", o->mc_print_percent}, {"restante_s", o->mc_left_time},
              {"camera", o->has_ipcam},          {"cores", cores_de(o)},
              {"sinc_filamentos", true}}; // a tela pode pedir "dispositivos.filamentos_projeto"
    if (DevBed* bed = o->GetBed())
        c["mesa"] = {{"atual", bed->GetBedTemp()}};
    if (DevExtderSystem* ext = o->GetExtderSystem())
        c["bico"] = {{"atual", ext->GetNozzleTempCurrent(0)}, {"alvo", ext->GetNozzleTempTarget(0)}};
    auto ctl = CONTROLES.find(o->printer_agent_id);
    json controles = json::array(), espera = json::array();
    {
        auto so = SO_IMPRIMINDO.find(o->printer_agent_id);
        const bool imprimindo = o->is_in_printing();
        std::vector<std::string> lista = controles_do_agente(o->printer_agent_id);
        if (ctl == CONTROLES.end() && !cores_de(o).empty()) { // padrão + sistema de cores: carregar/descarregar
            lista.push_back("carregar");
            lista.push_back("descarregar");
        }
        for (const std::string& k : lista) {
            const bool precisa = so != SO_IMPRIMINDO.end() && std::find(so->second.begin(), so->second.end(), k) != so->second.end();
            (precisa && !imprimindo ? espera : controles).push_back(k);
        }
    }
    // estado real da luz, para as marcas cujo protocolo informa (a tela usa no lugar do "último comando enviado")
    {
        const std::string& ag = o->printer_agent_id;
        if ((ag == "filuna_anycubic_lan" || ag == "filuna_elegoo_cc2" || ag == "filuna_bambu_lan" || ag == "bbl") && o->GetLamp() != nullptr)
            c["luz"] = o->GetLamp()->IsChamberLightOn();
    }
    c["controles"]           = controles;
    c["controles_impressao"] = espera; // existem, mas só durante uma impressão
    // Bambu em modo LAN: A1, A1 mini, P1 e A2 mandam JPEG pela porta 6000 (rtsp_url "disable"); P2S, X1 e H2 usam
    // RTSPS (rtsp_url "rtsps://..."), que este repasse ainda não converte.
    if (o->printer_agent_id == "filuna_bambu_lan" && !o->get_access_code().empty() && !o->get_dev_ip().empty() &&
        !boost::algorithm::starts_with(o->local_rtsp_url, "rtsp")) {
        Filuna::CameraRelay::set_bambu(o->get_dev_id(), o->get_dev_ip(), o->get_access_code());
        if (const int porta = Filuna::CameraRelay::porta(); porta > 0) {
            c["camera"]         = true;
            c["camera_url"]     = "bambu-lan";
            c["camera_repasse"] = "http://127.0.0.1:" + std::to_string(porta) + "/bambu?d=" + Http::url_encode(o->get_dev_id());
        }
    } else if (o->printer_agent_id == "filuna_bambu_lan" && boost::algorithm::starts_with(o->local_rtsp_url, "rtsps://") &&
               !o->get_access_code().empty()) {
        // P2S, X1, H2: H.264 por RTSPS, convertido para FLV no repasse (tocador mpegts.js da tela)
        Filuna::CameraRelay::set_bambu(o->get_dev_id(), o->get_dev_ip(), o->get_access_code());
        Filuna::CameraRelay::set_bambu_rtsp(o->get_dev_id(), o->local_rtsp_url);
        if (const int porta = Filuna::CameraRelay::porta(); porta > 0) {
            c["camera"]     = true;
            c["camera_url"] = "bambu-rtsp/flv";
            c["camera_flv"] = "http://127.0.0.1:" + std::to_string(porta) + "/bambu-rtsp?d=" + Http::url_encode(o->get_dev_id());
        }
    }
    const std::string cam = Filuna::camera_url(o->get_dev_id());
    if (!cam.empty())
        c["camera_url"] = cam; // câmera ao vivo que o agente da marca descobriu
    // vídeo em HTTP-FLV (Anycubic e outras): a tela lê pelo repasse local, que acrescenta a permissão que a
    // impressora não manda
    // câmera pelo repasse local também para as demais (permite à tela tirar uma foto para a IA conferir a mesa)
    if (cam.size() > 4) {
        std::string h, pt, cm;
        const int   porta = Filuna::CameraRelay::destino_valido(cam, h, pt, cm) ? Filuna::CameraRelay::porta() : 0;
        if (porta > 0)
            c["camera_repasse"] = "http://127.0.0.1:" + std::to_string(porta) + "/cam?u=" + Http::url_encode(cam);
    }
    if (cam.size() > 4 && cam.find("/flv") != std::string::npos) {
        std::string h, pt, cm;
        const int   porta = Filuna::CameraRelay::destino_valido(cam, h, pt, cm) ? Filuna::CameraRelay::porta() : 0;
        if (porta > 0)
            c["camera_flv"] = "http://127.0.0.1:" + std::to_string(porta) + "/cam?u=" + Http::url_encode(cam);
    }
    std::string err = Filuna::erro(o->get_dev_id());
    // Bambu: o erro vem no próprio relatório (print_error), com o texto oficial da Bambu quando conhecido
    if (err.empty() && o->print_error != 0 && o->printer_agent_id == "filuna_bambu_lan") {
        wxString txt;
        if (HMSQuery* q = wxGetApp().get_hms_query())
            txt = q->query_print_error_msg(o, o->print_error);
        err = (txt.empty() ? std::string("A impressora informou um erro") : std::string(txt.ToUTF8().data())) + " (código " + o->get_print_error_str() + ")";
    }
    if (!err.empty())
        c["erro"] = err; // último erro informado pela impressora
    {
        auto tr = trocados().find(o->get_dev_id());
        if (tr != trocados().end() && !tr->second.empty())
            c["trocados"] = json(tr->second); // entradas com filamento trocado, à espera do cadastro no estoque
    }
    const json ajs = json::parse(Filuna::ajustes(o->get_dev_id()), nullptr, false);
    if (ajs.is_object() && !ajs.empty())
        c["ajustes"] = ajs; // ventiladores e velocidade como a impressora informa
    const json arqs = json::parse(Filuna::arquivos(o->get_dev_id()), nullptr, false);
    if (arqs.is_array() && !arqs.empty())
        c["arquivos"] = arqs; // últimos arquivos guardados na impressora
    return c;
}

// dev_id -> id da impressora cadastrada no banco local (vínculo guardado no app_config)
std::string chave_vinculo(const std::string& dev_id) { return "filuna_dispositivo_" + dev_id; }

// Agentes que falam com o Moonraker (Klipper). Eles não procuram a impressora na rede: só anunciam o IP gravado no
// perfil (MoonrakerPrinterAgent::start_discovery). Para estas marcas o Filuna faz a busca na rede local.
bool agente_moonraker(const std::string& ag)
{
    return ag == "snapmaker" || ag == "qidi" || ag == "crealityprint" || ag == "moonraker";
}

// IPs que aceitam conexão TCP na porta dentro do tempo dado (todos ao mesmo tempo, sem travar a tela).
std::vector<std::string> portas_abertas(const std::vector<std::string>& ips, unsigned short porta, std::chrono::milliseconds espera)
{
    namespace net = boost::asio;
    net::io_context io;
    std::vector<std::string> abertos;
    std::vector<std::unique_ptr<net::ip::tcp::socket>> socks;
    socks.reserve(ips.size());
    for (const auto& ip : ips) {
        boost::system::error_code ec;
        const auto addr = net::ip::make_address(ip, ec);
        if (ec)
            continue;
        socks.push_back(std::make_unique<net::ip::tcp::socket>(io));
        net::ip::tcp::socket* s = socks.back().get();
        s->async_connect({addr, porta}, [ip, &abertos](const boost::system::error_code& e) {
            if (!e)
                abertos.push_back(ip);
        });
    }
    net::steady_timer t(io, espera);
    t.async_wait([&socks](const boost::system::error_code&) {
        for (auto& s : socks) {
            boost::system::error_code ig;
            s->close(ig);
        }
    });
    io.run();
    return abertos;
}

// O endereço responde como Moonraker? Devolve o nome da máquina (hostname) quando houver.
bool moonraker_responde(const std::string& host, std::string& nome)
{
    bool ok = false;
    Http::get("http://" + host + "/printer/info")
        .timeout_connect(2)
        .timeout_max(4)
        .on_complete([&ok, &nome](std::string body, unsigned status) {
            if (status != 200)
                return;
            const json j = json::parse(body, nullptr, false);
            if (j.is_object() && j.contains("result") && j["result"].is_object()) {
                ok   = true;
                nome = j["result"].value("hostname", std::string());
            }
        })
        .on_error([&host](std::string, std::string erro, unsigned) {
            BOOST_LOG_TRIVIAL(info) << "Filuna: " << host << " não é Moonraker: " << erro;
        })
        .perform_sync();
    return ok;
}

// Resultado da última busca Klipper, para o diagnóstico da tela ("busca terminou sem achar").
std::mutex& busca_mutex()
{
    static std::mutex m;
    return m;
}
json& busca_info()
{
    static json j = json::object();
    return j;
}

// Procura na rede local impressoras Moonraker (portas 7125 e 80) e as põe na lista da rede do programa com o
// agente da marca escolhida no assistente. Roda numa linha separada; o resultado entra pela linha da tela.
void procurar_moonraker(const std::string& agente, const std::string& modelo, const std::string& printer_type)
{
    static std::atomic<bool> rodando{false};
    if (rodando.exchange(true))
        return;
    std::thread([agente, modelo, printer_type] {
        std::vector<std::string> ips;
        const std::vector<std::string> redes = Filuna::redes_locais();
        {
            std::lock_guard<std::mutex> l(busca_mutex());
            busca_info() = {{"redes", redes}, {"rodando", true}};
        }
        for (const auto& prefixo : redes)
            for (int i = 1; i < 255; ++i)
                ips.push_back(prefixo + std::to_string(i));
        std::vector<std::pair<std::string, std::string>> achadas; // endereço (ip ou ip:porta), nome
        std::set<std::string> vistos;
        for (const unsigned short porta : {static_cast<unsigned short>(7125), static_cast<unsigned short>(80)}) // Moonraker direto, depois atrás do nginx
            for (const auto& ip : portas_abertas(ips, porta, std::chrono::milliseconds(900))) {
                if (vistos.count(ip))
                    continue;
                const std::string host = porta == 80 ? ip : ip + ":" + std::to_string(porta);
                std::string nome;
                if (moonraker_responde(host, nome)) {
                    vistos.insert(ip);
                    achadas.emplace_back(host, nome);
                }
            }
        BOOST_LOG_TRIVIAL(info) << "Filuna: busca Moonraker (" << agente << ") achou " << achadas.size() << " em " << ips.size() << " endereços";
        {
            std::lock_guard<std::mutex> l(busca_mutex());
            busca_info()["rodando"]   = false;
            busca_info()["enderecos"] = ips.size();
            busca_info()["achadas"]   = achadas.size();
        }
        wxGetApp().CallAfter([achadas, agente, modelo, printer_type] {
            DeviceManager* dev = wxGetApp().getDeviceManager();
            if (dev == nullptr)
                return;
            for (const auto& [host, nome] : achadas) {
                const std::string ip = host.substr(0, host.find(':'));
                BBLocalMachine m;
                m.dev_name         = (nome.empty() ? modelo : nome) + " (" + ip + ")";
                m.dev_ip           = host;
                m.dev_id           = "ip-" + ip;
                m.printer_type     = printer_type.empty() ? modelo : printer_type;
                m.printer_agent_id = agente;
                m.access_code      = "lan";
                dev->insert_local_device(m, "lan", "free", "", "lan");
            }
        });
        rodando = false;
    }).detach();
}

} // namespace

std::string vendor_orca(const std::string& modulo_id)
{
    auto it = VENDOR.find(minusculas(modulo_id));
    return it == VENDOR.end() ? std::string() : it->second;
}

std::string agente_da_marca(const std::string& modulo_id, const std::string& modelo)
{
    const std::string id = minusculas(modulo_id);
    if (id == "elegoo" && modelo.find("Carbon 2") == std::string::npos && modelo.find("CC2") == std::string::npos)
        return "filuna_elegoo_sdcp"; // Centauri Carbon / Neptune 4: SDCP v3
    auto it = AGENTE.find(id);
    return it == AGENTE.end() ? std::string("orca") : it->second;
}

json fabricantes()
{
    FilunaAtualizacao& atual = FilunaAtualizacao::instancia();
    const json cat = atual.catalogo();
    json lista = json::array();
    for (const auto& m : cat.value("modulos", json::array())) {
        const std::string id = m.value("id", std::string());
        if (id.empty() || !VENDOR.count(id))
            continue;
        const std::string vid = vendor_orca(id);
        const auto base = modelos_da_base(vid);
        const auto ok = homologados(m);
        int disponiveis = 0;
        for (const auto& pm : base)
            if (std::any_of(ok.begin(), ok.end(), [&](const std::string& n) { return casa_modelo(n, pm.nome) || (!pm.id.empty() && n == pm.id); }))
                ++disponiveis;
        lista.push_back({{"id", id}, {"nome", m.value("nome", vid)}, {"versao", m.value("versao", std::string())},
                         {"estado", m.value("estado", std::string())},
                         {"logo", std::string(SITE) + "/api/slicer/v1/modulos/logos/" + id},
                         {"modelos_disponiveis", disponiveis},
                         {"modo_motor", m.contains("politica_motor") ? m["politica_motor"].value("modo", std::string()) : std::string("OFICIAL")},
                         {"nota", m.contains("politica_motor") ? m["politica_motor"].value("nota_catalogo", std::string()) : std::string()},
                         {"complemento_instalado", atual.modulo_atualizado(id)}});
    }
    return {{"fabricantes", lista}, {"conta", !atual.visiveis().empty()}};
}

json modelos(const std::string& modulo_id)
{
    const json cat = FilunaAtualizacao::instancia().catalogo();
    json guarda;
    const json* m = modulo_do_catalogo(cat, modulo_id, guarda);
    if (m == nullptr)
        return {{"erro", "Fabricante fora do catálogo desta conta."}};
    const std::string vid = vendor_orca(modulo_id);
    const auto base = modelos_da_base(vid);
    if (base.empty())
        return {{"erro", "Perfis do fabricante não estão nesta versão do Filuna."}};
    const auto ok = homologados(*m);
    const auto fora = indisponiveis(*m);
    json lista = json::array();
    for (const auto& pm : base) {
        const bool disponivel = std::any_of(ok.begin(), ok.end(), [&](const std::string& n) { return casa_modelo(n, pm.nome) || (!pm.id.empty() && n == pm.id); }) &&
                                std::none_of(fora.begin(), fora.end(), [&](const std::string& n) { return n == pm.nome; });
        json bicos = json::array();
        for (const auto& v : pm.bicos)
            bicos.push_back(v);
        lista.push_back({{"id", pm.id}, {"nome", pm.nome}, {"familia", pm.familia}, {"foto", cover(vid, pm.id, pm.nome)},
                         {"bicos", bicos}, {"disponivel", disponivel},
                         {"mensagem", disponivel ? std::string() : mensagem_indisponivel(*m)}});
    }
    // modelos citados no catálogo que não têm perfil nesta base (ex.: lista oficial maior que a do Orca)
    for (const auto& n : fora)
        if (std::none_of(base.begin(), base.end(), [&](const ModeloBase& pm) { return pm.nome == n; }))
            lista.push_back({{"id", n}, {"nome", n}, {"foto", std::string()}, {"disponivel", false}, {"mensagem", mensagem_indisponivel(*m)}});
    std::sort(lista.begin(), lista.end(), [](const json& a, const json& b) {
        if (a["disponivel"] != b["disponivel"])
            return a["disponivel"].get<bool>();
        return a["nome"].get<std::string>() < b["nome"].get<std::string>();
    });
    return {{"fabricante", modulo_id}, {"nome", m->value("nome", vid)}, {"modelos", lista}};
}

json complemento(const std::string& modulo_id)
{
    FilunaAtualizacao& atual = FilunaAtualizacao::instancia();
    const json cat = atual.catalogo();
    json guarda;
    const json* m = modulo_do_catalogo(cat, modulo_id, guarda);
    if (m == nullptr)
        return {{"erro", "Fabricante fora do catálogo desta conta."}};
    std::uint64_t bytes = 0;
    json itens = json::array();
    for (const auto& c : m->value("componentes", json::array())) {
        bytes += c.value("bytes", std::uint64_t(0));
        itens.push_back(c.value("tipo", std::string()));
    }
    return {{"fabricante", modulo_id}, {"versao", m->value("versao", std::string())}, {"bytes", bytes}, {"itens", itens},
            {"instalado", atual.modulo_atualizado(modulo_id)}, {"instalada_versao", atual.versao_instalada(modulo_id)},
            {"libera", json::array({"rede", "impressão", "controle", "câmera", "cores"})}};
}

// Perfil de máquina do modelo; se o fabricante/modelo ainda não estiver instalado neste computador, instala como o
// assistente do Orca faz (PresetBundle::apply_vendor_config) e procura de novo.
const Preset* preset_instalando(const std::string& vid, const std::string& modelo)
{
    AppConfig* cfg = wxGetApp().app_config;
    PresetBundle* bundle = wxGetApp().preset_bundle;
    const Preset* p = preset_do_modelo(vid, modelo);
    if (p == nullptr && cfg != nullptr && bundle != nullptr) {
        // Fabricante ainda não instalado neste computador (bundle->printers só tem os instalados):
        // instala o modelo como o assistente do Orca faz (PresetBundle::apply_vendor_config) e procura de novo.
        std::set<std::string> bicos;
        for (const auto& pm : modelos_da_base(vid))
            if (pm.nome == modelo)
                bicos.insert(pm.bicos.begin(), pm.bicos.end());
        if (bicos.empty())
            bicos.insert("0.4");
        const std::map<std::string, std::map<std::string, std::set<std::string>>> novos{{vid, {{modelo, bicos}}}};
        try {
            if (bundle->apply_vendor_config(novos, std::map<std::string, std::string>(), cfg, false)) {
                cfg->save();
                wxGetApp().load_current_presets();
            }
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Filuna: instalar perfis de " << vid << ": " << e.what();
        }
        p = preset_do_modelo(vid, modelo);
    }
    return p;
}

json preparar(const std::string& modulo_id, const std::string& modelo)
{
    const std::string vid = vendor_orca(modulo_id);
    AppConfig* cfg = wxGetApp().app_config;
    PresetBundle* bundle = wxGetApp().preset_bundle;
    const Preset* p = preset_instalando(vid, modelo);
    if (p == nullptr)
        return {{"erro", "Não há perfil de máquina para " + modelo + " nesta versão do Filuna."}};
    const std::string model_id = p->config.has("printer_model") ? p->config.opt_string("printer_model") : std::string();
    const std::string variant  = p->config.has("printer_variant") ? p->config.opt_string("printer_variant") : std::string();
    // Instala o modelo (como no assistente do Orca) e seleciona o perfil em Preparar.
    if (cfg != nullptr && !model_id.empty()) {
        cfg->set_variant(vid, model_id, variant, true);
        bundle->load_installed_printers(*cfg);
    }
    const std::string agente = agente_da_marca(modulo_id, modelo);
    if (cfg != nullptr)
        cfg->set("filuna_agente_" + vid, agente); // GUI_App::resolve_printer_agent_id respeita este vínculo
    Tab* tab = wxGetApp().get_tab(Preset::TYPE_PRINTER);
    if (tab != nullptr)
        tab->select_preset(p->name);
    // Trocar para impressora que não é Bambu faz o Orca inserir a aba dele ("Printer panel", página de exemplo
    // com imagem) logo depois da aba Dispositivo do Filuna, e a tela passava a mostrar essa página no meio do
    // assistente (beta 14/11: os cliques do dono caíram na imagem). Volta para o Dispositivo do Filuna.
    wxGetApp().CallAfter([] { FilunaPanel::open_tab("filuna.dispositivo", std::string()); });
    // Agente da marca ao vivo (GUI_App::switch_printer_agent lê o vínculo acima) + busca na LAN
    if (!NetworkAgentFactory::is_printer_agent_registered(agente))
        return {{"erro", "Agente de rede da marca não está disponível (" + agente + ")."}, {"perfil", p->name}};
    wxGetApp().switch_printer_agent();
    NetworkAgent* net = wxGetApp().getAgent();
    std::string ativo;
    if (net != nullptr) {
        if (auto ag = net->get_printer_agent())
            ativo = ag->get_agent_info().id;
        net->start_discovery(true, true);
    }
    if (agente_moonraker(agente))
        procurar_moonraker(agente, modelo, model_id);
    return {{"perfil", p->name}, {"agente", agente}, {"agente_ativo", ativo}, {"modelo_id", model_id}};
}

json descobertas()
{
    json lista = json::array();
    DeviceManager* dev = wxGetApp().getDeviceManager();
    if (dev == nullptr)
        return {{"impressoras", lista}};
    for (auto& [id, o] : dev->get_local_machinelist())
        if (o != nullptr)
            lista.push_back({{"dev_id", o->get_dev_id()}, {"nome", o->get_dev_name()}, {"ip", o->get_dev_ip()}, {"modelo", o->printer_type},
                             {"modelo_nome", std::string(o->get_printer_type_display_str().ToUTF8().data())},
                             {"livre", o->bind_state != "occupied"}, {"ligada", o->is_online()},
                             {"precisa_codigo", o->get_access_code().empty() && !o->has_access_right()}});
    json busca;
    {
        std::lock_guard<std::mutex> l(busca_mutex());
        busca = busca_info();
    }
    return {{"impressoras", lista}, {"agente", dev->get_current_printer_agent_id()}, {"busca_klipper", busca}};
}

// O programa só trata como "acessível" a impressora de rede que tem um código gravado (herança do modo LAN da
// Bambu). Marcas que conectam sem código precisam de um valor: a Centauri Carbon 2 usa 123456 de fábrica; no
// Modo LAN da Anycubic e no SDCP da Elegoo a senha não é usada (as credenciais vêm do aperto de mão).
void garantir_codigo(MachineObject* o)
{
    if (o == nullptr || !o->get_access_code().empty())
        return;
    const std::string ag = o->printer_agent_id;
    if (ag == "filuna_elegoo_cc2")
        o->set_access_code("123456", false);
    else if ((ag.rfind("filuna_", 0) == 0 && ag != "filuna_bambu_lan") || agente_moonraker(ag))
        // Regra única para as marcas do Filuna: o DeviceManager só deixa selecionar impressora local com
        // código preenchido. Onde a marca não usa código (credenciais vêm do aperto de mão), vai um valor
        // neutro; onde usa, o agente da marca devolve o erro de acesso com a própria mensagem.
        o->set_access_code("lan", false);
}

// Torna a marca desta impressora a ativa do programa (agente de rede e perfil do modelo em Preparar). Sem isso,
// conectar uma Bambu depois de procurar uma Snapmaker falhava: a impressora não estava na lista do agente atual.
void ativar_marca_de(MachineObject* o)
{
    // perfil de máquina do modelo em Preparar (se o fabricante for conhecido pelo agente)
    const std::string agente = o->printer_agent_id;
    for (const auto& [mid, ag] : AGENTE)
        if (ag == agente || (agente == "filuna_elegoo_sdcp" && mid == "elegoo")) {
            const std::string vid = vendor_orca(mid);
            // a impressora diz "Centauri Carbon 2"; o perfil chama "Elegoo Centauri Carbon 2"
            const Preset* p = preset_do_modelo(vid, o->printer_type);
            if (p == nullptr)
                p = preset_do_modelo(vid, vid + " " + o->printer_type);
            if (p == nullptr) {
                // Bambu: printer_type é o código (N7 = P2S); o nome do modelo vem da tabela da marca. Se o modelo
                // ainda não estiver instalado (o cliente só tinha a A1), instala o perfil dele agora.
                const wxString nome = o->get_printer_type_display_str();
                p = preset_instalando(vid, std::string(nome.ToUTF8().data()));
            }
            // O programa mantém UMA impressora ativa: abrir o cartão dela traz de volta o agente de rede e o
            // perfil da marca dela (antes ficava no da última marca usada e ela aparecia desconectada).
            if (AppConfig* cfg = wxGetApp().app_config)
                cfg->set("filuna_agente_" + vid, agente);
            if (p != nullptr)
                if (Tab* tab = wxGetApp().get_tab(Preset::TYPE_PRINTER))
                    tab->select_preset(p->name);
            wxGetApp().switch_printer_agent();
            break;
        }
}

json conectar(const std::string& dev_id, const std::string& codigo)
{
    DeviceManager* dev = wxGetApp().getDeviceManager();
    if (dev == nullptr)
        return {{"erro", "Gerenciador de dispositivos indisponível."}};
    MachineObject* o = dev->get_local_machine(dev_id);
    if (o == nullptr)
        o = dev->get_my_machine(dev_id);
    if (o == nullptr)
        return {{"erro", "Impressora não está mais na rede."}};
    if (o->printer_agent_id != dev->get_current_printer_agent_id())
        ativar_marca_de(o);
    if (!codigo.empty())
        o->set_access_code(codigo, false);
    else
        garantir_codigo(o);
    if (!dev->set_selected_machine(dev_id))
        return {{"erro", "Não foi possível conectar. Confira o código de acesso. " "Na impressora: Configurações › Rede (WLAN), ligue o Modo LAN e o Modo desenvolvedor e copie o Código de acesso."}, {"precisa_codigo", true}};
    dev->set_local_selected_machine(dev_id);
    if (NetworkAgent* net = wxGetApp().getAgent())
        net->set_user_selected_machine(dev_id);
    return {{"ok", true}, {"impressora", card(o)}};
}

json conectar_ip(const std::string& modulo_id, const std::string& modelo, const std::string& ip, const std::string& codigo,
                 const std::string& nome)
{
    DeviceManager* dev = wxGetApp().getDeviceManager();
    if (dev == nullptr)
        return {{"erro", "Gerenciador de dispositivos indisponível."}};
    if (ip.empty())
        return {{"erro", "Informe o endereço IP da impressora."}};
    const std::string vid = vendor_orca(modulo_id);
    const Preset* p = preset_do_modelo(vid, modelo);
    BBLocalMachine m;
    m.dev_name         = nome.empty() ? modelo + " (" + ip + ")" : nome;
    m.dev_ip           = ip;
    m.dev_id           = "ip-" + ip;
    m.printer_type     = (p != nullptr && p->config.has("printer_model")) ? p->config.opt_string("printer_model") : modelo;
    m.printer_agent_id = agente_da_marca(modulo_id, modelo);
    m.access_code      = codigo;
    MachineObject* o = dev->insert_local_device(m, "lan", "free", "", codigo);
    if (o == nullptr)
        return {{"erro", "Não foi possível registrar o endereço."}};
    return conectar(o->get_dev_id(), codigo);
}

json ponte_instrucoes(const std::string& modulo_id)
{
    try {
        boost::filesystem::ifstream in(boost::filesystem::path(resources_dir()) / "filuna" / "ponte-lan.json", std::ios::binary);
        if (in) {
            json todos = json::parse(in);
            if (todos.contains(modulo_id))
                return todos[modulo_id];
            if (todos.contains("padrao"))
                return todos["padrao"];
        }
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "Filuna: ponte-lan.json: " << e.what();
    }
    return {{"titulo", "Ligar o modo LAN na impressora"},
            {"passos", json::array({"Na tela da impressora, abra Configurações › Rede.", "Ative o modo LAN (rede local).",
                                    "Anote o endereço IP e o código de acesso, se houver.", "Volte aqui e toque em Procurar de novo."})}};
}

json registrar(const std::string& nome, const std::string& modulo_id, const std::string& modelo, const std::string& dev_id)
{
    Filuna::FilunaDb& db = filuna_db();
    const std::string id = db.criar_impressora(nome, modulo_id, modelo, dev_id, modulo_id);
    if (AppConfig* cfg = wxGetApp().app_config)
        cfg->set(chave_vinculo(dev_id), id);
    filuna_sync_avisar(); // cadastro sobe para a conta (servidor → app Android) em segundos
    return {{"id", id}, {"nome", nome}};
}

json estado()
{
    json lista = json::array();
    DeviceManager* dev = wxGetApp().getDeviceManager();
    if (dev == nullptr)
        return {{"impressoras", lista}};
    AppConfig* cfg = wxGetApp().app_config;
    std::map<std::string, MachineObject*> todas = dev->get_my_machine_list();
    for (auto& [id, o] : dev->get_local_machinelist())
        todas.emplace(id, o);
    MachineObject* sel = dev->get_selected_machine();
    for (auto& [id, o] : todas) {
        if (o == nullptr)
            continue;
        json c = card(o);
        c["selecionada"] = (sel != nullptr && sel->get_dev_id() == o->get_dev_id());
        c["cadastro_id"] = cfg != nullptr ? cfg->get(chave_vinculo(o->get_dev_id())) : std::string();
        lista.push_back(c);
    }
    // cadastradas no banco local que não estão na rede agora: aparecem desligadas
    for (const auto& imp : filuna_db().impressoras()) {
        const bool vista = std::any_of(lista.begin(), lista.end(), [&](const json& c) { return c.value("cadastro_id", std::string()) == imp.id; });
        if (!vista)
            lista.push_back({{"dev_id", std::string()}, {"cadastro_id", imp.id}, {"nome", imp.nome}, {"modelo_nome", imp.modelo},
                             {"fabricante", imp.fabricante}, {"ligada", false}, {"estado", "desligada"}, {"cores", json::array()},
                             {"camera", false}, {"selecionada", false}});
    }
    return {{"impressoras", lista}, {"agente", dev->get_current_printer_agent_id()}};
}

json selecionar(const std::string& dev_id)
{
    DeviceManager* dev = wxGetApp().getDeviceManager();
    if (dev == nullptr)
        return {{"erro", "Gerenciador de dispositivos indisponível."}};
    MachineObject* o = dev->get_my_machine(dev_id);
    if (o == nullptr)
        o = dev->get_local_machine(dev_id);
    if (o == nullptr)
        return {{"erro", "Impressora não encontrada."}};
    ativar_marca_de(o);
    garantir_codigo(o);
    // A Bambu só conecta com o código de acesso da tela dela: sem ele, pedir o código em vez de falhar calado.
    if (o->printer_agent_id == "filuna_bambu_lan" && o->get_access_code().empty() && !o->has_access_right())
        return {{"erro", "Esta impressora pede o código de acesso. " "Na impressora: Configurações › Rede (WLAN), ligue o Modo LAN e o Modo desenvolvedor e copie o Código de acesso."}, {"precisa_codigo", true}};
    if (!dev->set_selected_machine(dev_id))
        return {{"erro", "Não foi possível conectar. Confira o código de acesso. " "Na impressora: Configurações › Rede (WLAN), ligue o Modo LAN e o Modo desenvolvedor e copie o Código de acesso."}, {"precisa_codigo", true}};
    dev->set_local_selected_machine(dev_id);
    if (NetworkAgent* net = wxGetApp().getAgent())
        net->set_user_selected_machine(dev_id);
    return {{"ok", true}, {"impressora", card(o)}};
}

// Pausar, retomar ou parar a impressão da impressora ATIVA (a do cartão aberto).
json comando(const std::string& dev_id, const std::string& cmd)
{
    DeviceManager* dev = wxGetApp().getDeviceManager();
    if (dev == nullptr)
        return {{"erro", "Gerenciador de dispositivos indisponível."}};
    MachineObject* o = dev->get_selected_machine();
    if (o == nullptr || (!dev_id.empty() && o->get_dev_id() != dev_id))
        return {{"erro", "Abra o cartão da impressora antes de comandar."}};
    int r = -1;
    if (cmd == "pausar")
        r = o->command_task_pause();
    else if (cmd == "retomar")
        r = o->command_task_resume();
    else if (cmd == "cancelar" || cmd == "parar")
        r = o->command_task_abort();
    else
        return {{"erro", "Comando desconhecido."}};
    if (r != 0)
        return {{"erro", "A impressora não aceitou o comando (" + std::to_string(r) + ")."}};
    return {{"ok", true}};
}

// Controle direto da impressora ATIVA: temperatura, eixos, luz, ventiladores, velocidade, carregar/descarregar.
// O pedido vai como {"filuna": {"command": tipo, ...}} para o motor de rede da marca, que traduz para o protocolo dela.
json controle(const std::string& dev_id, const json& d)
{
    DeviceManager* dev = wxGetApp().getDeviceManager();
    if (dev == nullptr)
        return {{"erro", "Gerenciador de dispositivos indisponível."}};
    MachineObject* o = dev->get_selected_machine();
    if (o == nullptr || (!dev_id.empty() && o->get_dev_id() != dev_id))
        return {{"erro", "Abra o cartão da impressora antes de comandar."}};
    const std::string tipo = d.value("tipo", std::string());
    auto ctl = CONTROLES.find(o->printer_agent_id);
    const bool padrao = ctl == CONTROLES.end();
    if (!padrao && std::find(ctl->second.begin(), ctl->second.end(), tipo) == ctl->second.end())
        return {{"erro", "Este controle ainda não existe para esta impressora."}};
    auto limitar = [&](const char* k, double lo, double hi) {
        double v = d.contains(k) && d[k].is_number() ? d[k].get<double>() : 0.0;
        return std::max(lo, std::min(hi, v));
    };
    json f = {{"command", tipo}};
    if (tipo == "bico")
        f["valor"] = int(limitar("valor", 0, 320));
    else if (tipo == "mesa")
        f["valor"] = int(limitar("valor", 0, 120));
    else if (tipo == "ventilador" || tipo == "ventilador_aux" || tipo == "ventilador_caixa")
        f["valor"] = int(limitar("valor", 0, 100));
    else if (tipo == "velocidade")
        f["valor"] = int(limitar("valor", 0, 3));
    else if (tipo == "luz")
        f["ligar"] = d.value("ligar", false);
    else if (tipo == "home") {
        std::string e = minusculas(d.value("eixos", std::string("xyz")));
        if (e.empty() || e.find_first_not_of("xyz") != std::string::npos)
            e = "xyz";
        f["eixos"] = e;
    } else if (tipo == "mover") {
        const std::string e = minusculas(d.value("eixo", std::string()));
        if (e != "x" && e != "y" && e != "z")
            return {{"erro", "Eixo inválido."}};
        f["eixo"]      = e;
        f["distancia"] = limitar("distancia", -100, 100);
    } else if (tipo == "carregar" || tipo == "descarregar") {
        f["unidade"] = int(limitar("unidade", 0, 15));
        f["slot"]    = int(limitar("slot", 0, 15));
    } else if (tipo == "secar") {
        // secagem do filamento na unidade de cores (ACE Pro e equivalentes)
        f["ligar"]       = d.value("ligar", false);
        f["unidade"]     = int(limitar("unidade", 0, 15));
        f["temperatura"] = int(d.contains("temperatura") ? limitar("temperatura", 35, 65) : 45);
        f["minutos"]     = int(d.contains("minutos") ? limitar("minutos", 30, 1440) : 240);
    }
    if (o->is_in_printing() == false) {
        auto so = SO_IMPRIMINDO.find(o->printer_agent_id);
        if (so != SO_IMPRIMINDO.end() && std::find(so->second.begin(), so->second.end(), tipo) != so->second.end())
            return {{"erro", "Esta impressora só aceita este ajuste durante uma impressão."}};
    }
    int r = 0;
    if (padrao) {
        // comandos padrão do programa (mesmos do painel nativo)
        if (tipo == "bico")
            r = o->command_set_nozzle(f.value("valor", 0));
        else if (tipo == "mesa")
            r = o->command_set_bed(f.value("valor", 0));
        else if (tipo == "home")
            r = o->command_go_home();
        else if (tipo == "mover")
            r = o->command_axis_control(boost::to_upper_copy(f.value("eixo", std::string("x"))), 1.0, f.value("distancia", 0.0),
                                        f.value("eixo", std::string()) == "z" ? 900 : 3000);
        else if (tipo == "luz")
{
            if (o->GetLamp() == nullptr)
                r = -1;
            else
                o->GetLamp()->CtrlSetChamberLight(f.value("ligar", false) ? DevLamp::LIGHT_EFFECT_ON : DevLamp::LIGHT_EFFECT_OFF);
        }
        else if (tipo == "ventilador" || tipo == "ventilador_aux" || tipo == "ventilador_caixa")
            r = o->GetFan() != nullptr ? o->GetFan()->command_control_fan(tipo == "ventilador" ? 1 : tipo == "ventilador_aux" ? 2 : 3,
                                                                          f.value("valor", 0) * 255 / 100) : -1;
        else if (tipo == "velocidade")
            r = o->command_set_printing_speed(DevPrintingSpeedLevel(std::max(1, std::min(4, f.value("valor", 2) + 1))));
        else if (tipo == "carregar" || tipo == "descarregar")
            r = o->command_ams_change_filament(tipo == "carregar", std::to_string(f.value("unidade", 0)), std::to_string(f.value("slot", 0)));
        else
            return {{"erro", "Este controle ainda não existe para esta impressora."}};
    } else if (agente_moonraker(o->printer_agent_id)) {
        const std::string g = gcode_moonraker(f);
        if (g.empty())
            return {{"erro", "Este controle ainda não existe para esta impressora."}};
        r = o->publish_json(json{{"print", {{"command", "gcode_line"}, {"param", g}, {"sequence_id", "0"}}}}, 1);
    } else
        r = o->publish_json(json{{"filuna", f}}, 1);
    if (r != 0)
        return {{"erro", "A impressora não aceitou o comando (" + std::to_string(r) + ")."}};
    return {{"ok", true}};
}

void sincronizar_estado(const json& estado, const std::string& token);

// Vigia das impressoras, ligado uma vez e independente da aba aberta: a cada 10 s lê o estado, avisa quando
// uma impressão termina (aviso dentro do programa e do Windows) e manda o estado para a conta a cada 30 s —
// é daí que o site, o app e os alertas (WhatsApp/Telegram) ficam sabendo. Antes isso só rodava com a tela
// Dispositivo aberta, então fora dela ninguém era avisado. Vale para qualquer marca.
json troca_vista(const std::string& dev_id)
{
    trocados().erase(dev_id);
    return json{{"ok", true}};
}

void monitor_iniciar()
{
    static wxTimer* vigia = nullptr;
    if (vigia != nullptr)
        return;
    vigia = new wxTimer();
    vigia->Bind(wxEVT_TIMER, [](wxTimerEvent&) {
        static std::map<std::string, std::string> antes;
        static int                                volta = 0;
        json e;
        try {
            e = estado();
        } catch (const std::exception&) {
            return;
        }
        for (const auto& i : e.value("impressoras", json::array())) {
            const std::string dev = i.value("dev_id", std::string()), est = i.value("estado", std::string());
            if (dev.empty() || !i.value("selecionada", false))
                continue; // só a impressora ativa tem leitura ao vivo
            const std::string a = antes[dev];
            if ((a == "imprimindo" || a == "pausada") && (est == "concluida" || est == "livre")) {
                const wxString texto = wxString::FromUTF8("Impressão concluída na " + i.value("nome", std::string("impressora")) +
                                                          ". Retire a peça e limpe a mesa.");
                if (Plater* pl = wxGetApp().plater())
                    if (auto* nm = pl->get_notification_manager())
                        nm->push_notification(std::string(texto.ToUTF8().data()));
                wxNotificationMessage aviso(wxString::FromUTF8("Filuna Slicer"), texto, wxGetApp().mainframe);
                aviso.Show();
            }
            {
                // Troca de filamento (qualquer marca): a entrada passou a ter um carretel, ou mudou de material/cor,
                // depois da primeira leitura. Só vale quando a leitura nova se repete (20 s), para não avisar no
                // meio de um carregamento. Avisa e deixa marcado para a tela perguntar do estoque.
                static std::map<std::string, std::map<std::string, std::string>> fil_antes, fil_cand;
                std::map<std::string, std::string> agora_f;
                for (const auto& cr : i.value("cores", json::array())) {
                    if (!cr.is_object())
                        continue;
                    const auto txt = [&cr](const char* k) {
                        return cr.contains(k) ? (cr[k].is_string() ? cr[k].get<std::string>() : cr[k].dump()) : std::string();
                    };
                    agora_f[txt("unidade") + ":" + txt("slot")] = cr.value("vazio", false) ? std::string() : txt("material") + "|" + txt("cor");
                }
                const bool primeira = fil_antes.find(dev) == fil_antes.end();
                auto&      ant      = fil_antes[dev];
                auto&      cand     = fil_cand[dev];
                std::string novas;
                for (const auto& [k, sig] : agora_f) {
                    if (primeira) {
                        ant[k] = sig;
                        continue;
                    }
                    if (ant[k] == sig) {
                        cand.erase(k);
                        continue;
                    }
                    if (cand[k] != sig) { // primeira vez que aparece assim: espera a próxima leitura confirmar
                        cand[k] = sig;
                        continue;
                    }
                    ant[k] = sig;
                    cand.erase(k);
                    if (sig.empty())
                        continue; // carretel retirado: nada a cadastrar
                    trocados()[dev].insert(k);
                    const auto pt = k.find(':');
                    int        n  = 0;
                    try { n = std::stoi(k.substr(pt + 1)) + 1; } catch (...) {}
                    novas += (novas.empty() ? "" : ", ") + std::to_string(n) + " (" + sig.substr(0, sig.find('|')) + ")";
                }
                if (!novas.empty()) {
                    const wxString texto = wxString::FromUTF8("Filamento trocado na " + i.value("nome", std::string("impressora")) + ": entrada " + novas +
                                                              ". Abra Dispositivo para cadastrar no estoque.");
                    if (Plater* pl = wxGetApp().plater())
                        if (auto* nm = pl->get_notification_manager())
                            nm->push_notification(std::string(texto.ToUTF8().data()));
                    wxNotificationMessage aviso(wxString::FromUTF8("Filuna Slicer — filamento trocado"), texto, wxGetApp().mainframe);
                    aviso.Show();
                }
            }
            const std::string err = i.value("erro", std::string());
            static std::map<std::string, std::string> erro_antes;
            if (!err.empty() && erro_antes[dev] != err) { // erro novo da impressora (filamento rompido, falha…)
                const wxString texto = wxString::FromUTF8(i.value("nome", std::string("Impressora")) + ": " + err + ".");
                if (Plater* pl = wxGetApp().plater())
                    if (auto* nm = pl->get_notification_manager())
                        nm->push_notification(std::string(texto.ToUTF8().data()));
                wxNotificationMessage aviso(wxString::FromUTF8("Filuna Slicer — impressão com problema"), texto, wxGetApp().mainframe, wxICON_WARNING);
                aviso.Show();
            }
            erro_antes[dev] = err;
            antes[dev] = est;
        }
        if (++volta % 3 == 0)
            if (AppConfig* cfg = wxGetApp().app_config)
                sincronizar_estado(e, cfg->get("filuna_token"));
    });
    vigia->Start(10000);
}

void sincronizar_estado(const json& estado, const std::string& token)
{
    if (token.empty())
        return;
    static std::mutex trava;
    static std::chrono::steady_clock::time_point ultima;
    {
        std::lock_guard<std::mutex> g(trava);
        const auto agora = std::chrono::steady_clock::now();
        if (agora - ultima < std::chrono::seconds(25))
            return;
        ultima = agora;
    }
    json corpo = json::array();
    for (const auto& c : estado.value("impressoras", json::array()))
        corpo.push_back({{"cadastro_id", c.value("cadastro_id", std::string())}, {"dev_id", c.value("dev_id", std::string())},
                         {"nome", c.value("nome", std::string())}, {"modelo", c.value("modelo_nome", std::string())},
                         {"estado", c.value("estado", std::string())}, {"ligada", c.value("ligada", false)},
                         {"erro", c.value("erro", std::string())},
                         {"progresso", c.value("progresso", 0)}, {"restante_s", c.value("restante_s", 0)},
                         {"cores", c.value("cores", json::array())}, {"camera", c.value("camera", false)},
                         {"bico", c.value("bico", json::object())}, {"mesa", c.value("mesa", json::object())}});
    std::thread([corpo, token] {
        Http http = Http::post(std::string(SITE) + "/api/slicer/v1/dispositivos/estado");
        http.header("Authorization", "Bearer " + token).header("Content-Type", "application/json")
            .set_post_body(json({{"impressoras", corpo}}).dump())
            .timeout_connect(5).timeout_max(15)
            .on_error([](std::string, std::string erro, unsigned) { BOOST_LOG_TRIVIAL(debug) << "Filuna: dispositivos/estado: " << erro; })
            .perform_sync();
    }).detach();
}

}}} // namespace Slic3r::GUI::FilunaDispositivos
