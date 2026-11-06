#!/usr/bin/env python3
"""Bambu E5/E8 byte-exact G-code comparison.

Report four hashes (official Windows RAW, compiled Windows RAW, both with
ONLY the generator timestamp line removed). Preserve generator identity,
version, estimated print time, comments, config, executable commands and all
other bytes. If the inputs are .gcode.3mf archives, compare extracted
Metadata/plate_1.gcode and audit every internal ZIP member separately.
ZIP container bytes are NOT an equality criterion: ZIP timestamps vary.
"""
from __future__ import annotations
import argparse, hashlib, itertools, json, re, zipfile
from pathlib import Path

# Strict, explicit generator date/time comment formats ONLY. Generic
# "model printing time", program identity and layer times MUST be retained.
TIMESTAMP = re.compile(
    rb"^;\s*(?:generated (?:at|on)|generation (?:date|time|timestamp)|"
    rb"created (?:at|on)|creation (?:date|time)|file generated at)\s*[:=]?\s*"
    rb"(?:\d{4}[-/]\d{1,2}[-/]\d{1,2}|\d{1,2}[-/]\d{1,2}[-/]\d{4})"
    rb"(?:[ T]\d{1,2}:\d{2}(?::\d{2})?(?:\.\d+)?(?:\s*(?:UTC|Z|[+-]\d{2}:?\d{2}))?)?"
    rb"\s*$", re.I
)
GCODE_ENTRY = "Metadata/plate_1.gcode"

def lines_for(path: Path):
    """Binary-safe line iteration. Keep exact line-ending bytes."""
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as z:
            members=z.namelist()
            if GCODE_ENTRY not in members:
                raise ValueError(f"{path.name}: ZIP missing {GCODE_ENTRY}")
            with z.open(GCODE_ENTRY) as f:
                yield from f
    else:
        with path.open("rb") as f:
            yield from f

def code_hash(path: Path):
    sha=hashlib.sha256()
    for raw in lines_for(path):
        sha.update(raw)
    return sha.hexdigest()

def normalized(path: Path):
    sha=hashlib.sha256()
    header=False
    dropped=[]
    for number,raw in enumerate(lines_for(path),1):
        line=raw.rstrip(b"\r\n")
        if line == b"; HEADER_BLOCK_START":
            header=True
        if header and TIMESTAMP.fullmatch(line):
            dropped.append({"line":number,"text":line.decode("utf-8","replace")})
        else:
            sha.update(raw)
        if line == b"; HEADER_BLOCK_END":
            header=False
    return sha.hexdigest(),dropped

def retained(path: Path):
    header=False
    for number,raw in enumerate(lines_for(path),1):
        l=raw.rstrip(b"\r\n")
        if l==b"; HEADER_BLOCK_START":
            header=True
        if not (header and TIMESTAMP.fullmatch(l)):
            yield number,raw
        if l==b"; HEADER_BLOCK_END":
            header=False

def difference(a: Path,b: Path):
    for left,right in itertools.zip_longest(retained(a),retained(b)):
        if left is None or right is None or left[1]!=right[1]:
            return {"official_line":left[0] if left else None,
                    "official_text":left[1].decode("utf-8","replace").rstrip("\r\n")[:240] if left else None,
                    "compiled_line":right[0] if right else None,
                    "compiled_text":right[1].decode("utf-8","replace").rstrip("\r\n")[:240] if right else None}
    return None

def zip_members(path: Path):
    if not zipfile.is_zipfile(path):return None
    d={}
    with zipfile.ZipFile(path) as z:
        for info in z.infolist():
            if info.is_dir():continue
            sha=hashlib.sha256()
            with z.open(info) as f:
                for chunk in iter(lambda:f.read(1024*1024),b""):
                    sha.update(chunk)
            d[info.filename]={"sha256":sha.hexdigest(),"bytes":info.file_size}
    return d

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--official",type=Path,required=True)
    p.add_argument("--filuna",type=Path,required=True)
    p.add_argument("--out",type=Path,required=True)
    p.add_argument("--gate",choices=["B","C","NEGATIVE"],default="NEGATIVE")
    p.add_argument("--candidate-is-integrated-oem",action="store_true")
    args=p.parse_args()
    raw_a=code_hash(args.official);raw_b=code_hash(args.filuna)
    norm_a,lines_a=normalized(args.official)
    norm_b,lines_b=normalized(args.filuna)
    ea=zip_members(args.official);eb=zip_members(args.filuna)
    internal=None
    if ea is not None or eb is not None:
        a=ea or {};b=eb or {}
        internal={k:{"official":a.get(k),"compiled":b.get(k),
                     "same_sha256":a.get(k,{}).get("sha256")==b.get(k,{}).get("sha256")
                       if k in a and k in b else False}
                  for k in sorted(set(a)|set(b))}
    result={
      "gate":args.gate,"criterion":"exact G-code SHA256, remove ONLY generator timestamp line in HEADER_BLOCK",
      "official_source":args.official.name,"compiled_source":args.filuna.name,
      "official_win_sha256_raw":raw_a,"compiled_win_sha256_raw":raw_b,
      "official_win_sha256_normalized":norm_a,"compiled_win_sha256_normalized":norm_b,
      "official_timestamp_lines_removed":lines_a,
      "compiled_timestamp_lines_removed":lines_b,
      "raw_identical":raw_a==raw_b,"normalized_identical":norm_a==norm_b,
      "first_difference_after_normalization":difference(args.official,args.filuna),
      "package_member_comparison":internal,
      "package_container_hash_ignored":ea is not None or eb is not None
    }
    if args.gate=="B":
        result["e8_acceptance"]="PASS" if norm_a==norm_b else "FAIL"
    elif args.gate=="C":
        result["e8_acceptance"]="PASS" if args.candidate_is_integrated_oem and norm_a==norm_b else "FAIL" if args.candidate_is_integrated_oem else "NOT_VALID_E8_CANDIDATE"
    else:
        result["e8_acceptance"]="NEGATIVE_CONTROL"
    # Keep old keys used by existing QA scripts.
    result["raw_official_sha256"]=raw_a
    result["raw_candidate_sha256"]=raw_b
    result["normalized_official_sha256"]=norm_a
    result["normalized_candidate_sha256"]=norm_b
    result["byte_equal_after_allowed_normalization"]=norm_a==norm_b
    result["first_difference"]=result["first_difference_after_normalization"]
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(result,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(result,ensure_ascii=False,indent=2))
    return 0 if result["e8_acceptance"]=="PASS" else 1

if __name__=="__main__":
    raise SystemExit(main())
