// Filuna CAD: the Design tab's tools as floating columns over the 3D view (see FilunaCadUi.hpp).
// Every button runs an action the tab already had — the offer verbs, the key maps, the panel's
// own handlers — so nothing the ribbon could do is lost; the ribbon is only hidden.

#include "slic3r/GUI/CAD/DesignPanel.hpp"
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

#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>

namespace Slic3r { namespace GUI {

namespace {

wxString pt(const char* s) { return wxString::FromUTF8(s); }

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

private:
    static inline FilunaCadIaPainel* g_atual = nullptr;
    FilunaPanel* m_tela{nullptr};
    wxWebView*   m_ia{nullptr};
    bool         m_aberta{false};
};

json rpc(const std::string& method, const json& params)
{
    json r;
    try { r = json::parse(filuna_cad_rpc(method, params.dump())); }
    catch (const std::exception& ex) { return json{{"ok", false}, {"erro", ex.what()}}; }
    if (r.contains("error"))
        return json{{"ok", false}, {"erro", r["error"].value("message", std::string("erro"))}};
    return json{{"ok", true}, {"resultado", r.contains("result") ? r["result"] : json()}};
}

} // namespace

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
                tools["tools"] = keep;
            }
            out["comandos"] = tools;
        }
        return out;
    }
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
            const std::string m = c.value("metodo", std::string());
            const json params   = c.contains("parametros") && c["parametros"].is_object() ? c["parametros"] : json::object();
            json r;
            if (m.empty()) r = json{{"ok", false}, {"erro", "passo sem 'metodo'"}};
            else if (blocked_for_ai(m)) r = json{{"ok", false}, {"erro", m + " não está liberado para a IA"}};
            else {
                const std::string extra = p->filuna_ia_extra(m);
                r = extra.empty() ? rpc(m, params) : json::parse(extra);
            }
            r["metodo"] = m;
            // A control method can answer ok at the envelope and still report a refusal inside.
            if (r.value("ok", false) && r["resultado"].is_object() && r["resultado"].contains("ok") && r["resultado"]["ok"].is_boolean()
                && !r["resultado"]["ok"].get<bool>())
                r["ok"] = false;
            passos.push_back(r);
            if (!r.value("ok", false)) { tudo = false; break; }
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
    if (acao == "cadia.painel") {   // the page's own close button
        const bool mostrar = d.value("mostrar", false);
        wxGetApp().CallAfter([mostrar] { if (DesignPanel* dp = DesignPanel::if_built()) dp->filuna_toggle_ia(mostrar); });
        return json::object();
    }
    if (acao == "cadia.estado")
        return json{{"aberto", p != nullptr && p->filuna_ia_open()},
                    {"ia_aberta", FilunaCadIaPainel::atual() != nullptr && FilunaCadIaPainel::atual()->aberta()}};
    return std::nullopt;
}

}} // namespace Slic3r::GUI
