#include "libslic3r/CAD/CadAnalise.hpp"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <Standard_Failure.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pln.hxx>

#include <algorithm>
#include <cmath>
#include <utility>

namespace Slic3r { namespace CadAnalise {

namespace {

bool caixa_de(const TopoDS_Shape& s, std::array<double, 3>& mn, std::array<double, 3>& mx)
{
    Bnd_Box box;
    BRepBndLib::AddOptimal(s, box, /*useTriangulation*/ true, /*useShapeTolerance*/ false);
    if (box.IsVoid()) return false;
    box.Get(mn[0], mn[1], mn[2], mx[0], mx[1], mx[2]);
    return true;
}

using P2 = std::pair<double, double>;

double cruz(const P2& o, const P2& a, const P2& b)
{
    return (a.first - o.first) * (b.second - o.second) - (a.second - o.second) * (b.first - o.first);
}

// Andrew's monotone chain; counter-clockwise, no repeated last point.
std::vector<P2> casco_convexo(std::vector<P2> p)
{
    std::sort(p.begin(), p.end());
    p.erase(std::unique(p.begin(), p.end()), p.end());
    if (p.size() < 3) return p;
    std::vector<P2> h(2 * p.size());
    size_t k = 0;
    for (size_t i = 0; i < p.size(); ++i) {
        while (k >= 2 && cruz(h[k - 2], h[k - 1], p[i]) <= 0) --k;
        h[k++] = p[i];
    }
    for (size_t i = p.size() - 1, t = k + 1; i > 0; --i) {
        while (k >= t && cruz(h[k - 2], h[k - 1], p[i - 1]) <= 0) --k;
        h[k++] = p[i - 1];
    }
    h.resize(k - 1);
    return h;
}

// Inside a counter-clockwise convex polygon, or within `tol` of it.
bool dentro_do_casco(const std::vector<P2>& h, const P2& q, double tol)
{
    if (h.size() < 3) return false;
    for (size_t i = 0; i < h.size(); ++i) {
        const P2& a = h[i];
        const P2& b = h[(i + 1) % h.size()];
        const double len = std::hypot(b.first - a.first, b.second - a.second);
        if (len <= 0) continue;
        if (cruz(a, b, q) / len < -tol) return false;   // signed distance to the edge, outside beyond tol
    }
    return true;
}

} // namespace

bool arestas_de_nome(const std::string& n, Arestas& out)
{
    if (n == "topo" || n == "top")                      { out = Arestas::Topo; return true; }
    if (n == "base" || n == "bottom")                   { out = Arestas::Base; return true; }
    if (n == "verticais" || n == "vertical")            { out = Arestas::Verticais; return true; }
    if (n == "horizontais" || n == "horizontal")        { out = Arestas::Horizontais; return true; }
    if (n == "circulares" || n == "circular" || n == "circles") { out = Arestas::Circulares; return true; }
    if (n == "todas" || n == "all")                     { out = Arestas::Todas; return true; }
    return false;
}

const char* nomes_de_arestas() { return "topo, base, verticais, horizontais, circulares, todas"; }

std::vector<int> selecionar_arestas(const TopoDS_Shape& shape, Arestas qual, double tol)
{
    std::vector<int> ids;
    if (shape.IsNull()) return ids;
    std::array<double, 3> mn{}, mx{};
    if (!caixa_de(shape, mn, mx)) return ids;

    TopTools_IndexedMapOfShape mapa;
    TopExp::MapShapes(shape, TopAbs_EDGE, mapa);
    TopTools_IndexedDataMapOfShapeListOfShape faces_da_aresta;
    TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, faces_da_aresta);

    for (int i = 1; i <= mapa.Extent(); ++i) {
        const TopoDS_Edge& e = TopoDS::Edge(mapa(i));
        if (BRep_Tool::Degenerated(e)) continue;
        bool costura = false;
        if (const TopTools_ListOfShape* fs = faces_da_aresta.Seek(e))
            for (const TopoDS_Shape& f : *fs)
                if (BRep_Tool::IsClosed(e, TopoDS::Face(f))) { costura = true; break; }
        if (costura) continue;

        BRepAdaptor_Curve c(e);
        const double t0 = c.FirstParameter(), t1 = c.LastParameter();
        const gp_Pnt a = c.Value(t0), b = c.Value(t1), m = c.Value(0.5 * (t0 + t1));
        const bool plana_em_z = std::abs(a.Z() - b.Z()) < tol && std::abs(a.Z() - m.Z()) < tol;
        bool pega = false;
        switch (qual) {
        case Arestas::Topo: pega = plana_em_z && std::abs(a.Z() - mx[2]) < tol; break;
        case Arestas::Base: pega = plana_em_z && std::abs(a.Z() - mn[2]) < tol; break;
        case Arestas::Verticais:
            if (c.GetType() == GeomAbs_Line) pega = std::abs(c.Line().Direction().Z()) > 0.999;
            break;
        case Arestas::Horizontais: pega = plana_em_z; break;
        case Arestas::Circulares: pega = c.GetType() == GeomAbs_Circle; break;
        case Arestas::Todas: pega = true; break;
        }
        if (pega) ids.push_back(i - 1);
    }
    return ids;
}

bool codigo_bloqueia(const std::string& c)
{
    return c == "corpo_aberto" || c == "volume_zero" || c == "nao_cabe_na_mesa";
}

LaudoCorpo laudar_corpo(const TopoDS_Shape& shape, const Mesa& mesa, int max_faces_check)
{
    LaudoCorpo r;
    if (shape.IsNull()) {
        r.codigos = {"corpo_aberto", "volume_zero"};
        return r;
    }
    const double tol = 0.05;

    // Closed: at least one solid, every shell closed, no shell or face loose outside a solid, valid B-rep.
    int  solidos = 0;
    bool fechado = true;
    for (TopExp_Explorer ex(shape, TopAbs_SOLID); ex.More(); ex.Next()) ++solidos;
    for (TopExp_Explorer ex(shape, TopAbs_SHELL); ex.More() && fechado; ex.Next())
        if (!BRep_Tool::IsClosed(ex.Current())) fechado = false;
    if (TopExp_Explorer(shape, TopAbs_SHELL, TopAbs_SOLID).More()) fechado = false;
    if (TopExp_Explorer(shape, TopAbs_FACE, TopAbs_SHELL).More()) fechado = false;
    int n_faces = 0;
    for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next()) ++n_faces;
    if (fechado && solidos > 0 && n_faces <= max_faces_check) {
        try {
            if (!BRepCheck_Analyzer(shape).IsValid()) fechado = false;
        } catch (const Standard_Failure&) {
            fechado = false;
        }
    }
    r.fechado = fechado && solidos > 0;
    r.pedacos = solidos;

    // Volume and centre of mass over the solids only (a loose sheet encloses nothing).
    double vol = 0;
    gp_XYZ momento(0, 0, 0);
    for (TopExp_Explorer ex(shape, TopAbs_SOLID); ex.More(); ex.Next()) {
        GProp_GProps p;
        BRepGProp::VolumeProperties(ex.Current(), p);
        const double v = std::abs(p.Mass());
        vol += v;
        momento += p.CentreOfMass().XYZ() * v;
    }
    r.volume_mm3 = vol;
    if (vol > 0) {
        const gp_XYZ cm = momento / vol;
        r.centro_massa = {cm.X(), cm.Y(), cm.Z()};
    }

    if (!caixa_de(shape, r.min, r.max)) {
        r.codigos = {"corpo_aberto", "volume_zero"};
        return r;
    }
    const double dx = r.max[0] - r.min[0], dy = r.max[1] - r.min[1], dz = r.max[2] - r.min[2];
    r.cabe_na_mesa = dx <= mesa.xmax - mesa.xmin + 0.01 && dy <= mesa.ymax - mesa.ymin + 0.01 && dz <= mesa.zmax + 0.01;
    r.fora_da_mesa = std::abs(r.min[2]) > tol || r.min[0] < mesa.xmin - tol || r.max[0] > mesa.xmax + tol ||
                     r.min[1] < mesa.ymin - tol || r.max[1] > mesa.ymax + tol || r.max[2] > mesa.zmax + tol;

    // Support: planar faces whose outward normal points down, lying at the lowest Z. The body stands on the
    // convex hull of those faces (a table stands on its four feet), so that is where the centre of mass must fall.
    std::vector<P2> pontos;
    for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next()) {
        const TopoDS_Face& f = TopoDS::Face(ex.Current());
        BRepAdaptor_Surface s(f, false);
        if (s.GetType() != GeomAbs_Plane) continue;
        const gp_Pln pl = s.Plane();
        double nz = pl.Axis().Direction().Z();
        if (f.Orientation() == TopAbs_REVERSED) nz = -nz;
        if (nz > -0.999) continue;
        std::array<double, 3> fmn{}, fmx{};
        if (!caixa_de(f, fmn, fmx) || fmx[2] - r.min[2] > tol) continue;
        GProp_GProps p;
        BRepGProp::SurfaceProperties(f, p);
        r.apoio_mm2 += std::abs(p.Mass());
        for (TopExp_Explorer ee(f, TopAbs_EDGE); ee.More(); ee.Next()) {
            const TopoDS_Edge& e = TopoDS::Edge(ee.Current());
            if (BRep_Tool::Degenerated(e)) continue;
            BRepAdaptor_Curve c(e);
            const int n = c.GetType() == GeomAbs_Line ? 1 : 16;
            for (int k = 0; k <= n; ++k) {
                const gp_Pnt q = c.Value(c.FirstParameter() + (c.LastParameter() - c.FirstParameter()) * k / n);
                pontos.emplace_back(q.X(), q.Y());
            }
        }
    }
    r.centro_massa_no_apoio = vol > 0 && dentro_do_casco(casco_convexo(pontos), {r.centro_massa[0], r.centro_massa[1]}, tol);

    if (!r.fechado) r.codigos.push_back("corpo_aberto");
    if (vol < 1e-3) r.codigos.push_back("volume_zero");
    if (solidos > 1) r.codigos.push_back("pedacos_soltos");
    if (!r.cabe_na_mesa) r.codigos.push_back("nao_cabe_na_mesa");
    else if (r.fora_da_mesa) r.codigos.push_back("fora_da_mesa");
    if (r.fechado && vol >= 1e-3 && !r.centro_massa_no_apoio) r.codigos.push_back("pode_tombar");
    return r;
}

}} // namespace Slic3r::CadAnalise
