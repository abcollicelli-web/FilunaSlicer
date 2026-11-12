#pragma once

// Filuna screens (Dispositivo, Calibração, Meu espaço and the sidebar footer) are pages under
// resources/web/filuna/ shown in a WebView. The account data comes from filuna3d.com; everything the
// page needs from the slicer itself (presets, slice result, calibration, tabs) goes through the bridge
// handled here: the page posts {"filuna":1,"id","acao","dados"} and gets Filuna._resposta(id, ok, dados).

#include <string>

#include <nlohmann/json.hpp>
#include <wx/panel.h>
#include <wx/string.h>
#include <wx/webview.h>

namespace Slic3r { namespace GUI {

class FilunaPanel : public wxPanel
{
public:
    // page: file name under resources/web/filuna/, e.g. "dispositivo.html".
    FilunaPanel(wxWindow* parent, const std::string& page);
    ~FilunaPanel() override = default;

    bool Show(bool show = true) override;

    // Calls Filuna._evento(name, data) in the page.
    void send_event(const std::string& name, const nlohmann::json& data = nlohmann::json::object());

private:
    void create_browser();
    void on_script_message(wxWebViewEvent& evt);
    void reply(const std::string& id, bool ok, const nlohmann::json& data);
    nlohmann::json handle(const std::string& action, const nlohmann::json& data);

    wxWebView*  m_browser{nullptr};
    wxString    m_url;
    bool        m_reset_on_show{false};
};

}} // namespace Slic3r::GUI
