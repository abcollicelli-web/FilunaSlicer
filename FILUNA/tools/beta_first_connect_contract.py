#!/usr/bin/env python3
"""FILUNA beta first-connect QA / three-physical-print validation (data-only).

Reference contract, NOT the connected Filuna C++ DeviceBridge, signed
production service or physical printer. In particular: client claims cannot
be accepted as physical evidence. The server must inject an independently
verifying attestation callback before any beta promotion is possible.
Never issues device commands, opens LAN ports or cameras, or collects media.
"""
from __future__ import annotations

import hashlib
import json
import re
from typing import Callable

REQUIRED_AUTO=("identity","profile_integrity","transport","access",
               "machine_state","gcode_reference","safety")
CAPABILITY_AUTO={"camera":"camera_stream","multicolor":"slot_inventory"}
CAPABILITY_HUMAN={"camera":"camera_visual_confirmation",
                  "multicolor":"filament_slots_confirmed"}
COHORT_FIELDS=("manufacturer","model","variant","firmware","module_sha256",
               "capabilities_version")
VALID_SHA=re.compile(r"^[a-f0-9]{64}$")
STATUS_PENDING="VALIDACAO_PENDENTE"
STATUS_TEST="LIBERADA_PARA_TESTE_DE_IMPRESSAO"
STATUS_PARTIAL="PARCIALMENTE_HOMOLOGADA"
STATUS_BETA="HOMOLOGADA_BETA_LIBERADA_IMPRESSAO"


def digest(data: bytes)->str:
    return hashlib.sha256(data).hexdigest()


def model_key(identity:dict)->str:
    """Model/firmware/module identity, never customer serial/IP/secret."""
    if not isinstance(identity,dict):
        raise ValueError("INVALID_IDENTITY")
    vals=[]
    for field in COHORT_FIELDS:
        val=identity.get(field)
        if not isinstance(val,str) or not val or len(val)>256:
            raise ValueError("MISSING_COHORT_FIELD_"+field)
        vals.append(val)
    if not VALID_SHA.fullmatch(identity["module_sha256"]):
        raise ValueError("INVALID_OEM_MODULE_SHA256")
    return digest(json.dumps(vals,ensure_ascii=False,separators=(",",":")).encode())


def _require_evidence(evidence:dict,step:str,actor:str):
    item=evidence.get(step)
    if not isinstance(item,dict):
        raise ValueError("STEP_MISSING_"+step)
    if item.get("actor")!=actor:
        raise ValueError("WRONG_PROOF_ACTOR_"+step)
    if item.get("result")!="PASS":
        raise ValueError("STEP_NOT_PASS_"+step)
    token=item.get("proof_sha256")
    if not isinstance(token,str) or not VALID_SHA.fullmatch(token):
        raise ValueError("EVIDENCE_HASH_MISSING_"+step)


def validate_first_connect(identity:dict,capabilities:dict,
                           evidence:dict)->dict:
    """Fail closed. System-only gates cannot be manually approved.

    Camera needs real local preview proof from the app + owner's confirmation.
    Multimaterial needs actual OEM slot mapping + owner's filament confirmation.
    Unknown capability means PENDING, not N/A or PASS.
    """
    key=model_key(identity)
    if not isinstance(evidence,dict) or not isinstance(capabilities,dict):
        raise ValueError("MISSING_CAPABILITIES_OR_EVIDENCE")
    allowed=("camera","multicolor")
    for name in allowed:
        if not isinstance(capabilities.get(name),bool):
            raise ValueError("UNVERIFIED_CAPABILITY_"+name)
    signed_capabilities=capabilities.get("trusted_vendor_contract")
    if signed_capabilities is not True:
        raise ValueError("OEM_CAPABILITIES_NOT_VERIFIED")
    for step in REQUIRED_AUTO:
        _require_evidence(evidence,step,"SYSTEM")
    na=[]
    for name in allowed:
        if capabilities[name]:
            _require_evidence(evidence,CAPABILITY_AUTO[name],"SYSTEM")
            _require_evidence(evidence,CAPABILITY_HUMAN[name],"USER")
        else:
            # Must have vendor capability proof, not a client's "N/A" click.
            na.extend((CAPABILITY_AUTO[name],CAPABILITY_HUMAN[name]))
    # This is a local test readiness decision only, not permission to print.
    return {"status":STATUS_TEST,"cohort":key,"gate":"FIRST_CONNECT_PASS",
            "not_app_homologation":True,"capability_na_proven":na,
            "client_authorizes_any_print_separately":True}


def beta_cohort_status(identity:dict,tests:list[dict],
                       verify_server_attestation:Callable[[dict],bool]|None)->dict:
    """Only 3 independent REAL beta physical jobs may promote a model cohort.

    No attestation verifier = zero physical tests, regardless of client claims.
    Verifier is mandatory backend trust boundary; no unsigned local JSON may
    promote. Duplicate physical ID, job ID or retransmission is not counted.
    """
    key=model_key(identity)
    if not callable(verify_server_attestation):
        return {"cohort":key,"count":0,"status":STATUS_PENDING,
                "reason":"SERVER_ATTESTATION_VERIFIER_NOT_CONNECTED"}
    seen_test=set();seen_job=set();approved=[]
    for test in tests:
        if not isinstance(test,dict):
            continue
        if test.get("cohort")!=key or test.get("environment")!="PHYSICAL_BETA":
            continue
        # A confirmed critical incident automatically revokes the beta model
        # status. Client-submitted allegations cannot trigger this gate alone.
        if (test.get("critical_incident") is True and
                test.get("verified_source")=="FILUNA_BETA_SERVER" and
                verify_server_attestation(test) is True):
            return {"cohort":key,"count":0,"status":STATUS_PENDING,
                    "reason":"VERIFIED_CRITICAL_INCIDENT_REOPENS_BETA_GATES",
                    "requires_fresh_per_printer_first_connect":True,
                    "not_authorization_for_stable_release":True}
        if (test.get("verified_source")!="FILUNA_BETA_SERVER" or
                test.get("status")!="FINISHED_CONFIRMED" or
                test.get("all_mandatory_checks_pass") is not True or
                test.get("user_verified_physical_result") is not True or
                test.get("critical_incident") is not False):
            continue
        keys=("physical_test_id","job_id","printer_pseudonym","installation_pseudonym",
              "profile_sha256","gcode_sha256","upload_ack_sha256")
        if any(not isinstance(test.get(field),str) or not test[field] for field in keys):
            continue
        if any(not VALID_SHA.fullmatch(test[k]) for k in
               ("profile_sha256","gcode_sha256","upload_ack_sha256")):
            continue
        if (test["physical_test_id"] in seen_test or test["job_id"] in seen_job):
            continue
        stages=test.get("stage_chain")
        if not isinstance(stages,list) or stages!=[
                "GCODE_GENERATED","UPLOAD_ACK","PRINT_STARTED",
                "PRINT_FINISHED","USER_PHYSICAL_RESULT"]:
            continue
        begin=test.get("started_at_epoch")
        end=test.get("finished_at_epoch")
        if not isinstance(begin,(int,float)) or not isinstance(end,(int,float)) or end<=begin:
            continue
        if verify_server_attestation(test) is not True:
            continue
        seen_test.add(test["physical_test_id"])
        seen_job.add(test["job_id"])
        approved.append(test["physical_test_id"])
    count=len(approved)
    status=STATUS_BETA if count>=3 else STATUS_PARTIAL if count>=1 else STATUS_PENDING
    return {"cohort":key,"count":count,"status":status,
            "requires_fresh_per_printer_first_connect":True,
            "not_authorization_for_stable_release":True,
            "proofs":approved[:3]}


def onboarding_message(first_connect:dict,cohort:dict)->str:
    """Do not advertise a model to a new printer with failed local checks."""
    if first_connect.get("gate")!="FIRST_CONNECT_PASS":
        return "Validação pendente"
    if first_connect.get("cohort")!=cohort.get("cohort"):
        return "Validação pendente"
    status=cohort.get("status")
    if status==STATUS_BETA:
        return "Impressora homologada — liberada para impressão"
    if status==STATUS_PARTIAL:
        return "Impressora parcialmente homologada"
    return "Impressora liberada para teste de impressão"
