#!/usr/bin/env python3
"""Filuna vs official Bambu A1 — isolated LAN and G-code acceptance runner.

Executes the native Filuna OEM identity/module tests against one fixed
simulated device, then compares previously generated real slicer G-code.
Never opens a printer session, uploads a file, prints, or reports hardware PASS.
Run only in the isolated 10.77.0.0/24 test namespace.
"""
from __future__ import annotations
import argparse
import importlib.util
import json
import socket
import subprocess
import sys
import tempfile
from pathlib import Path

def read_compiler(root: Path, source: str, out: Path) -> tuple[int,str]:
    call=subprocess.run(["g++","-std=c++17","-Wall","-Wextra","-Werror",
                         str(root/"FILUNA/qa"/source),"-o",str(out)],
                        capture_output=True,text=True,timeout=35)
    return call.returncode,(call.stdout+call.stderr)[-1800:]

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--repo",type=Path,default=Path(__file__).resolve().parents[3])
    ap.add_argument("--official-gcode",type=Path,required=True)
    ap.add_argument("--filuna-gcode",type=Path,required=True)
    ap.add_argument("--out",type=Path,required=True)
    ap.add_argument("--fixture-host",default="10.77.0.12")
    ap.add_argument("--fixture-listen",default="10.77.0.1")
    args=ap.parse_args()
    if args.fixture_host!="10.77.0.12" or args.fixture_listen!="10.77.0.1":
        raise ValueError("refusing non-isolated LAN target")
    result={"kind":"Filuna/Orca/Bambu overlay vs official Bambu Studio",
            "physical_hardware_tested":False,"official_binary_reexecuted":False,
            "filuna_app_windows_rebuilt":False,"network":"isolated 10.77.0.0/24 emulator",
            "checks":[]}
    def record(name:str,status:str,detail:str):
        result["checks"].append({"name":name,"status":status,"detail":detail})
    with tempfile.TemporaryDirectory(prefix="filuna-bambu-qa-") as tmp:
        work=Path(tmp)
        c,msg=read_compiler(args.repo,"test_bambu_a1_module.cpp",work/"a1-module")
        record("native_module_compile","PASS" if c==0 else "FAIL",msg or "g++ C++17 -Werror")
        if c==0:
            v=subprocess.run([str(work/"a1-module")],capture_output=True,text=True,timeout=10)
            record("device_to_prepare_module","PASS" if v.returncode==0 else "FAIL",
                   (v.stdout+v.stderr)[-1000:])
        c,msg=read_compiler(args.repo,"test_bambu_a1_ssdp.cpp",work/"a1-ssdp")
        record("native_discovery_compile","PASS" if c==0 else "FAIL",msg or "g++ C++17 -Werror")
        if c==0:
            listener=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
            sender=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
            try:
                listener.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
                listener.bind((args.fixture_listen,2021))
                listener.settimeout(4)
                sender.sendto(b"M-SEARCH * HTTP/1.1\r\nST: urn:bambulab-com:device:3dprinter:1\r\n\r\n",
                              (args.fixture_host,1990))
                response,_=listener.recvfrom(8192)
                capture=work/"response.ssdp"
                capture.write_bytes(response)
                v=subprocess.run([str(work/"a1-ssdp"),str(capture)],
                                 capture_output=True,text=True,timeout=10)
                record("bambu_a1_ssdp_emulator","PASS" if v.returncode==0 else "FAIL",
                       (v.stdout+v.stderr)[-900:])
            except (OSError,subprocess.TimeoutExpired) as exc:
                record("bambu_a1_ssdp_emulator","FAIL",repr(exc))
            finally:
                sender.close();listener.close()
    module_path=args.repo/"FILUNA/qa/gcode/compare_oem_gcode.py"
    spec=importlib.util.spec_from_file_location("filuna_gcode_compare",module_path)
    mod=importlib.util.module_from_spec(spec)
    assert spec and spec.loader
    spec.loader.exec_module(mod)
    if not args.official_gcode.is_file() or not args.filuna_gcode.is_file():
        record("gcode_official_vs_filuna","NOT_RUN","missing one or both output files")
    else:
        result["gcode"]=mod.compare(args.official_gcode,args.filuna_gcode)
        record("gcode_official_vs_filuna",result["gcode"]["status"],
               f"official sha256={result['gcode']['official']['sha256']}; "
               f"Filuna sha256={result['gcode']['filuna']['sha256']}")
    checks=result["checks"]
    result["status"]="PASS_FOR_EMULATOR_AND_GCODE_ONLY" if all(x["status"]=="PASS" for x in checks)         and result.get("gcode",{}).get("status")=="PRELIMINARY_MATCH_NOT_HARDWARE_PASS"         else "HOMOLOGATION_BLOCKED"
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(result,indent=2,ensure_ascii=False)+"\n")
    print(json.dumps({"status":result["status"],"checks":checks},ensure_ascii=False))
    return 0 if result["status"]=="PASS_FOR_EMULATOR_AND_GCODE_ONLY" else 1

if __name__=="__main__":
    sys.exit(main())
