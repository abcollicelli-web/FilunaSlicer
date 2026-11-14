"""Model-scoped OEM system preset manifests: positive and fail-closed QA."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest

from oem_model_scoped_manifest_probe import make_source_bundle


def sample():
    machine = {
        "name": "Example Printer 0.4 nozzle", "from": "system",
        "type": "machine", "inherits": "Official ancestor",
        "printer_model": "Example Printer", "nozzle_diameter": ["0.4"],
        "retraction_distances_when_cut": ["0"],
        "enable_long_retraction_when_cut": "0",
        "machine_start_gcode": "G1 X1"
    }
    process = {
        "name": "0.20mm Example", "from": "system", "type": "process",
        "inherits": "Official process ancestor",
        "compatible_printers": [machine["name"]],
        "outer_wall_speed": "55"
    }
    filament = {
        "name": "PLA Example", "from": "system", "type": "filament",
        "inherits": "Official PLA ancestor",
        "compatible_printers": [machine["name"]],
        "temperature_vitrification": ["50"]
    }
    return {"machine": machine, "process": process, "filament": filament}


class ModelManifestProbeTests(unittest.TestCase):
    def test_minimal_valid_tree_uses_original_effective_values(self):
        with tempfile.TemporaryDirectory() as tmp:
            profiles = sample()
            original = json.dumps(profiles, sort_keys=True)
            report = make_source_bundle(profiles, "Example", Path(tmp) / "bundle")
            self.assertEqual(report["native_gcode_parity"], "NOT_PROVEN")
            self.assertEqual(report["shipping_status"], "FORBIDDEN_QA_ONLY")
            self.assertEqual(json.dumps(profiles, sort_keys=True), original)
            root = Path(tmp) / "bundle"
            manifest = json.loads((root / "Example.json").read_text())
            for kind in ("machine", "process", "filament"):
                records = manifest[kind + "_list"]
                self.assertEqual(len(records), 1)
                payload = json.loads(
                    (root / "Example" / records[0]["sub_path"]).read_text())
                self.assertEqual(payload["from"], "system")
                self.assertEqual(payload["inherits"], "")
                for key, value in profiles[kind].items():
                    if key != "inherits":
                        self.assertEqual(payload[key], value)
            m = json.loads((root / "Example" / "machine" /
                            "Example Printer 0.4 nozzle.json").read_text())
            self.assertEqual(m["retraction_distances_when_cut"], ["0"])

    def test_unverified_user_preset_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            profiles = sample()
            profiles["machine"]["from"] = "User"
            with self.assertRaisesRegex(ValueError, "ONLY_VERIFIED"):
                make_source_bundle(profiles, "Example", Path(tmp) / "x")

    def test_unproven_printer_compatibility_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            profiles = sample()
            profiles["filament"]["compatible_printers"] = ["Other printer"]
            with self.assertRaisesRegex(ValueError, "PROFILE_COMPATIBILITY"):
                make_source_bundle(profiles, "Example", Path(tmp) / "x")

    def test_unknown_predicate_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            profiles = sample()
            profiles["process"]["compatible_printers_condition"] = "unknown()"
            with self.assertRaisesRegex(ValueError, "PREDICATE"):
                make_source_bundle(profiles, "Example", Path(tmp) / "x")

    def test_path_traversal_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            profiles = sample()
            profiles["machine"]["name"] = "../escape"
            with self.assertRaisesRegex(ValueError, "UNSAFE"):
                make_source_bundle(profiles, "Example", Path(tmp) / "x")

    def test_nozzle_identity_required(self):
        with tempfile.TemporaryDirectory() as tmp:
            profiles = sample()
            profiles["machine"]["nozzle_diameter"] = []
            with self.assertRaisesRegex(ValueError, "NOZZLE"):
                make_source_bundle(profiles, "Example", Path(tmp) / "x")

    def test_incomplete_source_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            profiles = sample()
            del profiles["filament"]
            with self.assertRaisesRegex(ValueError, "THREE_RESOLVED"):
                make_source_bundle(profiles, "Example", Path(tmp) / "x")


if __name__ == "__main__":
    unittest.main()
