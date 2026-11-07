#!/usr/bin/env python3
"""Flashforge ORIGINAL AppImage v1.7.9 CLI AB using automatic catalog/resolver.

Two models only as QA fixtures; the catalog indexes ALL official machine,
process and filament presets. Standalone OEM binary, NOT native Filuna parity.
Do not compile OEM or install firmware. No physical network/printing.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import shutil
import sys

from flashforge_appimage_original_qa import obtain, discover_profiles, launch, digest, ROOT, SHA256

sys.path.insert(0,str(ROOT/"FILUNA"/"tools"))
from oem_universal_inventory import read_catalog

FIXTURES={
 "ad5x_04":("Flashforge AD5X 0.4 nozzle",
           "0.20mm Standard @FF AD5X",
           "Flashforge HS PLA @FF AD5X"),
 "ad5m_04":("Flashforge Adventurer 5M 0.4 Nozzle",
            "0.20mm Standard @Flashforge AD5M 0.4 Nozzle",
            "Flashforge Generic PLA")
}


def prepare(catalog, names, dest):
    effective={}
    original={}
    for kind,name in zip(("machine","process","filament"),names):
        key=kind+":"+name
        if key not in catalog["profiles"]:
            raise ValueError("OEM_PRESET_NOT_IN_AUTOMATIC_CATALOG "+key)
        rec=catalog["profiles"][key]
        if rec["status"]!="RESOLVED":
            raise ValueError("UNRESOLVED_OEM_PRESET "+key)
        cfg=rec["effective"]
        if cfg.get("type")!=kind or cfg.get("name")!=name:
            raise ValueError("OEM_EFFECTIVE_PRESET_MISMATCH "+key)
        path=dest/(kind+".json")
        path.write_text(json.dumps(cfg,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
        effective[kind]=path
        original[kind]={"name":name,"source":rec["file"],"sha256_resolved":digest(path),
                        "inherits":rec["parents"]}
    if names[0] not in catalog["profiles"]["process:"+names[1]]["effective"].get("compatible_printers",[]):
        raise ValueError("OEM_PROCESS_NOT_COMPATIBLE_WITH_MACHINE")
    filament_allowed=catalog["profiles"]["filament:"+names[2]]["effective"].get("compatible_printers")
    if filament_allowed and names[0] not in filament_allowed:
        raise ValueError("OEM_FILAMENT_NOT_COMPATIBLE_WITH_MACHINE")
    machine=effective["machine"]
    if "0.4" not in names[0]:
        raise ValueError("NOT_THIS_0_4_QA_FIXTURE")
    if catalog["profiles"]["machine:"+names[0]]["effective"].get("nozzle_diameter") != ["0.4"]:
        raise ValueError("OEM_MACHINE_NOZZLE_0_4_MISMATCH")
    (dest/"exact-profile-lock.json").write_text(json.dumps(original,ensure_ascii=False,indent=2)+"\n")
    return effective


def main():
    p=argparse.ArgumentParser()
    p.add_argument("--cache",type=Path,default=Path(".flashforge-appimage-cache"))
    p.add_argument("--out",type=Path,default=Path("qa-flashforge-ad5x-ad5m-ab"))
    p.add_argument("--timeout",type=int,default=180)
    a=p.parse_args()
    cache=a.cache.resolve();out=a.out.resolve()
    cache.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
    rows=[]
    def add(model,stage,status,detail):
        assert status in ("PASS","FAIL","PENDING")
        rows.append({"model":model,"stage":stage,"status":status,"detail":detail})
        print("FLASHFORGE_CLI_AB",model,stage,status,detail,flush=True)
    try:
        archive=obtain(cache)
        add("common","OFFICIAL_APPIMAGE_SHA","PASS",SHA256)
        extracted=cache/"squashfs-root"
        exe=extracted/"AppRun"
        if not exe.is_file():
            code=launch([str(archive),"--appimage-extract"],out/"extract.log",cache,180)
            if code!=0 or not exe.is_file():
                raise ValueError("ORIGINAL_APPIMAGE_EXTRACT_FAILED_"+str(code))
        exe.chmod(exe.stat().st_mode|0o111)
        roots=discover_profiles(extracted)
        if not roots:raise ValueError("ORIGINAL_FLASHFORGE_PROFILES_MISSING")
        catalog=read_catalog(roots[0])
        (out/"all-profiles-summary.json").write_text(
            json.dumps({"profiles_per_type":catalog["profiles_per_type"],
                        "model_count":len(catalog["machines"]),
                        "inheritance_issues":catalog["issues"],
                        "root":str(roots[0].relative_to(extracted))},
                       ensure_ascii=False,indent=2)+"\n")
        add("common","ALL_MODELS_ALL_PROFILES_CATALOG","PASS",
            "machines="+str(len(catalog["machines"]))+" presets="+str(catalog["profiles_per_type"])
            +" issues="+str(len(catalog["issues"])))
        if catalog["issues"]:
            add("common","ALL_PROFILES_INHERITANCE","PENDING",
                "Review "+str(len(catalog["issues"]))+" raw catalog anomalies")
        else:
            add("common","ALL_PROFILES_INHERITANCE","PASS","0 errors")
        model=ROOT/"fixtures"/"cube20.stl"
        if not model.is_file():raise ValueError("CUBE_MISSING")
        for key,names in FIXTURES.items():
            destination=out/key
            destination.mkdir(parents=True,exist_ok=True)
            try:
                files=prepare(catalog,names,destination)
                add(key,"EFFECTIVE_OEM_PRESETS","PASS","machine+process+PLA resolved")
                runs=[]
                for letter in ("A","B"):
                    folder=destination/("run-"+letter)
                    if folder.exists():shutil.rmtree(folder)
                    folder.mkdir()
                    command=[
                        "xvfb-run","-a","-s","-screen 0 1600x900x24",
                        str(exe),"--debug","3","--slice","0",
                        "--load-settings",str(files["machine"])+";"+str(files["process"]),
                        "--load-filaments",str(files["filament"]),
                        "--outputdir",str(folder),str(model)]
                    log=destination/("official-"+letter+".log")
                    code=launch(command,log,extracted,a.timeout)
                    gcodes=sorted(p for p in folder.rglob("*.gcode")
                                  if p.is_file() and p.stat().st_size>0)
                    if code!=0 or len(gcodes)!=1:
                        add(key,"OFFICIAL_"+letter,"FAIL",
                            "exit="+str(code)+" files="+str(len(gcodes))+" see "+log.name)
                        break
                    file=gcodes[0]
                    runs.append({"path":str(file.relative_to(out)),"sha256":digest(file),
                                 "bytes":file.stat().st_size})
                    add(key,"OFFICIAL_"+letter,"PASS",
                        "bytes="+str(file.stat().st_size)+" SHA256="+digest(file))
                (destination/"outputs.json").write_text(json.dumps(runs,indent=2)+"\n")
                if len(runs)==2 and runs[0]["path"]!=runs[1]["path"] and runs[0]["sha256"]==runs[1]["sha256"]:
                    add(key,"OEM_ORIGINAL_A_B","PASS","RAW_SHA256_IDENTICAL")
                elif len(runs)==2:
                    add(key,"OEM_ORIGINAL_A_B","FAIL","DIFFERENT_GCODE_RAW_HASH_OR_PATH; no unproven normalization")
                else:
                    add(key,"OEM_ORIGINAL_A_B","PENDING","official CLI did not produce two complete G-codes")
            except Exception as exc:
                add(key,"MODEL_RUNNER","FAIL",type(exc).__name__+": "+str(exc))
            add(key,"FILUNA_PARITY","PENDING","no separate actual Filuna-native G-code")
            add(key,"NETWORK_AND_IFS","PENDING","no app native device, upload ACK or multi-color proof")
            add(key,"PHYSICAL_BETA","PENDING","requires later supervised hardware beta")
    except Exception as exc:
        add("common","RUNNER","FAIL",type(exc).__name__+": "+str(exc))
    (out/"results.json").write_text(json.dumps(rows,ensure_ascii=False,indent=2)+"\n")
    (out/"RESUMO.md").write_text("# Flashforge official AppImage 1.7.9 A/B AD5X + AD5M\n\n"
        "Samples do not restrict the automatic full manufacturer catalog; no native Filuna parity.\n\n"
        "| Model | Gate | Status | Details |\n|---|---|---|---|\n"+
        "".join("| "+r["model"]+" | "+r["stage"]+" | "+r["status"]+" | "+
                r["detail"].replace("|","/")+" |\n" for r in rows),encoding="utf-8")
    return 1 if any(x["status"]=="FAIL" for x in rows) else 0

if __name__=="__main__":
    raise SystemExit(main())
