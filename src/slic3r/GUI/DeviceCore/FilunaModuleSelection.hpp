// Filuna native compatibility module selector, AGPL-3.0-or-later.
// A selector is not a module loader: it never executes third-party binaries,
// changes firmware, uploads a job, or marks a physical printer supported.
#pragma once
#include <array>
#include <string_view>
namespace Slic3r::Filuna {
enum class ModuleState { Missing, CandidateNotValidated, MockPartial, IntegrationValidated, HardwareValidated };
struct CompatModule {
    std::string_view module_id;
    std::string_view model_id;
    std::string_view protocol_id;
    unsigned abi;
    unsigned version_major, version_minor, version_patch;
    ModuleState state;
    bool fully_verified_firmware;
    // Unspecified firmware is NOT a wildcard for automatic command activation.
};
inline constexpr std::array<CompatModule, 4> kModuleCandidates = {{
    {"filuna-bambu-a1", "BBL-A1", "bambu-ssdp", 1, 0, 1, 0, ModuleState::MockPartial, false},
    {"filuna-elegoo-cc2", "Elegoo-CC2", "elegoo-cc2-mqtt", 1, 0, 1, 0, ModuleState::MockPartial, false},
    {"filuna-anycubic-kobra3v2", "ANYCUBIC-KOBRA3-V2", "anycubic-pending", 1, 0, 1, 0, ModuleState::CandidateNotValidated, false},
    {"filuna-flashforge-ad5x", "FLASHFORGE-AD5X", "flashforge-pending", 1, 0, 1, 0, ModuleState::CandidateNotValidated, false}
}};
inline const CompatModule* select_candidate(std::string_view model_id, unsigned app_abi)
{
    const CompatModule* candidate = nullptr;
    for (const auto& m : kModuleCandidates) {
        if (m.model_id != model_id || m.abi != app_abi)
            continue;
        if (!candidate || (m.version_major > candidate->version_major ||
            (m.version_major == candidate->version_major && m.version_minor > candidate->version_minor) ||
            (m.version_major == candidate->version_major && m.version_minor == candidate->version_minor &&
             m.version_patch > candidate->version_patch)))
            candidate = &m;
    }
    return candidate;
}
inline bool may_enable_hardware_operations(const CompatModule& m, bool serial_confirmed,
                                          bool firmware_confirmed, bool pairing_confirmed)
{
    return m.state == ModuleState::HardwareValidated && m.fully_verified_firmware &&
           serial_confirmed && firmware_confirmed && pairing_confirmed;
}
} // namespace Slic3r::Filuna
