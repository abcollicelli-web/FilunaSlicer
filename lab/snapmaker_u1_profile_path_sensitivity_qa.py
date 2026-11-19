#!/usr/bin/env python3
"""Snapmaker U1 vendor CLI: isolate sensitivity to profile input PATH only.

All original official v2.4.1 profiles and binary are immutable. The special
process-only null access is bypassed in a temporary model-derived copy (as
already experimentally proven); duplicates of that copy have identical SHA.
Change exactly one path at a time: process, machine, filament. Never normalize
moves, speeds, temperatures, tool changes or G-code instructions.
"""
from __future__ import annotations
import argparse
import difflib
import json
from pathlib import Path
import shutil

from portable_first_qa import OFFICIAL,digest,obtain,extract_executable
from snapmaker_u1_inherit import build_effective_original_presets
from snapmaker_runtime_nozzle_guard_qa import guarded_runtime_process
from snapmaker_u1_guard_probe import run_cli
from snapmaker_u1_temporal_ab_qa import header_date_only_normalization

ROOT=Path(__file__).resolve().parents[1]


def compare(a:Path,b:Path,out:Path,label:str):
    aa=header_date_only_normalization(a.read_bytes())
    bb=header_date_only_normalization(b.read_bytes())
    same=aa["normalized_sha256"]==bb["normalized_sha256"]
    proof={"case":label,"base":aa,"candidate":bb,
           "header_only_equal":same,
           "path_does_not_change_gcode":same}
    if not same:
        left=a.read_bytes().decode("utf-8","replace").splitlines()
        right=b.read_bytes().decode("utf-8","replace").splitlines()
        differences=list(difflib.unified_diff(left,right,fromfile="base",tofile=label,
                                              lineterm="",n=1))
        path=out/("diff-"+label+".txt")
        path.write_text("\n".join(differences[:1000])+"\n",encoding="utf-8")
        proof["diff_line_count"]=len(differences)
        proof["diff_excerpt"]=path.name
    return proof


def main():
    p=argparse.ArgumentParser()
    p.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
    p.add_argument("--out",type=Path,default=Path("qa-snapmaker-u1-path-sensitivity"))
    a=p.parse_args()
    cache=a.cache.resolve();out=a.out.resolve()
    cache.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
    rows=[]
    def add(stage,status,detail):
        assert status in ("PASS","FAIL","PENDING")
        rows.append({"stage":stage,"status":status,"detail":detail})
        print("U1_PATH_SENSITIVITY",stage,status,detail,flush=True)
    try:
        cfg=OFFICIAL["snapmaker"]
        archive,url=obtain(cfg,cache)
        exe=extract_executable(archive,cfg["exe"],
                               cache/"snapmaker-portable-extracted").resolve()
        add("OEM_ORIGINAL_ZIP","PASS",digest(archive))
        effective=build_effective_original_presets(out)
        original=out/"corrected-processed-A.json"
        provenance=guarded_runtime_process(effective["machine"],effective["process"],original)
        machine=effective["machine"];filament=effective["filament"]
        def duplicate(src:Path,name:str)->Path:
            dest=out/name
            shutil.copyfile(src,dest)
            if digest(src)!=digest(dest):
                raise ValueError("DUPLICATED_PROFILE_BYTES_DIFFER")
            return dest
        process_b=duplicate(original,"alternate-name-process-B.json")
        machine_b=duplicate(machine,"alternate-name-machine-B.json")
        filament_b=duplicate(filament,"alternate-name-filament-B.json")
        # Same-name/different-folder tests isolate basename from full path.
        # Repeated controls rule out order/time/process-state effects.
        alias_dir=out/"renamed-location";alias_dir.mkdir(parents=True,exist_ok=True)
        process_same_name=alias_dir/original.name
        filament_same_name=alias_dir/filament.name
        shutil.copyfile(original,process_same_name)
        shutil.copyfile(filament,filament_same_name)
        if digest(original)!=digest(process_same_name) or digest(filament)!=digest(filament_same_name):
            raise ValueError("PATH_CONTROL_DUPLICATE_CHANGED_CONTENT")
        cases=[
            ("base",machine,original,filament),
            ("process-path-only",machine,process_b,filament),
            ("base-after-process",machine,original,filament),
            ("process-same-basename-newdir",machine,process_same_name,filament),
            ("machine-path-only",machine_b,original,filament),
            ("filament-path-only",machine,original,filament_b),
            ("base-after-filament",machine,original,filament),
            ("filament-same-basename-newdir",machine,original,filament_same_name),
            ("process-path-repeat",machine,process_b,filament),
            ("filament-path-repeat",machine,original,filament_b),
            ("base-final-repeat",machine,original,filament)
        ]
        (out/"source-lock.json").write_text(json.dumps({
            "upstream_package_sha256":digest(archive),
            "full_original_provenance":provenance,
            "same_bytes_all_duplicates":{
                "process_sha":digest(original),
                "machine_sha":digest(machine),
                "filament_sha":digest(filament)},
            "tests":[{"case":name,"machine":str(m.relative_to(out)),
                      "process":str(p.relative_to(out)),
                      "filament":str(f.relative_to(out))}
                     for name,m,p,f in cases],
            "changes_per_case":"only one profile path, no bytes",
            "no_native_filuna_module_executed":True
        },indent=2)+"\n")
        gcodes={}
        for name,machine_path,process_path,filament_path in cases:
            r=run_cli(exe,machine_path,process_path,filament_path,
                      ROOT/"fixtures"/"cube20.stl",out,name)
            if r["exit_code"]!=0 or len(r["gcodes"])!=1:
                add("CASE_"+name,"FAIL","exit="+str(r["exit_code"])
                    +" output="+str(len(r["gcodes"])))
                break
            path=out/r["gcodes"][0]["path"]
            gcodes[name]=path
            add("CASE_"+name,"PASS","bytes="+str(path.stat().st_size)+" raw_sha="+digest(path))
        variants=[]
        for name,_,_,_ in cases[1:]:
            if "base" not in gcodes or name not in gcodes:
                add("PATH_AB_"+name,"PENDING","G-code missing for comparison")
                continue
            proof=compare(gcodes["base"],gcodes[name],out,name)
            variants.append(proof)
            # A difference is a *valid and useful diagnostic outcome* but it
            # does NOT pass G-code reproducibility. Report failure separately.
            add("PATH_AB_"+name,"PASS" if proof["header_only_equal"] else "FAIL",
                "normalized_equal="+str(proof["header_only_equal"])
                +" A="+proof["base"]["normalized_sha256"]
                +" B="+proof["candidate"]["normalized_sha256"])
        (out/"path-comparison.json").write_text(json.dumps(
            variants,indent=2,ensure_ascii=False)+"\n")
        # A fail-closed distinction between a pure filename alias issue and
        # genuine non-repeatability across separate OEM CLI process instances.
        try:
            normalized={name:header_date_only_normalization(p.read_bytes())["normalized_sha256"]
                        for name,p in gcodes.items()}
            proof={
               "all_same_original_config_bytes":True,
               "input_paths_only_difference":True,
               "base_consistent_across_repetition":len({normalized[n] for n in
                   ("base","base-after-process","base-after-filament","base-final-repeat")})==1,
               "process_alias_consistent":normalized["process-path-only"]==normalized["process-path-repeat"],
               "filament_alias_consistent":normalized["filament-path-only"]==normalized["filament-path-repeat"],
               "process_basename_vs_directory":{
                   "different_basename":normalized["process-path-only"],
                   "same_basename_in_other_directory":normalized["process-same-basename-newdir"],
                   "base":normalized["base"]},
               "filament_basename_vs_directory":{
                   "different_basename":normalized["filament-path-only"],
                   "same_basename_in_other_directory":normalized["filament-same-basename-newdir"],
                   "base":normalized["base"]},
               "source": "Snapmaker v2.4.1 original Windows portable, SHA locked, only temporary nozzle guard",
               "filuna_parity_or_network_proved":False}
            (out/"repeat-and-path-causality.json").write_text(json.dumps(proof,indent=2)+"\\n")
            add("BASE_REPEAT_INDEPENDENT", "PASS" if proof["base_consistent_across_repetition"] else "FAIL",
                "same normalized G-code across 4 independent CLI invocations="+str(proof["base_consistent_across_repetition"]))
            add("ALIAS_REPEAT_INDEPENDENT",
                "PASS" if proof["process_alias_consistent"] and proof["filament_alias_consistent"] else "FAIL",
                "process_alias_consistent="+str(proof["process_alias_consistent"])+
                ", filament_alias_consistent="+str(proof["filament_alias_consistent"]))
        except KeyError as err:
            add("PATH_CAUSALITY_INCOMPLETE","FAIL","Missing case: "+str(err))
        add("FILUNA_NATIVE_PARITY","PENDING","no Filuna application binary")
        add("NETWORK_AND_CONTROL","PENDING","no U1 device access/ACK simulation")
        add("PHYSICAL_BETA","PENDING","only later with beta and real hardware")
    except Exception as err:
        add("RUNNER","FAIL",type(err).__name__+": "+str(err))
    (out/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
    (out/"RESUMO.md").write_text("# Snapmaker U1 exact-profile path sensitivity\n\n"
      "Official OEM CLI + temporary guard, no Filuna native integration.\n\n"
      "| Gate | Status | Details |\n|---|---|---|\n"+
      "".join("| "+x["stage"]+" | "+x["status"]+" | "+x["detail"].replace("|","/")+" |\n"
              for x in rows),encoding="utf-8")
    return 1 if any(r["status"]=="FAIL" for r in rows) else 0


if __name__=="__main__":
    raise SystemExit(main())
