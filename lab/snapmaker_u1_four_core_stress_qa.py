#!/usr/bin/env python3
"""Strict repeatability of four-tool G-code from the original Snapmaker OEM CLI.

This is an isolated vendor engine QA, NOT Filuna parity, NOT OEM firmware
safety, NOT approval to upload, print, install or release. Input 3MF is
synthetic, created from the Core fixture that the official CLI can import.
Only the exact generated-at timestamp in HEADER_BLOCK may be excluded.
"""
from __future__ import annotations
import argparse
import collections
import hashlib
import json
from pathlib import Path
import re
import sys

from portable_first_qa import OFFICIAL, obtain, extract_executable, digest
from snapmaker_u1_inherit import build_effective_original_presets
from snapmaker_runtime_nozzle_guard_qa import guarded_runtime_process
from snapmaker_u1_guard_probe import run_cli
from snapmaker_u1_fourtool_original_qa import build_minimal_core_3mf, build_four_from_core_3mf
from snapmaker_u1_temporal_ab_qa import header_date_only_normalization
from snapmaker_spatial_gcode_validator import policy_from_yaml, validate_gcode

ROOT=Path(__file__).resolve().parents[1]
ZIP_SHA="1f002e2168c576f800bcfd0509b5e164b5ad65a0963abc2bef8317d929d48c9d"

def execute(cache:Path,out:Path,runs:int)->dict:
    report={"status":"NOT_RUN","scope":"original Snapmaker Orca 2.4.1 Windows CLI synthetic 4-tool stress",
            "model":"Snapmaker U1 (0.4 nozzle)","requested_runs":runs,
            "official_engine_unchanged":True,"official_gcode_unchanged":True,
            "input_is_synthetic":True,"filuna_native_parity":"NOT_RUN",
            "network_ack":"NOT_RUN","physical_beta":"NOT_RUN",
            "firmware_macro_safety":"NOT_RUN","samples":[]}
    try:
        if not 2<=runs<=40:
            raise ValueError("RUNS_OUT_OF_BOUNDS")
        cache.mkdir(parents=True,exist_ok=True)
        out.mkdir(parents=True,exist_ok=True)
        archive,url=obtain(OFFICIAL["snapmaker"],cache)
        if digest(archive)!=ZIP_SHA:
            raise ValueError("ORIGINAL_OEM_ZIP_SHA_MISMATCH")
        executable=extract_executable(archive,OFFICIAL["snapmaker"]["exe"],
                                      cache/"snapmaker-portable-extracted").resolve()
        profile=build_effective_original_presets(out)
        adjusted=out/"qa-process-nozzle-guard.json"
        report["official_archive_sha256"]=digest(archive)
        report["official_download_url"]=url
        report["temporary_process_guard"]=guarded_runtime_process(
            profile["machine"],profile["process"],adjusted)
        single=out/"qa-seed-core-cube.3mf"
        four=out/"qa-synthetic-four-tools.3mf"
        build_minimal_core_3mf(single)
        report["fixture"]=build_four_from_core_3mf(single,four)
        variants={}
        report["arrange_mode"]="--arrange 0"
        for index in range(runs):
            label=f"stress-{index:02d}"
            attempt=run_cli(executable,profile["machine"],adjusted,
                            profile["filament"],four,out,label,timeout=220,arrange=0)
            result={"index":index,"exit_code":attempt["exit_code"],
                    "gcode_count":len(attempt["gcodes"]),
                    "command_log":attempt["log"]}
            if attempt["exit_code"]!=0 or len(attempt["gcodes"])!=1:
                result["status"]="FAIL_VENDOR_CLI"
                report["samples"].append(result)
                report["status"]="FAIL_VENDOR_CLI"
                return report
            file=out/attempt["gcodes"][0]["path"]
            data=file.read_bytes()
            meta=header_date_only_normalization(data)
            defines=[line.decode(errors="replace") for line in data.splitlines()
                     if line.startswith(b"EXCLUDE_OBJECT_DEFINE ")]
            result["exclude_object_define"]=defines
            if len(defines)!=4:
                report["status"]="FAIL_OBJECT_DEFINITIONS_MISSING"
                report["samples"].append(result)
                return report
            tools=sorted({int(m.group(1)) for m in re.finditer(
                rb"(?m)^\s*T([0-3])(?:\s|;|$)",data)})
            changes=len(re.findall(rb"(?m)^;\s*CP TOOLCHANGE START",data))
            result.update({"status":"PASS_FILE_GENERATED",
                           "original_sha256":meta["raw_sha256"],
                           "functional_sha256":meta["normalized_sha256"],
                           "bytes":meta["original_length_bytes"],
                           "lines":meta["lines"],
                           "tool_ids":tools,"toolchange_blocks":changes,
                           "timestamp":meta["timestamp"]})
            report["samples"].append(result)
            if tools!=[0,1,2,3] or changes<3:
                report["status"]="FAIL_FOUR_TOOLS_NOT_PROVEN"
                return report
            if meta["normalized_sha256"] not in variants:
                variants[meta["normalized_sha256"]]=file
        counter=collections.Counter(x["functional_sha256"] for x in report["samples"])
        report["functional_variants"]=dict(counter)
        report["unique_functional_toolpaths"]=len(counter)
        report["actual_completed_runs"]=len(report["samples"])
        report["spatial_by_unique_toolpath"]=[]
        policy=policy_from_yaml(ROOT/"FILUNA/fabricantes/snapmaker/vendor.yaml")
        for sha,file in variants.items():
            risk=validate_gcode(file.read_bytes(),policy,source="official")
            report["spatial_by_unique_toolpath"].append({
                "normalized_sha256":sha,"safety_status":risk["status"],
                "issues":risk["issues"][:5],"warnings":risk["warnings"][:5]})
        report["status"]="PASS_20_OEM_RUNS_VARIANTS_RECORDED"
        return report
    except Exception as exc:
        report["status"]="FAIL_"+type(exc).__name__
        report["error"]=str(exc)
        return report

def main()->int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
    ap.add_argument("--out",type=Path,default=Path("qa-u1-fourcore-stress"))
    ap.add_argument("--runs",type=int,default=20)
    args=ap.parse_args()
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    result=execute(args.cache.resolve(),out,args.runs)
    (out/"stress-proof.json").write_text(
        json.dumps(result,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    print("U1_OFFICIAL_FOUR_CORE_STRESS",result["status"],
          "attempts",len(result["samples"]),
          "variants",result.get("unique_functional_toolpaths","N/A"),flush=True)
    print("NO_FILUNA_PARITY_NO_HARDWARE_NO_PRINT_APPROVAL",flush=True)
    return 0 if result["status"]=="PASS_20_OEM_RUNS_VARIANTS_RECORDED" else 2

if __name__=="__main__":
    sys.exit(main())
