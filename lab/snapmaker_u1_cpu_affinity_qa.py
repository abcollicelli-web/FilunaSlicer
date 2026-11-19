#!/usr/bin/env python3
"""Isolate Snapmaker 2.4.1 U1 original CLI G-code drift vs CPU parallel scheduling.

Portable original OEM ZIP, fixed STL, exact same effective OEM machine/process/
filament (only preexisting guarded process load). 24 independent runs with OS
default scheduling, 24 with ONE assigned Windows CPU processor. All actual G-code
is compared with header TIMESTAMP ONLY stripped; toolpath changes remain FAIL.
Setting CPU affinity is diagnostic only, not a Filuna production patch.
"""
from __future__ import annotations
import argparse
import ctypes
import json
import os
from pathlib import Path
import subprocess

from portable_first_qa import OFFICIAL,obtain,extract_executable,digest
from snapmaker_u1_inherit import build_effective_original_presets
from snapmaker_runtime_nozzle_guard_qa import guarded_runtime_process
from snapmaker_u1_temporal_ab_qa import header_date_only_normalization
ROOT=Path(__file__).resolve().parents[1]


def run(exe,machine,process,filament,folder,label,affinity):
    path=folder/label;path.mkdir(parents=True,exist_ok=True)
    command=[str(exe),"--debug","3","--slice","0","--load-settings",
             str(machine)+";"+str(process),"--load-filaments",
             ";".join([str(filament)]*4),"--outputdir",str(path),
             str((ROOT/"fixtures"/"cube20.stl").resolve())]
    log=folder/(label+".log")
    details={"case":label,"single_cpu_requested":affinity,
             "argv":command}
    with log.open("w",encoding="utf-8",errors="replace") as fp:
        fp.write("COMMAND_JSON="+json.dumps(command,ensure_ascii=False)+"\n")
        fp.flush()
        with subprocess.Popen(command,stdout=fp,stderr=subprocess.STDOUT,
                              cwd=exe.parent) as child:
            if affinity:
                handle=ctypes.c_void_p(int(child._handle))
                process_mask=ctypes.c_size_t()
                system_mask=ctypes.c_size_t()
                k=ctypes.windll.kernel32
                k.GetProcessAffinityMask.restype=ctypes.c_int
                k.GetProcessAffinityMask.argtypes=[
                    ctypes.c_void_p,ctypes.POINTER(ctypes.c_size_t),
                    ctypes.POINTER(ctypes.c_size_t)]
                k.SetProcessAffinityMask.restype=ctypes.c_int
                k.SetProcessAffinityMask.argtypes=[ctypes.c_void_p,ctypes.c_size_t]
                if not k.GetProcessAffinityMask(handle,ctypes.byref(process_mask),
                                                ctypes.byref(system_mask)):
                    details["affinity_error"]="GET_PROCESS_AFFINITY_FAILED"
                elif process_mask.value==0:
                    details["affinity_error"]="ALLOWED_CPU_MASK_EMPTY"
                else:
                    selected=process_mask.value & -process_mask.value
                    details["assigned_affinity_mask"]=hex(selected)
                    if not k.SetProcessAffinityMask(handle,ctypes.c_size_t(selected)):
                        details["affinity_error"]="SET_PROCESS_AFFINITY_FAILED"
                    else:
                        details["affinity_enforced"]=True
            try:
                child.wait(timeout=90)
            except subprocess.TimeoutExpired:
                child.kill();child.wait()
                details["exit"]=124
            else:
                details["exit"]=child.returncode
    gcodes=sorted(path.glob("*.gcode"))
    if details["exit"] or len(gcodes)!=1 or (affinity and not details.get("affinity_enforced")):
        details["status"]="FAIL_CLI_OR_UNENFORCED_SINGLE_CPU"
        return details
    g=gcodes[0]
    sig=header_date_only_normalization(g.read_bytes())
    details.update({"status":"PASS_GCODE_GENERATED","path":str(g.relative_to(folder)),
                    "bytes":g.stat().st_size,
                    "normalized_sha256":sig["normalized_sha256"],
                    "raw_sha256":sig["raw_sha256"],
                    "timestamp":sig["timestamp"]})
    return details


def main():
    p=argparse.ArgumentParser()
    p.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
    p.add_argument("--out",type=Path,default=Path("qa-snapmaker-u1-affinity"))
    p.add_argument("--runs-per-group",type=int,default=24)
    a=p.parse_args()
    if not 2<=a.runs_per_group<=75:raise ValueError("RUN_COUNT_OUT_OF_RANGE")
    out=a.out.resolve();cache=a.cache.resolve()
    out.mkdir(parents=True,exist_ok=True);cache.mkdir(parents=True,exist_ok=True)
    data={"scope":"ORIGINAL OEM PORTABLE ONLY; NO FILUNA NATIVE TEST",
          "experiment":"Windows CPU processor affinity vs original U1 CLI path variance",
          "groups":{}}
    try:
        archive,url=obtain(OFFICIAL["snapmaker"],cache)
        exe=extract_executable(archive,OFFICIAL["snapmaker"]["exe"],
                               cache/"snapmaker-portable-extracted").resolve()
        fs=build_effective_original_presets(out)
        proc=out/"one-identical-process.json"
        guard=guarded_runtime_process(fs["machine"],fs["process"],proc)
        data["upstream_package_sha256"]=digest(archive)
        data["exact_effective_profile_sha256"]={x:digest(y) for x,y in
            (("machine",fs["machine"]),("process",proc),("filament",fs["filament"]))}
        data["guard_provenance"]=guard
        all_good=True
        for key,enforce in (("normal-multicore",False),("one-cpu",True)):
            records=[]
            for i in range(a.runs_per_group):
                label=key+"-"+str(i).zfill(2)
                r=run(exe,fs["machine"],proc,fs["filament"],out,label,enforce)
                records.append(r)
                print("SNAPMAKER_CPU",label,r["status"],r.get("bytes"),
                      r.get("normalized_sha256",""),flush=True)
                if r["status"]!="PASS_GCODE_GENERATED":
                    all_good=False
                    break
            hashes={}
            for x in records:
                h=x.get("normalized_sha256")
                if h:hashes[h]=hashes.get(h,0)+1
            data["groups"][key]={"records":records,"hash_counts":hashes,
                                 "unique_non_timestamp_toolpaths":len(hashes),
                                 "all_probes_completed":len(records)==a.runs_per_group,
                                 "strict_repeatability":"PASS" if len(hashes)==1 and
                                    len(records)==a.runs_per_group and all(
                                    x["status"]=="PASS_GCODE_GENERATED" for x in records)
                                    else "FAIL"}
        data["result"]="PASS_SINGLE_CPU_STABLE_WITH_BASELINE_VARIANCE" if (
            all_good and data["groups"]["one-cpu"]["strict_repeatability"]=="PASS" and
            data["groups"]["normal-multicore"]["strict_repeatability"]=="FAIL") else (
            "PASS_BOTH_GROUPS_STABLE_IN_THIS_RUN" if all_good and all(
                v["strict_repeatability"]=="PASS" for v in data["groups"].values())
            else "DIAGNOSTIC_NO_CAUSAL_SUCCESS_OR_CLI_ERROR")
    except Exception as e:
        data["result"]="FAIL_RUNNER"
        data["exception"]=repr(e)
    (out/"result.json").write_text(json.dumps(data,ensure_ascii=False,indent=2)+"\n")
    (out/"RESUMO.md").write_text(
        "# U1 original CLI: CPU affinity controlled experiment\n\n"
        "Result: **"+data["result"]+"**. Original SHA, no Filuna native engine. "
        "All differences in actual G-code remain failures for homologation.\n")
    print("SNAPMAKER_CPU RESULT",data["result"],flush=True)
    return 0 if data["result"].startswith("PASS") else 1


if __name__=="__main__":
    raise SystemExit(main())
