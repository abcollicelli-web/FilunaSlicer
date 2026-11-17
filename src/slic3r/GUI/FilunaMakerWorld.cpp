#include "FilunaMakerWorld.hpp"

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

FilunaMakerWorld::FilunaMakerWorld(wxWindow* parent) : wxPanel(parent, wxID_ANY)
{
    SetBackgroundColour(*wxWHITE);
    auto* topo = new FilunaLateral::Botoes(this, false);
    topo->SetMinSize(wxSize(-1, FromDIP(30)));
    auto botoes = [] {
        const bool navegador = wxGetApp().app_config->get("filuna_makerworld_navegador") == "1";
        return std::vector<FilunaLateral::Botao>{FilunaLateral::Botao{wxString::FromUTF8("‹ Voltar")}, FilunaLateral::Botao{wxString::FromUTF8("Avançar ›")},
                                                 FilunaLateral::Botao{wxString::FromUTF8("Início do MakerWorld")},
                                                 FilunaLateral::Botao{wxString::FromUTF8("Recarregar")},
                                                 FilunaLateral::Botao{wxString::FromUTF8(navegador ? "Navegador: ligado" : "Usar meu navegador")}};
    };
    topo->definir(botoes());
    m_browser = WebView::CreateWebView(this, wxString::FromUTF8(INICIO));
    topo->ao_clicar = [this, topo, botoes](int i, const wxRect&) {
        if (i == 4) {
            usar_navegador();
            topo->definir(botoes());
            return;
        }
        if (!m_browser)
            return;
        if (i == 0 && m_browser->CanGoBack())
            m_browser->GoBack();
        else if (i == 1 && m_browser->CanGoForward())
            m_browser->GoForward();
        else if (i == 2)
            m_browser->LoadURL(wxString::FromUTF8(INICIO));
        else if (i == 3)
            m_browser->Reload();
    };
    auto* s = new wxBoxSizer(wxVERTICAL);
    s->Add(topo, 0, wxEXPAND | wxALL, FromDIP(6));
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
        if (wxMessageBox(wxString::FromUTF8("O botão \"Abrir no Bambu Studio\" do MakerWorld no seu navegador está abrindo no Filuna.\n\n"
                                            "Devolver esse botão ao Bambu Studio? (o Bambu Studio volta a atendê-lo quando for aberto)"),
                         wxString::FromUTF8("MakerWorld no navegador"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
            return;
        wxGetApp().disassociate_url(L"bambustudioopen");
        cfg->set("filuna_makerworld_navegador", "0");
        return;
    }
    if (wxMessageBox(wxString::FromUTF8("Para entrar com Google, Apple ou Facebook pelo seu navegador:\n\n"
                                        "1. O MakerWorld abre no seu navegador; entre na sua conta lá.\n"
                                        "2. No modelo, clique em \"Open in Bambu Studio\": o modelo abre aqui, na Preparação.\n\n"
                                        "Isso faz o botão \"Abrir no Bambu Studio\" abrir no Filuna em vez do Bambu Studio. Continuar?"),
                     wxString::FromUTF8("MakerWorld no navegador"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
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
