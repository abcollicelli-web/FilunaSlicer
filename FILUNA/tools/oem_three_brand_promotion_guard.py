#!/usr/bin/env python3
"""Fail-closed source/manifest promotion guard for the three OEM pilot brands.

Purpose: PREVENT false internal homologation or real-printer activation when
the repo still has OEM adapter placeholders, no Filuna native G-code,
unreviewed dependencies or a documented G-code safety blocker.

A "PASS_GUARD" result is NOT "PASS_INTERNAL_HOMOLOGATION".
No OEM binary is installed, compiled, linked, shipped or used.
This three-brand gate is a QA pilot, not an exclusion list for Filuna's
universal model/profile catalog.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path

PILOT_BRANDS=("snapmaker","anycubic","flashforge")
APPROVAL_TOKENS=("HOMOLOGADO", "HOMOLOGATED", "APPROVED", "RELEASED",
                 "PASS_INTERNO", "PASS_INTERNAL", "FULLY_VALIDATED",
                 "PASS_NATIVE", "ENABLED_FOR_PRINT", "PHYSICAL_HOMOLOGATED")
BLOCK_WORDS=("BLOCKED", "NOT_RUN", "NOT_INTEGRATED", "PENDING", "FAIL")


def _promoted(value):
    if not isinstance(value,str):
        return False
    s=value.upper()
    if any(w in s for w in BLOCK_WORDS):
        return False
    return any(x in s for x in APPROVAL_TOKENS) or s in {
        "PASS","SUCCESS","COMPLETE","SUPPORTED","READY"
    }


def inspect_brand(repo:Path,brand:str)->dict:
    if brand not in PILOT_BRANDS:
        raise ValueError("OUTSIDE_THREE_BRAND_QA_PILOT")
    manifest=repo/"FILUNA"/"fabricantes"/brand/"manifest.json"
    adapter=repo/"fabricantes"/brand/"adapter"/(brand+"_adapter.cpp")
    if manifest.is_symlink() or adapter.is_symlink():
        raise ValueError("UNSAFE_MANIFEST_OR_ADAPTER_SYMLINK")
    state={"brand":brand,"evidence_kind":"STATIC_SOURCE_GUARD_ONLY",
           "native_filuna_engine_tested":False,
           "real_device_connected":False,
           "errors":[]}
    if not manifest.is_file() or not adapter.is_file():
        state["errors"].append("MISSING_MANIFEST_OR_NATIVE_ADAPTER")
        return state
    m=json.loads(manifest.read_text(encoding="utf-8"))
    cpp=adapter.read_text(encoding="utf-8")
    qa=m.get("qa",{})
    state["manifest_internal_status"]=qa.get("internal_homologation")
    state["stub_adapter"]="PendingVendorAdapter" in cpp or "native OEM SDK linkage NOT YET IMPLEMENTED" in cpp
    state["physical_activation"]=m.get("automatic_hardware_activation")
    state["native_slicing"]=qa.get("filuna_native_slicing",qa.get("u1_filuna_native_gcode_parity"))
    state["native_network"]=qa.get("filuna_native_network",qa.get("u1_network_protocol"))
    state["source_licence_or_dependency_review"]=qa.get("full_oem_dependencies_review",qa.get("source_review_complete"))
    if m.get("manufacturer_id")!=brand:
        state["errors"].append("BRAND_ID_MISMATCH")
    model=m.get("model_catalog",m.get("model_discovery",{}))
    if model.get("fixed_models_whitelist") is True or model.get("not_a_homologated_model_whitelist") is False:
        state["errors"].append("QA_SAMPLES_WRONGLY_SET_AS_SUPPORTED_MODEL_WHITELIST")
    if m.get("automatic_hardware_activation") is True:
        state["errors"].append("UNAUTHORIZED_AUTOMATIC_HARDWARE_ACTIVATION")
    if state["stub_adapter"] and _promoted(qa.get("internal_homologation")):
        state["errors"].append("FALSE_HOMOLOGATION_WITH_PENDING_NATIVE_CPP_ADAPTER")
    if _promoted(m.get("state")) and state["stub_adapter"]:
        state["errors"].append("FALSE_MODULE_STATUS_WITH_PENDING_NATIVE_CPP_ADAPTER")
    if _promoted(qa.get("internal_homologation")):
        if state["stub_adapter"]:
            state["errors"].append("REAL_NATIVE_CPP_ADAPTER_MISSING")
        if not _promoted(state["native_slicing"]):
            state["errors"].append("REAL_FILUNA_CPP_ENGINE_GCODE_PARITY_MISSING")
        if not _promoted(state["native_network"]):
            state["errors"].append("REAL_FILUNA_CPP_NETWORK_AND_ACK_MISSING")
        if not _promoted(state["source_licence_or_dependency_review"]):
            state["errors"].append("FULL_OEM_DEPENDENCIES_LICENSE_REVIEW_MISSING")
    if brand=="snapmaker":
        safety=qa.get("u1_final_purge_safety_regression",{})
        stress=qa.get("u1_original_240_portable_regression_reference",{})
        if _promoted(qa.get("internal_homologation")) and (
            safety.get("simulated_firmware_gcode_safety")!="PASS" or
            stress.get("result","").startswith("FAIL") or
            qa.get("u1_original_cli_40_run_stress","").startswith("FAIL")):
            state["errors"].append("U1_REAL_TOOLPATH_AND_ZHOP_SAFETY_NOT_CLEARED")
    for field in ("physical_beta","physical_homologation"):
        if _promoted(qa.get(field)):
            state["errors"].append("PHYSICAL_PRINT_CANNOT_BE_PROVED_BY_STATIC_QA")
    # This tool must not itself label the maker validated: it checks only that
    # the known blockers are correctly retained and not hidden by a PASS label.
    state["guard_status"]="PASS_NO_FALSE_PROMOTION" if not state["errors"] else "FAIL_UNSAFE_PROMOTION_OR_METADATA"
    state["internal_homologation_proven_here"]=False
    return state


def assess(repo:Path)->dict:
    cases=[inspect_brand(repo,x) for x in PILOT_BRANDS]
    return {"gate":"THREE_OEM_PROMOTION_GUARD_SOURCE_ONLY",
            "status":"PASS_GUARD_NO_FALSE_HOMOLOGATION" if all(
                x["guard_status"].startswith("PASS") for x in cases)
                else "FAIL_GUARD",
            "native_homologated_by_this_tool":[],
            "cases":cases,
            "scope":"metadata, native adapter implementation presence, recorded safety blockers; never an actual Filuna print or device test"}


def main():
    p=argparse.ArgumentParser()
    p.add_argument("--repo",type=Path,default=Path(__file__).resolve().parents[2])
    p.add_argument("--out",type=Path)
    args=p.parse_args()
    result=assess(args.repo.resolve())
    if args.out:
        args.out.parent.mkdir(parents=True,exist_ok=True)
        args.out.write_text(json.dumps(result,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    print(json.dumps(result,ensure_ascii=False,indent=2))
    return 0 if result["status"].startswith("PASS_GUARD_") else 1


if __name__=="__main__":
    raise SystemExit(main())
