// Filuna native hardware compatibility registry, evidence-only layer.
// AGPL-3.0-or-later. Identifies model-specific capabilities; never claims a
// simulated network response is physical firmware compatibility.
// Vendor implementations are to be ported through separately audited adapters.
#pragma once
#include <array>
#include <string>
#include <string_view>

namespace Slic3r::Filuna {
enum class Vendor { Bambu, Elegoo, Anycubic, Flashforge, Unknown };
enum class IntegrationEvidence { NotRun, PartialSimulated, FilunaIntegrationPass, HardwarePass };
struct VendorModel {
    Vendor vendor;
    std::string_view model_id;
    std::string_view display;
    std::string_view protocol;
    bool discovered_in_mock;
    bool pairing_verified_in_filuna;
    bool firmware_verified;
};
inline constexpr std::array<VendorModel, 4> kTargetModels = {{
    {Vendor::Bambu, "BBL-A1", "Bambu Lab A1", "bambu-ssdp/mqtt-tls/ftps", true, false, false},
    {Vendor::Elegoo, "Elegoo-CC2", "Elegoo Centauri Carbon 2", "elegoo-cc2-udp/mqtt", true, false, false},
    {Vendor::Anycubic, "ANYCUBIC-KOBRA3-V2", "Anycubic Kobra 3 V2", "anycubic-model-specific", false, false, false},
    {Vendor::Flashforge, "FLASHFORGE-AD5X", "Flashforge AD5X", "flashforge-lan-model-specific", false, false, false},
}};
inline const VendorModel* model_for_name(std::string_view model)
{
    for (const auto& candidate : kTargetModels) {
        if (candidate.display == model || candidate.model_id == model)
            return &candidate;
    }
    return nullptr;
}
inline IntegrationEvidence integration_evidence(const VendorModel& model)
{
    if (model.firmware_verified) return IntegrationEvidence::HardwarePass;
    if (model.pairing_verified_in_filuna) return IntegrationEvidence::FilunaIntegrationPass;
    if (model.discovered_in_mock) return IntegrationEvidence::PartialSimulated;
    return IntegrationEvidence::NotRun;
}
inline std::string_view evidence_label(IntegrationEvidence status)
{
    switch (status) {
    case IntegrationEvidence::HardwarePass: return "HARDWARE_PASS";
    case IntegrationEvidence::FilunaIntegrationPass: return "FILUNA_INTEGRATION_PASS";
    case IntegrationEvidence::PartialSimulated: return "PARTIAL_SIMULATED";
    default: return "NOT_RUN";
    }
}
} // namespace Slic3r::Filuna
