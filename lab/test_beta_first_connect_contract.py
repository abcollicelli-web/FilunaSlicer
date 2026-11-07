#!/usr/bin/env python3
"""Strict synthetic tests for first-connect+beta status, no OEM printers touched."""
import copy
import hashlib
from pathlib import Path
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"FILUNA"/"tools"))
from beta_first_connect_contract import (
  validate_first_connect,beta_cohort_status,model_key,onboarding_message,
  STATUS_PENDING,STATUS_PARTIAL,STATUS_BETA,STATUS_TEST
)

def h(text="verified"):
    return hashlib.sha256(text.encode()).hexdigest()

def identity(name,manufacturer="anycubic",firmware="1.0"):
    return {"manufacturer":manufacturer,"model":name,"variant":"0.4mm",
            "firmware":firmware,"module_sha256":h(manufacturer+"module"),
            "capabilities_version":"2026-11-07"}

def step(actor,tag):
    return {"actor":actor,"result":"PASS","proof_sha256":h(tag)}

def evidence():
    e={}
    for k in ("identity","profile_integrity","transport","access",
              "machine_state","gcode_reference","safety"):
        e[k]=step("SYSTEM",k)
    e["camera_stream"]=step("SYSTEM","real_camera_stream")
    e["camera_visual_confirmation"]=step("USER","user_saw_camera_local")
    e["slot_inventory"]=step("SYSTEM","OEM_4_slots")
    e["filament_slots_confirmed"]=step("USER","owner_confirmed_colors")
    return e

def sample_job(key,n):
    return {"cohort":key,"environment":"PHYSICAL_BETA",
            "verified_source":"FILUNA_BETA_SERVER","status":"FINISHED_CONFIRMED",
            "all_mandatory_checks_pass":True,
            "user_verified_physical_result":True,
            "critical_incident":False,
            "physical_test_id":"physical-"+str(n),"job_id":"job-"+str(n),
            "printer_pseudonym":"printer-"+str(n),
            "installation_pseudonym":"install-"+str(n),
            "profile_sha256":h("profile"),"gcode_sha256":h("gcode"+str(n)),
            "upload_ack_sha256":h("ack"+str(n)),
            "started_at_epoch":1000+n*100,
            "finished_at_epoch":1100+n*100,
            "stage_chain":["GCODE_GENERATED","UPLOAD_ACK","PRINT_STARTED",
                           "PRINT_FINISHED","USER_PHYSICAL_RESULT"],
            "verified_attestation":"signed-and-server-checked"}
def server_verified(job):
    return job.get("verified_attestation")=="signed-and-server-checked"

class AllManufacturersBetaContractTests(unittest.TestCase):
    def test_all_three_brand_first_connect(self):
        for vendor,model in (("snapmaker","U1"),
                             ("anycubic","Kobra 3 V2"),
                             ("flashforge","AD5X")):
            with self.subTest(vendor=vendor):
                obj=identity(model,manufacturer=vendor)
                result=validate_first_connect(
                    obj,{"camera":True,"multicolor":True,
                         "trusted_vendor_contract":True},evidence())
                self.assertEqual(result["status"],STATUS_TEST)
                self.assertEqual(result["cohort"],model_key(obj))

    def test_system_ports_cannot_be_user_approved(self):
        e=evidence();e["transport"]["actor"]="USER"
        with self.assertRaisesRegex(ValueError,"WRONG_PROOF_ACTOR_transport"):
            validate_first_connect(identity("U1"),{"camera":True,"multicolor":True,
                "trusted_vendor_contract":True},e)

    def test_camera_preview_is_not_user_confirmation(self):
        e=evidence();del e["camera_visual_confirmation"]
        with self.assertRaisesRegex(ValueError,"STEP_MISSING_camera_visual_confirmation"):
            validate_first_connect(identity("U1"),{"camera":True,"multicolor":True,
                "trusted_vendor_contract":True},e)

    def test_multicolor_slot_confirmation_is_mandatory(self):
        e=evidence();e["filament_slots_confirmed"]["result"]="FAIL"
        with self.assertRaisesRegex(ValueError,"STEP_NOT_PASS_filament_slots_confirmed"):
            validate_first_connect(identity("AD5X","flashforge"),
                {"camera":True,"multicolor":True,"trusted_vendor_contract":True},e)

    def test_no_camera_or_multicolor_only_with_verified_oem_contract(self):
        e=evidence()
        result=validate_first_connect(identity("Kobra"),
              {"camera":False,"multicolor":False,"trusted_vendor_contract":True},e)
        self.assertEqual(len(result["capability_na_proven"]),4)
        with self.assertRaisesRegex(ValueError,"OEM_CAPABILITIES_NOT_VERIFIED"):
            validate_first_connect(identity("Kobra"),
              {"camera":False,"multicolor":False,"trusted_vendor_contract":False},e)

    def test_mismatched_proof_sha_rejected(self):
        e=evidence();e["transport"]["proof_sha256"]="not-a-real-hash"
        with self.assertRaisesRegex(ValueError,"EVIDENCE_HASH_MISSING_transport"):
            validate_first_connect(identity("U1"),
              {"camera":True,"multicolor":True,"trusted_vendor_contract":True},e)

    def test_one_then_three_real_physical_prints(self):
        obj=identity("Kobra 3 V2")
        ready=validate_first_connect(obj,{"camera":True,"multicolor":True,
              "trusted_vendor_contract":True},evidence())
        key=model_key(obj)
        s0=beta_cohort_status(obj,[],server_verified)
        self.assertEqual(s0["count"],0)
        self.assertEqual(onboarding_message(ready,s0),
                         "Impressora liberada para teste de impressão")
        s1=beta_cohort_status(obj,[sample_job(key,1)],server_verified)
        self.assertEqual(s1["status"],STATUS_PARTIAL)
        self.assertEqual(onboarding_message(ready,s1),
                         "Impressora parcialmente homologada")
        s2=beta_cohort_status(obj,[sample_job(key,1),sample_job(key,2)],
                              server_verified)
        self.assertEqual(s2["status"],STATUS_PARTIAL)
        s3=beta_cohort_status(obj,[sample_job(key,x) for x in (1,2,3)],
                              server_verified)
        self.assertEqual(s3["count"],3)
        self.assertEqual(s3["status"],STATUS_BETA)
        self.assertEqual(onboarding_message(ready,s3),
                         "Impressora homologada — liberada para impressão")

    def test_three_retransmissions_count_as_one(self):
        obj=identity("U1","snapmaker");key=model_key(obj)
        dup=sample_job(key,1)
        result=beta_cohort_status(obj,[dup,copy.deepcopy(dup),copy.deepcopy(dup)],
                                  server_verified)
        self.assertEqual(result["count"],1)
        self.assertEqual(result["status"],STATUS_PARTIAL)

    def test_finished_without_physical_owner_confirmation_not_counted(self):
        obj=identity("AD5X","flashforge");job=sample_job(model_key(obj),1)
        job["user_verified_physical_result"]=False
        self.assertEqual(beta_cohort_status(obj,[job],server_verified)["count"],0)

    def test_untrusted_client_claims_never_count_without_server_verifier(self):
        obj=identity("U1","snapmaker");data=[sample_job(model_key(obj),i) for i in (1,2,3)]
        self.assertEqual(beta_cohort_status(obj,data,None)["count"],0)
        for item in data:item["verified_attestation"]="client-can-lie"
        self.assertEqual(beta_cohort_status(obj,data,server_verified)["count"],0)

    def test_simulation_not_physical_and_no_ack_not_counted(self):
        obj=identity("Kobra");a=sample_job(model_key(obj),1)
        a["environment"]="SIMULATOR"
        b=sample_job(model_key(obj),2)
        b["stage_chain"].remove("UPLOAD_ACK")
        self.assertEqual(beta_cohort_status(obj,[a,b],server_verified)["count"],0)

    def test_firmware_changes_reopen_model_cohort(self):
        model=identity("Kobra 3 V2",firmware="1.0")
        newer=identity("Kobra 3 V2",firmware="2.0")
        data=[sample_job(model_key(model),n) for n in (1,2,3)]
        self.assertEqual(beta_cohort_status(model,data,server_verified)["count"],3)
        self.assertEqual(beta_cohort_status(newer,data,server_verified)["count"],0)

    def test_failed_local_check_cannot_display_homologation(self):
        obj=identity("Kobra");good=[sample_job(model_key(obj),n) for n in (1,2,3)]
        s=beta_cohort_status(obj,good,server_verified)
        self.assertEqual(onboarding_message({"gate":"FAIL","cohort":model_key(obj)},s),
                         "Validação pendente")

    def test_print_cancelled_or_duplicate_job_id_not_counted(self):
        obj=identity("U1");key=model_key(obj)
        a=sample_job(key,1)
        b=sample_job(key,2);b["job_id"]=a["job_id"]
        c=sample_job(key,3);c["status"]="CANCELLED"
        self.assertEqual(beta_cohort_status(obj,[a,b,c],server_verified)["count"],1)

    def test_verified_critical_incident_revokes_three_previous_passes(self):
        obj=identity("AD5X","flashforge");key=model_key(obj)
        jobs=[sample_job(key,n) for n in (1,2,3)]
        self.assertEqual(beta_cohort_status(obj,jobs,server_verified)["status"],
                         STATUS_BETA)
        incident={"cohort":key,"environment":"PHYSICAL_BETA",
                  "critical_incident":True,
                  "verified_source":"FILUNA_BETA_SERVER",
                  "verified_attestation":"signed-and-server-checked"}
        revoked=beta_cohort_status(obj,jobs+[incident],server_verified)
        self.assertEqual(revoked["count"],0)
        self.assertEqual(revoked["status"],STATUS_PENDING)
        incident["verified_attestation"]="client-can-lie"
        self.assertEqual(beta_cohort_status(obj,jobs+[incident],server_verified)["status"],
                         STATUS_BETA)

if __name__=="__main__":
    unittest.main()
