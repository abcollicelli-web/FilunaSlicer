#ifndef slic3r_CadRaster_hpp_
#define slic3r_CadRaster_hpp_

// Filuna CAD: a small software rasterizer that draws the Design bodies for the customer's AI ("ver"), with no
// OpenGL window — it works with the Design tab hidden. Orthographic camera, z-buffer, flat shading from a fixed
// light, one colour per body, outlines where the depth, the body or the surface direction jumps, and the bed
// (grid on Z=0 seen from above, the bed line seen from the side). 2x supersampled. No dependencies.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Slic3r { namespace CadRaster {

enum class Vista { Iso, Frente, Topo, Direita, Esquerda, Tras, Baixo };

// Portuguese names (iso, frente, topo, direita, esquerda, tras, baixo) and the English ones.
bool        vista_de_nome(const std::string& nome, Vista& v);
const char* nome_da_vista(Vista v);
const char* nomes_das_vistas();

// One body: `xyz` holds n_vertices * 3 floats (world mm), `tri` n_tri * 3 vertex indices.
struct Malha {
    const float*           xyz{nullptr};
    size_t                 n_vertices{0};
    const int32_t*         tri{nullptr};
    size_t                 n_tri{0};
    std::array<uint8_t, 3> cor{{255, 46, 138}};
};

struct Mesa {
    bool   tem{false};
    double xmin{0}, ymin{0}, xmax{0}, ymax{0};
};

struct Imagem {
    int                   largura{0}, altura{0};
    std::vector<uint8_t>  rgb;                 // largura * altura * 3
    double                escala_mm_por_px{0};
    std::array<double, 3> centro{};            // world point at the image centre
    std::array<double, 3> direita{}, cima{}, olhar{};
    std::array<double, 3> min{}, max{};        // world box of what was drawn
    bool                  vazia{true};         // nothing to draw (no triangles)
};

Imagem desenhar(const std::vector<Malha>& malhas, Vista vista, int tamanho, const Mesa& mesa);

// World point -> pixel (x right, y down) in `im`.
std::array<double, 2> projetar(const Imagem& im, const std::array<double, 3>& p);

// PPM (P6), for tests and debugging.
std::string para_ppm(const Imagem& im);

}} // namespace Slic3r::CadRaster

#endif // slic3r_CadRaster_hpp_
