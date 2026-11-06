#!/usr/bin/env python3
"""Compare official Bambu A1 macro source to Filuna, without modifying either.
Exit 1 on differences, missing components, or malformed JSON.
Official input is an explicitly supplied unpacked release or verified source snapshot.
"""
import argparse
import hashlib
import json
from pathlib import Path

def check(manifest, official_root, filuna_root):
    results=[]
    for item in manifest["components"]:
        rel=Path(item["path"])
        a=official_root/rel
        b=filuna_root/rel
        row={"path":str(rel),"upstream_blob_sha_reference":item["upstream_blob_sha"]}
        try:
            aa=json.loads(a.read_text(encoding="utf-8-sig"))
            bb=json.loads(b.read_text(encoding="utf-8-sig"))
            commands={k for k in aa if k.endswith("_gcode")}
            if not commands:
                raise ValueError("no *_gcode entries in official")
            unmatched=[k for k in commands if aa[k]!=bb.get(k)]
            row.update(official_content_sha256=hashlib.sha256(a.read_bytes()).hexdigest(),
                       filuna_content_sha256=hashlib.sha256(b.read_bytes()).hexdigest(),
                       macro_keys=sorted(commands), unequal_macro_keys=unmatched,
                       status="PASS" if not unmatched else "MISMATCH")
        except (OSError,ValueError,TypeError,KeyError) as e:
            row.update(status="FAIL",error=str(e))
        results.append(row)
    return {"gate":"BAMBU_A1_OEM_MACRO_PARITY","status":"PASS" if all(x["status"]=="PASS" for x in results) else "FAIL",
            "results":results,"slicing_equivalent":False,"oem_module_loaded":False,
            "printer_hardware_tested":False}

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--manifest",type=Path,default=Path(__file__).with_name("bambu_a1_source_manifest.json"))
    p.add_argument("--official-root",type=Path,required=True)
    p.add_argument("--filuna-root",type=Path,required=True)
    p.add_argument("--out",type=Path)
    args=p.parse_args()
    result=check(json.loads(args.manifest.read_text()),args.official_root,args.filuna_root)
    if args.out:
        args.out.parent.mkdir(parents=True,exist_ok=True)
        args.out.write_text(json.dumps(result,indent=2,ensure_ascii=False)+"\n")
    print(json.dumps({"gate":result["gate"],"status":result["status"],
                       "results":[{"path":x["path"],"status":x["status"],"unequal_macro_keys":x.get("unequal_macro_keys")} for x in result["results"]]},
                     ensure_ascii=False))
    return 0 if result["status"]=="PASS" else 1

if __name__=="__main__":
    raise SystemExit(main())
