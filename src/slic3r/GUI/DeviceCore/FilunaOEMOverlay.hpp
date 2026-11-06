// Filuna — shared OEM overlay selection contract (NOT a dynamic loader).
// A single Orca engine is retained. Device and Prepare must resolve to the
// same versioned manufacturer delta, never standalone official slicer binaries.
// AGPL-3.0-or-later; no proprietary OEM networking plugin.
#pragma once
#include "FilunaModuleSelection.hpp"
#include <array>
#include <string_view>
namespace Slic3r::Filuna {

struct OEMOverlayBinding {
    std::string_view model_id;
    std::string_view manufacturer;
    std::string_view manifest_relative_path;
    std::string_view module_id;
    unsigned abi;
};

inline constexpr std::array<OEMOverlayBinding,4> kOEMBindings = {{
    {"BBL-A1", "bambu", "FILUNA/fabricantes/bambu/manifest.json", "filuna-bambu-a1", 1},
    {"Elegoo-CC2", "elegoo", "FILUNA/fabricantes/elegoo/manifest.json", "filuna-elegoo-cc2", 1},
    {"ANYCUBIC-KOBRA3-V2", "anycubic", "FILUNA/fabricantes/anycubic/manifest.json", "filuna-anycubic-kobra3v2", 1},
    {"FLASHFORGE-AD5X", "flashforge", "FILUNA/fabricantes/flashforge/manifest.json", "filuna-flashforge-ad5x", 1}
}};

// Use this from both the Device registration path and the Prepare selection
// path. Unknown models and incompatible ABI must not load an arbitrary module.
inline const OEMOverlayBinding* resolve_oem_overlay(std::string_view model_id, unsigned app_abi)
{
    const auto* selected=select_candidate(model_id,app_abi);
    if(!selected) return nullptr;
    for(const auto& binding:kOEMBindings)
        if(binding.model_id==model_id && binding.abi==app_abi &&
           binding.module_id==selected->module_id)
            return &binding;
    return nullptr;
}

enum class OEMStage { Device, Prepare };
inline const OEMOverlayBinding* resolve_oem_for_stage(
    OEMStage /*stage*/, std::string_view model_id, unsigned app_abi)
{
    // Deliberately identical resolution regardless of entry point.
    return resolve_oem_overlay(model_id,app_abi);
}
} // namespace Slic3r::Filuna
