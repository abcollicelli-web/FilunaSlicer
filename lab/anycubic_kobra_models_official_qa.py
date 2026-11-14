#!/usr/bin/env python3
"""Anycubic original v2.0.0.6 CLI A/B, three independently selected OEM model fixtures; universal catalog for every model.

Original vendor Linux .deb extracted in isolated GitHub Actions, no OEM build
or system install. No Filuna parity, network, ACE authentication or physical test.
"""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import urllib.request

from anycubic_vendor_deb_qa import (
 APT_ROOT, PACKAGES, FILENAME, SHA256, BYTES, APP_VERSION,
 apt_records, find_exact_package, download, sha
)
from portable_first_qa import digest
from anycubic_original_gcode_strict import compare_files

ROOT=Path(__file__).resolve().parents[1]
TARGETS={
 "kobra_s1_04": "Anycubic Kobra S1 0.4 nozzle",
 "kobra_3_v2_04": "Anycubic Kobra 3 V2 0.4 nozzle",
 "kobra_4_04": "Anycubic Kobra 4 0.4 nozzle",
}
FIELDS=("type","from","name","inherits","compatible_printers")


def profiles_for(root:Path, name:str):
    folder=root/"usr/share/AnycubicSlicerNext/resources/profiles/Anycubic"
    stem=name
    paths={
      "machine":folder/"machine"/(stem+".json"),
      "process":folder/"process"/("0.20mm Standard @"+stem+".json"),
      "filament":folder/"filament"/("Anycubic PLA @"+stem+".json"),
    }
    config={}
    for kind,path in paths.items():
        if not path.is_file():
            raise ValueError("MISSING_"+kind.upper()+"_OFFICIAL_PROFILE "+str(path))
        values=json.loads(path.read_text(encoding="utf-8-sig"))
        if values.get("type")!=kind or values.get("name")!=path.stem:
            raise ValueError("MISMATCH_"+kind.upper()+"_TYPE_OR_NAME "+
                             str(values.get("type"))+"/"+str(values.get("name")))
        config[kind]=values
    machine=config["machine"]
    if "0.4" not in machine.get("name",""):
        raise ValueError("WRONG_NOZZLE_NAME")
    nozzles=machine.get("nozzle_diameter")
    if nozzles and nozzles!=["0.4"] and nozzles!="0.4":
        raise ValueError("NOZZLE_NOT_0_4 "+str(nozzles))
    for kind in ("process","filament"):
        compatible=config[kind].get("compatible_printers",[])
        if compatible and name not in compatible:
            raise ValueError(kind.upper()+"_NOT_COMPATIBLE_WITH_SELECTED_MACHINE")
    return paths,config


def official_library_dirs(root: Path) -> list[Path]:
    """Bundled shared libs from SHA-pinned original OEM .deb (not system installs)."""
    if not root.is_dir():
        raise ValueError("OFFICIAL_EXTRACT_ROOT_MISSING")
    libs=sorted(p for p in root.rglob("*.so*") if p.is_file() and
                (p.name.endswith(".so") or ".so." in p.name))
    if not libs:
        raise ValueError("OFFICIAL_VENDOR_SHARED_LIBRARIES_MISSING")
    return sorted({p.parent for p in libs})


def run(exe:Path, root:Path, paths:dict, cube:Path, out:Path, letter, timeout):
    directory=out/("run-"+letter)
    if directory.exists():shutil.rmtree(directory)
    directory.mkdir(parents=True)
    args=["--debug","3","--slice","0",
          "--load-settings",str(paths["machine"])+";"+str(paths["process"]),
          "--load-filaments",str(paths["filament"]),
          "--outputdir",str(directory),str(cube.resolve())]
    env=os.environ.copy()
    env["GDK_BACKEND"]="x11"
    env["LIBGL_ALWAYS_SOFTWARE"]="1"
    vendor_dirs=official_library_dirs(root)
    env["LD_LIBRARY_PATH"]=":".join(str(p) for p in vendor_dirs)+(
        ":"+env["LD_LIBRARY_PATH"] if env.get("LD_LIBRARY_PATH") else "")
    command=["xvfb-run","-a",str(exe),*args]
    log=out/("official-"+letter+".log")
    with log.open("w",encoding="utf-8",errors="replace") as fp:
        fp.write("COMMAND_JSON="+json.dumps(command,ensure_ascii=False)+"\n")
        fp.flush()
        try:
            code=subprocess.run(command,cwd=exe.parent,env=env,
                                stdout=fp,stderr=subprocess.STDOUT,
                                timeout=timeout,check=False).returncode
        except subprocess.TimeoutExpired:
            code=124
            fp.write("\nQA_TIMEOUT\n")
        except OSError as err:
            code=127
            fp.write("\nQA_OSERROR="+repr(err)+"\n")
    gcodes=sorted(p for p in directory.rglob("*") if p.is_file() and
                   p.suffix.lower() in (".gcode",".gco") and p.stat().st_size>0)
    proof={"run":letter,"exit_code":code,"gcodes":[
      {"path":str(p.relative_to(out)),"bytes":p.stat().st_size,
       "sha256":sha(p)} for p in gcodes],
       "log":str(log.relative_to(out))}
    (out/("inventory-"+letter+".json")).write_text(
      json.dumps(proof,indent=2)+"\n",encoding="utf-8")
    return proof


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--cache",type=Path,default=Path(".anycubic-vendor-cache"))
    parser.add_argument("--out",type=Path,default=Path("qa-anycubic-kobra-ab"))
    parser.add_argument("--timeout",type=int,default=180)
    parser.add_argument("--stress-count",type=int,default=0,help="additional independent original CLI runs per model; 0 for regular A/B")
    args=parser.parse_args()
    cache=args.cache.resolve();out=args.out.resolve()
    cache.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
    rows=[]
    def add(model,stage,status,detail,evidence=""):
        assert status in ("PASS","FAIL","PENDING")
        rows.append({"model":model,"stage":stage,"status":status,
                     "detail":detail,"evidence":evidence})
        print("ANYCUBIC_KOBRA",model,stage,status,detail,flush=True)
    try:
        req=urllib.request.Request(PACKAGES,headers={"User-Agent":"Filuna-OEM-QA/1"})
        with urllib.request.urlopen(req,timeout=60) as fp:
            text=fp.read(5*1024*1024+1)
        if len(text)>5*1024*1024:
            raise ValueError("VENDOR_APT_INDEX_TOO_LARGE")
        info=find_exact_package(apt_records(text.decode("utf-8")))
        package=cache/Path(FILENAME).name
        if not package.exists() or package.stat().st_size!=BYTES or digest(package)!=SHA256:
            package.unlink(missing_ok=True)
            download(APT_ROOT+FILENAME,package,BYTES+1)
        if package.stat().st_size!=BYTES or digest(package)!=SHA256:
            raise ValueError("OFFICIAL_PACKAGE_HASH_OR_SIZE_MISMATCH")
        add("common","OFFICIAL_PACKAGE_SHA","PASS",SHA256)
        folder=cache/"extracted"
        exe=folder/"usr/bin/AnycubicSlicerNext"
        if not exe.is_file():
            folder.mkdir(parents=True,exist_ok=True)
            with (out/"dpkg-extract.log").open("w") as fp:
                code=subprocess.run(["dpkg-deb","-x",str(package),str(folder)],
                                    stdout=fp,stderr=subprocess.STDOUT,timeout=180,
                                    check=False).returncode
            if code:raise ValueError("DPKG_EXTRACT_FAILED_"+str(code))
        version=folder/"usr/share/AnycubicSlicerNext/resources/build-version.txt"
        if not version.is_file() or version.read_text().strip()!=APP_VERSION:
            raise ValueError("EXTRACTED_APP_VERSION_NOT_VERIFIED")
        if not exe.is_file():raise ValueError("OFFICIAL_CLI_MISSING")
        exe.chmod(exe.stat().st_mode|0o111)
        add("common","OFFICIAL_APP_EXTRACTED","PASS",APP_VERSION)
        with (out/"official-runtime-dependencies.log").open("w") as fp:
            subprocess.run(["ldd",str(exe)],stdout=fp,stderr=subprocess.STDOUT,
                           timeout=40,check=False)
        vendor_dirs=official_library_dirs(folder)
        (out/"vendor-libraries.lock.json").write_text(
            json.dumps({"trusted_archive_sha256":SHA256,
                        "library_dirs":[str(p.relative_to(folder)) for p in vendor_dirs],
                        "library_count":sum(1 for p in folder.rglob("*.so*") if p.is_file()),
                        "scope":"original verified OEM package, no network activation"},
                       ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
        add("common","OFFICIAL_BUNDLED_LIBRARIES","PASS",
            "count_dirs="+str(len(vendor_dirs))+" libcloud_mqtt="+
            str(any((p/"libcloud_mqtt.so").is_file() for p in vendor_dirs)),
            "vendor-libraries.lock.json")
        # Whole manufacturer catalog, not a whitelist of the two QA fixtures.
        # The pilot A/B models below are samples only; all original machine,
        # process and filament files are indexed and inherited generically.
        import sys
        sys.path.insert(0,str(ROOT/"FILUNA"/"tools"))
        from oem_universal_inventory import read_catalog
        entire=read_catalog(
            folder/"usr/share/AnycubicSlicerNext/resources/profiles/Anycubic")
        (out/"all-models-all-profiles.json").write_text(
            json.dumps(entire,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
        add("common","ALL_MODELS_CATALOG_INVENTORY","PASS",
            "machines="+str(len(entire["machines"]))+
            " profiles="+str(entire["profiles_per_type"])+
            " issues="+str(len(entire["issues"])),
            "all-models-all-profiles.json")
        add("common","ALL_PROFILES_INHERITANCE",
            "PASS" if not entire["issues"] else "PENDING",
            "issues="+str(len(entire["issues"]))+
            " (catalog presence is not proven compatibility)")
        cube=ROOT/"fixtures/cube20.stl"
        if not cube.is_file():raise ValueError("CUBE_FIXTURE_MISSING")
        for target_id,name in TARGETS.items():
            dest=out/target_id;dest.mkdir(parents=True,exist_ok=True)
            try:
                paths,values=profiles_for(folder,name)
                (dest/"profile-lock.json").write_text(
                    json.dumps({"name":name,"version":APP_VERSION,"sha256_archive":SHA256,
                                "profiles":{k:{"relative":str(p.relative_to(folder)),
                                                "sha256":sha(p),
                                                "inherits":values[k].get("inherits")}
                                            for k,p in paths.items()}},indent=2,ensure_ascii=False)
                    +"\n",encoding="utf-8")
                add(target_id,"EXACT_MODEL_PRESET","PASS","machine/process/PLA names and compatibility",
                    str((dest/"profile-lock.json").relative_to(out)))
                results=[]
                for letter in ("A","B"):
                    item=run(exe,folder,paths,cube,dest,letter,args.timeout)
                    if item["exit_code"]!=0 or len(item["gcodes"])!=1:
                        add(target_id,"OFFICIAL_"+letter,"FAIL",
                            "CLI_EXIT="+str(item["exit_code"])+" GCODE_COUNT="+str(len(item["gcodes"])),
                            str((dest/("official-"+letter+".log")).relative_to(out)))
                        break
                    g=item["gcodes"][0]
                    results.append(g)
                    add(target_id,"OFFICIAL_"+letter,"PASS",
                        "bytes="+str(g["bytes"])+" sha256="+g["sha256"],g["path"])
                if len(results)<2:
                    add(target_id,"OFFICIAL_A_B","PENDING","TWO_SUCCESSFUL_CLI_RUNS_NOT_AVAILABLE")
                elif results[0]["path"]==results[1]["path"]:
                    add(target_id,"OFFICIAL_A_B","FAIL","GCODE_PATH_REUSED")
                else:
                    # run() serializes paths relative to the PER-MODEL dest,
                    # not the global QA root. A/B files are separate real OEM
                    # outputs; fail closed on missing or reused files.
                    input_a=(dest/results[0]["path"]).resolve()
                    input_b=(dest/results[1]["path"]).resolve()
                    if input_a==input_b or not input_a.is_relative_to(dest.resolve()) or not input_b.is_relative_to(dest.resolve()):
                        raise ValueError("UNSAFE_OR_REUSED_OFFICIAL_AB_PATH")
                    if not input_a.is_file() or not input_b.is_file():
                        raise FileNotFoundError(
                            "OFFICIAL_GCODE_PATH_RESOLUTION_MISMATCH "+
                            json.dumps({"destination":str(dest),
                                        "a":str(input_a),"b":str(input_b),
                                        "a_present":input_a.is_file(),
                                        "b_present":input_b.is_file()}))
                    if sha(input_a)!=results[0]["sha256"] or sha(input_b)!=results[1]["sha256"]:
                        raise ValueError("OFFICIAL_OUTPUT_FILE_CHANGED_BEFORE_AB_COMPARISON")
                    add(target_id,"OUTPUT_FILE_PROVENANCE","PASS",
                        "distinct_files_verified_sha256_and_paths")
                    proof=compare_files(input_a,input_b,
                                        dest/"strict-original-ab-proof.json")
                    outcome=proof["status"]
                    passed=outcome.startswith("PASS_")
                    add(target_id,"OFFICIAL_A_B","PASS" if passed else "FAIL",
                        outcome+" normalized_sha256="+proof.get("normalized_sha256","")+
                        " changed_lines="+str(proof.get("changed_lines",[])),
                        "strict-original-ab-proof.json")
                if args.stress_count:
                    if not 1<=args.stress_count<=50:
                        raise ValueError("STRESS_COUNT_OUT_OF_ALLOWED_RANGE")
                    if len(results)<2 or len(item["gcodes"])!=1:
                        add(target_id,"STRESS_GCODE","PENDING","REFERENCE_A_B_NOT_READY")
                    else:
                        base=(dest/results[0]["path"]).resolve()
                        stress_results=[]
                        for n in range(args.stress_count):
                            label="stress-"+str(n).zfill(2)
                            result=run(exe,folder,paths,cube,dest,label,args.timeout)
                            if result["exit_code"] or len(result["gcodes"])!=1:
                                stress_results.append({"run":label,"status":"FAIL_OEM_CLI",
                                                       "exit":result["exit_code"]})
                                continue
                            gc=(dest/result["gcodes"][0]["path"]).resolve()
                            if gc==base or not gc.is_file():
                                stress_results.append({"run":label,"status":"FAIL_INPUT_OUTPUT_PATH"})
                                continue
                            proof=compare_files(base,gc,dest/("strict-"+label+"-proof.json"))
                            stress_results.append({
                                "run":label,"status":proof["status"],"gcode_sha256":result["gcodes"][0]["sha256"],
                                "bytes":result["gcodes"][0]["bytes"],
                                "normalized_sha256":proof.get("normalized_sha256","")
                            })
                        (dest/"original-cli-stress-results.json").write_text(
                            json.dumps(stress_results,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
                        allpass=(
                            len(stress_results)==args.stress_count and
                            all(q["status"].startswith("PASS_") for q in stress_results)
                        )
                        add(target_id,"STRESS_GCODE",
                            "PASS" if allpass else "FAIL",
                            "runs="+str(len(stress_results))+" all_equal_header_only="+str(allpass),
                            "original-cli-stress-results.json")
            except Exception as exc:
                add(target_id,"MODEL_RUNNER","FAIL",repr(exc))
            add(target_id,"FILUNA_PARITY","PENDING","NO_SEPARATE_FILUNA_NATIVE_CLI_OUTPUT")
            add(target_id,"NETWORK","PENDING","NO_APP_DEVICE_PROTOCOL_SIMULATOR_PROOF")
            add(target_id,"PHYSICAL_BETA","PENDING","ONLY_AFTER_FINAL_BUILD_AND_SUPERVISED_BETA")
    except Exception as exc:
        add("common","RUNNER","FAIL",type(exc).__name__+": "+str(exc))
    (out/"results.json").write_text(json.dumps(rows,indent=2,ensure_ascii=False)+"\n")
    (out/"RESUMO.md").write_text("# Anycubic original 2.0.0.6 — Kobra S1, Kobra 3 V2, Kobra 4 official CLI A/B\n\n"
      "Original verified .deb extraction on isolated Linux; not an official portable ZIP.\n"
      "No physical print, Filuna parity, ACE authentication, or vendor recompilation.\n\n"
      "| Model | Gate | Status | Detail |\n|---|---|---|---|\n"
      + "".join("| "+r["model"]+" | "+r["stage"]+" | "+r["status"]+" | "+
                str(r["detail"]).replace("|","/")+" |\n" for r in rows),encoding="utf-8")
    return 1 if any(x["status"]=="FAIL" for x in rows) else 0


if __name__=="__main__":
    raise SystemExit(main())
