#ifndef slic3r_FilunaCadUi_hpp_
#define slic3r_FilunaCadUi_hpp_

// Filuna CAD: the Design tab's tools as floating columns over the 3D view, instead of a ribbon on
// top and a form column on the side. Left column: project, groups, search, the tool families and,
// at the bottom, the view switches. Right column: snap, unit, view menu, display modes, screenshot
// and history; in a sketch, the constraints. A family opens a flyout beside its tile; a selection
// turns the left column into the actions that apply to it.
//
// Drawn with ImGui inside the canvas frame (DesignCanvas's overlay pass), so it moves, scales and
// hides with the canvas. Knows nothing about the CAD document: DesignPanel hands it a layout of
// items, each with its own callbacks, every frame.

#include <functional>
#include <map>
#include <string>
#include <vector>

#include <wx/string.h>

struct ImVec2;

namespace Slic3r { namespace GUI {

struct CadUiItem {
    std::string                 icon;      // resources/images/<icon>.svg, drawn white and tinted
    wxString                    label;
    std::function<wxString()>   sub;       // second line: a state ("Ligada") or the variant in use
    std::string                 keys;      // "Ctrl+Shift+V": shown as key chips
    std::function<void()>       run;       // click; null with `more` = the click opens the flyout
    std::function<bool()>       enabled;   // null = always
    std::function<bool()>       on;        // toggled: dark tile
    std::vector<CadUiItem>      more;      // flyout
    bool                        dot{false};// "something new here", as on Ferramentas
    // A right-column caption ("Restrição"): its tile opens nothing, the label is a title.
    bool                        caption{false};
};

using CadUiGroup = std::vector<CadUiItem>;

// A flyout panel that is not a list of tools: display modes, snap, units, views. Rows of toggles
// or choices, drawn as a card beside the tile that opened it.
struct CadUiRow {
    wxString              title;     // a section title when `pick` and `toggle` are both null
    std::function<bool()> active;    // choice: the current one; toggle: on
    std::function<void()> pick;      // a choice
    std::function<void()> toggle;    // a switch
};

struct CadUiSearchEntry {
    wxString              label;
    wxString              where;     // the family it lives in
    std::string           icon;
    std::string           keys;
    std::function<void()> run;
    bool                  enabled{true};
};

struct CadUiLayout {
    std::vector<CadUiGroup> left_top, left_mid, left_bottom;
    std::vector<CadUiGroup> right_top, right_mid;
    // Card flyouts for right-column tiles, by the tile's icon name.
    std::map<std::string, std::vector<CadUiRow>> cards;
    // Top centre: the tool in progress with its Cancel / Confirm, or a one-line hint.
    wxString              pill_title;
    wxString              pill_hint;
    std::function<void()> confirm, cancel;
    bool                  can_confirm{false};
    wxString              bottom_note;     // "1 face": what the selection is
    // Room the left and right side panels (Grupos, História) take, in canvas px: the columns
    // move in beside them.
    float                 left_inset{0.f}, right_inset{0.f};
};

class FilunaCadUi
{
public:
    // Inside the canvas's ImGui frame. `scale` follows the monitor's DPI (as the canvas toolbar).
    void render(const CadUiLayout& layout, float scale, bool dark);

    // The search field (Ctrl+F) holds the keyboard while it is open.
    void open_search(std::vector<CadUiSearchEntry> entries);
    void close_search();
    bool search_open() const { return m_search_open; }
    void search_move(int delta);       // arrow keys
    void search_run();                 // Enter
    bool wants_keyboard() const { return m_search_open; }

    bool flyout_open() const { return !m_flyout.empty(); }
    void close_flyout() { m_flyout.clear(); }
    // Width the right column takes at this scale (tiles + margin), for the view cube beside it.
    static float right_column_width(float scale);
    // Where the bottom-left column ends, so the status line can start past it.
    float left_column_right() const { return m_left_right; }
    // Asked by a click handler: did the last frame put a tile or card under this canvas point?
    bool covers(float x, float y) const;

private:
    struct Rect { float x0, y0, x1, y1; };
    void*  icon(const std::string& name);
    void   column(const char* id, const std::vector<CadUiGroup>& groups, float x, float y, bool right,
                  float scale, const CadUiLayout& layout);
    float  group_height(const CadUiGroup& g, float scale) const;
    void   flyout(const CadUiItem& item, float x, float y, float scale, bool right);
    void   card(const std::vector<CadUiRow>& rows, float x, float y, bool right, float scale);
    void   pill(const CadUiLayout& layout, float scale);
    void   search(float scale);

    std::map<std::string, void*> m_icons;
    std::string                  m_flyout;       // "<column>/<group>/<item>" of the open flyout
    std::vector<Rect>            m_hit;          // last frame's tiles and cards
    float                        m_left_right{0.f};
    bool                         m_dark{false};
    const CadUiItem*             m_pend_item{nullptr};   // this frame's open flyout, drawn last
    const std::vector<CadUiRow>* m_pend_card{nullptr};
    float                        m_pend_x{0.f}, m_pend_y{0.f};
    bool                         m_pend_right{false};
    float                        m_pend_s{1.f};

    bool                          m_search_open{false};
    bool                          m_search_focus{false};
    char                          m_search_text[96]{};
    int                           m_search_sel{0};
    std::vector<CadUiSearchEntry> m_search_all;
    std::vector<int>              m_search_hits;
};

}} // namespace Slic3r::GUI

#endif // slic3r_FilunaCadUi_hpp_
