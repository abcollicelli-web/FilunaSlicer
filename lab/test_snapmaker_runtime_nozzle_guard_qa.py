#!/usr/bin/env python3
"""Snapmaker CLI source-preserving workaround negative and positive controls."""
import json
from pathlib import Path
import tempfile
import unittest
from snapmaker_runtime_nozzle_guard_qa import guarded_runtime_process


class TestGuardedRuntimeProcess(unittest.TestCase):
    def pair(self,root,machine=None,process=None):
        printer={"type":"machine","name":"Snapmaker Test Model 0.4 nozzle",
                 "nozzle_diameter":["0.4","0.6","0.4","0.6"]}
        material={"type":"process","name":"0.20mm Test Quality",
                  "compatible_printers":["Snapmaker Test Model 0.4 nozzle"],
                  "wipe_tower_filament":"0","layer_height":"0.20"}
        printer.update(machine or {})
        material.update(process or {})
        a=root/"machine.json";b=root/"process.json";c=root/"runtime.json"
        a.write_text(json.dumps(printer));b.write_text(json.dumps(material))
        return a,b,c

    def test_no_model_specific_hardcoding_and_full_original_preservation(self):
        with tempfile.TemporaryDirectory() as t:
            a,b,c=self.pair(Path(t))
            before=a.read_bytes(),b.read_bytes()
            evidence=guarded_runtime_process(a,b,c)
            self.assertEqual(evidence["nozzle_diameters"],["0.4","0.6","0.4","0.6"])
            self.assertEqual((a.read_bytes(),b.read_bytes()),before)
            payload=json.loads(c.read_text())
            self.assertEqual(payload["wipe_tower_filament"],"0")
            self.assertEqual(payload["layer_height"],"0.20")
            self.assertEqual(payload["nozzle_diameter"],["0.4","0.6","0.4","0.6"])
            self.assertFalse(evidence["manufacturer_profiles_modified"])
            self.assertFalse(evidence["native_filuna_integration"])

    def test_foreign_model_is_rejected(self):
        with tempfile.TemporaryDirectory() as t:
            a,b,c=self.pair(Path(t),{"name":"Snapmaker Other"})
            with self.assertRaisesRegex(ValueError,"NOT_COMPATIBLE"):
                guarded_runtime_process(a,b,c)
            self.assertFalse(c.exists())

    def test_no_nozzles_rejected(self):
        with tempfile.TemporaryDirectory() as t:
            a,b,c=self.pair(Path(t),{"nozzle_diameter":[]})
            with self.assertRaisesRegex(ValueError,"NOZZLE_DIAMETERS_INVALID"):
                guarded_runtime_process(a,b,c)

    def test_invalid_nozzle_rejected(self):
        with tempfile.TemporaryDirectory() as t:
            a,b,c=self.pair(Path(t),{"nozzle_diameter":["0","-1"]})
            with self.assertRaisesRegex(ValueError,"NOZZLE_DIAMETERS_INVALID"):
                guarded_runtime_process(a,b,c)

    def test_existing_process_nozzles_cannot_be_overwritten(self):
        with tempfile.TemporaryDirectory() as t:
            a,b,c=self.pair(Path(t),process={"nozzle_diameter":["0.8"]})
            with self.assertRaisesRegex(ValueError,"ALREADY_HAS_NOZZLE"):
                guarded_runtime_process(a,b,c)

    def test_no_wipe_tower_cannot_be_rewritten(self):
        with tempfile.TemporaryDirectory() as t:
            a,b,c=self.pair(Path(t))
            p=json.loads(b.read_text());p.pop("wipe_tower_filament")
            b.write_text(json.dumps(p))
            with self.assertRaisesRegex(ValueError,"NOT_THE_VERIFIED"):
                guarded_runtime_process(a,b,c)


if __name__=="__main__":
    unittest.main()
