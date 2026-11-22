#include "FilunaAssistente.hpp"

#include "FilunaPanel.hpp"
#include "GLCanvas3D.hpp"
#include "GLToolbar.hpp"
#include "Selection.hpp"
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
#include <wx/dialog.h>
#include <wx/dirdlg.h>
#include <wx/sizer.h>
#include <wx/uri.h>
#include <wx/webview.h>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <locale>
#include <set>
#include <sstream>

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

// Ajuste que nunca muda por aqui: o que roda programa ou escreve G-code próprio no computador do cliente.
bool ajuste_travado(const std::string& chave)
{
    return chave.find("gcode") != std::string::npos || chave.find("post_process") != std::string::npos ||
           chave == "filename_format" || chave.find("script") != std::string::npos;
}

// Muda os ajustes pedidos de uma vez (como se o cliente tivesse mudado na lateral) e devolve os valores de antes, para o
// Desfazer da tela. Vale qualquer ajuste do Processo ou do Filamento (temperatura, velocidade, ventilador, retração…);
// os da lista AJUSTES ainda têm o intervalo conferido; os outros, o próprio Orca recusa se o valor não servir.
json ajustar_processo(const json& pedidos)
{
    if (!pedidos.is_object() || pedidos.empty())
        throw std::runtime_error("Nenhum ajuste pedido.");
    const auto&        do_processo = Preset::print_options();
    const auto&        do_filamento = Preset::filament_options();
    Tab*               tabs[2]      = {&aba_processo(), wxGetApp().get_tab(Preset::TYPE_FILAMENT)};
    DynamicPrintConfig mudancas[2];
    json               antes = json::object();
    for (const auto& [chave, bruto] : pedidos.items()) {
        if (ajuste_travado(chave))
            throw std::runtime_error("Por segurança o assistente não muda \"" + chave + "\".");
        const int qual = std::find(do_processo.begin(), do_processo.end(), chave) != do_processo.end()   ? 0
                         : std::find(do_filamento.begin(), do_filamento.end(), chave) != do_filamento.end() ? 1
                                                                                                        : -1;
        if (qual < 0 || tabs[qual] == nullptr || tabs[qual]->get_config() == nullptr)
            throw std::runtime_error("Ajuste desconhecido no Processo e no Filamento: \"" + chave + "\".");
        std::string valor = bruto.is_string() ? bruto.get<std::string>() : bruto.dump();
        if (bruto.is_boolean())
            valor = bruto.get<bool>() ? "1" : "0";
        if (const Ajuste* a = ajuste(chave); a != nullptr && a->minimo < a->maximo) {
            // número com ponto, sem depender do idioma do Windows (com vírgula decimal, strtod lia "0.24" como 0)
            std::istringstream ler(valor);
            ler.imbue(std::locale::classic());
            double n = 0.;
            ler >> n;
            if (ler.fail() || n < a->minimo || n > a->maximo)
                throw std::runtime_error("Valor fora do aceito para \"" + chave + "\": " + valor);
        }
        const ConfigOption* atual = tabs[qual]->get_config()->option(chave);
        if (atual == nullptr)
            throw std::runtime_error("Ajuste \"" + chave + "\" não existe neste perfil.");
        // listas por extrusora (temperatura, fluxo…): um valor só vale para todas
        if (atual->is_vector() && valor.find(',') == std::string::npos) {
            const size_t n = static_cast<const ConfigOptionVectorBase*>(atual)->size();
            std::string  v = valor;
            for (size_t i = 1; i < n; ++i)
                v += "," + valor;
            valor = v;
        }
        try {
            mudancas[qual].set_deserialize_strict(chave, valor);
        } catch (const std::exception&) {
            throw std::runtime_error("Valor não aceito para \"" + chave + "\": " + valor);
        }
        antes[chave] = atual->serialize();
    }
    for (int i = 0; i < 2; ++i)
        if (!mudancas[i].empty()) {
            tabs[i]->load_config(mudancas[i]);
            mesa().on_config_change(*tabs[i]->get_config());
        }
    return {{"antes", antes}, {"valores", ler_processo()}};
}

// Troca impressora, filamento (primeira entrada) ou perfil de processo pelo nome do perfil, como na lateral.
json trocar_perfil(const std::string& tipo, const std::string& nome)
{
    const Preset::Type t = tipo == "impressora" ? Preset::TYPE_PRINTER : tipo == "filamento" ? Preset::TYPE_FILAMENT
                         : tipo == "processo"   ? Preset::TYPE_PRINT
                                                : Preset::TYPE_INVALID;
    if (t == Preset::TYPE_INVALID)
        throw std::runtime_error("Tipo de perfil: impressora, filamento ou processo.");
    PresetCollection& col = t == Preset::TYPE_PRINTER ? wxGetApp().preset_bundle->printers
                          : t == Preset::TYPE_FILAMENT ? wxGetApp().preset_bundle->filaments
                                                       : wxGetApp().preset_bundle->prints;
    const Preset* p = col.find_preset(nome, false);
    if (p == nullptr || !p->is_visible)
        throw std::runtime_error("Perfil não encontrado: " + nome);
    Tab* tab = wxGetApp().get_tab(t);
    if (tab == nullptr)
        throw std::runtime_error("Os perfis ainda não abriram.");
    tab->select_preset(nome);
    return {{"ok", true}, {"tipo", tipo}, {"nome", nome}};
}

// Perfis que dá para escolher agora (os compatíveis com a impressora atual), para a IA trocar pelo nome.
json perfis_disponiveis()
{
    PresetBundle& b     = *wxGetApp().preset_bundle;
    auto          lista = [](const PresetCollection& col, bool so_compat) {
        json l = json::array();
        for (const Preset& p : col)
            if (p.is_visible && (!so_compat || p.is_compatible) && l.size() < 60)
                l.push_back(p.name);
        return l;
    };
    return {{"impressora_atual", b.printers.get_edited_preset().name},
            {"filamento_atual", b.filaments.get_edited_preset().name},
            {"processo_atual", b.prints.get_edited_preset().name},
            {"impressoras", lista(b.printers, false)},
            {"filamentos", lista(b.filaments, true)},
            {"processos", lista(b.prints, true)}};
}

// Cópias que não estão inteiras dentro de nenhuma mesa, pela regra do fatiador. No projeto cada mesa tem a sua posição.
int copias_fora_das_mesas(PartPlateList& mesas, const ModelObject& obj, int indice)
{
    if (!obj.printable)
        return 0;
    int fora = 0;
    for (int j = 0; j < int(obj.instances.size()); ++j) {
        if (!obj.instances[j]->printable)
            continue;
        BoundingBoxf3 caixa  = obj.instance_convex_hull_bounding_box(j);
        bool          dentro = false;
        for (int m = 0; m < mesas.get_plate_count() && !dentro; ++m)
            dentro = !mesas.get_plate(m)->check_outside(indice, j, &caixa);
        if (!dentro)
            ++fora;
    }
    return fora;
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
    for (int indice = 0; indice < int(plater.model().objects.size()); ++indice) {
        const ModelObject* obj = plater.model().objects[indice];
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
        const int fora = copias_fora_das_mesas(plater.get_partplate_list(), *obj, indice);
        lista.push_back({{"nome", obj->name},
                         {"indice", indice},
                         {"copias_fora", fora},
                         {"tamanho_mm", {caixa.size().x(), caixa.size().y(), altura}},
                         {"bordas_abertas", st.open_edges},
                         {"reparados", st.repaired_errors.edges_fixed + st.repaired_errors.degenerate_facets + st.repaired_errors.facets_removed +
                                           st.repaired_errors.facets_reversed + st.repaired_errors.backwards_edges},
                         {"cabe_na_mesa", fora == 0},
                         {"area_no_ar_mm2", std::round(no_ar)},
                         {"area_base_mm2", std::round(base)},
                         // alta e de base estreita: tomba ou solta da mesa no meio da impressão
                         {"base_pequena", altura > 15. && altura > 3. * std::sqrt(std::max(base, 1.)) && base < 600.},
                         {"escala_porcento", std::round(obj->instances.front()->get_scaling_factor().x() * 1000.) / 10.},
                         {"copias", obj->instances.size()}});
    }
    return {{"pecas", lista},
            {"mesa_mm", area.defined ? json{area.size().x(), area.size().y(), altura_max} : json()},
            {"impressora", bundle.printers.get_edited_preset().name},
            {"com_suporte", com_suporte},
            {"com_aba", com_aba},
            {"processo", ler_processo()}};
}

// Orienta só a peça pedida ou, sem peça, só as da mesa aberta. O orientar do Orca gira o projeto inteiro quando a seleção
// está vazia, então sem seleção confirmada nada roda.
void orientar_pecas(Plater& plater, int peca)
{
    const ModelObjectPtrs& objetos = plater.model().objects;
    PartPlate*             aberta  = plater.get_partplate_list().get_curr_plate();
    std::vector<int>       alvo;
    for (int i = 0; i < int(objetos.size()); ++i)
        for (int j = 0; j < int(objetos[i]->instances.size()); ++j)
            if (peca >= 0 ? i == peca : aberta->contain_instance(i, j)) {
                alvo.push_back(i);
                break;
            }
    Selection& selecao = plater.canvas3D()->get_selection();
    selecao.remove_all();
    for (const int i : alvo)
        selecao.add_object(unsigned(i), false);
    if (alvo.empty() || selecao.get_content().size() != alvo.size()) {
        BOOST_LOG_TRIVIAL(warning) << "Assistente: orientar sem peça selecionada; nada foi girado";
        return;
    }
    plater.set_prepare_state(Job::JobPrepareState::PREPARE_STATE_DEFAULT);
    plater.orient();
}

// Mexe nas peças da mesa como o cliente mexeria (cada comando vira um passo do Desfazer, Ctrl+Z):
//   escalar  {fator} ou {maior_lado_mm}   girar {eixo: x|y|z, graus}   copias {quantidade: total de cópias}
// "peca" = índice da peça (0 = primeira); ausente ou -1 = todas.
json comando_na_mesa(const json& d)
{
    Plater&            plater = mesa();
    Model&             model  = plater.model();
    const std::string  acao   = d.value("acao", std::string());
    const int          peca   = d.value("peca", -1);
    if (model.objects.empty())
        throw std::runtime_error("Coloque um modelo na mesa primeiro.");
    if (peca >= int(model.objects.size()))
        throw std::runtime_error("Essa peça não existe na mesa.");
    std::vector<size_t> alvos;
    for (size_t i = 0; i < model.objects.size(); ++i)
        if (peca < 0 || int(i) == peca)
            alvos.push_back(i);

    if (acao == "escalar") {
        plater.take_snapshot("Assistente: escala");
        for (size_t i : alvos) {
            ModelObject* obj = model.objects[i];
            double       f   = d.value("fator", 0.);
            if (d.contains("maior_lado_mm")) {
                const Vec3d  t     = obj->instance_bounding_box(0).size();
                const double maior = std::max({t.x(), t.y(), t.z()});
                if (maior <= 0.)
                    continue;
                f = d.value("maior_lado_mm", 0.) / maior;
            }
            if (!(f >= 0.05 && f <= 20.))
                throw std::runtime_error("Escala fora do aceito (de 5% a 2000%).");
            for (ModelInstance* inst : obj->instances)
                inst->set_scaling_factor(inst->get_scaling_factor() * f);
            obj->invalidate_bounding_box();
            obj->ensure_on_bed();
        }
        plater.changed_objects(alvos);
    } else if (acao == "girar") {
        const std::string eixo  = d.value("eixo", std::string("z"));
        const double      graus = d.value("graus", 0.);
        if (std::abs(graus) > 360. || (eixo != "x" && eixo != "y" && eixo != "z"))
            throw std::runtime_error("Giro inválido: eixo x, y ou z e até 360 graus.");
        const Vec3d   vetor = eixo == "x" ? Vec3d::UnitX() : eixo == "y" ? Vec3d::UnitY() : Vec3d::UnitZ();
        const Matrix3d rot  = Eigen::AngleAxisd(graus * M_PI / 180., vetor).toRotationMatrix();
        plater.take_snapshot("Assistente: girar");
        for (size_t i : alvos) {
            ModelObject* obj = model.objects[i];
            for (ModelInstance* inst : obj->instances)
                inst->rotate(rot);
            obj->invalidate_bounding_box();
            obj->ensure_on_bed();
        }
        plater.changed_objects(alvos);
    } else if (acao == "copias") {
        const int total = d.value("quantidade", 1);
        if (total < 1 || total > 50)
            throw std::runtime_error("Quantidade de cópias de 1 a 50.");
        for (size_t i : alvos) {
            const int atual = int(model.objects[i]->instances.size());
            if (total == atual)
                continue;
            plater.canvas3D()->get_selection().add_object(unsigned(i), true);
            if (total > atual)
                plater.increase_instances(size_t(total - atual));
            else
                plater.decrease_instances(size_t(atual - total));
        }
    } else if (acao == "espelhar") {
        const std::string eixo = d.value("eixo", std::string("x"));
        if (eixo != "x" && eixo != "y" && eixo != "z")
            throw std::runtime_error("Espelhar: eixo x, y ou z.");
        plater.take_snapshot("Assistente: espelhar");
        for (size_t i : alvos) {
            model.objects[i]->mirror(eixo == "x" ? X : eixo == "y" ? Y : Z);
            model.objects[i]->invalidate_bounding_box();
            model.objects[i]->ensure_on_bed();
        }
        plater.changed_objects(alvos);
    } else if (acao == "remover") {
        if (peca < 0)
            throw std::runtime_error("Diga qual peça remover (índice).");
        plater.remove(size_t(peca));
    } else if (acao == "desfazer") {
        plater.undo();
    } else {
        throw std::runtime_error("Comando desconhecido: " + acao);
    }
    return analisar_pecas();
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
    // os que a Plater mostra na tela (ex.: "balanço flutuante", que vem do passo de suporte e não ficava na lista)
    for (const std::string& texto : mesa().avisos_do_fatiamento())
        if (vistos.insert(texto).second && lista.size() < 10)
            lista.push_back({{"texto", texto}, {"grave", false}});
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
        filuna_ia_preparar_navegador(m_ia);
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

// Janelinha de login (Google, Apple, autorização do conector): abre como janela de verdade, ligada à página que a
// pediu. O login com Google do Claude responde à página de origem por essa ligação; carregar a janelinha no próprio
// painel (como era) perdia a resposta e a tela ficava em branco. O que não for da IA nem da Filuna vai para o navegador
// do cliente.
static void ia_nova_janela(wxWebViewEvent& e)
{
    const wxString url = e.GetURL();
    if (site_de_ia(url) || url.empty() || url == "about:blank")
        return; // segue para wxEVT_WEBVIEW_NEWWINDOW_FEATURES, que cria a janela
    e.Veto();
    wxLaunchDefaultBrowser(url);
}

static void ia_janela_de_login(wxWebViewEvent& e)
{
    wxWebViewWindowFeatures* features = e.GetTargetWindowFeatures();
    if (features == nullptr)
        return;
    auto* janela = new wxDialog(wxGetApp().mainframe, wxID_ANY, wxString::FromUTF8("Entrar na sua IA"), wxDefaultPosition,
                                wxGetApp().mainframe->FromDIP(wxSize(520, 700)), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    wxWebView* filho = features->GetChildWebView();
    if (filho == nullptr || !filho->Create(janela, wxID_ANY, e.GetURL())) {
        janela->Destroy();
        return;
    }
    filho->SetUserAgent(wxString::FromUTF8(NAVEGADOR_COMUM));
    if (features->GetSize().IsFullySpecified())
        janela->SetClientSize(janela->FromDIP(features->GetSize()));
    auto* s = new wxBoxSizer(wxVERTICAL);
    s->Add(filho, 1, wxEXPAND);
    janela->SetSizer(s);
    // a página fecha a janelinha (window.close) quando o login termina
    filho->Bind(wxEVT_WEBVIEW_WINDOW_CLOSE_REQUESTED, [janela](wxWebViewEvent&) { janela->CallAfter([janela] { janela->Destroy(); }); });
    filho->Bind(wxEVT_WEBVIEW_NEWWINDOW, [filho](wxWebViewEvent& ev) {
        ev.Veto();
        if (site_de_ia(ev.GetURL()))
            filho->LoadURL(ev.GetURL());
        else if (!ev.GetURL().empty())
            wxLaunchDefaultBrowser(ev.GetURL());
    });
    janela->Bind(wxEVT_CLOSE_WINDOW, [janela](wxCloseEvent&) { janela->Destroy(); });
    janela->CentreOnParent();
    janela->Show();
}

void filuna_ia_preparar_navegador(wxWebView* web)
{
    web->SetUserAgent(wxString::FromUTF8(NAVEGADOR_COMUM));
    web->Bind(wxEVT_WEBVIEW_NEWWINDOW, &ia_nova_janela);
    web->Bind(wxEVT_WEBVIEW_NEWWINDOW_FEATURES, &ia_janela_de_login);
}

bool filuna_site_de_ia(const wxString& url) { return site_de_ia(url); }

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
    if (acao == "mesa.comando")
        return comando_na_mesa(d);
    if (acao == "perfil.trocar")
        return trocar_perfil(d.value("tipo", std::string()), d.value("nome", std::string()));
    if (acao == "perfil.lista")
        return perfis_disponiveis();
    if (acao == "processo.ler")
        return json{{"valores", ler_processo()}};
    if (acao == "processo.ajustar")
        return ajustar_processo(d.contains("valores") ? d["valores"] : json::object());
    if (acao == "peca.orientar" || acao == "peca.arrumar") {
        if (mesa().model().objects.empty())
            throw std::runtime_error("Coloque um modelo na mesa primeiro.");
        const bool orientar = acao == "peca.orientar";
        const int  peca     = d.value("peca", -1);
        if (orientar && (peca < -1 || peca >= int(mesa().model().objects.size())))
            throw std::runtime_error("Essa peça não está mais no projeto.");
        wxGetApp().CallAfter([orientar, peca] {
            if (Plater* plater = wxGetApp().plater()) {
                if (orientar)
                    orientar_pecas(*plater, peca);
                else
                    plater->arrange();
            }
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
