#!/usr/bin/env python3
"""Filuna independent G-code trace comparator (dry run; NEVER executes printer commands).

Reference must be a real, separately generated ElegooSlicer G-code with provenance.
Identical metadata does not imply equivalence; comparison uses executable G/M/T trace,
XYZE/F motion, thermal/fan commands and bounds. Unsafe/unknown machine-specific commands
block certification until reviewed. This is NOT a firmware simulator or safety proof.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
import re
from pathlib import Path

MOVE={"G0","G1","G2","G3"}
KNOWN={"G0","G1","G2","G3","G4","G10","G11","G20","G21","G28","G29","G90","G91","G92",
       "M0","M1","M17","M18","M20","M21","M24","M25","M26","M27","M28","M29","M30",
       "M42","M73","M75","M76","M77","M82","M83","M84","M104","M105","M106","M107",
       "M109","M110","M112","M114","M115","M117","M118","M119","M140","M141","M142",
       "M190","M191","M200","M201","M203","M204","M205","M206","M207","M208","M209",
       "M220","M221","M400","M420","M500","M501","M502","M503","M600","M900"}
WORD=re.compile(r"([A-Z])\s*([-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?)")
CMD=re.compile(r"^\s*(?:N\d+\s+)?([GMT]\d+(?:\.\d+)?)\b",re.I)
class ParseError(ValueError):pass
def canonical_command(s):
    m=CMD.match(s)
    if not m:return None
    cmd=m.group(1).upper()
    letter=cmd[0]
    try:
        num=float(cmd[1:])
        return f"{letter}{int(num)}" if num.is_integer() else cmd
    except ValueError:return cmd
def read_trace(path:Path,bed_x:float,bed_y:float,max_z:float)->dict:
    xyz={"X":0.0,"Y":0.0,"Z":0.0,"E":0.0}
    absolute_xyz=True;absolute_e=True;units_mm=True
    moves=[];controls=[];issues=[];counts={};travel=0.0;extruded=0.0
    minmax={k:[float("inf"),float("-inf")] for k in ("X","Y","Z")}
    for line_no,line in enumerate(path.open(encoding="utf-8",errors="replace"),1):
        if line_no>2000000:raise ParseError("line limit exceeded")
        raw=line.split(";",1)[0].strip()
        if not raw or raw.startswith("("):continue
        command=canonical_command(raw)
        if command is None:
            issues.append({"line":line_no,"issue":"unrecognized-line","text":raw[:100]})
            continue
        counts[command]=counts.get(command,0)+1
        words={k:float(v) for k,v in WORD.findall(raw[len(CMD.match(raw).group(0)):].upper())}
        if command=="G20":units_mm=False;issues.append({"line":line_no,"issue":"inch-units"})
        if command=="G21":units_mm=True
        if command=="G90":absolute_xyz=True
        if command=="G91":absolute_xyz=False
        if command=="M82":absolute_e=True
        if command=="M83":absolute_e=False
        if command=="G92":
            for axis in xyz:
                if axis in words:xyz[axis]=words[axis]
        if command in MOVE:
            old=dict(xyz)
            for axis in xyz:
                if axis in words:
                    xyz[axis]=words[axis] if (absolute_e if axis=="E" else absolute_xyz) else xyz[axis]+words[axis]
            distance=math.dist([old["X"],old["Y"],old["Z"]],[xyz["X"],xyz["Y"],xyz["Z"]])
            travel+=distance
            extruded+=max(0.0,xyz["E"]-old["E"])
            for axis in ("X","Y","Z"):
                minmax[axis][0]=min(minmax[axis][0],xyz[axis])
                minmax[axis][1]=max(minmax[axis][1],xyz[axis])
            if "F" in words and words["F"]<=0:issues.append({"line":line_no,"issue":"nonpositive-feed"})
            if command in ("G2","G3"):issues.append({"line":line_no,"issue":"arc-requires-firmware-validation"})
            moves.append({"cmd":command,**{k:round(xyz[k],5) for k in xyz},
                          "F":round(words.get("F",-1),4)})
        else:
            controls.append((command,tuple(sorted((k,round(v,5)) for k,v in words.items()))))
        if command not in KNOWN and not command.startswith("T"):
            issues.append({"line":line_no,"issue":"machine-specific-or-unknown","cmd":command})
        if command in ("M104","M109") and words.get("S",0)>350:
            issues.append({"line":line_no,"issue":"nozzle-temp-above-350C"})
        if command in ("M140","M190") and words.get("S",0)>110:
            issues.append({"line":line_no,"issue":"bed-temp-above-110C"})
        if any(not math.isfinite(v) for v in words.values()):
            issues.append({"line":line_no,"issue":"nonfinite-parameter"})
    for axis,maximum in (("X",bed_x),("Y",bed_y),("Z",max_z)):
        low,high=minmax[axis]
        if low!=float("inf") and (low<0 or high>maximum):
            issues.append({"issue":"out-of-bounds","axis":axis,"min":low,"max":high,"limit":maximum})
    def digest(obj):
        return hashlib.sha256(json.dumps(obj,separators=(",",":"),ensure_ascii=True).encode()).hexdigest()
    return {"file_sha256":hashlib.sha256(path.read_bytes()).hexdigest(),
            "lines_of_motion":len(moves),"control_commands":len(controls),
            "commands":counts,"travel_mm":round(travel,3),"positive_extrusion_mm":round(extruded,3),
            "bounds_mm":{k:v for k,v in minmax.items() if v[0]!=float("inf")},
            "motion_sha256":digest(moves),"control_sha256":digest(controls),
            "motions":moves,"controls":controls,"issues":issues}
def compare(a:dict,b:dict)->dict:
    keys=("lines_of_motion","control_commands","motion_sha256","control_sha256")
    differences={k:{"official":a[k],"filuna":b[k]} for k in keys if a[k]!=b[k]}
    for k in ("travel_mm","positive_extrusion_mm","bounds_mm","commands"):
        if a[k]!=b[k]:differences[k]={"official":a[k],"filuna":b[k]}
    for kind in ("motions","controls"):
        if a[kind]!=b[kind]:
            first=next((i for i,(x,y) in enumerate(zip(a[kind],b[kind])) if x!=y),
                       min(len(a[kind]),len(b[kind])))
            differences[kind+"_first_mismatch"]={"index":first,
                "official":a[kind][first] if first<len(a[kind]) else None,
                "filuna":b[kind][first] if first<len(b[kind]) else None}
    return differences
def main():
    p=argparse.ArgumentParser()
    p.add_argument("--official",type=Path,required=True)
    p.add_argument("--filuna",type=Path,required=True)
    p.add_argument("--official-provenance",required=True,help="official slicer release and profile, not Filuna/Orca")
    p.add_argument("--filuna-provenance",required=True)
    p.add_argument("--bed-x",type=float,required=True)
    p.add_argument("--bed-y",type=float,required=True)
    p.add_argument("--max-z",type=float,required=True)
    p.add_argument("--report",type=Path,required=True)
    a=p.parse_args()
    if a.official.resolve()==a.filuna.resolve():
        raise SystemExit("INVALID TEST: official and Filuna must be different source files")
    if not a.official.is_file() or not a.filuna.is_file():
        raise SystemExit("Both real G-code files are required; no synthetic reference accepted")
    o=read_trace(a.official,a.bed_x,a.bed_y,a.max_z)
    f=read_trace(a.filuna,a.bed_x,a.bed_y,a.max_z)
    diff=compare(o,f)
    provenance_verified = ("SYNTHETIC" not in a.official_provenance.upper() and
                           "SYNTHETIC" not in a.filuna_provenance.upper() and
                           "ELEGOO" in a.official_provenance.upper() and
                           "FILUNA" in a.filuna_provenance.upper())
    identical=(not diff and not o["issues"] and not f["issues"] and provenance_verified)
    report={"scope":"offline-trace-only-not-firmware-simulation-not-hardware-safety",
        "official_provenance":a.official_provenance,"filuna_provenance":a.filuna_provenance,
        "printer_bounds_mm":{"x":a.bed_x,"y":a.bed_y,"z":a.max_z},
        "status":"PARTIAL_TRACE_EQUIVALENT" if identical else "BLOCKED_DIFFERENCES_OR_UNSUPPORTED",
        "provenance_text_passed_basic_checks":provenance_verified,
        "provenance_is_independently_verified":False,
        "is_hardware_homologation":False,"is_release_evidence":False,
        "official":{k:v for k,v in o.items() if k not in ("motions","controls")},
        "filuna":{k:v for k,v in f.items() if k not in ("motions","controls")},
        "differences":diff}
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps(report,indent=2,ensure_ascii=False)+"\n")
    print(report["status"],"differences",len(diff),"official issues",len(o["issues"]),"filuna issues",len(f["issues"]))
    return 0 if identical else 1
if __name__=="__main__":raise SystemExit(main())
