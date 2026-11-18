// Filuna — laboratório: reproduz o que o programa faz ao ABRIR com uma impressora escolhida, sem janela.
//
// O Orca foi escrito pensando só em filamento; com uma impressora de resina escolhida, partes da abertura leem
// opções que só existem em impressora de filamento (bico, extrusor) e o programa cai. Este teste roda a mesma
// sequência da abertura (carregar o catálogo, carregar as escolhas, montar a configuração completa) e diz em que
// passo quebrou, em segundos, em vez de esperar 40 minutos de compilação no Windows.
//
// uso: abertura <pasta resources> <Fabricante> <impressora>

#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvgrast.h"

#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Format/CXDLP.hpp"
#include "libslic3r/GCode/WipeTowerEstimate.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/SLAPrint.hpp"
#include "libslic3r/TriangleMesh.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/libslic3r.h"

#include <boost/nowide/iostream.hpp>

#include <csignal>
#include <cstdlib>
#include <string>

using namespace Slic3r;

namespace {
const char* g_passo = "inicio";

void ao_cair(int sinal)
{
    boost::nowide::cout << "\n*** CAIU no passo: " << g_passo << " (sinal " << sinal << ")\n";
    boost::nowide::cout.flush();
    std::_Exit(3);
}

void passo(const char* p)
{
    g_passo = p;
    boost::nowide::cout << "  -> " << p << "\n";
    boost::nowide::cout.flush();
}
} // namespace

int main(int argc, char** argv)
{
    if (argc < 4) {
        boost::nowide::cerr << "uso: abertura <pasta resources> <Fabricante> <impressora>\n";
        return 2;
    }
    std::signal(SIGSEGV, ao_cair);
    std::signal(SIGABRT, ao_cair);

    const std::string recursos = argv[1], fabricante = argv[2], impressora = argv[3];
    try {
        set_logging_level(1);
        set_resources_dir(recursos);
        set_data_dir(recursos + "/../dados-teste-abertura");

        passo("carregar o catalogo");
        PresetBundle bundle;
        bundle.load_vendor_configs_from_json(recursos + "/profiles", fabricante, PresetBundle::LoadConfigBundleAttribute::LoadSystem,
                                             ForwardCompatibilitySubstitutionRule::EnableSystemSilent, nullptr, false);

        passo("preparar as escolhas (como o OrcaSlicer.conf)");
        AppConfig cfg;
        cfg.set("presets", PRESET_PRINTER_NAME, impressora);
        // A impressora tem de estar instalada (a lista de modelos do OrcaSlicer.conf), senão fica escondida.
        if (const Preset* pr = bundle.printers.find_preset(impressora)) {
            const std::string modelo   = pr->config.opt_string("printer_model");
            const std::string variante = pr->config.opt_string("printer_variant");
            cfg.set_variant(fabricante, modelo, variante.empty() ? "default" : variante, true);
            boost::nowide::cout << "     instalada: " << fabricante << " / " << modelo << " / " << variante << "\n";
        } else {
            boost::nowide::cout << "     (a impressora " << impressora << " nao existe no catalogo)\n";
        }

        passo("load_selections (o que a abertura faz)");
        bundle.load_selections(cfg);

        const Preset& p = bundle.printers.get_selected_preset();
        boost::nowide::cout << "     impressora escolhida: " << p.name
                            << (p.printer_technology() == ptSLA ? "  [RESINA]" : "  [filamento]") << "\n";
        if (p.printer_technology() == ptSLA)
            boost::nowide::cout << "     processo de resina: " << bundle.sla_prints.get_selected_preset_name() << "\n"
                                << "     resina: " << bundle.sla_materials.get_selected_preset_name() << "\n";

        passo("campos de impressora de filamento (a janela le varios direto da impressora)");
        {
            const DynamicPrintConfig& ed = bundle.printers.get_edited_preset().config;
            int faltam = 0;
            for (const std::string& k : Preset::printer_options())
                if (!ed.has(k)) {
                    if (++faltam <= 8)
                        boost::nowide::cout << "     falta: " << k << "\n";
                }
            boost::nowide::cout << "     campos de impressora de filamento que faltam: " << faltam << "\n";
            for (const char* k : {"use_3mf", "pellet_modded_printer", "support_multi_bed_types", "host_type", "nozzle_diameter"})
                boost::nowide::cout << "     " << k << (ed.has(k) ? ": tem" : ": FALTA") << "\n";
        }

        passo("full_config (a configuracao que o fatiamento usa)");
        DynamicPrintConfig full = bundle.full_config();
        boost::nowide::cout << "     opcoes na configuracao completa: " << full.keys().size() << "\n";
        for (const char* k : {"flush_volumes_matrix", "filament_colour", "wipe_tower_x", "enable_prime_tower", "nozzle_diameter"})
            boost::nowide::cout << "     completa " << k << (full.has(k) ? ": tem" : ": FALTA") << "\n";
        if (const auto* torre = full.option<ConfigOptionBool>("enable_prime_tower"))
            boost::nowide::cout << "     torre de limpeza: " << (torre->value ? "LIGADA" : "desligada") << "\n";

        passo("estimar a torre de limpeza (o que a janela faz ao montar a mesa)");
        {
            const WipeTowerFootprint f = estimate_wipe_tower_footprint(full, WipeTowerType::Type2, {0u, 1u}, 0.2, 10.);
            boost::nowide::cout << "     profundidade estimada: " << f.depth << "\n";
        }

        if (p.printer_technology() == ptSLA) {
            passo("fatiar um cubo com a configuracao completa dos perfis");
            Model        model;
            ModelObject* o = model.add_object();
            o->add_volume(TriangleMesh(its_make_cube(10., 10., 10.)));
            o->add_instance();
            o->center_around_origin(false);
            o->ensure_on_bed(false);
            const BoundingBoxf mesa(full.opt<ConfigOptionPoints>("printable_area")->values);
            for (ModelInstance* i : o->instances)
                i->set_offset(Vec3d(mesa.center().x(), mesa.center().y(), i->get_offset().z()));
            SLAPrint     print;
            CXDLPArchive arquivo;
            print.set_printer(&arquivo);
            print.apply(model, full);
            const std::string erro = print.validate().string;
            if (!erro.empty())
                boost::nowide::cout << "     conferencia: " << erro << "\n";
            print.process();
            boost::nowide::cout << "     camadas: " << print.print_layers().size() << "\n";
            const std::string saida = recursos + "/../dados-teste-abertura/cubo.cxdlpv4";
            arquivo.exportar(saida, print);
            boost::nowide::cout << "     gravado: " << saida << "\n";
        }

        passo("export_selections (gravar as escolhas, como ao fechar)");
        bundle.export_selections(cfg);

        passo("fim");
        boost::nowide::cout << "\nABRIU SEM CAIR.\n";
        return 0;
    } catch (const std::exception& ex) {
        boost::nowide::cout << "\n*** ERRO no passo " << g_passo << ": " << ex.what() << "\n";
        return 1;
    }
}
