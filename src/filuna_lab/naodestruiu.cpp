// Filuna — confere que a resina não atrapalhou o filamento.
//
// A resina mexeu em codigo compartilhado: um preset padrao a mais na colecao de impressoras, nomes das colecoes
// de resina, duas listas novas no catalogo do fabricante e a versao do cache. Este teste carrega um fabricante
// de filamento e exercita o caminho normal: contar presets, escolher uma impressora, montar a configuracao
// completa e conferir que ela continua sendo de filamento.
//
// uso: naodestruiu <pasta de resources> <Fabricante> <impressora de filamento>

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

namespace {
int falhas = 0;

void confere(const char* o_que, bool ok, const std::string& detalhe = {})
{
    boost::nowide::cout << (ok ? "  ok   " : "  FALHA") << "  " << o_que;
    if (!detalhe.empty()) boost::nowide::cout << "   (" << detalhe << ")";
    boost::nowide::cout << "\n";
    if (!ok) ++falhas;
}
} // namespace

int main(int argc, char** argv)
{
    if (argc < 4) {
        boost::nowide::cerr << "uso: naodestruiu <pasta resources> <Fabricante> <impressora>\n";
        return 2;
    }
    const std::string recursos = argv[1], fabricante = argv[2], impressora = argv[3];

    try {
        set_logging_level(1);
        set_resources_dir(recursos);
        set_data_dir(recursos + "/../dados-teste-naodestruiu");

        PresetBundle bundle;
        auto r = bundle.load_vendor_configs_from_json(recursos + "/profiles", fabricante,
                                                      PresetBundle::LoadConfigBundleAttribute::LoadSystem,
                                                      ForwardCompatibilitySubstitutionRule::EnableSystemSilent, nullptr, false);
        boost::nowide::cout << "fabricante " << fabricante << ": " << r.second << " perfis\n\n";

        // 1) O padrao de filamento continua sendo o primeiro, e o de resina e o segundo.
        const Preset& padrao_fff = bundle.printers.default_preset(0);
        const Preset& padrao_sla = bundle.printers.default_preset(1);
        confere("o padrao 0 das impressoras continua de filamento", padrao_fff.printer_technology() == ptFFF,
                padrao_fff.name);
        confere("o padrao 1 das impressoras e o de resina", padrao_sla.printer_technology() == ptSLA, padrao_sla.name);

        // 2) Escolher uma impressora de filamento continua funcionando.
        bundle.printers.select_preset_by_name(impressora, true);
        const Preset& sel = bundle.printers.get_selected_preset();
        confere("escolher a impressora de filamento", sel.name == impressora, sel.name);
        confere("ela continua sendo de filamento", sel.printer_technology() == ptFFF);

        // 3) A configuracao completa sai inteira e com valores de filamento.
        const DynamicPrintConfig cfg = bundle.full_config();
        const ConfigOption* bico = cfg.option("nozzle_diameter");
        confere("a configuracao completa tem o bico", bico != nullptr, bico ? bico->serialize() : "sem bico");
        const ConfigOption* tec = cfg.option("printer_technology");
        confere("a configuracao completa diz filamento", tec != nullptr && tec->serialize() == "FFF",
                tec ? tec->serialize() : "sem tecnologia");

        // 4) As colecoes de filamento continuam cheias; as de resina so tem o que o fabricante trouxer.
        size_t imp_fff = 0, imp_sla = 0;
        for (const Preset& p : bundle.printers)
            if (p.is_system) (p.printer_technology() == ptSLA ? imp_sla : imp_fff)++;
        confere("as impressoras de filamento continuam no lugar", imp_fff > 0, std::to_string(imp_fff));
        size_t proc = 0, fil = 0;
        for (const Preset& p : bundle.prints) if (p.is_system) ++proc;
        for (const Preset& p : bundle.filaments) if (p.is_system) ++fil;
        confere("os processos de filamento continuam no lugar", proc > 0, std::to_string(proc));
        confere("os filamentos continuam no lugar", fil > 0, std::to_string(fil));
        boost::nowide::cout << "       (resina neste fabricante: " << imp_sla << " impressora(s))\n";

        // 5) Os nomes das pastas das colecoes voltaram a valer (antes diziam "invalid").
        confere("a pasta do processo de resina tem nome", bundle.sla_prints.section_name() == "sla_print",
                bundle.sla_prints.section_name());
        confere("a pasta das resinas tem nome", bundle.sla_materials.section_name() == "sla_materials",
                bundle.sla_materials.section_name());
        confere("a pasta do processo de filamento nao mudou", bundle.prints.section_name() == PRESET_PRINT_NAME,
                bundle.prints.section_name());
        confere("a pasta dos filamentos nao mudou", bundle.filaments.section_name() == PRESET_FILAMENT_NAME,
                bundle.filaments.section_name());

        boost::nowide::cout << "\n" << (falhas == 0 ? "TUDO CERTO: o filamento continua como antes."
                                                    : "ATENCAO: " + std::to_string(falhas) + " falha(s)")
                            << "\n";
        return falhas == 0 ? 0 : 1;
    } catch (const std::exception& ex) {
        boost::nowide::cerr << "falhou: " << ex.what() << "\n";
        return 1;
    }
}
