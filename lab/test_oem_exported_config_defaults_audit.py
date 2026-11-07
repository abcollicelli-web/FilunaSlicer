"""Never confuse OEM's exported full defaults with a partial preset JSON."""
import unittest
from oem_exported_config_defaults_audit import compare_gcode_config, read_config


def gcode(keys: dict) -> str:
    return ("; CONFIG_BLOCK_START\n" +
            "".join(f"; {k} = {v}\n" for k, v in keys.items()) +
            "; CONFIG_BLOCK_END\nG1 X1 E0.1\n")


class ExportedConfigDefaultsAuditTests(unittest.TestCase):
    def test_flashforge_wall_auto_is_not_ccw(self):
        data = compare_gcode_config(gcode({"wall_direction": "auto"}),
                                    gcode({"wall_direction": "ccw"}))
        self.assertEqual(data["status"], "BLOCKED_EFFECTIVE_DEFAULTS_CONFLICT")
        self.assertEqual(data["conflicting_shared_values"]["wall_direction"],
                         {"oem": "auto", "filuna": "ccw"})
        self.assertEqual(data["native_parity"], "NOT_PROVEN")

    def test_oem_hidden_defaults_must_be_reported(self):
        data = compare_gcode_config(
            gcode({"filament_change_length": "0", "wipe_tower_type": "type1"}),
            gcode({"filament_change_length": "10", "wipe_tower_type": "type2"}))
        self.assertEqual(len(data["conflicting_shared_values"]), 2)

    def test_unknown_vendor_settings_are_not_silent(self):
        data = compare_gcode_config(
            gcode({"wall_filament": "1", "shared": "1"}),
            gcode({"shared": "1", "other": "1"}))
        self.assertEqual(data["oem_keys_missing_from_filuna"], ["wall_filament"])
        self.assertEqual(data["filuna_only_keys"], ["other"])
        self.assertEqual(data["status"], "BLOCKED_EFFECTIVE_DEFAULTS_CONFLICT")

    def test_identical_config_not_a_parity_pass(self):
        x = gcode({"wall_direction": "ccw"})
        data = compare_gcode_config(x, x)
        self.assertEqual(data["status"],
                         "DIAGNOSTIC_FULL_CONFIG_IDENTICAL_NOT_HOMOLOGATED")
        self.assertEqual(data["native_parity"], "NOT_PROVEN")

    def test_missing_config_block_fails_closed(self):
        data = compare_gcode_config("G1 X1", gcode({"wall_direction": "ccw"}))
        self.assertEqual(data["status"], "BLOCKED_INVALID_OR_MISSING_CONFIG_BLOCK")

    def test_duplicate_config_values_preserved_not_discarded(self):
        config = read_config("; CONFIG_BLOCK_START\n; wipe_tower_x = 15.000\n; wipe_tower_x = 15\n; CONFIG_BLOCK_END")
        self.assertEqual(config["wipe_tower_x"], ["15.000", "15"])
        result = compare_gcode_config(
            gcode({"wipe_tower_x": "15"}),
            "; CONFIG_BLOCK_START\n; wipe_tower_x = 15.000\n; wipe_tower_x = 15\n; CONFIG_BLOCK_END\n")
        self.assertEqual(result["status"], "BLOCKED_EFFECTIVE_DEFAULTS_CONFLICT")
        self.assertEqual(result["conflicting_shared_values"]["wipe_tower_x"],
                         {"oem": "15", "filuna": ["15.000", "15"]})

    def test_incomplete_config_rejected(self):
        with self.assertRaisesRegex(ValueError, "ONE_COMPLETE"):
            read_config("; CONFIG_BLOCK_START\n; foo = 1\n")


if __name__ == "__main__":
    unittest.main()
