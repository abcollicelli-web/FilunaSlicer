#!/usr/bin/env python3
"""Audit Bambu A1 official firmware capability matrix vs Orca runtime matrix.

No silent feature enabling. Any drift requires engineering review; a missing
property or safety-relevant change never becomes hardware PASS.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path

def audit(official: Path, runtime: Path):
    a=json.loads(official.read_text(encoding="utf-8-sig"))["00.00.00.00"]
    b=json.loads(runtime.read_text(encoding="utf-8-sig"))["00.00.00.00"]
    fields=[("print."+key,a.get("print",{}).get(key),b.get("print",{}).get(key))
            for key in sorted(set(a.get("print",{}))|set(b.get("print",{})))]
    fields.extend((key,a.get(key),b.get(key))
                  for key in sorted(set(a)|set(b)) if key!="print")
    differences=[{"field":key,"official":av,"filuna":bv}
                 for key,av,bv in fields if av!=bv]
    return {
      "model_id":a.get("model_id"),"official_serial_prefix":a.get("sn_prefix"),
      "official_sha256":hashlib.sha256(official.read_bytes()).hexdigest(),
      "runtime_sha256":hashlib.sha256(runtime.read_bytes()).hexdigest(),
      "status":"PASS" if not differences else "MISMATCH_REVIEW_REQUIRED",
      "differences":differences,
      "printer_firmware_tested":False,"does_not_auto_enable_capabilities":True}
def main():
    p=argparse.ArgumentParser()
    p.add_argument("--official",type=Path,required=True)
    p.add_argument("--runtime",type=Path,required=True)
    p.add_argument("--out",type=Path)
    args=p.parse_args()
    obj=audit(args.official,args.runtime)
    if args.out:
      args.out.parent.mkdir(parents=True,exist_ok=True)
      args.out.write_text(json.dumps(obj,indent=2,ensure_ascii=False)+"\n")
    print(json.dumps({"model_id":obj["model_id"],"status":obj["status"],
        "changed_fields":[x["field"] for x in obj["differences"]]},ensure_ascii=False))
    return 0 if obj["status"]=="PASS" else 1

if __name__=="__main__":
    raise SystemExit(main())
