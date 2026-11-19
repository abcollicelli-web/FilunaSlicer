#!/usr/bin/env python3
"""Snapmaker official U1 2.4.1 4×20 independent CLI controlled-factor experiment.

Diagnose functional differences around seam and wipe. Experimental preset
overrides are NEVER official profile parity and NEVER promoted to Filuna
production automatically, regardless of repeatability result.

Group A: true OEM effective configuration + only proven null-access load guard.
Group B: disable wipe for all nozzle slots, same rest of OEM profile.
Group C: seam_position nearest instead of aligned, same rest.
Group D: B and C together, to test interaction.
G-code changes BETWEEN groups are EXPECTED; each group must be internally
stable (same normalized toolpath over 20 original OEM runs) for a diagnosis.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path

from portable_first_qa import OFFICIAL,obtain,extract_executable,digest
from snapmaker_u1_inherit import build_effective_original_presets
from snapmaker_runtime_nozzle_guard_qa import guarded_runtime_process
from snapmaker_u1_guard_probe import run_cli
from snapmaker_u1_temporal_ab_qa import header_date_only_normalization

ROOT=Path(__file__).resolve().parents[1]


def clone_json(p:Path,out:Path,mutations:dict)->Path:
    before=json.loads(p.read_text(encoding="utf-8-sig"))
    if not isinstance(before,dict):raise ValueError("SOURCE_CONFIG_NOT_JSON_OBJECT")
    modified=json.loads(json.dumps(before))
    for key,(expected,replacement) in mutations.items():
        if modified.get(key)!=expected:
            raise ValueError("OEM_PRESET_UNEXPECTED_INITIAL_VALUE_"+key)
        modified[key]=replacement
    original_keys=set(before)
    if set(modified)!=original_keys:
        raise ValueError("MUTATION_ADDED_OR_REMOVED_PROFILE_KEYS")
    diffs={k for k in original_keys if before[k]!=modified[k]}
    if diffs!=set(mutations):
        raise ValueError("NOT_EXACTLY_REQUESTED_MUTATION")
    out.write_text(json.dumps(modified,indent=2,ensure_ascii=False)+"\n",
                   encoding="utf-8")
    return out


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
    parser.add_argument("--out",type=Path,default=Path("qa-snapmaker-u1-seam-wipe-factors"))
    parser.add_argument("--runs-per-group",type=int,default=20)
    a=parser.parse_args()
    if not 2<=a.runs_per_group<=50:raise ValueError("INVALID_GROUP_SIZE")
    cache=a.cache.resolve();out=a.out.resolve()
    cache.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
    summary={"experiment":"Four factor combinations seam_position/wipe",
             "input_vendor":"original official Snapmaker Orca v2.4.1",
             "not_production_settings":True,
             "not_filuna_native_homologation":True,
             "cases":{}}
    try:
        archive,url=obtain(OFFICIAL["snapmaker"],cache)
        exe=extract_executable(archive,OFFICIAL["snapmaker"]["exe"],
                               cache/"snapmaker-portable-extracted").resolve()
        original=build_effective_original_presets(out)
        p0=out/"effective-OEM-with-nozzle-null-guard.json"
        provenance=guarded_runtime_process(original["machine"],original["process"],p0)
        m0=original["machine"]
        m1=clone_json(m0,out/"machine-diagnostic-wipe-off.json",
                      {"wipe":(["1"]*4,["0"]*4)})
        p1=clone_json(p0,out/"process-diagnostic-seam-nearest.json",
                      {"seam_position":("aligned","nearest")})
        groups={
            "original-baseline":(m0,p0),
            "wipe-off-only":(m1,p0),
            "nearest-seam-only":(m0,p1),
            "wipe-off-and-nearest":(m1,p1)}
        summary["original_vendor_zip_sha256"]=digest(archive)
        summary["original_profile_hashes"]={k:digest(x) for k,x in original.items()}
        summary["guard_provenance"]=provenance
        summary["exact_experimental_mutations"]={
            "wipe-off-only":{"machine.wipe":["1"]*4,"diagnostic_replacement":["0"]*4},
            "nearest-seam-only":{"process.seam_position":"aligned","diagnostic_replacement":"nearest"}}
        ran_all=True
        for group,(m,p) in groups.items():
            rows=[]
            for index in range(a.runs_per_group):
                label=group+"-"+str(index).zfill(2)
                result=run_cli(exe,m,p,original["filament"],
                               ROOT/"fixtures/cube20.stl",out,label)
                if result["exit_code"]!=0 or len(result["gcodes"])!=1:
                    rows.append({"case":label,"error":"OEM_CLI_NO_SINGLE_GCODE",
                                 "exit":result["exit_code"],"count":len(result["gcodes"])})
                    ran_all=False
                    break
                g=out/result["gcodes"][0]["path"]
                h=header_date_only_normalization(g.read_bytes())
                row={"index":index,"model":"cube20","toolpath_sha256":h["normalized_sha256"],
                     "source_gcode_sha256":h["raw_sha256"],"bytes":g.stat().st_size,
                     "gcode":str(g.relative_to(out))}
                rows.append(row)
                print("SNAPMAKER_SEAM_WIPE",group,index,row["bytes"],
                      row["toolpath_sha256"],flush=True)
            counts={}
            for row in rows:
                h=row.get("toolpath_sha256")
                if h:counts[h]=counts.get(h,0)+1
            status="PASS_EXPERIMENTAL_INTERNAL_REPEATABILITY" if (
                len(rows)==a.runs_per_group and len(counts)==1
                and all("toolpath_sha256" in r for r in rows)) else "FAIL_GCODE_VARIANCE_OR_CLI"
            summary["cases"][group]={"status":status,"runs":len(rows),
                                     "toolpath_hashes":counts,"records":rows,
                                     "profile_sha256":{"machine":digest(m),
                                                       "process":digest(p),
                                                       "filament":digest(original["filament"])}}
            print("SNAPMAKER_SEAM_WIPE GROUP",group,status,
                  "unique_hashes="+str(len(counts)),flush=True)
        # FAIL if *any* group differs: the vendor original is not certified
        # deterministic by one successful experimental configuration.
        summary["result"]="PASS_ALL_FOUR_EXPERIMENTAL_GROUPS" if (
            ran_all and all(x["status"].startswith("PASS") for x in summary["cases"].values())
        ) else "FAIL_ONE_OR_MORE_OFFICIAL_OR_EXPERIMENTAL_GROUPS"
    except Exception as e:
        summary["result"]="FAIL_RUNNER"
        summary["exception"]=repr(e)
    (out/"result.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False)+"\n")
    (out/"RESUMO.md").write_text("# U1 OEM original, seam/wipe controlled factors\n\n"
        "**"+summary["result"]+"**. No OEM/Filuna compiled binary, no physical tests; "
        "experimental factor groups not authorized as production profiles.\n\n"+
        "| Group | QA repeated results | Distinct G-code toolpaths |\n|---|---|---|\n"+
        "".join("| "+name+" | "+x["status"]+" | "+str(len(x["toolpath_hashes"]))+" |\n"
                for name,x in summary["cases"].items()),encoding="utf-8")
    print("SNAPMAKER_SEAM_WIPE RESULT",summary["result"],flush=True)
    return 0 if summary["result"].startswith("PASS_") else 1


if __name__=="__main__":
    raise SystemExit(main())
