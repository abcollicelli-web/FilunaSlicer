// Filuna — laboratório da resina: confere se o catálogo do fabricante entrega as impressoras de resina.
// Carrega os perfis de um fabricante e lista o que entrou em cada coleção.
//
// uso: catalogo <pasta de resources> <Fabricante>

#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvgrast.h"

#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/libslic3r.h"

#include <boost/nowide/iostream.hpp>

#include <string>

using namespace Slic3r;

int main(int argc, char** argv)
{
    if (argc < 3) {
        boost::nowide::cerr << "uso: catalogo <pasta resources> <Fabricante>\n";
        return 2;
    }
    const std::string recursos = argv[1], fabricante = argv[2];

    try {
        set_logging_level(1);
        set_resources_dir(recursos);
        set_data_dir(recursos + "/../dados-teste-catalogo");

        PresetBundle bundle;
        const std::string pasta = recursos + "/profiles";
        auto r = bundle.load_vendor_configs_from_json(pasta, fabricante,
                                                      PresetBundle::LoadConfigBundleAttribute::LoadSystem,
                                                      ForwardCompatibilitySubstitutionRule::EnableSilent, nullptr,
                                                      /*allow_cache*/ false);
        boost::nowide::cout << "perfis carregados: " << r.second << "\n\n";

        auto lista = [](const char* titulo, const PresetCollection& c, bool so_resina) {
            boost::nowide::cout << titulo << ":\n";
            int n = 0;
            for (const Preset& p : c) {
                if (!p.is_system) continue;
                if (so_resina && p.printer_technology() != ptSLA) continue;
                boost::nowide::cout << "   " << p.name;
                if (const ConfigOption* o = p.config.option("sla_archive_format"))
                    boost::nowide::cout << "   [arquivo " << o->serialize() << "]";
                if (const ConfigOption* o = p.config.option("display_pixels_x"))
                    boost::nowide::cout << "   [tela " << o->serialize() << " x "
                                        << p.config.option("display_pixels_y")->serialize() << "]";
                if (const ConfigOption* o = p.config.option("exposure_time"))
                    boost::nowide::cout << "   [exposicao " << o->serialize() << " s]";
                if (const ConfigOption* o = p.config.option("layer_height"))
                    boost::nowide::cout << "   [camada " << o->serialize() << " mm]";
                boost::nowide::cout << "\n";
                ++n;
            }
            if (n == 0) boost::nowide::cout << "   (nenhum)\n";
            boost::nowide::cout << "\n";
        };

        lista("Impressoras de RESINA", bundle.printers, true);
        lista("Processos de resina", bundle.sla_prints, false);
        lista("Resinas", bundle.sla_materials, false);

        int fff = 0;
        for (const Preset& p : bundle.printers)
            if (p.is_system && p.printer_technology() == ptFFF) ++fff;
        boost::nowide::cout << "impressoras de filamento deste fabricante: " << fff << " (tem de continuar como antes)\n";
        return 0;
    } catch (const std::exception& ex) {
        boost::nowide::cerr << "falhou: " << ex.what() << "\n";
        return 1;
    }
}
