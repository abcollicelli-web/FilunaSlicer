// Filuna — laboratório da resina: fatia um modelo com o motor SLA e grava o arquivo da impressora,
// sem abrir a janela do programa. Serve para conferir o .cxdlpv4 antes de levar para a impressora.
//
// uso: laboratorio <modelo.stl> <saida.cxdlpv4> [altura_camada_mm]

// O nanosvg é só cabeçalho: a implementação tem de sair deste arquivo, e antes de qualquer cabeçalho da
// libslic3r (vários incluem o nanosvg sem a macro, e aí a guarda de inclusão calaria a implementação).
#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvgrast.h"

#include "libslic3r/Model.hpp"
#include "libslic3r/SLAPrint.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Format/CXDLP.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/libslic3r.h"

#include <boost/nowide/iostream.hpp>

#include <cstdio>
#include <iostream>
#include <string>

using namespace Slic3r;

namespace {

// Halot R6: tela de 6,08" 2K, mesa de 130,56 x 82,62 x 160 mm. Os números batem com o manual da Creality;
// a resolução exata ainda vai ser confirmada na impressora do cliente.
void perfil_halot_r6(DynamicPrintConfig& c)
{
    c.set_key_value("printer_technology", new ConfigOptionEnum<PrinterTechnology>(ptSLA));
    c.set_key_value("printer_model", new ConfigOptionString("CL-R6"));
    c.set_key_value("printer_variant", new ConfigOptionString("default"));
    c.set_key_value("sla_archive_format", new ConfigOptionString("CXDLPV4"));

    c.set_key_value("display_width", new ConfigOptionFloat(130.56));
    c.set_key_value("display_height", new ConfigOptionFloat(82.62));
    c.set_key_value("display_pixels_x", new ConfigOptionInt(2560));
    c.set_key_value("display_pixels_y", new ConfigOptionInt(1620));
    c.set_key_value("display_orientation", new ConfigOptionEnum<SLADisplayOrientation>(sladoLandscape));
    c.set_key_value("display_mirror_x", new ConfigOptionBool(false));
    c.set_key_value("display_mirror_y", new ConfigOptionBool(false));
    c.set_key_value("printable_height", new ConfigOptionFloat(160.));

    auto* area = new ConfigOptionPoints();
    area->values = {Vec2d(0, 0), Vec2d(130.56, 0), Vec2d(130.56, 82.62), Vec2d(0, 82.62)};
    c.set_key_value("printable_area", area);

    c.set_key_value("sla_lift_height", new ConfigOptionFloat(6.));
    c.set_key_value("sla_lift_speed", new ConfigOptionFloat(80.));
    c.set_key_value("sla_bottom_lift_height", new ConfigOptionFloat(7.));
    c.set_key_value("sla_bottom_lift_speed", new ConfigOptionFloat(60.));
    c.set_key_value("sla_retract_speed", new ConfigOptionFloat(150.));

    // resina padrão cinza
    c.set_key_value("exposure_time", new ConfigOptionFloat(2.5));
    c.set_key_value("initial_exposure_time", new ConfigOptionFloat(30.));
    c.set_key_value("sla_bottom_layers", new ConfigOptionInt(4));
    c.set_key_value("faded_layers", new ConfigOptionInt(3));
    c.set_key_value("sla_light_pwm", new ConfigOptionInt(255));
    c.set_key_value("sla_bottom_light_pwm", new ConfigOptionInt(255));
    c.set_key_value("sla_light_off_delay", new ConfigOptionFloat(0.5));
    c.set_key_value("sla_bottom_light_off_delay", new ConfigOptionFloat(0.5));
    c.set_key_value("material_density", new ConfigOptionFloat(1.1));
    c.set_key_value("bottle_cost", new ConfigOptionFloat(179.));
    c.set_key_value("bottle_volume", new ConfigOptionFloat(1000.));

    // processo
    c.set_key_value("supports_enable", new ConfigOptionBool(true));
    c.set_key_value("pad_enable", new ConfigOptionBool(true));
    c.set_key_value("filename_format", new ConfigOptionString("[input_filename_base]"));
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 3) {
        boost::nowide::cerr << "uso: laboratorio <modelo.stl> <saida.cxdlpv4> [altura_camada_mm]\n";
        return 2;
    }
    const std::string entrada = argv[1], saida = argv[2];
    const double      altura  = argc > 3 ? std::atof(argv[3]) : 0.05;

    try {
        set_logging_level(1);

        DynamicPrintConfig cfg;
        cfg.apply(SLAFullPrintConfig::defaults());
        perfil_halot_r6(cfg);
        cfg.set_key_value("layer_height", new ConfigOptionFloat(altura));
        cfg.set_key_value("initial_layer_height", new ConfigOptionFloat(altura));
        cfg.normalize_fdm();

        boost::nowide::cout << "lendo " << entrada << "\n";
        Model model = Model::read_from_file(entrada, nullptr, nullptr, LoadStrategy::AddDefaultInstances);
        if (model.objects.empty()) {
            boost::nowide::cerr << "o arquivo não tem nenhum objeto\n";
            return 1;
        }

        // Põe a peça no meio da mesa e apoiada nela.
        for (ModelObject* o : model.objects) {
            o->center_around_origin(false);
            o->ensure_on_bed(false);
            for (ModelInstance* i : o->instances) i->set_offset(Vec3d(130.56 / 2., 82.62 / 2., i->get_offset().z()));
        }

        SLAPrint      print;
        CXDLPArchive  arquivo;
        print.set_printer(&arquivo);
        print.set_status_callback([](const PrintBase::SlicingStatus& s) {
            boost::nowide::cout << "  " << int(s.percent) << "% " << s.text << "\n";
        });

        print.apply(model, cfg);
        const std::string erro = print.validate().string;
        if (!erro.empty()) {
            boost::nowide::cerr << "o perfil não passou na conferência: " << erro << "\n";
            return 1;
        }

        print.process();

        const SLAPrintStatistics& e = print.print_statistics();
        boost::nowide::cout << "camadas: " << print.print_layers().size() << "\n"
                            << "resina: " << (e.objects_used_material + e.support_used_material) / 1000. << " ml\n"
                            << "peso: " << e.total_weight << " g\n"
                            << "custo: " << e.total_cost << "\n"
                            << "tempo: " << e.estimated_print_time << " s\n";

        arquivo.exportar(saida, print);
        boost::nowide::cout << "gravado: " << saida << "\n";
        return 0;
    } catch (const std::exception& ex) {
        boost::nowide::cerr << "falhou: " << ex.what() << "\n";
        return 1;
    }
}
