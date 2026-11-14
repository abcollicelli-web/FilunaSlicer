"""Fail-closed status classification for real OEM-versus-preexisting-Filuna diagnostic."""
import unittest
from filuna_baseline_three_oem_qacheck import baseline_sample_verdict

class BaselineSingleSampleTests(unittest.TestCase):
    def test_identical_sample_not_homologated(self):
        status=baseline_sample_verdict(b"G1 X1 Y2 E0.5\n",b"G1 X1 Y2 E0.5\n")
        self.assertEqual(status,"DIAGNOSTIC_ONE_SAMPLE_IDENTICAL_NOT_HOMOLOGATED")
        self.assertFalse(status.startswith("PASS"))
    def test_different_toolpath_fails(self):
        self.assertEqual(baseline_sample_verdict(b"G1 X1\n",b"G1 X2\n"),
                         "FAIL_GCODE_DIFFERENT_NOT_OEM_PARITY")
    def test_empty_rejected(self):
        self.assertEqual(baseline_sample_verdict(b"",b""),"FAIL_EMPTY_GCODE")
        self.assertEqual(baseline_sample_verdict(b"G1 X1\n",b""),"FAIL_EMPTY_GCODE")

if __name__=="__main__":
    unittest.main()
