#!/usr/bin/env python3
"""Static checks on the real CORE-04 matrix (not integration evidence)."""
import hashlib, json, unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
M = json.loads((ROOT / "FILUNA/qa/baseline/cases.json").read_text(encoding="utf-8"))


class T(unittest.TestCase):
    def test_fixture_hashes_real(self):
        for f in M["fixtures"]:
            p = ROOT / f["path"]
            self.assertTrue(p.is_file(), f["path"])
            self.assertGreater(p.stat().st_size, 0)
            self.assertEqual(hashlib.sha256(p.read_bytes()).hexdigest(), f["sha256"], f["id"])

    def test_no_synthetic_fixtures(self):
        for f in M["fixtures"]:
            self.assertNotIn("FILUNA/qa/fixtures/samples", f["path"])

    def test_presets_exist_and_exact_models(self):
        for p in M["presets"]:
            for k in ("machine", "process", "filament"):
                if p.get(k):
                    self.assertTrue((ROOT / p[k]).is_file(), p[k])
        cc2 = next(p for p in M["presets"] if p["id"] == "PR-ELEGOO-CC2")
        self.assertIn("Centauri Carbon 2", cc2["machine"])
        k3 = next(p for p in M["presets"] if p["id"] == "PR-ANYCUBIC-K3V2")
        self.assertIn("Kobra 3 V2", k3["machine"])
        self.assertIn("Kobra 3 V2", k3["process"])

    def test_formats_covered(self):
        self.assertEqual({f["format"] for f in M["fixtures"]}, {"OBJ", "STL", "STEP", "3MF"})


if __name__ == "__main__":
    unittest.main(verbosity=2)
