#pragma once

// Filuna — aba MakerWorld: o site oficial makerworld.com dentro do Filuna. O cliente entra com a conta Bambu no próprio
// site (não é o Filuna se passando pelo Bambu Studio). O que o site manda abrir no fatiador vem direto para a
// Preparação, sem passar pela pasta de downloads:
//   - o link "Abrir no Bambu Studio" (bambustudioopen://) ou "Abrir no OrcaSlicer" (orcaslicer://open?file=…) vai para
//     o baixador que o Filuna já tem, o mesmo que atende esses links vindos do navegador;
//   - o download de um modelo (3MF, STL, STEP, OBJ, ZIP) é guardado numa pasta do Filuna e aberto na Preparação.

#include <wx/panel.h>
#include <wx/string.h>

#include <string>

class wxWebView;
class wxWebViewEvent;

namespace Slic3r { namespace GUI {

class FilunaMakerWorld : public wxPanel
{
public:
    explicit FilunaMakerWorld(wxWindow* parent);

private:
    void on_navigating(wxWebViewEvent& e);
    void on_new_window(wxWebViewEvent& e);
    void instalar_downloads(); // pega os downloads do navegador embutido (WebView2), quando ele estiver pronto
    void abrir_link(const wxString& url);
    void abrir_arquivo(const std::string& caminho);

    wxWebView* m_browser{nullptr};
    bool       m_downloads_ok{false};
};

// O link que o MakerWorld manda abrir num fatiador (bambustudioopen://, bambustudio://open?file=, orcaslicer://open?file=).
bool filuna_link_de_modelo(const wxString& url);

}} // namespace Slic3r::GUI
