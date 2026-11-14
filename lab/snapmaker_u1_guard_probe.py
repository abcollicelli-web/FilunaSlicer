#!/usr/bin/env python3
"""Snapmaker U1 v2.4.1 Windows CLI access violation - isolated regression probe.

The manufacturer profiles remain byte-identical and immutable. We test a
diagnostic copy with exactly ONE field appended. This experiment is NOT a
passing official A/B; do not confuse it with Filuna parity or physical QA.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import zipfile

from portable_first_qa import OFFICIAL, digest, obtain, extract_executable
from snapmaker_u1_inherit import build_effective_original_presets
# Fixture helper is imported lazily in main to avoid the existing reverse import.

ROOT = Path(__file__).resolve().parents[1]
ACCESS_VIOLATION = {3221225477, -1073741819}


def list_gcodes(folder: Path):
    return [p for p in folder.rglob("*") if p.is_file()
            and p.suffix.lower() in (".gcode", ".gco") and p.stat().st_size > 0]


def run_cli(exe, machine, process, filament, cube, out, label, timeout=120, arrange=None):
    target=out/label
    if target.exists():
        shutil.rmtree(target)
    target.mkdir()
    log=out/(label+".log")
    settings=str(machine)+";"+str(process)
    filaments=";".join([str(filament)]*4)
    argv=["--debug","3","--slice","0","--load-settings",settings,
          "--load-filaments",filaments,
          "--outputdir",str(target),str(cube.resolve())]
    if arrange is not None:
        argv=["--arrange",str(arrange),*argv]
    command=[str(exe),*argv]
    with log.open("w",encoding="utf-8",errors="replace") as fp:
        fp.write("COMMAND_JSON="+json.dumps(command,ensure_ascii=False)+"\n")
        fp.flush()
        try:
            code=subprocess.run(command,cwd=exe.parent,stdout=fp,
                                stderr=subprocess.STDOUT,timeout=timeout,
                                check=False).returncode
        except subprocess.TimeoutExpired:
            code=124
            fp.write("\nTIMEOUT\n")
    gcodes=list_gcodes(target)
    inventory={"case":label,"exit_code":code,
               "access_violation":code in ACCESS_VIOLATION,
               "gcodes":[{"path":str(p.relative_to(out)),"bytes":p.stat().st_size,
                          "sha256":digest(p)} for p in gcodes],
               "log":log.name}
    (out/(label+"-inventory.json")).write_text(json.dumps(inventory,indent=2)+"\n")
    return inventory


def diagnostic_copy(original: Path, target: Path):
    payload=json.loads(original.read_text(encoding="utf-8-sig"))
    if payload.get("type")!="process":
        raise ValueError("NOT_A_PROCESS_JSON")
    if payload.get("name")!="0.20mm Standard @Snapmaker U1 (0.4 nozzle)":
        raise ValueError("NOT_U1_PROCESS")
    original_value=payload.get("different_settings_to_system")
    if original_value is not None:
        raise ValueError("DIAGNOSTIC_FIELD_ALREADY_PRESENT: cannot isolate hypothesis")
    payload["different_settings_to_system"]=[""]
    target.write_text(json.dumps(payload,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    other=json.loads(target.read_text(encoding="utf-8"))
    other.pop("different_settings_to_system")
    if other!=json.loads(original.read_text(encoding="utf-8-sig")):
        raise ValueError("DIAGNOSTIC_COPY_CHANGED_ADDITIONAL_FIELDS")
    return {"source_sha256":digest(original),"copy_sha256":digest(target),
            "single_diagnostic_change":"different_settings_to_system: ['']",
            "source_unchanged":True}


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
    ap.add_argument("--out",type=Path,default=Path("qa-snapmaker-u1-guard-probe"))
    args=ap.parse_args();cache=args.cache.resolve();out=args.out.resolve()
    cache.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
    rows=[]
    def add(stage,status,detail,evidence=""):
        assert status in ("PASS","FAIL","PENDING")
        rows.append(dict(stage=stage,status=status,detail=detail,evidence=evidence))
        print("SNAPMAKER_GUARD",stage,status,detail,flush=True)
    try:
        cfg=OFFICIAL["snapmaker"]
        archive,url=obtain(cfg,cache)
        add("ZIP_ORIGINAL","PASS","sha256="+digest(archive),url)
        exe=extract_executable(archive,cfg["exe"],cache/"snapmaker-portable-extracted").resolve()
        add("BINARIO_ORIGINAL","PASS",str(exe.relative_to(cache)))
        effective=build_effective_original_presets(out)
        add("HERANCA_VERIFICADA","PASS","12 official blobs; only temporary effective configs",
            "effective-profile-derivation.json")
        orig=effective["process"]
        modified=out/"diagnostic-process-single-field.json"
        patch=diagnostic_copy(orig,modified)
        (out/"single-field-change.lock.json").write_text(json.dumps(patch,indent=2)+"\n")
        add("DIAGNOSTIC_SINGLE_FIELD_COPY","PASS",
            "changed only different_settings_to_system on temporary copy",
            "single-field-change.lock.json")
        from snapmaker_u1_fourtool_original_qa import build_minimal_core_3mf, build_four_from_core_3mf
        seed=out/"core-single-cube.3mf"
        cube=out/"four-tools.3mf"
        build_minimal_core_3mf(seed)
        build_four_from_core_3mf(seed,cube)
        add("FOUR_TOOL_FIXTURE","PASS","sha256="+digest(cube),"four-tools.3mf")
        baseline=run_cli(exe,effective["machine"],orig,effective["filament"],cube,out,"baseline-original-effective-A",arrange=0)
        if baseline["access_violation"] and not baseline["gcodes"]:
            add("CONTROL_ORIGINAL_AV","PASS",
                "reproduced official 0xC0000005 with intact effective process",
                "baseline-original-effective-A.log")
        else:
            add("CONTROL_ORIGINAL_AV","PENDING",
                "original returned "+str(baseline["exit_code"])+"; inspect log; no causality inference",
                "baseline-original-effective-A.log")
        diagnostic=[]
        for letter in ("A","B"):
            run=run_cli(exe,effective["machine"],modified,effective["filament"],cube,
                        out,"single-field-diagnostic-"+letter,arrange=0)
            if run["exit_code"]!=0 or len(run["gcodes"])!=1:
                add("DIAGNOSTIC_"+letter,"FAIL",
                    "exit="+str(run["exit_code"])+" generated="+str(len(run["gcodes"])),
                    run["log"])
                continue
            diagnostic.append(run)
            add("DIAGNOSTIC_"+letter,"PASS","generated bytes="+str(run["gcodes"][0]["bytes"])
                +" sha256="+run["gcodes"][0]["sha256"],run["log"])
        if len(diagnostic)==2:
            a=diagnostic[0]["gcodes"][0]
            b=diagnostic[1]["gcodes"][0]
            if a["path"]==b["path"]:
                add("DIAGNOSTIC_AB","FAIL","SAME_OUTPUT_REUSED")
            elif a["sha256"]==b["sha256"]:
                add("DIAGNOSTIC_AB","PASS","diagnostic COPY raw SHA equal="+a["sha256"])
            else:
                add("DIAGNOSTIC_AB","FAIL","GCODE_RAW_SHA_DIFFERENT; no normalizations allowed")
        else:
            add("DIAGNOSTIC_AB","PENDING","NO_TWO_SUCCESSFUL_DIAGNOSTIC_OUTPUTS")
        baseline_b=run_cli(exe,effective["machine"],orig,effective["filament"],cube,out,
                           "baseline-original-effective-B",arrange=0)
        if baseline["exit_code"]==0 and baseline_b["exit_code"]==0 and len(baseline["gcodes"])==1 and len(baseline_b["gcodes"])==1:
            add("OFFICIAL_UNMODIFIED_AB","PASS","both original effective controls generated G-code",
                "baseline-original-effective-B-inventory.json")
        else:
            add("OFFICIAL_UNMODIFIED_AB","PENDING",
                "A exit="+str(baseline["exit_code"])+" B exit="+str(baseline_b["exit_code"])+
                "; no successful original A/B certification",
                "baseline-original-effective-B-inventory.json")
        add("FILUNA_PARITY","PENDING","No separately executing Filuna motor proof")
        add("NETWORK","PENDING","No model-specific simulated device contract")
        add("PHYSICAL_BETA","PENDING","Only after all internal QA and final Filuna compilation")
    except Exception as err:
        add("RUNNER","FAIL",type(err).__name__+": "+str(err))
    (out/"results.json").write_text(json.dumps(rows,ensure_ascii=False,indent=2)+"\n")
    (out/"RESUMO.md").write_text("# Snapmaker U1 v2.4.1 — process vector guard diagnostic\n\n"
        "This tests a modified **temporary copy** of one preset. Not manufacturer parity or Filuna homologation.\n\n"
        "| Gate | State | Detail |\n|---|---|---|\n"
        + "".join("| "+r["stage"]+" | "+r["status"]+" | "+r["detail"].replace("|","/")+" |\n"
                  for r in rows),encoding="utf-8")
    return 1 if any(x["status"]=="FAIL" for x in rows) else 0


if __name__=="__main__":
    raise SystemExit(main())
