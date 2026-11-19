"""Real-motion triage distinguishes functional deltas without certifying an OEM."""
import unittest
from flashforge_real_motion_triage import triage


class FlashforgeMotionTriageTests(unittest.TestCase):
    def test_extrusion_not_rounded_away(self):
        a = b";LAYER_CHANGE\nG1 X101.185 Y101.185 E.02839\n"
        b = b";LAYER_CHANGE\nG1 X101.185 Y101.185 E.0284\n"
        r = triage(a, b)
        self.assertEqual(r["status"], "FAIL_FUNCTIONAL_OR_METADATA_MISMATCH")
        self.assertEqual(r["homologation"], "NOT_PROVEN")
        self.assertEqual(r["total_differences"]["extrusion_different"], 1)
        self.assertNotIn("geometry_edit_groups", r["total_differences"])

    def test_speed_change_detected(self):
        r = triage(b"G1 X2 Y3 F7947\n", b"G1 X2 Y3 F7970\n")
        self.assertEqual(r["total_differences"]["feedrate_different"], 1)

    def test_added_geometry_not_counted_as_rounding(self):
        r = triage(b"G1 X1\nG1 X2\nG1 X3\n",
                   b"G1 X1\nG1 X3\n")
        self.assertEqual(r["motion_counts"], {"official": 3, "filuna": 2})
        self.assertEqual(r["total_differences"]["official_unaligned_moves"], 1)
        self.assertEqual(r["total_differences"]["filuna_unaligned_moves"], 0)

    def test_different_arc_center_detected(self):
        r = triage(b"G3 X1 Y1 I-.05 J1.216 P1\n",
                   b"G3 X1 Y1 I-.08 J1.216 P1\n")
        self.assertEqual(r["total_differences"]["geometry_edit_groups"], 1)

    def test_one_exact_sample_not_homologated(self):
        r = triage(b"G1 X1\n", b"G1 X1\n")
        self.assertEqual(r["status"], "DIAGNOSTIC_ONE_SAMPLE_RAW_IDENTICAL_NOT_HOMOLOGATED")
        self.assertEqual(r["homologation"], "NOT_PROVEN")

    def test_temperature_difference_blocks(self):
        r = triage(b"M104 S215\nG1 X1\n", b"M104 S220\nG1 X1\n")
        self.assertEqual(r["status"], "FAIL_FUNCTIONAL_OR_METADATA_MISMATCH")
        self.assertEqual(r["homologation"], "NOT_PROVEN")
        self.assertEqual(r["mismatching_layers"], 0)

    def test_missing_gcode_blocks(self):
        self.assertEqual(triage(b"G1 X1\n", b"")["status"],
                         "BLOCKED_EMPTY_OR_MISSING_GCODE")

    def test_layer_localization_and_fan_count(self):
        a = b";LAYER_CHANGE\nG1 X1\n;LAYER_CHANGE\nM106 S255\nG1 X2\n"
        b = b";LAYER_CHANGE\nG1 X1\n;LAYER_CHANGE\nG1 X3\n"
        r = triage(a, b)
        self.assertEqual(r["per_layer_differences"][0]["layer"], 1)
        self.assertEqual(r["command_counts"]["M106"], {"official": 1, "filuna": 0})


if __name__ == "__main__":
    unittest.main()
