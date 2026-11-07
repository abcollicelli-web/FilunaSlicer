"""G-code diff MUST fail closed without implying Filuna native OEM approval."""
import json
from pathlib import Path
import tempfile
import unittest

from oem_functional_gcode_diff import analyze_bytes, evaluate


class GcodeDifferentialTests(unittest.TestCase):
    def test_movement_difference_not_hidden(self):
        result = analyze_bytes(b"; generated at 12:00\nG1 X1 Y1 E0.5\n",
                               b"; generated at 12:01\nG1 X2 Y1 E0.5\n",
                               "flashforge")
        self.assertEqual(result["status"], "FAIL_GCODE_COMMAND_STREAM_DIFFERENT")
        self.assertEqual(result["filuna_parity"], "NOT_PROVEN")
        self.assertFalse(result["noncomment_command_equal_diagnostic_only"])
        self.assertEqual(result["first_command_difference"]["official"][0]["text"],
                         "G1 X1 Y1 E0.5")

    def test_timestamp_only_diagnostic_never_certifies(self):
        result = analyze_bytes(b"; generated at 12:00\nG1 X1\n",
                               b"; generated at 12:01\nG1 X1\n",
                               "anycubic")
        self.assertEqual(result["status"],
                         "DIAGNOSTIC_COMMANDS_EQUAL_METADATA_NOT_CERTIFIED")
        self.assertTrue(result["noncomment_command_equal_diagnostic_only"])
        self.assertEqual(result["filuna_parity"], "NOT_PROVEN")

    def test_single_exact_sample_not_homologated(self):
        result = analyze_bytes(b"G1 X1\n", b"G1 X1\n", "snapmaker")
        self.assertEqual(result["status"],
                         "DIAGNOSTIC_ONE_SAMPLE_IDENTICAL_NOT_HOMOLOGATED")
        self.assertEqual(result["filuna_parity"], "NOT_PROVEN")

    def test_temperature_and_toolchange_reported(self):
        result = analyze_bytes(b"M104 S210\nT0\n", b"M104 S220\nT1\n", "flashforge")
        self.assertEqual(result["status"], "FAIL_GCODE_COMMAND_STREAM_DIFFERENT")
        self.assertEqual(result["families"]["TOOLCHANGE_T0"]["official"], 1)
        self.assertEqual(result["families"]["TOOLCHANGE_T1"]["filuna"], 1)

    def test_motion_fingerprint_flags_real_extrusion_and_speed_difference(self):
        official = b"; header\nM73 P5 R15\nG1 X101.185 Y101.185 E.02839\nG1 F7947\n"
        filuna = b"; header\nM73 P5 R15\nG1 X101.185 Y101.185 E.0284\nG1 F7970\n"
        result = analyze_bytes(official, filuna, "flashforge")
        self.assertEqual(result["motion_counts"], {"official": 2, "filuna": 2})
        self.assertFalse(result["motion_equal_diagnostic_only"])
        self.assertEqual(result["first_motion_difference"]["index"], 0)
        self.assertEqual(result["filuna_parity"], "NOT_PROVEN")

    def test_motion_hash_equal_is_not_sufficient_for_parity(self):
        result = analyze_bytes(b"M104 S215\nG1 X1\n",
                               b"M104 S220\nG1 X1\n", "flashforge")
        self.assertTrue(result["motion_equal_diagnostic_only"])
        self.assertEqual(result["status"], "FAIL_GCODE_COMMAND_STREAM_DIFFERENT")
        self.assertEqual(result["filuna_parity"], "NOT_PROVEN")

    def test_missing_candidate_never_succeeds(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            official = root / "original.gcode"
            official.write_bytes(b"G1 X1\n")
            result = evaluate("anycubic", official, root / "filuna-does-not-exist")
            self.assertEqual(result["status"], "BLOCKED_FILUNA_GCODE_NOT_GENERATED")
            self.assertEqual(result["filuna_parity"], "NOT_PROVEN")

    def test_multiple_candidates_are_ambiguous(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            official = root / "original.gcode"
            official.write_bytes(b"G1 X1\n")
            candidates = root / "filuna"
            candidates.mkdir()
            (candidates / "a.gcode").write_bytes(b"G1 X1\n")
            (candidates / "b.gcode").write_bytes(b"G1 X1\n")
            result = evaluate("flashforge", official, candidates)
            self.assertEqual(result["status"], "BLOCKED_MULTIPLE_FILUNA_GCODE_UNSCOPED")
            self.assertEqual(result["filuna_parity"], "NOT_PROVEN")

    def test_empty_file_not_certified(self):
        self.assertEqual(
            analyze_bytes(b"G1 X1\n", b"", "anycubic")["status"],
            "DIAGNOSTIC_EMPTY_GCODE")


if __name__ == "__main__":
    unittest.main()
