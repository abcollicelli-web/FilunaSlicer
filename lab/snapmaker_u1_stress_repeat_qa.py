#!/usr/bin/env python3
"""Snapmaker U1 original portable controlled repeatability across 40 CLI processes.

Stress test for previously observed 249098-vs-250183 G-code toolpath drift.
Entire OEM source, binary ZIP, original machine/process/filament blobs are pinned;
only a temporary process nozzle-diameter guard is used, identical for ALL runs.
No OEM compilation, Filuna native integration, network or physical print.
"""
from __future__ import annotations

import argparse
import difflib
import json
from pathlib import Path
import random
import shutil

from portable_first_qa import OFFICIAL,obtain,extract_executable,digest
from snapmaker_u1_inherit import build_effective_original_presets
from snapmaker_runtime_nozzle_guard_qa import guarded_runtime_process
from snapmaker_u1_guard_probe import run_cli
from snapmaker_u1_temporal_ab_qa import header_date_only_normalization

ROOT=Path(__file__).resolve().parents[1]


def main():
    p=argparse.ArgumentParser()
    p.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
    p.add_argument("--out",type=Path,default=Path("qa-snapmaker-u1-40-run-stress"))
    a=p.parse_args()
    out=a.out.resolve();cache=a.cache.resolve()
    out.mkdir(parents=True,exist_ok=True);cache.mkdir(parents=True,exist_ok=True)
    records=[];final="FAIL_NOT_RUN"
    try:
        cfg=OFFICIAL["snapmaker"]
        archive,url=obtain(cfg,cache)
        exe=extract_executable(archive,cfg["exe"],cache/"snapmaker-portable-extracted").resolve()
        files=build_effective_original_presets(out)
        proc=out/"guarded-source-process.json"
        guard=guarded_runtime_process(files["machine"],files["process"],proc)
        palt=out/"process_alias.json";falt=out/"filament_alias.json"
        shutil.copyfile(proc,palt);shutil.copyfile(files["filament"],falt)
        assert digest(proc)==digest(palt)
        assert digest(files["filament"])==digest(falt)
        inputs={
            "base":(files["machine"],proc,files["filament"]),
            "process-alias":(files["machine"],palt,files["filament"]),
            "filament-alias":(files["machine"],proc,falt),
            "machine-alias":(files["machine"],proc,files["filament"])
        }
        sequence=[x for x in inputs for _ in range(10)]
        random.Random(20261107).shuffle(sequence)
        out_rows={"source_vendor": "Snapmaker","version":"2.4.1","source_original_zip_sha256":digest(archive),
                  "runtime_guard":guard,"number_of_independent_cli_processes":len(sequence),
                  "environment":"windows-2022 GitHub Actions, sequential, immutable pinned vendor",
                  "scope":"source-original CLI with workaround, NOT native Filuna parity"}
        for i,label in enumerate(sequence):
            machine,process,filament=inputs[label]
            run=run_cli(exe,machine,process,filament,
                        ROOT/"fixtures"/"cube20.stl",out,"stress-"+str(i).zfill(2))
            row={"sequence":i,"selected_case":label,"exit_code":run["exit_code"],
                 "gcode_count":len(run["gcodes"]),"log":run["log"]}
            if run["exit_code"] or len(run["gcodes"])!=1:
                records.append(row)
                print("SNAPMAKER_STRESS FAIL_CLI",i,label,run["exit_code"],flush=True)
                break
            gc=out/run["gcodes"][0]["path"]
            sha=header_date_only_normalization(gc.read_bytes())
            row.update({"gcode_path":str(gc.relative_to(out)), "bytes":gc.stat().st_size,
                        "raw_sha256":sha["raw_sha256"],
                        "normalized_sha256":sha["normalized_sha256"],
                        "generated_at":sha["timestamp"]})
            records.append(row)
            print("SNAPMAKER_STRESS RUN",i,label,
                  row["bytes"],row["normalized_sha256"],flush=True)
        unique={r.get("normalized_sha256") for r in records if r.get("normalized_sha256")}
        total=len(records)
        if total==len(sequence) and len(unique)==1:
            final="PASS_ALL_40_CLI_PROCESSES_STRICT_HEADER_ONLY"
        elif total<len(sequence):
            final="FAIL_CLI_RUN_STOPPED_EARLY"
        else:
            final="FAIL_MULTIPLE_NON_TIMESTAMP_TOOLPATH_VARIANTS"
        out_rows.update({"result":final,"executed":total,"toolpath_hashes":sorted(unique),
                         "counts":{sha:sum(r.get("normalized_sha256")==sha for r in records)
                                   for sha in unique},
                         "observations":records})
        if len(unique)>1:
            bysha={}
            for row in records:
                if row.get("normalized_sha256") and row["normalized_sha256"] not in bysha:
                    bysha[row["normalized_sha256"]]=out/row["gcode_path"]
            first,second=list(bysha.values())[:2]
            diff=list(difflib.unified_diff(
                first.read_text(errors="replace").splitlines(),
                second.read_text(errors="replace").splitlines(),
                fromfile="GCODE_variant_A",tofile="GCODE_variant_B",
                lineterm="",n=1))
            (out/"NON_TIMESTAMP_TOOLPATH_DIFF.txt").write_text(
                "\n".join(diff[:1200])+"\n",encoding="utf-8")
            out_rows["diff_report"]="NON_TIMESTAMP_TOOLPATH_DIFF.txt"
        (out/"repetitions-and-provenance.json").write_text(
            json.dumps(out_rows,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    except Exception as err:
        final="FAIL_RUNNER_"+type(err).__name__
        (out/"runner-exception.json").write_text(
            json.dumps({"status":final,"details":str(err)})+"\n",encoding="utf-8")
    (out/"RESUMO.md").write_text(
        "# Snapmaker U1 original CLI stress repeatability\n\n"
        "Result: **"+final+"**. Only original OEM executable with temporary "
        "guard, NOT native Filuna, not LAN/device, not physical.\n\n"
        "A prior workflow produced different actual movements and remains "
        "historical evidence even when this current stress passes.\n",encoding="utf-8")
    print("SNAPMAKER_STRESS RESULT",final,flush=True)
    return 0 if final.startswith("PASS_") else 1


if __name__=="__main__":
    raise SystemExit(main())
