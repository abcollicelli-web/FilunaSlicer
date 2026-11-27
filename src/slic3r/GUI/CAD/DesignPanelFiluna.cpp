// Filuna CAD: the Design tab's tools as floating columns over the 3D view (see FilunaCadUi.hpp).
// Every button runs an action the tab already had — the offer verbs, the key maps, the panel's
// own handlers — so nothing the ribbon could do is lost; the ribbon is only hidden.

#include "slic3r/GUI/CAD/DesignPanel.hpp"
#include "slic3r/GUI/FilunaIdioma.hpp"
#include "slic3r/GUI/CAD/DesignCanvas.hpp"
#include "slic3r/GUI/CAD/DesignOffer.hpp"
#include "slic3r/GUI/CAD/FilunaCadIa.hpp"
#include "slic3r/GUI/CAD/FilunaCadUi.hpp"
#include "slic3r/GUI/CAD/McpControl.hpp"

#include "libslic3r/CAD/CadDocument.hpp"
#include "libslic3r/CAD/GeometryEngine.hpp"
#include "libslic3r/Format/STL.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/FilunaAssistente.hpp"
#include "slic3r/GUI/FilunaPanel.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/Widgets/WebView.hpp"
#include "slic3r/Utils/Http.hpp"

#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>

#include <nlohmann/json.hpp>
#include <wx/aui/framemanager.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>
#include <wx/uri.h>
#include <wx/webview.h>
#include <wx/statline.h>
#include <wx/timer.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <thread>
#include <utility>
#include <vector>

namespace Slic3r { namespace GUI {

namespace {

wxString pt(const char* s) { return Filuna::trw(s); } // português no código; inglês pelo dicionário

// The atlas names its icons design_*; the columns draw the white fcad_* twins of the same art.
std::string fcad_icon(const char* design_icon)
{
    if (design_icon == nullptr) return "fcad_mais";
    std::string n(design_icon);
    if (n.rfind("design_", 0) == 0) return "fcad_" + n.substr(7);
    return "fcad_mais";
}

const OfferVerb* verb(const char* id)
{
    for (int i = 0; i < kOfferVerbCount; ++i)
        if (std::strcmp(kOfferVerbs[i].id, id) == 0) return &kOfferVerbs[i];
    return nullptr;
}

wxString verb_name(const OfferVerb& v) { return I18N::translate(v.name, "Design"); }

} // namespace

void DesignPanel::filuna_setup()
{
    if (!m_filuna_look || m_viewport == nullptr) return;
    // The ribbon and the sketch banner stay built (their key maps and handlers are the actions
    // the columns run) but are not shown: the columns replace them.
    if (m_toolbar != nullptr) m_toolbar->Hide();
    if (m_toolbar_line != nullptr) m_toolbar_line->Hide();
    if (m_sketch_banner != nullptr) m_sketch_banner->Hide();
    Layout();
    // The side panel (history, parts, a tool's settings) opens on the right, as History does in
    // the reference, and only when asked for or when a tool needs its settings.
    m_viewport->set_sidebar_collapse([] { return CollapseSide::None; }, [] {});
    wxAuiPaneInfo& side = m_aui.GetPane(m_form);
    if (side.IsOk()) side.Dock().Right();
    m_side_open = false;
    collapse_sidebar(true);
    m_viewport->overlay_covers = [this](float x, float y) { return m_cad_ui.covers(x, y); };
    m_viewport->set_filuna_look([this] { filuna_overlay(); },
                                FilunaCadUi::right_column_width(m_viewport->ui_scale()));
}

void DesignPanel::filuna_sync_sidebar()
{
    if (!m_filuna_look) return;
    const bool card = (m_active != Tool::None && m_active != Tool::Sketch) || m_ui_mode == UiMode::Constrain
                      || body_move_pending();
    const bool want = m_side_open || card;
    if (want == m_sidebar_collapsed) collapse_sidebar(!want);
}

bool DesignPanel::filuna_verb_applies(const OfferVerb& v, uint32_t bit) const
{
    int sketches = 0;
    for (const auto& f : m_doc.features)
        if (f.type == CadFeatureType::Sketch) ++sketches;
    bool sheet = false;
    for (const auto& b : m_doc.bodies)
        if (CadDocument::is_sheet_shape(b.shape)) { sheet = true; break; }
    return v.action != nullptr && (v.accepts & bit) && v.need_bodies <= int(m_doc.bodies.size()) &&
           v.need_sketches <= sketches && (!v.need_sheet || sheet);
}

wxString DesignPanel::filuna_tool_title() const
{
    switch (m_active) {
    case Tool::Extrude:        return pt("Extrusão");
    case Tool::Dressup:        return pt("Filete e chanfro");
    case Tool::Hole:           return pt("Furo");
    case Tool::Thread:         return pt("Rosca");
    case Tool::Shell:          return pt("Casca");
    case Tool::Revolve:        return pt("Revolução");
    case Tool::Sweep:          return pt("Varredura");
    case Tool::Pattern:        return pt("Padrão");
    case Tool::Plane:          return pt("Plano");
    case Tool::Loft:           return pt("Transição");
    case Tool::Draft:          return pt("Inclinação");
    case Tool::Boolean:        return pt("União, subtração e interseção");
    case Tool::Cut:            return pt("Cortar");
    case Tool::Insert:         return pt("Inserir");
    case Tool::Axis:           return pt("Eixo");
    case Tool::CoordSys:       return pt("Sistema de coordenadas");
    case Tool::SurfaceExtrude: return pt("Superfície por extrusão");
    case Tool::SurfaceRevolve: return pt("Superfície por revolução");
    case Tool::SurfaceLoft:    return pt("Superfície por transição");
    case Tool::SurfaceFill:    return pt("Preencher superfície");
    case Tool::SurfaceOffset:  return pt("Deslocar superfície");
    case Tool::ThickenSurface: return pt("Engrossar superfície");
    case Tool::Transform:      return pt("Mover e girar");
    case Tool::Mirror:         return pt("Espelhamento");
    case Tool::Thicken:        return pt("Engrossar");
    case Tool::Rib:            return pt("Nervura");
    case Tool::Project:        return pt("Projetar");
    case Tool::DeleteFace:     return pt("Excluir face");
    case Tool::Helix:          return pt("Hélice");
    case Tool::Mate:           return pt("Encaixar");
    default:                   return wxString();
    }
}


void DesignPanel::filuna_layout(CadUiLayout& l)
{
    const bool  sketching = m_ui_mode == UiMode::Sketch;
    const int   kind      = offer_selection_kind();
    const uint32_t bit    = offer_bit(OfferSel(kind));
    auto        later     = [this](std::function<void()> f) {
        return [this, f] {
            CallAfter([this, f] {
                f();
                if (m_viewport) m_viewport->request_repaint();
            });
        };
    };
    // A verb from the atlas, as a column item: its own name, key and icon, greyed when the
    // current selection is not something it acts on (`gate`).
    auto from_verb = [&](const char* id, const char* label = nullptr, bool gate = false) {
        CadUiItem it;
        const OfferVerb* v = verb(id);
        if (v == nullptr) return it;
        it.icon  = fcad_icon(v->icon);
        it.label = label ? pt(label) : verb_name(*v);
        it.keys  = v->key ? v->key : "";
        const std::string action = v->action ? v->action : "";
        if (!action.empty()) it.run = later([this, action] { run_offer_action(action.c_str()); });
        else it.enabled = [] { return false; };
        if (gate) {
            const bool ok = filuna_verb_applies(*v, bit);
            it.enabled    = [ok] { return ok; };
        }
        return it;
    };
    auto item = [&](const char* icon, const char* label, std::function<void()> run, const char* keys = "") {
        CadUiItem it;
        it.icon  = icon;
        it.label = pt(label);
        it.keys  = keys;
        if (run) it.run = later(std::move(run));
        return it;
    };
    auto onoff = [](bool v) { return v ? pt("Ligada") : pt("Desligada"); };

    // --- Top left: the project and its groups -------------------------------------------------
    {
        CadUiItem proj = item("fcad_projeto", "Projeto", nullptr);
        proj.sub = [] { return pt("Modelando"); };
        proj.more = {
            item("fcad_novo", "Novo design", [this] { on_new_design(); }),
            item("fcad_arquivo", "Abrir modelo (STL, 3MF, OBJ)", [this] { on_import_mesh(); }),
            item("fcad_step", "Abrir CAD (STEP)", [this] { on_import_step(); }),
            item("fcad_svg", "Abrir desenho (SVG)", [this] { on_import_svg(); }),
            item("fcad_exportar", "Exportar STL", [this] { filuna_export_stl(); }),
            item("fcad_exportar", "Exportar STEP", [this] { on_export_step(); }),
            item("fcad_exportar", "Salvar projeto (3MF)", [] { if (wxGetApp().plater()) wxGetApp().plater()->save_project(); }, "Ctrl+S"),
            item("fcad_preparar", "Mandar para a Preparação", [this] { on_commit(); }),
            item("fcad_desfazer", "Desfazer", [this] { do_undo_redo(false); }, "Ctrl+Z"),
            item("fcad_refazer", "Refazer", [this] { do_undo_redo(true); }, "Ctrl+Y"),
        };
        CadUiItem groups = item("fcad_grupos", "Grupos", nullptr);
        groups.run = [this] { CallAfter([this] { m_side_open = !m_side_open; filuna_sync_sidebar(); }); };
        groups.on  = [this] { return m_side_open; };
        l.left_top = { { proj }, { groups } };
    }

    CadUiItem search = item("fcad_pesquisar", "Pesquisar", nullptr, "Ctrl+F");
    search.run = [this] { CallAfter([this] { filuna_open_search(); }); };

    CadUiItem isolate = item("fcad_isolar", "Isolar", [this] { filuna_isolate(); });
    isolate.sub = [this, onoff] { return onoff(m_isolated); };
    isolate.on  = [this] { return m_isolated; };
    CadUiItem section = item("fcad_secao", "Visualizar seção", [this] { toggle_section_view(); });
    section.sub = [this, onoff] { return onoff(m_section_on); };
    section.on  = [this] { return m_section_on; };

    if (sketching) {
        // --- Sketch: the drawing tools on the left, the constraints on the right -------------
        CadUiItem leave = item("fcad_sair", "Sair do esboço", [this] { tool_confirm(); });
        CadUiItem plane = item("fcad_plano", "", nullptr);
        plane.label   = m_sketch_on.IsEmpty() ? pt("Nenhum plano ativo") : pt("Plano: ") + m_sketch_on;
        plane.enabled = [] { return false; };

        CadUiItem line = from_verb("sk_line_t", "Linha");
        line.more = { from_verb("sk_line_t", "Linha"), from_verb("sk_polyline", "Polilinha") };
        CadUiItem arc = from_verb(m_var_arc == 1 ? "sk_arc_tangent" : m_var_arc == 2 ? "sk_arc_center" : "sk_arc_t", "Arco");
        arc.sub  = [this] { return m_var_arc == 1 ? pt("Tangente") : m_var_arc == 2 ? pt("Centro") : pt("3 pontos"); };
        arc.more = { from_verb("sk_arc_t", "Arco por 3 pontos"), from_verb("sk_arc_tangent", "Arco tangente"),
                     from_verb("sk_arc_center", "Arco pelo centro") };
        for (int i = 0; i < 3; ++i) { auto r = arc.more[i].run; arc.more[i].run = [this, i, r] { m_var_arc = i; if (r) r(); }; }
        CadUiItem spline = from_verb("sk_spline", "Spline");
        spline.sub = [] { return pt("Ajuste"); };
        static const char* rect_ids[] = { "sk_rect", "sk_rect_center", "sk_rect_oblique", "sk_rect_rounded" };
        static const char* rect_sub[] = { "Canto", "Centro", "Inclinado", "Arredondado" };
        CadUiItem rect = from_verb(rect_ids[m_var_rect], "Retângulo");
        rect.sub  = [this] { return pt(rect_sub[m_var_rect]); };
        rect.more = { from_verb("sk_rect", "Retângulo por canto"), from_verb("sk_rect_center", "Retângulo pelo centro"),
                      from_verb("sk_rect_oblique", "Retângulo inclinado"), from_verb("sk_rect_rounded", "Retângulo arredondado") };
        for (int i = 0; i < 4; ++i) { auto r = rect.more[i].run; rect.more[i].run = [this, i, r] { m_var_rect = i; if (r) r(); }; }
        static const char* circle_ids[] = { "sk_circle", "sk_circle_2pt", "sk_circle_3pt" };
        static const char* circle_sub[] = { "Centro", "2 pontos", "3 pontos" };
        CadUiItem circle = from_verb(circle_ids[m_var_circle], "Círculo");
        circle.sub  = [this] { return pt(circle_sub[m_var_circle]); };
        circle.more = { from_verb("sk_circle", "Círculo pelo centro"), from_verb("sk_circle_2pt", "Círculo por 2 pontos"),
                        from_verb("sk_circle_3pt", "Círculo por 3 pontos") };
        for (int i = 0; i < 3; ++i) { auto r = circle.more[i].run; circle.more[i].run = [this, i, r] { m_var_circle = i; if (r) r(); }; }
        CadUiItem more = item("fcad_mais", "Mais", nullptr);
        more.dot  = true;
        more.more = { from_verb("sk_ellipse", "Elipse"),      from_verb("sk_polygon", "Polígono"),
                      from_verb("sk_slot", "Rasgo"),          from_verb("sk_point_t", "Ponto"),
                      from_verb("sk_offset", "Deslocar aresta"), from_verb("sk_extend", "Estender"),
                      from_verb("sk_fillet", "Filete"),       from_verb("sk_chamfer", "Chanfro"),
                      from_verb("sk_text", "Texto"),          from_verb("sk_svg", "Desenho SVG"),
                      from_verb("sk_move", "Mover"),          from_verb("sk_rotate", "Girar"),
                      from_verb("sk_scale", "Dimensionar"),   from_verb("sk_dimension", "Cota"),
                      from_verb("sk_array", "Padrão linear"), from_verb("sk_array_polar", "Padrão circular"),
                      from_verb("sk_mirror", "Espelhamento") };
        CadUiItem trim = from_verb("sk_trim", "Aparar");
        CadUiItem del  = item("fcad_excluir", "Excluir", [this] {
            if (m_viewport && !m_viewport->delete_selected_sketch_entities()) set_status(StatusKind::Info, pt("Selecione o que excluir"));
        });
        del.enabled = [this] { return m_viewport && m_viewport->sketch_selection_count() > 0; };
        del.keys    = "Del";
        CadUiItem constr = item("fcad_construction", "Construção", nullptr, "Q");
        constr.run = later([this] {
            if (m_construction) {
                m_construction->SetValue(!m_construction->GetValue());
                if (m_viewport) m_viewport->set_sketch_construction(m_construction->GetValue());
            }
        });
        constr.on  = [this] { return m_construction && m_construction->GetValue(); };
        constr.sub = [this, onoff] { return onoff(m_construction && m_construction->GetValue()); };
        l.left_mid    = { { search }, { leave, plane }, { line, arc, spline, rect, circle, more }, { trim, del }, { constr } };
        l.left_bottom = { { isolate, section } };

        auto cons = [&](const char* icon, const char* label, const char* keys, SketchConstraintType t) {
            CadUiItem it = item(icon, label, [this, t] { apply_constraint(t); }, keys);
            it.enabled   = [this] { return m_viewport && m_viewport->sketch_selection_count() > 0; };
            return it;
        };
        CadUiItem caption = item("fcad_restricao", "Restrição", nullptr);
        caption.caption   = true;
        CadUiItem cmore   = item("fcad_mais", "Mais", nullptr);
        CadUiItem hv      = item("fcad_hv", "Horizontal/vertical", [this] { filuna_hv(); }, "Shift+V");
        hv.enabled        = [this] { return m_viewport && m_viewport->sketch_selection_count() > 0; };
        cmore.more = { hv,
                       cons("fcad_igual", "Igual", "Shift+E", SketchConstraintType::EqualLength),
                       cons("fcad_simetria", "Simetria", "Shift+S", SketchConstraintType::Symmetric),
                       cons("fcad_c_equal_radius", "Raio igual", "", SketchConstraintType::EqualRadius),
                       cons("fcad_c_collinear", "Colinear", "", SketchConstraintType::Collinear),
                       cons("fcad_c_radius", "Raio", "", SketchConstraintType::Radius),
                       cons("fcad_c_diameter", "Diâmetro", "", SketchConstraintType::Diameter),
                       cons("fcad_c_angle", "Ângulo", "", SketchConstraintType::Angle),
                       cons("fcad_c_dist_x", "Distância horizontal", "", SketchConstraintType::DistanceX),
                       cons("fcad_c_dist_y", "Distância vertical", "", SketchConstraintType::DistanceY) };
        CadUiItem disc = item("fcad_desconectar", "Desconectar", [this] { filuna_disconnect(); });
        disc.enabled   = [this] { return m_viewport && m_viewport->sketch_selection_count() > 0; };
        CadUiItem lock = cons("fcad_bloquear", "Bloquear", "Shift+L", SketchConstraintType::Fix);
        CadUiItem make = item("fcad_criar_construcao", "Criar construção", [this] {
            auto it = m_keys_sketch.find('Q');
            if (it != m_keys_sketch.end() && it->second) it->second();
        });
        make.enabled = [this] { return m_viewport && m_viewport->sketch_selection_count() > 0; };
        l.right_mid = { {}, {}, {}, { caption },
                        { cons("fcad_c_parallel", "Paralelo", "Shift+A", SketchConstraintType::Parallel),
                          cons("fcad_c_perpendicular", "Perpendicular", "Shift+P", SketchConstraintType::Perpendicular),
                          cons("fcad_c_tangent", "Tangente", "Shift+T", SketchConstraintType::Tangent),
                          cons("fcad_c_coincident", "Coincidente", "Shift+N", SketchConstraintType::Coincident),
                          cons("fcad_c_midpoint", "Ponto médio", "Shift+M", SketchConstraintType::Midpoint),
                          cons("fcad_c_concentric", "Concêntrico", "Shift+C", SketchConstraintType::Concentric) },
                        { cmore }, { disc }, { lock }, { make } };
    } else if (kind != int(OfferSel::None) && m_active == Tool::None && !m_choosing_sketch_plane) {
        // --- Something is selected: the left column becomes what applies to it -------------
        CadUiItem clear = item("fcad_desmarcar", "Desmarcar tudo", [this] { escape(); }, "Esc");
        const wxString head = offer_header(kind);
        clear.sub = [head] { return head; };
        std::vector<CadUiItem> live, rest;
        for (int i = 0; i < kOfferVerbCount; ++i) {
            const OfferVerb& v = kOfferVerbs[i];
            if (v.sketch_mode || std::strcmp(v.id, "delete") == 0 || std::strcmp(v.id, "delete_body") == 0) continue;
            if (!filuna_verb_applies(v, bit)) continue;
            (live.size() < 7 ? live : rest).push_back(from_verb(v.id));
        }
        if (!rest.empty()) {
            CadUiItem more = item("fcad_mais", "Mais", nullptr);
            more.more = rest;
            live.push_back(more);
        }
        const OfferVerb* dv = verb("delete");
        const bool del_ok   = dv && filuna_verb_applies(*dv, bit);
        CadUiItem del = from_verb(del_ok ? "delete" : "delete_body", "Excluir", true);
        del.icon      = "fcad_excluir";
        l.left_mid    = { { search }, { clear }, live, { del } };
        l.left_bottom = { { isolate, section, item("fcad_medir", "Medir", [this] { filuna_measure(); }) } };
        l.bottom_note = head;
    } else {
        // --- Nothing selected: the tool families -----------------------------------------
        CadUiItem sketch = from_verb("sketch", "Esboço");
        sketch.icon      = "fcad_esboco";
        CadUiItem ins = item("fcad_inserir", "Inserir", nullptr);
        ins.more = { item("fcad_variavel", "Criar variável", [this] { on_add_variable(); }),
                     item("fcad_imagem", "Desenho (SVG)", [this] { on_import_svg(); }),
                     item("fcad_arquivo", "Arquivo (STL, 3MF, OBJ)", [this] { on_import_mesh(); }),
                     item("fcad_step", "Arquivo CAD (STEP)", [this] { on_import_step(); }),
                     item("fcad_texto", "Texto", [this] { on_add_text(); }),
                     from_verb("hole", "Furos e encaixes"),
                     from_verb("project", "Projetar") };
        CadUiItem build = item("fcad_construir", "Construir", nullptr);
        build.more = { from_verb("plane", "Plano"), from_verb("axis", "Eixo"), from_verb("coordsys_v", "Sistema de coordenadas"),
                       from_verb("helix", "Hélice"), from_verb("surf_extrude", "Superfície por extrusão"),
                       from_verb("surf_revolve", "Superfície por revolução"), from_verb("surf_loft", "Superfície por transição"),
                       from_verb("surf_fill", "Preencher superfície"), from_verb("surf_offset", "Deslocar superfície"),
                       from_verb("thicken_surf", "Engrossar superfície") };
        CadUiItem tr = item("fcad_transformar", "Transformar", nullptr);
        tr.more = { from_verb("transform", "Mover/Girar"), from_verb("align", "Alinhar"), from_verb("mate", "Encaixar"),
                    from_verb("mirror", "Espelhamento"), from_verb("pattern", "Padrão linear"),
                    from_verb("pattern_circular", "Padrão circular"), from_verb("pat_curve", "Padrão em curva") };
        tr.more[1].icon = "fcad_alinhar";
        tr.more[2].icon = "fcad_encaixe";
        CadUiItem tools = item("fcad_ferramentas", "Ferramentas", nullptr);
        tools.dot  = true;
        tools.more = { from_verb("fillet", "Filete"), from_verb("chamfer", "Chanfro"), from_verb("extrude", "Extrusão"),
                       from_verb("shell", "Casca"), from_verb("loft", "Transição"), from_verb("boolean", "União"),
                       from_verb("bool_subtract", "Subtrair"), from_verb("bool_intersect", "Intersectar"),
                       from_verb("split", "Dividir corpo"), from_verb("cut", "Cortar"), from_verb("revolve", "Revolução"),
                       from_verb("sweep", "Varredura"), from_verb("hole", "Furo"), from_verb("thread", "Rosca"),
                       from_verb("rib", "Nervura"), from_verb("draft", "Inclinação"), from_verb("thicken", "Engrossar"),
                       from_verb("delete_face", "Excluir face") };
        tools.more[5].icon = "fcad_uniao";
        tools.more[6].icon = "fcad_subtrair";
        tools.more[7].icon = "fcad_intersectar";
        CadUiItem vis = item("fcad_visualizar", "Visualizar", nullptr);
        vis.more = { item("fcad_cor", "Cor do corpo", [this] { on_set_body_color(); }),
                     item("fcad_massa", "Volume e área", [this] { on_mass_properties(); }),
                     item("fcad_interferencia", "Interferência", [this] { on_check_interference(); }) };
        l.left_mid    = { { search }, { sketch, ins, build, tr, tools, vis } };
        l.left_bottom = { { isolate, section, item("fcad_medir", "Medir", [this] { filuna_measure(); }) } };
    }

    // --- Right column -------------------------------------------------------------------------
    CadUiItem magnet = item("fcad_ima", "", nullptr);
    magnet.run = [] {};
    CadUiItem unit = item("fcad_unidade", "", nullptr);
    unit.run = [] {};
    CadUiItem views = item("fcad_vistas", "", nullptr);
    views.run = [] {};
    l.right_top = { { magnet, unit, views } };

    CadUiItem modes = item("fcad_modos", "Modos de exibição", nullptr);
    modes.run = [] {};
    modes.sub = [this] { return m_translucent ? pt("Raio-X") : pt("Sombreado"); };
    CadUiItem shot = item("fcad_captura", "Captura de tela", [this] { filuna_screenshot(); });
    CadUiItem hist = item("fcad_historia", "História", nullptr, "Ctrl+Alt+H");
    hist.run = [this] { CallAfter([this] { m_side_open = !m_side_open; filuna_sync_sidebar(); }); };
    hist.on  = [this] { return m_side_open; };
    CadUiItem ia = item("fcad_ia", "Modelar com IA", nullptr);
    ia.sub = [] { return pt("Claude ou ChatGPT"); };
    ia.run = [this] { CallAfter([this] { filuna_toggle_ia(!m_ia_open); }); };
    ia.on  = [this] { return m_ia_open; };
    ia.dot = true;
    if (l.right_mid.empty()) l.right_mid = { { modes }, { shot }, { hist, ia } };
    else { l.right_mid[0] = { modes }; l.right_mid[1] = { shot }; l.right_mid[2] = { hist, ia }; }

    auto choice = [&](const char* t, std::function<bool()> on, std::function<void()> f) {
        CadUiRow r; r.title = pt(t); r.active = std::move(on); r.pick = later(std::move(f)); return r;
    };
    auto toggle = [&](const char* t, std::function<bool()> on, std::function<void()> f) {
        CadUiRow r; r.title = pt(t); r.active = std::move(on); r.toggle = later(std::move(f)); return r;
    };
    auto title = [](const char* t) { CadUiRow r; r.title = pt(t); return r; };
    l.cards["fcad_modos"] = {
        title("Sombreamento"),
        choice("Sombreado", [this] { return !m_translucent; }, [this] { m_translucent = false; if (m_viewport) m_viewport->set_body_translucent(false); }),
        choice("Raio-X", [this] { return m_translucent; }, [this] { m_translucent = true; if (m_viewport) m_viewport->set_body_translucent(true); }),
        title("Opções"),
        toggle("Mostrar mesa", [this] { return m_show_bed; }, [this] { toggle_bed(); }),
        toggle("Mostrar planos de origem", [this] { return m_show_origin; }, [this] { toggle_origin(); }),
    };
    l.cards["fcad_ima"] = {
        title("Encaixar em"),
        toggle("Pontos, linhas e ângulos", [this] { return m_viewport && m_viewport->snap_enabled(); },
               [this] { if (m_viewport) m_viewport->set_snap_enabled(!m_viewport->snap_enabled()); }),
        title("Segure Shift para desenhar livre"),
    };
    l.cards["fcad_unidade"] = {
        title("Unidades"),
        choice("Milímetro", [] { return true; }, [] {}),
        title("Peças para impressão 3D: sempre em mm"),
    };
    l.cards["fcad_vistas"] = {
        title("Vistas"),
        choice("Visualização padrão", [] { return false; }, [this] { if (m_viewport) m_viewport->set_view("iso"); }),
        choice("Superior", [] { return false; }, [this] { if (m_viewport) m_viewport->set_view("top"); }),
        choice("Inferior", [] { return false; }, [this] { if (m_viewport) m_viewport->set_view("bottom"); }),
        choice("Frontal", [] { return false; }, [this] { if (m_viewport) m_viewport->set_view("front"); }),
        choice("Posterior", [] { return false; }, [this] { if (m_viewport) m_viewport->set_view("rear"); }),
        choice("Direita", [] { return false; }, [this] { if (m_viewport) m_viewport->set_view("right"); }),
        choice("Esquerda", [] { return false; }, [this] { if (m_viewport) m_viewport->set_view("left"); }),
        choice("Enquadrar tudo", [] { return false; }, [this] { if (m_viewport) m_viewport->fit_view(); }),
    };
    if (sketching)
        l.cards["fcad_vistas"].push_back(choice("De frente para o esboço", [] { return false; },
                                                [this] { if (m_viewport) m_viewport->view_normal_to_sketch(); }));

    // --- Top centre: the tool in progress -----------------------------------------------------
    if (m_choosing_sketch_plane) {
        l.pill_title = pt("Esboço");
        l.pill_hint  = pt("Clique numa face plana ou num plano");
        l.cancel     = later([this] { tool_cancel(); });
    } else if (sketching) {
        l.pill_title  = wxString::Format(pt("Esboço %d"), m_feature_counter + 1);
        l.pill_hint   = pt("N = de frente para o plano");
        l.cancel      = later([this] { tool_cancel(); });
        l.confirm     = later([this] { tool_confirm(); });
        l.can_confirm = confirm_enabled();
    } else if (m_ui_mode == UiMode::Constrain) {
        l.pill_title  = pt("Restrições do esboço");
        l.cancel      = later([this] { tool_cancel(); });
        l.confirm     = later([this] { tool_confirm(); });
        l.can_confirm = confirm_enabled();
    } else if (m_active != Tool::None || body_move_pending()) {
        l.pill_title  = body_move_pending() ? pt("Mover") : filuna_tool_title();
        l.cancel      = later([this] { tool_cancel(); });
        l.confirm     = later([this] { tool_confirm(); });
        l.can_confirm = confirm_enabled();
    }
}

void DesignPanel::filuna_overlay()
{
    if (m_viewport == nullptr) return;
    const float s = m_viewport->ui_scale();
    m_viewport->set_filuna_inset(FilunaCadUi::right_column_width(s));
    CadUiLayout l;
    filuna_layout(l);
    m_cad_ui.render(l, s, wxGetApp().dark_mode());
}

void DesignPanel::filuna_open_search()
{
    std::vector<CadUiSearchEntry> all;
    const bool sketching = m_ui_mode == UiMode::Sketch;
    const uint32_t bit   = offer_bit(OfferSel(offer_selection_kind()));
    for (int i = 0; i < kOfferVerbCount; ++i) {
        const OfferVerb& v = kOfferVerbs[i];
        if (v.sketch_mode != sketching || v.action == nullptr) continue;
        CadUiSearchEntry e;
        e.label   = verb_name(v);
        e.where   = I18N::translate(kOfferRowNames[v.row], "Design");
        e.icon    = fcad_icon(v.icon);
        e.keys    = v.key ? v.key : "";
        e.enabled = filuna_verb_applies(v, bit) || (v.accepts & offer_bit(OfferSel::None)) || sketching;
        const std::string action(v.action);
        e.run = [this, action] { CallAfter([this, action] { run_offer_action(action.c_str()); }); };
        all.push_back(std::move(e));
    }
    auto extra = [&](const char* icon, const char* label, const char* where, std::function<void()> f) {
        CadUiSearchEntry e;
        e.icon  = icon;
        e.label = pt(label);
        e.where = pt(where);
        e.run   = [this, f] { CallAfter(f); };
        all.push_back(std::move(e));
    };
    extra("fcad_arquivo", "Abrir modelo (STL, 3MF, OBJ)", "Projeto", [this] { on_import_mesh(); });
    extra("fcad_step", "Abrir CAD (STEP)", "Projeto", [this] { on_import_step(); });
    extra("fcad_svg", "Abrir desenho (SVG)", "Projeto", [this] { on_import_svg(); });
    extra("fcad_exportar", "Exportar STL", "Projeto", [this] { filuna_export_stl(); });
    extra("fcad_exportar", "Exportar STEP", "Projeto", [this] { on_export_step(); });
    extra("fcad_preparar", "Mandar para a Preparação", "Projeto", [this] { on_commit(); });
    extra("fcad_medir", "Medir", "Ferramentas", [this] { filuna_measure(); });
    extra("fcad_isolar", "Isolar", "Visualizar", [this] { filuna_isolate(); });
    extra("fcad_secao", "Visualizar seção", "Visualizar", [this] { toggle_section_view(); });
    extra("fcad_captura", "Captura de tela", "Visualizar", [this] { filuna_screenshot(); });
    extra("fcad_variavel", "Criar variável", "Inserir", [this] { on_add_variable(); });
    m_cad_ui.open_search(std::move(all));
    if (m_viewport) {
        m_viewport->SetFocus();   // ImGui is fed by the canvas's key handler
        m_viewport->request_repaint();
    }
}

// The panel's key hook, before any shortcut: the search field and an open flyout come first.
bool DesignPanel::filuna_key(int key, bool ctrl, bool shift)
{
    if (!m_filuna_look) return false;
    (void)shift;
    if (m_cad_ui.search_open()) {
        if (key == WXK_ESCAPE) { m_cad_ui.close_search(); }
        else if (key == WXK_RETURN || key == WXK_NUMPAD_ENTER) { m_cad_ui.search_run(); }
        else if (key == WXK_DOWN) { m_cad_ui.search_move(1); }
        else if (key == WXK_UP) { m_cad_ui.search_move(-1); }
        else return false;   // a character for the field: let it reach the canvas
        if (m_viewport) m_viewport->request_repaint();
        return true;
    }
    if (key == WXK_ESCAPE && m_cad_ui.flyout_open()) {
        m_cad_ui.close_flyout();
        if (m_viewport) m_viewport->request_repaint();
        return true;
    }
    if (ctrl && (key == 'F' || key == 'f' || key == 6)) {   // 6 = Ctrl+F as a control character
        filuna_open_search();
        return true;
    }
    if (ctrl && !shift && (key == 'H' || key == 'h' || key == 8) && wxGetKeyState(WXK_ALT)) {
        m_side_open = !m_side_open;
        filuna_sync_sidebar();
        return true;
    }
    return false;
}

void DesignPanel::filuna_isolate()
{
    sync_body_visible();
    if (m_isolated) {
        if (m_isolate_saved.size() == m_body_visible.size()) m_body_visible = m_isolate_saved;
        else std::fill(m_body_visible.begin(), m_body_visible.end(), true);
        m_isolated = false;
    } else {
        const int b = m_sel_solid_body >= 0 ? m_sel_solid_body : tree_body_selection();
        if (b < 0 || b >= int(m_body_visible.size())) {
            set_status(StatusKind::Info, pt("Selecione um corpo para isolar"));
            return;
        }
        m_isolate_saved = m_body_visible;
        for (size_t i = 0; i < m_body_visible.size(); ++i) m_body_visible[i] = int(i) == b;
        m_isolated = true;
    }
    if (m_viewport != nullptr) {
        feed_bodies();
        m_viewport->set_solid_pick(&m_doc.bodies, &m_disp_pick_mesh, &m_doc.display_tri_face, &m_doc.display_tri_body,
                                   &m_body_visible, &m_body_xform);
    }
    refresh_tree();
    set_status(StatusKind::Info, m_isolated ? pt("Só o corpo selecionado está à vista") : pt("Todos os corpos à vista"));
}

void DesignPanel::filuna_measure()
{
    const int nb = int(m_doc.bodies.size());
    if (m_sel_solid_body < 0 || m_sel_solid_body >= nb) {
        set_status(StatusKind::Info, pt("Selecione uma aresta, uma face ou um corpo para medir"));
        return;
    }
    const TopoDS_Shape& shape = m_doc.bodies[m_sel_solid_body].shape;
    if (m_sel_solid_edge >= 0) {
        const TopoDS_Edge e = GeometryEngine::edge_by_index(shape, m_sel_solid_edge);
        GProp_GProps      g;
        BRepGProp::LinearProperties(e, g);
        const auto c = GeometryEngine::circle_of_edge(e);
        if (c.ok)
            set_status(StatusKind::Info, wxString::Format(pt("Aresta circular: raio %.2f mm, diâmetro %.2f mm, comprimento %.2f mm"),
                                                          c.radius, 2.0 * c.radius, g.Mass()));
        else
            set_status(StatusKind::Info, wxString::Format(pt("Aresta: comprimento %.2f mm"), g.Mass()));
        return;
    }
    if (m_sel_solid_face >= 0) {
        const TopoDS_Face f = GeometryEngine::face_by_index(shape, m_sel_solid_face);
        GProp_GProps      g;
        BRepGProp::SurfaceProperties(f, g);
        const auto c = GeometryEngine::cylinder_of_face(f);
        if (c.ok)
            set_status(StatusKind::Info, wxString::Format(pt("Face cilíndrica: área %.2f mm², diâmetro %.2f mm"), g.Mass(), 2.0 * c.radius));
        else
            set_status(StatusKind::Info, wxString::Format(pt("Face: área %.2f mm²"), g.Mass()));
        return;
    }
    const auto mp = GeometryEngine::mass_properties(shape);
    Bnd_Box    bb;
    BRepBndLib::Add(shape, bb);
    double x0 = 0, y0 = 0, z0 = 0, x1 = 0, y1 = 0, z1 = 0;
    if (!bb.IsVoid()) bb.Get(x0, y0, z0, x1, y1, z1);
    set_status(StatusKind::Info, wxString::Format(pt("Corpo: %.2f × %.2f × %.2f mm, volume %.2f mm³, área %.2f mm²"),
                                                  x1 - x0, y1 - y0, z1 - z0, mp.volume, mp.surface_area));
}

void DesignPanel::filuna_export_stl()
{
    if (m_active != Tool::None) confirm_tool();
    if (m_doc.bodies.empty()) {
        set_status(StatusKind::Info, pt("Nada para exportar: crie ou abra um corpo primeiro"));
        return;
    }
    wxFileDialog dlg(this, pt("Exportar STL"), wxEmptyString, "modelo.stl", pt("STL (*.stl)|*.stl"),
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;
    rebuild_disp_meshes();   // the bodies where they are shown, Move offsets included
    TriangleMesh all;
    for (size_t b = 0; b < m_disp_body_meshes.size(); ++b)
        if (b >= m_body_visible.size() || m_body_visible[b]) all.merge(m_disp_body_meshes[b]);
    if (all.its.indices.empty()) {
        set_status(StatusKind::Info, pt("Nenhum corpo visível para exportar"));
        return;
    }
    const bool ok = store_stl(dlg.GetPath().ToUTF8().data(), &all, true);
    set_status(ok ? StatusKind::Ok : StatusKind::Error, ok ? pt("STL exportado") : pt("Não foi possível gravar o STL"));
}

void DesignPanel::filuna_screenshot()
{
    if (m_viewport == nullptr) return;
    const std::time_t now = std::time(nullptr);
    char              name[64];
    std::strftime(name, sizeof(name), "Filuna CAD %Y-%m-%d %H%M%S.png", std::localtime(&now));
    wxFileDialog dlg(this, pt("Salvar captura de tela"), wxEmptyString, wxString::FromUTF8(name), pt("PNG (*.png)|*.png"),
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;
    m_viewport->save_screenshot(dlg.GetPath().ToUTF8().data(), [this](bool ok) {
        set_status(ok ? StatusKind::Ok : StatusKind::Error, ok ? pt("Captura de tela salva") : pt("Não foi possível salvar a captura"));
    });
}

// One button for both, as in the reference: a line lying closer to horizontal becomes horizontal.
void DesignPanel::filuna_hv()
{
    if (m_viewport == nullptr) return;
    const std::vector<int>&           sel = m_viewport->sketch_selection();
    const std::vector<SketchEntity>&  ent = m_viewport->sketch_entities();
    SketchConstraintType              t   = SketchConstraintType::Horizontal;
    if (!sel.empty() && sel[0] >= 0 && sel[0] < int(ent.size()) && ent[sel[0]].type == SketchEntity::Type::Line) {
        const Vec2d d = ent[sel[0]].p1 - ent[sel[0]].p0;
        if (std::abs(d.y()) > std::abs(d.x())) t = SketchConstraintType::Vertical;
    }
    apply_constraint(t);
}

// Undo the joins of the selected elements: their coincidences go, the geometry stays put.
void DesignPanel::filuna_disconnect()
{
    if (m_viewport == nullptr) return;
    const std::vector<int> sel = m_viewport->sketch_selection();
    const auto&            cs  = m_viewport->sketch_constraints();
    std::vector<int>       drop;
    for (int i = 0; i < int(cs.size()); ++i)
        if (cs[i].type == SketchConstraintType::Coincident &&
            (std::find(sel.begin(), sel.end(), cs[i].ea) != sel.end() || std::find(sel.begin(), sel.end(), cs[i].eb) != sel.end()))
            drop.push_back(i);
    for (auto it = drop.rbegin(); it != drop.rend(); ++it) m_viewport->remove_sketch_constraint(*it);
    set_status(StatusKind::Info, drop.empty() ? pt("Nada ligado a desconectar") : pt("Desconectado"));
    m_viewport->request_repaint();
}

// --- Modelar com IA ------------------------------------------------------------------------------
// The customer's own Claude or ChatGPT (their subscription, no API key, no credit), opened inside
// the Design tab, models through the Filuna connector: it reads filuna_cad and sends steps with
// filuna_cad_fazer; the page "cad-ia.html" fetches them and runs them here, one by one, through the
// same control methods the Design tab is built on (McpControl.cpp), each an Undo step.
namespace {

using json = nlohmann::json;

const char* const kBlockedForAi[] = { "import_step", "import_mesh", "validate_against" };

bool blocked_for_ai(const std::string& m)
{
    for (const char* b : kBlockedForAi)
        if (m == b) return true;
    return false;
}

class FilunaCadIaPainel : public wxPanel
{
public:
    explicit FilunaCadIaPainel(wxWindow* parent) : wxPanel(parent, wxID_ANY)
    {
        SetBackgroundColour(*wxWHITE);
        m_tela  = new FilunaPanel(this, "cad-ia.html");
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(m_tela, 1, wxEXPAND);
        SetSizer(s);
        g_atual = this;
    }
    ~FilunaCadIaPainel() override { if (g_atual == this) g_atual = nullptr; }
    static FilunaCadIaPainel* atual() { return g_atual; }

    void abrir_ia(const wxString& url)
    {
        if (m_ia == nullptr) {
            m_ia = WebView::CreateWebView(this, url);
            if (m_ia == nullptr)
                throw std::runtime_error("O navegador embutido não abriu neste computador.");
            filuna_ia_preparar_navegador(m_ia);
            GetSizer()->Add(m_ia, 1, wxEXPAND);
        } else if (!url.empty()) {
            m_ia->LoadURL(url);
        }
        GetSizer()->GetItem(m_tela)->SetProportion(0);
        m_tela->SetMinSize(wxSize(-1, FromDIP(132)));
        m_ia->Show();
        m_aberta = true;
        Layout();
    }
    void fechar_ia()
    {
        if (m_ia != nullptr) m_ia->Hide();
        GetSizer()->GetItem(m_tela)->SetProportion(1);
        m_tela->SetMinSize(wxSize(-1, -1));
        m_aberta = false;
        Layout();
    }
    void comando(const std::string& c)
    {
        if (m_ia == nullptr) return;
        if (c == "voltar" && m_ia->CanGoBack()) m_ia->GoBack();
        else if (c == "recarregar") m_ia->Reload();
        else if (c == "navegador") wxLaunchDefaultBrowser(m_ia->GetCurrentURL());
    }
    bool aberta() const { return m_aberta; }
    void evento(const std::string& nome, const json& dados) { if (m_tela != nullptr) m_tela->send_event(nome, dados); }

private:
    static inline FilunaCadIaPainel* g_atual = nullptr;
    FilunaPanel* m_tela{nullptr};
    wxWebView*   m_ia{nullptr};
    bool         m_aberta{false};
};

// Every refusal on the AI's path has the same shape: a stable code to act on, the message (the old site reads
// "erro") and a hint saying what to send instead.
json erro_ia(const std::string& codigo, const std::string& erro, const std::string& dica)
{
    return json{{"ok", false}, {"codigo", codigo}, {"erro", erro}, {"dica", dica}};
}

json rpc(const std::string& method, const json& params)
{
    json r;
    try { r = json::parse(filuna_cad_rpc(method, params.dump())); }
    catch (const std::exception& ex) { return erro_ia("erro_interno", ex.what(), "tente de novo; se repetir, avise o cliente"); }
    if (!r.contains("error")) return json{{"ok", true}, {"resultado", r.contains("result") ? r["result"] : json()}};
    const int         code = r["error"].value("code", -32000);
    const std::string msg  = r["error"].value("message", std::string("erro"));
    if (code == -32010)
        return erro_ia("generation_vencida", msg, "os ids mudaram: releia query_topology e mande os ids novos com a generation nova");
    if (code == -32002)
        return erro_ia("design_ocupado", msg, "há um esboço ou ferramenta aberta: mande \"concluir\" ou \"cancelar\" (ou sketch_commit) antes");
    if (code == -32601) return erro_ia("metodo_desconhecido", msg, "veja a lista de comandos em filuna_cad");
    if (code == -32602) return erro_ia("valor_invalido", msg, "confira o tipo do parâmetro na lista de comandos de filuna_cad");
    if (msg.find("type_error") != std::string::npos)
        return erro_ia("tipo_errado", msg, "um parâmetro veio com o tipo errado (texto no lugar de número, lista no lugar de objeto...)");
    return erro_ia("erro_na_operacao", msg, "confira os parâmetros e o estado com describe_scene; ids de face e aresta vêm de query_topology");
}

// Extrude a sketch that is already in the document (by default the last one): what makes a circle,
// a slot or any freehand sketch into a solid. The control surface's own extrude only takes a
// rectangle or a polygon given inline.
json extrude_sketch(DesignPanel* p, const json& params)
{
    std::string why;
    if (p->mcp_busy(false, why))
        return erro_ia("design_ocupado", "Design ocupado: " + why, "mande \"concluir\" ou \"cancelar\" antes");
    CadDocument& doc = p->mcp_doc();
    int s = params.value("sketch", -1);
    if (s < 0)
        for (int i = int(doc.features.size()) - 1; i >= 0; --i)
            if (doc.features[i].type == CadFeatureType::Sketch) { s = i; break; }
    if (s < 0 || s >= int(doc.features.size()) || doc.features[s].type != CadFeatureType::Sketch)
        return erro_ia("sem_esboco", "não há esboço para extrudar (sketch = índice de um recurso Sketch)",
                       "desenhe com sketch_begin, sketch_add e sketch_commit, ou passe o índice certo em 'sketch'");
    const double d = params.value("distance", 10.0);
    if (d <= 0) return erro_ia("fora_da_faixa", "distance precisa ser maior que zero", "distance em mm, maior que 0");
    const std::string b = params.value("boolean", std::string("new"));
    const BooleanMode mode = b == "union" || b == "add" ? BooleanMode::Add
                           : b == "subtract" || b == "cut" ? BooleanMode::Cut
                           : b == "intersect" ? BooleanMode::Intersect : BooleanMode::New;
    doc.checkpoint();
    const int e = doc.add_extrude(s, d, false, mode, "Extrude");
    CadFeature& fe = doc.features[e];
    fe.flip      = params.value("flip", false);
    fe.taper_deg = params.value("taper", 0.0);
    const std::string end = params.value("end", std::string("blind"));
    if      (end == "symmetric")   fe.extrude_end = ExtrudeEnd::Symmetric;
    else if (end == "through_all") fe.extrude_end = ExtrudeEnd::ThroughAll;
    else                           fe.extrude_end = ExtrudeEnd::Blind;
    const bool ok = doc.recompute();
    if (!ok) { const std::string w = doc.error; doc.undo(); doc.error = w; }
    p->mcp_after_change();
    return json{{"ok", ok}, {"resultado", json{{"ok", ok}, {"sketch_index", s}, {"extrude_index", e},
                                              {"bodies", int(doc.bodies.size())}, {"error", doc.error}}}};
}

// The control surface describes its modelling tools; the sketch session and the Filuna extras are
// described here, so the AI gets every command it may send in one list.
json extra_tools()
{
    auto par = [](const char* n, const char* t, const char* d) { return json{{"name", n}, {"type", t}, {"description", d}}; };
    auto tool = [](const char* n, const char* s, json ps) { return json{{"name", n}, {"summary", s}, {"params", ps}}; };
    json tamanho = par("tamanho", "integer", "lado da imagem em pixels, 256 a 1024 (padrão 640)");
    tamanho["min"] = 256;
    tamanho["max"] = 1024;
    return json::array({
        tool("ver", "Mostra a peça como imagem (PNG), desenhada sem depender da tela: use para conferir a forma depois de "
                    "modelar. Vistas: iso, frente, topo, direita, esquerda, tras, baixo. A imagem traz a vista, a escala, uma "
                    "régua, a caixa em mm e o número de cada corpo. Imagem não é medida: para medir use describe_scene e measure.",
             json::array({par("vistas", "array|string", "lista de vistas (padrão [\"iso\"])"), tamanho,
                          par("corpo", "integer", "índice de um corpo só (padrão: todos os visíveis)")})),
        tool("laudo", "Laudo de imprimibilidade (só leitura): status ok | aviso | bloqueado e os códigos corpo_aberto, "
                      "volume_zero, nao_cabe_na_mesa (bloqueiam), pedacos_soltos, fora_da_mesa, pode_tombar (avisos), com "
                      "volume, caixa, área de apoio e se o centro de massa cai no apoio de cada corpo. Vem também em todo "
                      "passo que muda a peça.",
             json::array()),
        tool("sketch_begin", "Abre um esboço num plano (XY = mesa, XZ = frente, YZ = lado). Depois: sketch_add, sketch_commit.",
             json::array({par("plane", "string", "XY | XZ | YZ (padrão XY)")})),
        tool("sketch_add", "Desenha no esboço aberto: 'entities' = lista de {type: line|circle|arc|point, ...}; ou um só "
                           "(type + campos); ou 'rect' [x0,y0,x1,y1] (retângulo fechado). line: p0 [x,y], p1 [x,y]; circle: "
                           "center [x,y], radius; arc: center, radius e start_deg/end_deg em graus (ou start_angle/end_angle "
                           "em radianos); point: p [x,y]. construction: true para linha de construção (não vira sólido).",
             json::array({par("entities", "array", "lista de entidades"), par("rect", "array", "[x0, y0, x1, y1] em mm"),
                          par("type", "string", "uma entidade só"), par("construction", "boolean", "geometria de construção"),
                          par("p0", "array", "[x,y] (line)"), par("p1", "array", "[x,y] (line)"), par("p", "array", "[x,y] (point)"),
                          par("center", "array", "[x,y] (circle, arc)"), par("centre", "array", "o mesmo que center"),
                          par("radius", "number", "mm (circle, arc)"), par("start_deg", "number", "graus (arc)"),
                          par("end_deg", "number", "graus (arc)"), par("start_angle", "number", "radianos (arc)"),
                          par("end_angle", "number", "radianos (arc)")})),
        tool("sketch_describe", "Lista o que está desenhado no esboço aberto (índices, medidas, laços fechados).", json::array()),
        tool("sketch_select", "Seleciona entidades do esboço pelos índices (para restrições e operações).",
             json::array({par("entities", "array", "índices")})),
        tool("sketch_delete", "Apaga entidades do esboço (índices ou a seleção).", json::array({par("entities", "array", "índices")})),
        tool("sketch_construction", "Liga/desliga construção nas entidades.", json::array({par("entities", "array", "índices")})),
        tool("sketch_offset", "Desloca (offset) entidades por uma distância em mm.",
             json::array({par("distance", "number", "mm"), par("entities", "array", "índices")})),
        tool("sketch_mirror", "Espelha entidades por um eixo de axis_a a axis_b.",
             json::array({par("axis_a", "array", "[x,y]"), par("axis_b", "array", "[x,y]"), par("entities", "array", "índices")})),
        tool("sketch_set_value", "Dá a medida da seleção (comprimento da linha, raio/diâmetro, ângulo ou distância entre duas).",
             json::array({par("value", "number", "mm ou graus")})),
        tool("sketch_commit", "Fecha o esboço e guarda no design. Depois use extrude_sketch, revolve...", json::array()),
        tool("sketch_cancel", "Descarta o esboço aberto.", json::array()),
        tool("extrude_sketch", "Extruda um esboço já guardado (padrão: o último) — é assim que círculos, rasgos e desenhos "
                               "livres viram sólido.",
             json::array({par("sketch", "integer", "índice do recurso Sketch (padrão: o último)"), par("distance", "number", "mm"),
                          par("boolean", "string", "new | union | subtract | intersect"),
                          par("end", "string", "blind | symmetric | through_all"), par("flip", "boolean", "inverter o sentido"),
                          par("taper", "number", "inclinação da parede em graus")})),
        tool("set_variable", "Cria ou muda uma variável do design (nome = expressão, ex.: largura = 60).",
             json::array({par("name", "string", "nome"), par("expr", "string", "expressão em mm")})),
        tool("set_feature_expr", "Liga um campo de um recurso a uma expressão (ex.: distance = altura/2).",
             json::array({par("feature", "integer", "índice do recurso"), par("field", "string", "campo"), par("expr", "string", "expressão")})),
        tool("list_verbs", "Lista as ações da ferramenta que valem para a seleção atual.",
             json::array({par("applicable_only", "boolean", "só as que valem agora"), par("sketch_mode", "boolean", "as do esboço")})),
        tool("run_verb", "Roda uma ação da ferramenta pelo id (de list_verbs).", json::array({par("verb", "string", "id")})),
        tool("desfazer", "Desfaz o último passo.", json::array()),
        tool("refazer", "Refaz o passo desfeito.", json::array()),
        tool("concluir", "Conclui o esboço ou a ferramenta que o cliente deixou aberta.", json::array()),
        tool("cancelar", "Cancela o esboço ou a ferramenta aberta.", json::array()),
        tool("enquadrar", "Ajusta a vista para mostrar tudo.", json::array()),
    });
}

// --- validation of the AI's steps -------------------------------------------------------------------------------

// method -> its parameter list, from the same descriptions the AI reads (describe_tools + extra_tools).
const json& esquemas()
{
    static json m;   // null until describe_tools answers; a null schema validates nothing
    if (!m.is_object()) {
        json tools = rpc("describe_tools", json::object()).value("resultado", json::object());
        if (tools.contains("tools") && tools["tools"].is_array()) {
            json out = json::object();
            for (const json& t : tools["tools"]) out[t.value("name", std::string())] = t.value("params", json::array());
            for (const json& t : extra_tools()) out[t.value("name", std::string())] = t.value("params", json::array());
            out["view"] = out["ver"];
            m = std::move(out);
        }
    }
    return m;
}

bool numero_invalido(const json& j)
{
    if (j.is_number_float()) return !std::isfinite(j.get<double>());
    if (j.is_structured())
        for (const json& x : j)
            if (numero_invalido(x)) return true;
    return false;
}

bool tipo_confere(const json& v, const std::string& tipos)
{
    size_t i = 0;
    while (i <= tipos.size()) {
        const size_t f = std::min(tipos.find('|', i), tipos.size());
        const std::string t = tipos.substr(i, f - i);
        i = f + 1;
        if ((t == "number" && v.is_number()) || (t == "boolean" && v.is_boolean()) || (t == "string" && v.is_string()) ||
            (t == "array" && v.is_array()) || (t == "object" && v.is_object()) ||
            (t == "integer" && (v.is_number_integer() || (v.is_number_float() && std::floor(v.get<double>()) == v.get<double>()))))
            return true;
        if (t != "number" && t != "boolean" && t != "string" && t != "array" && t != "object" && t != "integer")
            return true;   // a type this check does not know: let the handler judge it
    }
    return false;
}

std::string nomes_dos_parametros(const json& lista)
{
    std::string s;
    for (const json& p : lista) s += (s.empty() ? "" : ", ") + p.value("name", std::string());
    return s.empty() ? std::string("nenhum") : s;
}

// Does this call name a face or an edge by id? Those ids expire with every rebuild, so the call must say which
// topology it read them from.
bool usa_ids_de_topologia(const std::string& m, const json& params)
{
    auto ativo = [](const json& v) { return (v.is_number() && v.get<double>() >= 0) || (v.is_array() && !v.empty()); };
    for (const char* k : {"edge", "edges", "face", "faces", "up_to_face"})
        if (params.contains(k) && ativo(params[k])) return true;
    if (m == "measure")
        for (const char* k : {"a", "b"})
            if (params.contains(k) && params[k].is_object() && (params[k].contains("face") || params[k].contains("edge"))) return true;
    return false;
}

std::optional<json> validar(DesignPanel* p, const std::string& m, const json& params)
{
    if (numero_invalido(params))
        return erro_ia("valor_invalido", "número inválido (NaN ou infinito) nos parâmetros", "mande números finitos, em mm ou graus");
    const json& esq = esquemas();
    if (esq.contains(m)) {
        const json& lista = esq[m];
        for (auto it = params.begin(); it != params.end(); ++it) {
            const std::string& k = it.key();
            if (k == "generation") continue;
            const json* def = nullptr;
            for (const json& d : lista)
                if (d.value("name", std::string()) == k) { def = &d; break; }
            if (def == nullptr)
                return erro_ia("parametro_desconhecido", m + " não conhece o parâmetro '" + k + "'",
                               "aceitos: " + nomes_dos_parametros(lista) + " (e generation)");
            const json& v = it.value();
            const std::string tipo = def->value("type", std::string());
            if (!tipo.empty() && !tipo_confere(v, tipo))
                return erro_ia("tipo_errado", "'" + k + "' de " + m + " precisa ser " + tipo + ", veio " + v.type_name(),
                               def->contains("description") ? k + ": " + def->value("description", std::string()) : k + ": " + tipo);
            if (def->contains("enum") && (*def)["enum"].is_array()) {
                bool achou = false;
                for (const json& e : (*def)["enum"]) achou = achou || e == v;
                if (!achou) return erro_ia("valor_invalido", "'" + k + "' = " + v.dump() + " não vale em " + m, "use um de: " + (*def)["enum"].dump());
            }
            if (v.is_number()) {
                const double x = v.get<double>();
                if ((def->contains("min") && x < (*def)["min"].get<double>()) || (def->contains("max") && x > (*def)["max"].get<double>()))
                    return erro_ia("fora_da_faixa", "'" + k + "' = " + v.dump() + " fora da faixa em " + m,
                                   k + " vai de " + (def->contains("min") ? (*def)["min"].dump() : std::string("-")) + " a " +
                                       (def->contains("max") ? (*def)["max"].dump() : std::string("-")));
            }
        }
    }
    if (params.contains("profile")) {
        const json& pr = params["profile"];
        bool ok = pr.is_array() && pr.size() >= 3;
        for (size_t i = 0; ok && i < pr.size(); ++i)
            ok = pr[i].is_array() && pr[i].size() >= 2 && pr[i][0].is_number() && pr[i][1].is_number();
        if (!ok)
            return erro_ia("perfil_invalido", "profile precisa de pelo menos 3 pontos [[x,y],[x,y],[x,y],...]",
                           "um contorno fechado em mm no plano do esboço; para um retângulo centrado, tire profile e use width/height");
    }
    if (usa_ids_de_topologia(m, params) && !params.contains("generation")) {
        std::string dica = "a generation atual é " + std::to_string(p->mcp_doc().topo_generation) +
                           ": leia os ids em query_topology e mande \"generation\" junto com eles";
        if (m == "fillet" || m == "chamfer")
            dica += ", ou troque 'edge' por \"arestas\": \"topo\" | \"base\" | \"verticais\" | \"horizontais\" | \"circulares\" | \"todas\"";
        return erro_ia("generation_obrigatorio", m + " recebeu ids de face/aresta sem 'generation'", dica);
    }
    return std::nullopt;
}

// Steps after which the part may have changed: they get the printability report.
bool muda_a_peca(const std::string& m)
{
    static const std::set<std::string> leitura = {
        "describe_tools", "describe_scene", "query_topology", "measure", "mass_properties", "slice_body", "list_verbs",
        "check_interference", "ver", "view", "laudo", "enquadrar", "mandar_para_preparacao"};
    return leitura.count(m) == 0 && m.rfind("sketch_", 0) != 0;
}

json laudo_atual()
{
    return rpc("laudo", json::object()).value("resultado", json());
}

json mandar_para_preparacao(DesignPanel* p, const json& params)
{
    if (p->mcp_doc().bodies.empty()) return erro_ia("sem_corpo", "não há peça para mandar", "modele a peça antes");
    const json l = laudo_atual();
    if (l.value("status", std::string()) == "bloqueado" && !params.value("forcar", false)) {
        std::string cods;
        for (const json& c : l.value("codigos", json::array())) cods += (cods.empty() ? "" : ", ") + c.get<std::string>();
        json r = erro_ia("laudo_bloqueado", "a peça não está pronta para imprimir: " + cods,
                         "corrija o que o laudo aponta e mande de novo; se o cliente quiser mesmo assim, mande {\"forcar\": true}");
        r["laudo"] = l;
        return r;
    }
    json r = json::parse(p->filuna_ia_extra("mandar_para_preparacao"));
    r["laudo"] = l;
    return r;
}

// One step of a request, on the main thread: validated, run, and judged.
json executar_passo(DesignPanel* p, const json& c)
{
    const std::string m = c.is_object() && c.contains("metodo") && c["metodo"].is_string() ? c["metodo"].get<std::string>() : std::string();
    json r;
    try {
        const json params = c.is_object() && c.contains("parametros") ? c["parametros"] : json::object();
        if (m.empty())
            r = erro_ia("passo_invalido", "passo sem 'metodo'", "cada passo é {\"metodo\": \"...\", \"parametros\": {...}}");
        else if (!params.is_object())
            r = erro_ia("tipo_errado", "'parametros' precisa ser um objeto", "{\"metodo\": \"" + m + "\", \"parametros\": {...}}");
        else if (blocked_for_ai(m))
            r = erro_ia("metodo_bloqueado", m + " não está liberado para a IA", "esses comandos leem arquivos do computador do cliente");
        else if (std::optional<json> e = validar(p, m, params))
            r = *e;
        else if (m == "extrude_sketch")
            r = extrude_sketch(p, params);
        else if (m == "mandar_para_preparacao")
            r = mandar_para_preparacao(p, params);
        else {
            const std::string extra = p->filuna_ia_extra(m);
            r = extra.empty() ? rpc(m, params) : json::parse(extra);
        }
        // A control method can answer ok at the envelope and still report a refusal inside.
        if (r.value("ok", false) && r.contains("resultado") && r["resultado"].is_object() && r["resultado"].contains("ok") &&
            r["resultado"]["ok"].is_boolean() && !r["resultado"]["ok"].get<bool>()) {
            r["ok"]     = false;
            r["codigo"] = "operacao_falhou";
            r["erro"]   = r["resultado"].value("error", std::string("a operação não deu certo"));
            r["dica"]   = "o passo foi desfeito e a peça ficou como antes; mude as medidas (raio menor, distância maior...) e tente de novo";
        }
        if (!r.value("ok", false) && !r.contains("codigo")) {
            r["codigo"] = "erro_na_operacao";
            if (!r.contains("dica")) r["dica"] = "confira os parâmetros e o estado com describe_scene";
        }
        if (r.value("ok", false) && muda_a_peca(m)) r["laudo"] = laudo_atual();
    } catch (const std::exception& ex) {
        r = erro_ia(std::string(ex.what()).find("type_error") != std::string::npos ? "tipo_errado" : "erro_na_operacao", ex.what(),
                    "confira os parâmetros na lista de comandos de filuna_cad");
    }
    r["metodo"] = m;
    return r;
}

// The steps that left something to undo, for the panel's "Desfazer".
bool conta_para_desfazer(const json& passo)
{
    const std::string m = passo.value("metodo", std::string());
    return passo.value("ok", false) && m != "desfazer" && m != "refazer" && (muda_a_peca(m) || m == "sketch_commit");
}

std::string id_desta_instancia()
{
    static const std::string id = [] {
        std::random_device rd;
        std::mt19937_64    g((uint64_t(rd()) << 32) ^ rd() ^ uint64_t(std::time(nullptr)));
        char b[20];
        std::snprintf(b, sizeof(b), "%016llx", static_cast<unsigned long long>(g()));
        return std::string(b);
    }();
    return id;
}

std::string api_base() { return filuna_site_base() + "/api/slicer/v1"; }

struct RespostaHttp { bool ok{false}; unsigned status{0}; std::string corpo; };

// Off the main thread only.
RespostaHttp http_site(const std::string& metodo, const std::string& rota, const std::string& token, const std::string& corpo = {})
{
    RespostaHttp r;
    Http h = metodo == "GET" ? Http::get(api_base() + rota) : Http::post(api_base() + rota);
    h.header("Authorization", "Bearer " + token).timeout_connect(5).timeout_max(30);
    if (metodo != "GET") h.header("Content-Type", "application/json").set_post_body(corpo);
    h.on_complete([&r](std::string b, unsigned st) { r.ok = st >= 200 && st < 300; r.status = st; r.corpo = std::move(b); })
        .on_error([&r](std::string b, std::string, unsigned st) { r.status = st; r.corpo = std::move(b); })
        .perform_sync();
    return r;
}

std::string id_do_pedido(const json& p)
{
    if (!p.contains("id")) return std::string();
    return p["id"].is_string() ? p["id"].get<std::string>() : p["id"].dump();
}

} // namespace

// --- the "Modelar com IA" queue ---------------------------------------------------------------------------------
// Read by C++ every 3 s for as long as the customer's session lasts — not by the page: a hidden WebView may stop
// its timers, a layout change may hide the pane, a reload may forget what already ran. A request is reserved for
// this instance (?reservar=1&instancia=, newer site), marked as started before its first step, run one step per
// event-loop turn (the window keeps answering) within 120 s, and its result posted; a POST that fails is retried.
struct FilunaIaFila : std::enable_shared_from_this<FilunaIaFila>
{
    explicit FilunaIaFila(DesignPanel* p) : painel(p) {}
    ~FilunaIaFila() { if (timer) timer->Stop(); }

    struct Timer : wxTimer {
        std::function<void()> f;
        void Notify() override { if (f) f(); }
    };

    DesignPanel*           painel;
    std::unique_ptr<Timer> timer;
    bool                   rede{false};            // a request to the site is in flight
    bool                   envio_novo{false};      // something was queued to post while `rede` was busy
    std::string            token_publicado;        // the tool list went to the site for this account
    std::string            ultimo_estado;
    std::deque<json>       fila;                   // fetched, waiting to run here
    std::set<std::string>  iniciados;              // never run the same request twice
    std::vector<std::pair<std::string, std::string>> envios;   // (route, body) still to post
    json                   conectadas = json::array();
    std::string            endereco;
    json                   historico = json::array();          // newest first, for the panel

    bool                   executando{false};
    json                   atual;
    json                   passos;
    size_t                 proximo{0};
    bool                   tudo{true};
    std::chrono::steady_clock::time_point inicio;

    static constexpr int kOrcamentoSeg = 120;

    void ligar()
    {
        if (!timer) {
            timer = std::make_unique<Timer>();
            std::weak_ptr<FilunaIaFila> w = shared_from_this();
            timer->f = [w] { if (auto f = w.lock()) f->tique(); };
        }
        if (!timer->IsRunning()) timer->Start(3000);
        tique();
    }

    std::string token() const { return wxGetApp().app_config->get("filuna_token"); }

    json resumo() const
    {
        std::string ocupado;
        if (executando) ocupado = "Sua " + atual.value("ia", std::string("IA")) + " está modelando: " + atual.value("texto", std::string());
        return json{{"sessao", painel->filuna_ia_session()}, {"ocupado", ocupado}, {"log", historico},
                    {"conectadas", conectadas}, {"endereco", endereco}, {"na_fila", int(fila.size())}, {"instancia", id_desta_instancia()}};
    }

    void avisar_tela() const
    {
        if (FilunaCadIaPainel* t = FilunaCadIaPainel::atual()) t->evento("cadia.fila", resumo());
    }

    // Every 3 s on the main thread: keep the pane in step, publish the Design state if it changed, fetch requests.
    void tique()
    {
        if (!painel->filuna_ia_session()) {   // ended: only the posts still owed (results, refusals) go out
            if (envios.empty() && !executando) timer->Stop();
            else if (!rede) tique_envios();
            return;
        }
        painel->filuna_ia_sync_pane();
        const std::string tk = token();
        if (tk.empty() || rede || executando) return;

        const bool levar = token_publicado != tk;
        std::optional<json> e;
        try { e = filuna_cad_acao("cad.estado", json{{"com_comandos", levar}}); } catch (const std::exception&) {}
        std::string corpo_estado;
        if (e && e->value("aberto", false)) {
            json est = *e;
            json ferramentas = est.contains("comandos") ? est["comandos"] : json();
            est.erase("comandos");
            const std::string t = est.dump();
            if (t != ultimo_estado || levar) {
                json corpo{{"estado", est}};
                if (levar && !ferramentas.is_null()) corpo["ferramentas"] = ferramentas;
                corpo_estado = corpo.dump();
                ultimo_estado = t;
            }
        }
        auto envios_agora = envios;
        rede = true;
        std::weak_ptr<FilunaIaFila> w = shared_from_this();
        std::thread([w, tk, corpo_estado, envios_agora, levar] {
            bool publicou = true;
            if (!corpo_estado.empty()) publicou = http_site("POST", "/ia/cad", tk, corpo_estado).ok;
            std::vector<size_t> enviados;
            for (size_t i = 0; i < envios_agora.size(); ++i) {
                const RespostaHttp r = http_site("POST", envios_agora[i].first, tk, envios_agora[i].second);
                if (r.ok || (r.status >= 400 && r.status < 500)) enviados.push_back(i);   // a 4xx will not get better
            }
            const RespostaHttp g = http_site("GET", "/ia/cad/pedidos?reservar=1&instancia=" + id_desta_instancia(), tk);
            wxGetApp().CallAfter([w, tk, publicou, levar, enviados, envios_agora, g] {
                auto f = w.lock();
                if (!f) return;
                f->rede = false;
                if (!publicou) f->ultimo_estado.clear();
                else if (levar) f->token_publicado = tk;
                for (size_t k = enviados.size(); k-- > 0;) {
                    auto it = std::find(f->envios.begin(), f->envios.end(), envios_agora[enviados[k]]);
                    if (it != f->envios.end()) f->envios.erase(it);
                }
                if (f->envio_novo) { f->envio_novo = false; f->tique_envios(); }
                f->receber(g);
            });
        }).detach();
    }

    void receber(const RespostaHttp& g)
    {
        const json r = json::parse(g.corpo, nullptr, false);
        if (!g.ok || !r.is_object()) return;
        if (r.contains("conectadas")) conectadas = r["conectadas"];
        if (r.contains("endereco") && r["endereco"].is_string()) endereco = r["endereco"].get<std::string>();
        // An older site does not reserve: it lists every 'novo' request, and `iniciados` keeps them from running twice.
        if (r.contains("pedidos") && r["pedidos"].is_array())
            for (const json& p : r["pedidos"]) {
                const std::string id = id_do_pedido(p);
                if (id.empty() || iniciados.count(id)) continue;
                bool ja = false;
                for (const json& q : fila) ja = ja || id_do_pedido(q) == id;
                if (!ja) fila.push_back(p);
            }
        if (!painel->filuna_ia_session()) {   // ended while the fetch was out: refuse what came
            recusar_fila();
            return;
        }
        avisar_tela();
        if (!executando && !fila.empty()) iniciar_proximo();
    }

    void iniciar_proximo()
    {
        if (executando || fila.empty()) return;
        atual = fila.front();
        fila.pop_front();
        const std::string id = id_do_pedido(atual);
        iniciados.insert(id);   // before the first step
        executando = true;
        passos     = json::array();
        proximo    = 0;
        tudo       = true;
        inicio     = std::chrono::steady_clock::now();
        // The customer's AI is working: show its pane and the Design tab, whatever layout hid them.
        if (!painel->filuna_ia_open()) painel->filuna_toggle_ia(true);
        if (wxGetApp().mainframe != nullptr && !painel->IsShownOnScreen()) wxGetApp().mainframe->select_filuna_tab(TAB_ID_DESIGN);
        avisar_tela();
        seguir();
    }

    void seguir()
    {
        std::weak_ptr<FilunaIaFila> w = shared_from_this();
        wxGetApp().CallAfter([w] { if (auto f = w.lock()) f->passo(); });
    }

    void passo()
    {
        if (!executando) return;
        json cmds = atual.contains("comandos") ? atual["comandos"] : json::array();
        if (cmds.is_string()) cmds = json::parse(cmds.get<std::string>(), nullptr, false);   // a site that stores it as text
        if (!cmds.is_array() || cmds.empty() || cmds.size() > 60) {
            passos.push_back(erro_ia("pedido_invalido", cmds.is_array() && cmds.size() > 60 ? "passos demais num pedido (máximo 60)" : "nenhum passo para fazer",
                                     "mande de 1 a 60 passos em 'comandos'"));
            tudo = false;
            return terminar();
        }
        if (proximo >= cmds.size()) return terminar();
        const auto passou = std::chrono::steady_clock::now() - inicio;
        if (passou > std::chrono::seconds(kOrcamentoSeg)) {
            for (size_t i = proximo; i < cmds.size(); ++i) {
                json e = erro_ia("tempo_esgotado", "o pedido passou de " + std::to_string(kOrcamentoSeg) + " s e este passo não foi feito",
                                 "mande os passos que faltam num pedido novo, em partes menores");
                e["metodo"] = cmds[i].is_object() ? cmds[i].value("metodo", std::string()) : std::string();
                passos.push_back(e);
            }
            tudo = false;
            return terminar();
        }
        json r = executar_passo(painel, cmds[proximo++]);
        const bool ok = r.value("ok", false);
        passos.push_back(std::move(r));
        if (!ok) { tudo = false; return terminar(); }
        seguir();   // next step on the next event-loop turn: the window answers in between
    }

    void terminar()
    {
        executando = false;
        const std::string id = id_do_pedido(atual);
        int feitos = 0, alteracoes = 0;
        std::string erro;
        for (const json& s : passos) {
            if (s.value("ok", false)) ++feitos;
            else if (erro.empty()) erro = s.value("metodo", std::string()) + ": " + s.value("erro", std::string());
            if (conta_para_desfazer(s)) ++alteracoes;
        }
        // The images leave the result (it is cut at 40,000 characters on the site) and go in their own field.
        json imagens = json::array();
        for (json& s : passos)
            if (s.contains("resultado") && s["resultado"].is_object() && s["resultado"].contains("imagens") && s["resultado"]["imagens"].is_array())
                for (json& im : s["resultado"]["imagens"])
                    if (im.is_object() && im.contains("png_base64")) {
                        imagens.push_back(json{{"nome", im.value("vista", std::string())}, {"png_base64", im["png_base64"]}});
                        im.erase("png_base64");
                        im["imagem"] = int(imagens.size()) - 1;
                    }
        // The site takes up to 6 images and nginx a 4 MB body: past either, the last images stay out, and the step
        // that made each one says so.
        constexpr size_t kMaxImagens = 6, kMaxCorpo = 3500000;
        auto tamanho_corpo = [&] { return json{{"resultado", passos}, {"imagens", imagens}}.dump().size(); };
        while (!imagens.empty() && (imagens.size() > kMaxImagens || tamanho_corpo() > kMaxCorpo)) {
            const int ultima = int(imagens.size()) - 1;
            imagens.erase(imagens.end() - 1);
            for (json& s : passos)
                if (s.contains("resultado") && s["resultado"].is_object() && s["resultado"].contains("imagens"))
                    for (json& im : s["resultado"]["imagens"])
                        if (im.is_object() && im.contains("imagem") && im["imagem"].is_number_integer() && im["imagem"].get<int>() == ultima) {
                            im["imagem"]  = nullptr;
                            im["omitida"] = "imagem não enviada: passou do limite do site (6 imagens, 3,5 MB); peça menos vistas por pedido";
                        }
        }
        const json total = atual.contains("comandos") && atual["comandos"].is_array() ? int(atual["comandos"].size()) : 0;
        historico.insert(historico.begin(), json{{"id", id}, {"ia", atual.value("ia", std::string())}, {"texto", atual.value("texto", std::string())},
                                                 {"ok", tudo}, {"feitos", feitos}, {"total", total}, {"alteracoes", alteracoes}, {"erro", erro}});
        if (historico.size() > 8) historico.erase(historico.begin() + 8, historico.end());
        postar(id, json{{"estado", tudo ? "feito" : "erro"}, {"resultado", passos}, {"imagens", imagens}});
        avisar_tela();
        if (!fila.empty()) {
            std::weak_ptr<FilunaIaFila> w = shared_from_this();
            wxGetApp().CallAfter([w] { if (auto f = w.lock()) f->iniciar_proximo(); });
        }
    }

    void postar(const std::string& id, const json& corpo)
    {
        if (id.empty()) return;
        envios.emplace_back("/ia/cad/pedidos/" + Http::url_encode(id), corpo.dump());
        if (!rede) tique_envios();
        else envio_novo = true;   // sent as soon as the request in flight returns
    }

    // Post what is waiting now rather than on the next tick (which skips while a request runs).
    void tique_envios()
    {
        const std::string tk = token();
        if (tk.empty() || envios.empty()) return;
        auto envios_agora = envios;
        rede = true;
        std::weak_ptr<FilunaIaFila> w = shared_from_this();
        std::thread([w, tk, envios_agora] {
            std::vector<size_t> enviados;
            for (size_t i = 0; i < envios_agora.size(); ++i) {
                const RespostaHttp r = http_site("POST", envios_agora[i].first, tk, envios_agora[i].second);
                if (r.ok || (r.status >= 400 && r.status < 500)) enviados.push_back(i);
            }
            wxGetApp().CallAfter([w, enviados, envios_agora] {
                auto f = w.lock();
                if (!f) return;
                f->rede = false;
                for (size_t k = enviados.size(); k-- > 0;) {
                    auto it = std::find(f->envios.begin(), f->envios.end(), envios_agora[enviados[k]]);
                    if (it != f->envios.end()) f->envios.erase(it);
                }
                if (f->envio_novo) { f->envio_novo = false; f->tique_envios(); }
            });
        }).detach();
    }

    void recusar_fila()
    {
        while (!fila.empty()) {
            const std::string id = id_do_pedido(fila.front());
            fila.pop_front();
            if (id.empty() || iniciados.count(id)) continue;
            iniciados.insert(id);
            postar(id, json{{"estado", "recusado"}, {"resultado", "o cliente encerrou o Modelar com IA"}});
        }
    }

    // The customer ended the session: stop reading, refuse what is queued here and what the site still holds.
    // A request already running finishes and is posted as usual.
    void encerrar()
    {
        recusar_fila();   // the timer keeps going only until what is owed has been posted (see tique)
        const std::string tk = token();
        if (tk.empty()) return;
        std::weak_ptr<FilunaIaFila> w = shared_from_this();
        std::thread([w, tk] {
            const RespostaHttp g = http_site("GET", "/ia/cad/pedidos?reservar=1&instancia=" + id_desta_instancia(), tk);
            wxGetApp().CallAfter([w, g] {
                auto f = w.lock();
                if (!f || f->painel->filuna_ia_session()) return;   // reopened meanwhile: the queue will run them
                f->receber(g);
            });
        }).detach();
        avisar_tela();
    }

    // The panel's "Desfazer": undo what the last request changed.
    void desfazer_ultimo()
    {
        if (historico.empty() || historico[0].value("desfeito", false)) return;
        const int n = historico[0].value("alteracoes", 0);
        for (int i = 0; i < n; ++i) painel->filuna_ia_extra("desfazer");
        historico[0]["desfeito"] = true;
        avisar_tela();
    }
};

void DesignPanel::filuna_toggle_ia(bool show)
{
    if (show && m_cad_ia == nullptr) {
        m_cad_ia = new FilunaCadIaPainel(m_form->GetParent());
        m_aui.AddPane(m_cad_ia, wxAuiPaneInfo().Name("filuna_cad_ia").Right().Layer(2).CaptionVisible(false)
                                    .PaneBorder(false).CloseButton(false).Floatable(false)
                                    .BestSize(FromDIP(wxSize(440, -1))).MinSize(FromDIP(wxSize(340, -1))));
    }
    if (m_cad_ia == nullptr) return;
    m_ia_open = show;
    m_aui.GetPane(m_cad_ia).Show(show);
    m_aui.Update();
    if (m_viewport) m_viewport->request_repaint();
    if (show) {
        m_ia_session = true;
        if (!m_ia_fila) m_ia_fila = std::make_shared<FilunaIaFila>(this);
        m_ia_fila->ligar();
    } else if (m_ia_session) {
        m_ia_session = false;
        if (m_ia_fila) m_ia_fila->encerrar();
    }
}

void DesignPanel::filuna_ia_sync_pane()
{
    if (m_cad_ia == nullptr) return;
    wxAuiPaneInfo& pane = m_aui.GetPane(m_cad_ia);
    if (!pane.IsOk()) return;
    if (m_ia_session && !pane.IsShown()) {
        pane.Show(true);
        m_aui.Update();
    }
    m_ia_open = pane.IsShown();
}

std::string DesignPanel::filuna_mode_name() const
{
    if (m_choosing_sketch_plane) return "escolhendo o plano do esboço";
    switch (m_ui_mode) {
    case UiMode::Sketch:    return "esboço aberto";
    case UiMode::Constrain: return "restrições do esboço";
    default:                return m_active != Tool::None ? "ferramenta aberta: " + filuna_tool_title().ToStdString(wxConvUTF8) : "livre";
    }
}

std::string DesignPanel::filuna_ia_extra(const std::string& m)
{
    auto ok = [](const char* t) { return json{{"ok", true}, {"resultado", t}}.dump(); };
    if (m == "desfazer") { do_undo_redo(false); return ok("desfeito"); }
    if (m == "refazer") { do_undo_redo(true); return ok("refeito"); }
    if (m == "enquadrar") { if (m_viewport) m_viewport->fit_view(); return ok("vista ajustada"); }
    if (m == "novo") { clear_document(); return ok("design vazio"); }
    if (m == "mandar_para_preparacao") { on_commit(); return ok("mandado para a Preparação"); }
    // What the customer left open (a sketch, a tool card) would refuse every modelling step: the AI
    // closes it the way the customer would, keeping it (concluir) or dropping it (cancelar).
    if (m == "concluir") { tool_confirm(); return ok("esboço ou ferramenta concluída"); }
    if (m == "cancelar") { tool_cancel(); return ok("esboço ou ferramenta cancelada"); }
    return std::string();
}

std::optional<json> filuna_cad_acao(const std::string& acao, const json& d)
{
    if (acao.rfind("cad.", 0) != 0 && acao.rfind("cadia.", 0) != 0) return std::nullopt;
    DesignPanel* p = DesignPanel::if_built();

    if (acao == "cad.estado") {
        if (p == nullptr) return json{{"aberto", false}};
        json scene = rpc("describe_scene", json::object());
        json out{{"aberto", true}, {"modo", p->filuna_mode_name()}, {"cena", scene.value("resultado", json())}};
        if (d.value("com_comandos", false)) {
            json tools = rpc("describe_tools", json::object()).value("resultado", json::object());
            if (tools.contains("tools") && tools["tools"].is_array()) {
                json keep = json::array();
                for (const json& t : tools["tools"])
                    if (!blocked_for_ai(t.value("name", std::string()))) keep.push_back(t);
                for (const json& t : extra_tools()) keep.push_back(t);
                tools["tools"] = keep;
            }
            out["comandos"] = tools;
        }
        return out;
    }
    // Synchronous run (the panel's own undo, older pages). The AI's queue runs step by step in FilunaIaFila.
    if (acao == "cad.executar") {
        p = DesignPanel::ensure();
        if (p == nullptr) throw std::runtime_error("O Design ainda não abriu.");
        if (wxGetApp().mainframe != nullptr && !p->IsShownOnScreen())
            wxGetApp().mainframe->select_filuna_tab(TAB_ID_DESIGN);
        const json cmds = d.contains("comandos") ? d["comandos"] : json::array();
        if (!cmds.is_array() || cmds.empty()) throw std::runtime_error("Nenhum passo para fazer.");
        if (cmds.size() > 60) throw std::runtime_error("Passos demais num pedido.");
        json passos = json::array();
        bool tudo   = true;
        for (const json& c : cmds) {
            json r = executar_passo(p, c);
            const bool ok = r.value("ok", false);
            passos.push_back(std::move(r));
            if (!ok) { tudo = false; break; }
        }
        return json{{"ok", tudo}, {"passos", passos}, {"modo", p->filuna_mode_name()}};
    }
    if (acao == "cadia.abrir") {
        const wxString url = wxString::FromUTF8(d.value("url", std::string()));
        if (!url.StartsWith("https://") || !filuna_site_de_ia(url)) throw std::runtime_error("Endereço de IA não aceito.");
        FilunaCadIaPainel* painel = FilunaCadIaPainel::atual();
        if (painel == nullptr) throw std::runtime_error("Abra 'Modelar com IA' no Design primeiro.");
        painel->abrir_ia(url);
        return json::object();
    }
    if (acao == "cadia.fechar_ia") {
        if (FilunaCadIaPainel* painel = FilunaCadIaPainel::atual()) painel->fechar_ia();
        return json::object();
    }
    if (acao == "cadia.comando") {
        if (FilunaCadIaPainel* painel = FilunaCadIaPainel::atual()) painel->comando(d.value("comando", std::string()));
        return json::object();
    }
    if (acao == "cadia.painel") {   // the page's own close button: the customer ends the session
        const bool mostrar = d.value("mostrar", false);
        wxGetApp().CallAfter([mostrar] { if (DesignPanel* dp = DesignPanel::if_built()) dp->filuna_toggle_ia(mostrar); });
        return json::object();
    }
    if (acao == "cadia.agora") {   // the page wants the queue read now (after login, "Conferir")
        if (p != nullptr && p->filuna_ia_session())
            wxGetApp().CallAfter([] { if (DesignPanel* dp = DesignPanel::if_built()) dp->filuna_ia_tick_now(); });
        return json::object();
    }
    if (acao == "cadia.desfazer_ultimo") {
        if (p != nullptr) p->filuna_ia_undo_last();
        return json::object();
    }
    if (acao == "cadia.estado") {
        json e{{"aberto", p != nullptr && p->filuna_ia_open()},
               {"ia_aberta", FilunaCadIaPainel::atual() != nullptr && FilunaCadIaPainel::atual()->aberta()}};
        if (p != nullptr) e.update(p->filuna_ia_summary());
        return e;
    }
    return std::nullopt;
}

void DesignPanel::filuna_ia_tick_now()
{
    if (m_ia_fila) m_ia_fila->tique();
}

void DesignPanel::filuna_ia_undo_last()
{
    if (m_ia_fila) m_ia_fila->desfazer_ultimo();
}

nlohmann::json DesignPanel::filuna_ia_summary() const
{
    if (m_ia_fila) return m_ia_fila->resumo();
    return json{{"sessao", m_ia_session}, {"ocupado", ""}, {"log", json::array()}, {"conectadas", json::array()}, {"endereco", ""}};
}

}} // namespace Slic3r::GUI
