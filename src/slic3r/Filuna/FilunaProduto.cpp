#include "FilunaProduto.hpp"

#include "FilunaCalculos.hpp"

#include "libslic3r/Model.hpp"
#include "libslic3r/TriangleMesh.hpp"

#include <vector>

namespace Slic3r::Filuna {

std::string assinatura_do_modelo(const Model& modelo)
{
    std::vector<std::string> objetos;
    for (const ModelObject* obj : modelo.objects) {
        if (obj == nullptr || obj->volumes.empty() || obj->instances.empty())
            continue;
        std::vector<VolumeMalha> volumes;
        for (const ModelVolume* vol : obj->volumes) {
            const indexed_triangle_set& its = vol->mesh().its;
            VolumeMalha                 v;
            v.vertices.reserve(its.vertices.size() * 3);
            for (const stl_vertex& p : its.vertices) {
                v.vertices.push_back(p.x());
                v.vertices.push_back(p.y());
                v.vertices.push_back(p.z());
            }
            v.indices.reserve(its.indices.size() * 3);
            for (const stl_triangle_vertex_indices& t : its.indices) {
                v.indices.push_back(t(0));
                v.indices.push_back(t(1));
                v.indices.push_back(t(2));
            }
            volumes.push_back(std::move(v));
        }
        const Vec3d escala = obj->instances.front()->get_scaling_factor();
        objetos.push_back(assinatura_objeto(volumes, escala.x(), escala.y(), escala.z()));
    }
    return objetos.empty() ? std::string() : assinatura_produto(std::move(objetos));
}

} // namespace Slic3r::Filuna
