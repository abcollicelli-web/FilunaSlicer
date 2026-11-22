#include "FilunaMidia.hpp"

#include "FilunaAssistente.hpp"
#include "FilunaDispositivos.hpp"
#include "FilunaPanel.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "Plater.hpp"
#include "Widgets/CheckBox.hpp"
#include "Widgets/Label.hpp"
#include "Widgets/StateColor.hpp"
#include "Widgets/WebView.hpp"

#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/PresetBundle.hpp"

#include <boost/log/trivial.hpp>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <wx/webview.h>

#include <chrono>
#include <cstdint>
#include <map>
#include <stdexcept>

namespace Slic3r { namespace GUI {

using json = nlohmann::json;

namespace {

const char* const CHAVE_DISPONIVEL = "filuna_midia_disponivel"; // gravada pelas telas com a resposta do servidor
const char* const CHAVE_POST       = "filuna_midia_post";       // a caixinha da tela de envio (ligada por padrão)
constexpr auto    ESPERA_MAXIMA    = std::chrono::minutes(45);  // impressão que não começa nesse tempo: sem post

bool disponivel()
{
    const AppConfig* c = wxGetApp().app_config;
    return c != nullptr && c->get(CHAVE_DISPONIVEL) == "1";
}

bool post_ligado()
{
    const AppConfig* c = wxGetApp().app_config;
    return c != nullptr && c->get(CHAVE_POST) != "0";
}

std::string impressora_em_uso()
{
    PresetBundle* pb = wxGetApp().preset_bundle;
    if (pb == nullptr)
        return {};
    std::string nome = pb->physical_printers.get_selected_printer_name();
    return nome.empty() ? pb->printers.get_edited_preset().name : nome;
}

// A peça na mesa agora: nome, material e cor do primeiro filamento e a imagem da mesa (referência para a IA).
json dados_da_peca(const std::string& dev_id)
{
    json d = {{"dev_id", dev_id}, {"impressora", impressora_em_uso()}, {"produto", ""}, {"material", ""}, {"cor", ""}, {"foto_mesa", ""}};
    Plater* plater = wxGetApp().plater();
    if (plater == nullptr)
        return d;
    std::string produto = into_u8(plater->get_project_name());
    if (!plater->model().objects.empty() && (produto.empty() || produto == into_u8(_L("Untitled"))))
        produto = plater->model().objects.front()->name;
    d["produto"] = produto;
    if (PresetBundle* pb = wxGetApp().preset_bundle) {
        if (!pb->filament_presets.empty())
            if (const Preset* p = pb->filaments.find_preset(pb->filament_presets.front()))
                if (const auto* o = p->config.option<ConfigOptionStrings>("filament_type"); o && !o->values.empty())
                    d["material"] = o->values.front();
        if (const auto* cores = pb->project_config.option<ConfigOptionStrings>("filament_colour"); cores && !cores->values.empty())
            d["cor"] = cores->values.front();
    }
    try {
        d["foto_mesa"] = filuna_imagem_da_mesa().value("imagem", std::string());
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "Filuna post: sem imagem da mesa: " << e.what();
    }
    return d;
}

// A janela do post: a tela midia.html à esquerda e o ChatGPT do cliente à direita.
class JanelaPost : public wxDialog
{
public:
    explicit JanelaPost(json dados)
        : wxDialog(wxGetApp().mainframe, wxID_ANY, wxString::FromUTF8("Post nas redes sociais"), wxDefaultPosition,
                   wxGetApp().mainframe->FromDIP(wxSize(1440, 860)), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX)
        , m_dados(std::move(dados))
    {
        SetBackgroundColour(*wxWHITE);
        m_sizer = new wxBoxSizer(wxHORIZONTAL);
        m_tela  = new FilunaPanel(this, "midia.html");
        m_tela->SetMinSize(FromDIP(wxSize(500, -1))); // o ChatGPT ao lado precisa de mais de 800 px para o layout de computador
        m_sizer->Add(m_tela, 0, wxEXPAND);
        SetSizer(m_sizer);
        Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { Destroy(); });
        CentreOnParent();
    }
    ~JanelaPost() override;

    const json& dados() const { return m_dados; }

    void abrir_ia(const wxString& url)
    {
        if (m_ia == nullptr) {
            m_ia = WebView::CreateWebView(this, url);
            if (m_ia == nullptr)
                throw std::runtime_error("O navegador embutido não abriu neste computador.");
            filuna_ia_preparar_navegador(m_ia);
            m_ia->Bind(wxEVT_WEBVIEW_SCRIPT_RESULT, &JanelaPost::on_script, this);
            m_sizer->Add(m_ia, 1, wxEXPAND);
            Layout();
        } else if (!url.empty()) {
            m_ia->LoadURL(url);
        }
    }

    // O resultado chega depois, pelo aviso "midia.script" {id, ok, valor} para a tela.
    void script(const std::string& js, const std::string& id)
    {
        if (m_ia == nullptr)
            throw std::runtime_error("Abra a IA primeiro.");
        const std::intptr_t n = ++m_contador;
        m_pedidos[n]          = id;
        m_ia->RunScriptAsync(wxString::FromUTF8(js), reinterpret_cast<void*>(n));
    }

    void recarregar()
    {
        if (m_ia != nullptr)
            m_ia->Reload();
    }

private:
    void on_script(wxWebViewEvent& e)
    {
        const auto n  = reinterpret_cast<std::intptr_t>(e.GetClientData());
        const auto it = m_pedidos.find(n);
        if (it == m_pedidos.end())
            return;
        const json aviso = {{"id", it->second}, {"ok", !e.IsError()}, {"valor", std::string(e.GetString().ToUTF8())}};
        m_pedidos.erase(it);
        FilunaPanel::broadcast("midia.script", aviso);
    }

    json                                  m_dados;
    wxBoxSizer*                           m_sizer{nullptr};
    FilunaPanel*                          m_tela{nullptr};
    wxWebView*                            m_ia{nullptr};
    std::intptr_t                         m_contador{0};
    std::map<std::intptr_t, std::string> m_pedidos;
};

JanelaPost* g_janela = nullptr;

JanelaPost::~JanelaPost()
{
    if (g_janela == this)
        g_janela = nullptr;
}

void abrir_janela(json dados)
{
    if (g_janela != nullptr) {
        g_janela->Raise();
        return;
    }
    g_janela = new JanelaPost(std::move(dados));
    g_janela->Show();
}

// Envio com a caixinha ligada: espera a impressora estar imprimindo sem erro.
struct Pendente
{
    std::string                           dev_id;
    json                                  dados;
    std::chrono::steady_clock::time_point desde;
};
std::optional<Pendente> g_pendente;
wxTimer*                g_vigia = nullptr;

void vigiar()
{
    if (!g_pendente) {
        g_vigia->Stop();
        return;
    }
    if (std::chrono::steady_clock::now() - g_pendente->desde > ESPERA_MAXIMA) {
        BOOST_LOG_TRIVIAL(info) << "Filuna post: a impressão em " << g_pendente->dev_id << " não começou a tempo; sem post";
        g_pendente.reset();
        return;
    }
    const json e = FilunaDispositivos::estado();
    for (const auto& c : e.value("impressoras", json::array())) {
        if (c.value("dev_id", std::string()) != g_pendente->dev_id)
            continue;
        const std::string estado = c.value("estado", std::string());
        if (!c.value("erro", std::string()).empty()) {
            BOOST_LOG_TRIVIAL(info) << "Filuna post: a impressora avisou erro; sem post";
            g_pendente.reset();
        } else if (estado == "imprimindo" && c.value("progresso", 0) >= 1) {
            json dados = std::move(g_pendente->dados);
            g_pendente.reset();
            abrir_janela(std::move(dados));
        }
        return;
    }
}

std::string dev_selecionado()
{
    const json e = FilunaDispositivos::estado();
    for (const auto& c : e.value("impressoras", json::array()))
        if (c.value("selecionada", false))
            return c.value("dev_id", std::string());
    return {};
}

} // namespace

void filuna_midia_caixa_no_envio(wxWindow* pai, wxSizer* sizer)
{
    if (pai == nullptr || sizer == nullptr || !disponivel())
        return;
    auto* linha = new wxBoxSizer(wxHORIZONTAL);
    auto* caixa = new ::CheckBox(pai);
    caixa->SetValue(post_ligado());
    caixa->Bind(wxEVT_TOGGLEBUTTON, [](wxCommandEvent& e) {
        wxGetApp().app_config->set(CHAVE_POST, e.IsChecked() ? "1" : "0");
        e.Skip();
    });
    linha->Add(caixa, 0, wxALL | wxALIGN_CENTER, pai->FromDIP(2));
    auto* texto = new wxStaticText(pai, wxID_ANY,
                                   wxString::FromUTF8("Post nas redes sociais: quando a impressão começar sem erro, a sua IA cria a foto "
                                                      "do produto com a sua marca"));
    texto->SetFont(::Label::Body_13);
    texto->SetForegroundColour(StateColor::darkModeColorFor(wxColour("#323A3D")));
    linha->Add(texto, 0, wxALL | wxALIGN_CENTER, pai->FromDIP(2));
    sizer->Add(linha, 0, wxTOP | wxBOTTOM, pai->FromDIP(4));
}

void filuna_midia_envio_confirmado(const std::string& dev_id)
{
    if (!disponivel() || !post_ligado())
        return;
    const std::string dev = dev_id.empty() ? dev_selecionado() : dev_id;
    if (dev.empty()) {
        BOOST_LOG_TRIVIAL(info) << "Filuna post: envio sem impressora conhecida; sem post";
        return;
    }
    g_pendente = Pendente{dev, dados_da_peca(dev), std::chrono::steady_clock::now()};
    if (g_vigia == nullptr) {
        g_vigia = new wxTimer();
        g_vigia->Bind(wxEVT_TIMER, [](wxTimerEvent&) { vigiar(); });
    }
    g_vigia->Start(10000);
    BOOST_LOG_TRIVIAL(info) << "Filuna post: esperando a impressão começar em " << dev;
}

std::optional<json> filuna_midia_acao(const std::string& acao, const json& d)
{
    if (acao.rfind("midia.", 0) != 0)
        return std::nullopt;
    AppConfig* cfg = wxGetApp().app_config;
    if (acao == "midia.disponivel") {
        cfg->set(CHAVE_DISPONIVEL, d.value("disponivel", false) ? "1" : "0");
        return json::object();
    }
    if (acao == "midia.preferencia") {
        if (d.contains("post"))
            cfg->set(CHAVE_POST, d.value("post", true) ? "1" : "0");
        return json{{"post", post_ligado()}, {"disponivel", disponivel()}};
    }
    if (!disponivel())
        throw std::runtime_error("Post nas redes sociais não está liberado para esta conta.");
    if (acao == "midia.abrir") {
        // "Criar post da peça na mesa" (Meu espaço › Posts): sem esperar impressão.
        json dados      = dados_da_peca(dev_selecionado());
        dados["origem"] = "mesa";
        wxGetApp().CallAfter([dados]() mutable { abrir_janela(std::move(dados)); });
        return json::object();
    }
    if (acao == "midia.dados")
        return g_janela != nullptr ? json(g_janela->dados()) : json::object();
    if (acao == "midia.fechar") {
        if (g_janela != nullptr)
            g_janela->CallAfter([] { if (g_janela != nullptr) g_janela->Close(); });
        return json::object();
    }
    if (acao == "midia.ia") {
        if (g_janela == nullptr)
            throw std::runtime_error("A janela do post está fechada.");
        const std::string a = d.value("acao", std::string());
        if (a == "abrir") {
            const wxString url = wxString::FromUTF8(d.value("url", std::string("https://chatgpt.com/")));
            if (!url.StartsWith("https://") || !filuna_site_de_ia(url))
                throw std::runtime_error("Endereço de IA não aceito.");
            g_janela->abrir_ia(url);
        } else if (a == "script") {
            g_janela->script(d.value("js", std::string()), d.value("id", std::string()));
        } else if (a == "recarregar") {
            g_janela->recarregar();
        } else {
            throw std::runtime_error("Ação da IA desconhecida.");
        }
        return json::object();
    }
    throw std::runtime_error("Ação desconhecida: " + acao);
}

}} // namespace Slic3r::GUI
