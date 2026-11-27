// Filuna CAD: edge selection by geometry, the printability report ("laudo") and the software rasterizer ("ver").
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "libslic3r/CAD/CadAnalise.hpp"
#include "libslic3r/CAD/CadRaster.hpp"

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pln.hxx>

#include <algorithm>
#include <string>
#include <vector>

using namespace Slic3r;
using Catch::Approx;

namespace {

TopoDS_Shape caixa(double x, double y, double z, double dx, double dy, double dz)
{
    return BRepPrimAPI_MakeBox(gp_Pnt(x, y, z), dx, dy, dz).Shape();
}

TopoDS_Shape caixa_furada()
{
    return BRepAlgoAPI_Cut(caixa(100, 100, 0, 40, 30, 20),
                           BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(120, 115, -1), gp_Dir(0, 0, 1)), 4, 30).Shape()).Shape();
}

bool tem(const CadAnalise::LaudoCorpo& l, const char* c) { return std::find(l.codigos.begin(), l.codigos.end(), c) != l.codigos.end(); }

CadAnalise::Mesa mesa256()
{
    CadAnalise::Mesa m;
    m.do_perfil = true;
    return m;
}

struct MalhaSimples { std::vector<float> v; std::vector<int32_t> t; };

MalhaSimples malha(const TopoDS_Shape& s)
{
    BRepMesh_IncrementalMesh(s, 0.05, false, 0.3, true);
    MalhaSimples m;
    for (TopExp_Explorer ex(s, TopAbs_FACE); ex.More(); ex.Next()) {
        const TopoDS_Face& f = TopoDS::Face(ex.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tr = BRep_Tool::Triangulation(f, loc);
        if (tr.IsNull()) continue;
        const int base = int(m.v.size() / 3);
        for (int i = 1; i <= tr->NbNodes(); ++i) {
            const gp_Pnt p = tr->Node(i).Transformed(loc.Transformation());
            m.v.insert(m.v.end(), {float(p.X()), float(p.Y()), float(p.Z())});
        }
        for (int i = 1; i <= tr->NbTriangles(); ++i) {
            int a, b, c;
            tr->Triangle(i).Get(a, b, c);
            m.t.insert(m.t.end(), {base + a - 1, base + b - 1, base + c - 1});
        }
    }
    return m;
}

} // namespace

TEST_CASE("Filuna CAD: edges picked by geometry", "[CadDocument][CadAnalise]")
{
    using A = CadAnalise::Arestas;
    const TopoDS_Shape bloco = caixa(100, 100, 0, 40, 30, 20);
    CHECK(CadAnalise::selecionar_arestas(bloco, A::Topo).size() == 4);
    CHECK(CadAnalise::selecionar_arestas(bloco, A::Base).size() == 4);
    CHECK(CadAnalise::selecionar_arestas(bloco, A::Verticais).size() == 4);
    CHECK(CadAnalise::selecionar_arestas(bloco, A::Horizontais).size() == 8);
    CHECK(CadAnalise::selecionar_arestas(bloco, A::Todas).size() == 12);
    CHECK(CadAnalise::selecionar_arestas(bloco, A::Circulares).empty());

    const TopoDS_Shape cil = BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(128, 128, 0), gp_Dir(0, 0, 1)), 5, 10).Shape();
    CHECK(CadAnalise::selecionar_arestas(cil, A::Circulares).size() == 2);
    CHECK(CadAnalise::selecionar_arestas(cil, A::Verticais).empty());   // the seam is not an edge
    CHECK(CadAnalise::selecionar_arestas(cil, A::Todas).size() == 2);

    const TopoDS_Shape furada = caixa_furada();
    CHECK(CadAnalise::selecionar_arestas(furada, A::Topo).size() == 5);   // the hole's rim is on top too
    CHECK(CadAnalise::selecionar_arestas(furada, A::Circulares).size() == 2);

    A a;
    CHECK((CadAnalise::arestas_de_nome("verticais", a) && a == A::Verticais));
    CHECK_FALSE(CadAnalise::arestas_de_nome("diagonais", a));
}

TEST_CASE("Filuna CAD: printability report", "[CadDocument][CadAnalise]")
{
    const CadAnalise::Mesa mesa = mesa256();

    CadAnalise::LaudoCorpo l = CadAnalise::laudar_corpo(caixa_furada(), mesa);
    CHECK(l.fechado);
    CHECK(l.pedacos == 1);
    CHECK(l.codigos.empty());
    CHECK(l.volume_mm3 == Approx(40 * 30 * 20 - 3.14159265 * 16 * 20).margin(1.0));
    CHECK(l.apoio_mm2 == Approx(40 * 30 - 3.14159265 * 16).margin(1.0));

    l = CadAnalise::laudar_corpo(caixa(100, 100, 5, 10, 10, 10), mesa);
    CHECK(tem(l, "fora_da_mesa"));

    l = CadAnalise::laudar_corpo(caixa(-20, 0, 0, 300, 10, 10), mesa);
    CHECK(tem(l, "nao_cabe_na_mesa"));
    CHECK(CadAnalise::codigo_bloqueia("nao_cabe_na_mesa"));

    l = CadAnalise::laudar_corpo(BRepAlgoAPI_Fuse(caixa(10, 10, 0, 10, 10, 10), caixa(50, 10, 0, 10, 10, 10)).Shape(), mesa);
    CHECK(tem(l, "pedacos_soltos"));
    CHECK(l.pedacos == 2);

    l = CadAnalise::laudar_corpo(BRepBuilderAPI_MakeFace(gp_Pln(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)), 0, 10, 0, 10).Shape(), mesa);
    CHECK(tem(l, "corpo_aberto"));
    CHECK(tem(l, "volume_zero"));

    // A small foot and a long arm to one side tips over.
    l = CadAnalise::laudar_corpo(BRepAlgoAPI_Fuse(caixa(100, 100, 0, 10, 10, 2), caixa(100, 100, 2, 60, 10, 10)).Shape(), mesa);
    CHECK(tem(l, "pode_tombar"));

    // A four-legged table stands: the centre of mass falls between the feet, not on one of them.
    TopoDS_Shape mesinha = caixa(100, 100, 10, 50, 50, 3);
    for (int i = 0; i < 4; ++i)
        mesinha = BRepAlgoAPI_Fuse(mesinha, caixa(100 + (i & 1) * 45, 100 + (i >> 1) * 45, 0, 5, 5, 10.5)).Shape();
    l = CadAnalise::laudar_corpo(mesinha, mesa);
    CHECK(l.centro_massa_no_apoio);
    CHECK_FALSE(tem(l, "pode_tombar"));
    CHECK(l.apoio_mm2 == Approx(100).margin(0.5));

    // A cylinder lying on its side has no flat face under it.
    l = CadAnalise::laudar_corpo(BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(100, 100, 5), gp_Dir(1, 0, 0)), 5, 30).Shape(), mesa);
    CHECK(tem(l, "pode_tombar"));
}

TEST_CASE("Filuna CAD: software views of the bodies", "[CadDocument][CadAnalise]")
{
    const MalhaSimples m = malha(caixa_furada());
    CadRaster::Malha c;
    c.xyz        = m.v.data();
    c.n_vertices = m.v.size() / 3;
    c.tri        = m.t.data();
    c.n_tri      = m.t.size() / 3;
    CadRaster::Mesa mesa;
    mesa.tem = true; mesa.xmax = 256; mesa.ymax = 256;
    for (const char* nome : {"iso", "frente", "topo", "direita", "esquerda", "tras", "baixo"}) {
        CadRaster::Vista v;
        REQUIRE(CadRaster::vista_de_nome(nome, v));
        const CadRaster::Imagem im = CadRaster::desenhar({c}, v, 320, mesa);
        CHECK_FALSE(im.vazia);
        REQUIRE(im.rgb.size() == 320u * 320u * 3u);
        // The middle of the box's wall is drawn in the body's colour family, not the background.
        const auto p = CadRaster::projetar(im, {105, 102, 10});
        const uint8_t* px = &im.rgb[(size_t(p[1]) * 320 + size_t(p[0])) * 3];
        CHECK_FALSE((px[0] == 246 && px[1] == 247 && px[2] == 249));
    }
    CadRaster::Vista v;
    CHECK_FALSE(CadRaster::vista_de_nome("diagonal", v));
}
