#!/usr/bin/env python3
"""Fast offline tests for OEM G-code parity and effective preset comparison."""
import importlib.util
import json
import pathlib
import tempfile
import unittest

HERE=pathlib.Path(__file__).resolve().parent

def module(name,filename):
    spec=importlib.util.spec_from_file_location(name,HERE/filename)
    mod=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod

gc=module("oem_gcode","compare_oem_gcode.py")
presets=module("oem_presets","audit_oem_preset_parity.py")

class TestOEMGate(unittest.TestCase):
    def test_same_output_is_preliminary_only(self):
        with tempfile.TemporaryDirectory() as folder:
            f=pathlib.Path(folder)/"official.gcode"
            f.write_text("; demo\nG1 X1 Y1 E0.1\nM104 S210\n")
            r=gc.compare(f,f)
            self.assertEqual(r["status"],"PRELIMINARY_MATCH_NOT_HARDWARE_PASS")
            self.assertFalse(r["hardware_homologated"])
            self.assertFalse(r["module_autoload_tested"])
    def test_critical_macro_difference_blocks_parity(self):
        with tempfile.TemporaryDirectory() as folder:
            a=pathlib.Path(folder)/"a.gcode";b=pathlib.Path(folder)/"b.gcode"
            a.write_text("G1 X1 E1\nM104 S210\n")
            b.write_text("G1 X1 E1\nM104 S220\n")
            self.assertEqual(gc.compare(a,b)["status"],"MISMATCH")
    def test_preset_inheritance_difference_blocks(self):
        with tempfile.TemporaryDirectory() as folder:
            root=pathlib.Path(folder)
            for vendor,temperature in [("OEM", "210"),("FILUNA","220")]:
                p=root/vendor/"machine"
                p.mkdir(parents=True)
                (p/"base.json").write_text(json.dumps({"name":"base","temperature":temperature}))
                (p/"A1.json").write_text(json.dumps({"name":"A1","inherits":"base","nozzle":"0.4"}))
            a=presets.analyze(root/"OEM",root/"FILUNA",[("machine","A1")])
            self.assertEqual(a["status"],"MISMATCH")
            self.assertIn("temperature",a["records"][0]["differences"])
    def test_missing_preset_fails_closed(self):
        with tempfile.TemporaryDirectory() as folder:
            root=pathlib.Path(folder)
            a=presets.analyze(root/"OEM",root/"FILUNA",[("machine","A1")])
            self.assertEqual(a["records"][0]["status"],"FAIL")
    def test_preset_cycle_fails_closed(self):
        with tempfile.TemporaryDirectory() as folder:
            root=pathlib.Path(folder)
            for vendor in ("OEM","FILUNA"):
                p=root/vendor/"machine";p.mkdir(parents=True)
                (p/"A1.json").write_text('{"inherits":"base"}')
                (p/"base.json").write_text('{"inherits":"A1"}')
            a=presets.analyze(root/"OEM",root/"FILUNA",[("machine","A1")])
            self.assertEqual(a["records"][0]["status"],"FAIL")

if __name__=="__main__":unittest.main()
