#!/usr/bin/env python3
"""Prusa CORE One + CORE One L 0.4 nozzle - official portable CLI A/B.

Independent models; exact PrusaResearch.ini version_2.9.7 original Git blob.
This is OEM-only, NOT Filuna parity, camera, network or physical homologation.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import urllib.request

from portable_first_qa import OFFICIAL, digest, obtain, extract_executable
from prusa_mk4s_official_qa import ROOT, SOURCE, BLOB, METADATA, sections, resolve, git_blob, command

TARGETS={
 "coreone_04":{
  "name":"Prusa CORE One 0.4 nozzle","printer_model":"COREONE",
  "print":"0.20mm SPEED @COREONE 0.4","filament":"Prusament PLA @COREONE",
  "bed_shape":"0x0,250x0,250x220,0x220","max_print_height":"270"},
 "coreone_l_04":{
  "name":"Prusa CORE One L 0.4 nozzle","printer_model":"COREONEL",
  "print":"0.20mm SPEED @COREONEL 0.4","filament":"Prusament PLA @COREONE",
  "bed_shape":"0x0,300x0,300x300,0x300","max_print_height":"330"},
}


def derive(index, target):
    model, pm=resolve(index,"printer",target["name"])
    process, pp=resolve(index,"print",target["print"])
    filament, pf=resolve(index,"filament",target["filament"])
    checks={
      "printer_model":target["printer_model"],
      "printer_variant":"0.4",
      "nozzle_diameter":"0.4",
      "nozzle_high_flow":"0",
      "bed_shape":target["bed_shape"],
      "max_print_height":target["max_print_height"],
      "default_print_profile":target["print"],
      "default_filament_profile":'"'+target["filament"]+'"'
    }
    for key, expected in checks.items():
        if model.get(key)!=expected:
            raise ValueError("WRONG_PRESET_"+key+" expected "+expected+" actual "+str(model.get(key)))
    settings={}
    duplicates=[]
    for partname,part in (("process",process),("filament",filament),("printer",model)):
        for key,value in part.items():
            if key in METADATA:
                continue
            if key in settings and settings[key]!=value:
                duplicates.append({"key":key,"from":partname,"replaced":settings[key]})
            settings[key]=value
    if not settings.get("start_gcode") or settings.get("binary_gcode")!="1":
        raise ValueError("OFFICIAL_COREONE_START_CODE_OR_BINARY_FORMAT_MISSING")
    return settings,{"printer":pm,"process":pp,"filament":pf},duplicates


def qa_one(exe, index, target_id, target, out, cube, timeout, rows):
    def add(stage,status,detail,evidence=""):
        assert status in ("PASS","FAIL","PENDING")
        rows.append({"model":target_id,"stage":stage,"status":status,
                     "detail":detail,"evidence":evidence})
        print("PRUSA_COREONE",target_id,stage,status,detail,flush=True)
    try:
        stage=out/target_id
        stage.mkdir(parents=True,exist_ok=True)
        settings,parents,collisions=derive(index,target)
        effective=stage/"effective-profile.ini"
        effective.write_text(
            "# temporary config derived from exact original PrusaResearch.ini; not OEM source\n"
            +"\n".join(k+" = "+v for k,v in sorted(settings.items()))+"\n",
            encoding="utf-8")
        (stage/"preset-lock.json").write_text(json.dumps({
          "printer":target["name"],"print":target["print"],"filament":target["filament"],
          "official_repo":"prusa3d/PrusaSlicer","tag":"version_2.9.7",
          "source_blob":BLOB,"inheritance":parents,"overrides":collisions,
          "effective_sha256":digest(effective),"binary_gcode":settings["binary_gcode"]
        },ensure_ascii=False,indent=2)+"\n")
        add("EXACT_MODEL_AND_INHERITANCE","PASS",
            "0.4 nozzle, non-HF, exact official model, default process and filament",
            str((stage/"preset-lock.json").relative_to(out)))
        successes=[]
        for letter in ("A","B"):
            directory=stage/("run-"+letter)
            if directory.exists():shutil.rmtree(directory)
            directory.mkdir()
            output=directory/"official-coreone.bgcode"
            argv=["--printer-technology","FFF","--load",str(effective),
                  "--export-gcode","--output",str(output),str(cube.resolve())]
            exitcode=command(exe,argv,stage/("official-"+letter+".log"),exe.parent,timeout)
            if exitcode!=0 or not output.is_file() or output.stat().st_size==0:
                add("OFFICIAL_"+letter,"FAIL","exit="+str(exitcode)
                    +" file="+str(output.is_file()),"official-"+letter+".log")
                break
            sha=digest(output)
            successes.append({"path":str(output.relative_to(out)),
                              "bytes":output.stat().st_size,"sha256":sha})
            add("OFFICIAL_"+letter,"PASS","bytes="+str(output.stat().st_size)
                +" sha256="+sha,str(output.relative_to(out)))
        (stage/"outputs.json").write_text(json.dumps(successes,indent=2)+"\n")
        if len(successes)<2:
            add("OFFICIAL_A_B","PENDING","OFFICIAL_CLI_NOT_YET_SUCCESSFUL")
        elif successes[0]["path"]==successes[1]["path"]:
            add("OFFICIAL_A_B","FAIL","SAME_OUTPUT_FILE_REUSED")
        elif successes[0]["sha256"]!=successes[1]["sha256"]:
            add("OFFICIAL_A_B","FAIL","RAW_SHA_DIFFERENT; no normalization approved")
        else:
            add("OFFICIAL_A_B","PASS","RAW_SHA_IDENTICAL "+successes[0]["sha256"])
    except Exception as exc:
        add("MODEL_RUNNER","FAIL",type(exc).__name__+": "+str(exc))
    add("FILUNA_PARITY","PENDING","Separate Filuna native G-code not compared")
    add("NETWORK","PENDING","Real Filuna DeviceBridge simulator not exercised")
    add("PHYSICAL_BETA","PENDING","Hardware only after internal QA and final build")


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
    parser.add_argument("--out",type=Path,default=Path("qa-prusa-coreone"))
    parser.add_argument("--timeout",type=int,default=180)
    args=parser.parse_args()
    cache=args.cache.resolve();out=args.out.resolve()
    cache.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
    rows=[]
    try:
        cfg=OFFICIAL["prusa"]
        archive,url=obtain(cfg,cache)
        exe=extract_executable(archive,cfg["exe"],cache/"prusa-portable-extracted").resolve()
        request=urllib.request.Request(SOURCE,headers={"User-Agent":"FilunaOEM-QA/1"})
        with urllib.request.urlopen(request,timeout=90) as response:
            source=response.read(4*1024*1024+1)
        if len(source)>4*1024*1024 or git_blob(source)!=BLOB:
            raise ValueError("OFFICIAL_PRUSA_CONFIG_BLOB_MISMATCH")
        index=sections(source.decode("utf-8-sig"))
        cube=ROOT/"fixtures/cube20.stl"
        if not cube.is_file():raise ValueError("CUBE20_MISSING")
        (out/"source.lock.json").write_text(json.dumps({
          "exe":str(exe.relative_to(cache)),"zip_sha256":digest(archive),
          "source_url":SOURCE,"source_git_blob":BLOB,
          "source_sha256":hashlib.sha256(source).hexdigest(),
          "cube_sha256":digest(cube)},indent=2)+"\n")
        for tid,model in TARGETS.items():
            qa_one(exe,index,tid,model,out,cube,args.timeout,rows)
    except Exception as exc:
        rows.append({"model":"all","stage":"RUNNER","status":"FAIL","detail":repr(exc)})
        print("PRUSA_COREONE RUNNER FAIL",str(exc),flush=True)
    (out/"results.json").write_text(json.dumps(rows,indent=2,ensure_ascii=False)+"\n")
    summary=["# Prusa CORE One / CORE One L — official Windows 2.9.7 A/B",
             "Source-pinned official console only, not Filuna or network/physical QA.",
             "","| Model | Stage | Result | Evidence |","|---|---|---|---|"]
    summary+=["| "+r["model"]+" | "+r["stage"]+" | "+r["status"]+
              " | "+str(r.get("detail","")).replace("|","/")+" |" for r in rows]
    (out/"RESUMO.md").write_text("\n".join(summary)+"\n",encoding="utf-8")
    return 1 if any(x["status"]=="FAIL" for x in rows) else 0


if __name__=="__main__":
    raise SystemExit(main())
