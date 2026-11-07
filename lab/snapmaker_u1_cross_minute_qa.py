#!/usr/bin/env python3
"""Snapmaker official portable CLI: cross-minute non-timestamp G-code regression.

The source, SHA256-pinned original exe and all presets stay untouched.
Only a *temporary* runtime process copy adds the selected original machine's
nozzle diameter, to avoid the confirmed v2.4.1 load-time null access.

Two separate CLI processes run on either side of a wall-clock minute boundary.
No normalization beyond the exact HEADER_BLOCK generated-at line is permitted.
Any toolpath, purge, retract, M73 or macro difference is a QA failure.
"""
from __future__ import annotations
import argparse
import difflib
import json
from pathlib import Path
import time

from portable_first_qa import OFFICIAL,digest,obtain,extract_executable
from snapmaker_u1_inherit import build_effective_original_presets
from snapmaker_u1_guard_probe import run_cli
from snapmaker_runtime_nozzle_guard_qa import guarded_runtime_process
from snapmaker_u1_temporal_ab_qa import header_date_only_normalization, compare_temporally_separated

ROOT=Path(__file__).resolve().parents[1]


def main():
    p=argparse.ArgumentParser()
    p.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
    p.add_argument("--out",type=Path,default=Path("qa-snapmaker-u1-cross-minute"))
    args=p.parse_args()
    out=args.out.resolve();cache=args.cache.resolve()
    out.mkdir(parents=True,exist_ok=True);cache.mkdir(parents=True,exist_ok=True)
    rows=[]
    def add(stage,status,details,evidence=""):
        assert status in ("PASS","FAIL","PENDING")
        rows.append(dict(gate=stage,status=status,details=details,evidence=evidence))
        print("U1_CROSS_MINUTE",stage,status,details,flush=True)
    try:
        cfg=OFFICIAL["snapmaker"]
        archive,url=obtain(cfg,cache)
        exe=extract_executable(archive,cfg["exe"],
                               cache/"snapmaker-portable-extracted").resolve()
        add("ORIGINAL_ZIP","PASS",digest(archive),url)
        effective=build_effective_original_presets(out)
        dest=out/"runtime-only-process.json"
        metadata=guarded_runtime_process(effective["machine"],effective["process"],dest)
        (out/"origin.lock.json").write_text(json.dumps(metadata,indent=2)+"\n",encoding="utf-8")
        add("CONFIG_PROVENANCE","PASS","original source unmodified; model-specific nozzles from machine")
        gcodes=[]
        for letter in ("A","B"):
            if letter=="B":
                # Wait for a DIFFERENT minute, not merely a different second.
                # This isolates a suspected time-dependent segmentation/pattern.
                delay=62-(time.time()%60)
                add("MINUTE_BOUNDARY","PASS","wait="+str(round(delay,3))+" seconds between processes")
                time.sleep(delay)
            result=run_cli(exe,effective["machine"],dest,effective["filament"],
                           ROOT/"fixtures/cube20.stl",out,"cross-minute-"+letter)
            if result["exit_code"]!=0 or len(result["gcodes"])!=1:
                add("OFFICIAL_"+letter,"FAIL","exit="+str(result["exit_code"])
                    +" gcode_count="+str(len(result["gcodes"])),result["log"])
                break
            path=out/result["gcodes"][0]["path"]
            info=header_date_only_normalization(path.read_bytes())
            gcodes.append((path,info))
            add("OFFICIAL_"+letter,"PASS","bytes="+str(path.stat().st_size)
                +" header="+info["timestamp"]
                +" raw_sha="+info["raw_sha256"],
                str(path.relative_to(out)))
        if len(gcodes)==2:
            a,b=gcodes
            ta=a[1]["timestamp"];tb=b[1]["timestamp"]
            if ta[:16]==tb[:16]:
                add("DISTINCT_MINUTE_CONTROL","FAIL","same minute from generated-at header")
            else:
                add("DISTINCT_MINUTE_CONTROL","PASS",ta+" / "+tb)
                diffmeta={"A":a[1],"B":b[1],
                          "only_timestamp_normalization_permitted":True,
                          "different_minute_verified":True}
                try:
                    proof=compare_temporally_separated(a[0].read_bytes(),b[0].read_bytes())
                    diffmeta["result"]="PASS"
                    diffmeta["proof"]=proof
                    add("CROSS_MINUTE_GCODE","PASS",
                        "only HEADER_BLOCK timestamp changed; normalized_sha="+
                        proof["A"]["normalized_sha256"])
                except ValueError as error:
                    original_a=a[0].read_bytes().decode("utf-8","replace").splitlines()
                    original_b=b[0].read_bytes().decode("utf-8","replace").splitlines()
                    lines=list(difflib.unified_diff(original_a,original_b,
                        fromfile="A",tofile="B",lineterm="",n=1))
                    (out/"cross-minute-unapproved-gcode-diff.txt").write_text(
                        "\n".join(lines[:1300])+"\n",encoding="utf-8")
                    diffmeta["result"]="FAIL"
                    diffmeta["exception"]=str(error)
                    diffmeta["diff_lines"]=len(lines)
                    diffmeta["diff_excerpt_file"]="cross-minute-unapproved-gcode-diff.txt"
                    add("CROSS_MINUTE_GCODE","FAIL",
                        str(error)+" diff_lines="+str(len(lines)),
                        "cross-minute-unapproved-gcode-diff.txt")
                (out/"cross-minute-proof.json").write_text(
                    json.dumps(diffmeta,ensure_ascii=False,indent=2)+"\n")
        add("ORIGINAL_CLI_AB","PENDING","original unmodified process still crashes")
        add("FILUNA_NATIVE_PARITY","PENDING","no Filuna integrated U1 motor G-code")
        add("NETWORK_ACCESS_CONTROL","PENDING","no Snapmaker U1 native SDK bridge QA")
        add("PHYSICAL_BETA","PENDING","after all internal gates and final Filuna build")
    except Exception as exc:
        add("RUNNER","FAIL",type(exc).__name__+": "+str(exc))
    (out/"results.json").write_text(json.dumps(rows,indent=2,ensure_ascii=False)+"\n")
    (out/"RESUMO.md").write_text(
        "# Snapmaker U1 original portable - cross-minute CLI session reproducibility\n\n"
        "No non-timestamp G-code changes may be normalized or hidden.\n\n"
        "| Gate | State | Details |\n|---|---|---|\n"
        +"".join("| "+x["gate"]+" | "+x["status"]+" | "+x["details"].replace("|","/")+" |\n"
                 for x in rows),encoding="utf-8")
    return 1 if any(x["status"]=="FAIL" for x in rows) else 0

if __name__=="__main__":
    raise SystemExit(main())
