#pragma once

// Filuna screens (Dispositivo, Calibração, Meu espaço and the sidebar footer) are pages under
// resources/web/filuna/ shown in a WebView. The account data comes from filuna3d.com; everything the
// page needs from the slicer itself (presets, slice result, calibration, tabs) goes through the bridge
// handled here: the page posts {"filuna":1,"id","acao","dados"} and gets Filuna._resposta(id, ok, dados).

#include <memory>
#include <string>

#include <nlohmann/json.hpp>
#include <wx/panel.h>
#include <wx/timer.h>
#include <wx/string.h>
#include <wx/webview.h>

namespace Slic3r {
class Preset;
namespace GUI {

// O perfil de filamento que a peça da mesa atual usa (a entrada da extrusora dela), não o aberto para edição.
const Preset& filuna_filamento_da_peca();

// Base of the site for the Slicer API (/api/slicer/v1/...): "https://filuna3d.com", or, to test a package
// against a site that is not published, the FILUNA_SITE_TESTE environment variable when it is http://127.0.0.1,
// http://localhost (optionally with a port) or https://<sub>.filuna3d.com. Any other value is ignored. Login and
// OAuth do not use this.
std::string filuna_site_base();

class FilunaPanel : public wxPanel
{
public:
    // page: file name under resources/web/filuna/, e.g. "dispositivo.html".
    FilunaPanel(wxWindow* parent, const std::string& page);
    ~FilunaPanel() override = default;

    bool Show(bool show = true) override;

    // Calls Filuna._evento(name, data) in the page.
    void send_event(const std::string& name, const nlohmann::json& data = nlohmann::json::object());

    // One Filuna account for the whole program: called whenever the device token changes (login or logout in
    // the home page or in any Filuna page). Every page gets the "conta" event and the home page is told too.
    static void account_changed(const std::string& token, const FilunaPanel* source = nullptr);

    // Opens a tab and, for Meu espaço, one of its sections ("estoque", "loja"…). The section is also kept until
    // the page asks for it ("secao.pendente"), so it works when the page is built only now.
    static void open_tab(const std::string& tab, const std::string& section = std::string());

    // Sends an event to every Filuna page (e.g. "preferencias" after the Preferences dialog closes).
    static void broadcast(const std::string& name, const nlohmann::json& data = nlohmann::json::object());

private:
    void create_browser();
    void on_script_message(wxWebViewEvent& evt);
    void reply(const std::string& id, bool ok, const nlohmann::json& data);
    nlohmann::json handle(const std::string& action, const nlohmann::json& data);

    std::unique_ptr<wxTimer> m_estado_timer; // estado ao vivo dos cards de Dispositivos
    wxWebView*  m_browser{nullptr};
    wxString    m_url;
    bool        m_reset_on_show{false};
};

}} // namespace Slic3r::GUI
