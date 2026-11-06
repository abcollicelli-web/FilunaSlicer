#!/usr/bin/env python3
"""Filuna OEM QA: numeric G-code comparison by feature (no printer I/O).

Tracks modal XYZ/E/F and M204; parses FEATURE markers (not only TYPE).
Reports extrusion vs travel motion, linear distance, E per-feature,
retractions, positive Z-only travel (z-hop proxy), and feed/accel changes.
This analysis is NOT a physical-printer approval; commanded motion timings
are not firmware-dynamics estimates. G2/G3 arc length not modeled.
"""
from __future__ import annotations
import argparse
import collections
import hashlib
import json
import math
import re
from pathlib import Path

PARM = re.compile(r"([XYZEFS])\s*(-?(?:\d+(?:\.\d*)?|\.\d+))")
FEATURE = re.compile(r"^;\s*(?:FEATURE|TYPE):\s*(.+)", re.I)
def inspect(path: Path) -> dict:
    pos = {"X": 0.0, "Y": 0.0, "Z": 0.0, "E": 0.0}
    modal_f = None
    modal_a = None
    absolute_xyz = True
    relative_e = True
    feature = "Startup / no feature"
    features = collections.defaultdict(lambda: collections.Counter())
    meta = {}
    lines = path.read_text(encoding="utf-8",errors="replace").splitlines()
    current_block = ""
    for raw in lines:
        line = raw.strip()
        if "HEADER_BLOCK_START" in line: current_block="HEADER"
        if current_block == "HEADER" and line.startswith("; "):
            for term in ("model printing time:", "total layer number:", "max_z_height:",
                         "filament_density:", "filament_diameter:", "total filament length"):
                if term in line: meta[term] = line
        if "HEADER_BLOCK_END" in line: current_block=""
        fm=FEATURE.match(line)
        if fm: feature=fm.group(1).strip()
        if not line or line.startswith(";"):continue
        cmd = line.split(";",1)[0].strip()
        if not cmd:continue
        op = cmd.split(maxsplit=1)[0].upper()
        if op=="G90":absolute_xyz=True
        elif op=="G91":absolute_xyz=False
        elif op=="M82":relative_e=False
        elif op=="M83":relative_e=True
        elif op in ("G0","G1","G2","G3"):
            fields={m.group(1):float(m.group(2)) for m in PARM.finditer(cmd)}
            old=pos.copy()
            for k in ("X","Y","Z"):
                if k in fields:pos[k]=fields[k] if absolute_xyz else pos[k]+fields[k]
            if "E" in fields:
                pos["E"] = pos["E"]+fields["E"] if relative_e else fields["E"]
            e_delta=pos["E"]-old["E"]
            distance=math.dist((old["X"],old["Y"],old["Z"]),(pos["X"],pos["Y"],pos["Z"]))
            if "F" in fields:
                if modal_f is not None and not math.isclose(modal_f,fields["F"],abs_tol=1e-6):
                    features[feature]["feed_changes"]+=1
                modal_f=fields["F"]
            f=features[feature]
            f["commands"]+=1
            if op in ("G2","G3"): f["arc_commands"]+=1
            if e_delta>1e-8 and distance>1e-8:
                f["extrusion_moves"]+=1;f["extruded_E_mm"]+=e_delta;f["extrusion_path_mm"]+=distance
            elif e_delta < -1e-8:
                f["retractions"]+=1;f["retracted_E_mm"]+=abs(e_delta)
                if distance>1e-8:f["travel_mm"]+=distance
            else:
                f["travel_moves"]+=1;f["travel_mm"]+=distance
                if abs(pos["X"]-old["X"])<1e-8 and abs(pos["Y"]-old["Y"])<1e-8 and pos["Z"]>old["Z"]+1e-8:
                    f["z_up_only_moves"]+=1
        elif op=="G92":
            fields={m.group(1):float(m.group(2)) for m in PARM.finditer(cmd)}
            for k in pos:
                if k in fields:pos[k]=fields[k]
        elif op=="M204":
            # Accept M204 Snnn and older M204 Pnnn / Tnnn forms.
            fields={m.group(1):float(m.group(2)) for m in re.finditer(r"([SPT])\s*(-?(?:\d+(?:\.\d*)?|\.\d+))",cmd)}
            new=fields.get("S",fields.get("P",None))
            features[feature]["M204_commands"]+=1
            if new is not None and modal_a is not None and not math.isclose(new,modal_a,abs_tol=1e-6):
                features[feature]["M204_effective_changes"]+=1
            if new is not None:modal_a=new
    def round_values(d):
        return {k:round(v,5) if isinstance(v,float) else v for k,v in sorted(d.items())}
    return {"source":str(path),"sha256":hashlib.sha256(path.read_bytes()).hexdigest(),
            "bytes":path.stat().st_size,"features":{name:round_values(dict(data)) for name,data in sorted(features.items())},
            "header_fields":meta,"limitations":["Arc movement geometry not expanded","Feature labels are producer annotations",
             "M204 modal tracking does not reproduce acceleration planning",
             "Z-only rises include ordinary Z changes, not validated z-hop detection",
             "No per-feature firmware-estimated time in G-code: not inferred"]}

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--official",type=Path,required=True)
    p.add_argument("--orca",type=Path,required=True)
    p.add_argument("--out",type=Path,required=True)
    args=p.parse_args()
    a=inspect(args.official);b=inspect(args.orca)
    names=sorted(set(a["features"])|set(b["features"]))
    keys=["extrusion_moves","travel_moves","extruded_E_mm","extrusion_path_mm","travel_mm",
          "M204_commands","M204_effective_changes","feed_changes","retractions","z_up_only_moves","arc_commands"]
    rows=[]
    for name in names:
        lhs=a["features"].get(name,{})
        rhs=b["features"].get(name,{})
        rows.append({"feature":name,**{key:{"official":lhs.get(key,0),"orca":rhs.get(key,0),
            "delta":round(rhs.get(key,0)-lhs.get(key,0),5)} for key in keys}})
    result={"reference":"Bambu Studio official vs Orca base (not current Filuna OEM engine)",
            "official":a,"orca":b,"features_comparison":rows,
            "hardware_homologated":False,"slicing_engine_oem_loaded":False}
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(result,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    for row in rows:
        print(f'{row["feature"][:25]:25} G_ex={row["extrusion_moves"]["official"]}/{row["extrusion_moves"]["orca"]} '
              f'T={row["travel_moves"]["official"]}/{row["travel_moves"]["orca"]} '
              f'E={row["extruded_E_mm"]["official"]:.2f}/{row["extruded_E_mm"]["orca"]:.2f} '
              f'M204={row["M204_commands"]["official"]}/{row["M204_commands"]["orca"]}')
    return 0

if __name__=="__main__": raise SystemExit(main())
