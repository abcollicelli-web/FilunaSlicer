#include "FilunaIALado.hpp"
#include "FilunaIdioma.hpp"

#include "FilunaAssistente.hpp"
#include "FilunaPanel.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "Widgets/WebView.hpp"

#include <wx/display.h>
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/webview.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <stdexcept>

namespace Slic3r { namespace GUI {

using json = nlohmann::json;

namespace {

// A janela da IA: só o navegador, com o jeito de navegador comum (o login do Google aceita) e as janelas de login
// abrindo por cima. Fica do lado direito da tela, deixando o Slicer à esquerda.
class JanelaIALado : public wxFrame
{
public:
    JanelaIALado()
        : wxFrame(wxGetApp().mainframe, wxID_ANY, Filuna::trw("IA do Filuna · sua conta"), wxDefaultPosition, wxDefaultSize,
                  wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
    {
        SetBackgroundColour(*wxWHITE);
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        SetSizer(sizer);
        // metade direita da tela onde está o Slicer (mínimo de 820 px: o layout de computador da IA)
        const int      tela = wxDisplay::GetFromWindow(wxGetApp().mainframe);
        const wxRect   area = wxDisplay(tela == wxNOT_FOUND ? 0 : unsigned(tela)).GetClientArea();
        const int      w    = std::max(FromDIP(820), area.width * 45 / 100);
        SetSize(wxRect(area.GetRight() - w + 1, area.y, w, area.height));
        Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) {
            FilunaPanel::broadcast("ia.lado.fechada");
            Destroy();
        });
    }
    ~JanelaIALado() override;

    void abrir(const wxString& url)
    {
        if (m_ia == nullptr) {
            m_ia = WebView::CreateWebView(this, url);
            if (m_ia == nullptr)
                throw std::runtime_error("O navegador embutido não abriu neste computador.");
            filuna_ia_preparar_navegador(m_ia);
            m_ia->Bind(wxEVT_WEBVIEW_SCRIPT_RESULT, &JanelaIALado::resultado, this);
            GetSizer()->Add(m_ia, 1, wxEXPAND);
            Layout();
        } else if (!url.empty() && m_ia->GetCurrentURL() != url) {
            m_ia->LoadURL(url);
        }
        Show();
        Raise();
    }

    void script(const std::string& js, const std::string& id)
    {
        if (m_ia == nullptr)
            throw std::runtime_error("Abra a IA primeiro.");
        const std::intptr_t n = ++m_contador;
        m_pedidos[n]          = id;
        m_ia->RunScriptAsync(wxString::FromUTF8(js), reinterpret_cast<void*>(n));
    }

private:
    void resultado(wxWebViewEvent& e)
    {
        const auto n  = reinterpret_cast<std::intptr_t>(e.GetClientData());
        const auto it = m_pedidos.find(n);
        if (it == m_pedidos.end())
            return;
        const json aviso = {{"id", it->second}, {"ok", !e.IsError()}, {"valor", std::string(e.GetString().ToUTF8())}};
        m_pedidos.erase(it);
        FilunaPanel::broadcast("ia.lado.script", aviso);
    }

    wxWebView*                            m_ia{nullptr};
    std::intptr_t                         m_contador{0};
    std::map<std::intptr_t, std::string> m_pedidos;
};

JanelaIALado* g_janela = nullptr;

JanelaIALado::~JanelaIALado()
{
    if (g_janela == this)
        g_janela = nullptr;
}

} // namespace

std::optional<json> filuna_ia_lado_acao(const std::string& acao, const json& d)
{
    if (acao == "ia.lado.abrir") {
        const wxString url = wxString::FromUTF8(d.value("url", std::string("https://gemini.google.com/app")));
        if (!url.StartsWith("https://") || !filuna_site_de_ia(url))
            throw std::runtime_error("Endereço de IA não aceito.");
        if (g_janela == nullptr)
            g_janela = new JanelaIALado();
        g_janela->abrir(url);
        return json::object();
    }
    if (acao == "ia.lado.script") {
        if (g_janela == nullptr)
            throw std::runtime_error("A janela da IA está fechada.");
        const std::string js = d.value("js", std::string());
        if (js.empty() || js.size() > 8u * 1024u * 1024u)
            throw std::runtime_error("Roteiro vazio ou grande demais.");
        g_janela->script(js, d.value("id", std::string()));
        return json::object();
    }
    if (acao == "ia.lado.fechar") {
        if (g_janela != nullptr)
            g_janela->CallAfter([] { if (g_janela != nullptr) g_janela->Close(); });
        return json::object();
    }
    return std::nullopt;
}

}} // namespace Slic3r::GUI
