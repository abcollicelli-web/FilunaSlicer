#include <boost/asio.hpp> // no Windows o boost precisa vir antes dos cabeçalhos do wxWidgets
#include "FilunaDispositivos.hpp"
#include "FilunaIdioma.hpp"
#include "FilunaCasa.hpp"
#include "FilunaCasaAvisos.hpp"
#include "FilunaAoVivo.hpp"

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
#include "DeviceCore/DevHMS.h"
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
#include "slic3r/Utils/FilunaFlashforgeAgent.hpp"

#include <boost/algorithm/string.hpp>
#include <wx/notifmsg.h>
#include <wx/timer.h>
#include "NotificationManager.hpp"
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
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

// Raio X do equipamento: tudo o que a impressora informa além do básico, para o histórico e o diagnóstico.
// Junta o que o agente da marca mediu (Filuna::extra: potência, rotação, placas no Klipper) com o que o relatório
// já traz no MachineObject (câmara, alvo da mesa, Wi-Fi, coolers, caixas de filamento, HMS, firmware).
json extra_de(MachineObject* o)
{
    json ex = json::parse(Filuna::extra(o->get_dev_id()), nullptr, false);
    if (!ex.is_object())
        ex = json::object();
    if (DevBed* bed = o->GetBed())
        ex["mesa_alvo"] = bed->GetBedTempTarget();
    if (o->chamber_temp > -100.0f && o->chamber_temp != 0.0f)
        ex["camara"] = std::round(o->chamber_temp * 10.0f) / 10.0f;
    if (!o->wifi_signal.empty())
        ex["wifi"] = o->wifi_signal;
    const std::string& ag = o->printer_agent_id;
    if ((ag == "filuna_bambu_lan" || ag == "bbl") && o->GetFan() != nullptr) {
        DevFan* f = o->GetFan();
        ex["coolers"] = {{"peca", f->GetCoolingFanSpeed()}, {"hotend", f->GetHeatBreakFanSpeed()}, {"auxiliar", f->GetBigFan1Speed()},
                         {"camara", f->GetBigFan2Speed()}};
    }
    if (auto fila = o->GetFilaSystem()) {
        json caixas = json::array();
        for (auto& [ams_id, ams] : fila->GetAmsList()) {
            if (ams == nullptr)
                continue;
            json cx = {{"unidade", ams_id}};
            if (ams->GetCurrentTemperature() > -100.0f && ams->GetCurrentTemperature() < 200.0f)
                cx["temperatura"] = std::round(ams->GetCurrentTemperature() * 10.0f) / 10.0f;
            if (ams->GetHumidityPercent() >= 0)
                cx["umidade"] = ams->GetHumidityPercent();
            else if (o->is_support_ams_humidity)
                cx["umidade_nivel"] = ams->GetHumidityLevel(); // 1 (seco) a 5 (úmido) no AMS da Bambu
            if (cx.size() > 1)
                caixas.push_back(cx);
        }
        if (!caixas.empty())
            ex["caixas"] = caixas;
    }
    if (DevHMS* hms = o->GetHMS()) {
        json lista = json::array();
        for (const auto& it : hms->GetHMSItems())
            lista.push_back({{"codigo", it.get_long_error_code()}, {"nivel", int(it.get_level())}});
        if (!lista.empty())
            ex["hms"] = lista;
    }
    const std::string fw = o->get_ota_version();
    if (!fw.empty())
        ex["firmware"] = fw;
    return ex;
}

// Câmera ao vivo que o agente da marca descobriu (Filuna::set_camera_url). Vídeo em HTTP-FLV (Anycubic e outras): a
// tela lê pelo repasse local, que acrescenta a permissão que a impressora não manda; câmera pelo repasse local também
// para as demais (permite à tela tirar uma foto para a IA conferir a mesa e mostrar o quadro nos cartões).
void camera_do_agente(json& c, const std::string& dev_id)
{
    const std::string cam = Filuna::camera_url(dev_id);
    if (cam.empty())
        return;
    c["camera_url"] = cam;
    std::string h, pt, cm;
    const int   porta = cam.size() > 4 && Filuna::CameraRelay::destino_valido(cam, h, pt, cm) ? Filuna::CameraRelay::porta() : 0;
    if (porta <= 0)
        return;
    const std::string repasse = "http://127.0.0.1:" + std::to_string(porta) + "/cam?u=" + Http::url_encode(cam);
    c["camera_repasse"]       = repasse;
    if (cam.find("/flv") != std::string::npos)
        c["camera_flv"] = repasse;
}

json card(MachineObject* o)
{
    json c = {{"dev_id", o->get_dev_id()}, {"nome", o->get_dev_name()}, {"ip", o->get_dev_ip()}, {"modelo", o->printer_type},
              {"modelo_nome", std::string(o->get_printer_type_display_str().ToUTF8().data())},
              {"ligada", o->is_online()},      {"estado", estado_legivel(o)},
              {"conexao", o->dev_connection_type}, {"agente", o->printer_agent_id},
              {"progresso", o->mc_print_percent}, {"restante_s", o->mc_left_time}, {"etapa", o->stage_curr},
              {"camera", o->has_ipcam},          {"cores", cores_de(o)},
              {"sinc_filamentos", true}}; // a tela pode pedir "dispositivos.filamentos_projeto"
    if (DevBed* bed = o->GetBed())
        c["mesa"] = {{"atual", bed->GetBedTemp()}};
    if (DevExtderSystem* ext = o->GetExtderSystem()) {
        c["bico"] = {{"atual", ext->GetNozzleTempCurrent(0)}, {"alvo", ext->GetNozzleTempTarget(0)}};
        // Raio X da Impressora: qual filamento estava passando quando deu o erro
        if (const auto atual = ext->GetCurrentExtder(); atual && !atual->GetSlotNow().slot_id.empty())
            c["slot_ativo"] = {{"unidade", atual->GetSlotNow().ams_id}, {"slot", atual->GetSlotNow().slot_id}};
    }
    // Raio X da Impressora: em que camada e com que código o erro aconteceu
    c["camada"]  = o->curr_layer;
    c["camadas"] = o->total_layers;
    c["peca"]    = o->subtask_name; // Filuna Home: o aviso diz qual peça terminou
    if (o->print_error != 0 && unsigned(o->print_error) != 0x0300400Cu && unsigned(o->print_error) != 0x0500400Eu)
        c["codigo_erro"] = o->get_print_error_str();
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
    camera_do_agente(c, o->get_dev_id());
    std::string err = Filuna::erro(o->get_dev_id());
    // Bambu: o erro vem no próprio relatório (print_error), com o texto oficial da Bambu quando conhecido
    // Cancelada pelo cliente (0300-400C): não é erro da impressora, nem para o site nem para o Raio X.
    const bool cancelada = unsigned(o->print_error) == 0x0300400Cu || unsigned(o->print_error) == 0x0500400Eu;
    if (err.empty() && o->print_error != 0 && !cancelada && o->printer_agent_id == "filuna_bambu_lan") {
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
    c["extra"] = extra_de(o);
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
            for (const auto& ip : Filuna::portas_abertas(ips, porta, std::chrono::milliseconds(900))) {
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

// Mensagem de falha de conexão por marca. O texto da Bambu (Modo LAN, Modo desenvolvedor e Código de acesso) aparecia
// para todas as marcas e fazia o cliente de uma Flashforge procurar na impressora um modo que ela não tem.
static std::string falha_conexao(const MachineObject* o)
{
    const std::string ag = o != nullptr ? o->printer_agent_id : std::string();
    if (ag == "filuna_bambu_lan")
        return "Não foi possível conectar. Confira o código de acesso. Na impressora: Configurações › Rede (WLAN), ligue o Modo LAN e o "
               "Modo desenvolvedor e copie o Código de acesso.";
    const std::string suporte = " Se continuar, o suporte Filuna pode testar a conexão por você (Suporte remoto, na tela Início).";
    if (ag == "filuna_flashforge_lan")
        return "Não foi possível conectar à Flashforge. Confira se ela está ligada e na mesma rede do computador, e o código de "
               "verificação: na impressora, Configurações › Rede, com o modo somente LAN ligado." + suporte;
    return "Não foi possível conectar à impressora. Confira se ela está ligada, na mesma rede do computador e com o modo LAN da "
           "marca ativo." + suporte;
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
    FilunaAoVivo::ativa(dev_id); // libera a conexão em segundo plano antes de o programa conectar
    if (!dev->set_selected_machine(dev_id))
        return {{"erro", falha_conexao(o)}, {"precisa_codigo", true}};
    dev->set_local_selected_machine(dev_id);
    if (NetworkAgent* net = wxGetApp().getAgent())
        net->set_user_selected_machine(dev_id);
    return {{"ok", true}, {"impressora", card(o)}};
}

// IP de impressora na rede de casa: quatro números de 0 a 255 sem zero à esquerda (a versão do firmware, como
// "01.09.00.00", parece um IP e passava) e numa faixa de rede local (192.168, 10, 172.16-31, 169.254).
bool ip_da_rede_local(const std::string& ip)
{
    std::vector<int> n;
    std::string parte;
    for (size_t i = 0; i <= ip.size(); ++i) {
        if (i == ip.size() || ip[i] == '.') {
            if (parte.empty() || parte.size() > 3 || (parte.size() > 1 && parte[0] == '0'))
                return false;
            const int v = std::stoi(parte);
            if (v > 255)
                return false;
            n.push_back(v);
            parte.clear();
        } else if (ip[i] >= '0' && ip[i] <= '9')
            parte += ip[i];
        else
            return false;
    }
    if (n.size() != 4)
        return false;
    return n[0] == 10 || (n[0] == 192 && n[1] == 168) || (n[0] == 172 && n[1] >= 16 && n[1] <= 31) || (n[0] == 169 && n[1] == 254);
}

std::string cadastro_de(const std::string& dev_id);

// Anycubic no Modo LAN: o número de série (cn) vem em GET http://<ip>:18910/info. Vazio se ela não respondeu.
std::string serie_anycubic(const std::string& ip)
{
    std::string serie;
    Http::get("http://" + ip + ":18910/info")
        .timeout_connect(4)
        .timeout_max(8)
        .on_complete([&serie](std::string corpo, unsigned) {
            const json j = json::parse(corpo, nullptr, false);
            if (j.is_object() && j.contains("cn") && j["cn"].is_string())
                serie = j["cn"].get<std::string>();
        })
        .perform_sync();
    return serie.size() >= 6 && serie.size() <= 64 ? serie : std::string();
}

json conectar_ip(const std::string& modulo_id, const std::string& modelo, const std::string& ip_digitado, const std::string& codigo,
                 const std::string& nome, const std::string& serie_informada)
{
    // "10.0.0.43." (ponto no fim, copiado da tela) ou espaços: limpa antes de conferir (Luara, 19/11)
    std::string ip = boost::trim_copy(ip_digitado);
    while (!ip.empty() && (ip.back() == '.' || ip.back() == ','))
        ip.pop_back();
    DeviceManager* dev = wxGetApp().getDeviceManager();
    if (dev == nullptr)
        return {{"erro", "Gerenciador de dispositivos indisponível."}};
    if (ip.empty())
        return {{"erro", "Informe o endereço IP da impressora."}};
    if (!ip_da_rede_local(ip))
        return {{"erro", "\"" + ip + "\" não é um endereço da sua rede (exemplo: 192.168.0.50). Ele aparece na tela da impressora, "
                          "em Configurações › Rede (WLAN) › IP — não confunda com a versão do firmware."}};
    const std::string vid = vendor_orca(modulo_id);
    const Preset* p = preset_do_modelo(vid, modelo);
    BBLocalMachine m;
    m.dev_name         = nome.empty() ? modelo + " (" + ip + ")" : nome;
    m.dev_ip           = ip;
    m.dev_id           = "ip-" + ip;
    if (modulo_id == "bambu") {
        // A Bambu precisa do número de série (é o tópico do MQTT); ele está no certificado da porta 8883.
        const std::string serie = Filuna::cn_do_certificado(ip, 8883);
        if (serie.size() < 10)
            return {{"erro", "Não achei uma Bambu Lab no IP " + ip + ". Confira o IP e se o Modo LAN (Somente LAN) está ligado na impressora."}};
        m.dev_id = serie;
    } else if (modulo_id == "anycubic") {
        // Pelo número de série, e não pelo IP: se o roteador trocar o IP, a mesma Kobra continua com o mesmo cadastro
        // (antes ficava "ip-10.0.0.151" e a Kobra X da Luara não respondia mais nesse endereço). Cadastro antigo pelo IP
        // do mesmo modelo, único: passa a ser o desta série.
        const std::string serie = serie_anycubic(ip);
        if (!serie.empty()) {
            m.dev_id = serie;
            AppConfig* cfg = wxGetApp().app_config;
            if (cfg != nullptr && cadastro_de(serie).empty()) {
                std::vector<std::string> mesmos;
                for (const auto& imp : filuna_db().impressoras())
                    if (boost::iequals(imp.fabricante, "anycubic") && boost::iequals(imp.modelo, modelo))
                        mesmos.push_back(imp.id);
                if (mesmos.size() == 1)
                    cfg->set(chave_vinculo(serie), mesmos.front());
            }
        }
    } else if (modulo_id == "flashforge") {
        // A API local da Flashforge só responde com o número de série + código de verificação. Antes o cadastro pelo
        // IP usava "ip-<ip>" no lugar da série e a impressora recusava sempre. A série vem da pergunta direta ao IP.
        std::string nome_ff, serie = serie_informada;
        if (serie.empty() && !FilunaFlashforgeAgent::perguntar(ip, nome_ff, serie))
            return {{"erro", "A Flashforge no IP " + ip + " não respondeu. Confira se ela está ligada e na mesma rede do computador "
                              "(Configurações › Rede, modo somente LAN), ou informe o número de série (Configurações › Sobre)."},
                    {"precisa_serie", true}};
        if (codigo.empty())
            return {{"erro", "Informe o código de verificação da Flashforge (Configurações › Rede, com o modo somente LAN ligado)."},
                    {"precisa_codigo", true}, {"serie", serie}};
        m.dev_id = serie;
        if (nome.empty() && !nome_ff.empty())
            m.dev_name = nome_ff + " (" + ip + ")";
    }
    m.printer_type     = (p != nullptr && p->config.has("printer_model")) ? [&] { const std::string n = p->config.opt_string("printer_model"); if (modulo_id == "bambu" && p->vendor != nullptr) for (const auto& vm : p->vendor->models) if (vm.name == n && !vm.model_id.empty()) return vm.model_id; return n; }() : modelo; // Bambu: o código do modelo (N7 = P2S), como a busca grava; o nome fazia a tela de envio dizer "incompatível"
    m.printer_agent_id = agente_da_marca(modulo_id, modelo);
    m.access_code      = codigo;
    MachineObject* o = dev->insert_local_device(m, "lan", "free", "", codigo);
    if (o == nullptr)
        return {{"erro", "Não foi possível registrar o endereço."}};
    return conectar(o->get_dev_id(), codigo);
}

// ---- Assistência remota: rede do cliente e fatiador ----

namespace {

// Portas das marcas no IP da impressora (e a página web, que quase todas têm).
const std::vector<std::pair<unsigned short, std::string>> PORTAS_MARCAS = {
    {8883, "Bambu Lab (MQTT)"},  {990, "Bambu Lab (arquivos)"},  {8898, "Flashforge (API local)"}, {8899, "Flashforge (G-code)"},
    {7125, "Klipper/Moonraker (Creality, Qidi, Snapmaker)"},      {3030, "Elegoo Centauri Carbon 2"},
    {18910, "Anycubic (Modo LAN)"}, {80, "página web da impressora"}};

// Portas abertas num IP, todas testadas ao mesmo tempo.
std::set<unsigned short> portas_do_ip(const std::string& ip, std::chrono::milliseconds espera)
{
    std::vector<std::thread> ts;
    std::mutex               mx;
    std::set<unsigned short> abertas;
    for (const auto& pm : PORTAS_MARCAS) {
        const unsigned short porta = pm.first;
        ts.emplace_back([&ip, &mx, &abertas, porta, espera] {
            if (!Filuna::portas_abertas({ip}, porta, espera).empty()) {
                std::lock_guard<std::mutex> l(mx);
                abertas.insert(porta);
            }
        });
    }
    for (auto& t : ts)
        t.join();
    return abertas;
}

// Endereço deste computador por onde o tráfego sai até o IP ("connect" em UDP não manda nada).
std::string saida_para(const std::string& ip)
{
    namespace net = boost::asio;
    boost::system::error_code ec;
    net::io_context           io;
    net::ip::udp::socket      s(io);
    s.open(net::ip::udp::v4(), ec);
    if (!ec)
        s.connect({net::ip::make_address_v4(ip, ec), 9}, ec);
    if (ec)
        return std::string();
    const auto ep = s.local_endpoint(ec);
    return ec ? std::string() : ep.address().to_string();
}

std::string lista_de(const std::set<unsigned short>& portas)
{
    std::string s;
    for (unsigned short p : portas)
        s += (s.empty() ? "" : ", ") + std::to_string(p);
    return s;
}

} // namespace

json rede_diagnostico(const std::string& ip, const std::string& modulo_id)
{
    const std::vector<std::string> redes = Filuna::redes_locais();
    json r = {{"redes_do_computador", redes}};
    if (ip.empty())
        return r;
    if (!ip_da_rede_local(ip))
        return {{"erro", "\"" + ip + "\" não é um endereço de rede local (exemplo: 192.168.0.50)."}, {"redes_do_computador", redes}};
    const std::string prefixo = ip.substr(0, ip.rfind('.') + 1);
    const bool        mesma   = std::find(redes.begin(), redes.end(), prefixo) != redes.end();
    r["ip"]                   = ip;
    r["saida_do_computador"]  = saida_para(ip);
    r["mesma_rede"]           = mesma;
    const std::set<unsigned short> abertas = portas_do_ip(ip, std::chrono::milliseconds(1500));
    json portas = json::array();
    for (const auto& [porta, uso] : PORTAS_MARCAS)
        portas.push_back({{"porta", porta}, {"uso", uso}, {"aberta", abertas.count(porta) > 0}});
    r["portas"] = portas;
    std::string nome, serie;
    const bool  ff = (modulo_id.empty() || modulo_id == "flashforge") && FilunaFlashforgeAgent::perguntar(ip, nome, serie, 2000);
    if (ff)
        r["flashforge"] = {{"nome", nome}, {"serie", serie}};
    if (abertas.count(8883))
        r["bambu_serie"] = Filuna::cn_do_certificado(ip, 8883);
    if (!mesma)
        r["resumo"] = "O computador não está na rede " + prefixo + "x (ele está em " + boost::algorithm::join(redes, ", ") +
                      "). Computador e impressora precisam estar no mesmo roteador, fora do Wi-Fi de visitante.";
    else if (abertas.empty() && !ff)
        r["resumo"] = "Nada respondeu em " + ip + ": a impressora está desligada, em repouso, com outro IP, ou com o modo LAN "
                      "desligado.";
    else if (ff)
        r["resumo"] = "A Flashforge respondeu (" + nome + ", série " + serie + "): dá para cadastrar pelo IP com o código de "
                      "verificação dela.";
    else
        r["resumo"] = "A impressora responde em " + ip + " (portas " + lista_de(abertas) + ").";
    return r;
}

json rede_procurar()
{
    const std::vector<std::string> redes = Filuna::redes_locais(), ips = Filuna::enderecos_locais();
    std::map<std::string, std::set<unsigned short>> portas;
    // uma porta por vez (todos os endereços ao mesmo tempo): poucos milhares de conexões abertas no máximo
    for (const auto& pm : PORTAS_MARCAS)
        for (const auto& ip : Filuna::portas_abertas(ips, pm.first, std::chrono::milliseconds(1200)))
            portas[ip].insert(pm.first);
    const auto ff = FilunaFlashforgeAgent::perguntar_rede(2500);
    std::set<std::string> todos;
    for (const auto& [ip, _] : portas)
        todos.insert(ip);
    for (const auto& [ip, _] : ff)
        todos.insert(ip);
    json lista = json::array();
    for (const auto& ip : todos) {
        json marcas = json::array(), abertas = json::array();
        if (portas.count(ip))
            for (unsigned short p : portas[ip]) {
                abertas.push_back(p);
                for (const auto& [porta, uso] : PORTAS_MARCAS)
                    if (porta == p && p != 80)
                        marcas.push_back(uso);
            }
        json a = {{"ip", ip}, {"portas", abertas}, {"marcas", marcas}};
        if (ff.count(ip))
            a["flashforge"] = {{"nome", ff.at(ip).first}, {"serie", ff.at(ip).second}};
        lista.push_back(a);
    }
    return {{"redes_do_computador", redes}, {"aparelhos", lista}};
}

json fatiador_estado()
{
    PresetBundle* b = wxGetApp().preset_bundle;
    if (b == nullptr)
        return {{"erro", "Predefinições indisponíveis."}};
    Preset&           pr     = b->printers.get_edited_preset();
    const std::string modelo = pr.config.has("printer_model") ? pr.config.opt_string("printer_model") : std::string();
    const std::string codigo = pr.get_printer_type(b);
    json filamentos = json::array();
    for (const auto& f : b->filament_presets)
        filamentos.push_back(f);
    json r = {{"impressora", pr.name}, {"processo", b->prints.get_edited_preset().name}, {"filamentos", filamentos},
              {"modelo_do_perfil", modelo}, {"codigo_do_modelo", codigo}};
    DeviceManager* dev = wxGetApp().getDeviceManager();
    if (MachineObject* o = dev != nullptr ? dev->get_selected_machine() : nullptr) {
        r["impressora_conectada"] = {{"dev_id", o->get_dev_id()}, {"nome", o->get_dev_name()}, {"modelo", o->printer_type},
                                     {"agente", o->printer_agent_id}};
        r["combina"] = o->printer_type == codigo || o->printer_type == modelo;
    }
    return r;
}

json fatiador_ajustar(const std::string& dev_id)
{
    DeviceManager* dev = wxGetApp().getDeviceManager();
    if (dev == nullptr)
        return {{"erro", "Gerenciador de dispositivos indisponível."}};
    MachineObject* o = dev_id.empty() ? dev->get_selected_machine() : dev->get_local_machine(dev_id);
    if (o == nullptr && !dev_id.empty())
        o = dev->get_my_machine(dev_id);
    if (o == nullptr)
        return {{"erro", "Impressora não encontrada no programa."}};
    ativar_marca_de(o);
    return fatiador_estado();
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

// Cadastro da impressora com este dev_id: o do vínculo, se ainda existe, ou o de mesmo número de série (refaz o
// vínculo). Uma impressora tem um cadastro só, mesmo que o cliente troque o código LAN, o IP ou o nome.
std::string cadastro_de(const std::string& dev_id)
{
    if (dev_id.empty())
        return {};
    Filuna::FilunaDb& db  = filuna_db();
    AppConfig*        cfg = wxGetApp().app_config;
    const std::string v   = cfg != nullptr ? cfg->get(chave_vinculo(dev_id)) : std::string();
    if (db.impressora_ativa(v))
        return v;
    const std::string id = db.impressora_por_serie(dev_id);
    if (cfg != nullptr && !id.empty())
        cfg->set(chave_vinculo(dev_id), id);
    return id;
}

json registrar(const std::string& nome, const std::string& modulo_id, const std::string& modelo, const std::string& dev_id)
{
    Filuna::FilunaDb& db = filuna_db();
    // Já cadastrada (ex.: o cliente trocou o código LAN e passou de novo pelo assistente): só atualiza.
    std::string id = cadastro_de(dev_id);
    if (!id.empty())
        db.atualizar_impressora(id, nome, modulo_id, modelo, dev_id, modulo_id);
    else
        id = db.criar_impressora(nome, modulo_id, modelo, dev_id, modulo_id);
    // recadastro (ex.: reconectar depois de "esquecer"): as bobinas que estavam nesta impressora voltam a ela
    if (const int n = db.religar_bobinas_de_cadastros_excluidos(); n > 0)
        BOOST_LOG_TRIVIAL(info) << "Filuna Dispositivos: " << n << " bobina(s) religadas ao cadastro " << id;
    if (AppConfig* cfg = wxGetApp().app_config; cfg != nullptr && !dev_id.empty())
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
    FilunaAoVivo::ativa(sel != nullptr ? sel->get_dev_id() : std::string());
    // Cadastros repetidos de versões antigas (o assistente criava um novo a cada passagem): junta uma vez por sessão.
    static bool juntou = false;
    if (!juntou) {
        juntou = true;
        for (const auto& [velho, fica] : filuna_db().juntar_impressoras_repetidas())
            BOOST_LOG_TRIVIAL(info) << "Filuna Dispositivos: cadastro repetido " << velho << " juntado em " << fica;
        if (const int n = filuna_db().religar_bobinas_de_cadastros_excluidos(); n > 0)
            BOOST_LOG_TRIVIAL(info) << "Filuna Dispositivos: " << n << " bobina(s) religadas ao cadastro ativo da impressora";
    }
    for (auto& [id, o] : todas) {
        if (o == nullptr)
            continue;
        json c = card(o);
        // não selecionada: o estado vem do acompanhamento em segundo plano (várias impressoras ao vivo)
        if (sel == nullptr || sel->get_dev_id() != o->get_dev_id()) {
            const json av = FilunaAoVivo::estado_de(o->get_dev_id()); // guardado: items() aponta para ele
            for (auto& [k, v] : av.items())
                c[k] = v;
        }
        c["selecionada"] = (sel != nullptr && sel->get_dev_id() == o->get_dev_id());
        c["cadastro_id"] = cadastro_de(o->get_dev_id());
        // Conectada de verdade e ainda sem cadastro (ex.: aberta pelo cartão, sem passar pelo assistente): cadastra
        // uma vez, para aparecer no site e no app como as do assistente.
        if (c["selecionada"].get<bool>() && o->is_connected())
            FilunaAoVivo::lembrar(o); // guarda IP, agente e código para acompanhar em segundo plano depois
        // Também a acompanhada em segundo plano e ligada (a A1 e a A2L do Fernando sumiram do site e da Farm porque só a
        // aberta era recadastrada). Excluir e esquecer param o acompanhamento, então a excluída não volta por aqui.
        const bool ligada_ao_fundo = !c["selecionada"].get<bool>() && c.value("ao_vivo", false) && c.value("ligada", false);
        if (cfg != nullptr && c["cadastro_id"].get<std::string>().empty() &&
            ((c["selecionada"].get<bool>() && o->is_connected()) || ligada_ao_fundo)) {
            std::string modulo;
            for (const auto& [mid, ag] : AGENTE)
                if (ag == o->printer_agent_id || (o->printer_agent_id == "filuna_elegoo_sdcp" && mid == "elegoo"))
                    modulo = mid;
            if (!modulo.empty()) {
                const std::string modelo = std::string(o->get_printer_type_display_str().ToUTF8().data());
                c["cadastro_id"] = registrar(o->get_dev_name(), modulo, modelo, o->get_dev_id()).value("id", std::string());
            }
        }
        lista.push_back(c);
    }
    // cadastradas no banco local que não estão na rede agora: aparecem desligadas
    for (const auto& imp : filuna_db().impressoras()) {
        const bool vista = std::any_of(lista.begin(), lista.end(), [&](const json& c) { return c.value("cadastro_id", std::string()) == imp.id; });
        if (!vista) {
            json c = {{"dev_id", std::string()}, {"cadastro_id", imp.id}, {"nome", imp.nome}, {"modelo_nome", imp.modelo},
                      {"fabricante", imp.fabricante}, {"ligada", false}, {"estado", "desligada"}, {"cores", json::array()},
                      {"camera", false}, {"selecionada", false}};
            // fora da lista da rede do programa (outra marca ativa), mas acompanhada em segundo plano: estado ao vivo
            for (const std::string& d : FilunaAoVivo::acompanhadas())
                if (cfg != nullptr && cfg->get(chave_vinculo(d)) == imp.id) {
                    c["dev_id"]   = d;
                    const json av = FilunaAoVivo::estado_de(d);
                    for (auto& [k, v] : av.items())
                        c[k] = v;
                    // câmera do agente em segundo plano (Anycubic, Elegoo, Klipper): o cartão da lista mostra o quadro
                    if (c.value("ligada", false)) {
                        camera_do_agente(c, d);
                        if (c.contains("camera_url"))
                            c["camera"] = true;
                    }
                    break;
                }
            lista.push_back(c);
        }
    }
    return {{"impressoras", lista}, {"agente", dev->get_current_printer_agent_id()}};
}

json esquecer(const std::string& dev_id)
{
    DeviceManager* dev = wxGetApp().getDeviceManager();
    AppConfig*     cfg = wxGetApp().app_config;
    if (dev == nullptr || cfg == nullptr || dev_id.empty())
        return {{"erro", "Informe a impressora (dev_id)."}};
    if (MachineObject* s = dev->get_selected_machine(); s != nullptr && s->get_dev_id() == dev_id)
        return {{"erro", "É a impressora aberta no programa: abra outra impressora antes de esquecer esta."}};
    FilunaAoVivo::esquecer(dev_id);
    dev->erase_local_machine(dev_id);
    cfg->erase_local_machine(dev_id);
    cfg->erase("app", "filuna_dispositivo_" + dev_id);
    cfg->save();
    BOOST_LOG_TRIVIAL(info) << "Filuna Dispositivos: impressora esquecida pelo suporte: " << dev_id;
    return {{"ok", true}, {"esquecida", dev_id}};
}

json excluir(const std::string& dev_id, const std::string& cadastro_id)
{
    DeviceManager* dev = wxGetApp().getDeviceManager();
    AppConfig*     cfg = wxGetApp().app_config;
    if (dev_id.empty() && cadastro_id.empty())
        return {{"erro", "Informe a impressora."}};
    std::string cadastro = cadastro_id;
    if (!dev_id.empty()) {
        if (cadastro.empty() && cfg != nullptr)
            cadastro = cfg->get(chave_vinculo(dev_id));
        if (dev != nullptr) {
            // a aberta no programa: desconecta antes (id vazio desconecta e tira a seleção)
            if (MachineObject* s = dev->get_selected_machine(); s != nullptr && s->get_dev_id() == dev_id)
                dev->set_selected_machine("");
            dev->erase_local_machine(dev_id);
        }
        FilunaAoVivo::esquecer(dev_id);
        if (cfg != nullptr) {
            cfg->erase_local_machine(dev_id);
            cfg->erase("app", "filuna_dispositivo_" + dev_id);
            cfg->erase("app", chave_vinculo(dev_id));
        }
    }
    if (!cadastro.empty())
        filuna_db().excluir_impressora(cadastro);
    if (cfg != nullptr)
        cfg->save();
    filuna_sync_avisar(); // a exclusão sobe para a conta: some do site e do app
    BOOST_LOG_TRIVIAL(info) << "Filuna Dispositivos: impressora excluída pelo cliente: " << dev_id << " " << cadastro;
    return {{"ok", true}};
}

bool temperaturas(const std::string& dev_id, double& bico, double& bico_alvo, double& mesa, double& mesa_alvo)
{
    bico = bico_alvo = mesa = mesa_alvo = 0.;
    DeviceManager* dev = wxGetApp().getDeviceManager();
    if (dev == nullptr || dev_id.empty())
        return false;
    MachineObject* o = dev->get_local_machine(dev_id);
    if (o == nullptr)
        o = dev->get_my_machine(dev_id);
    if (o == nullptr)
        return false;
    bool achou = false;
    if (DevExtderSystem* ext = o->GetExtderSystem()) {
        bico     = ext->GetNozzleTempCurrent(0);
        bico_alvo = ext->GetNozzleTempTarget(0);
        achou    = true;
    }
    if (DevBed* bed = o->GetBed()) {
        mesa  = bed->GetBedTemp();
        achou = true;
    }
    return achou;
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
    FilunaAoVivo::ativa(dev_id); // libera a conexão em segundo plano antes de o programa conectar
    // A Bambu só conecta com o código de acesso da tela dela: sem ele, pedir o código em vez de falhar calado.
    if (o->printer_agent_id == "filuna_bambu_lan" && o->get_access_code().empty() && !o->has_access_right())
        return {{"erro", "Esta impressora pede o código de acesso. " "Na impressora: Configurações › Rede (WLAN), ligue o Modo LAN e o Modo desenvolvedor e copie o Código de acesso."}, {"precisa_codigo", true}};
    if (!dev->set_selected_machine(dev_id))
        return {{"erro", falha_conexao(o)}, {"precisa_codigo", true}};
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
    FilunaAoVivo::iniciar();
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
            if (dev.empty() || !(i.value("selecionada", false) || i.value("ao_vivo", false)))
                continue; // leitura ao vivo: a impressora ativa e as acompanhadas em segundo plano
            const std::string a = antes[dev];
            {
                // Filuna Home: o motor de avisos decide quais aparelhos da casa avisam (TV, luz, popup, plugins…)
                json dados = {{"impressora", i.value("nome", std::string())}, {"peca", i.value("peca", std::string())},
                              {"progresso", i.value("progresso", 0)}, {"erro", i.value("erro", std::string())}};
                if ((a == "imprimindo" || a == "pausada") && (est == "concluida" || est == "livre"))
                    FilunaCasaAvisos::evento("concluida", dados);
                else if (a == "imprimindo" && est == "pausada" && i.value("erro", std::string()).empty())
                    FilunaCasaAvisos::evento("pausada", dados);
            }
            if ((a == "imprimindo" || a == "pausada") && (est == "concluida" || est == "livre")) {
                const wxString texto = Filuna::trw_valores("Impressão concluída na {0}. Retire a peça e limpe a mesa.", {i.value("nome", std::string(Filuna::tr("impressora")))});
                if (Plater* pl = wxGetApp().plater())
                    if (auto* nm = pl->get_notification_manager())
                        nm->push_notification(std::string(texto.ToUTF8().data()));
                wxNotificationMessage aviso(Filuna::trw("Filuna Slicer"), texto, wxGetApp().mainframe);
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
                    // cor sem "#" e sem o alfa, em maiúsculas: "ff6a14ff" e "#FF6A14" são a mesma bobina
                    std::string cor = txt("cor");
                    cor.erase(std::remove(cor.begin(), cor.end(), '#'), cor.end());
                    std::transform(cor.begin(), cor.end(), cor.begin(), [](unsigned char ch) { return char(std::toupper(ch)); });
                    std::string mat = txt("material");
                    std::transform(mat.begin(), mat.end(), mat.begin(), [](unsigned char ch) { return char(std::toupper(ch)); });
                    agora_f[txt("unidade") + ":" + txt("slot")] = cr.value("vazio", false) ? std::string() : mat + "|" + cor.substr(0, 6);
                }
                // Durante a impressão a impressora alterna as entradas (troca de cor) e algumas, como a CC2, mudam a
                // leitura da entrada em uso: isso não é bobina trocada. Na impressão e logo depois dela a leitura vira
                // a nova referência, sem perguntar do estoque.
                static std::map<std::string, bool> imprimia;
                const bool em_impressao = est == "imprimindo" || est == "pausada";
                const bool religar      = em_impressao || imprimia[dev];
                imprimia[dev]           = em_impressao;
                const bool primeira = fil_antes.find(dev) == fil_antes.end();
                auto&      ant      = fil_antes[dev];
                auto&      cand     = fil_cand[dev];
                std::string novas;
                for (const auto& [k, sig] : agora_f) {
                    if (primeira || religar) {
                        ant[k] = sig;
                        cand.erase(k);
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
                    FilunaCasaAvisos::evento("filamento", {{"impressora", i.value("nome", std::string())},
                                                           {"texto", "filamento trocado na entrada " + novas + "."}});
                    const wxString texto = Filuna::trw_valores("Filamento trocado na {0}: entrada {1}. Abra Dispositivo para cadastrar no estoque.", {i.value("nome", std::string(Filuna::tr("impressora"))), novas});
                    if (Plater* pl = wxGetApp().plater())
                        if (auto* nm = pl->get_notification_manager())
                            nm->push_notification(std::string(texto.ToUTF8().data()));
                    wxNotificationMessage aviso(Filuna::trw("Filuna Slicer — filamento trocado"), texto, wxGetApp().mainframe);
                    aviso.Show();
                }
            }
            {
                // Troca de cor na impressão: a impressora passou a puxar outra entrada. Avisa no Filuna (no máximo um
                // aviso por minuto, para impressão com muitas trocas não encher a tela).
                static std::map<std::string, int>                                   entrada_antes;
                static std::map<std::string, std::chrono::steady_clock::time_point> aviso_cor;
                int entrada = 0;
                const json sa = i.value("slot_ativo", json::object());
                if (est == "imprimindo" && sa.is_object()) {
                    const auto n = [&sa](const char* k) {
                        if (!sa.contains(k)) return -1;
                        try { return sa[k].is_number() ? sa[k].get<int>() : std::stoi(sa[k].get<std::string>()); } catch (...) { return -1; }
                    };
                    const int u = n("unidade"), sl = n("slot");
                    if (u >= 0 && u < 64 && sl >= 0 && sl < 16)
                        entrada = u * 4 + sl + 1;
                }
                const int  ant_e = entrada_antes[dev];
                const auto agora = std::chrono::steady_clock::now();
                if (entrada > 0 && ant_e > 0 && entrada != ant_e && agora - aviso_cor[dev] > std::chrono::seconds(60)) {
                    aviso_cor[dev] = agora;
                    const wxString texto = Filuna::trw_valores("{0}: troca de cor, agora puxando a entrada {1} (antes {2}).", {i.value("nome", std::string(Filuna::tr("Impressora"))), std::to_string(entrada), std::to_string(ant_e)});
                    if (Plater* pl = wxGetApp().plater())
                        if (auto* nm = pl->get_notification_manager())
                            nm->push_notification(std::string(texto.ToUTF8().data()));
                }
                entrada_antes[dev] = entrada;
            }
            const std::string err = i.value("erro", std::string());
            static std::map<std::string, std::string> erro_antes;
            if (!err.empty() && erro_antes[dev] != err) // Filuna Home: erro novo vai para os aparelhos da casa
                FilunaCasaAvisos::evento("erro", {{"impressora", i.value("nome", std::string())}, {"peca", i.value("peca", std::string())}, {"erro", err}});
            if (!err.empty() && erro_antes[dev] != err) { // erro novo da impressora (filamento rompido, falha…)
                const wxString texto = wxString::FromUTF8(i.value("nome", std::string("Impressora")) + ": " + err + ".");
                if (Plater* pl = wxGetApp().plater())
                    if (auto* nm = pl->get_notification_manager())
                        nm->push_notification(std::string(texto.ToUTF8().data()));
                wxNotificationMessage aviso(Filuna::trw("Filuna Slicer — impressão com problema"), texto, wxGetApp().mainframe, wxICON_WARNING);
                aviso.Show();
            }
            erro_antes[dev] = err;
            antes[dev] = est;
        }
        if (++volta % 3 == 0) {
            if (AppConfig* cfg = wxGetApp().app_config)
                sincronizar_estado(e, cfg->get("filuna_token"));
            FilunaCasa::publicar_home_assistant(e); // Filuna Home: impressoras no Home Assistant, se ligado
        }
        if (AppConfig* cfg = wxGetApp().app_config) // tomadas esperando o bico esfriar e avisos do site (vendas, API)
            FilunaCasaAvisos::vigiar(e, cfg->get("filuna_token"));
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
                         {"etapa", c.value("etapa", -1)}, // etapa da impressora: o site avisa aquecendo, troca de filamento e paradas
                         {"cores", c.value("cores", json::array())}, {"camera", c.value("camera", false)},
                         {"bico", c.value("bico", json::object())}, {"mesa", c.value("mesa", json::object())},
                         // Raio X da Impressora: camada, filamento em uso e código do erro
                         {"camada", c.value("camada", 0)}, {"camadas", c.value("camadas", 0)},
                         {"slot_ativo", c.value("slot_ativo", json::object())}, {"codigo_erro", c.value("codigo_erro", std::string())},
                         // Raio X do equipamento: coolers/velocidade como a impressora informa e as medidas extras
                         // (câmara, alvo da mesa, umidade das caixas, HMS, firmware, potência e rotação no Klipper)
                         {"ajustes", c.value("ajustes", json::object())}, {"extra", c.value("extra", json::object())},
                         {"agente", c.value("agente", std::string())}});
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
