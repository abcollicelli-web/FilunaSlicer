import importlib.util
import json
from pathlib import Path
import unittest

root=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location("tracker",root/"check_bambu_orca_upstream.py")
m=importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

manifest={"upstream":{"repository":"bambulab/BambuStudio","branch":"master",
                      "tracked_code_candidates":[{"path":"src/libslic3r/GCode.cpp","blob_sha":"hash2"}]},
          "components":[{"path":"resources/a.json","upstream_blob_sha":"hash1"}],
          "filuna":{"upstream_orca_pin":"orcapin"}}

def getter(path):
    if "a.json" in path:return {"sha":"hash1"}
    if "GCode.cpp" in path:return {"sha":"hash2"}
    if "OrcaSlicer" in path:return {"sha":"orcapin"}
    raise AssertionError("unexpected endpoint "+path)

class DeltaTest(unittest.TestCase):
    def test_unchanged_sources_not_update(self):
        r=m.observe(manifest,getter)
        self.assertFalse(r["needs_review"])
        self.assertTrue(all(x["status"]=="SAME" for x in r["rows"]))
    def test_changed_oem_flags_review(self):
        def modified(path):
            if "GCode.cpp" in path:return {"sha":"changed"}
            return getter(path)
        r=m.observe(manifest,modified)
        self.assertTrue(r["needs_review"])
        self.assertEqual(r["rows"][1]["status"],"UPDATED_REVIEW_REQUIRED")
        self.assertTrue(r["does_not_auto_update"])
    def test_api_failure_flags_review(self):
        def fail(path):
            if "a.json" in path:raise ValueError("network failed")
            return getter(path)
        r=m.observe(manifest,fail)
        self.assertEqual(r["rows"][0]["status"],"CHECK_FAILED")
        self.assertTrue(r["needs_review"])
    def test_orca_update_flags_review(self):
        def modified(path):
            if "OrcaSlicer" in path:return {"sha":"new-orca"}
            return getter(path)
        r=m.observe(manifest,modified)
        self.assertTrue(r["needs_review"])
        self.assertEqual(r["rows"][-1]["status"],"UPDATED_REVIEW_REQUIRED")

if __name__=="__main__":unittest.main()
