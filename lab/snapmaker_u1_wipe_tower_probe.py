#!/usr/bin/env python3
"""Snapmaker U1 controlled source-correlated loader fault isolation.

Official sources and full profiles are verified unchanged. Diagnostic copies
vary ONE runtime config option ('wipe_tower_filament' or 'nozzle_diameter')
to test the exact null-dereference candidate in Snapmaker/OrcaSlicer v2.4.1
src/libslic3r/PrintConfig.cpp DynamicPrintConfig::normalize_fdm.
Neither a modified copy nor its G-code is an official parity proof.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
from portable_first_qa import OFFICIAL,digest,obtain,extract_executable
from snapmaker_u1_inherit import build_effective_original_presets
from snapmaker_u1_guard_probe import run_cli

ROOT=Path(__file__).resolve().parents[1]
EXCEPTION_ACCESS_VIOLATION={3221225477,-1073741819}


def derive_one_field(source:Path,destination:Path,kind:str):
    original=json.loads(source.read_text(encoding="utf-8-sig"))
    if original.get("type")!="process":
        raise ValueError("SOURCE_NOT_PROCESS")
    changed=dict(original)
    if kind=="minus_wipe_tower":
        if "wipe_tower_filament" not in changed:
            raise ValueError("ORIGINAL_WIPE_TOWER_OPTION_MISSING")
        changed.pop("wipe_tower_filament")
        expected_diff=["wipe_tower_filament"]
    elif kind=="add_nozzle_diameter":
        if "nozzle_diameter" in changed:
            raise ValueError("PROCESS_ALREADY_HAS_NOZZLE")
        changed["nozzle_diameter"]=["0.4","0.4","0.4","0.4"]
        expected_diff=["nozzle_diameter"]
    elif kind=="minimal_no_wipe":
        chosen={"type","name","from","setting_id","instantiation","compatible_printers"}
        changed={k:v for k,v in original.items() if k in chosen}
        expected_diff=sorted(set(original)-set(changed))
    elif kind=="minimal_plus_wipe":
        chosen={"type","name","from","setting_id","instantiation",
                "compatible_printers","wipe_tower_filament"}
        changed={k:v for k,v in original.items() if k in chosen}
        expected_diff=sorted(set(original)-set(changed))
    else:
        raise ValueError("UNKNOWN_DIAGNOSTIC")
    if changed.get("type")!="process" or changed.get("name")!=original["name"]:
        raise ValueError("IDENTITY_CHANGED")
    destination.write_text(json.dumps(changed,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    parsed=json.loads(destination.read_text(encoding="utf-8"))
    if parsed!=changed:
        raise ValueError("DIAGNOSTIC_SERIALIZATION_CHANGED")
    actual_diff=sorted(k for k in set(original)|set(changed)
                       if original.get(k)!=changed.get(k))
    if actual_diff!=expected_diff:
        raise ValueError("PROBE_DIFFERENTIAL_INVALID expected="+repr(expected_diff)
                         +" actual="+repr(actual_diff))
    return {"source_sha256":digest(source),"modified_sha256":digest(destination),
            "exact_differential_keys":actual_diff,"source_preserved":True,
            "result_process_option_count":len(changed)}


def did_load_process(log:Path)->bool:
    return "loaded process config" in log.read_text(encoding="utf-8",errors="replace")


def run():
    p=argparse.ArgumentParser()
    p.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
    p.add_argument("--out",type=Path,default=Path("qa-snapmaker-u1-wipe-tower-root-cause"))
    args=p.parse_args()
    cache=args.cache.resolve();out=args.out.resolve()
    cache.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
    records=[];results={}
    def add(stage,status,detail,evidence=""):
        assert status in ("PASS","FAIL","PENDING")
        records.append({"gate":stage,"status":status,"detail":detail,"evidence":evidence})
        print("U1_WIPE_TOWER",stage,status,detail,flush=True)
    try:
        cfg=OFFICIAL["snapmaker"]
        archive,url=obtain(cfg,cache)
        exe=extract_executable(archive,cfg["exe"],
                               cache/"snapmaker-portable-extracted").resolve()
        add("ORIGINAL_PORTABLE_HASH","PASS",digest(archive),url)
        effective=build_effective_original_presets(out)
        process=effective["process"]
        payload=json.loads(process.read_text(encoding="utf-8-sig"))
        machine=json.loads(effective["machine"].read_text(encoding="utf-8-sig"))
        if ("wipe_tower_filament" not in payload
            or "nozzle_diameter" in payload
            or len(machine.get("nozzle_diameter",[]))!=4):
            raise ValueError("UPSTREAM_EXPECTED_NULL_ACCESS_PRECONDITIONS_NOT_MET")
        add("UPSTREAM_NULL_ACCESS_PRECONDITION","PASS",
            "process contains wipe_tower_filament without nozzle_diameter; machine has 4 nozzles")
        cube=(ROOT/"fixtures/cube20.stl").resolve()
        if not cube.is_file():raise ValueError("FIXTURE_MISSING")
        cases=["control","minimal_no_wipe","minimal_plus_wipe",
               "minus_wipe_tower","add_nozzle_diameter"]
        for name in cases:
            if name=="control":
                path=process
            else:
                path=out/("process-"+name+".json")
                lock=derive_one_field(process,path,name)
                (out/("lock-"+name+".json")).write_text(json.dumps(lock,indent=2)+"\n")
            result=run_cli(exe,effective["machine"],path,effective["filament"],
                           cube,out,name)
            loaded=did_load_process(out/result["log"])
            results[name]={**result,"process_load_completed":loaded,
                           "process_file_sha256":digest(path)}
            add("CASE_"+name,"PASS" if loaded else "PENDING",
                "exit="+str(result["exit_code"])+" loaded_process="+str(loaded)
                +" AV="+str(result["access_violation"])
                +" gcode="+str(len(result["gcodes"])),result["log"])
        original=results["control"]
        minimal=results["minimal_no_wipe"]
        minimal_bad=results["minimal_plus_wipe"]
        removed=results["minus_wipe_tower"]
        added=results["add_nozzle_diameter"]
        root_proved=(original["access_violation"] and not original["process_load_completed"]
                     and minimal["process_load_completed"]
                     and minimal_bad["access_violation"]
                     and not minimal_bad["process_load_completed"]
                     and removed["process_load_completed"]
                     and added["process_load_completed"])
        add("NULL_DEREFERENCE_CAUSAL_DIFFERENTIAL",
            "PASS" if root_proved else "PENDING",
            "controlled minimal A/B insertion reproduces crash; removing or supplying nozzle cures profile load"
            if root_proved else "differential incomplete: inspect all CLI logs and source")
        if removed["exit_code"]==0 and len(removed["gcodes"])==1:
            second=run_cli(exe,effective["machine"],out/"process-minus_wipe_tower.json",
                           effective["filament"],cube,out,"minus-wipe-tower-B")
            results["minus_wipe_tower_B"]={**second,
                "process_load_completed":did_load_process(out/second["log"])}
            same=(second["exit_code"]==0 and len(second["gcodes"])==1
                  and second["gcodes"][0]["path"]!=removed["gcodes"][0]["path"]
                  and second["gcodes"][0]["sha256"]==removed["gcodes"][0]["sha256"])
            add("MODIFIED_COPY_A_B",
                "PASS" if same else "PENDING",
                "diagnostic-only distinct outputs and SHA match" if same else "diagnostic G-code A/B not proven")
        else:
            add("MODIFIED_COPY_A_B","PENDING","diagnostic process did not produce first G-code")
        add("ORIGINAL_OFFICIAL_A_B","PENDING","unmodified official CLI must succeed without process edits")
        add("FILUNA_NATIVE_PARITY","PENDING","separate integrated Filuna OEM execution not proven")
        add("SNAPMAKER_NETWORK","PENDING","U1 native discovery, auth, control, camera, upload untested")
        add("PHYSICAL_BETA","PENDING","only after all internal gates, final build and supervised beta")
    except Exception as exc:
        add("RUNNER","FAIL",type(exc).__name__+": "+str(exc))
    (out/"run-matrix.json").write_text(json.dumps(results,ensure_ascii=False,indent=2)+"\n")
    (out/"results.json").write_text(json.dumps(records,ensure_ascii=False,indent=2)+"\n")
    (out/"RESUMO.md").write_text("# Snapmaker U1 official v2.4.1 — wipe-tower null access differential\n\n"
      "These are temporary diagnostic copies; official profiles and source remain unchanged.\n\n"
      "| Stage | Status | Evidence |\n|---|---|---|\n"+
      "".join("| "+x["gate"]+" | "+x["status"]+" | "+x["detail"].replace("|","/")+" |\n"
              for x in records),encoding="utf-8")
    return 1 if any(x["status"]=="FAIL" for x in records) else 0

if __name__=="__main__":
    raise SystemExit(run())
