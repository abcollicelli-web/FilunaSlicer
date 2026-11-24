#include "FilunaCadUi.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>

#include <boost/algorithm/string/case_conv.hpp>

#include "libslic3r/Utils.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/ImGuiWrapper.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace Slic3r { namespace GUI {

namespace {

// Sizes at scale 1, in canvas px.
constexpr float kTile    = 32.f;   // one tool tile
constexpr float kPad     = 3.f;    // card padding around the tiles
constexpr float kGap     = 10.f;   // between two cards of one column
constexpr float kMargin  = 10.f;   // from the canvas edge
constexpr float kRadius  = 10.f;
constexpr float kLabelGap = 9.f;   // tile column -> label

struct Palette {
    ImU32 card, border, shadow, tile_hover, tile_on, icon, icon_on, icon_off, text, sub, chip_bg,
          chip_text, label_bg, accent, accent_text;
};

Palette palette(bool dark)
{
    if (dark)
        return { IM_COL32(42, 42, 46, 255),   IM_COL32(64, 64, 70, 255),   IM_COL32(0, 0, 0, 60),
                 IM_COL32(58, 58, 64, 255),   IM_COL32(242, 242, 244, 255), IM_COL32(230, 230, 233, 255),
                 IM_COL32(28, 28, 30, 255),   IM_COL32(230, 230, 233, 80),  IM_COL32(237, 237, 240, 255),
                 IM_COL32(154, 154, 160, 255), IM_COL32(60, 60, 66, 255),   IM_COL32(170, 170, 176, 255),
                 IM_COL32(30, 30, 34, 190),   IM_COL32(194, 24, 91, 255),  IM_COL32(255, 255, 255, 255) };
    return { IM_COL32(255, 255, 255, 255), IM_COL32(226, 226, 230, 255), IM_COL32(0, 0, 0, 22),
             IM_COL32(240, 240, 243, 255), IM_COL32(28, 28, 30, 255),    IM_COL32(44, 44, 46, 255),
             IM_COL32(255, 255, 255, 255), IM_COL32(44, 44, 46, 70),     IM_COL32(28, 28, 30, 255),
             IM_COL32(142, 142, 147, 255), IM_COL32(236, 236, 240, 255), IM_COL32(138, 138, 142, 255),
             IM_COL32(246, 246, 248, 205), IM_COL32(194, 24, 91, 255),   IM_COL32(255, 255, 255, 255) };
}

std::string u8(const wxString& s) { return std::string(s.ToUTF8().data()); }

ImFont* bold()
{
    ImGuiWrapper* w = wxGetApp().imgui();
    ImFont*       f = ImGui::GetFont();
    if (w != nullptr && w->push_bold_font()) {
        f = ImGui::GetFont();
        w->pop_bold_font();
    }
    return f;
}

std::vector<std::string> split_keys(const std::string& keys)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : keys) {
        if (c == '+' && !cur.empty()) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

const int kWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse;

// A white rounded card with the soft shadow the tiles sit on.
void draw_card(ImDrawList* dl, ImVec2 a, ImVec2 b, float r, const Palette& p)
{
    for (int i = 3; i >= 1; --i) {
        const float o = float(i);
        dl->AddRectFilled(ImVec2(a.x - o * 0.5f, a.y + o * 0.6f), ImVec2(b.x + o * 0.5f, b.y + o * 1.4f), p.shadow, r + o);
    }
    dl->AddRectFilled(a, b, p.card, r);
    dl->AddRect(a, b, p.border, r, 0, 1.f);
}

struct Text {
    std::string s;
    ImFont*     font;
    float       size;
    ImVec2      dim;
};

Text make_text(const std::string& s, ImFont* font, float size)
{
    return { s, font, size, font->CalcTextSizeA(size, FLT_MAX, 0.f, s.c_str()) };
}

} // namespace

void* FilunaCadUi::icon(const std::string& name)
{
    if (name.empty()) return nullptr;
    auto it = m_icons.find(name);
    if (it != m_icons.end()) return it->second;
    ImTextureID id = nullptr;
    // Twice the drawn size, so the white line art stays crisp at 150 % and 200 %.
    if (!IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/" + name + ".svg", 64, 64, id))
        id = nullptr;
    m_icons[name] = id;
    return id;
}

float FilunaCadUi::group_height(const CadUiGroup& g, float s) const
{
    return (float(g.size()) * kTile + 2.f * kPad) * s;
}

float FilunaCadUi::right_column_width(float s)
{
    return (kTile + 2.f * kPad + kMargin + 6.f) * s;
}

bool FilunaCadUi::covers(float x, float y) const
{
    for (const Rect& r : m_hit)
        if (x >= r.x0 && x <= r.x1 && y >= r.y0 && y <= r.y1) return true;
    return false;
}

void FilunaCadUi::column(const char* id, const std::vector<CadUiGroup>& groups, float x, float y, bool right,
                         float s, const CadUiLayout& layout)
{
    const Palette p     = palette(m_dark);
    const float   tile  = kTile * s;
    const float   pad   = kPad * s;
    const float   fsize = ImGui::GetFontSize();
    ImFont*       fb    = bold();
    ImFont*       fr    = ImGui::GetFont();
    const bool    flyout_here = !m_flyout.empty() && m_flyout.rfind(std::string(id) + "/", 0) == 0;

    for (size_t gi = 0; gi < groups.size(); ++gi) {
        const CadUiGroup& g = groups[gi];
        if (g.empty()) continue;
        const float h = group_height(g, s);

        // The labels decide the window's width: tile column + gap + the widest row.
        float label_w = 0.f;
        std::vector<Text> labels, subs;
        std::vector<std::vector<Text>> chips;
        for (const CadUiItem& it : g) {
            labels.push_back(make_text(u8(it.label), fb, fsize));
            subs.push_back(make_text(it.sub ? u8(it.sub()) : std::string(), fr, fsize * 0.80f));
            std::vector<Text> c;
            float cw = 0.f;
            for (const std::string& k : split_keys(it.keys)) {
                c.push_back(make_text(k, fr, fsize * 0.78f));
                cw += c.back().dim.x + 9.f * s;
            }
            chips.push_back(c);
            label_w = std::max(label_w, std::max(labels.back().dim.x + (cw > 0.f ? cw + 4.f * s : 0.f), subs.back().dim.x) +
                                            (it.dot ? 12.f * s : 0.f) + 16.f * s);
        }
        const bool  hide_labels = flyout_here;   // the flyout takes the labels' place, as in the reference
        const float card_w      = tile + 2.f * pad;
        const float win_w       = card_w + (hide_labels ? 0.f : kLabelGap * s + label_w);
        const float win_x       = right ? x - win_w : x;
        const float card_x      = right ? x - card_w : x;

        ImGui::SetNextWindowPos(ImVec2(win_x, y));
        ImGui::SetNextWindowSize(ImVec2(win_w, h));
        const std::string wid = std::string("##fcad_") + id + std::to_string(gi);
        ImGui::Begin(wid.c_str(), nullptr, kWindowFlags);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        draw_card(dl, ImVec2(card_x, y), ImVec2(card_x + card_w, y + h), kRadius * s, p);
        m_hit.push_back({ card_x, y, card_x + card_w, y + h });

        for (size_t ii = 0; ii < g.size(); ++ii) {
            const CadUiItem& it      = g[ii];
            const float      ty      = y + pad + float(ii) * tile;
            const ImVec2     ta(card_x + pad, ty), tb(card_x + pad + tile, ty + tile);
            const bool       enabled = !it.enabled || it.enabled();
            const bool       on      = it.on && it.on();
            const std::string key    = std::string(id) + "/" + std::to_string(gi) + "/" + std::to_string(ii);

            // The whole row is the button: tile and label, as in the reference.
            const float row_x0 = right ? (hide_labels ? card_x : win_x) : card_x;
            const float row_x1 = right ? card_x + card_w : (hide_labels ? card_x + card_w : win_x + win_w);
            ImGui::SetCursorScreenPos(ImVec2(row_x0, ty));
            ImGui::InvisibleButton((key + "##b").c_str(), ImVec2(std::max(row_x1 - row_x0, 1.f), tile));
            const bool hovered = ImGui::IsItemHovered() && !it.caption;
            const bool clicked = ImGui::IsItemClicked(0) && !it.caption;
            if (!hide_labels && !right && !labels[ii].s.empty())
                m_hit.push_back({ card_x + card_w, ty, win_x + win_w, ty + tile });
            if (!hide_labels && right && !labels[ii].s.empty())
                m_hit.push_back({ win_x, ty, card_x, ty + tile });

            if (on)
                dl->AddRectFilled(ImVec2(ta.x + 1.f, ta.y + 1.f), ImVec2(tb.x - 1.f, tb.y - 1.f), p.tile_on, 7.f * s);
            else if (hovered && enabled)
                dl->AddRectFilled(ImVec2(ta.x + 1.f, ta.y + 1.f), ImVec2(tb.x - 1.f, tb.y - 1.f), p.tile_hover, 7.f * s);
            if (void* tex = icon(it.icon)) {
                const float  is = 19.f * s;
                const ImVec2 ia(ta.x + (tile - is) * 0.5f, ta.y + (tile - is) * 0.5f);
                dl->AddImage(tex, ia, ImVec2(ia.x + is, ia.y + is), ImVec2(0, 0), ImVec2(1, 1),
                             on ? p.icon_on : (enabled ? p.icon : p.icon_off));
            }
            // A family: the small corner mark that says "more inside".
            if (!it.more.empty()) {
                const ImVec2 c(tb.x - 4.f * s, tb.y - 4.f * s);
                dl->AddTriangleFilled(ImVec2(c.x, c.y - 5.f * s), ImVec2(c.x, c.y), ImVec2(c.x - 5.f * s, c.y),
                                      on ? p.icon_on : p.sub);
            }
            if (!hide_labels && !labels[ii].s.empty()) {
                const Text& l = labels[ii];
                const Text& sb = subs[ii];
                float cw = 0.f;
                for (const Text& c : chips[ii]) cw += c.dim.x + 9.f * s;
                const float tw  = std::max(l.dim.x + (cw > 0.f ? cw + 4.f * s : 0.f) + (it.dot ? 12.f * s : 0.f), sb.dim.x);
                const float th  = sb.s.empty() ? l.dim.y : l.dim.y + sb.dim.y;
                const float lx  = right ? card_x - kLabelGap * s - tw : card_x + card_w + kLabelGap * s;
                const float ly  = ty + (tile - th) * 0.5f;
                dl->AddRectFilled(ImVec2(lx - 5.f * s, ly - 2.f * s), ImVec2(lx + tw + 5.f * s, ly + th + 2.f * s), p.label_bg, 6.f * s);
                const ImU32 tc = enabled ? p.text : p.icon_off;
                float       cx = lx;
                if (right) {   // right column: chips first, then the label, flush right
                    cx = lx + tw - l.dim.x - cw - (cw > 0.f ? 4.f * s : 0.f);
                    for (const Text& c : chips[ii]) {
                        dl->AddRectFilled(ImVec2(cx, ly + 1.f * s), ImVec2(cx + c.dim.x + 6.f * s, ly + l.dim.y - 1.f * s), p.chip_bg, 3.f * s);
                        dl->AddText(c.font, c.size, ImVec2(cx + 3.f * s, ly + (l.dim.y - c.dim.y) * 0.5f), p.chip_text, c.s.c_str());
                        cx += c.dim.x + 9.f * s;
                    }
                    if (cw > 0.f) cx += 4.f * s - 3.f * s;
                    dl->AddText(l.font, l.size, ImVec2(lx + tw - l.dim.x, ly), tc, l.s.c_str());
                    if (!sb.s.empty())
                        dl->AddText(sb.font, sb.size, ImVec2(lx + tw - sb.dim.x, ly + l.dim.y), p.sub, sb.s.c_str());
                } else {
                    dl->AddText(l.font, l.size, ImVec2(lx, ly), tc, l.s.c_str());
                    cx = lx + l.dim.x + 6.f * s;
                    for (const Text& c : chips[ii]) {
                        dl->AddRectFilled(ImVec2(cx, ly + 1.f * s), ImVec2(cx + c.dim.x + 6.f * s, ly + l.dim.y - 1.f * s), p.chip_bg, 3.f * s);
                        dl->AddText(c.font, c.size, ImVec2(cx + 3.f * s, ly + (l.dim.y - c.dim.y) * 0.5f), p.chip_text, c.s.c_str());
                        cx += c.dim.x + 9.f * s;
                    }
                    if (it.dot)
                        dl->AddCircleFilled(ImVec2(cx + 3.f * s, ly + l.dim.y * 0.5f), 3.f * s, p.accent);
                    if (!sb.s.empty())
                        dl->AddText(sb.font, sb.size, ImVec2(lx, ly + l.dim.y), p.sub, sb.s.c_str());
                }
            }
            if (hovered && !enabled && !it.label.IsEmpty())
                ImGui::SetTooltip("%s", u8(it.label).c_str());

            if (clicked && enabled) {
                if (!it.more.empty() && !it.run) {
                    m_flyout = (m_flyout == key) ? std::string() : key;
                } else if (!it.more.empty() && it.run) {
                    // A tool with variants: the body runs the one in use, the corner opens the rest.
                    const ImVec2 m = ImGui::GetIO().MousePos;
                    if (m.x > tb.x - tile * 0.38f && m.y > tb.y - tile * 0.38f && m.x <= tb.x && m.y <= tb.y)
                        m_flyout = (m_flyout == key) ? std::string() : key;
                    else { m_flyout.clear(); it.run(); }
                } else if (it.run) {
                    const bool is_card = layout.cards.count(it.icon) > 0;
                    if (is_card) m_flyout = (m_flyout == key) ? std::string() : key;
                    else { m_flyout.clear(); it.run(); }
                } else if (layout.cards.count(it.icon) > 0) {
                    m_flyout = (m_flyout == key) ? std::string() : key;
                }
            }
            if (m_flyout == key) {   // drawn after every column, so it lies on top of them
                auto cit = layout.cards.find(it.icon);
                m_pend_item  = &it;
                m_pend_card  = cit != layout.cards.end() ? &cit->second : nullptr;
                m_pend_x     = right ? card_x - 8.f * s : card_x + card_w + 8.f * s;
                m_pend_y     = ty - pad;
                m_pend_right = right;
                m_pend_s     = s;
            }
        }
        ImGui::End();
        y += h + kGap * s;
        if (!right) m_left_right = std::max(m_left_right, card_x + card_w);
    }
}

void FilunaCadUi::flyout(const CadUiItem& parent, float x, float y, float s, bool right)
{
    const Palette p      = palette(m_dark);
    const float   tile   = kTile * s;
    const float   pad    = kPad * s;
    const float   fsize  = ImGui::GetFontSize();
    ImFont*       fb     = bold();
    ImFont*       fr     = ImGui::GetFont();
    const float   top    = kMargin * s;
    const float   bottom = ImGui::GetIO().DisplaySize.y - kMargin * s;
    // A long family (Ferramentas) wraps into a second column rather than running off the screen.
    const int     n      = int(parent.more.size());
    const int     cols   = std::max(1, int(std::ceil((float(n) * tile + 2.f * pad) / std::max(bottom - top, tile * 4.f))));
    const int     per    = (n + cols - 1) / cols;
    const float   h      = float(per) * tile + 2.f * pad;
    if (y + h > bottom) y = std::max(top, bottom - h);

    float label_w = 0.f;
    for (const CadUiItem& it : parent.more) {
        float w = fb->CalcTextSizeA(fsize, FLT_MAX, 0.f, u8(it.label).c_str()).x;
        for (const std::string& k : split_keys(it.keys))
            w += fr->CalcTextSizeA(fsize * 0.78f, FLT_MAX, 0.f, k.c_str()).x + 9.f * s;
        if (it.sub) w = std::max(w, fr->CalcTextSizeA(fsize * 0.8f, FLT_MAX, 0.f, u8(it.sub()).c_str()).x);
        label_w = std::max(label_w, w + 22.f * s);
    }
    const float card_w = tile + 2.f * pad;
    const float col_w  = card_w + kLabelGap * s + label_w;
    const float win_w  = float(cols) * col_w + float(cols - 1) * 6.f * s;
    const float win_x  = right ? x - win_w : x;
    ImGui::SetNextWindowPos(ImVec2(win_x, y));
    ImGui::SetNextWindowSize(ImVec2(win_w, h));
    ImGui::Begin("##fcad_flyout", nullptr, kWindowFlags);
    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
    ImDrawList* dl = ImGui::GetWindowDrawList();
    m_hit.push_back({ win_x, y, win_x + win_w, y + h });
    for (int c = 0; c < cols; ++c) {
        // Right column: tiles on the right of each block, labels to their left.
        const float bx     = right ? win_x + win_w - float(c + 1) * col_w - float(c) * 6.f * s : win_x + float(c) * (col_w + 6.f * s);
        const float card_x = right ? bx + col_w - card_w : bx;
        const int   first  = c * per, last = std::min(n, first + per);
        if (first >= last) break;
        draw_card(dl, ImVec2(card_x, y), ImVec2(card_x + card_w, y + float(last - first) * tile + 2.f * pad), kRadius * s, p);
        for (int i = first; i < last; ++i) {
            const CadUiItem& it      = parent.more[size_t(i)];
            const float      ty      = y + pad + float(i - first) * tile;
            const bool       enabled = !it.enabled || it.enabled();
            const bool       on      = it.on && it.on();
            ImGui::SetCursorScreenPos(ImVec2(bx, ty));
            ImGui::InvisibleButton(("##fly" + std::to_string(i)).c_str(), ImVec2(col_w, tile));
            const bool   hovered = ImGui::IsItemHovered();
            const ImVec2 ta(card_x + pad, ty), tb(card_x + pad + tile, ty + tile);
            if (on)
                dl->AddRectFilled(ImVec2(ta.x + 1, ta.y + 1), ImVec2(tb.x - 1, tb.y - 1), p.tile_on, 7.f * s);
            else if (hovered && enabled)
                dl->AddRectFilled(ImVec2(ta.x + 1, ta.y + 1), ImVec2(tb.x - 1, tb.y - 1), p.tile_hover, 7.f * s);
            if (void* tex = icon(it.icon)) {
                const float  is = 19.f * s;
                const ImVec2 ia(ta.x + (tile - is) * 0.5f, ta.y + (tile - is) * 0.5f);
                dl->AddImage(tex, ia, ImVec2(ia.x + is, ia.y + is), ImVec2(0, 0), ImVec2(1, 1),
                             on ? p.icon_on : (enabled ? p.icon : p.icon_off));
            }
            const Text  l  = make_text(u8(it.label), fb, fsize);
            const Text  sb = make_text(it.sub ? u8(it.sub()) : std::string(), fr, fsize * 0.8f);
            const float th = sb.s.empty() ? l.dim.y : l.dim.y + sb.dim.y;
            std::vector<Text> chips;
            float tw = l.dim.x;
            for (const std::string& k : split_keys(it.keys)) { chips.push_back(make_text(k, fr, fsize * 0.78f)); tw += chips.back().dim.x + 9.f * s; }
            if (!chips.empty()) tw += 4.f * s;
            if (it.dot) tw += 12.f * s;
            const float bw = std::max(tw, sb.dim.x);
            const float lx = right ? card_x - kLabelGap * s - bw : card_x + card_w + kLabelGap * s;
            const float ly = ty + (tile - th) * 0.5f;
            dl->AddRectFilled(ImVec2(lx - 5.f * s, ly - 2.f * s), ImVec2(lx + bw + 5.f * s, ly + th + 2.f * s), p.label_bg, 6.f * s);
            dl->AddText(l.font, l.size, ImVec2(lx, ly), enabled ? p.text : p.icon_off, l.s.c_str());
            float cx = lx + l.dim.x + 6.f * s;
            for (const Text& c2 : chips) {
                dl->AddRectFilled(ImVec2(cx, ly + 1.f * s), ImVec2(cx + c2.dim.x + 6.f * s, ly + l.dim.y - 1.f * s), p.chip_bg, 3.f * s);
                dl->AddText(c2.font, c2.size, ImVec2(cx + 3.f * s, ly + (l.dim.y - c2.dim.y) * 0.5f), p.chip_text, c2.s.c_str());
                cx += c2.dim.x + 9.f * s;
            }
            if (it.dot) dl->AddCircleFilled(ImVec2(cx + 3.f * s, ly + l.dim.y * 0.5f), 3.f * s, p.accent);
            if (!sb.s.empty()) dl->AddText(sb.font, sb.size, ImVec2(lx, ly + l.dim.y), p.sub, sb.s.c_str());
            if (ImGui::IsItemClicked(0) && enabled && it.run) {
                m_flyout.clear();
                it.run();
            }
        }
    }
    ImGui::End();
}

void FilunaCadUi::card(const std::vector<CadUiRow>& rows, float x, float y, bool right, float s)
{
    const Palette p     = palette(m_dark);
    const float   fsize = ImGui::GetFontSize();
    ImFont*       fb    = bold();
    ImFont*       fr    = ImGui::GetFont();
    const float   row_h = 30.f * s;
    const float   pad   = 12.f * s;
    float         w     = 0.f;
    for (const CadUiRow& r : rows)
        w = std::max(w, (r.pick || r.toggle ? fr : fb)->CalcTextSizeA(fsize, FLT_MAX, 0.f, u8(r.title).c_str()).x +
                            (r.toggle ? 64.f * s : 24.f * s));
    w = std::max(w + 2.f * pad, 220.f * s);
    const float h = float(rows.size()) * row_h + 2.f * pad;
    const float bottom = ImGui::GetIO().DisplaySize.y - kMargin * s;
    if (y + h > bottom) y = std::max(kMargin * s, bottom - h);
    const float x0 = right ? x - w : x;

    ImGui::SetNextWindowPos(ImVec2(x0, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("##fcad_card", nullptr, kWindowFlags);
    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
    ImDrawList* dl = ImGui::GetWindowDrawList();
    draw_card(dl, ImVec2(x0, y), ImVec2(x0 + w, y + h), kRadius * s, p);
    m_hit.push_back({ x0, y, x0 + w, y + h });
    for (size_t i = 0; i < rows.size(); ++i) {
        const CadUiRow& r  = rows[i];
        const float     ry = y + pad + float(i) * row_h;
        const std::string t = u8(r.title);
        if (!r.pick && !r.toggle) {   // section title
            dl->AddText(fb, fsize, ImVec2(x0 + pad, ry + (row_h - fsize) * 0.5f), p.text, t.c_str());
            continue;
        }
        ImGui::SetCursorScreenPos(ImVec2(x0 + 4.f * s, ry));
        ImGui::InvisibleButton(("##row" + std::to_string(i)).c_str(), ImVec2(w - 8.f * s, row_h));
        const bool hovered = ImGui::IsItemHovered();
        const bool active  = r.active && r.active();
        if (hovered || (active && r.pick))
            dl->AddRectFilled(ImVec2(x0 + 4.f * s, ry + 1.f), ImVec2(x0 + w - 4.f * s, ry + row_h - 1.f), p.tile_hover, 6.f * s);
        if (active && r.pick)
            dl->AddRectFilled(ImVec2(x0 + 6.f * s, ry + 7.f * s), ImVec2(x0 + 8.f * s, ry + row_h - 7.f * s), p.accent, 1.f);
        dl->AddText(fr, fsize, ImVec2(x0 + pad + 8.f * s, ry + (row_h - fsize) * 0.5f), p.text, t.c_str());
        if (r.toggle) {   // a switch, Filuna magenta when on
            const float  sw = 34.f * s, sh = 18.f * s;
            const ImVec2 a(x0 + w - pad - sw, ry + (row_h - sh) * 0.5f);
            dl->AddRectFilled(a, ImVec2(a.x + sw, a.y + sh), active ? p.accent : p.chip_bg, sh * 0.5f);
            const float kx = active ? a.x + sw - sh * 0.5f : a.x + sh * 0.5f;
            dl->AddCircleFilled(ImVec2(kx, a.y + sh * 0.5f), sh * 0.5f - 2.f * s, IM_COL32(255, 255, 255, 255));
        }
        if (ImGui::IsItemClicked(0)) {
            if (r.toggle) r.toggle();
            else if (r.pick) { r.pick(); m_flyout.clear(); }
        }
    }
    ImGui::End();
}

void FilunaCadUi::pill(const CadUiLayout& l, float s)
{
    if (l.pill_title.IsEmpty() && l.pill_hint.IsEmpty()) return;
    const Palette p     = palette(m_dark);
    const float   fsize = ImGui::GetFontSize();
    ImFont*       fb    = bold();
    ImFont*       fr    = ImGui::GetFont();
    const ImVec2  ds    = ImGui::GetIO().DisplaySize;
    const Text    title = make_text(u8(l.pill_title), fb, fsize);
    const Text    hint  = make_text(u8(l.pill_hint), fr, fsize * 0.9f);
    const bool    tool  = !l.pill_title.IsEmpty() && (l.confirm || l.cancel);
    const Text    bc    = make_text(u8(_L("Cancelar")), fb, fsize);
    const Text    bk    = make_text(u8(_L("Confirmar")), fb, fsize);
    const float   pad   = 12.f * s;
    const float   bh    = 26.f * s;
    float w = pad;
    if (!title.s.empty()) w += title.dim.x + pad;
    if (!hint.s.empty()) w += hint.dim.x + pad;
    if (tool) w += bc.dim.x + bk.dim.x + 4.f * pad + 6.f * s;
    const float h = bh + 12.f * s;
    const float x = (ds.x - w) * 0.5f, y = kMargin * s;

    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("##fcad_pill", nullptr, kWindowFlags);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    draw_card(dl, ImVec2(x, y), ImVec2(x + w, y + h), h * 0.5f, p);
    m_hit.push_back({ x, y, x + w, y + h });
    float cx = x + pad;
    const float cy = y + h * 0.5f;
    if (!title.s.empty()) {
        dl->AddText(title.font, title.size, ImVec2(cx, cy - title.dim.y * 0.5f), p.text, title.s.c_str());
        cx += title.dim.x + pad;
    }
    if (!hint.s.empty()) {
        dl->AddText(hint.font, hint.size, ImVec2(cx, cy - hint.dim.y * 0.5f), p.sub, hint.s.c_str());
        cx += hint.dim.x + pad;
    }
    if (tool) {
        auto button = [&](const Text& t, bool primary, bool enabled, const char* id) {
            const float bw = t.dim.x + 2.f * pad;
            const ImVec2 a(cx, cy - bh * 0.5f), b(cx + bw, cy + bh * 0.5f);
            ImGui::SetCursorScreenPos(a);
            ImGui::InvisibleButton(id, ImVec2(bw, bh));
            const bool hov = ImGui::IsItemHovered() && enabled;
            if (primary)
                dl->AddRectFilled(a, b, enabled ? (hov ? IM_COL32(168, 18, 78, 255) : p.accent) : p.chip_bg, bh * 0.5f);
            else
                dl->AddRectFilled(a, b, hov ? p.tile_hover : p.chip_bg, bh * 0.5f);
            dl->AddText(t.font, t.size, ImVec2(a.x + pad, cy - t.dim.y * 0.5f),
                        primary ? (enabled ? p.accent_text : p.icon_off) : p.text, t.s.c_str());
            cx += bw + 6.f * s;
            return ImGui::IsItemClicked(0) && enabled;
        };
        if (button(bc, false, bool(l.cancel), "##fcad_cancel") && l.cancel) l.cancel();
        if (button(bk, true, l.can_confirm && bool(l.confirm), "##fcad_ok") && l.confirm) l.confirm();
    }
    ImGui::End();
}

void FilunaCadUi::open_search(std::vector<CadUiSearchEntry> entries)
{
    m_search_all   = std::move(entries);
    m_search_open  = true;
    m_search_focus = true;
    m_search_sel   = 0;
    m_search_text[0] = '\0';
    m_flyout.clear();
}

void FilunaCadUi::close_search()
{
    m_search_open = false;
    m_search_all.clear();
    m_search_hits.clear();
}

void FilunaCadUi::search_move(int delta)
{
    if (m_search_hits.empty()) return;
    m_search_sel = std::clamp(m_search_sel + delta, 0, int(m_search_hits.size()) - 1);
}

void FilunaCadUi::search_run()
{
    if (m_search_sel < 0 || m_search_sel >= int(m_search_hits.size())) return;
    const CadUiSearchEntry e = m_search_all[m_search_hits[m_search_sel]];
    close_search();
    if (e.enabled && e.run) e.run();
}

void FilunaCadUi::search(float s)
{
    if (!m_search_open) return;
    const Palette p     = palette(m_dark);
    const ImVec2  ds    = ImGui::GetIO().DisplaySize;
    const float   fsize = ImGui::GetFontSize();
    ImFont*       fb    = bold();
    ImFont*       fr    = ImGui::GetFont();
    const float   w     = std::min(460.f * s, ds.x - 40.f * s);
    const float   row_h = 32.f * s;

    // Filter: every word typed must appear in the name or the family, accents and case aside.
    std::string q = boost::algorithm::to_lower_copy(std::string(m_search_text));
    m_search_hits.clear();
    for (size_t i = 0; i < m_search_all.size(); ++i) {
        const std::string hay = boost::algorithm::to_lower_copy(u8(m_search_all[i].label) + " " + u8(m_search_all[i].where));
        bool ok = true;
        size_t a = 0;
        while (ok && a < q.size()) {
            size_t b = q.find(' ', a);
            if (b == std::string::npos) b = q.size();
            if (b > a && hay.find(q.substr(a, b - a)) == std::string::npos) ok = false;
            a = b + 1;
        }
        if (ok) m_search_hits.push_back(int(i));
        if (m_search_hits.size() >= 12) break;
    }
    m_search_sel = std::clamp(m_search_sel, 0, std::max(0, int(m_search_hits.size()) - 1));

    const float h = 52.f * s + float(m_search_hits.size()) * row_h + 10.f * s;
    const float x = (ds.x - w) * 0.5f, y = 58.f * s;
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("##fcad_search", nullptr, kWindowFlags);
    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
    ImDrawList* dl = ImGui::GetWindowDrawList();
    draw_card(dl, ImVec2(x, y), ImVec2(x + w, y + h), kRadius * s, p);
    m_hit.push_back({ x, y, x + w, y + h });

    ImGui::SetCursorScreenPos(ImVec2(x + 12.f * s, y + 10.f * s));
    ImGui::PushItemWidth(w - 24.f * s);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(p.chip_bg));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(p.text));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f * s);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.f * s, 7.f * s));
    if (m_search_focus) { ImGui::SetKeyboardFocusHere(); m_search_focus = false; }
    ImGui::InputTextWithHint("##fcad_q", u8(_L("Pesquisar ferramentas")).c_str(), m_search_text, sizeof(m_search_text));
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    ImGui::PopItemWidth();

    float ry = y + 52.f * s;
    for (size_t k = 0; k < m_search_hits.size(); ++k) {
        const CadUiSearchEntry& e = m_search_all[m_search_hits[k]];
        ImGui::SetCursorScreenPos(ImVec2(x + 6.f * s, ry));
        ImGui::InvisibleButton(("##sr" + std::to_string(k)).c_str(), ImVec2(w - 12.f * s, row_h));
        if (ImGui::IsItemHovered()) m_search_sel = int(k);
        if (int(k) == m_search_sel)
            dl->AddRectFilled(ImVec2(x + 6.f * s, ry), ImVec2(x + w - 6.f * s, ry + row_h), p.tile_hover, 6.f * s);
        if (void* tex = icon(e.icon)) {
            const float is = 20.f * s;
            dl->AddImage(tex, ImVec2(x + 14.f * s, ry + (row_h - is) * 0.5f), ImVec2(x + 14.f * s + is, ry + (row_h + is) * 0.5f),
                         ImVec2(0, 0), ImVec2(1, 1), e.enabled ? p.icon : p.icon_off);
        }
        const std::string l = u8(e.label), wh = u8(e.where);
        dl->AddText(fb, fsize, ImVec2(x + 44.f * s, ry + (row_h - fsize) * 0.5f), e.enabled ? p.text : p.icon_off, l.c_str());
        const float lw = fb->CalcTextSizeA(fsize, FLT_MAX, 0.f, l.c_str()).x;
        dl->AddText(fr, fsize * 0.85f, ImVec2(x + 52.f * s + lw, ry + (row_h - fsize * 0.85f) * 0.5f), p.sub, wh.c_str());
        if (!e.keys.empty()) {
            const Text c = make_text(e.keys, fr, fsize * 0.78f);
            const float cx = x + w - 18.f * s - c.dim.x;
            dl->AddRectFilled(ImVec2(cx - 3.f * s, ry + 8.f * s), ImVec2(cx + c.dim.x + 3.f * s, ry + row_h - 8.f * s), p.chip_bg, 3.f * s);
            dl->AddText(c.font, c.size, ImVec2(cx, ry + (row_h - c.dim.y) * 0.5f), p.chip_text, c.s.c_str());
        }
        if (ImGui::IsItemClicked(0)) { m_search_sel = int(k); ImGui::End(); search_run(); return; }
        ry += row_h;
    }
    ImGui::End();
}

void FilunaCadUi::render(const CadUiLayout& l, float s, bool dark)
{
    m_dark = dark;
    m_hit.clear();
    m_left_right = 0.f;
    const ImVec2 ds = ImGui::GetIO().DisplaySize;

    // A click on the model closes whatever flyout is open, as clicking away does in the reference.
    if (!m_flyout.empty() && ImGui::IsMouseClicked(0) && !ImGui::GetIO().WantCaptureMouse)
        m_flyout.clear();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));

    const float lx = kMargin * s + l.left_inset;
    const float rx = ds.x - kMargin * s - l.right_inset;
    const float full = s;
    // A column that would not fit the canvas height shrinks as a whole, never runs off the bottom.
    auto needed = [this](const std::vector<const std::vector<CadUiGroup>*>& parts, float sc, float extra) {
        float h = kMargin * sc * 2.f + extra * sc;
        for (const auto* part : parts)
            for (const CadUiGroup& g : *part)
                if (!g.empty()) h += group_height(g, sc) + kGap * sc;
        return h;
    };
    const float need_l = needed({ &l.left_top, &l.left_mid, &l.left_bottom }, full, 2.f * kGap);
    const float need_r = needed({ &l.right_top, &l.right_mid }, full, kGap);
    const float sl = need_l > ds.y ? std::max(full * 0.55f, full * ds.y / need_l) : full;
    const float sr = need_r > ds.y ? std::max(full * 0.55f, full * ds.y / need_r) : full;

    // Left: the top cards from the top, the bottom cards from the bottom, the tools in between —
    // centred in the room left, or right under the top cards when there is no room to centre.
    float y = kMargin * sl;
    column("lt", l.left_top, lx, y, false, sl, l);
    float top_end = y;
    for (const CadUiGroup& g : l.left_top) if (!g.empty()) top_end += group_height(g, sl) + kGap * sl;
    float bot_h = 0.f;
    for (const CadUiGroup& g : l.left_bottom) if (!g.empty()) bot_h += group_height(g, sl) + kGap * sl;
    float mid_h = 0.f;
    for (const CadUiGroup& g : l.left_mid) if (!g.empty()) mid_h += group_height(g, sl) + kGap * sl;
    const float room   = ds.y - kMargin * sl - bot_h - top_end;
    const float mid_y  = top_end + std::max(kGap * sl, (room - mid_h) * 0.42f);
    column("lm", l.left_mid, lx, mid_y, false, sl, l);
    column("lb", l.left_bottom, lx, ds.y - kMargin * sl - bot_h + kGap * sl, false, sl, l);

    // Right: the top card leaves room for the view cube beside it; the rest stack below.
    float ry = kMargin * sr;
    column("rt", l.right_top, rx, ry, true, sr, l);
    for (const CadUiGroup& g : l.right_top) if (!g.empty()) ry += group_height(g, sr) + kGap * sr;
    column("rm", l.right_mid, rx, ry, true, sr, l);

    if (m_pend_card != nullptr)
        card(*m_pend_card, m_pend_x, m_pend_y, m_pend_right, m_pend_s);
    else if (m_pend_item != nullptr)
        flyout(*m_pend_item, m_pend_x, m_pend_y, m_pend_s, m_pend_right);
    m_pend_item = nullptr;
    m_pend_card = nullptr;
    pill(l, s);

    if (!l.bottom_note.IsEmpty()) {
        const std::string n = u8(l.bottom_note);
        const ImVec2 d = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0.f, n.c_str());
        ImGui::GetForegroundDrawList()->AddText(ImVec2((ds.x - d.x) * 0.5f, ds.y - kMargin * s - d.y - 26.f * s),
                                                palette(dark).sub, n.c_str());
    }
    search(s);
    ImGui::PopStyleVar(3);
}

}} // namespace Slic3r::GUI
