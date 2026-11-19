#!/usr/bin/env python3
"""Negative/positive verification: guard PASS never equals OEM homologation."""
from __future__ import annotations
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"FILUNA"/"tools"))
from oem_three_brand_promotion_guard import assess,inspect_brand

class ThreeBrandPromotionGuardTests(unittest.TestCase):
    def setUp(self):
        tmp=tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.repo=Path(tmp.name)
        self.data={}
        for brand in ("snapmaker","anycubic","flashforge"):
            manifest=self.repo/"FILUNA"/"fabricantes"/brand/"manifest.json"
            source=self.repo/"fabricantes"/brand/"adapter"/(brand+"_adapter.cpp")
            manifest.parent.mkdir(parents=True)
            source.parent.mkdir(parents=True)
            m={
                "manufacturer_id":brand,
                "state":"OFFICIAL_OEM_QA_IN_PROGRESS_NATIVE_NOT_INTEGRATED",
                "automatic_hardware_activation":False,
                "model_catalog":{"fixed_models_whitelist":False},
                "qa":{"internal_homologation":"BLOCKED",
                      "filuna_native_slicing":"NOT_RUN",
                      "filuna_native_network":"NOT_RUN",
                      "full_oem_dependencies_review":"BLOCKED_64",
                      "physical_beta":"NOT_RUN"}
            }
            if brand=="snapmaker":
                m["qa"].update({
                    "u1_final_purge_safety_regression":{"simulated_firmware_gcode_safety":"NOT_RUN"},
                    "u1_original_240_portable_regression_reference":{"result":"FAIL_REAL_TOOLPATH_VARIANTS"},
                    "u1_original_cli_40_run_stress":"FAIL_NON_TIMESTAMP_VARIANTS"})
            self.data[brand]=(manifest,source,m)
            source.write_text('#include "PendingVendorAdapter.hpp"\n')
            manifest.write_text(json.dumps(m))

    def save(self,brand):
        man,src,doc=self.data[brand]
        man.write_text(json.dumps(doc))

    def test_all_three_pending_are_valid_guard_pass_not_homologated(self):
        r=assess(self.repo)
        self.assertEqual(r["status"],"PASS_GUARD_NO_FALSE_HOMOLOGATION")
        self.assertEqual(r["native_homologated_by_this_tool"],[])
        self.assertTrue(all(x["internal_homologation_proven_here"] is False
                            for x in r["cases"]))

    def test_stub_cannot_claim_homologated(self):
        d=self.data["flashforge"][2]
        d["qa"]["internal_homologation"]="HOMOLOGADO"
        self.save("flashforge")
        self.assertIn("FALSE_HOMOLOGATION_WITH_PENDING_NATIVE_CPP_ADAPTER",
                      inspect_brand(self.repo,"flashforge")["errors"])

    def test_native_parity_missing_blocks_fake_promotion(self):
        d=self.data["anycubic"][2]
        d["qa"]["internal_homologation"]="PASS_INTERNAL"
        self.save("anycubic")
        self.assertIn("REAL_FILUNA_CPP_ENGINE_GCODE_PARITY_MISSING",
                      inspect_brand(self.repo,"anycubic")["errors"])
        self.assertIn("REAL_FILUNA_CPP_NETWORK_AND_ACK_MISSING",
                      inspect_brand(self.repo,"anycubic")["errors"])

    def test_snapmaker_safety_blocker_cannot_be_hidden(self):
        d=self.data["snapmaker"][2]
        d["qa"]["internal_homologation"]="PASS_INTERNO"
        self.save("snapmaker")
        self.assertIn("U1_REAL_TOOLPATH_AND_ZHOP_SAFETY_NOT_CLEARED",
                      inspect_brand(self.repo,"snapmaker")["errors"])

    def test_activating_real_hardware_is_always_blocked(self):
        d=self.data["flashforge"][2]
        d["automatic_hardware_activation"]=True
        self.save("flashforge")
        self.assertIn("UNAUTHORIZED_AUTOMATIC_HARDWARE_ACTIVATION",
                      inspect_brand(self.repo,"flashforge")["errors"])

    def test_two_model_qa_cannot_be_permanent_whitelist(self):
        d=self.data["anycubic"][2]
        d["model_catalog"]["fixed_models_whitelist"]=True
        self.save("anycubic")
        self.assertIn("QA_SAMPLES_WRONGLY_SET_AS_SUPPORTED_MODEL_WHITELIST",
                      inspect_brand(self.repo,"anycubic")["errors"])

    def test_wrong_manufacturer_identity_blocks(self):
        d=self.data["anycubic"][2]
        d["manufacturer_id"]="flashforge"
        self.save("anycubic")
        self.assertIn("BRAND_ID_MISMATCH",inspect_brand(self.repo,"anycubic")["errors"])

    def test_none_of_three_can_claim_physical_beta_completed_statically(self):
        d=self.data["snapmaker"][2]
        d["qa"]["physical_beta"]="HOMOLOGATED"
        self.save("snapmaker")
        self.assertIn("PHYSICAL_PRINT_CANNOT_BE_PROVED_BY_STATIC_QA",
                      inspect_brand(self.repo,"snapmaker")["errors"])

    def test_unknown_vendor_cannot_bypass_gate(self):
        with self.assertRaisesRegex(ValueError,"OUTSIDE_THREE_BRAND_QA_PILOT"):
            inspect_brand(self.repo,"bambu")

if __name__=="__main__":
    unittest.main()
