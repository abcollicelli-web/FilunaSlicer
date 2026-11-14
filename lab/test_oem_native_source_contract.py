"""Source-only regression checks for targeted OEM compatibility patches.

No executable is built or run by these tests. Passing these tests is NOT
G-code parity, safety approval, network integration or OEM homologation.
"""
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
CONFIG_CPP = (ROOT / "src/libslic3r/PrintConfig.cpp").read_text(encoding="utf-8")
CONFIG_HPP = (ROOT / "src/libslic3r/PrintConfig.hpp").read_text(encoding="utf-8")
PRINT_CPP = (ROOT / "src/libslic3r/Print.cpp").read_text(encoding="utf-8")
GCODE_CPP = (ROOT / "src/libslic3r/GCode.cpp").read_text(encoding="utf-8")


def cooling_mode_from_official_rule(filaments):
    """Test-vector model of the pinned Snapmaker implementation (not C++ execution)."""
    mode = 0
    for high_temp, vitrification in filaments:
        if high_temp:
            continue
        if vitrification <= 50:
            return 2
        if vitrification <= 70:
            mode = 1
    return mode


class OEMSourceGuardTests(unittest.TestCase):
    def test_01_anycubic_unmodified_official_zero_disabled(self):
        # Source-only fixture: explicitly disabled long cut retraction with 0 mm.
        self.assertIn('def = this->add("retraction_distances_when_cut",coFloats);',
                      CONFIG_CPP)
        self.assertRegex(CONFIG_CPP, re.compile(
            r'add\("retraction_distances_when_cut",coFloats\);.{0,500}def->min = 0;',
            re.S))
        self.assertIn('def->set_default_value(new ConfigOptionFloats {18});', CONFIG_CPP)

    def test_02_active_cut_guard_preserves_original_10_18(self):
        self.assertIn('if (!m_config.long_retractions_when_cut.get_at(extruder))',
                      PRINT_CPP)
        self.assertIn('if (!std::isfinite(distance) || distance < 10.0 || distance > 18.0)', PRINT_CPP)
        self.assertIn('return { L("Enabled filament cut retraction requires a finite distance',
                      PRINT_CPP)

    def test_03_nullable_retraction_override_fills_correct_vector(self):
        needle='if (opt_key == "retraction_distances_when_cut"'
        seg=CONFIG_CPP.split(needle,1)[1][:500]
        self.assertIn('opt_retraction_distance_default.values.push_back(ptr->nil_value());',
                      seg)
        self.assertIn('opt_new_filament = &opt_retraction_distance_default;',seg)
        self.assertNotIn('opt_long_retraction_default.values.push_back',seg)

    def test_04_snapmaker_field_in_native_schema_and_default(self):
        self.assertIn('((ConfigOptionBools,              filament_is_high_temperature))',
                      CONFIG_HPP)
        self.assertIn('def = this->add("filament_is_high_temperature", coBools);',
                      CONFIG_CPP)
        self.assertIn('def->set_default_value(new ConfigOptionBools{false});',
                      CONFIG_CPP)

    def test_05_snapmaker_mode_gated_by_start_macro(self):
        self.assertIn('machine_start_gcode.value.find("chamber_cooling_mode")',
                      GCODE_CPP)
        self.assertIn('get_filament_config_index(extruder)', GCODE_CPP)
        self.assertIn('m_config.filament_is_high_temperature.get_at(fi)',GCODE_CPP)
        self.assertIn('m_config.temperature_vitrification.get_at(fi)',GCODE_CPP)
        self.assertIn('set("chamber_cooling_mode", new ConfigOptionInt(chamber_cooling_mode))',
                      GCODE_CPP)
        self.assertLess(
            GCODE_CPP.index('set("chamber_cooling_mode"'),
            GCODE_CPP.index('std::string machine_start_gcode = this->placeholder_parser_process'))

    def test_06_original_snapmaker_fixture_selects_strong_cooling(self):
        fil=json.loads((ROOT / "fixtures/oem/snapmaker/official-profiles/filament.json").read_text(encoding="utf-8"))
        machine=json.loads((ROOT / "fixtures/oem/snapmaker/official-profiles/machine.json").read_text(encoding="utf-8"))
        self.assertIn("chamber_cooling_mode", machine["machine_start_gcode"])
        case=[(bool(int(fil["filament_is_high_temperature"][0])),
               int(fil["temperature_vitrification"][0]))]
        self.assertEqual(case,[(False,45)])
        self.assertEqual(cooling_mode_from_official_rule(case),2)

    def test_07_chamber_mode_boundary_and_mixed_filaments(self):
        self.assertEqual(cooling_mode_from_official_rule([(True,45)]),0)
        self.assertEqual(cooling_mode_from_official_rule([(False,80)]),0)
        self.assertEqual(cooling_mode_from_official_rule([(False,70)]),1)
        self.assertEqual(cooling_mode_from_official_rule([(False,50)]),2)
        self.assertEqual(cooling_mode_from_official_rule([(True,42),(False,65)]),1)
        self.assertEqual(cooling_mode_from_official_rule([(False,60),(False,49)]),2)

    def test_08_no_postprocessor_fake_parity(self):
        # Source-only check: actual OEM output may not be rewritten to match.
        g=(ROOT/"lab/oem_functional_gcode_diff.py").read_text(encoding="utf-8")
        self.assertIn('"filuna_parity": "NOT_PROVEN"', g)
        self.assertIn('FAIL_GCODE_COMMAND_STREAM_DIFFERENT',g)
        self.assertIn('Comment-stripped hashes are diagnostic ONLY',g)

    def test_09_no_vendor_binary_or_model_whitelist_added(self):
        for src in (CONFIG_CPP,PRINT_CPP,GCODE_CPP):
            self.assertNotIn('filuna_oem_vendor_binary_fallback',src)
        self.assertNotIn('Anycubic Kobra 3 V2',PRINT_CPP)
        self.assertNotIn('Snapmaker U1',GCODE_CPP)


    def test_10_flashforge_auto_schema_is_opt_in(self):
        perimeter = (ROOT / "src/libslic3r/PerimeterGenerator.cpp").read_text(encoding="utf-8")
        self.assertRegex(CONFIG_HPP, r"enum class WallDirection\s*\{\s*CounterClockwise,\s*Clockwise,[\s\S]*?Auto,\s*Count,", msg="enum keeps existing Orca CCW/CW indexes")
        self.assertIn('{ "auto",  int(WallDirection::Auto)}', CONFIG_CPP)
        self.assertIn('def->enum_values.push_back("auto");', CONFIG_CPP)
        self.assertIn('new ConfigOptionEnum<WallDirection>(WallDirection::CounterClockwise)', CONFIG_CPP)
        self.assertIn('perimeter_generator.config->wall_direction == WallDirection::Auto', perimeter)

    def test_11_flashforge_auto_mode_gates_original_reversal(self):
        perimeter = (ROOT / "src/libslic3r/PerimeterGenerator.cpp").read_text(encoding="utf-8")
        self.assertEqual(perimeter.count('const bool auto_oem_wall_direction = config->wall_direction == WallDirection::Auto;'), 2)
        self.assertEqual(perimeter.count('if (!config->overhang_reverse && !auto_oem_wall_direction) {'), 2)
        self.assertEqual(perimeter.count('if (auto_oem_wall_direction || config->overhang_reverse) {'), 2)
        self.assertIn('perimeter_generator.config->wall_direction == WallDirection::Auto ||', perimeter)
        self.assertNotIn('manufacturer_id == "flashforge"', perimeter)
        self.assertNotIn('Flashforge AD5X', perimeter)

    def test_12_missing_native_flashforge_cannot_promote(self):
        manifest = json.loads((ROOT / "FILUNA/fabricantes/flashforge/manifest.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["qa"]["filuna_native_slicing"], "NOT_RUN")
        self.assertNotEqual(manifest["qa"]["internal_homologation"], "PASS")
        self.assertFalse(manifest["automatic_hardware_activation"])


    def test_13_flashforge_auto_survives_legacy_profile_parser(self):
        # Regression: the newly supported OEM Auto value must not be
        # silently transformed back into CCW by Orca's legacy import.
        self.assertIn('{ "auto",  int(WallDirection::Auto)}', CONFIG_CPP)
        self.assertNotIn('opt_key == "wall_direction" && value == "auto"', CONFIG_CPP)
        self.assertIn('Do not silently migrate it to ccw', CONFIG_CPP)
        self.assertIn('new ConfigOptionEnum<WallDirection>(WallDirection::CounterClockwise)', CONFIG_CPP)

    def test_14_snapmaker_gcode_is_not_rewritten_or_blindly_blocked(self):
        # Owner's binding rule: generated G-code is immutable and safety is
        # delegated to the independent spatial validator.
        self.assertNotIn('unverified_oem_chamber_macro', GCODE_CPP)
        self.assertNotIn('candidate_safe_final_purge_travel', GCODE_CPP)
        self.assertNotIn('Final purge Z-hop safety not yet validated', GCODE_CPP)
        self.assertIn('gcode += gcodegen.writer().unlift();', GCODE_CPP)
        self.assertIn('|| tool_change_on_wipe_tower);', GCODE_CPP)
        self.assertNotIn('filuna_oem_final_purge_lift', CONFIG_HPP)
        self.assertNotIn('filuna_oem_final_purge_lift', CONFIG_CPP)

    def test_15_snapmaker_vendor_still_blocked_even_with_source_guard(self):
        manifest = json.loads((ROOT / "FILUNA/fabricantes/snapmaker/manifest.json").read_text(encoding="utf-8"))
        self.assertNotEqual(manifest["qa"]["internal_homologation"], "PASS")
        self.assertEqual(manifest["capabilities"]["native_slicing"], "NOT_RUN")
        self.assertEqual(manifest["qa"]["u1_filuna_native_gcode_parity"], "NOT_RUN")


    def test_16_flashforge_legacy_infill_source_port_isolated(self):
        fill = (ROOT / "src/libslic3r/Fill/FillRectilinear.cpp").read_text(encoding="utf-8")
        self.assertIn('((ConfigOptionBool,                filuna_oem_legacy_infill_anchor))', CONFIG_HPP)
        # Critical C++ type check: Fill::print_config is PrintConfig*, not
        # PrintRegionConfig*. A member registered in the wrong macro compiles
        # neither the engine nor the FillRectilinear port.
        self.assertEqual(
            CONFIG_HPP.split('PRINT_CONFIG_CLASS_DERIVED_DEFINE(\n    PrintConfig,',1)[1]
            .split('// This object is mapped to Perl as Slic3r::Config::Full.',1)[0]
            .count('((ConfigOptionBool,                filuna_oem_legacy_infill_anchor))'),1)
        self.assertNotIn(
            'filuna_oem_legacy_infill_anchor',
            CONFIG_HPP.split('PRINT_CONFIG_CLASS_DEFINE(\n    PrintRegionConfig,',1)[1]
            .split('PRINT_CONFIG_CLASS_DEFINE(\n    MachineEnvelopeConfig,',1)[0])
        self.assertIn('def = this->add("filuna_oem_legacy_infill_anchor", coBool);', CONFIG_CPP)
        self.assertRegex(CONFIG_CPP, re.compile(
            r'add\("filuna_oem_legacy_infill_anchor", coBool\);.{0,550}new ConfigOptionBool\(false\)',
            re.S))
        self.assertIn('this->print_config->filuna_oem_legacy_infill_anchor.value', fill)
        self.assertIn('if (!use_legacy_oem_anchor)', fill)
        self.assertIn('bounding_box = this->extended_object_bounding_box();', fill)
        self.assertIn('bounding_box.translate(c.rotated(- rotate_vector.first) - c);', fill)
        self.assertNotIn('Flashforge AD5X', fill)
        self.assertNotIn('manufacturer_id == "flashforge"', fill)

    def test_17_flashforge_module_stays_unhomologated_without_real_compilation(self):
        manifest = json.loads((ROOT / "FILUNA/fabricantes/flashforge/manifest.json").read_text(encoding="utf-8"))
        self.assertNotEqual(manifest["qa"]["internal_homologation"], "PASS")
        self.assertNotEqual(manifest["qa"]["filuna_native_slicing"], "PASS")
        self.assertFalse(manifest["automatic_hardware_activation"])


    def test_18_snapmaker_validator_vendor_proof_and_no_mutation(self):
        validator=(ROOT/"lab/snapmaker_spatial_gcode_validator.py").read_text(encoding="utf-8")
        d=json.loads((ROOT/"FILUNA/fabricantes/snapmaker/vendor.yaml").read_text(encoding="utf-8"))
        self.assertEqual(d["gcode_policy"],"READ_ONLY_SPATIAL_VALIDATION_NEVER_POSTPROCESS")
        self.assertIn("def paired_audit(", validator)
        self.assertIn("def acknowledge_risk(", validator)
        self.assertIn("def arc_points(", validator)
        self.assertIn("GCODE_CHANGED_DURING_VALIDATION", validator)
        self.assertNotIn("all_travels", [x["scope"] for x in d["safe_macros"]])



if __name__ == "__main__":
    unittest.main()
