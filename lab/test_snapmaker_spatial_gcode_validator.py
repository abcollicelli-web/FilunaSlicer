#!/usr/bin/env python3
"""Read-only geometry/permission regression tests; synthetic cases are NOT OEM approval."""
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"lab"))
from snapmaker_spatial_gcode_validator import (
    ACK, acknowledge_risk, paired_audit, policy_from_yaml, validate_gcode
)

VENDOR=policy_from_yaml(ROOT/"FILUNA/fabricantes/snapmaker/vendor.yaml")

def geometry_square(*, lift=False, tail=True):
    lines=[
        "G90","M83",
        "G1 X10 Y10 Z1.0",
        "G1 X20 Y10 E1",
        "G1 X20 Y20 E1",
        "G1 X10 Y20 E1",
        "G1 X10 Y10 E1",
        "G1 X9 Y15",  # setup (still may cross deposited wall)
        "; stop printing object cube id:0",
        ";FILUNA_FINAL_PURGE"]
    if lift:
        lines.append("G1 Z2.0")
    lines.append("G1 X21 Y15 F24000")
    return ("\n".join(lines)+"\n").encode("ascii")


class SpatialSafetyTests(unittest.TestCase):
    def test_01_vendor_macro_has_pinned_qualified_evidence(self):
        self.assertEqual(VENDOR["volume_mm"],[270,270,270])
        self.assertEqual(len(VENDOR["safe_macros"]),1)
        macro=VENDOR["safe_macros"][0]
        self.assertEqual(macro["name"],"PRINT_END")
        self.assertIn("not_final_purge",macro["scope"])
        self.assertNotIn("all_travels",macro["scope"])

    def test_02_final_purge_collision_blocked_in_official(self):
        original=geometry_square()
        report=validate_gcode(original,VENDOR,source="official",scan="critical")
        self.assertEqual(report["status"],"BLOCK",report)
        self.assertFalse(report["send_allowed"])
        self.assertIn("TRAVEL_CROSSES_PRINTED_GEOMETRY",
                      [x["code"] for x in report["issues"]])
        self.assertEqual(report["gcode_sha256"],hashlib.sha256(original).hexdigest())
        self.assertFalse(report["gcode_modified"])
        self.assertGreater(report["issues"][0]["line"],0)

    def test_03_verified_clearance_releases(self):
        report=validate_gcode(geometry_square(lift=True),VENDOR,source="official")
        self.assertEqual(report["status"],"PASS",report)
        self.assertTrue(report["send_allowed"])

    def test_04_auto_spiral_outside_volume_warns_not_rewrites(self):
        raw=(b"G90\nG1 X260 Y260 Z1\n;FILUNA_SPIRAL_HOP\n"
             b"G3 X272 Y262 Z1.5 I6 J1 F3000\n")
        report=validate_gcode(raw,VENDOR,source="official")
        self.assertEqual(report["status"],"WARN",report)
        self.assertIn("AUTO_SPIRAL_OUT_OF_VOLUME",
                      [x["code"] for x in report["warnings"]])

    def test_05_local_filuna_collision_requires_official_comparison(self):
        report=validate_gcode(geometry_square(),VENDOR,source="filuna")
        self.assertEqual(report["status"],"REVIEW_REQUIRED",report)
        self.assertFalse(report["send_allowed"])

    def test_06_unequal_filuna_file_never_inherits_oem_pass(self):
        report=paired_audit(geometry_square(),geometry_square(lift=True),VENDOR)
        self.assertEqual(report["status"],"ENGINE_DIVERGENCE",report)
        self.assertFalse(report["gcode_exact_match"])
        self.assertFalse(report["send_allowed"])

    def test_07_equal_oem_risk_is_blocked_with_attribution(self):
        raw=geometry_square()
        report=paired_audit(raw,raw,VENDOR)
        self.assertEqual(report["status"],"BLOCK",report)
        self.assertEqual(report["origin"],"OFFICIAL_IDENTICAL")

    def test_08_override_requires_paired_official_and_consented_user(self):
        raw=geometry_square()
        report=paired_audit(raw,raw,VENDOR)
        with tempfile.TemporaryDirectory() as d:
            audit=Path(d)/"audit.jsonl"
            with self.assertRaisesRegex(ValueError,"EXPLICIT_INFORMED"):
                acknowledge_risk(report,"operator","yes",audit)
            accepted=acknowledge_risk(report,"operator",ACK,audit)
            self.assertEqual(accepted["status"],"USER_ACCEPTED_RISK")
            self.assertTrue(accepted["send_allowed"])
            event=json.loads(audit.read_text().splitlines()[0])
            self.assertEqual(event["gcode_sha256"],hashlib.sha256(raw).hexdigest())

    def test_09_engine_divergence_cannot_be_overridden_as_oem(self):
        report=paired_audit(geometry_square(),geometry_square(lift=True),VENDOR)
        with tempfile.TemporaryDirectory() as d:
            with self.assertRaisesRegex(ValueError,"OVERRIDE_REQUIRES"):
                acknowledge_risk(report,"operator",ACK,Path(d)/"audit.jsonl")

    def test_10_no_unsafe_macro_whitelist(self):
        with tempfile.TemporaryDirectory() as d:
            policy=json.loads((ROOT/"FILUNA/fabricantes/snapmaker/vendor.yaml").read_text())
            policy["safe_macros"][0]["scope"]="all_travels"
            p=Path(d)/"vendor.yaml"
            p.write_text(json.dumps(policy))
            with self.assertRaisesRegex(ValueError,"BLANKET_MACRO"):
                policy_from_yaml(p)

    def test_11_other_manufacturers_use_same_engine(self):
        # Synthetic regression only. Real A1/Creality official fixtures require
        # separate pinned download and must not be replaced by these tests.
        for vendor in ["bambu","creality"]:
            p=dict(VENDOR,manufacturer=vendor,printer_model="QA fixture")
            report=validate_gcode(geometry_square(lift=True),p,source="official")
            self.assertEqual(report["status"],"PASS",report)

    def test_12_critical_mode_skips_unrelated_routine_intralayer_travel(self):
        raw=geometry_square().replace(b"; stop printing object cube id:0",b"; layer: 1").replace(
            b";FILUNA_FINAL_PURGE",b"; ordinary travel")
        report=validate_gcode(raw,VENDOR,source="official")
        self.assertEqual(report["status"],"PASS",report)

    def test_13_single_case_never_mutates_input_file(self):
        raw=geometry_square()
        with tempfile.TemporaryDirectory() as d:
            f=Path(d)/"part.gcode"
            f.write_bytes(raw)
            original=f.read_bytes()
            validate_gcode(f.read_bytes(),VENDOR,source="official")
            self.assertEqual(f.read_bytes(),original)


    def test_14_prime_tower_travel_not_misread_as_part_collision(self):
        raw=(b"G90\nM83\nG1 X80 Y80 Z1\n;TYPE:Outer wall\n"
             b"G1 X90 Y80 E1\nG1 X90 Y90 E1\nG1 X80 Y90 E1\n"
             b";TYPE:Prime tower\nG1 X100 Y220\nG1 X120 Y220 E1\n"
             b"G1 X120 Y240 E1\nG1 X100 Y240 E1\nG1 X100 Y220 E1\n"
             b"T1\nG1 X119 Y230 F30000\n")
        result=validate_gcode(raw,VENDOR,source="official")
        self.assertEqual(result["status"],"PASS",result)
        self.assertGreater(result["prime_tower_deposition_segments"],0)
        self.assertEqual(result["issues"],[])



    def test_15_relative_z_before_homing_anchor_is_unknown_not_below_bed(self):
        raw=(b"G90\nG28 X\nG91\nG1 Z5 F1200\nG1 Z-5 F1200\n"
             b"G90\nG0 X10 Y10 Z1\nM83\nG1 X20 Y10 E1\n")
        report=validate_gcode(raw,VENDOR,source="official")
        self.assertEqual(report["status"],"PASS",report)
        self.assertEqual(report["warnings"],[])



    def test_16_layer_change_reported_in_collision(self):
        raw=geometry_square().replace(
            b"G1 X20 Y10 E1", b";LAYER_CHANGE\nG1 X20 Y10 E1")
        report=validate_gcode(raw,VENDOR,source="official")
        self.assertEqual(report["status"],"BLOCK",report)
        self.assertEqual(report["issues"][0]["layer"],0)


if __name__=="__main__":
    unittest.main()
