#include "FilunaPanel.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <stdexcept>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>

#include <wx/sizer.h>
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
#include "Plater.hpp"
#include "Tab.hpp"
#include "WebViewDialog.hpp"
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
    if (const auto* model = printer.config.option<ConfigOptionString>("printer_model"))
        r["modelo"] = model->value;
    r["bico"] = first_value<ConfigOptionFloats>(printer.config, "nozzle_diameter");
    return r;
}

json filament_info()
{
    json r = printer_info();
    PresetBundle* bundle = wxGetApp().preset_bundle;
    if (bundle == nullptr)
        return r;
    const Preset& filament = bundle->filaments.get_edited_preset();
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

json slice_summary()
{
    json r = {{"fatiado", false}, {"objetos", 0}};
    Plater* plater = wxGetApp().plater();
    if (plater == nullptr)
        return r;
    r["objetos"] = plater->model().objects.size();
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

} // namespace

FilunaPanel::FilunaPanel(wxWindow* parent, const std::string& page) : wxPanel(parent, wxID_ANY)
{
    SetBackgroundColour(wxColour(0xED, 0xEB, 0xE7));
    m_url = file_url_from_path(boost::filesystem::path(resources_dir()) / "web" / "filuna" / page);
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

void FilunaPanel::account_changed(const std::string& token, const FilunaPanel* source)
{
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
    const std::string id     = msg.value("id", std::string());
    const std::string action = msg.value("acao", std::string());
    const json data          = msg.contains("dados") && msg["dados"].is_object() ? msg["dados"] : json::object();
    try {
        reply(id, true, handle(action, data));
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "Filuna bridge " << action << ": " << e.what();
        reply(id, false, {{"erro", e.what()}});
    }
}

json FilunaPanel::handle(const std::string& action, const json& d)
{
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
    if (action == "secao.pendente") {
        const std::string section = pending_section();
        pending_section().clear();
        return {{"secao", section}};
    }
    if (action == "perfis.salvos") {
        // The user's own process presets (the ones made with "Salvar" in Preparação), for Meu espaço.
        json list = json::array();
        if (PresetBundle* bundle = wxGetApp().preset_bundle) {
            const std::string current = bundle->prints.get_selected_preset_name();
            for (const Preset& preset : bundle->prints)
                if (preset.is_user() && preset.is_visible)
                    list.push_back({{"nome", preset.name}, {"base", preset.inherits()}, {"atual", preset.name == current}});
        }
        return {{"perfis", list}};
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
