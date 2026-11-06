// FILUNA: Bambu A1 process-config bridge, within the one Orca engine.
// This is a first OEM config integration, NOT the whole Bambu planner.
// Do not run for other manufacturers or unrecognized device/printer mismatch.
#pragma once
#include "libslic3r/PrintConfig.hpp"
namespace Slic3r::Filuna::Bambu {

// Apply only documented OEM-vs-Orca DEFAULT differences. Every change is
// scoped by the caller to a registered Bambu A1 with matching printer preset.
// User overrides different from the Orca fallback must not be overwritten.
// Return the number of explicitly translated options for QA telemetry.
inline unsigned apply_a1_slicing_defaults(DynamicPrintConfig& cfg)
{
    unsigned changed = 0;
    if (const auto* x = cfg.option<ConfigOptionBool>("precise_outer_wall");
        x && x->value) {
        cfg.set_key_value("precise_outer_wall", new ConfigOptionBool(false));
        ++changed;
    }
    if (const auto* x = cfg.option<ConfigOptionBool>("accel_to_decel_enable");
        x && x->value) {
        cfg.set_key_value("accel_to_decel_enable", new ConfigOptionBool(false));
        ++changed;
    }
    if (const auto* x = cfg.option<ConfigOptionEnum<InfillPattern>>("internal_solid_infill_pattern");
        x && x->value == ipMonotonic) {
        // ipZigZag previously existed in Orca, but PrintConfig rejected it
        // for internal solid fill. The manufacturer delta enables that key.
        cfg.set_key_value("internal_solid_infill_pattern",
                          new ConfigOptionEnum<InfillPattern>(ipZigZag));
        ++changed;
    }
    if (const auto* x = cfg.option<ConfigOptionFloatsNullable>("travel_jerk");
        x && !x->values.empty() && x->values.front() == 12.0) {
        cfg.set_key_value("travel_jerk", new ConfigOptionFloatsNullable{9.0});
        ++changed;
    }
    return changed;
}

} // namespace Slic3r::Filuna::Bambu
