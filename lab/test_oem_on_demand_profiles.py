#!/usr/bin/env python3
"""Positive/negative QA for the future data-only on-demand Filuna profile flow."""
from __future__ import annotations
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"FILUNA"/"tools"))
from oem_universal_inventory import read_catalog
from oem_on_demand_profiles import prepare_model_bundle, verify_model_bundle

SHA="a"*40
MACHINE="Brand Printer X 0.4 nozzle"

class DemandProfileTests(unittest.TestCase):
    def setUp(self):
        self.dir=tempfile.TemporaryDirectory()
        self.addCleanup(self.dir.cleanup)
        self.root=Path(self.dir.name)
        self.vendor=self.root/"official"/"profiles"/"Brand"
        self.vendor.mkdir(parents=True)
        self.mkdir_preset("machine",{
            "type":"machine","from":"system","name":MACHINE,
            "printer_model":"Brand Printer X","nozzle_diameter":["0.4"],
            "default_print_profile":"0.20mm Standard X",
            "default_filament_profile":["PLA X"]})
        self.mkdir_preset("process",{"type":"process","from":"system",
            "name":"0.20mm Standard X","compatible_printers":[MACHINE],
            "layer_height":"0.2"})
        self.mkdir_preset("filament",{"type":"filament","from":"system",
            "name":"PLA X","compatible_printers":[MACHINE],
            "filament_type":["PLA"]})
        self.mkdir_preset("filament",{"type":"filament","from":"system",
            "name":"Foreign Filament","compatible_printers":["Brand Y"]})

    def mkdir_preset(self,kind,payload):
        folder=self.vendor/kind
        folder.mkdir(parents=True,exist_ok=True)
        name=payload["name"].replace(" ","-")
        (folder/(name+".json")).write_text(
            json.dumps(payload,ensure_ascii=False)+"\n",encoding="utf-8")

    def build(self):
        return prepare_model_bundle(
            read_catalog(self.vendor),"brand",MACHINE,SHA,self.root/"bundle")

    def test_build_exact_model_only_no_foreign_presets(self):
        b=self.build()
        self.assertEqual(b["profile_counts"],{"machine":1,"process":1,"filament":1})
        self.assertEqual(b["shipping_status"],"NOT_SIGNED_NOT_ONLINE_NOT_INTEGRATED")
        self.assertNotIn("Foreign Filament",str(b))
        self.assertEqual(verify_model_bundle(self.root/"bundle")["machine"],MACHINE)
        self.assertEqual(len(list((self.root/"bundle"/"data").rglob("*.json"))),3)

    def test_tampered_profile_fails_integrity(self):
        b=self.build()
        p=self.root/"bundle"/b["files"][1]["path"]
        p.write_bytes(p.read_bytes()+b"malicious")
        with self.assertRaisesRegex(ValueError,"PROFILE_HASH_OR_SIZE_MISMATCH"):
            verify_model_bundle(self.root/"bundle")

    def test_unknown_machine_not_auto_selected(self):
        with self.assertRaisesRegex(ValueError,"MODEL_NOT_IN_VERIFIED_ORIGINAL_CATALOG"):
            prepare_model_bundle(read_catalog(self.vendor),"brand",
                "Brand Printer X 0.8 nozzle",SHA,self.root/"bundle")

    def test_unknown_predicate_excluded_and_reported(self):
        self.mkdir_preset("process",{"type":"process","name":"Experimental Conditional",
            "compatible_printers":[MACHINE],
            "compatible_printers_condition":"some_unsupported_firmware_rule"})
        b=self.build()
        self.assertIn("OEM_COMPATIBILITY_PREDICATE_NOT_EVALUATED",
            [e["reason"] for e in b["blocked_compatibility"]])
        self.assertNotIn("Experimental Conditional",[r["name"] for r in b["files"]])

    def test_existing_nonempty_folder_refused(self):
        d=self.root/"bundle";d.mkdir()
        (d/"existing.json").write_text("{}")
        with self.assertRaisesRegex(ValueError,"BUNDLE_DESTINATION_MUST_BE_EMPTY"):
            self.build()

    def test_unpinned_vendor_commit_refused(self):
        with self.assertRaisesRegex(ValueError,"IMMUTABLE_OFFICIAL_COMMIT_REQUIRED"):
            prepare_model_bundle(read_catalog(self.vendor),"brand",MACHINE,
                "main",self.root/"bundle")

    def test_unknown_default_reports_blocker(self):
        self.mkdir_preset("machine",{"type":"machine","name":MACHINE,
            "printer_model":"Brand Printer X","nozzle_diameter":["0.4"],
            "default_print_profile":"NOT_VERIFIED_DEFAULT",
            "default_filament_profile":["PLA X"]})
        b=self.build()
        self.assertIn("DEFAULT_PROCESS_NOT_PROVEN_COMPATIBLE",
            [e["reason"] for e in b["blocked_compatibility"]])

if __name__=="__main__":
    unittest.main()
