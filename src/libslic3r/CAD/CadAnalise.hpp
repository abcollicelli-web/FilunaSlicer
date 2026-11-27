#ifndef slic3r_CadAnalise_hpp_
#define slic3r_CadAnalise_hpp_

// Filuna CAD: what the customer's AI needs to judge a body without seeing it.
//  - Edge selection by geometry ("topo", "base", "verticais"...), so a fillet or a chamfer can be asked for
//    without first reading edge ids that expire with the next rebuild.
//  - The printability report ("laudo") of one body: closed solid, volume, loose pieces, fits and sits on the
//    bed, support area and whether the centre of mass falls inside it. Pure OCCT, no GUI, cheap enough to run
//    after every modelling step.

#include <TopoDS_Shape.hxx>

#include <array>
#include <string>
#include <vector>

namespace Slic3r { namespace CadAnalise {

enum class Arestas { Topo, Base, Verticais, Horizontais, Circulares, Todas };

// "topo" | "base" | "verticais" | "horizontais" | "circulares" | "todas" (also the English names).
bool arestas_de_nome(const std::string& nome, Arestas& out);
const char* nomes_de_arestas();   // the accepted names, for error hints

// Edge ids of `shape` (indices into TopExp::MapShapes(shape, TopAbs_EDGE), the ids query_topology and the
// fillet/chamfer handlers use) that match `qual`. Seam edges of closed surfaces and degenerate edges are never
// returned: neither is a real edge a fillet could round.
//   topo        both ends and the middle at the body's highest Z
//   base        both ends and the middle at the body's lowest Z
//   verticais   straight edges along +-Z
//   horizontais edges lying in one horizontal plane (lines and arcs)
//   circulares  circles and circular arcs
//   todas       every real edge
std::vector<int> selecionar_arestas(const TopoDS_Shape& shape, Arestas qual, double tol = 0.01);

// The printer's bed in world mm (the Design tab's world is the bed's: the modelling origin is its centre).
struct Mesa {
    double xmin{0}, ymin{0}, xmax{256}, ymax{256}, zmax{256};
    bool   do_perfil{false};   // false = the printer profile could not be read; these are the 256 mm defaults
};

struct LaudoCorpo {
    bool                  fechado{false};
    double                volume_mm3{0};
    int                   pedacos{0};          // solids in the body
    std::array<double, 3> min{}, max{};        // world bounding box
    bool                  cabe_na_mesa{false};
    bool                  fora_da_mesa{false};
    double                apoio_mm2{0};        // planar faces facing down at the lowest Z
    bool                  centro_massa_no_apoio{false};
    std::array<double, 3> centro_massa{};
    std::vector<std::string> codigos;          // corpo_aberto, volume_zero, pedacos_soltos, nao_cabe_na_mesa, fora_da_mesa, pode_tombar
};

// Codes that stop the part from going to the printer, as opposed to warnings.
bool codigo_bloqueia(const std::string& codigo);

// `max_faces_check`: above this face count the full BRepCheck pass is skipped (the closedness test still runs),
// so a big imported solid does not cost seconds after every step.
LaudoCorpo laudar_corpo(const TopoDS_Shape& shape, const Mesa& mesa, int max_faces_check = 3000);

}} // namespace Slic3r::CadAnalise

#endif // slic3r_CadAnalise_hpp_
