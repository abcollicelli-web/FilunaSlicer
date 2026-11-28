#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "FilunaAbertos.hpp"

#include "FilunaIdioma.hpp"
#include "FilunaImpressao.hpp"
#include "GLCanvas3D.hpp"
#include "GLToolbar.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "IMToolbar.hpp"
#include "ImGuiWrapper.hpp"
#include "MainFrame.hpp"
#include "MsgDialog.hpp"
#include "NotificationManager.hpp"
#include "PartPlate.hpp"
#include "Plater.hpp"
#include "Tab.hpp"
#include "Jobs/Worker.hpp"

#include "libslic3r/Format/bbs_3mf.hpp"
#include "libslic3r/GCode/ThumbnailData.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r_version.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <ctime>

#include <wx/timer.h>
#include <wx/utils.h>

namespace fs = boost::filesystem;

namespace Slic3r { namespace GUI { namespace FilunaAbertos {

namespace {

// Um arquivo da lista. Os campos de projeto valem para os que não estão na tela; o que está na tela é lido do Plater.
struct Aberto
{
    unsigned    id{ 0 };
    std::string temporario;  // instantâneo .3mf; vazio = projeto vazio
    std::string projeto;     // arquivo do projeto no disco (vazio = sem título)
    std::string nome;
    bool        modificado{ false };
    bool        protegido{ false };  // modelo do catálogo Filuna (só imprimir)
    bool        fatiado{ false };
    bool        na_visualizacao{ false };

    std::shared_ptr<const ThumbnailData> miniatura;
    unsigned                             versao_miniatura{ 0 };
    unsigned                             versao_prato{ unsigned(-1) };
    bool                                 miniatura_viva{ false };
};

std::vector<Aberto> g_abertos;
size_t              g_ativo{ 0 };
unsigned            g_proximo_id{ 1 };
bool                g_trocando{ false };
wxArrayString       g_pendentes;
wxTimer*            g_espera{ nullptr };
bool                g_avisou_espera{ false };

constexpr ImU32 MAGENTA = IM_COL32(0xE0, 0x18, 0x6B, 255);
constexpr ImU32 VIOLETA = IM_COL32(0x6D, 0x4A, 0xE8, 255);
constexpr ImU32 PAPEL   = IM_COL32(0xF6, 0xF2, 0xEB, 255);

fs::path pasta()
{
    return fs::path(data_dir()) / "filuna" / "abertos";
}

// Os temporários de execuções que já acabaram (queda, Windows desligado) somem depois de alguns dias.
void limpar_antigos()
{
    boost::system::error_code ec;
    const fs::path dir = pasta();
    if (!fs::is_directory(dir, ec))
        return;
    const std::time_t limite = std::time(nullptr) - 3 * 24 * 3600;
    for (fs::directory_iterator it(dir, ec), fim; !ec && it != fim; it.increment(ec)) {
        if (fs::is_regular_file(it->path(), ec) && fs::last_write_time(it->path(), ec) < limite)
            fs::remove(it->path(), ec);
    }
}

void garantir()
{
    if (!g_abertos.empty())
        return;
    limpar_antigos();
    Aberto a;
    a.id = g_proximo_id++;
    g_abertos.push_back(std::move(a));
    g_ativo = 0;
}

Plater* plater() { return wxGetApp().plater(); }

std::string nome_atual()
{
    std::string nome = into_u8(plater()->get_project_name());
    return nome.empty() ? Filuna::tr("Sem título") : nome;
}

std::string projeto_atual() { return into_u8(plater()->get_project_filename(".3mf")); }

bool atual_vazio()
{
    Plater* p = plater();
    return p->model().objects.empty() && !p->only_gcode_mode() && !p->is_gcode_3mf();
}

// Mudança que se perde ao fechar sem salvar (a mesma conta da pergunta do Orca, mais os perfis alterados).
bool atual_modificado()
{
    Plater* p = plater();
    if (FilunaImpressao::modelo_protegido() || p->only_gcode_mode() || p->is_gcode_3mf())
        return false;  // não se salva: nada a perder
    if (p->model().objects.empty() && projeto_atual().empty())
        return false;
    return p->is_project_dirty() || !p->up_to_date(false, false);
}

void redesenhar()
{
    if (Plater* p = plater()) {
        for (GLCanvas3D* c : { p->get_view3D_canvas3D(), p->get_preview_canvas3D() }) {
            if (c != nullptr) {
                c->set_as_dirty();
                c->request_extra_frame();
            }
        }
    }
}

void avisar(const std::string& texto)
{
    if (Plater* p = plater())
        p->get_notification_manager()->push_notification(texto);
}

// Pode trocar de projeto agora (nada carregando, nenhuma janela modal, nenhuma ferramenta pintando).
bool livre()
{
    GUI_App& app = wxGetApp();
    if (app.is_closing() || app.mainframe == nullptr || app.plater() == nullptr || g_trocando)
        return false;
    Plater* p = app.plater();
    if (p->is_loading_project() || !app.mainframe->IsEnabled() || !p->get_ui_job_worker().is_idle())
        return false;
    if (GLCanvas3D* c = p->get_view3D_canvas3D(); c != nullptr && c->get_gizmos_manager().is_in_editing_mode(false))
        return false;
    return true;
}

int perguntar_salvar(const std::string& nome)
{
    MessageDialog dlg(wxGetApp().mainframe,
                      Filuna::trw_valores("“{0}” tem mudanças que não foram salvas. Salvar antes de fechar?", { nome }),
                      wxString(SLIC3R_APP_FULL_NAME) + " - " + Filuna::trw("Arquivos abertos"),
                      wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxCENTRE);
    dlg.SetButtonLabel(wxID_YES, Filuna::trw("Salvar"));
    dlg.SetButtonLabel(wxID_NO, Filuna::trw("Descartar"));
    dlg.SetButtonLabel(wxID_CANCEL, Filuna::trw("Cancelar"));
    return dlg.ShowModal();
}

void apagar_temporario(Aberto& a)
{
    if (a.temporario.empty())
        return;
    boost::system::error_code ec;
    fs::remove(fs::path(a.temporario), ec);
    a.temporario.clear();
}

// Guarda o projeto da tela no item ativo (instantâneo 3MF). false = não conseguiu (o projeto continua na tela).
bool guardar_atual()
{
    Plater* p = plater();
    Aberto& a = g_abertos[g_ativo];
    p->exit_gizmo();

    a.projeto         = projeto_atual();
    a.nome            = nome_atual();
    a.modificado      = atual_modificado();
    a.protegido       = FilunaImpressao::modelo_protegido();
    PartPlate* prato  = p->get_partplate_list().get_curr_plate();
    a.fatiado         = prato != nullptr && prato->is_slice_result_valid();
    a.na_visualizacao = p->is_preview_shown();

    if (atual_vazio()) {
        apagar_temporario(a);
        a.miniatura.reset();
        ++a.versao_miniatura;
        return true;
    }

    boost::system::error_code ec;
    fs::create_directories(pasta(), ec);
    const fs::path arquivo = pasta() / (std::to_string(wxGetProcessId()) + "-" + std::to_string(a.id) + ".3mf");
    const SaveStrategy jeito = SaveStrategy::SplitModel | SaveStrategy::ShareMesh | SaveStrategy::FullPathSources | SaveStrategy::Silence;
    if (p->export_3mf(arquivo, jeito) < 0) {
        BOOST_LOG_TRIVIAL(error) << "Filuna abertos: nao guardou o instantaneo em " << arquivo.string();
        MessageDialog(wxGetApp().mainframe, Filuna::trw("Não deu para guardar o projeto atual para trocar de arquivo. Confira o espaço no disco."),
                      Filuna::trw("Arquivos abertos"), wxOK | wxICON_WARNING).ShowModal();
        return false;
    }
    a.temporario = arquivo.string();
    if (prato != nullptr && prato->thumbnail_data.is_valid()) {
        a.miniatura = std::make_shared<ThumbnailData>(prato->thumbnail_data);
        ++a.versao_miniatura;
    }
    return true;
}

// Deixa o projeto da tela pronto para ser trocado sem as perguntas do Orca: o que importava já está no instantâneo.
void soltar_atual()
{
    GUI_App&          app = wxGetApp();
    PrinterTechnology tec = app.preset_bundle->printers.get_edited_preset().printer_technology();
    bool              mexeu = false;
    for (Tab* tab : app.tabs_list) {
        if (tab->supports_printer_technology(tec) && tab->current_preset_is_dirty()) {
            tab->get_presets()->discard_current_changes();
            mexeu = true;
        }
    }
    if (mexeu)
        app.load_current_presets(false);
    plater()->up_to_date(true, false);
    plater()->up_to_date(true, true);
}

// Põe na tela o projeto guardado em `a`. false = o instantâneo não carregou.
bool restaurar(Aberto& a)
{
    Plater* p = plater();
    boost::system::error_code ec;
    if (a.temporario.empty() || !fs::exists(fs::path(a.temporario), ec)) {
        p->new_project(true, true);
        if (!a.nome.empty())
            p->set_project_name(from_u8(a.nome));
        return a.temporario.empty();
    }

    p->load_project(from_u8(a.temporario), "<silence>");
    if (p->model().objects.empty())
        return false;

    if (!a.projeto.empty())
        p->set_project_filename(from_u8(a.projeto));
    else
        p->set_project_name(from_u8(a.nome.empty() ? Filuna::tr("Sem título") : a.nome));
    p->up_to_date(true, false);
    p->up_to_date(true, true);
    p->reset_project_dirty_after_save();
    if (a.modificado) {
        // A pergunta de salvar (fechar o item, fechar o programa) volta a valer para este projeto.
        Slic3r::put_other_changes();
        p->set_plater_dirty(true);
    }
    p->update_title_dirty_status();
    FilunaImpressao::proteger_modelo(a.protegido);

    if (a.fatiado) {
        // O fatiamento não vai no instantâneo: refaz em segundo plano a mesa que estava fatiada.
        wxPostEvent(p, SimpleEvent(EVT_GLTOOLBAR_SLICE_PLATE));
        if (a.na_visualizacao)
            wxGetApp().mainframe->select_tab(TAB_ID_PREVIEW);
    }
    apagar_temporario(a);  // na tela, o projeto vive no Plater
    return true;
}

size_t indice(unsigned id)
{
    for (size_t i = 0; i < g_abertos.size(); ++i)
        if (g_abertos[i].id == id)
            return i;
    return size_t(-1);
}

// Troca para o item `destino` (já guardado o atual, ou não, conforme `guardar`).
bool trocar_para(size_t destino, bool guardar)
{
    if (destino >= g_abertos.size() || destino == g_ativo)
        return destino == g_ativo;
    g_trocando = true;
    ScopeGuard fim([]() { g_trocando = false; redesenhar(); });

    if (guardar && !guardar_atual())
        return false;
    const size_t anterior = g_ativo;
    soltar_atual();
    if (restaurar(g_abertos[destino])) {
        g_ativo = destino;
        return true;
    }
    BOOST_LOG_TRIVIAL(error) << "Filuna abertos: instantaneo nao carregou: " << g_abertos[destino].temporario;
    MessageDialog(wxGetApp().mainframe, Filuna::trw_valores("Não deu para reabrir “{0}”.", { g_abertos[destino].nome }),
                  Filuna::trw("Arquivos abertos"), wxOK | wxICON_WARNING).ShowModal();
    if (guardar) {
        soltar_atual();
        restaurar(g_abertos[anterior]);  // volta para o que estava na tela
    }
    return false;
}

bool mesmo_arquivo(const std::string& a, const std::string& b)
{
    if (a.empty() || b.empty())
        return false;
    boost::system::error_code ec1, ec2;
    const fs::path pa = fs::weakly_canonical(fs::path(a), ec1);
    const fs::path pb = fs::weakly_canonical(fs::path(b), ec2);
#ifdef _WIN32
    return boost::iequals((ec1 ? fs::path(a) : pa).generic_wstring(), (ec2 ? fs::path(b) : pb).generic_wstring());
#else
    return (ec1 ? fs::path(a) : pa) == (ec2 ? fs::path(b) : pb);
#endif
}

// Cada projeto (.3mf) e cada G-code vira um arquivo aberto; os modelos soltos (STL, STEP, OBJ...) juntos viram um.
std::vector<wxArrayString> agrupar(const wxArrayString& arquivos)
{
    std::vector<wxArrayString> grupos;
    wxArrayString              soltos;
    for (const wxString& arq : arquivos) {
        const wxString baixo = arq.Lower();
        if (baixo.EndsWith(".3mf") || baixo.EndsWith(".gcode") || baixo.EndsWith(".g")) {
            wxArrayString um;
            um.Add(arq);
            grupos.push_back(um);
        } else
            soltos.Add(arq);
    }
    if (!soltos.IsEmpty())
        grupos.push_back(soltos);
    return grupos;
}

void abrir_grupo(const wxArrayString& grupo)
{
    Plater* p = plater();
    if (grupo.size() == 1) {
        // Já aberto: só mostra.
        const std::string caminho = into_u8(grupo[0]);
        if (mesmo_arquivo(caminho, projeto_atual()))
            return;
        for (size_t i = 0; i < g_abertos.size(); ++i)
            if (i != g_ativo && mesmo_arquivo(caminho, g_abertos[i].projeto)) {
                trocar_para(i, true);
                return;
            }
    }

    if (atual_vazio() && projeto_atual().empty() && !atual_modificado()) {
        p->load_files(grupo);  // a tela está vazia: o arquivo abre nela mesma
        return;
    }
    if (g_abertos.size() >= LIMITE) {
        MessageDialog(wxGetApp().mainframe,
                      Filuna::trw_valores("Já há {0} arquivos abertos. Feche um deles (o × na coluna da esquerda) para abrir outro.",
                                          { std::to_string(LIMITE) }),
                      Filuna::trw("Arquivos abertos"), wxOK | wxICON_INFORMATION).ShowModal();
        return;
    }

    g_trocando = true;
    ScopeGuard fim([]() { g_trocando = false; redesenhar(); });
    if (!guardar_atual())
        return;
    const size_t anterior = g_ativo;
    soltar_atual();
    p->new_project(true, true);
    Aberto novo;
    novo.id = g_proximo_id++;
    g_abertos.push_back(std::move(novo));
    g_ativo = g_abertos.size() - 1;

    p->load_files(grupo);
    if (atual_vazio()) {
        // Não abriu (cancelado ou com erro): some com o item novo e volta para o anterior.
        g_abertos.pop_back();
        g_ativo = anterior;
        soltar_atual();
        restaurar(g_abertos[g_ativo]);
    }
}

void abrir_novos(const wxArrayString& arquivos)
{
    garantir();
    for (const wxArrayString& grupo : agrupar(arquivos))
        abrir_grupo(grupo);
    redesenhar();
}

void tentar_pendentes()
{
    if (g_pendentes.IsEmpty())
        return;
    if (!livre()) {
        if (!g_avisou_espera && wxGetApp().mainframe != nullptr && wxGetApp().mainframe->IsEnabled()) {
            g_avisou_espera = true;
            avisar(Filuna::tr("O arquivo que você abriu entra na lista assim que a tarefa atual terminar."));
        }
        if (g_espera != nullptr)
            g_espera->StartOnce(500);
        return;
    }
    g_avisou_espera = false;
    wxArrayString agora;
    agora.swap(g_pendentes);
    abrir_novos(agora);
}

} // namespace

std::vector<Item> itens()
{
    std::vector<Item> lista;
    Plater*           p = plater();
    if (p == nullptr)
        return lista;
    garantir();
    for (size_t i = 0; i < g_abertos.size(); ++i) {
        Aberto& a = g_abertos[i];
        Item    it;
        it.id    = a.id;
        it.ativo = i == g_ativo;
        if (it.ativo && !g_trocando) {
            it.nome       = nome_atual();
            it.caminho    = projeto_atual();
            it.modificado = atual_modificado();
            // Miniatura viva: a do prato atual, quando a vista já a gerou.
            PartPlate*     prato  = p->get_partplate_list().get_curr_plate();
            const unsigned versao = p->plate_toolbar_image_version();
            if (atual_vazio()) {
                if (a.miniatura) {
                    a.miniatura.reset();
                    ++a.versao_miniatura;
                }
            } else if (prato != nullptr && prato->thumbnail_data.is_valid() && (a.versao_prato != versao || !a.miniatura_viva)) {
                a.miniatura      = std::make_shared<ThumbnailData>(prato->thumbnail_data);
                a.miniatura_viva = true;
                a.versao_prato   = versao;
                ++a.versao_miniatura;
            } else if (a.versao_prato != versao) {
                a.miniatura_viva = false;
                a.versao_prato   = versao;
            }
        } else {
            it.nome       = a.nome;
            it.caminho    = a.projeto;
            it.modificado = a.modificado;
        }
        it.miniatura        = a.miniatura;
        it.versao_miniatura = a.versao_miniatura;
        lista.push_back(std::move(it));
    }
    return lista;
}

void ativar(unsigned id)
{
    garantir();
    const size_t i = indice(id);
    if (i == size_t(-1) || i == g_ativo)
        return;
    if (!livre()) {
        avisar(Filuna::tr("Espere terminar o que está em andamento para trocar de arquivo."));
        return;
    }
    trocar_para(i, true);
}

void fechar(unsigned id)
{
    garantir();
    size_t i = indice(id);
    if (i == size_t(-1))
        return;
    if (!livre()) {
        avisar(Filuna::tr("Espere terminar o que está em andamento para fechar o arquivo."));
        return;
    }
    Plater* p     = plater();
    bool    ativo = i == g_ativo;
    if (ativo ? atual_modificado() : g_abertos[i].modificado) {
        const int r = perguntar_salvar(ativo ? nome_atual() : g_abertos[i].nome);
        if (r == wxID_CANCEL)
            return;
        if (r == wxID_YES) {
            // Salvar é o do Orca (Salvar como quando não tem arquivo): o projeto precisa estar na tela.
            if (!ativo && !trocar_para(i, true))
                return;
            if (p->save_project() != wxID_YES)
                return;
            i     = g_ativo;
            ativo = true;
        }
    }

    if (!ativo) {
        apagar_temporario(g_abertos[i]);
        g_abertos.erase(g_abertos.begin() + i);
        if (i < g_ativo)
            --g_ativo;
        redesenhar();
        return;
    }

    if (g_abertos.size() == 1) {
        // O último: fica um projeto vazio.
        g_trocando = true;
        ScopeGuard fim([]() { g_trocando = false; redesenhar(); });
        soltar_atual();
        p->new_project(true, false);
        Aberto& a = g_abertos.front();
        apagar_temporario(a);
        a = Aberto{};
        a.id = g_proximo_id++;
        return;
    }

    // Fecha o da tela e mostra o vizinho (o de baixo; sendo o último, o de cima). O fechado não é guardado.
    const size_t vizinho = i + 1 < g_abertos.size() ? i + 1 : i - 1;
    if (!trocar_para(vizinho, false))
        return;
    apagar_temporario(g_abertos[i]);
    g_abertos.erase(g_abertos.begin() + i);
    if (i < g_ativo)
        --g_ativo;
    redesenhar();
}

void abrir_outro()
{
    if (!livre())
        return;
    garantir();
    if (g_abertos.size() >= LIMITE && !(atual_vazio() && projeto_atual().empty())) {
        MessageDialog(wxGetApp().mainframe,
                      Filuna::trw_valores("Já há {0} arquivos abertos. Feche um deles (o × na coluna da esquerda) para abrir outro.",
                                          { std::to_string(LIMITE) }),
                      Filuna::trw("Arquivos abertos"), wxOK | wxICON_INFORMATION).ShowModal();
        return;
    }
    wxArrayString arquivos;
    wxGetApp().import_model(wxGetApp().mainframe, arquivos);
    if (!arquivos.IsEmpty())
        abrir_novos(arquivos);
}

void abrir_de_fora(const wxArrayString& arquivos)
{
    for (const wxString& arq : arquivos)
        g_pendentes.Add(arq);
    if (g_espera == nullptr) {
        g_espera = new wxTimer();
        g_espera->Bind(wxEVT_TIMER, [](wxTimerEvent&) { tentar_pendentes(); });
    }
    tentar_pendentes();
}

bool confirmar_saida()
{
    if (g_abertos.size() < 2)
        return true;
    // Um por vez: salvar um que não está na tela o traz para a tela, e o que estava nela passa a ser perguntado.
    for (;;) {
        size_t i = 0;
        while (i < g_abertos.size() && (i == g_ativo || !g_abertos[i].modificado))
            ++i;
        if (i == g_abertos.size())
            return true;
        const int r = perguntar_salvar(g_abertos[i].nome);
        if (r == wxID_CANCEL)
            return false;
        if (r == wxID_NO) {
            g_abertos[i].modificado = false;
            continue;
        }
        if (!trocar_para(i, true) || plater()->save_project() != wxID_YES)
            return false;
    }
}

void encerrar()
{
    if (g_espera != nullptr)
        g_espera->Stop();
    for (Aberto& a : g_abertos)
        apagar_temporario(a);
}

// ---------------------------------------------------------------------------------------------------------------
// A coluna

namespace {

constexpr float DIAMETRO = 38.f;
constexpr float RESPIRO  = 7.f;   // entre a borda da coluna e os círculos
constexpr float ENTRE    = 8.f;   // entre um círculo e outro
constexpr float AO_LADO  = 6.f;   // até a coluna seguinte (pratos)

// Iniciais do nome: primeira letra das duas primeiras palavras (ou as duas primeiras letras).
std::string iniciais(const std::string& nome)
{
    std::string base = nome;
    const size_t ponto = base.find_last_of('.');
    if (ponto != std::string::npos && ponto > 0)
        base = base.substr(0, ponto);
    auto letra = [&base](size_t pos) {
        size_t fim = pos + 1;
        while (fim < base.size() && (static_cast<unsigned char>(base[fim]) & 0xC0) == 0x80)
            ++fim;
        return base.substr(pos, fim - pos);
    };
    auto separador = [](char c) { return c == ' ' || c == '_' || c == '-' || c == '.' || c == '(' || c == ')'; };
    std::vector<size_t> comecos;
    for (size_t k = 0; k < base.size() && comecos.size() < 2; ++k)
        if (!separador(base[k]) && (k == 0 || separador(base[k - 1])) && (static_cast<unsigned char>(base[k]) & 0xC0) != 0x80)
            comecos.push_back(k);
    std::string r;
    if (comecos.size() >= 2)
        r = letra(comecos[0]) + letra(comecos[1]);
    else if (comecos.size() == 1) {
        r = letra(comecos[0]);
        const size_t seg = comecos[0] + r.size();
        if (seg < base.size() && !separador(base[seg]))
            r += letra(seg);
    }
    wxString w = from_u8(r).Upper();
    return w.empty() ? std::string("?") : into_u8(w);
}

ImU32 cor_do_item(unsigned id)
{
    static const ImU32 cores[] = { VIOLETA, MAGENTA, IM_COL32(0x1F, 0x8A, 0x7E, 255), IM_COL32(0xD9, 0x7A, 0x12, 255),
                                   IM_COL32(0x2F, 0x6F, 0xDB, 255), IM_COL32(0x8E, 0x3B, 0xB8, 255) };
    return cores[id % (sizeof(cores) / sizeof(cores[0]))];
}

ImU32 com_alfa(ImU32 cor, float alfa)
{
    const ImU32 a = ImU32(std::clamp(alfa, 0.f, 1.f) * 255.f + .5f);
    return (cor & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
}

} // namespace

Coluna::Coluna() = default;
Coluna::~Coluna() = default;

float largura(float escala)
{
    return (DIAMETRO + 2.f * RESPIRO + AO_LADO) * escala;
}

void desenhar(Coluna& coluna, float x, float topo, float base, float escala, bool escuro)
{
    const std::vector<Item> lista = itens();
    if (lista.empty())
        return;
    // Com um arquivo só, fica só o "+" (discreto); a lista aparece com dois ou mais.
    const bool  mostrar_itens = lista.size() >= 2;
    const float d             = DIAMETRO * escala;
    const float r             = .5f * d;
    const float respiro       = RESPIRO * escala;
    const float entre         = ENTRE * escala;
    const size_t n            = mostrar_itens ? lista.size() : 0;
    const float conteudo      = 2.f * respiro + float(n + 1) * d + float(n) * entre;
    const float altura        = std::min(conteudo, std::max(d + 2.f * respiro, base - topo));
    const float larg          = d + 2.f * respiro;

    // Texturas das miniaturas (só com o contexto GL da vista, que é o de agora).
    for (auto it = coluna.texturas.begin(); it != coluna.texturas.end();) {
        const bool existe = std::any_of(lista.begin(), lista.end(), [&it](const Item& i) { return i.id == it->first && i.miniatura; });
        it = existe ? std::next(it) : coluna.texturas.erase(it);
    }
    for (const Item& item : lista) {
        if (!item.miniatura || !mostrar_itens)
            continue;
        auto& tex = coluna.texturas[item.id];
        if (!tex.second || tex.first != item.versao_miniatura) {
            tex.second = std::make_unique<IMToolbarItem>();
            tex.second->set_thumbnail(*item.miniatura);
            tex.first = item.versao_miniatura;
        }
    }

    ImGuiWrapper& imgui = *wxGetApp().imgui();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(respiro, respiro));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, entre));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, .5f * larg);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, escuro ? ImVec4(.13f, .13f, .15f, .62f)
                                                    : ImVec4(0xF6 / 255.f, 0xF2 / 255.f, 0xEB / 255.f, mostrar_itens ? .92f : .7f));
    imgui.set_next_window_pos(x, topo, ImGuiCond_Always, 0.f, 0.f);
    imgui.set_next_window_size(larg, altura, ImGuiCond_Always);
    imgui.begin(std::string("##FilunaAbertos"), ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
                                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

    ImDrawList* dl       = ImGui::GetWindowDrawList();
    const ImU32 anel_vaz = escuro ? IM_COL32(255, 255, 255, 40) : IM_COL32(0, 0, 0, 30);
    const ImU32 fundo_x  = escuro ? IM_COL32(70, 70, 78, 245) : IM_COL32(58, 54, 50, 235);

    for (size_t k = 0; k < n; ++k) {
        const Item&  item  = lista[k];
        const ImVec2 pos   = ImGui::GetCursorScreenPos();
        const ImVec2 c     = pos + ImVec2(r, r);
        ImGui::PushID(int(item.id));
        const bool clicou  = ImGui::InvisibleButton("##arquivo", ImVec2(d, d), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
        const bool sobre   = ImGui::IsItemHovered();
        const bool do_meio = clicou && ImGui::IsMouseReleased(ImGuiMouseButton_Middle);
        ImGui::PopID();

        // Fechar: o "×" no canto de cima, à direita, quando o mouse está em cima.
        const ImVec2 xc     = c + ImVec2(r * .72f, -r * .72f);
        const float  xr     = 7.5f * escala;
        const ImVec2 mouse  = ImGui::GetIO().MousePos;
        const bool   no_x   = sobre && (mouse.x - xc.x) * (mouse.x - xc.x) + (mouse.y - xc.y) * (mouse.y - xc.y) <= (xr + escala) * (xr + escala);

        auto tex = coluna.texturas.find(item.id);
        if (tex != coluna.texturas.end() && tex->second.second && tex->second.second->texture_id) {
            dl->AddCircleFilled(c, r, IM_COL32_WHITE, 48);
            // O prato inteiro fica pequeno no círculo: mostra o miolo.
            dl->AddImageRounded(tex->second.second->texture_id, pos, pos + ImVec2(d, d), ImVec2(.14f, .86f), ImVec2(.86f, .14f), IM_COL32_WHITE, r);
        } else {
            dl->AddCircleFilled(c, r, cor_do_item(item.id), 48);
            const std::string ini   = iniciais(item.nome);
            ImFont*           fonte = ImGui::GetFont();
            const float       tam   = 15.f * escala;
            const ImVec2      med   = fonte->CalcTextSizeA(tam, FLT_MAX, 0.f, ini.c_str());
            dl->AddText(fonte, tam, c - med * .5f, IM_COL32_WHITE, ini.c_str());
        }

        if (item.ativo)
            dl->AddCircle(c, r + 3.f * escala, MAGENTA, 48, 2.5f * escala);
        else if (sobre)
            dl->AddCircle(c, r + 2.5f * escala, com_alfa(VIOLETA, .75f), 48, 2.f * escala);
        else
            dl->AddCircle(c, r, anel_vaz, 48, escala);

        if (sobre) {
            dl->AddCircleFilled(xc, xr, no_x ? MAGENTA : fundo_x, 24);
            dl->AddCircle(xc, xr, escuro ? IM_COL32(30, 30, 34, 255) : PAPEL, 24, 1.5f * escala);
            const float b = xr * .42f;
            dl->AddLine(xc + ImVec2(-b, -b), xc + ImVec2(b, b), IM_COL32_WHITE, 1.6f * escala);
            dl->AddLine(xc + ImVec2(-b, b), xc + ImVec2(b, -b), IM_COL32_WHITE, 1.6f * escala);
        } else if (item.modificado) {
            dl->AddCircleFilled(xc, 4.8f * escala, escuro ? IM_COL32(30, 30, 34, 255) : PAPEL, 16);
            dl->AddCircleFilled(xc, 3.4f * escala, MAGENTA, 16);
        }

        if (sobre) {
            std::string dica;
            if (no_x)
                dica = Filuna::tr_valores("Fechar “{0}”", { item.nome });
            else {
                dica = item.nome;
                if (item.modificado)
                    dica += "\n" + Filuna::tr("Alterado, ainda não salvo");
                if (!item.caminho.empty())
                    dica += "\n" + item.caminho;
                if (!item.ativo)
                    dica += "\n" + Filuna::tr("Clique para trocar; botão do meio fecha.");
            }
            imgui.tooltip(dica, ImGui::GetFontSize() * 24.f);
        }

        const unsigned id = item.id;
        if (do_meio || (clicou && no_x))
            wxGetApp().CallAfter([id]() { fechar(id); });
        else if (clicou && !item.ativo)
            wxGetApp().CallAfter([id]() { ativar(id); });
    }

    // "+": abrir outro arquivo.
    {
        const ImVec2 pos    = ImGui::GetCursorScreenPos();
        const ImVec2 c      = pos + ImVec2(r, r);
        const bool   clicou = ImGui::InvisibleButton("##+", ImVec2(d, d));
        const bool   sobre  = ImGui::IsItemHovered();
        const float  alfa   = sobre ? 1.f : (mostrar_itens ? .7f : .5f);
        if (sobre)
            dl->AddCircleFilled(c, r, com_alfa(VIOLETA, .12f), 48);
        dl->AddCircle(c, r - escala, com_alfa(VIOLETA, alfa), 48, 1.5f * escala);
        const float b = r * .38f;
        dl->AddLine(c + ImVec2(-b, 0.f), c + ImVec2(b, 0.f), com_alfa(VIOLETA, alfa), 2.f * escala);
        dl->AddLine(c + ImVec2(0.f, -b), c + ImVec2(0.f, b), com_alfa(VIOLETA, alfa), 2.f * escala);
        if (sobre)
            imgui.tooltip(Filuna::tr("Abrir outro arquivo (o atual continua aberto aqui na coluna)"), ImGui::GetFontSize() * 24.f);
        if (clicou)
            wxGetApp().CallAfter([]() { abrir_outro(); });
    }

    imgui.end();
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(4);
}

}}} // namespace Slic3r::GUI::FilunaAbertos
