#pragma once

#include "wx/wxprec.h"
#include "wx/aui/auibar.h"

#include "SelectMachine.hpp"

#include <wx/control.h>
#include <wx/string.h>
#include <wx/gdicmn.h>
#include <wx/event.h>
#include "slic3r/GUI/DeviceCore/DevDefs.h"

class wxFrame;
class wxMenuEvent;
class wxMenuItem;
class wxMouseCaptureLostEvent;
class wxMouseEvent;
class wxWindow;

// Filuna: height of the single navigation bar (logo, tabs, Exportar / Fatiar mesa, window buttons), in DIP.
constexpr int FILUNA_NAVBAR_HEIGHT = 56;

class CenteredTitle : public wxControl
{
public:
    CenteredTitle(wxWindow* parent);
    void SetTitle(const wxString& title);

    wxSize DoGetBestSize() const override;

protected:
#ifdef __WXMSW__
    WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) override;
#endif

private:
    wxString m_title;
};

class BBLTopbar : public wxAuiToolBar
{
public:
    BBLTopbar(wxWindow* pwin, wxFrame* parent);
    BBLTopbar(wxFrame* parent);
    void Init(wxFrame *parent);
    ~BBLTopbar();
    void UpdateToolbarWidth(int width);
    void Rescale();
    void OnIconize(wxAuiToolBarEvent& event);
    void OnFullScreen(wxAuiToolBarEvent& event);
    void OnCloseFrame(wxAuiToolBarEvent& event);
    void OnFileToolItem(wxAuiToolBarEvent& evt);
    void OnDropdownToolItem(wxAuiToolBarEvent& evt);
    void OnCalibToolItem(wxAuiToolBarEvent &evt);
    void OnMouseLeftDClock(wxMouseEvent& mouse);
    void OnMouseLeftDown(wxMouseEvent& event);
    void OnMouseLeftUp(wxMouseEvent& event);
    void OnMouseMotion(wxMouseEvent& event);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);
    void OnMenuClose(wxMenuEvent& event);
    void OnOpenProject(wxAuiToolBarEvent& event);
    void OnSaveProject(wxAuiToolBarEvent& event);
    void OnUndo(wxAuiToolBarEvent& event);
    void OnRedo(wxAuiToolBarEvent& event);
    void OnModelStoreClicked(wxAuiToolBarEvent& event);

    wxAuiToolBarItem* FindToolByCurrentPosition();
	
    void SetFileMenu(wxMenu* file_menu);
    void AddDropDownSubMenu(wxMenu* sub_menu, const wxString& title);
    void AddDropDownMenuItem(wxMenuItem* menu_item);
    wxMenu *GetTopMenu();
    wxMenu *GetCalibMenu();
    void SetTitle(wxString title);
    void SetMaximizedSize();
    void SetWindowSize();

    void EnableUndoRedoItems();
    void DisableUndoRedoItems();
    void EnableUndoRedo(bool undo, bool redo);

    void SaveNormalRect();

    void ShowCalibrationButton(bool show = true);

    // Filuna: the main tabs are moved into this panel, so the whole window has one navigation bar.
    wxWindow* GetTabsHost() const { return m_tabs_host; }

protected:
#ifdef __WXMSW__
    WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) override;
#endif

private:
    wxFrame* m_frame;
    wxAuiToolBarItem* m_file_menu_item;
    wxAuiToolBarItem* m_dropdown_menu_item;
    wxRect m_normalRect;
    wxPoint m_delta;
    wxPoint m_last_mouse_position{wxDefaultPosition};
    wxMenu m_top_menu;
    wxMenu* m_file_menu;
    wxMenu m_calib_menu;
    
    CenteredTitle*    m_title_ctrl { nullptr };
    wxString          m_titleText;

    
    wxAuiToolBarItem* m_undo_item { nullptr };
    wxAuiToolBarItem* m_redo_item { nullptr };
    wxAuiToolBarItem* m_calib_item { nullptr };
    wxAuiToolBarItem* maximize_btn { nullptr };
    wxWindow*         m_tabs_host { nullptr };


    wxBitmap maximize_bitmap;
    wxBitmap window_bitmap;

    int m_toolbar_h;
    bool m_skip_popup_file_menu;
    bool m_skip_popup_dropdown_menu;
    bool m_skip_popup_calib_menu;
};
