"""Negative and positive (diagnostic-only) cases for official Flashforge motion probe."""
import unittest
from flashforge_motion_parity_probe import first_motion_delta, movements, runtime_config


class FlashforgeMotionRegression(unittest.TestCase):
    def test_real_xy_divergence_fails(self):
        a = b";LAYER_CHANGE\nG1 X119.191 Y117.103 F30000\n"
        b = b";LAYER_CHANGE\nG1 X119.191 Y117.317 F30000\n"
        r = first_motion_delta(a, b)
        self.assertEqual(r["status"], "FAIL_REAL_GCODE_PARITY")
        self.assertEqual(r["native_parity"], "NOT_PROVEN")
        self.assertEqual(r["geometry_changed_blocks"], 1)
        self.assertEqual(r["first_differences"][0]["first_official_layer"], 1)

    def test_extrusion_only_change_never_passes(self):
        r = first_motion_delta(b"G1 X1 Y1 E.02839\n",
                               b"G1 X1 Y1 E.0284\n")
        self.assertTrue(r["geometry_equal"])
        self.assertFalse(r["movement_equal"])
        self.assertFalse(r["raw_equal"])
        self.assertEqual(r["matching_geometry_different_e"], 1)
        self.assertEqual(r["status"], "FAIL_REAL_GCODE_PARITY")

    def test_feedrate_only_change_never_passes(self):
        r = first_motion_delta(b"G1 X1 Y1 F7947\n",
                               b"G1 X1 Y1 F7970\n")
        self.assertEqual(r["matching_geometry_different_f"], 1)
        self.assertEqual(r["status"], "FAIL_REAL_GCODE_PARITY")

    def test_timestamp_comment_alone_not_native_parity(self):
        r = first_motion_delta(b"; generated 12:00\nG1 X1 Y1\n",
                               b"; generated 12:01\nG1 X1 Y1\n")
        self.assertTrue(r["geometry_equal"])
        self.assertFalse(r["raw_equal"])
        self.assertEqual(r["status"], "FAIL_REAL_GCODE_PARITY")

    def test_same_sample_not_homologated(self):
        r = first_motion_delta(b"G1 X1 Y1 E.5\n", b"G1 X1 Y1 E.5\n")
        self.assertEqual(r["status"], "DIAGNOSTIC_SINGLE_SAMPLE_IDENTICAL_NOT_HOMOLOGATED")
        self.assertEqual(r["native_parity"], "NOT_PROVEN")

    def test_first_layer_tracking(self):
        s = b";LAYER_CHANGE\nG1 X1 Y1\n;LAYER_CHANGE\nG1 X2 Y2\n"
        self.assertEqual([x["layer"] for x in movements(s)], [1, 2])

    def test_comment_only_or_empty_fail(self):
        r = first_motion_delta(b"G1 X1\n", b"")
        self.assertEqual(r["status"], "FAIL_EMPTY_GCODE")
        self.assertEqual(r["native_parity"], "NOT_PROVEN")

    def test_extra_move_never_passes(self):
        r = first_motion_delta(b"G1 X1 Y1\nG1 X2 Y2\n", b"G1 X1 Y1\n")
        self.assertFalse(r["geometry_equal"])
        self.assertGreater(r["geometry_changed_blocks"], 0)
        self.assertEqual(r["status"], "FAIL_REAL_GCODE_PARITY")


    def test_runtime_config_original_default_exposed_without_rewrite(self):
        official = (b"; CONFIG_BLOCK_START\n; wall_direction = auto\n"
                    b"; wipe_tower_type = type1\n; CONFIG_BLOCK_END\n"
                    b"G1 X1 Y1 E.5\n")
        filuna = (b"; CONFIG_BLOCK_START\n; wall_direction = ccw\n"
                  b"; wipe_tower_type = type2\n; CONFIG_BLOCK_END\n"
                  b"G1 X1 Y1 E.5\n")
        d = first_motion_delta(official, filuna)
        self.assertEqual(d["embedded_runtime_config_differences"]["wall_direction"],
                         {"official": "auto", "filuna": "ccw"})
        self.assertEqual(d["embedded_runtime_config_differences"]["wipe_tower_type"],
                         {"official": "type1", "filuna": "type2"})
        self.assertEqual(d["status"], "FAIL_REAL_GCODE_PARITY")
        self.assertEqual(d["native_parity"], "NOT_PROVEN")

    def test_runtime_config_outside_block_is_not_evidence(self):
        with self.assertRaisesRegex(ValueError, "ONE_COMPLETE_NONEMPTY_CONFIG_BLOCK_REQUIRED"):
            runtime_config(b"; wall_direction = auto\nG1 X1\n")
        self.assertEqual(runtime_config(
            b"; CONFIG_BLOCK_START\n; wall_direction = auto\n; CONFIG_BLOCK_END\n"),
            {"wall_direction": "auto"})


    def test_duplicate_oem_key_is_not_silently_overwritten(self):
        official = (b"; CONFIG_BLOCK_START\n; wipe_tower_x = 15.000\n"
                    b"; wipe_tower_x = 15\n; CONFIG_BLOCK_END\nG1 X1 Y1\n")
        filuna = (b"; CONFIG_BLOCK_START\n; wipe_tower_x = 15\n"
                  b"; CONFIG_BLOCK_END\nG1 X1 Y1\n")
        d = first_motion_delta(official, filuna)
        self.assertEqual(d["embedded_runtime_config_evidence"], "VALID")
        self.assertEqual(d["embedded_runtime_config_differences"]["wipe_tower_x"],
                         {"official": ["15.000", "15"], "filuna": "15"})
        self.assertEqual(d["status"], "FAIL_REAL_GCODE_PARITY")

    def test_malformed_embedded_config_is_explicitly_untrusted(self):
        official = b"; CONFIG_BLOCK_START\n; wall_direction = auto\nG1 X1 Y1\n"
        filuna = (b"; CONFIG_BLOCK_START\n; wall_direction = ccw\n"
                  b"; CONFIG_BLOCK_END\nG1 X1 Y1\n")
        d = first_motion_delta(official, filuna)
        self.assertTrue(d["embedded_runtime_config_evidence"].startswith(
            "INVALID_OR_MISSING_CONFIG_BLOCK:"))
        self.assertEqual(d["native_parity"], "NOT_PROVEN")

if __name__ == "__main__":
    unittest.main()
