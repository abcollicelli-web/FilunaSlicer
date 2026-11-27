#include "FilunaMakerWorld.hpp"
#include "FilunaIdioma.hpp"

#include "FilunaLateral.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "Plater.hpp"
#include "Widgets/WebView.hpp"
#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Utils.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/utils.h>
#include <wx/webview.h>

#ifdef _WIN32
#include <WebView2.h>
#endif

#include <functional>

namespace Slic3r { namespace GUI {

namespace {

const char* INICIO = "https://makerworld.com/";

bool do_site(const wxString& url)
{
    const wxString u = url.Lower();
    // o MakerWorld e o login da Bambu, inclusive "Entrar com Google/Apple/Facebook", que abrem em janela própria: tudo
    // fica na aba, para o login terminar aqui dentro (no navegador de fora a conta ficaria lá, não no Filuna)
    for (const char* h : {"https://makerworld.com", "https://www.makerworld.com", "https://bambulab.com", "https://www.bambulab.com",
                          "https://api.bambulab.com", "https://passport.bambulab.com", "https://makerworld.bblmw.com",
                          "https://accounts.google.com", "https://appleid.apple.com", "https://www.facebook.com", "https://m.facebook.com"})
        if (u.StartsWith(h))
            return true;
    return false;
}

// Arquivos de modelo que o Filuna abre; o resto (PDF, imagem…) segue como download comum do navegador.
bool arquivo_de_modelo(const std::string& nome)
{
    const std::string e = boost::to_lower_copy(boost::filesystem::path(nome).extension().string());
    for (const char* x : {".3mf", ".stl", ".step", ".stp", ".obj", ".amf", ".zip"})
        if (e == x)
            return true;
    return false;
}

#ifdef _WIN32
// Os dois avisos do WebView2 que a aba usa, sem depender da WRL: começo de download e mudança de estado dele.
class AoBaixar : public ICoreWebView2DownloadStartingEventHandler
{
public:
    using Funcao = std::function<HRESULT(ICoreWebView2DownloadStartingEventArgs*)>;
    explicit AoBaixar(Funcao f) : m_f(std::move(f)) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (riid == IID_IUnknown || riid == __uuidof(ICoreWebView2DownloadStartingEventHandler)) {
            *ppv = this;
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG r = --m_ref;
        if (r == 0)
            delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2*, ICoreWebView2DownloadStartingEventArgs* args) override { return m_f(args); }

private:
    ULONG  m_ref{1};
    Funcao m_f;
};

class AoMudarDownload : public ICoreWebView2StateChangedEventHandler
{
public:
    using Funcao = std::function<HRESULT(ICoreWebView2DownloadOperation*)>;
    explicit AoMudarDownload(Funcao f) : m_f(std::move(f)) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (riid == IID_IUnknown || riid == __uuidof(ICoreWebView2StateChangedEventHandler)) {
            *ppv = this;
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG r = --m_ref;
        if (r == 0)
            delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2DownloadOperation* op, IUnknown*) override { return m_f(op); }

private:
    ULONG  m_ref{1};
    Funcao m_f;
};

std::string de_largo(LPCWSTR s) { return s ? wxString(s).ToStdString(wxConvUTF8) : std::string(); }
#endif

} // namespace

bool filuna_link_de_modelo(const wxString& url)
{
    const wxString u = url.Lower();
    return u.StartsWith("bambustudioopen://") || u.StartsWith("bambustudio://open") || u.StartsWith("orcaslicer://open");
}

namespace {
// Barra fina da aba, no estilo Filuna: Voltar, Avançar, Recarregar e Início em ícones redondos, o nome do site e, à
// direita, a opção de usar o navegador do cliente.
class BarraNavegador : public wxPanel
{
public:
    enum { VOLTAR, AVANCAR, RECARREGAR, INICIO_MW, NAVEGADOR, NADA };
    std::function<void(int)> ao_clicar;

    explicit BarraNavegador(wxWindow* pai) : wxPanel(pai, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetBackgroundColour(*wxWHITE);
        SetMinSize(wxSize(-1, FromDIP(44)));
        Bind(wxEVT_PAINT, [this](wxPaintEvent&) { pintar(); });
        Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
        Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
            const int s = sob(e.GetPosition());
            if (s != m_sobre) {
                m_sobre = s;
                SetCursor(s == NADA ? wxNullCursor : wxCursor(wxCURSOR_HAND));
                Refresh();
            }
        });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_sobre = NADA; Refresh(); });
        Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e) {
            const int s = sob(e.GetPosition());
            if (s != NADA && ao_clicar)
                ao_clicar(s);
        });
    }
    void definir_navegador(bool ligado)
    {
        m_navegador = ligado;
        Refresh();
    }

private:
    wxRect area(int i) const
    {
        const int l = FromDIP(30), y = (GetClientSize().y - l) / 2;
        if (i <= INICIO_MW)
            return wxRect(FromDIP(10) + i * (l + FromDIP(4)), y, l, l);
        const int w = FromDIP(m_navegador ? 150 : 160);
        return wxRect(GetClientSize().x - w - FromDIP(12), y, w, l);
    }
    int sob(const wxPoint& p) const
    {
        for (int i = VOLTAR; i <= NAVEGADOR; ++i)
            if (area(i).Contains(p))
                return i;
        return NADA;
    }
    void pintar()
    {
        using namespace FilunaLateral::pintura;
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc)
            return;
        const wxSize s = GetClientSize();
        gc->SetPen(wxPen(AREIA_BORDA, 1));
        gc->StrokeLine(0, s.y - 1, s.x, s.y - 1);
        for (int i = VOLTAR; i <= INICIO_MW; ++i) {
            const wxRect r     = area(i);
            const bool   sobre = i == m_sobre;
            gc->SetPen(wxPen(sobre ? MAGENTA : AREIA_BORDA, FromDIP(1)));
            gc->SetBrush(wxBrush(sobre ? ROSA_CLARO : BRANCO));
            gc->DrawEllipse(r.x + 0.5, r.y + 0.5, r.width - 1, r.height - 1);
            const wxColour cor = sobre ? MAGENTA : CINZA;
            const double   cx = r.x + r.width / 2., cy = r.y + r.height / 2., t = FromDIP(5);
            gc->SetPen(wxPen(cor, FromDIP(2)));
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            wxGraphicsPath p = gc->CreatePath();
            if (i == VOLTAR || i == AVANCAR) {
                const double d = i == VOLTAR ? 1. : -1.;
                p.MoveToPoint(cx + d * t * 0.5, cy - t);
                p.AddLineToPoint(cx - d * t * 0.5, cy);
                p.AddLineToPoint(cx + d * t * 0.5, cy + t);
                gc->StrokePath(p);
            } else if (i == RECARREGAR) {
                desenhar_icone(*gc, this, FilunaLateral::Icone::sincronizar, wxRect2DDouble(cx - t * 1.4, cy - t * 1.4, t * 2.8, t * 2.8), cor, false);
            } else { // casinha
                p.MoveToPoint(cx - t, cy);
                p.AddLineToPoint(cx, cy - t);
                p.AddLineToPoint(cx + t, cy);
                p.MoveToPoint(cx - t * 0.7, cy - t * 0.2);
                p.AddLineToPoint(cx - t * 0.7, cy + t);
                p.AddLineToPoint(cx + t * 0.7, cy + t);
                p.AddLineToPoint(cx + t * 0.7, cy - t * 0.2);
                gc->StrokePath(p);
            }
        }
        // nome do site
        const int x = area(INICIO_MW).GetRight() + FromDIP(14);
        gc->SetFont(fonte(this, 10., true), TINTA);
        double tw = 0, th = 0;
        gc->GetTextExtent(wxString::FromUTF8("MakerWorld"), &tw, &th);
        gc->DrawText(wxString::FromUTF8("MakerWorld"), x, (s.y - th) / 2.);
        gc->SetFont(fonte(this, 8., false), CINZA);
        const int x2 = x + int(tw) + FromDIP(10), livre = area(NAVEGADOR).x - x2 - FromDIP(10);
        gc->DrawText(caber(this, Filuna::trw("site oficial · entre com a sua conta Bambu · \"Open in Bambu Studio\" abre aqui na Preparação"),
                           fonte(this, 8., false), livre),
                     x2, (s.y - FromDIP(14)) / 2.);
        FilunaLateral::Botao b{wxString::FromUTF8(m_navegador ? "Navegador: ligado" : "Usar meu navegador")};
        b.destaque = m_navegador;
        desenhar_botao(*gc, this, area(NAVEGADOR), b, m_sobre == NAVEGADOR, false);
    }

    int  m_sobre{NADA};
    bool m_navegador{false};
};

// Navegador comum (Edge), sem a marca "BBL-Slicer" que o Orca põe: com ela o MakerWorld acha que está dentro do Bambu
// Studio, esconde o próprio cabeçalho (fica a faixa em branco) e manda o "Abrir" por um canal interno do Bambu Studio.
// O Filuna não se passa pelo cliente oficial.
const char* NAVEGADOR_COMUM = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/142.0.0.0 "
                              "Safari/537.36 Edg/142.0.0.0";
} // namespace

FilunaMakerWorld::FilunaMakerWorld(wxWindow* parent) : wxPanel(parent, wxID_ANY)
{
    SetBackgroundColour(*wxWHITE);
    auto* topo = new BarraNavegador(this);
    topo->definir_navegador(wxGetApp().app_config->get("filuna_makerworld_navegador") == "1");
    m_browser = WebView::CreateWebView(this, wxString::FromUTF8(INICIO));
    if (m_browser)
        m_browser->SetUserAgent(wxString::FromUTF8(NAVEGADOR_COMUM));
    topo->ao_clicar = [this, topo](int i) {
        if (i == BarraNavegador::NAVEGADOR) {
            usar_navegador();
            topo->definir_navegador(wxGetApp().app_config->get("filuna_makerworld_navegador") == "1");
            return;
        }
        if (!m_browser)
            return;
        if (i == BarraNavegador::VOLTAR && m_browser->CanGoBack())
            m_browser->GoBack();
        else if (i == BarraNavegador::AVANCAR && m_browser->CanGoForward())
            m_browser->GoForward();
        else if (i == BarraNavegador::INICIO_MW)
            m_browser->LoadURL(wxString::FromUTF8(INICIO));
        else if (i == BarraNavegador::RECARREGAR)
            m_browser->Reload();
    };
    auto* s = new wxBoxSizer(wxVERTICAL);
    s->Add(topo, 0, wxEXPAND);
    if (m_browser) {
        s->Add(m_browser, 1, wxEXPAND);
        m_browser->Bind(wxEVT_WEBVIEW_NAVIGATING, &FilunaMakerWorld::on_navigating, this);
        m_browser->Bind(wxEVT_WEBVIEW_NEWWINDOW, &FilunaMakerWorld::on_new_window, this);
        // o WebView2 fica pronto só depois da primeira página: aí liga a captura dos downloads
        m_browser->Bind(wxEVT_WEBVIEW_LOADED, [this](wxWebViewEvent& e) {
            instalar_downloads();
            e.Skip();
        });
    }
    SetSizer(s);
}

// Para quando o Google não aceita o login dentro da aba: o cliente usa o MakerWorld no navegador dele (onde o login com
// Google funciona) e o botão "Abrir no Bambu Studio" do site passa a abrir no Filuna. Troca qual programa atende esse
// botão no Windows, por isso só com a confirmação do cliente; dá para devolver ao Bambu Studio pelo mesmo botão.
void FilunaMakerWorld::usar_navegador()
{
    AppConfig* cfg     = wxGetApp().app_config;
    const bool ligado  = cfg->get("filuna_makerworld_navegador") == "1";
    if (ligado) {
        if (wxMessageBox(Filuna::trw("O botão \"Abrir no Bambu Studio\" do MakerWorld no seu navegador está abrindo no Filuna.\n\nDevolver esse botão ao Bambu Studio? (o Bambu Studio volta a atendê-lo quando for aberto)"),
                         Filuna::trw("MakerWorld no navegador"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
            return;
        wxGetApp().disassociate_url(L"bambustudioopen");
        cfg->set("filuna_makerworld_navegador", "0");
        return;
    }
    if (wxMessageBox(Filuna::trw("Para entrar com Google, Apple ou Facebook pelo seu navegador:\n\n1. O MakerWorld abre no seu navegador; entre na sua conta lá.\n2. No modelo, clique em \"Open in Bambu Studio\": o modelo abre aqui, na Preparação.\n\nIsso faz o botão \"Abrir no Bambu Studio\" abrir no Filuna em vez do Bambu Studio. Continuar?"),
                     Filuna::trw("MakerWorld no navegador"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
        return;
    wxGetApp().associate_url(L"bambustudioopen");
    cfg->set("filuna_makerworld_navegador", "1");
    wxLaunchDefaultBrowser(wxString::FromUTF8(INICIO));
}

void FilunaMakerWorld::abrir_link(const wxString& url)
{
    BOOST_LOG_TRIVIAL(info) << "Filuna MakerWorld: link de modelo " << url.ToStdString(wxConvUTF8);
    // o mesmo baixador dos links vindos do navegador: vai para a Preparação, baixa o 3MF e abre o projeto
    const std::string u = url.ToStdString(wxConvUTF8);
    wxGetApp().CallAfter([u] { wxGetApp().start_download(u); });
}

void FilunaMakerWorld::on_navigating(wxWebViewEvent& e)
{
    if (filuna_link_de_modelo(e.GetURL())) {
        e.Veto();
        abrir_link(e.GetURL());
    }
}

void FilunaMakerWorld::on_new_window(wxWebViewEvent& e)
{
    const wxString url = e.GetURL();
    e.Veto();
    if (filuna_link_de_modelo(url))
        abrir_link(url);
    else if (do_site(url))
        m_browser->LoadURL(url); // login e páginas do próprio MakerWorld ficam na aba
    else if (!url.empty())
        wxLaunchDefaultBrowser(url);
}

void FilunaMakerWorld::abrir_arquivo(const std::string& caminho)
{
    BOOST_LOG_TRIVIAL(info) << "Filuna MakerWorld: abrindo na Preparação " << caminho;
    Plater* plater = wxGetApp().plater();
    if (!plater || !wxGetApp().mainframe)
        return;
    wxGetApp().mainframe->select_tab(wxString(TAB_ID_PREPARE));
    const wxString arquivo = wxString::FromUTF8(caminho);
    if (boost::to_lower_copy(boost::filesystem::path(caminho).extension().string()) == ".3mf")
        plater->load_project(arquivo); // projeto do MakerWorld: o Orca pergunta o que fazer com os perfis dele
    else {
        wxArrayString arr;
        arr.Add(arquivo);
        plater->load_files(arr);
    }
}

void FilunaMakerWorld::instalar_downloads()
{
#ifdef _WIN32
    if (m_downloads_ok || !m_browser)
        return;
    auto* webview = static_cast<ICoreWebView2*>(m_browser->GetNativeBackend());
    if (!webview)
        return;
    ICoreWebView2_4* webview4 = nullptr;
    if (FAILED(webview->QueryInterface(__uuidof(ICoreWebView2_4), reinterpret_cast<void**>(&webview4))) || !webview4) {
        BOOST_LOG_TRIVIAL(warning) << "Filuna MakerWorld: WebView2 sem suporte a downloads (ICoreWebView2_4)";
        m_downloads_ok = true;
        return;
    }
    const boost::filesystem::path pasta = boost::filesystem::path(data_dir()) / "filuna" / "makerworld";
    auto*                         h     = new AoBaixar([this, pasta](ICoreWebView2DownloadStartingEventArgs* args) -> HRESULT {
        LPWSTR sugerido = nullptr;
        args->get_ResultFilePath(&sugerido);
        const std::string nome = boost::filesystem::path(de_largo(sugerido)).filename().string();
        ::CoTaskMemFree(sugerido);
        if (!arquivo_de_modelo(nome))
            return S_OK; // não é modelo: o navegador segue como sempre
        boost::system::error_code ec;
        boost::filesystem::create_directories(pasta, ec);
        const boost::filesystem::path destino = pasta / nome;
        boost::filesystem::remove(destino, ec);
        args->put_ResultFilePath(destino.wstring().c_str());
        args->put_Handled(TRUE); // sem a janela de downloads do navegador
        ICoreWebView2DownloadOperation* op = nullptr;
        if (SUCCEEDED(args->get_DownloadOperation(&op)) && op) {
            const std::string caminho = destino.string();
            auto* mudou = new AoMudarDownload([this, caminho](ICoreWebView2DownloadOperation* o) -> HRESULT {
                COREWEBVIEW2_DOWNLOAD_STATE estado;
                if (FAILED(o->get_State(&estado)))
                    return S_OK;
                if (estado == COREWEBVIEW2_DOWNLOAD_STATE_COMPLETED)
                    CallAfter([this, caminho] { abrir_arquivo(caminho); });
                else if (estado == COREWEBVIEW2_DOWNLOAD_STATE_INTERRUPTED)
                    BOOST_LOG_TRIVIAL(warning) << "Filuna MakerWorld: download interrompido " << caminho;
                return S_OK;
            });
            EventRegistrationToken t;
            op->add_StateChanged(mudou, &t);
            mudou->Release();
            op->Release();
        }
        BOOST_LOG_TRIVIAL(info) << "Filuna MakerWorld: baixando " << destino.string();
        return S_OK;
    });
    EventRegistrationToken token;
    if (SUCCEEDED(webview4->add_DownloadStarting(h, &token)))
        m_downloads_ok = true;
    h->Release();
    webview4->Release();
#else
    m_downloads_ok = true;
#endif
}

}} // namespace Slic3r::GUI
