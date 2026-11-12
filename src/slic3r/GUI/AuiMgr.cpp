#include "AuiMgr.hpp"

#include "GUI_App.hpp"
#ifdef __WXGTK__
#include "LinuxDisplayBackend.hpp"
#endif

#include <wx/aui/dockart.h>
#include <wx/aui/floatpane.h>
#include <wx/aui/framemanager.h>
#include <wx/colour.h>
#include <wx/event.h>
#include <wx/gdicmn.h>

namespace Slic3r { namespace GUI {

namespace {

// TODO: listen on dark ui change
class FloatFrame : public wxAuiFloatingFrame
{
public:
    FloatFrame(wxWindow* parent, wxAuiManager* ownerMgr, const wxAuiPaneInfo& pane) : wxAuiFloatingFrame(parent, ownerMgr, pane)
    {
        wxGetApp().UpdateFrameDarkUI(this);
    }
};

} // namespace

void AuiMgr::init(wxWindow* window)
{
    SetManagedWindow(window);
    SetDockSizeConstraint(1, 1);
#ifdef __WXGTK__
    // Floating and its docking hints need global pointer positions and window moves, which Wayland does not provide.
    if (is_running_on_wayland())
        SetFlags(GetFlags() & ~wxAUI_MGR_ALLOW_FLOATING);
#endif
    GetArtProvider()->SetMetric(wxAUI_DOCKART_CAPTION_SIZE, 18);
    GetArtProvider()->SetMetric(wxAUI_DOCKART_GRADIENT_TYPE, wxAUI_GRADIENT_NONE);
    apply_color_mode();
}

void AuiMgr::apply_color_mode()
{
    const bool     is_dark    = wxGetApp().dark_mode();
    const wxColour sash_color = is_dark ? wxColour(38, 46, 48) : wxColour(206, 206, 206);
    // Filuna (Direção Visual C): in light mode the caption that drags the sidebar out keeps working but
    // takes the panel colour, so it no longer reads as a grey bar above "Impressora"; edges use the
    // warm Filuna line colour.
    const wxColour caption_color = is_dark ? sash_color : wxColour(255, 253, 251);
    const wxColour line_color    = is_dark ? sash_color : wxColour(231, 227, 221);
    GetArtProvider()->SetColour(wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR, caption_color);
    GetArtProvider()->SetColour(wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR, is_dark ? *wxWHITE : wxColour(107, 101, 96));
    GetArtProvider()->SetColour(wxAUI_DOCKART_SASH_COLOUR, line_color);
    GetArtProvider()->SetColour(wxAUI_DOCKART_BORDER_COLOUR, is_dark ? *wxBLACK : line_color);
}

void AuiMgr::track_docked_size(wxWindow* window)
{
    window->Bind(wxEVT_IDLE, [this, window](wxIdleEvent& evt) {
        wxAuiPaneInfo& pane = GetPane(window);
        if (pane.IsOk() && pane.IsShown() && pane.IsDocked() && pane.rect.GetWidth() > 0 && pane.rect.GetHeight() > 0) {
            const bool horizontal = pane.dock_direction == wxAUI_DOCK_TOP || pane.dock_direction == wxAUI_DOCK_BOTTOM;
            pane.BestSize(horizontal ? pane.best_size.GetWidth() : pane.rect.GetWidth(),
                          horizontal ? pane.rect.GetHeight() : pane.best_size.GetHeight());
        }
        evt.Skip();
    });
}

wxAuiPaneInfo AuiMgr::sidebar_pane_info()
{
    return wxAuiPaneInfo()
        .Name("sidebar")
        .Left()
        .CloseButton(false)
        .TopDockable(false)
        .BottomDockable(false)
        .BestSize(wxSize(39 * wxGetApp().em_unit(), 90 * wxGetApp().em_unit()));
}

wxAuiFloatingFrame* AuiMgr::CreateFloatingFrame(wxWindow* parent, const wxAuiPaneInfo& pane)
{
    return new FloatFrame(parent, this, pane);
}

}} // namespace Slic3r::GUI
