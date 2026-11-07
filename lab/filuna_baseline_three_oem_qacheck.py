#!/usr/bin/env python3
"""Compare a previously built Filuna Windows PORTABLE with original OEM G-code.

No compilation, installer or device connection. This baseline is pre-integration
and is expected NOT to match OEM. A pass of this diagnostic is not homologation.
"""
import argparse
import difflib
import hashlib
import json
from pathlib import Path
import subprocess

def h(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def model_files(vendor,artifact,model,out):
    if vendor=="anycubic":
        all_=json.loads((artifact/"all-models-all-profiles.json").read_text(encoding="utf-8"))
        prefix={"machine":"","process":"0.20mm Standard @","filament":"Anycubic PLA @"}
        paths={}
        for k,pre in prefix.items():
            record=all_["profiles"][k+":"+pre+model]
            if record["status"]!="RESOLVED":
                raise ValueError("UNRESOLVED_OFFICIAL_PROFILE")
            target=out/(k+".json")
            target.write_text(json.dumps(record["effective"],ensure_ascii=False,indent=2)+"\n")
            paths[k]=target
        return paths,artifact/"kobra_3_v2_04/run-A/plate_1.gcode","OFFICIAL_EFFECTIVE_NOT_RAW_LEAF"
    if vendor=="flashforge":
        paths={k:artifact/"ad5x_04"/(k+".json") for k in ("machine","process","filament")}
        return paths,artifact/"ad5x_04/run-A/plate_1.gcode","OFFICIAL_EXACT_EFFECTIVE"
    paths={"machine":artifact/"effective-machine.json",
           "process":artifact/"guarded-source-process.json",
           "filament":artifact/"effective-filament.json"}
    return paths,artifact/"stress-00/plate_1.gcode","OEM_PROCESS_TEMPORARY_GUARD"

def run_one(vendor,model,artifact,exe,stl,out):
    dest=out/vendor
    dest.mkdir(parents=True)
    data={"brand":vendor,"model":model,"status":"NOT_RUN",
          "baseline":"FILUNA_2026_11_05_REBRAND_WITHOUT_OEM_MODULES",
          "network":"NOT_RUN","real_hardware":"NOT_RUN"}
    try:
        paths,original,notes=model_files(vendor,artifact,model,dest)
        data["profile_kind"]=notes
        for x in [*paths.values(),original]:
            if not x.is_file():
                raise FileNotFoundError("OFFICIAL_INPUT_MISSING "+str(x))
        data["input_sha256"]={k:h(v) for k,v in paths.items()}
        data["official_gcode"]={"sha256":h(original),"bytes":original.stat().st_size}
        target=dest/"Filuna-CLI"
        target.mkdir()
        filaments=[str(paths["filament"])]*(4 if vendor=="snapmaker" else 1)
        cmd=[str(exe),"--debug","3","--slice","0",
             "--load-settings",str(paths["machine"])+";"+str(paths["process"]),
             "--load-filaments",";".join(filaments),
             "--outputdir",str(target),str(stl)]
        with (dest/"Filuna-cli.log").open("w",encoding="utf-8",errors="replace") as log:
            log.write("argv="+json.dumps(cmd)+"\n")
            log.flush()
            try:
                status=subprocess.run(cmd,cwd=exe.parent,stdout=log,
                         stderr=subprocess.STDOUT,timeout=180).returncode
            except subprocess.TimeoutExpired:
                status=124
        data["filuna_returncode"]=status
        products=list(target.rglob("*.gcode"))
        data["filuna_output_count"]=len(products)
        if status or len(products)!=1:
            data["status"]="FAIL_NO_FILUNA_GCODE_OR_CRASH"
        else:
            produced=products[0]
            data["filuna_gcode"]={"sha256":h(produced),"bytes":produced.stat().st_size}
            left=original.read_bytes();right=produced.read_bytes()
            if left==right:
                data["status"]="PASS_IDENTICAL_GCODE_SINGLE_SAMPLE"
            else:
                al=left.decode("utf-8","replace").splitlines()
                bl=right.decode("utf-8","replace").splitlines()
                diff=list(difflib.unified_diff(al,bl,fromfile="OEM",tofile="Filuna",
                                                lineterm="",n=1))
                (dest/"real-gcode-diff.txt").write_text("\n".join(diff[:600])+"\n",
                                                        encoding="utf-8")
                data["status"]="FAIL_OEM_FILUNA_GCODE_NOT_EQUAL"
                data["diff_line_count"]=len(diff)
                data["line_counts"]={"OEM":len(al),"Filuna":len(bl)}
    except Exception as e:
        data["status"]="FAIL_QA_SETUP"
        data["exception"]=type(e).__name__+": "+str(e)
    return data

def main():
    ap=argparse.ArgumentParser()
    for name in ("base","anycubic","flashforge","snapmaker","out"):
        ap.add_argument("--"+name,type=Path,required=True)
    a=ap.parse_args()
    exe=(a.base/"orca-slicer.exe").resolve()
    dll=(a.base/"OrcaSlicer.dll").resolve()
    if not exe.is_file() or not dll.is_file():
        raise ValueError("IMMUTABLE_FILUNA_PORTABLE_BASELINE_MISSING")
    out=a.out.resolve();out.mkdir(parents=True,exist_ok=True)
    (out/"base.sha.json").write_text(json.dumps({"artifact":"Filuna_Windows_x64_portable",
       "run":42102469888,"sha":"2dace0db86d305bec5b9627179f265012e4f5069",
       "exe_sha256":h(exe),"dll_sha256":h(dll)},indent=2))
    stl=(Path(__file__).resolve().parents[1]/"fixtures/cube20.stl").resolve()
    rows=[run_one("anycubic","Anycubic Kobra 3 V2 0.4 nozzle",a.anycubic,exe,stl,out),
          run_one("flashforge","Flashforge AD5X 0.4 nozzle",a.flashforge,exe,stl,out),
          run_one("snapmaker","Snapmaker U1 (0.4 nozzle)",a.snapmaker,exe,stl,out)]
    (out/"verdicts.json").write_text(json.dumps(rows,indent=2,ensure_ascii=False)+"\n")
    (out/"RESUMO.md").write_text("# Filuna prior portable baseline vs official OEM\n\n"
        "This is NEGATIVE DIAGNOSTIC, not homologation of native OEM modules.\n\n"
        "| Vendor | Actual status |\n|---|---|\n"+
        "".join("| "+x["brand"]+" | "+x["status"]+" |\n" for x in rows))
    for row in rows:
        print("FILUNA_NATIVE_BASELINE_VS_OEM",row["brand"],row["status"],flush=True)
    return 0  # diagnostic job collected statuses; promotion must inspect verdicts

if __name__=="__main__":
    raise SystemExit(main())
