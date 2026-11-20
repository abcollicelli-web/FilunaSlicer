#include "FilunaAssistente.hpp"

#include "FilunaPanel.hpp"
#include "GLToolbar.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "GUI_ObjectList.hpp"
#include "MainFrame.hpp"
#include "PartPlate.hpp"
#include "Plater.hpp"
#include "Tab.hpp"
#include "Jobs/Worker.hpp"

#include "libslic3r/Model.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Format/ProjectStrategy.hpp"

#include "Widgets/WebView.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <wx/clipbrd.h>
#include <wx/dirdlg.h>
#include <wx/sizer.h>
#include <wx/uri.h>
#include <wx/webview.h>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <set>

namespace Slic3r { namespace GUI {

using nlohmann::json;
namespace fs = boost::filesystem;

namespace {

// Ajustes do processo que o assistente pode ler e mudar, com o intervalo aceito para os números. Fora desta lista
// nada muda, venha o pedido da tela ou da IA.
struct Ajuste
{
    const char* chave;
    double      minimo;
    double      maximo; // minimo == maximo: valor de lista (o Orca recusa o que não existir)
};
const Ajuste AJUSTES[] = {
    {"enable_support", 0, 0},          {"support_type", 0, 0},          {"support_threshold_angle", 0, 90},
    {"brim_type", 0, 0},               {"brim_width", 0, 20},           {"wall_loops", 1, 12},
    {"sparse_infill_density", 0, 100}, {"sparse_infill_pattern", 0, 0}, {"layer_height", 0.04, 0.8},
    {"top_shell_layers", 0, 30},       {"bottom_shell_layers", 0, 30},  {"initial_layer_print_height", 0.05, 0.8},
};

const Ajuste* ajuste(const std::string& chave)
{
    for (const Ajuste& a : AJUSTES)
        if (chave == a.chave)
            return &a;
    return nullptr;
}

Plater& mesa()
{
    Plater* plater = wxGetApp().plater();
    if (plater == nullptr)
        throw std::runtime_error("A Preparação ainda não abriu.");
    return *plater;
}

Tab& aba_processo()
{
    Tab* tab = wxGetApp().get_tab(Preset::TYPE_PRINT);
    if (tab == nullptr || tab->get_config() == nullptr)
        throw std::runtime_error("Os ajustes do processo ainda não abriram.");
    return *tab;
}

json ler_processo()
{
    const DynamicPrintConfig& cfg = *aba_processo().get_config();
    json valores = json::object();
    for (const Ajuste& a : AJUSTES)
        if (const ConfigOption* opt = cfg.option(a.chave))
            valores[a.chave] = opt->serialize();
    return valores;
}

// Muda os ajustes pedidos de uma vez (como se o cliente tivesse mudado na lateral) e devolve os valores de antes, para o
// Desfazer da tela.
json ajustar_processo(const json& pedidos)
{
    if (!pedidos.is_object() || pedidos.empty())
        throw std::runtime_error("Nenhum ajuste pedido.");
    Tab&               tab = aba_processo();
    DynamicPrintConfig mudancas;
    json               antes = json::object();
    for (const auto& [chave, bruto] : pedidos.items()) {
        const Ajuste* a = ajuste(chave);
        if (a == nullptr)
            throw std::runtime_error("O assistente não muda o ajuste \"" + chave + "\".");
        std::string valor = bruto.is_string() ? bruto.get<std::string>() : bruto.dump();
        if (bruto.is_boolean())
            valor = bruto.get<bool>() ? "1" : "0";
        if (a->minimo < a->maximo) {
            char*        fim = nullptr;
            const double n   = std::strtod(valor.c_str(), &fim);
            if (fim == valor.c_str() || n < a->minimo || n > a->maximo)
                throw std::runtime_error("Valor fora do aceito para \"" + chave + "\": " + valor);
        }
        mudancas.set_deserialize_strict(chave, valor);
        if (const ConfigOption* atual = tab.get_config()->option(chave))
            antes[chave] = atual->serialize();
    }
    tab.load_config(mudancas);
    mesa().on_config_change(*tab.get_config());
    return {{"antes", antes}, {"valores", ler_processo()}};
}

// Conferência da peça antes de fatiar: o que dá para ver só pela malha, sem fatiar.
json analisar_pecas()
{
    Plater&              plater  = mesa();
    PresetBundle&        bundle  = *wxGetApp().preset_bundle;
    const DynamicPrintConfig& impressora = bundle.printers.get_edited_preset().config;
    BoundingBoxf         area;
    if (const auto* pts = impressora.option<ConfigOptionPoints>("printable_area"))
        for (const Vec2d& p : pts->values)
            area.merge(p);
    const double altura_max = impressora.opt_float("printable_height");
    const DynamicPrintConfig& proc = *aba_processo().get_config();
    const bool   com_suporte = proc.opt_bool("enable_support");
    const bool   com_aba     = proc.option("brim_type") != nullptr && proc.option("brim_type")->serialize() != "no_brim";

    json lista = json::array();
    for (const ModelObject* obj : plater.model().objects) {
        if (obj->instances.empty())
            continue;
        TriangleMesh malha = obj->raw_mesh();
        malha.transform(obj->instances.front()->get_matrix());
        const indexed_triangle_set& its = malha.its;
        float z_min = std::numeric_limits<float>::max(), z_max = std::numeric_limits<float>::lowest();
        for (const Vec3f& v : its.vertices) {
            z_min = std::min(z_min, v.z());
            z_max = std::max(z_max, v.z());
        }
        double no_ar = 0., base = 0.;
        for (const Vec3i32& f : its.indices) {
            const Vec3f& a = its.vertices[f[0]];
            const Vec3f& b = its.vertices[f[1]];
            const Vec3f& c = its.vertices[f[2]];
            const Vec3f  n = (b - a).cross(c - a);
            const double area_face = 0.5 * n.norm();
            if (area_face <= 0.)
                continue;
            const double nz    = n.z() / (2. * area_face);
            const float  menor = std::min({a.z(), b.z(), c.z()});
            if (menor < z_min + 0.05f) {
                if (nz < -0.99)
                    base += area_face; // face deitada na mesa
            } else if (nz < -0.7071) {
                no_ar += area_face; // voltada para baixo, mais de 45° fora do prumo, longe da mesa
            }
        }
        const TriangleMeshStats  st      = obj->get_object_stl_stats();
        const BoundingBoxf3      caixa   = obj->instance_bounding_box(0);
        const double             altura  = z_max - z_min;
        const bool cabe = (!area.defined || (caixa.min.x() >= area.min.x() - 0.01 && caixa.min.y() >= area.min.y() - 0.01 &&
                                             caixa.max.x() <= area.max.x() + 0.01 && caixa.max.y() <= area.max.y() + 0.01)) &&
                          (altura_max <= 0. || altura <= altura_max + 0.01);
        lista.push_back({{"nome", obj->name},
                         {"tamanho_mm", {caixa.size().x(), caixa.size().y(), altura}},
                         {"bordas_abertas", st.open_edges},
                         {"reparados", st.repaired_errors.edges_fixed + st.repaired_errors.degenerate_facets + st.repaired_errors.facets_removed +
                                           st.repaired_errors.facets_reversed + st.repaired_errors.backwards_edges},
                         {"cabe_na_mesa", cabe},
                         {"area_no_ar_mm2", std::round(no_ar)},
                         {"area_base_mm2", std::round(base)},
                         // alta e de base estreita: tomba ou solta da mesa no meio da impressão
                         {"base_pequena", altura > 15. && altura > 3. * std::sqrt(std::max(base, 1.)) && base < 600.}});
    }
    return {{"pecas", lista},
            {"mesa_mm", area.defined ? json{area.size().x(), area.size().y(), altura_max} : json()},
            {"impressora", bundle.printers.get_edited_preset().name},
            {"com_suporte", com_suporte},
            {"com_aba", com_aba},
            {"processo", ler_processo()}};
}

json avisos_do_fatiamento()
{
    json        lista = json::array();
    std::set<std::string> vistos;
    auto juntar = [&](const std::vector<PrintStateBase::Warning>& ws) {
        for (const PrintStateBase::Warning& w : ws)
            if (w.current && !w.message.empty() && vistos.insert(w.message).second && lista.size() < 10)
                lista.push_back({{"texto", w.message}, {"grave", w.level == PrintStateBase::WarningLevel::CRITICAL}});
    };
    const Print& print = mesa().fff_print();
    for (int s = 0; s < int(psCount); ++s)
        juntar(print.step_state_with_warnings(PrintStep(s)).warnings);
    for (const PrintObject* obj : print.objects())
        for (int s = 0; s < int(posCount); ++s)
            juntar(obj->step_state_with_warnings(PrintObjectStep(s)).warnings);
    return {{"avisos", lista}};
}

// ---- Pasta automática: só arquivos de dentro da pasta que o próprio cliente escolheu.
fs::path pasta_escolhida()
{
    const std::string p = wxGetApp().app_config->get("filuna_pasta_auto");
    if (p.empty() || !fs::is_directory(fs::path(p)))
        throw std::runtime_error("Escolha a pasta automática primeiro.");
    return fs::path(p);
}

fs::path arquivo_da_pasta(const std::string& nome)
{
    if (nome.empty() || nome.find_first_of("/\\:") != std::string::npos || nome == "." || nome == "..")
        throw std::runtime_error("Nome de arquivo inválido.");
    return pasta_escolhida() / fs::path(nome);
}

bool arquivo_de_modelo(const fs::path& p)
{
    const std::string n = p.filename().string();
    for (const char* ext : {".stl", ".3mf", ".obj", ".step", ".stp"})
        if (boost::iends_with(n, ext) && !boost::iends_with(n, ".gcode.3mf"))
            return true;
    return false;
}

json listar_pasta()
{
    const std::string escolhida = wxGetApp().app_config->get("filuna_pasta_auto");
    if (escolhida.empty() || !fs::is_directory(fs::path(escolhida)))
        return {{"pasta", escolhida}, {"arquivos", json::array()}, {"ok", false}};
    const fs::path pasta = escolhida, prontos = pasta / "Prontos";
    json           lista = json::array();
    for (const fs::directory_entry& e : fs::directory_iterator(pasta)) {
        boost::system::error_code ec;
        if (!fs::is_regular_file(e.path(), ec) || !arquivo_de_modelo(e.path()))
            continue;
        const fs::path pronto = prontos / (e.path().stem().string() + ".gcode.3mf");
        lista.push_back({{"nome", e.path().filename().string()},
                         {"bytes", fs::file_size(e.path(), ec)},
                         {"quando", (long long) fs::last_write_time(e.path(), ec)},
                         {"pronto", fs::exists(pronto, ec) && fs::last_write_time(pronto, ec) >= fs::last_write_time(e.path(), ec)}});
    }
    return {{"pasta", escolhida}, {"arquivos", lista}, {"ok", true}};
}

json estado_do_fatiamento()
{
    Plater&    plater = mesa();
    PartPlate* plate  = plater.get_partplate_list().get_curr_plate();
    return {{"fatiando", plater.is_background_process_slicing()},
            {"ocupado", !plater.get_ui_job_worker().is_idle()},
            {"fatiado", plate != nullptr && plate->is_slice_result_valid()},
            {"objetos", plater.model().objects.size()}};
}

// Sites que a aba "Minha IA" abre e por onde o login deles passa. Outro endereço vai para o navegador do cliente.
bool site_de_ia(const wxString& url)
{
    const wxString host = wxURI(url).GetServer().Lower();
    for (const char* s : {"claude.ai", "claude.com", "anthropic.com", "chatgpt.com", "openai.com", "gemini.google.com", "accounts.google.com",
                          "google.com", "gstatic.com", "filuna3d.com", "appleid.apple.com", "login.live.com", "login.microsoftonline.com"})
        if (host == s || host.EndsWith(wxString(".") + s))
            return true;
    return false;
}

// Navegador comum (Edge), sem a marca do fatiador que o Orca põe no WebView: os sites das IAs tratam como navegador.
const char* NAVEGADOR_COMUM = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/142.0.0.0 "
                              "Safari/537.36 Edg/142.0.0.0";

FilunaAssistentePainel* g_painel = nullptr;

} // namespace

FilunaAssistentePainel::FilunaAssistentePainel(wxWindow* parent) : wxPanel(parent, wxID_ANY)
{
    SetBackgroundColour(*wxWHITE);
    m_tela   = new FilunaPanel(this, "assistente.html");
    auto* s  = new wxBoxSizer(wxVERTICAL);
    s->Add(m_tela, 1, wxEXPAND);
    SetSizer(s);
    g_painel = this;
}

FilunaAssistentePainel::~FilunaAssistentePainel()
{
    if (g_painel == this)
        g_painel = nullptr;
}

FilunaAssistentePainel* FilunaAssistentePainel::atual() { return g_painel; }

void FilunaAssistentePainel::abrir_ia(const wxString& url)
{
    if (m_ia == nullptr) {
        m_ia = WebView::CreateWebView(this, url);
        if (m_ia == nullptr)
            throw std::runtime_error("O navegador embutido não abriu neste computador. Use o botão de abrir no navegador.");
        m_ia->SetUserAgent(wxString::FromUTF8(NAVEGADOR_COMUM));
        m_ia->Bind(wxEVT_WEBVIEW_NEWWINDOW, &FilunaAssistentePainel::on_new_window, this);
        GetSizer()->Add(m_ia, 1, wxEXPAND);
    } else if (!url.empty()) {
        m_ia->LoadURL(url);
    }
    // a tela fica só com as abas e a barra de navegação em cima do site
    GetSizer()->GetItem(m_tela)->SetProportion(0);
    m_tela->SetMinSize(wxSize(-1, FromDIP(96)));
    m_ia->Show();
    m_ia_aberta = true;
    Layout();
}

void FilunaAssistentePainel::fechar_ia()
{
    if (m_ia != nullptr)
        m_ia->Hide();
    GetSizer()->GetItem(m_tela)->SetProportion(1);
    m_tela->SetMinSize(wxSize(-1, -1));
    m_ia_aberta = false;
    Layout();
}

void FilunaAssistentePainel::ia_comando(const std::string& comando)
{
    if (m_ia == nullptr)
        return;
    if (comando == "voltar" && m_ia->CanGoBack())
        m_ia->GoBack();
    else if (comando == "recarregar")
        m_ia->Reload();
    else if (comando == "navegador")
        wxLaunchDefaultBrowser(m_ia->GetCurrentURL());
}

// Login com Google, conectores e janelas de autorização abrem no mesmo painel; o que não for da IA nem da Filuna vai
// para o navegador do cliente.
void FilunaAssistentePainel::on_new_window(wxWebViewEvent& e)
{
    const wxString url = e.GetURL();
    e.Veto();
    if (site_de_ia(url))
        m_ia->LoadURL(url);
    else if (!url.empty())
        wxLaunchDefaultBrowser(url);
}

void filuna_assistente_fatiado(bool ok)
{
    wxGetApp().CallAfter([ok] { FilunaPanel::broadcast("fatiado", {{"ok", ok}}); });
}

std::optional<json> filuna_assistente_acao(const std::string& acao, const json& d)
{
    if (acao == "assistente.abrir" || acao == "assistente.fechar") {
        const bool mostrar = acao == "assistente.abrir";
        const int  etapa   = d.value("etapa", 0);
        wxGetApp().CallAfter([mostrar, etapa] {
            // Na Preparação ou na Visualização o painel abre ali mesmo; de outra aba, volta para a Preparação.
            if (mostrar && wxGetApp().mainframe != nullptr && wxGetApp().plater() != nullptr && !wxGetApp().plater()->IsShownOnScreen())
                wxGetApp().mainframe->select_filuna_tab(TAB_ID_PREPARE);
            if (Plater* plater = wxGetApp().plater())
                plater->mostrar_assistente(mostrar);
            FilunaPanel::broadcast("assistente", {{"aberto", mostrar}, {"etapa", etapa}});
        });
        return json::object();
    }
    if (acao == "assistente.avisar") {
        // a tela do assistente conta às outras (rodapé) em que etapa o cliente está
        FilunaPanel::broadcast("assistente.etapas", d);
        return json::object();
    }
    if (acao == "ia.abrir") {
        const wxString url = wxString::FromUTF8(d.value("url", std::string()));
        if (!url.StartsWith("https://") || !site_de_ia(url))
            throw std::runtime_error("Endereço de IA não aceito.");
        FilunaAssistentePainel* p = FilunaAssistentePainel::atual();
        if (p == nullptr)
            throw std::runtime_error("Abra o Assistente primeiro.");
        p->abrir_ia(url);
        return json::object();
    }
    if (acao == "ia.fechar") {
        if (FilunaAssistentePainel* p = FilunaAssistentePainel::atual())
            p->fechar_ia();
        return json::object();
    }
    if (acao == "ia.comando") {
        if (FilunaAssistentePainel* p = FilunaAssistentePainel::atual())
            p->ia_comando(d.value("comando", std::string()));
        return json::object();
    }
    if (acao == "texto.copiar") {
        const std::string texto = d.value("texto", std::string());
        if (texto.size() > 4000)
            throw std::runtime_error("Texto grande demais.");
        if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(wxString::FromUTF8(texto)));
            wxTheClipboard->Close();
        }
        return json::object();
    }
    if (acao == "assistente.estado")
        return json{{"aberto", wxGetApp().plater() != nullptr && wxGetApp().plater()->assistente_visivel()},
                    {"ia_aberta", FilunaAssistentePainel::atual() != nullptr && FilunaAssistentePainel::atual()->ia_aberta()}};
    if (acao == "peca.analise")
        return analisar_pecas();
    if (acao == "processo.ler")
        return json{{"valores", ler_processo()}};
    if (acao == "processo.ajustar")
        return ajustar_processo(d.contains("valores") ? d["valores"] : json::object());
    if (acao == "peca.orientar" || acao == "peca.arrumar") {
        if (mesa().model().objects.empty())
            throw std::runtime_error("Coloque um modelo na mesa primeiro.");
        const bool orientar = acao == "peca.orientar";
        wxGetApp().CallAfter([orientar] {
            if (Plater* plater = wxGetApp().plater())
                orientar ? plater->orient() : plater->arrange();
        });
        return json::object();
    }
    if (acao == "fatiamento.estado")
        return estado_do_fatiamento();
    if (acao == "fatiamento.avisos")
        return avisos_do_fatiamento();
    if (acao == "fatiar" || acao == "imprimir") {
        if (mesa().model().objects.empty())
            throw std::runtime_error("Coloque um modelo na mesa primeiro.");
        const bool fatiar = acao == "fatiar";
        wxGetApp().CallAfter([fatiar] {
            if (Plater* plater = wxGetApp().plater())
                wxPostEvent(plater, SimpleEvent(fatiar ? EVT_GLTOOLBAR_SLICE_PLATE : EVT_GLTOOLBAR_PRINT_PLATE));
        });
        return json::object();
    }
    if (acao == "pasta.escolher") {
        wxDirDialog dlg(wxGetApp().mainframe, _L("Choose a folder"), from_u8(wxGetApp().app_config->get("filuna_pasta_auto")),
                        wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
        if (dlg.ShowModal() == wxID_OK)
            wxGetApp().app_config->set("filuna_pasta_auto", into_u8(dlg.GetPath()));
        return listar_pasta();
    }
    if (acao == "pasta.listar")
        return listar_pasta();
    if (acao == "pasta.abrir") {
        // Troca a mesa pelo arquivo da pasta. A tela só pede isto depois que o cliente ligou a pasta e confirmou.
        const fs::path arq = arquivo_da_pasta(d.value("nome", std::string()));
        if (!fs::is_regular_file(arq) || !arquivo_de_modelo(arq))
            throw std::runtime_error("Arquivo não encontrado na pasta.");
        Plater& plater = mesa();
        plater.new_project(true, true);
        if (plater.load_files(std::vector<fs::path>{arq}, LoadStrategy::LoadModel).empty())
            throw std::runtime_error("Não consegui abrir " + arq.filename().string() + ".");
        if (ObjectList* list = wxGetApp().obj_list())
            list->fix_all_through_cgal(false);
        return analisar_pecas();
    }
    if (acao == "pasta.salvar") {
        // A mesa fatiada vira <pasta>/Prontos/<nome>.gcode.3mf, que o cliente envia para a impressora quando quiser.
        const fs::path arq = arquivo_da_pasta(d.value("nome", std::string()));
        Plater&        plater = mesa();
        PartPlate*     plate  = plater.get_partplate_list().get_curr_plate();
        if (plate == nullptr || !plate->is_slice_result_valid())
            throw std::runtime_error("Fatie a mesa antes de salvar.");
        const fs::path prontos = arq.parent_path() / "Prontos";
        fs::create_directories(prontos);
        const fs::path saida = prontos / (arq.stem().string() + ".gcode.3mf");
        const SaveStrategy jeito = SaveStrategy::Silence | SaveStrategy::SplitModel | SaveStrategy::WithGcode | SaveStrategy::SkipModel;
        if (plater.export_3mf(saida, jeito, plater.get_partplate_list().get_curr_plate_index()) < 0)
            throw std::runtime_error("Não consegui salvar " + saida.filename().string() + ".");
        return json{{"arquivo", saida.filename().string()}};
    }
    return std::nullopt;
}

}} // namespace Slic3r::GUI
