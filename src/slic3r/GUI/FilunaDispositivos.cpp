#include "FilunaDispositivos.hpp"

#include "FilunaAtualizacao.hpp"
#include "FilunaDados.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "Plater.hpp"
#include "Tab.hpp"
#include "DeviceManager.hpp"
#include "DeviceCore/DevManager.h"
#include "DeviceCore/DevBed.h"
#include "DeviceCore/DevExtruderSystem.h"
#include "DeviceCore/DevFilaSystem.h"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/Filuna/FilunaDb.hpp"
#include "slic3r/Utils/Http.hpp"
#include "slic3r/Utils/NetworkAgent.hpp"
#include "slic3r/Utils/NetworkAgentFactory.hpp"

#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include <algorithm>
#include <chrono>
#include <map>
#include <mutex>

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
    {"anycubic", "moonraker"},          // família Klipper; ACE Pro via lane_data (rede-anycubic-moonraker)
    {"bambu", "filuna_bambu_lan"},      // LAN aberto (MQTT/FTPS), sem plug-in fechado
    {"creality", "crealityprint"},
    {"elegoo", "filuna_elegoo_cc2"},    // Centauri Carbon 2; CC/Neptune: SDCP (ver agente_da_marca)
    {"flashforge", "filuna_flashforge_lan"},
    {"qidi", "qidi"},
    {"prusa", "filuna_prusalink"},
    {"snapmaker", "snapmaker"},
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

// Caminho relativo à página (resources/web/filuna/) para a foto oficial do modelo (cover do Orca).
std::string cover(const std::string& vendor_id, const std::string& model_id)
{
    const boost::filesystem::path p = boost::filesystem::path(resources_dir()) / "profiles" / vendor_id / (model_id + "_cover.png");
    return boost::filesystem::exists(p) ? "../../profiles/" + vendor_id + "/" + model_id + "_cover.png" : std::string();
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
    if (o->is_in_printing_pause())
        return "pausada";
    if (o->is_in_printing())
        return "imprimindo";
    if (o->print_status == "FINISH")
        return "concluida";
    return "livre";
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
            cores.push_back({{"unidade", ams_id}, {"slot", tray_id}, {"cor", tray->color}, {"material", tray->m_fila_type},
                             {"vazio", tray->color.empty()}});
        }
    }
    return cores;
}

json card(MachineObject* o)
{
    json c = {{"dev_id", o->dev_id}, {"nome", o->dev_name}, {"ip", o->dev_ip}, {"modelo", o->printer_type},
              {"modelo_nome", std::string(o->get_printer_type_display_str().ToUTF8().data())},
              {"ligada", o->is_online()},      {"estado", estado_legivel(o)},
              {"conexao", o->dev_connection_type}, {"agente", o->printer_agent_id},
              {"progresso", o->mc_print_percent}, {"restante_s", o->mc_left_time},
              {"camera", o->has_ipcam},          {"cores", cores_de(o)}};
    if (DevBed* bed = o->GetBed())
        c["mesa"] = {{"atual", bed->GetBedTemp()}};
    if (DevExtderSystem* ext = o->GetExtderSystem())
        c["bico"] = {{"atual", ext->GetNozzleTempCurrent(0)}, {"alvo", ext->GetNozzleTempTarget(0)}};
    return c;
}

// dev_id -> id da impressora cadastrada no banco local (vínculo guardado no app_config)
std::string chave_vinculo(const std::string& dev_id) { return "filuna_dispositivo_" + dev_id; }

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
        const VendorProfile* vp = vendor_profile(vid);
        const auto ok = homologados(m);
        int disponiveis = 0;
        if (vp != nullptr)
            for (const auto& pm : vp->models)
                if (std::any_of(ok.begin(), ok.end(), [&](const std::string& n) { return nome_cobre_modelo(n, pm.name) || n == pm.id; }))
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
    const VendorProfile* vp = vendor_profile(vid);
    if (vp == nullptr)
        return {{"erro", "Perfis do fabricante não estão nesta versão do Filuna."}};
    const auto ok = homologados(*m);
    const auto fora = indisponiveis(*m);
    json lista = json::array();
    for (const auto& pm : vp->models) {
        const bool disponivel = std::any_of(ok.begin(), ok.end(), [&](const std::string& n) { return nome_cobre_modelo(n, pm.name) || n == pm.id; }) &&
                                std::none_of(fora.begin(), fora.end(), [&](const std::string& n) { return n == pm.name; });
        json bicos = json::array();
        for (const auto& v : pm.variants)
            bicos.push_back(v.name);
        lista.push_back({{"id", pm.id}, {"nome", pm.name}, {"familia", pm.family}, {"foto", cover(vid, pm.id)},
                         {"bicos", bicos}, {"disponivel", disponivel},
                         {"mensagem", disponivel ? std::string() : mensagem_indisponivel(*m)}});
    }
    // modelos citados no catálogo que não têm perfil nesta base (ex.: lista oficial maior que a do Orca)
    for (const auto& n : fora)
        if (std::none_of(vp->models.begin(), vp->models.end(), [&](const VendorProfile::PrinterModel& pm) { return pm.name == n; }))
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

json preparar(const std::string& modulo_id, const std::string& modelo)
{
    const std::string vid = vendor_orca(modulo_id);
    const Preset* p = preset_do_modelo(vid, modelo);
    if (p == nullptr)
        return {{"erro", "Não há perfil de máquina para " + modelo + " nesta versão do Filuna."}};
    AppConfig* cfg = wxGetApp().app_config;
    PresetBundle* bundle = wxGetApp().preset_bundle;
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
    // Agente da marca ao vivo + busca na LAN
    NetworkAgent* net = wxGetApp().getAgent();
    if (net != nullptr) {
        auto atual_ag = net->get_printer_agent();
        if (!atual_ag || atual_ag->get_agent_info().id != agente) {
            auto novo = NetworkAgentFactory::create_printer_agent_by_id(agente, net->get_cloud_agent(), data_dir());
            if (!novo)
                return {{"erro", "Agente de rede da marca não está disponível (" + agente + ")."}, {"perfil", p->name}};
            wxGetApp().set_live_printer_agent(novo);
        }
        net->start_discovery(true, true);
    }
    return {{"perfil", p->name}, {"agente", agente}, {"modelo_id", model_id}};
}

json descobertas()
{
    json lista = json::array();
    DeviceManager* dev = wxGetApp().getDeviceManager();
    if (dev == nullptr)
        return {{"impressoras", lista}};
    for (auto& [id, o] : dev->get_local_machinelist())
        if (o != nullptr)
            lista.push_back({{"dev_id", o->dev_id}, {"nome", o->dev_name}, {"ip", o->dev_ip}, {"modelo", o->printer_type},
                             {"modelo_nome", std::string(o->get_printer_type_display_str().ToUTF8().data())},
                             {"livre", o->bind_state != "occupied"}, {"ligada", o->is_online()},
                             {"precisa_codigo", o->get_access_code().empty() && !o->has_access_right()}});
    return {{"impressoras", lista}, {"agente", dev->get_current_printer_agent_id()}};
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
    if (!codigo.empty())
        o->set_access_code(codigo, false);
    if (!dev->set_selected_machine(dev_id))
        return {{"erro", "Não foi possível conectar. Confira o modo LAN e o código de acesso."}};
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
    return conectar(o->dev_id, codigo);
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
        c["selecionada"] = (sel != nullptr && sel->dev_id == o->dev_id);
        c["cadastro_id"] = cfg != nullptr ? cfg->get(chave_vinculo(o->dev_id)) : std::string();
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
    // perfil de máquina do modelo em Preparar (se o fabricante for conhecido pelo agente)
    const std::string agente = o->printer_agent_id;
    for (const auto& [mid, ag] : AGENTE)
        if (ag == agente || (agente == "filuna_elegoo_sdcp" && mid == "elegoo")) {
            const Preset* p = preset_do_modelo(vendor_orca(mid), o->printer_type);
            if (p == nullptr) {
                const wxString nome = o->get_printer_type_display_str();
                p = preset_do_modelo(vendor_orca(mid), std::string(nome.ToUTF8().data()));
            }
            if (p != nullptr)
                if (Tab* tab = wxGetApp().get_tab(Preset::TYPE_PRINTER))
                    tab->select_preset(p->name);
            break;
        }
    if (!dev->set_selected_machine(dev_id))
        return {{"erro", "Não foi possível conectar."}};
    if (NetworkAgent* net = wxGetApp().getAgent())
        net->set_user_selected_machine(dev_id);
    return {{"ok", true}, {"impressora", card(o)}};
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
