#!/usr/bin/env python3
"""Generic OEM Snapmaker process/nozzle normalization guard - isolated QA.

The official manufacturer original JSON files are immutable. This workaround
creates a temporary CLI-only effective-process copy using the selected official
machine's exact nozzle_diameter when process has wipe_tower_filament but lacks
nozzle_diameter. Nozzle count and values come from the manufacturer, NOT U1
hardcoding. This is necessary for the OEM 2.4.1 CLI normalize_fdm null access.

NOT a proof of native Filuna integration, official unmodified A/B or hardware.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
from portable_first_qa import OFFICIAL, digest, obtain, extract_executable
from snapmaker_u1_inherit import build_effective_original_presets
from snapmaker_u1_guard_probe import run_cli

ROOT=Path(__file__).resolve().parents[1]


def guarded_runtime_process(machine_file: Path, process_file: Path,
                            destination: Path) -> dict:
    printer=json.loads(machine_file.read_text(encoding="utf-8-sig"))
    process=json.loads(process_file.read_text(encoding="utf-8-sig"))
    if printer.get("type")!="machine" or process.get("type")!="process":
        raise ValueError("OEM_PRESET_KIND_MISMATCH")
    model=printer.get("name","")
    if not model or model not in process.get("compatible_printers",[]):
        raise ValueError("OEM_PROCESS_MACHINE_NOT_COMPATIBLE")
    nozzles=printer.get("nozzle_diameter")
    if not isinstance(nozzles,list) or not nozzles or not all(
      isinstance(n,str) and n.strip() and float(n)>0 and float(n)<5
      for n in nozzles):
        raise ValueError("OEM_NOZZLE_DIAMETERS_INVALID")
    if "wipe_tower_filament" not in process:
        raise ValueError("NOT_THE_VERIFIED_WIPE_TOWER_NULL_ACCESS_CASE")
    if "nozzle_diameter" in process:
        raise ValueError("PROCESS_ALREADY_HAS_NOZZLE_DIAMETER_NO_WORKAROUND_NEEDED")
    adjusted=dict(process)
    adjusted["nozzle_diameter"]=list(nozzles)
    destination.write_text(json.dumps(adjusted,ensure_ascii=False,indent=2)+"\n",
                           encoding="utf-8")
    check=json.loads(destination.read_text(encoding="utf-8"))
    check.pop("nozzle_diameter",None)
    if check!=process:
        raise ValueError("UNAUTHORIZED_PROCESS_PARAMETER_CHANGE")
    return {
      "vendor":"snapmaker", "printer":model,"nozzle_diameters":list(nozzles),
      "manufacturer_original_machine_sha256":digest(machine_file),
      "manufacturer_original_process_sha256":digest(process_file),
      "runtime_process_copy_sha256":digest(destination),
      "only_added_key":"nozzle_diameter",
      "origin":"same verified official upstream manufacturer machine effective profile",
      "behavior":"CLI-only workaround; preserves wipe_tower_filament and every other process setting",
      "manufacturer_profiles_modified":False,
      "native_filuna_integration":False,
      "official_unmodified_cli_parity":False}


def main():
    p=argparse.ArgumentParser()
    p.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
    p.add_argument("--out",type=Path,default=Path("qa-snapmaker-runtime-nozzle-guard"))
    args=p.parse_args()
    cache=args.cache.resolve();out=args.out.resolve()
    cache.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
    records=[]
    def add(stage,status,details,evidence=""):
        assert status in ("PASS","FAIL","PENDING")
        records.append({"gate":stage,"status":status,
                        "details":details,"evidence":evidence})
        print("SNAPMAKER_RUNTIME_GUARD",stage,status,details,flush=True)
    try:
        cfg=OFFICIAL["snapmaker"]
        archive,url=obtain(cfg,cache)
        exe=extract_executable(archive,cfg["exe"],
                               cache/"snapmaker-portable-extracted").resolve()
        add("OFFICIAL_PINNED_ZIP","PASS",digest(archive),url)
        files=build_effective_original_presets(out)
        original=files["process"]
        candidate=out/"runtime-nozzle-guard-process.json"
        lock=guarded_runtime_process(files["machine"],original,candidate)
        (out/"cli-only-guard.lock.json").write_text(json.dumps(
          lock,ensure_ascii=False,indent=2)+"\n")
        add("OEM_MACHINE_NOZZLE_DERIVATION","PASS",
            "selected model="+lock["printer"]+", nozzle_count="+str(len(lock["nozzle_diameters"]))
            +", process original preserved","cli-only-guard.lock.json")
        cube=(ROOT/"fixtures/cube20.stl").resolve()
        if not cube.is_file():
            raise ValueError("CUBE_FIXTURE_MISSING")
        gcodes=[]
        for name in ("A","B"):
            item=run_cli(exe,files["machine"],candidate,files["filament"],
                         cube,out,"runtime-nozzle-guard-"+name)
            if item["exit_code"]!=0 or len(item["gcodes"])!=1:
                add("CLI_COPY_"+name,"FAIL","exit="+str(item["exit_code"])
                    +" count="+str(len(item["gcodes"])),"runtime-nozzle-guard-"+name+".log")
                break
            gcodes.append(item["gcodes"][0])
            add("CLI_COPY_"+name,"PASS","bytes="+str(gcodes[-1]["bytes"])
                +" sha256="+gcodes[-1]["sha256"],item["log"])
        if len(gcodes)==2:
            same=gcodes[0]["sha256"]==gcodes[1]["sha256"]
            distinct=gcodes[0]["path"]!=gcodes[1]["path"]
            add("GUARDED_CLI_A_B","PASS" if (same and distinct) else "FAIL",
                "raw_sha="+gcodes[0]["sha256"]+" matches="+str(same)
                +" independent_outputs="+str(distinct))
        else:
            add("GUARDED_CLI_A_B","PENDING","two valid G-codes not produced")
        add("ORIGINAL_CLI_A_B","PENDING","official original process still crashes; guard is only diagnostic copy")
        add("FILUNA_PARITY","PENDING","native Filuna source delta not yet compiled/loaded")
        add("NETWORK_AND_CONTROL","PENDING","U1 network/access/print protocol not proven")
        add("PHYSICAL_BETA","PENDING","only after internal QA and final build")
    except Exception as e:
        add("RUNNER","FAIL",type(e).__name__+": "+str(e))
    (out/"results.json").write_text(json.dumps(records,indent=2,ensure_ascii=False)+"\n")
    (out/"RESUMO.md").write_text(
      "# Snapmaker v2.4.1 generic runtime nozzle guard, official original preserved\n\n"
      "Diagnostic/CLI workaround only, NOT an upstream change nor OEM homologation.\n\n"
      "| Gate | Status | Detail |\n|---|---|---|\n"
      +"".join("| "+r["gate"]+" | "+r["status"]+" | "+r["details"].replace("|","/")+" |\n"
               for r in records),encoding="utf-8")
    return 1 if any(x["status"]=="FAIL" for x in records) else 0


if __name__=="__main__":
    raise SystemExit(main())
