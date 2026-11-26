#include "FilunaPanel.hpp"
#include "slic3r/Utils/FilunaCameraUrls.hpp"
#include <ctime>
#include "FilunaAtualizacao.hpp"
#include "slic3r/Filuna/FilunaRecursos.hpp"
#include "slic3r/Filuna/FilunaVersoes.hpp"
#include "FilunaSync.hpp"
#include "FilunaDados.hpp"
#include "FilunaMidia.hpp"
#include "FilunaIALado.hpp"
#ifdef SLIC3R_CAD
#include "CAD/FilunaCadIa.hpp"
#endif
#include "FilunaDispositivos.hpp"
#include "FilunaDiagnostico.hpp"
#include "FilunaCasa.hpp"
#include "FilunaCasaAvisos.hpp"
#include "FilunaManutencao.hpp"
#include "FilunaImpressao.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <stdexcept>
#include <map>
#include <set>
#include <sstream>
#include <thread>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/fstream.hpp>

#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/utils.h>
#include <wx/base64.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/file.h>

#include <miniz.h>

#include "libslic3r/AppConfig.hpp"
#include "libslic3r/calib.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/SLAPrint.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/TriangleMesh.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/GCode/GCodeProcessor.hpp"
#include "libslic3r/GCode/ThumbnailData.hpp"

#include "GUI.hpp"
#include "GUI_App.hpp"
#include "I18N.hpp"
#include "GUI_ObjectList.hpp"
#include "MainFrame.hpp"
#include "PartPlate.hpp"
#include "PhysicalPrinterDialog.hpp"
#include "Plater.hpp"
#include "Tab.hpp"
#include "WebViewDialog.hpp"
#include "FilunaAssistente.hpp"
#include "libslic3r/miniz_extension.hpp"
#include "slic3r/Filuna/FilunaCalculos.hpp"
#include "slic3r/Utils/Http.hpp"
#include <regex>
#include <thread>
#if wxUSE_WEBVIEW_EDGE
#include <wx/msw/webview_edge.h>
#include <WebView2.h>
#endif
#include "Widgets/WebView.hpp"

using json = nlohmann::json;

namespace Slic3r { namespace GUI {

namespace {

// Every Filuna page alive, so one page can notify the others (e.g. preferences changed).
std::set<FilunaPanel*>& panels()
{
    static std::set<FilunaPanel*> all;
    return all;
}

// Section asked for by aba.abrir, kept until the page reads it with "secao.pendente".
std::string& pending_section()
{
    static std::string section;
    return section;
}

// Filuna works without internet: the pages keep their last data and the changes not sent yet in small JSON files
// under <data_dir>/filuna/local/, one per key (letters, digits, '.', '-' and '_' only).
boost::filesystem::path local_store_path(const std::string& key)
{
    if (key.empty() || key.size() > 120)
        throw std::runtime_error("Chave local inválida.");
    for (char c : key)
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '-' || c == '_'))
            throw std::runtime_error("Chave local inválida.");
    const boost::filesystem::path dir = boost::filesystem::path(data_dir()) / "filuna" / "local";
    boost::filesystem::create_directories(dir);
    return dir / (key + ".json");
}

// Only these app_config keys are reachable from the pages.
bool allowed_config_key(const std::string& key)
{
    return key == "filuna_token" || key == "filuna_ultima_aba" || key.rfind("filuna_pref_", 0) == 0;
}

template <class T> double first_value(const DynamicPrintConfig& cfg, const char* key)
{
    if (const auto* opt = cfg.option<T>(key); opt != nullptr && !opt->values.empty())
        return double(opt->values.front());
    return 0.;
}

std::string first_string(const DynamicPrintConfig& cfg, const char* key)
{
    if (const auto* opt = cfg.option<ConfigOptionStrings>(key); opt != nullptr && !opt->values.empty())
        return opt->values.front();
    return {};
}

json printer_info()
{
    json r = json::object();
    PresetBundle* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return r;
    const Preset& printer = bundle->printers.get_edited_preset();
    r["impressora"] = printer.label(false);
    r["tecnologia"] = printer.printer_technology() == ptSLA ? "resina" : "filamento";
    if (printer.printer_technology() == ptSLA && printer.config.has("sla_archive_format"))
        r["arquivo"] = printer.config.opt_string("sla_archive_format");
    if (const auto* model = printer.config.option<ConfigOptionString>("printer_model"))
        r["modelo"] = model->value;
    r["bico"] = first_value<ConfigOptionFloats>(printer.config, "nozzle_diameter");
    return r;
}

} // namespace

// O filamento que a peça usa de verdade: o da extrusora da primeira peça da mesa atual (cada entrada da lista de
// filamentos da lateral é uma extrusora). Antes o Assistente mostrava o perfil aberto para edição na aba de
// filamento, que com AMS (vários filamentos) é o último clicado: a peça em PLA aparecia como PETG.
const Preset& filuna_filamento_da_peca()
{
    PresetBundle& b     = *wxGetApp().preset_bundle;
    int           usada = -1;
    if (Plater* plater = wxGetApp().plater()) {
        PartPlate* mesa = plater->get_partplate_list().get_curr_plate();
        const auto& objetos = plater->model().objects;
        for (int i = 0; usada < 0 && i < int(objetos.size()); ++i) {
            const ModelObject* o = objetos[i];
            if (o == nullptr || !o->printable || o->instances.empty() || (mesa != nullptr && !mesa->contain_instance(i, 0)))
                continue;
            for (const ModelVolume* v : o->volumes)
                if (v->is_model_part()) {
                    usada = std::max(1, v->extruder_id()) - 1;
                    break;
                }
        }
    }
    if (usada < 0)
        usada = 0;
    if (usada < int(b.filament_presets.size())) {
        const std::string& nome = b.filament_presets[usada];
        if (b.filaments.get_selected_preset_name() == nome)
            return b.filaments.get_edited_preset(); // com os ajustes ainda não salvos
        if (const Preset* p = b.filaments.find_preset(nome))
            return *p;
    }
    return b.filaments.get_edited_preset();
}

namespace {

json filament_info()
{
    json r = printer_info();
    PresetBundle* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return r;
    const Preset& filament = filuna_filamento_da_peca();
    const DynamicPrintConfig& f = filament.config;
    r["filamento"]                     = filament.label(false);
    r["material"]                      = first_string(f, "filament_type");
    r["pressure_advance"]              = first_value<ConfigOptionFloats>(f, "pressure_advance");
    r["filament_flow_ratio"]           = first_value<ConfigOptionFloats>(f, "filament_flow_ratio");
    r["nozzle_temperature"]            = first_value<ConfigOptionInts>(f, "nozzle_temperature");
    r["filament_max_volumetric_speed"] = first_value<ConfigOptionFloats>(f, "filament_max_volumetric_speed");
    r["filament_density"]              = first_value<ConfigOptionFloats>(f, "filament_density");
    r["retraction_length"] = first_value<ConfigOptionFloats>(bundle->printers.get_edited_preset().config, "retraction_length");
    return r;
}

// Resina: o fatiamento fica no SLAPrint do Plater (a mesa só guarda o do filamento). Volumes em mm³, como os do
// filamento, para a conta de preço do servidor ser a mesma; o tempo é o mesmo que vai no .cxdlpv4.
json resin_slice_summary(json r)
{
    const SLAPrint& sla = wxGetApp().plater()->sla_print();
    if (!sla.finished())
        return r;
    const SLAPrintStatistics& stats = sla.print_statistics();
    const double peca = stats.objects_used_material, suporte = stats.support_used_material;
    if (peca + suporte <= 0.)
        return r;
    const DynamicPrintConfig& resina = wxGetApp().preset_bundle->sla_materials.get_edited_preset().config;
    const auto* dens = resina.option<ConfigOptionFloat>("material_density");
    const double densidade = dens != nullptr && dens->value > 0 ? dens->value : 1.1; // g/ml
    r["fatiado"]  = true;
    r["tempo_s"]  = std::isfinite(stats.estimated_print_time) ? stats.estimated_print_time : 0.;
    r["camadas"]  = sla.print_layers().size();
    r["volumes"]  = {{"peca", peca}, {"suporte", suporte}, {"torre", 0.}, {"purga", 0.}};
    r["material"] = "RESINA";
    r["gramas_estimadas"] = (peca + suporte) * densidade / 1000.;
    r["trocas_filamento"] = 0;
    return r;
}

json slice_summary()
{
    json r = {{"fatiado", false}, {"objetos", 0}};
    Plater* plater = wxGetApp().plater();
    if (plater == nullptr)
        return r;
    r["objetos"] = plater->model().objects.size();
    if (wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptSLA)
        return resin_slice_summary(r);
    PartPlate* plate = plater->get_partplate_list().get_curr_plate();
    if (plate == nullptr || !plate->is_slice_result_valid() || plate->get_slice_result() == nullptr)
        return r;
    const auto& stats = plate->get_slice_result()->print_statistics;
    auto sum = [](const std::map<size_t, double>& m) {
        double s = 0.;
        for (const auto& [id, v] : m)
            s += v;
        return s;
    };
    const double model = sum(stats.model_volumes_per_extruder), support = sum(stats.support_volumes_per_extruder),
                 tower = sum(stats.wipe_tower_volumes_per_extruder), flush = sum(stats.flush_per_filament);
    if (model + support + tower <= 0.)
        return r;
    const json fil = filament_info();
    const double density = fil.value("filament_density", 1.24) > 0 ? fil.value("filament_density", 1.24) : 1.24;
    r["fatiado"]  = true;
    r["tempo_s"]  = stats.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)].time;
    r["volumes"]  = {{"peca", model}, {"suporte", support}, {"torre", tower}, {"purga", flush}};
    r["material"] = fil.value("material", std::string("PLA"));
    r["gramas_estimadas"] = (model + support + tower + flush) * density / 1000.;
    r["trocas_filamento"] = stats.total_filament_changes;
    return r;
}

json recent_projects()
{
    json list = json::array();
    for (const std::string& path : wxGetApp().app_config->get_recent_projects()) {
        boost::system::error_code ec;
        const boost::filesystem::path p(path);
        if (!boost::filesystem::exists(p, ec))
            continue;
        json item = {{"caminho", path}, {"nome", p.stem().string()}};
        const std::time_t t = boost::filesystem::last_write_time(p, ec);
        if (!ec) {
            char buf[32];
            std::strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M", std::localtime(&t));
            item["quando"] = buf;
        }
        list.push_back(item);
    }
    std::reverse(list.begin(), list.end());   // newest first
    return {{"projetos", list}};
}

json model_objects()
{
    json list = json::array();
    if (Plater* plater = wxGetApp().plater()) {
        for (const ModelObject* obj : plater->model().objects) {
            const TriangleMeshStats s = obj->get_object_stl_stats();
            const RepairedMeshErrors& e = s.repaired_errors;
            list.push_back({{"nome", obj->name},
                            {"faces", s.number_of_facets},
                            {"bordas_abertas", s.open_edges},
                            {"reparados", e.edges_fixed + e.degenerate_facets + e.facets_removed + e.facets_reversed + e.backwards_edges}});
        }
    }
    return {{"objetos", list}};
}

double num(const json& d, const char* key, double fallback)
{
    if (!d.contains(key))
        return fallback;
    const json& v = d.at(key);
    if (v.is_number())
        return v.get<double>();
    if (v.is_string()) {
        try { return std::stod(v.get<std::string>()); } catch (...) {}
    }
    return fallback;
}

void start_calibration(const std::string& type, const json& p)
{
    Plater* plater = wxGetApp().plater();
    if (plater == nullptr)
        throw std::runtime_error("Preparação indisponível.");
    Calib_Params params;
    params.start = num(p, "inicio", 0.);
    params.end   = num(p, "fim", 0.);
    params.step  = num(p, "passo", 0.);
    if (type == "fluxo") {
        const int pass = num(p, "passe", 1.) >= 2. ? 2 : 1;
        plater->calib_flowrate(false, pass, num(p, "padrao", 0.) >= 1. ? ipMonotonic : ipArchimedeanChords);
        return;
    }
    if (params.step <= 0.)
        throw std::runtime_error("O passo precisa ser maior que zero.");
    if (type == "pa") {
        const int mode = int(num(p, "modo", 1.));
        params.mode          = mode == 3 ? CalibMode::Calib_PA_Tower : mode == 2 ? CalibMode::Calib_PA_Pattern : CalibMode::Calib_PA_Line;
        params.print_numbers = true;
        plater->calib_pa(params);
    } else if (type == "temperatura") {
        params.mode = CalibMode::Calib_Temp_Tower;
        plater->calib_temp(params);
    } else if (type == "vazao_max") {
        params.mode = CalibMode::Calib_Vol_speed_Tower;
        plater->calib_max_vol_speed(params);
    } else if (type == "retracao") {
        params.mode = CalibMode::Calib_Retraction_tower;
        plater->calib_retraction(params);
    } else {
        throw std::runtime_error("Calibração desconhecida.");
    }
}

json apply_calibration(const std::string& option, double value)
{
    // Retraction lives in the printer preset; the rest in the filament preset.
    const bool printer_opt = option == "retraction_length";
    if (option != "pressure_advance" && option != "filament_flow_ratio" && option != "nozzle_temperature" &&
        option != "filament_max_volumetric_speed" && !printer_opt)
        throw std::runtime_error("Ajuste desconhecido.");
    Tab* tab = wxGetApp().get_tab(printer_opt ? Preset::TYPE_PRINTER : Preset::TYPE_FILAMENT);
    if (tab == nullptr || tab->get_config() == nullptr)
        throw std::runtime_error("Perfil indisponível.");
    DynamicPrintConfig cfg = *tab->get_config();
    if (auto* f = cfg.option<ConfigOptionFloats>(option)) {
        for (double& v : f->values)
            v = value;
    } else if (auto* i = cfg.option<ConfigOptionInts>(option)) {
        for (int& v : i->values)
            v = int(std::lround(value));
        if (option == "nozzle_temperature")
            if (auto* first = cfg.option<ConfigOptionInts>("nozzle_temperature_initial_layer"))
                for (int& v : first->values)
                    v = int(std::lround(value));
    } else {
        throw std::runtime_error("Este perfil não tem esse ajuste.");
    }
    tab->load_config(cfg);
    tab->update_dirty();
    const PresetBundle* bundle = wxGetApp().preset_bundle;
    return {{"filamento", printer_opt ? bundle->printers.get_edited_preset().label(false) : bundle->filaments.get_edited_preset().label(false)}};
}

std::string ai_context()
{
    const json f = filament_info();
    const json s = slice_summary();
    std::ostringstream out;
    out << "Impressora: " << f.value("impressora", std::string()) << " (bico " << f.value("bico", 0.4) << " mm)\n";
    out << "Filamento: " << f.value("filamento", std::string()) << ", material " << f.value("material", std::string())
        << ", bico " << f.value("nozzle_temperature", 0.) << " °C, fluxo " << f.value("filament_flow_ratio", 1.)
        << ", pressure advance " << f.value("pressure_advance", 0.) << "\n";
    if (s.value("fatiado", false))
        out << "Fatiamento: " << int(s.value("tempo_s", 0.) / 60) << " min, cerca de " << int(s.value("gramas_estimadas", 0.))
            << " g, " << s.value("trocas_filamento", 0) << " trocas de filamento\n";
    else
        out << "Mesa ainda não fatiada; " << s.value("objetos", 0) << " objeto(s) na mesa.\n";
    return out.str();
}

// The current plate rendered as a PNG data URL, for product photos in the catalog.
json plate_image()
{
    Plater* plater = wxGetApp().plater();
    if (plater == nullptr || plater->model().objects.empty())
        throw std::runtime_error("Coloque um modelo na mesa primeiro.");
    PartPlateList& plates = plater->get_partplate_list();
    plater->update_plate_thumbnails(plates.get_curr_plate_index(), true);
    PartPlate* plate = plates.get_curr_plate();
    const ThumbnailData& data = plate->thumbnail_data;
    if (!data.is_valid())
        throw std::runtime_error("Não consegui gerar a imagem da mesa.");
    size_t size = 0;
    void* png = tdefl_write_image_to_png_file_in_memory_ex(data.pixels.data(), data.width, data.height, 4, &size, MZ_DEFAULT_LEVEL, 1);
    if (png == nullptr)
        throw std::runtime_error("Não consegui gerar a imagem da mesa.");
    const wxString b64 = wxBase64Encode(png, size);
    mz_free(png);
    return {{"imagem", "data:image/png;base64," + std::string(b64.ToUTF8())}};
}

// Photos picked on the computer, returned as data URLs (the page uploads them to the catalog).
json pick_images(wxWindow* parent)
{
    wxFileDialog dlg(parent, _L("Choose photos"), wxEmptyString, wxEmptyString, "JPG, PNG, WEBP (*.jpg;*.jpeg;*.png;*.webp)|*.jpg;*.jpeg;*.png;*.webp",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    json list = json::array();
    if (dlg.ShowModal() != wxID_OK)
        return {{"imagens", list}};
    wxArrayString paths;
    dlg.GetPaths(paths);
    for (const wxString& path : paths) {
        wxFile file(path);
        if (!file.IsOpened() || file.Length() <= 0 || file.Length() > 8 * 1024 * 1024)
            continue;
        std::vector<char> buf(size_t(file.Length()));
        if (static_cast<size_t>(file.Read(buf.data(), buf.size())) != buf.size())
            continue;
        const wxString ext = wxFileName(path).GetExt().Lower();
        const std::string mime = ext == "png" ? "image/png" : ext == "webp" ? "image/webp" : "image/jpeg";
        list.push_back({{"nome", std::string(wxFileName(path).GetFullName().ToUTF8())},
                        {"imagem", "data:" + mime + ";base64," + std::string(wxBase64Encode(buf.data(), buf.size()).ToUTF8())}});
        if (list.size() >= 12)
            break;
    }
    return {{"imagens", list}};
}

// Paramétricos: cada modelo é um pacote HTML+JS do servidor (/api/slicer/v1/recursos), instalado em
// <data_dir>/filuna/parametricos/<id>/<versão>/ e aberto nesta mesma janela. O roteiro abaixo entra em toda página
// do pacote: põe o botão de voltar e troca o "baixar STL" do gerador por "colocar na mesa da Preparação".
boost::filesystem::path pasta_parametricos() { return boost::filesystem::path(data_dir()) / "filuna" / "parametricos"; }

const char* const ROTEIRO_PARAMETRICO = R"JS((function () {
  if (location.hostname !== "parametricos.filuna.local" && !/\/filuna\/parametricos\//.test(decodeURIComponent(location.href).replace(/\\/g, "/"))) return;
  function env(acao, d) { try { window.wx.postMessage(JSON.stringify({ filuna: 1, id: "pm" + Date.now(), acao: acao, dados: d || {} })); } catch (e) {} }
  function aviso(t) {
    var a = document.getElementById("flAviso"); if (!a) return;
    a.textContent = t; a.style.display = "block"; clearTimeout(a._t); a._t = setTimeout(function () { a.style.display = "none"; }, 5000);
  }
  document.addEventListener("DOMContentLoaded", function () {
    var b = document.createElement("div");
    b.style.cssText = "position:fixed;top:10px;right:12px;z-index:2147483647;display:flex;gap:8px;align-items:center;font-family:Inter,Segoe UI,sans-serif";
    b.innerHTML = '<span id="flAviso" style="display:none;background:#1F8F5F;color:#fff;border-radius:10px;padding:9px 14px;font-size:13.5px;font-weight:600"></span>' +
      '<button id="flVoltar" style="height:38px;padding:0 14px;border-radius:10px;border:1px solid #E7E3DD;background:#fff;color:#2C2A28;font:600 14px Inter,Segoe UI,sans-serif;cursor:pointer">← Paramétricos</button>';
    document.body.appendChild(b);
    document.getElementById("flVoltar").onclick = function () { env("parametrico.voltar"); };
  });
  function mandar(href, nome) {
    fetch(href).then(function (r) { return r.arrayBuffer(); }).then(function (ab) {
      var u = new Uint8Array(ab), s = "";
      for (var k = 0; k < u.length; k += 0x8000) s += String.fromCharCode.apply(null, u.subarray(k, k + 0x8000));
      env("parametrico.mesa", { nome: nome, dados: btoa(s) });
      aviso("Peça enviada para a mesa da Preparação");
    }).catch(function () { aviso("Não deu para pegar a peça gerada"); });
  }
  function peca(a) { return a && a.download && /\.(stl|3mf|obj)$/i.test(a.download) && /^(blob:|data:)/.test(a.href || ""); }
  var clicar = HTMLAnchorElement.prototype.click;
  HTMLAnchorElement.prototype.click = function () { if (peca(this)) { mandar(this.href, this.download); return; } return clicar.apply(this, arguments); };
  document.addEventListener("click", function (ev) {
    var a = ev.target && ev.target.closest ? ev.target.closest("a[download]") : null;
    if (peca(a)) { ev.preventDefault(); ev.stopPropagation(); mandar(a.href, a.download); }
  }, true);
})();)JS";

const char* const HOST_PARAMETRICOS = "parametricos.filuna.local";

bool url_de_parametrico(const wxString& url)
{
    wxString u = url.Lower();
    u.Replace("\\", "/");
    u.Replace("%5c", "/");
    return u.StartsWith(wxString("https://") + HOST_PARAMETRICOS + "/") || u.Contains("/filuna/parametricos/");
}

// Pacote com módulos JS (three.js etc.) não roda de file:// (o navegador bloqueia módulos sem origem). No Windows a
// pasta dos paramétricos vira https://parametricos.filuna.local/ dentro do próprio WebView2, sem servidor nenhum.
bool mapear_parametricos(wxWebView* b)
{
#if wxUSE_WEBVIEW_EDGE
    ICoreWebView2* wv = b != nullptr ? (ICoreWebView2*) b->GetNativeBackend() : nullptr;
    if (wv == nullptr)
        return false;
    ICoreWebView2_3* wv3 = nullptr;
    if (wv->QueryInterface(&wv3) != S_OK || wv3 == nullptr)
        return false;
    const std::wstring pasta = pasta_parametricos().wstring();
    const HRESULT      hr    = wv3->SetVirtualHostNameToFolderMapping(wxString(HOST_PARAMETRICOS).wc_str(), pasta.c_str(),
                                                                       COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
    wv3->Release();
    return hr == S_OK;
#else
    (void) b;
    return false;
#endif
}

} // namespace

FilunaPanel::FilunaPanel(wxWindow* parent, const std::string& page) : wxPanel(parent, wxID_ANY)
{
    SetBackgroundColour(wxColour(0xED, 0xEB, 0xE7));
    FilunaDispositivos::monitor_iniciar(); // vigia das impressoras: avisos e estado para a conta, com qualquer aba aberta
    // Fase 7: correção de páginas vinda do servidor (recurso "telas" da geração deste programa, no catálogo assinado)
    // vale sobre as embutidas quando é mais nova.
    boost::filesystem::path base = boost::filesystem::path(resources_dir()) / "web" / "filuna";
    const std::string atualizadas =
        Filuna::Recursos::dir_da_correcao((boost::filesystem::path(data_dir()) / "filuna" / "recursos").string(), "telas",
                                          Filuna::versao_embutida("telas"));
    if (!atualizadas.empty() && boost::filesystem::exists(boost::filesystem::path(atualizadas) / page)) {
        base = atualizadas;
        BOOST_LOG_TRIVIAL(info) << "Filuna: tela " << page << " vem da correção do servidor: " << atualizadas;
    }
    m_url = file_url_from_path(base / page);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(sizer);
    create_browser();
    m_reset_on_show = WebView::NeedsRecreateOnShow();
    panels().insert(this);
    Bind(wxEVT_DESTROY, [this](wxWindowDestroyEvent& e) {
        if (e.GetEventObject() == this)
            panels().erase(this);
        e.Skip();
    });
}

void FilunaPanel::create_browser()
{
    m_browser = WebView::CreateWebView(this, wxEmptyString);
    if (m_browser == nullptr)
        return;
    m_browser->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &FilunaPanel::on_script_message, this, m_browser->GetId());
    if (m_url.EndsWith("parametricos.html"))
        m_browser->AddUserScript(wxString::FromUTF8(ROTEIRO_PARAMETRICO));
    GetSizer()->Insert(0, m_browser, wxSizerFlags().Expand().Proportion(1));
    WebView::LoadUrl(m_browser, m_url);
    Layout();
}

bool FilunaPanel::Show(bool show)
{
    if (show && m_reset_on_show && m_browser != nullptr) {
        m_reset_on_show = false;
        m_browser->Destroy();
        create_browser();
    }
    const bool changed = wxPanel::Show(show);
    if (show)
        send_event("mostrar");
    else if (m_estado_timer)
        m_estado_timer->Stop();
    return changed;
}

void FilunaPanel::send_event(const std::string& name, const json& data)
{
    if (m_browser == nullptr)
        return;
    WebView::RunScript(m_browser, wxString::FromUTF8("window.Filuna && Filuna._evento(" + json(name).dump() + "," + data.dump() + ");"));
}

void FilunaPanel::reply(const std::string& id, bool ok, const json& data)
{
    if (m_browser == nullptr)
        return;
    WebView::RunScript(m_browser, wxString::FromUTF8("window.Filuna && Filuna._resposta(" + json(id).dump() + "," + (ok ? "true" : "false") + "," +
                                                     data.dump() + ");"));
}

void FilunaPanel::open_tab(const std::string& tab, const std::string& section)
{
    MainFrame* frame = wxGetApp().mainframe;
    if (frame == nullptr)
        return;
    pending_section() = section;
    frame->select_filuna_tab(tab);
    if (!section.empty())
        for (FilunaPanel* p : panels())
            p->send_event("secao", {{"secao", section}});
}

void FilunaPanel::broadcast(const std::string& name, const json& data)
{
    for (FilunaPanel* p : panels())
        p->send_event(name, data);
}

void FilunaPanel::account_changed(const std::string& token, const FilunaPanel* source)
{
    FilunaSync::instancia().avisar(token); // entrou ou saiu da conta: sincroniza (ou para) com a conta nova
    FilunaAtualizacao::instancia().avisar(token); // e confere se há recursos ou módulos novos
    FilunaManutencao::conta(token);               // e se há versão nova do programa
    for (FilunaPanel* p : panels())
        if (p != source)
            p->send_event("conta", {{"token", token}});
    if (source != nullptr)
        if (WebViewPanel* home = WebViewPanel::if_built()) {
            const std::string js = "if (window.FlTokenDeFora) FlTokenDeFora(" + json(token).dump() + ");";
            home->RunScript(wxString::FromUTF8(js));
        }
}

void FilunaPanel::on_script_message(wxWebViewEvent& evt)
{
    json msg;
    try {
        msg = json::parse(evt.GetString().ToUTF8().data());
    } catch (...) {
        return;
    }
    if (!msg.is_object() || msg.value("filuna", 0) != 1)
        return;
    // Só as telas do próprio programa (file://) falam com o Slicer. Página da internet aberta aqui dentro (a Farm do
    // site) não chega aos comandos de impressora, arquivos e conta.
    if (m_browser == nullptr)
        return;
    const wxString atual = m_browser->GetCurrentURL();
    const bool     param = url_de_parametrico(atual);
    if (!atual.Lower().StartsWith("file:") && !param)
        return;
    const std::string id     = msg.value("id", std::string());
    const std::string action = msg.value("acao", std::string());
    // Página de um paramétrico (pacote do servidor): só pode pôr a peça na mesa e voltar à lista.
    // Pode também usar a IA ao lado (FilunaIALado.cpp): a geração sai na conta do cliente, na janela dela.
    if (param && action != "parametrico.mesa" && action != "parametrico.voltar" && action.rfind("ia.lado.", 0) != 0)
        return;
    const json data          = msg.contains("dados") && msg["dados"].is_object() ? msg["dados"] : json::object();
    // Rede (assistência remota): testar portas e procurar na rede levam segundos; rodam fora da linha da tela, para o
    // programa do cliente não congelar, e respondem quando terminam (se esta tela ainda estiver aberta).
    // Raio X do equipamento: leitura e testes esperam a impressora (e leem o estado pela linha da tela), então também
    // rodam fora dela. teste.estado, parar e presenca respondem na hora (mais abaixo).
    if (action == "rede.diagnostico" || action == "rede.procurar" || action == "diagnostico.ler" || action == "diagnostico.testes" ||
        action == "diagnostico.testar") {
        FilunaPanel* eu = this;
        std::thread([eu, id, action, data] {
            json r;
            bool ok = true;
            try {
                if (action == "diagnostico.ler")
                    r = FilunaDiagnostico::ler(data.value("dev_id", std::string()), data.value("origem", std::string("cliente")));
                else if (action == "diagnostico.testes")
                    r = FilunaDiagnostico::testes(data.value("dev_id", std::string()));
                else if (action == "diagnostico.testar")
                    r = FilunaDiagnostico::testar(data);
                else
                r = action == "rede.procurar" ? FilunaDispositivos::rede_procurar()
                                              : FilunaDispositivos::rede_diagnostico(data.value("ip", std::string()), data.value("fabricante", std::string()));
            } catch (const std::exception& e) {
                ok = false;
                r  = {{"erro", e.what()}};
            }
            wxGetApp().CallAfter([eu, id, ok, r, action]() mutable {
                if (action == "rede.procurar")
                    r["descobertas"] = FilunaDispositivos::descobertas(); // impressoras que a busca da marca já achou
                if (panels().count(eu))
                    eu->reply(id, ok, r);
            });
        }).detach();
        return;
    }
    try {
        json r;
        if (!FilunaManutencao::acao(action, data, r))
            r = handle(action, data);
        // Diagnóstico: resposta com "erro" também vai para o servidor (o dono vê em /admin/diagnostico)
        if (r.is_object() && r.contains("erro") && r["erro"].is_string())
            FilunaManutencao::diagnostico(action, r["erro"].get<std::string>(), {{"dados", data}});
        reply(id, true, r);
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "Filuna bridge " << action << ": " << e.what();
        // erros de rotina de conta/sincronia sem internet não são defeito do programa
        if (action.rfind("dispositivos.", 0) == 0 || action.rfind("atualizacao.", 0) == 0 || action.rfind("camera.", 0) == 0 ||
            action.rfind("fatiar", 0) == 0 || action.rfind("imprimir", 0) == 0)
            FilunaManutencao::diagnostico(action, e.what(), {{"dados", data}});
        reply(id, false, {{"erro", e.what()}});
    }
}

json FilunaPanel::handle(const std::string& action, const json& d)
{
    // Banco local (estoque, histórico, painel, manutenção): FilunaDados.cpp.
    if (auto dados = filuna_dados_acao(action, d))
        return *dados;
    // Assistente de impressão (etapas, conferência da peça, ajustes, pasta automática): FilunaAssistente.cpp.
    if (auto assistente = filuna_assistente_acao(action, d))
        return *assistente;
    // Post nas redes sociais (só com a ferramenta liberada para a conta): FilunaMidia.cpp.
    if (auto ia = filuna_ia_lado_acao(action, d))
        return *ia;
    if (auto midia = filuna_midia_acao(action, d))
        return *midia;
#ifdef SLIC3R_CAD
    // Filuna CAD com a IA do cliente (Modelar com IA): CAD/DesignPanelFiluna.cpp.
    if (auto cad = filuna_cad_acao(action, d))
        return *cad;
#endif
    AppConfig* cfg = wxGetApp().app_config;
    if (action == "config.ler") {
        const std::string key = d.value("chave", std::string());
        if (!allowed_config_key(key))
            throw std::runtime_error("Chave não permitida.");
        return {{"valor", cfg->get(key)}};
    }
    if (action == "config.gravar") {
        const std::string key = d.value("chave", std::string());
        if (!allowed_config_key(key))
            throw std::runtime_error("Chave não permitida.");
        const std::string value = d.value("valor", std::string());
        const bool account = key == "filuna_token" && cfg->get(key) != value;
        cfg->set(key, value);
        cfg->save();
        if (account)
            account_changed(value, this);
        return json::object();
    }
    if (action == "aba.abrir") {
        const std::string tab = d.value("aba", std::string()), section = d.value("secao", std::string());
        CallAfter([tab, section] { open_tab(tab, section); });
        return json::object();
    }
    if (action == "farm.destravar") { // reparo pelo suporte remoto
        FilunaImpressao::farm_destravar();
        return json{{"ok", true}};
    }
    if (action.rfind("farm.", 0) == 0 || action.rfind("modelo3d.", 0) == 0) {   // Farm local: abrir, analisar, medir e salvar o projeto (respostas longas por evento)
        FilunaPanel* eu = this;
        return FilunaImpressao::farm_acao(action, d, [eu](const std::string& nome, const json& dados) {
            wxGetApp().CallAfter([eu, nome, dados] {
                if (panels().count(eu))
                    eu->send_event(nome, dados);
            });
        });
    }
    if (action == "parametrico.abrir") {
        const std::string pid = d.value("id", std::string()), ver = d.value("versao", std::string()), sha = d.value("sha256", std::string()),
                          url = d.value("url", std::string()), entrada = d.value("entrada", std::string("index.html"));
        if (!std::regex_match(pid, std::regex("^[a-z0-9][a-z0-9-]{1,59}$")) || !std::regex_match(ver, std::regex("^\\d+\\.\\d+\\.\\d+$")) ||
            !std::regex_match(sha, std::regex("^[0-9a-f]{64}$")) || url.rfind("/api/slicer/v1/recursos/", 0) != 0 ||
            !std::regex_match(entrada, std::regex("^[A-Za-z0-9_./-]{1,80}$")) || entrada.find("..") != std::string::npos)
            throw std::runtime_error("Modelo paramétrico inválido.");
        const boost::filesystem::path pasta = pasta_parametricos() / pid / ver, pagina = pasta / entrada;
        const std::string token = wxGetApp().app_config->get("filuna_token");
        FilunaPanel* eu = this;
        const std::string rel = pid + "/" + ver + "/" + entrada;
        std::thread([eu, pasta, pagina, url, sha, token, rel] {
            std::string erro;
            boost::system::error_code ec;
            if (!boost::filesystem::exists(pasta / ".pronto", ec)) {
                boost::filesystem::remove_all(pasta, ec);
                boost::filesystem::create_directories(pasta, ec);
                std::string corpo;
                unsigned    status = 0;
                Http        http   = Http::get("https://filuna3d.com" + url);
                http.tls_verify(true);
                http.timeout_connect(15);
                http.timeout_max(300);
                http.header("Authorization", "Bearer " + token)
                    .on_complete([&](std::string b, unsigned st) { corpo = std::move(b), status = st; })
                    .on_error([&](std::string, std::string e, unsigned st) { status = st, erro = e; })
                    .perform_sync();
                const boost::filesystem::path zip = pasta / "pacote.zip";
                if (status == 200) {
                    boost::nowide::ofstream o(zip.string(), std::ios::binary | std::ios::trunc);
                    o.write(corpo.data(), std::streamsize(corpo.size()));
                }
                if (status != 200)
                    erro = "o servidor não entregou o modelo (" + (erro.empty() ? std::to_string(status) : erro) + ")";
                else if (Slic3r::Filuna::sha256_de_arquivo(zip.string()) != sha)
                    erro = "o pacote do modelo chegou diferente do esperado";
                else if (!extract_archive_confined(zip.string(), pasta.string()))
                    erro = "não abriu o pacote do modelo";
                else {
                    boost::filesystem::remove(zip, ec);
                    boost::nowide::ofstream(( pasta / ".pronto").string()) << "1";
                }
            }
            if (erro.empty() && !boost::filesystem::exists(pagina, ec))
                erro = "o pacote não tem a página do modelo";
            wxGetApp().CallAfter([eu, pagina, erro, rel] {
                if (panels().count(eu) == 0 || eu->m_browser == nullptr)
                    return;
                if (!erro.empty()) {
                    eu->send_event("parametrico.erro", {{"erro", erro}});
                    return;
                }
                if (mapear_parametricos(eu->m_browser))
                    eu->m_browser->LoadURL(wxString("https://") + HOST_PARAMETRICOS + "/" + wxString::FromUTF8(rel));
                else
                    WebView::LoadUrl(eu->m_browser, file_url_from_path(pagina));
            });
        }).detach();
        return json::object();
    }
    if (action == "parametrico.voltar") {
        CallAfter([this] { WebView::LoadUrl(m_browser, m_url); });
        return json::object();
    }
    if (action == "parametrico.mesa") {
        std::string nome = d.value("nome", std::string("parametrico.stl"));
        for (char& c : nome)
            if (!(std::isalnum((unsigned char) c) || c == '-' || c == '_' || c == '.'))
                c = '_';
        if (nome.size() > 80 || !std::regex_match(nome, std::regex("^[A-Za-z0-9_.-]+\\.(stl|STL|3mf|3MF|obj|OBJ)$")))
            nome = "parametrico.stl";
        const std::string b64 = d.value("dados", std::string());
        if (b64.empty() || b64.size() > 200u * 1024u * 1024u)
            throw std::runtime_error("Peça gerada vazia ou grande demais.");
        const wxMemoryBuffer bin = wxBase64Decode(b64.data(), b64.size());
        if (bin.GetDataLen() < 84)
            throw std::runtime_error("Peça gerada inválida.");
        const boost::filesystem::path gerados = pasta_parametricos() / "gerados";
        boost::system::error_code ec;
        boost::filesystem::create_directories(gerados, ec);
        const boost::filesystem::path arq = gerados / nome;
        {
            boost::nowide::ofstream o(arq.string(), std::ios::binary | std::ios::trunc);
            o.write(static_cast<const char*>(bin.GetData()), std::streamsize(bin.GetDataLen()));
            if (!o.good())
                throw std::runtime_error("Não gravou a peça gerada.");
        }
        CallAfter([arq] {
            Plater* plater = wxGetApp().plater();
            if (plater == nullptr)
                return;
            if (MainFrame* frame = wxGetApp().mainframe)
                frame->select_filuna_tab(TAB_ID_PREPARE);
            plater->load_files(std::vector<boost::filesystem::path>{arq}, LoadStrategy::LoadModel);
        });
        return json::object();
    }
    if (action == "secao.pendente") {
        const std::string section = pending_section();
        pending_section().clear();
        return {{"secao", section}};
    }
    if (action == "perfis.salvos") {
        // The user's own process presets (the ones made with "Salvar" in Preparação), for Meu espaço.
        // "em_uso" is the process preset selected in Preparação, even when it is a system preset.
        json        list = json::array();
        std::string current;
        if (PresetBundle* bundle = wxGetApp().preset_bundle) {
            current = bundle->prints.get_selected_preset_name();
            for (const Preset& preset : bundle->prints)
                if (preset.is_user() && preset.is_visible)
                    list.push_back({{"nome", preset.name}, {"base", preset.inherits()}, {"atual", preset.name == current}});
        }
        return {{"perfis", list}, {"em_uso", current}};
    }
    if (action == "perfil.usar") {
        const std::string name = d.value("nome", std::string());
        CallAfter([name] {
            if (MainFrame* frame = wxGetApp().mainframe)
                frame->select_filuna_tab(TAB_ID_PREPARE);
            if (Tab* tab = wxGetApp().get_tab(Preset::TYPE_PRINT))
                tab->select_preset(name);
        });
        return json::object();
    }
    if (action == "armazem.ler") {
        const boost::filesystem::path file = local_store_path(d.value("chave", std::string()));
        if (!boost::filesystem::exists(file))
            return {{"valor", nullptr}};
        boost::nowide::ifstream in(file.string());
        json value = json::parse(in, nullptr, false);
        return {{"valor", value.is_discarded() ? json() : value}};
    }
    if (action == "armazem.gravar") {
        const boost::filesystem::path file = local_store_path(d.value("chave", std::string()));
        const std::string text = d.contains("valor") ? d["valor"].dump() : std::string("null");
        if (text.size() > 8u * 1024u * 1024u)
            throw std::runtime_error("Dados grandes demais para guardar no computador.");
        const boost::filesystem::path tmp = file.string() + ".tmp";
        {
            boost::nowide::ofstream out(tmp.string(), std::ios::binary | std::ios::trunc);
            out << text;
            if (!out)
                throw std::runtime_error("Não foi possível gravar no computador.");
        }
        boost::filesystem::rename(tmp, file); // atomic replace: a crash never leaves half a file
        return json::object();
    }
    if (action == "impressoras.registradas") {
        // Printers registered in this Filuna Slicer (Preparação › Impressora › conexão); available without internet.
        json list = json::array();
        if (PresetBundle* bundle = wxGetApp().preset_bundle)
            for (const PhysicalPrinter& printer : bundle->physical_printers) {
                json presets = json::array();
                for (const std::string& name : printer.preset_names)
                    presets.push_back(name);
                const std::string host = printer.config.has("print_host") ? printer.config.opt_string("print_host") : std::string();
                // filamento ou resina, pela tecnologia do primeiro perfil (a tela Dispositivo separa as duas)
                std::string tecnologia = "filamento";
                for (const std::string& name : printer.preset_names)
                    if (const Preset* p = bundle->printers.find_preset(name)) {
                        tecnologia = p->printer_technology() == ptSLA ? "resina" : "filamento";
                        break;
                    }
                list.push_back({{"nome", printer.name}, {"perfis", presets}, {"endereco", host}, {"tecnologia", tecnologia}});
            }
        return {{"impressoras", list}};
    }
    if (action == "impressora.adicionar") {
        CallAfter([] {
            MainFrame* frame = wxGetApp().mainframe;
            if (frame == nullptr)
                return;
            PresetBundle* bundle = wxGetApp().preset_bundle;
            if (bundle != nullptr && bundle->is_bbl_vendor()) {
                // Bambu printers are found and bound in Orca's own device page.
                frame->select_filuna_tab(TAB_ID_MONITOR);
                return;
            }
            PhysicalPrinterDialog dlg(wxGetApp().plater()); // same parent as the Preparação connection button
            dlg.ShowModal();
            for (FilunaPanel* p : panels())
                p->send_event("impressoras");
        });
        return json::object();
    }
    if (action == "preferencias.abrir") {
        // Filuna options are part of the program settings (gear of the navigation bar).
        CallAfter([] { wxGetApp().open_preferences(); });
        return json::object();
    }
    if (action == "app.metricas") {
        // The Filuna pages use the width of the Preparação side panel, so every screen has the same left column.
        int side = 0;
        if (wxGetApp().plater() != nullptr)
            side = wxGetApp().sidebar().ToDIP(wxGetApp().sidebar().GetSize()).GetWidth();
        return {{"lateral", side >= 280 && side <= 560 ? side : 390}};
    }
    if (action == "impressora.atual")
        return printer_info();
    if (action == "filamento.atual")
        return filament_info();
    if (action == "fatiamento.resumo")
        return slice_summary();
    if (action == "contexto.ia")
        return {{"texto", ai_context()}};
    if (action == "calibracao.gerar") {
        const std::string type = d.value("tipo", std::string());
        const json params      = d.contains("parametros") ? d["parametros"] : json::object();
        // Validate now so the page can show the error; the test itself is built after this reply.
        if (type != "fluxo" && num(params, "passo", 0.) <= 0.)
            throw std::runtime_error("O passo precisa ser maior que zero.");
        CallAfter([type, params] {
            try {
                if (MainFrame* frame = wxGetApp().mainframe)
                    frame->select_filuna_tab(TAB_ID_PREPARE);
                start_calibration(type, params);
            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << "Filuna calibration: " << e.what();
            }
        });
        return json::object();
    }
    if (action == "calibracao.aplicar")
        return apply_calibration(d.value("opcao", std::string()), num(d, "valor", 0.));
    // ---- Dispositivos (fluxo V2 §6): fabricante → modelos homologados → complemento → LAN → card ----
    namespace FD = FilunaDispositivos;
    if (action == "dispositivos.fabricantes")
        return FD::fabricantes();
    if (action == "dispositivos.modelos")
        return FD::modelos(d.value("fabricante", std::string()));
    if (action == "dispositivos.complemento")
        return FD::complemento(d.value("fabricante", std::string()));
    if (action == "dispositivos.complemento.baixar") {
        // "Baixar complemento": update interno do módulo da marca, com progresso para a tela de carregamento
        const std::string id = d.value("fabricante", std::string());
        FilunaAtualizacao::instancia().instalar_modulo(id, [id](const json& p) {
            json ev = p;
            ev["fabricante"] = id;
            if (!p.value("ok", true) && p.contains("erro"))
                FilunaManutencao::diagnostico("dispositivos.complemento.baixar", p.value("erro", std::string()), {{"fabricante", id}});
            wxGetApp().CallAfter([ev] { FilunaPanel::broadcast("complemento.progresso", ev); });
        });
        return json::object();
    }
    if (action == "dispositivos.preparar")
        return FD::preparar(d.value("fabricante", std::string()), d.value("modelo", std::string()));
    if (action == "dispositivos.descobertas")
        return FD::descobertas();
    // ---- Filuna Home (Dispositivo › Casa): aparelhos da rede da casa ----
    if (action == "casa.procurar")
        return FilunaCasa::procurar();
    if (action == "casa.resultado")
        return FilunaCasa::resultado();
    if (action == "casa.acao")
        return FilunaCasa::acao(d);
    // motor de avisos: regras de cada aparelho, teste de conexão e histórico
    // Raio X do equipamento: andamento do teste, interromper e a confirmação de presença (só a tela do programa pede)
    if (action == "diagnostico.teste.estado")
        return FilunaDiagnostico::teste_estado();
    if (action == "diagnostico.parar")
        return FilunaDiagnostico::parar();
    if (action == "diagnostico.presenca")
        return FilunaDiagnostico::presenca(d.value("dev_id", std::string()));
    if (action == "casa.regras")
        return FilunaCasaAvisos::regras();
    if (action == "casa.regra.salvar")
        return FilunaCasaAvisos::salvar_regra(d);
    if (action == "casa.testar")
        return FilunaCasaAvisos::testar(d);
    if (action == "casa.historico")
        return FilunaCasaAvisos::historico();
    if (action == "casa.evento") { // aviso montado na tela (prévia de um evento real, sem esperar a impressora)
        FilunaCasaAvisos::evento(d.value("evento", std::string("mensagem")), d.value("dados", json::object()));
        return {{"ok", true}};
    }
    if (action == "dispositivos.conectar")
        return FD::conectar(d.value("dev_id", std::string()), d.value("codigo", std::string()));
    if (action == "dispositivos.conectar_ip")
        return FD::conectar_ip(d.value("fabricante", std::string()), d.value("modelo", std::string()), d.value("ip", std::string()),
                               d.value("codigo", std::string()), d.value("nome", std::string()), d.value("serie", std::string()));
    // ---- Assistência remota (suporte, lista fechada): fatiador e impressão de teste ----
    if (action == "fatiador.estado")
        return FD::fatiador_estado();
    if (action == "fatiador.ajustar_impressora")
        return FD::fatiador_ajustar(d.value("dev_id", std::string()));
    if (action == "impressao.teste")
        return FilunaImpressao::impressao_teste(d.value("dev_id", std::string()), d.value("material", std::string("PLA")),
                                                d.contains("posicao") && d["posicao"].is_number_integer() ? d["posicao"].get<int>() : 1);
    if (action == "impressao.teste.estado")
        return FilunaImpressao::impressao_teste_estado();
    if (action == "dispositivos.ponte")
        return FD::ponte_instrucoes(d.value("fabricante", std::string()));
    if (action == "dispositivos.registrar")
        return FD::registrar(d.value("nome", std::string()), d.value("fabricante", std::string()), d.value("modelo", std::string()),
                             d.value("dev_id", std::string()));
    if (action == "dispositivos.estado") {
        json e = FD::estado();
        FD::sincronizar_estado(e, cfg->get("filuna_token")); // conta → servidor → app Android (no máximo a cada 25 s)
        return e;
    }
    if (action == "dispositivos.controle")
        return FD::controle(d.value("dev_id", std::string()), d);
    if (action == "dispositivos.comando")
        return FD::comando(d.value("dev_id", std::string()), d.value("cmd", std::string()));
    if (action == "dispositivos.selecionar")
        return FD::selecionar(d.value("dev_id", std::string()));
    if (action == "dispositivos.excluir")
        return FD::excluir(d.value("dev_id", std::string()), d.value("cadastro_id", std::string()));
    if (action == "dispositivos.esquecer") // reparo pelo suporte remoto
        return FD::esquecer(d.value("dev_id", std::string()));
    if (action == "dispositivos.reconectar") // reparo pelo suporte remoto: refaz a conexão com o código guardado
        return FD::conectar(d.value("dev_id", std::string()), std::string());
    if (action == "dispositivos.troca_vista")
        return FD::troca_vista(d.value("dev_id", std::string()));
    if (action == "dispositivos.mesa_veredito") {
        // resultado da conferência da mesa pela câmera; a janela de impressão consulta antes de enviar
        Filuna::set_mesa(d.value("dev_id", std::string()),
                         json{{"livre", d.value("livre", true)}, {"motivo", d.value("motivo", std::string())}, {"em", (long long) std::time(nullptr)}}.dump());
        return json{{"ok", true}};
    }
    if (action == "dispositivos.filamentos_projeto") {
        // mesma sincronização do botão da Preparação, pedida do cartão da impressora
        wxGetApp().CallAfter([] {
            if (wxGetApp().plater() != nullptr)
                wxGetApp().sidebar().sync_ams_list();
        });
        return json{{"ok", true}};
    }
    if (action == "dispositivos.observar") {
        // a tela pede estado ao vivo enquanto está aberta; para quando sai
        const bool ligar = d.value("ligado", false);
        if (ligar) {
            if (!m_estado_timer) {
                m_estado_timer = std::make_unique<wxTimer>();
                m_estado_timer->Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
                    json e = FD::estado();
                    FD::sincronizar_estado(e, wxGetApp().app_config->get("filuna_token"));
                    send_event("dispositivos.estado", e);
                });
            }
            m_estado_timer->Start(3000);
        } else if (m_estado_timer) {
            m_estado_timer->Stop();
        }
        return json::object();
    }
    if (action == "camera.abrir") {
        // câmera da impressora selecionada: a janela do Orca (Dispositivo/monitor) já faz o vídeo
        CallAfter([] {
            if (MainFrame* frame = wxGetApp().mainframe)
                frame->select_filuna_tab(TAB_ID_MONITOR);
        });
        return json::object();
    }
    if (action == "url.abrir") {
        const std::string url = d.value("url", std::string());
        if (url.rfind("https://", 0) != 0)
            throw std::runtime_error("Endereço inválido.");
        wxLaunchDefaultBrowser(wxString::FromUTF8(url));
        return json::object();
    }
    if (action == "projetos.recentes")
        return recent_projects();
    if (action == "projeto.abrir") {
        const std::string path = d.value("caminho", std::string());
        CallAfter([path] {
            Plater* plater = wxGetApp().plater();
            if (plater == nullptr)
                return;
            if (MainFrame* frame = wxGetApp().mainframe)
                frame->select_filuna_tab(TAB_ID_PREPARE);
            const wxString file = wxString::FromUTF8(path);
            if (file.Lower().EndsWith(".3mf"))
                plater->load_project(file);
            else
                plater->load_files(wxArrayString(1, &file));
        });
        return json::object();
    }
    if (action == "projeto.importar") {
        CallAfter([] {
            if (MainFrame* frame = wxGetApp().mainframe)
                frame->select_filuna_tab(TAB_ID_PREPARE);
            if (Plater* plater = wxGetApp().plater())
                plater->add_file();
        });
        return json::object();
    }
    if (action == "mesa.imagem")
        return plate_image();
    if (action == "imagens.escolher")
        return pick_images(this);
    if (action == "modelo.objetos")
        return model_objects();
    if (action == "modelo.reparar") {
        if (ObjectList* list = wxGetApp().obj_list())
            list->fix_all_through_cgal(false);
        return model_objects();
    }
    if (action == "preferencias.mudou") {
        const json values = d.contains("valores") ? d["valores"] : json::object();
        for (FilunaPanel* p : panels())
            if (p != this)
                p->send_event("preferencias", {{"valores", values}});
        return json::object();
    }
    throw std::runtime_error("Ação desconhecida: " + action);
}

}} // namespace Slic3r::GUI
