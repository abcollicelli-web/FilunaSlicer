"""Negative and positive (diagnostic-only) cases for official Flashforge motion probe."""
import unittest
from flashforge_motion_parity_probe import first_motion_delta, movements


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


if __name__ == "__main__":
    unittest.main()
