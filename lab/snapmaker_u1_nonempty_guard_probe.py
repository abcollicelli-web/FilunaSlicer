#!/usr/bin/env python3
"""Snapmaker U1 official v2.4.1 process vector diagnosis, non-empty value.

The official upstream snapshots stay immutable; ONLY a temporary effective
process copy gets one scalar field with a nonempty vector. This is a controlled
hypothesis test, not an OEM parity pass nor a native Filuna module implementation.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import shutil
from portable_first_qa import OFFICIAL, digest, obtain, extract_executable
from snapmaker_u1_inherit import build_effective_original_presets
from snapmaker_u1_guard_probe import run_cli

ROOT=Path(__file__).resolve().parents[1]
CANDIDATES = [
    ("layer-height", ["layer_height"]),
    ("perimeter", ["perimeter_speed"]),
    ("two-values", ["layer_height", "perimeter_speed"]),
]

def patch_one_field(original:Path,destination:Path,values:list[str]):
    if not values or any(not x.strip() for x in values):
        raise ValueError("GUARD_VECTOR_MUST_BE_NONEMPTY")
    source=json.loads(original.read_text(encoding="utf-8-sig"))
    if source.get("type")!="process":
        raise ValueError("NOT_OFFICIAL_EFFECTIVE_PROCESS")
    if source.get("different_settings_to_system") is not None:
        raise ValueError("CONTROL_PROCESS_ALREADY_HAS_DIAGNOSTIC_OPTION")
    new=dict(source)
    new["different_settings_to_system"]=list(values)
    destination.write_text(json.dumps(new,indent=2,ensure_ascii=False)+"\n",
                           encoding="utf-8")
    check=json.loads(destination.read_text(encoding="utf-8"))
    check.pop("different_settings_to_system")
    if check!=source:
        raise ValueError("DIAGNOSTIC_CHANGED_MORE_THAN_ONE_FIELD")
    return {"original_sha256":digest(original),
            "diagnostic_sha256":digest(destination),
            "field":"different_settings_to_system",
            "one_field_only":True,"nonempty_values":values,
            "source_oem_preserved":True}

def execute():
    p=argparse.ArgumentParser()
    p.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
    p.add_argument("--out",type=Path,default=Path("qa-snapmaker-u1-nonempty-vector"))
    args=p.parse_args()
    cache=args.cache.resolve();out=args.out.resolve()
    cache.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
    rows=[];proof={}
    def add(stage,status,details,evidence=""):
        assert status in ("PASS","FAIL","PENDING")
        rows.append(dict(stage=stage,status=status,details=details,evidence=evidence))
        print("U1_NONEMPTY",stage,status,details,flush=True)
    try:
        cfg=OFFICIAL["snapmaker"]
        archive,url=obtain(cfg,cache)
        exe=extract_executable(archive,cfg["exe"],
                               cache/"snapmaker-portable-extracted").resolve()
        add("OFFICIAL_PINNED_BINARY","PASS","sha256="+digest(archive),url)
        effective=build_effective_original_presets(out)
        add("OFFICIAL_INHERITANCE","PASS","12 original Git objects verified",
            "effective-profile-derivation.json")
        original=effective["process"]
        cube=(ROOT/"fixtures/cube20.stl").resolve()
        if not cube.exists():raise ValueError("CUBE_MISSING")
        control=run_cli(exe,effective["machine"],original,effective["filament"],
                        cube,out,"original-control")
        proof["original_control"]=control
        add("ORIGINAL_CONTROL",
            "PASS" if control["access_violation"] and not control["gcodes"] else "PENDING",
            "exit="+str(control["exit_code"])+" crashes="+str(control["access_violation"]),
            control["log"])
        candidate_success=None
        for label,values in CANDIDATES:
            process=out/("diagnostic-process-"+label+".json")
            lock=patch_one_field(original,process,values)
            (out/("diagnostic-"+label+".lock.json")).write_text(
                json.dumps(lock,indent=2)+"\n",encoding="utf-8")
            outcome=run_cli(exe,effective["machine"],process,effective["filament"],
                            cube,out,"diagnostic-"+label+"-A")
            proof[label]=outcome
            if outcome["exit_code"]==0 and len(outcome["gcodes"])==1:
                add("DIAGNOSTIC_"+label,"PASS",
                    "diagnostic-only generated G-code "+outcome["gcodes"][0]["sha256"],
                    outcome["log"])
                candidate_success=(label,process,outcome)
                break
            add("DIAGNOSTIC_"+label,"PENDING",
                "exit="+str(outcome["exit_code"])+" access_violation="+str(outcome["access_violation"]),
                outcome["log"])
        if candidate_success:
            label,process,first=candidate_success
            second=run_cli(exe,effective["machine"],process,effective["filament"],
                           cube,out,"diagnostic-"+label+"-B")
            proof["diagnostic_second"]=second
            if (second["exit_code"]==0 and len(second["gcodes"])==1
                and first["gcodes"][0]["path"]!=second["gcodes"][0]["path"]
                and first["gcodes"][0]["sha256"]==second["gcodes"][0]["sha256"]):
                add("MODIFIED_COPY_A_B","PASS","raw same SHA256 "+second["gcodes"][0]["sha256"])
            else:
                add("MODIFIED_COPY_A_B","PENDING",
                    "cannot verify 2 independent matching diagnostic outputs")
        else:
            add("MODIFIED_COPY_A_B","PENDING","no candidate created one valid G-code")
        add("UNMODIFIED_OFFICIAL_AB","PENDING",
            "Original OEM process fails on standalone CLI; never promote diagnostic copy")
        add("FILUNA_NATIVE_PARITY","PENDING","no separate Filuna Slicer G-code")
        add("SNAPMAKER_NETWORK","PENDING","no U1 simulator/SDK integration contract proof")
        add("PHYSICAL_BETA","PENDING","only after internal gates and final Filuna build")
    except Exception as e:
        add("RUNNER","FAIL",type(e).__name__+": "+str(e))
    (out/"results.json").write_text(json.dumps(rows,indent=2)+"\n",encoding="utf-8")
    (out/"proof.json").write_text(json.dumps(proof,indent=2)+"\n",encoding="utf-8")
    (out/"RESUMO.md").write_text(
        "# Snapmaker U1 — nonempty process vector controlled fault isolation\n\n"
        "No unmodified OEM profiles, engine source or binary changed. This is "
        "a diagnostic-only copy and **not homologation**.\n\n"
        "| Gate | Status | Details |\n|---|---|---|\n"
        +"".join("| "+r["stage"]+" | "+r["status"]+" | "+
                 r["details"].replace("|","/")+" |\n" for r in rows),
        encoding="utf-8")
    return 1 if any(r["status"]=="FAIL" for r in rows) else 0

if __name__=="__main__":
    raise SystemExit(execute())
