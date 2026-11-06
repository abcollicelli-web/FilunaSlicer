// Filuna Bambu A1 module — shared Orca core with isolated Bambu-only delta.
// AGPL-3.0-or-later; provenance FILUNA/qa/gcode/bambu_a1_source_manifest.json.
// This is an in-process OEM module with active identity and speed-mapping code.
// It is NOT full Bambu Studio GCode or a redistributable network plugin.
// Do not mark slicing equivalence or LAN print support until corresponding gates pass.
#pragma once
#include "../transport/BambuLanDiscovery.hpp"
#include "../../../modules/bambu-a1/BambuSpeedMapping.hpp"
#include <cmath>
#include <string_view>
namespace Slic3r::Filuna::Bambu {
enum class ActivationState { WrongDevice, MathCandidateOnly, ReadyForValidatedGCode };
struct A1Module {
    static constexpr std::string_view id = "filuna-bambu-a1";
    static constexpr std::string_view protocol_id = "bambu-ssdp/mqtt-tls/ftps";
    static constexpr unsigned abi = 1;
    static bool accepts(std::string_view raw_model) {
        return raw_model == "N2S" || raw_model == "BBL-A1" ||
               raw_model == "Bambu Lab A1";
    }
    static ActivationState activate_for_prepare(std::string_view printer_model) {
        // Until GCode planner and parser are validated, speed math is
        // callable but MUST NOT silently alter paths emitted by Orca.
        return accepts(printer_model) ? ActivationState::MathCandidateOnly :
                                        ActivationState::WrongDevice;
    }
    static double bbs_mapping_speed(double d, double coefficient) {
        return BambuA1::mapping_speed(d, coefficient);
    }
    static bool identified_by_ssdp(std::string_view packet) {
        return parse_ssdp(packet).valid_a1;
    }
};
} // namespace Slic3r::Filuna::Bambu
