#!/usr/bin/env python3
"""Prepare OFFICIAL Bambu A1-mini (0.4) profile files for a self-generated 3MF.

Source: checkout of the official Bambu Studio v02.09.01.70 commit.
Recursively resolve each "inherits" chain WITHOUT changing physical values.
Inputs are official files, not a customer's project. The official Windows CLI
then creates and slices a 3MF from tests/data/20mm_cube.obj.

This script must not be used to edit or replace the private 3MF fixture.
"""
import argparse
import hashlib
import json
from pathlib import Path

PRESETS = {
    "machine": "Bambu Lab A1 mini 0.4 nozzle",
    "process": "0.20mm Standard @BBL A1M",
    "filament": "Bambu PLA Basic @BBL A1M",
}

def resolve(folder: Path, name: str, stack: tuple[str,...]=()) -> dict:
    if name in stack:
        raise ValueError("Circular Bambu preset inheritance: "+repr(stack+(name,)))
    path=folder/(name+".json")
    item=json.loads(path.read_text(encoding="utf-8-sig"))
    parent=item.pop("inherits",None)
    data=resolve(folder,parent,stack+(name,)) if parent else {}
    data.update(item)
    return data

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--bambu-source",required=True,type=Path)
    ap.add_argument("--out",required=True,type=Path)
    args=ap.parse_args()
    root=args.bambu_source
    out=args.out
    out.mkdir(parents=True,exist_ok=True)
    profiles=root/"resources"/"profiles"/"BBL"
    manifest={}
    for category,name in PRESETS.items():
        data=resolve(profiles/category,name)
        output=out/(category+".json")
        output.write_text(json.dumps(data,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
        sha=hashlib.sha256(output.read_bytes()).hexdigest()
        manifest[category]={"preset":name,"keys":len(data),"sha256":sha}
    (out/"profile-fixture-manifest.json").write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+"\n")
    for name,item in manifest.items():
        print(f"A1MINI_{name.upper()}={item['sha256']} preset={item['preset']} keys={item['keys']}")

if __name__=="__main__":
    main()
