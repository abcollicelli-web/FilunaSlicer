#!/usr/bin/env python3
"""Audit an OEM printer preset subtree against the Filuna vendor bundle.
No mutation of either input tree. Fail closed on inheritance gaps or differences.
AGPL-3.0-or-later.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path

def load(path: Path):
    return json.loads(path.read_text(encoding="utf-8-sig"))

def resolve(name: str, family: str, base: Path, chain=None):
    if chain is None: chain=[]
    if name in chain: raise ValueError("preset inheritance cycle: "+ " -> ".join(chain+[name]))
    path=base/family/(name+".json")
    if not path.is_file(): raise FileNotFoundError(str(path))
    data=load(path)
    parent=data.get("inherits")
    out={}
    if parent: out.update(resolve(parent, family, base,chain+[name]))
    # Preserve process data and machine settings; exclude only descriptive metadata
    for k,v in data.items():
        if k not in ("name","inherits","description","setting_id","from","type","instantiation","compatible_printers","compatible_printers_condition","filament_settings_id","print_settings_id"):
            out[k]=v
    return out

def analyze(a: Path,b:Path,selection:list[tuple[str,str]]):
    records=[]
    for family,name in selection:
        try:
            official=resolve(name,family,a)
            filuna=resolve(name,family,b)
            differing={}
            for k in sorted(official.keys()|filuna.keys()):
                if official.get(k)!=filuna.get(k):
                    differing[k]={"official":official.get(k),"filuna":filuna.get(k)}
            records.append({"family":family,"preset":name,"status":"PASS" if not differing else "MISMATCH","differences":differing})
        except (ValueError,FileNotFoundError,KeyError) as exc:
            records.append({"family":family,"preset":name,"status":"FAIL","error":str(exc)})
    return {"kind":"A1 effective preset parity not print certification", "records":records,
            "status":"PASS" if all(r["status"]=="PASS" for r in records) else "MISMATCH",
            "physical_printer_tested":False,"filuna_module_activated":False}
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--official",type=Path,required=True)
    ap.add_argument("--filuna",type=Path,required=True)
    ap.add_argument("--out",type=Path)
    args=ap.parse_args()
    result=analyze(args.official,args.filuna,[
        ("machine","Bambu Lab A1 0.4 nozzle"),
        ("filament","Bambu PLA Basic @BBL A1"),
        ("process","0.20mm Standard @BBL A1")])
    if args.out:args.out.write_text(json.dumps(result,indent=2,ensure_ascii=False)+"\n")
    summary=[{"name":r["preset"],"status":r["status"],"diff_keys":list(r.get("differences",{})),"error":r.get("error")} for r in result["records"]]
    print(json.dumps({"status":result["status"],"records":summary},ensure_ascii=False))
    return 0 if result["status"]=="PASS" else 1
if __name__=="__main__":raise SystemExit(main())
