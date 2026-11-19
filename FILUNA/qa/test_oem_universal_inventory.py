#!/usr/bin/env python3
"""Universal OEM discovery regression: all models, source, network & review."""
from __future__ import annotations
import json
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"tools"))
from oem_universal_inventory import (
    classify, own_profile, read_catalog, survey_source,
)


def profile(root, dirname, filename, kind, name, extra=None):
    p = root/dirname/(filename+".json")
    p.parent.mkdir(parents=True,exist_ok=True)
    p.write_text(json.dumps(dict({"type":kind,"name":name}, **(extra or {})))+"\n")
    return p


def repo_init(root):
    subprocess.run(["git", "init", "-q", str(root)],check=True)
    subprocess.run(["git", "-C", str(root), "config","user.email","qa@example.invalid"],check=True)
    subprocess.run(["git", "-C", str(root), "config","user.name","Filuna QA"],check=True)


def repo_commit(root, files):
    for name, content in files.items():
        path=root/name
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_text(content,encoding="utf-8")
    subprocess.run(["git","-C",str(root),"add","-A"],check=True)
    subprocess.run(["git","-C",str(root),"commit","-m","test","--quiet"],check=True)


class TestUniversalVendorCatalog(unittest.TestCase):
    def test_no_hardcoded_models_all_models_and_profiles_with_parents(self):
        with TemporaryDirectory() as temp:
            root=Path(temp)
            profile(root,"machine","base","machine","Generic Base",{
                "printer_model":"SYNTHETIC","nozzle_diameter":["0.4"]})
            profile(root,"machine","model_a","machine","Synthetic Model A",{
                "inherits":"Generic Base","printer_model":"MODEL-A"})
            profile(root,"machine","model_b","machine","Synthetic Model B",{
                "inherits":"Generic Base","printer_model":"MODEL-B"})
            profile(root,"process","draft","process","Draft quality",{
                "layer_height":"0.2","compatible_printers":["Synthetic Model A"]})
            profile(root,"process","fine","process","Fine quality",{
                "layer_height":"0.12","compatible_printers":["Synthetic Model B"]})
            profile(root,"filament","pla","filament","PLA",{
                "compatible_printers_condition":"printer_model==MODEL-A"})
            catalog=read_catalog(root)
            self.assertEqual(catalog["profiles_per_type"],
                {"filament":1,"machine":3,"process":2})
            self.assertEqual(catalog["files_total"],6)
            self.assertEqual({x["model_id"] for x in catalog["machines"]},
                {"SYNTHETIC","MODEL-A","MODEL-B"})
            self.assertEqual(catalog["profiles"]["machine:Synthetic Model A"]["effective"]["nozzle_diameter"],["0.4"])
            self.assertEqual(catalog["capability_proof"],"NOT_RUN")
            self.assertIn("NOT evaluated",catalog["compatibility_note"])

    def test_unresolved_parent_is_visible_not_dropped(self):
        with TemporaryDirectory() as temp:
            root=Path(temp)
            profile(root,"machine","bad","machine","Variant A",{"inherits":"missing-parent"})
            result=read_catalog(root)
            self.assertEqual(len(result["issues"]),1)
            self.assertIn("MISSING_PROFILE_PARENT",result["issues"][0]["error"])
            self.assertEqual(result["machines"][0]["status"],"UNRESOLVED")

    def test_cycle_is_visible_not_approved(self):
        with TemporaryDirectory() as temp:
            root=Path(temp)
            profile(root,"machine","a","machine","A",{"inherits":"B"})
            profile(root,"machine","b","machine","B",{"inherits":"A"})
            result=read_catalog(root)
            self.assertEqual(len(result["issues"]),2)
            self.assertTrue(all("INHERITANCE_CYCLE" in x["error"]
                                for x in result["issues"]))

    def test_duplicate_is_reported(self):
        with TemporaryDirectory() as temp:
            root=Path(temp)
            profile(root,"machine","a","machine","Same")
            profile(root,"other","b","machine","Same")
            result=read_catalog(root)
            self.assertTrue(any(x["issue"]=="DUPLICATE_PRESET" for x in result["issues"]))

    def test_network_and_access_are_cross_brand(self):
        for brand in ("anycubic","elegoo","snapmaker","bambu","qidi","creality","flashforge"):
            self.assertEqual(classify("src/slic3r/Utils/DeviceNetworkAgent.cpp",brand)[0],
                             "NETWORK_AND_DISCOVERY")
            self.assertEqual(classify("src/slic3r/Utils/TLSCredentialManager.cpp",brand)[0],
                             "ACCESS_AND_AUTH")
            self.assertEqual(classify("src/slic3r/Utils/PrinterUploadManager.cpp",brand)[0],
                             "FILE_TRANSFER_AND_SEND")
            self.assertEqual(classify("src/libslic3r/GCode.cpp",brand)[0],
                             "SLICING_ENGINE")

    def test_other_brand_profiles_never_mixed(self):
        self.assertTrue(own_profile("resources/profiles/Anycubic/machine/S1.json","anycubic"))
        self.assertFalse(own_profile("resources/profiles/BBL/machine/A1.json","anycubic"))
        self.assertTrue(own_profile("resources/profiles/Snapmaker/process/U1.json","snapmaker"))

    def test_unknown_changed_file_is_reviewed_not_ignored(self):
        self.assertEqual(classify("src/strange/unknown_implementation.cpp","snapmaker")[0],
                         "REVIEW_TRANSITIVE_SOURCE")
        self.assertEqual(classify("deps/newvendor/libx.h","snapmaker")[0],
                         "REVIEW_TRANSITIVE_DEPENDENCY")

    def test_source_git_blob_stages_full_network_and_control_without_duplication(self):
        with TemporaryDirectory() as temp:
            root=Path(temp);base=root/"base";official=root/"official";out=root/"out"
            repo_init(base);repo_init(official)
            common={"src/libslic3r/Shared.cpp":"void shared(){}\n"}
            repo_commit(base,common)
            repo_commit(official,{**common,
              "src/slic3r/Utils/PrinterNetworkAgent.cpp":"void network(){}\n",
              "src/slic3r/Utils/TLSCredentialManager.cpp":"void auth(){}\n",
              "src/slic3r/Utils/PrinterUpload.cpp":"void upload(){}\n",
              "src/slic3r/Utils/VendorDeviceController.cpp":"void controller(){}\n",
              "src/ext/newdependency.cpp":"void dependency(){}\n",
              "resources/profiles/Anycubic/machine/all.json":'{"all": true}\n',
              "resources/profiles/BBL/machine/excluded.json":'{"bambu": true}\n'})
            result=survey_source(official,base,"anycubic",out,copy=True)
            paths={x["path"]:x for x in result["files"]}
            self.assertNotIn("resources/profiles/BBL/machine/excluded.json",paths)
            self.assertEqual(paths["src/libslic3r/Shared.cpp"]["copy_status"],"SHARED_NOT_DUPLICATED")
            for path in ("src/slic3r/Utils/PrinterNetworkAgent.cpp",
                         "src/slic3r/Utils/TLSCredentialManager.cpp",
                         "src/slic3r/Utils/PrinterUpload.cpp",
                         "src/slic3r/Utils/VendorDeviceController.cpp",
                         "src/ext/newdependency.cpp"):
                self.assertEqual(paths[path]["copy_status"],"STAGED_FOR_AUDIT_NOT_INTEGRATED")
                self.assertEqual((out/"source"/path).read_bytes(),(official/path).read_bytes())
            self.assertIn("src/ext/newdependency.cpp",result["review_blockers"])
            self.assertEqual(result["status"],"SOURCE_INVENTORY_ONLY_NOT_INTEGRATED_NOT_HOMOLOGATED")


if __name__=="__main__":
    unittest.main()
