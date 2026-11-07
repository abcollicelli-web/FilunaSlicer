#!/usr/bin/env python3
"""Filuna OEM read-only G-code spatial safety gate.

Reads (never rewrites) generated G-code. This is a bounded geometric
risk detector, not a firmware simulator or a proof that every print is safe.
Only a risk also present in the official OEM reference can be classified as
manufacturer behavior. Unproven Filuna parity fails closed.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import re

NUM = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?"
WORD = re.compile(r"([XYZEFIJR])\s*(" + NUM + r")", re.I)
LAY = re.compile(r"(?:^|\s)(?:layer\s*:?\s*|layer_num\s*=?\s*)(\d+)", re.I)
FINAL = re.compile(r"final[\s_-]*purge|finish[\s_-]*wipe|final[\s_-]*wipe|FILUNA_FINAL_PURGE", re.I)
STOP = re.compile(r"stop printing object|EXCLUDE_OBJECT_END", re.I)
START = re.compile(r"start printing object|EXCLUDE_OBJECT_START", re.I)
SPIRAL = re.compile(r"z-hop.*spiral|spiral.*(?:z-hop|lift)|FILUNA_SPIRAL_HOP", re.I)
ACK = "ACEITO O RISCO DE COLISAO DA SNAPMAKER"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def policy_from_yaml(path: Path) -> dict:
    # JSON is a valid YAML 1.2 document. Using the JSON subset intentionally
    # avoids an executable YAML loader and third-party dependency.
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data.get("manufacturer"), str) or not data["manufacturer"]:
        raise ValueError("INVALID_VENDOR_ID")
    volume=data.get("volume_mm")
    if (not isinstance(volume,list) or len(volume)!=3 or
        any(not isinstance(v,(int,float)) or not math.isfinite(v) or v<=0
            for v in volume)):
        raise ValueError("INVALID_VOLUME")
    for item in data.get("safe_macros", []):
        if not all(isinstance(item.get(k), str) and item[k] for k in
                   ("name", "source_url", "source_sha", "evidence", "scope")):
            raise ValueError("MACRO_WITHOUT_TRACEABLE_EVIDENCE")
        # No blanket safety bypass: a macro's proof applies only to its own
        # documented scope, and never erases collision reports before it.
        if item.get("scope") == "all_travels":
            raise ValueError("BLANKET_MACRO_TRUST_FORBIDDEN")
    return data


def sample(a, b, pitch):
    dist = math.hypot(b[0]-a[0], b[1]-a[1])
    steps = max(1, int(math.ceil(dist / max(pitch, 0.1))))
    for i in range(steps+1):
        t = i / steps
        yield t, (a[0]+(b[0]-a[0])*t, a[1]+(b[1]-a[1])*t,
                  a[2]+(b[2]-a[2])*t)


def arc_points(a, b, args, clockwise, pitch):
    # Klipper I/J offsets are relative to start; one arc can also lift in Z.
    if "I" not in args and "J" not in args:
        return None
    cx = a[0]+args.get("I", 0.0)
    cy = a[1]+args.get("J", 0.0)
    r = math.hypot(a[0]-cx, a[1]-cy)
    if not (r > 0.0001 and math.isfinite(r)):
        return None
    start = math.atan2(a[1]-cy, a[0]-cx)
    end = math.atan2(b[1]-cy, b[0]-cx)
    delta = (end-start) % (2*math.pi)
    if clockwise:
        delta = -((start-end) % (2*math.pi))
    if abs(delta) < 1.e-8:
        delta = -2*math.pi if clockwise else 2*math.pi
    steps = min(20000, max(4, int(math.ceil(abs(delta)*r/max(pitch, 0.1)))))
    for i in range(steps+1):
        t=i/steps
        ang=start+delta*t
        yield t,(cx+r*math.cos(ang),cy+r*math.sin(ang),a[2]+(b[2]-a[2])*t)


def validate_gcode(raw: bytes, vendor: dict, *, source: str = "official",
                   scan: str = "critical", clearance: float = 0.20,
                   pitch: float = 0.50) -> dict:
    if source not in ("official", "filuna", "synthetic"):
        raise ValueError("INVALID_SOURCE")
    if scan not in ("critical", "all"):
        raise ValueError("INVALID_SCAN")
    if not (0 <= clearance <= 3 and 0.1 <= pitch <= 2):
        raise ValueError("INVALID_GEOMETRY_PARAMS")
    lines = raw.decode("utf-8", errors="replace").splitlines()
    # This slicer closes objects on *every layer*, not only after final print.
    final_object_close = max(
        (i for i,line in enumerate(lines,1) if STOP.search(line.partition(";")[2])),
        default=-1)
    res = pitch
    # Occupancy is max deposited Z per XY voxel and the deposition line.
    occupied = {}
    # Wipe/prime towers are deliberate sacrificial regions. Record their
    # extrusion separately: do not mistake normal travel near a built tower
    # for a toolhead crossing the printed object. Never suppress a travel
    # that actually crosses object occupancy, including the final purge.
    prime_tower_deposition = 0
    region = "object"
    pos = [None,None,None]
    epos = 0.0
    xyz_relative = False
    e_relative = False
    layer = None
    recent_tool = -1000
    final_tail = False
    in_short_retraction_wipe = False
    final_marker = -1000
    spiral_marker = -1000
    prior_extrusion = False
    issues = []
    warnings = []
    uncertain = []
    checked = 0
    deposition = 0
    max_events = 32
    maxdim = vendor["volume_mm"]
    if len(raw) > 50_000_000:
        raise ValueError("GCODE_TOO_LARGE_FOR_QA")
    for lineno,line in enumerate(lines,1):
        comments = line.partition(";")[2]
        feature = comments.strip().lower()
        if feature.startswith("type:prime tower") or feature.startswith("type:wipe tower"):
            region = "prime_tower"
        elif (feature.startswith("type:") and
              not ("prime tower" in feature or "wipe tower" in feature)) or START.search(comments):
            region = "object"
        # Intentional post-extrusion wipe retracts along a printed edge;
        # it is not a nozzle transit to the wipe tower. Ignore only
        # short XY retraction wipes, never a long unlifted travel.
        if comments.strip().startswith("WIPE_START"):
            in_short_retraction_wipe = True
        if comments.strip().startswith("WIPE_END"):
            in_short_retraction_wipe = False
        if START.search(comments):
            final_tail = False
        if STOP.search(comments) and lineno == final_object_close:
            final_tail = True
        if FINAL.search(comments):
            final_marker = lineno
        if SPIRAL.search(comments):
            spiral_marker = lineno
        if comments.strip().upper().startswith("LAYER_CHANGE"):
            layer = 0 if layer is None else layer + 1
        layer_m = LAY.search(comments)
        if layer_m:
            layer=int(layer_m.group(1))
        code=line.partition(";")[0].strip()
        if not code:
            continue
        cmd=code.split(None,1)[0].upper()
        if cmd=="G90":
            xyz_relative=False;continue
        if cmd=="G91":
            xyz_relative=True;continue
        if cmd=="M82":
            e_relative=False;continue
        if cmd=="M83":
            e_relative=True;continue
        if cmd.startswith("T") and cmd[1:].isdigit():
            if prior_extrusion: recent_tool=lineno
            continue
        args={k.upper():float(v) for k,v in WORD.findall(code)}
        if cmd in ("G28","G380"):
            # Homing and vendor-specific endstop/probe moves invalidate
            # the prior machine coordinate reference. Do not pretend that
            # a subsequent relative Z move starts at the synthetic origin.
            axes=[i for i,axis in enumerate("XYZ") if axis in args]
            for i in (axes or range(3)):
                pos[i]=None
            continue
        if cmd=="G92":
            for i,axis in enumerate("XYZ"):
                if axis in args:pos[i]=args[axis]
            if "E" in args:epos=args["E"]
            continue
        if cmd not in ("G0","G1","G2","G3"):
            # Firmware macro effects are not inferred from their name.
            continue
        nxt=pos[:]
        for i,axis in enumerate("XYZ"):
            if axis in args:
                nxt[i] = (
                    None if pos[i] is None else pos[i] + args[axis]
                ) if xyz_relative else args[axis]
        new_e=epos
        delta_e=0.0
        if "E" in args:
            new_e=epos+args["E"] if e_relative else args["E"]
            delta_e=new_e-epos
        epos=new_e
        if not all(x is not None and math.isfinite(x) for x in nxt):
            pos=nxt
            continue
        if not all(x is not None for x in pos):
            pos=nxt
            continue
        a=tuple(pos);b=tuple(nxt)
        pos=nxt
        if cmd in ("G2","G3"):
            pts=arc_points(a,b,args,cmd=="G2",res)
            if pts is None:
                if (lineno-recent_tool<=20 or final_tail or lineno-final_marker<=25):
                    uncertain.append({"code":"ARC_WITHOUT_CENTER_CANNOT_PROVE_SAFETY","line":lineno})
                continue
        else:
            pts=sample(a,b,res)
        pts=list(pts)
        # Bounds: every commanded XYZ point including intermediate arc points.
        outside=next(((t,q) for t,q in pts
                      if any(q[i] < -0.001 or q[i]>maxdim[i]+0.001 for i in range(3))),None)
        spiral=(lineno-spiral_marker<=12 or
                cmd in ("G2","G3") and b[2]>a[2]+1e-5)
        if outside and vendor.get("check_bounds",True) and len(warnings)<max_events:
            t,q=outside
            warnings.append({"code":"AUTO_SPIRAL_OUT_OF_VOLUME" if spiral else "TRAVEL_OUT_OF_PRINT_VOLUME",
                             "line":lineno,"layer":layer,"xyz_mm":[round(v,3) for v in q],
                             "action":"Use Normal Z-hop" if spiral else "Verify firmware parking envelope"})
        xy=math.hypot(b[0]-a[0], b[1]-a[1])
        if xy<0.0001:
            continue
        if delta_e>0.00001:
            prior_extrusion=True
            if region == "prime_tower":
                prime_tower_deposition+=1
                continue
            deposition+=1
            for _,q in pts:
                # Centerline + approximate nozzle-radius footprint in XY.
                x,y,z=q
                cells={(math.floor(x/res), math.floor(y/res)),
                       (math.floor((x+res/2)/res), math.floor(y/res)),
                       (math.floor((x-res/2)/res), math.floor(y/res)),
                       (math.floor(x/res),math.floor((y+res/2)/res)),
                       (math.floor(x/res),math.floor((y-res/2)/res))}
                for cell in cells:
                    prev=occupied.get(cell)
                    if prev is None or z >= prev[0]:
                        occupied[cell]=(z,lineno)
            continue
        focus=(scan=="all" or final_tail or lineno-final_marker<=25 or
               lineno-recent_tool<=20)
        if not focus or not prior_extrusion:
            continue
        if in_short_retraction_wipe and delta_e < 0 and xy <= 5.0:
            continue
        checked+=1
        hit=None
        # Exclude start/end points and freshly extruded adjacent segment.
        for t,q in pts:
            if t < 0.04 or t > 0.96:continue
            cell=(math.floor(q[0]/res),math.floor(q[1]/res))
            prev=occupied.get(cell)
            if prev and prev[1]<lineno-3 and q[2]<=prev[0]+clearance:
                hit=(q,prev)
                break
        if hit and len(issues)<max_events:
            q,(zprinted,zline)=hit
            issues.append({"code":"TRAVEL_CROSSES_PRINTED_GEOMETRY","line":lineno,
                           "layer":layer,"xyz_mm":[round(v,3) for v in q],
                           "printed_top_z_mm":round(zprinted,3),
                           "printed_at_line":zline,
                           "context":"FINAL_PURGE_OR_END" if final_tail or lineno-final_marker<=25
                                     else "TOOLCHANGE" if lineno-recent_tool<=20 else "TRAVEL",
                           "clearance_mm":clearance})
    status="BLOCK" if issues else "WARN" if warnings else "REVIEW_REQUIRED" if (uncertain or not deposition) else "PASS"
    if issues and source=="filuna":
        # Not manufacturer-attributed without a paired official reference.
        status="REVIEW_REQUIRED"
    return {
        "schema":1,"source":source,"status":status,"send_allowed":status in ("PASS","WARN"),
        "gcode_sha256":digest(raw),"gcode_bytes":len(raw),"gcode_lines":len(lines),
        "gcode_modified":False,"manufacturer":vendor["manufacturer"],
        "printer_model":vendor["printer_model"],"volume_mm":maxdim,
        "bounds_validated":bool(vendor.get("check_bounds",True)),
        "geometry_method":"Object-only XY cell occupancy excluding documented prime/wipe tower extrusion; conservative critical-travel sampling",
        "geometry_resolution_mm":res,"clearance_mm":clearance,
        "scope":scan,"tested_travels":checked,"deposition_segments":deposition,
        "prime_tower_deposition_segments":prime_tower_deposition,
        "issues":issues,"warnings":warnings,"unknowns":uncertain,
        "safe_macro_proof_scope":"Only macro-internal park; no blanket exemption for prior final purge",
        "firmware_macro_safety_for_final_purge":"NOT_PROVEN",
        "limitations":[
                       "Print volume alone is not the physical service/parking travel envelope; cross-brand bounds need certified OEM data",
                       "No firmware macro expansion or dynamic toolhead carriage envelope",
                       "No physical nozzle 3D collision mesh; occupancy is XY/Z approximation",
                       "Critical mode excludes routine same-layer travels to reduce false positives",
                       "A PASS is a bounded QA finding, not physical printer homologation"]
    }


def paired_audit(candidate: bytes, official: bytes, vendor: dict, *, clearance=0.2) -> dict:
    res=validate_gcode(candidate,vendor,source="filuna",clearance=clearance)
    ref=validate_gcode(official,vendor,source="official",clearance=clearance)
    exact=candidate==official
    res["official_gcode_sha256"]=ref["gcode_sha256"]
    res["official_status"]=ref["status"]
    res["gcode_exact_match"]=exact
    res["official_issues"]=ref["issues"]
    same=[]
    for issue in res["issues"]:
        if any(math.dist(issue["xyz_mm"],q["xyz_mm"])<=1.0 for q in ref["issues"]):
            same.append(issue)
    if exact:
        res["origin"]="OFFICIAL_IDENTICAL"
        res["status"]=ref["status"]
    elif res["issues"] and len(same)==len(res["issues"]) and ref["issues"]:
        res["origin"]="OEM_OFFICIAL_CONFIRMED_RISK"
        res["status"]="BLOCK"
    else:
        res["origin"]="FILUNA_ENGINE_OR_PROFILE_DIVERGENCE"
        res["status"]="ENGINE_DIVERGENCE"
    res["matched_official_risks"]=same
    res["send_allowed"]=res["status"] in ("PASS","WARN")
    return res


def acknowledge_risk(report: dict, operator: str, phrase: str, audit_log: Path) -> dict:
    if report["status"]!="BLOCK" or report.get("origin") not in (
            "OFFICIAL_IDENTICAL","OEM_OFFICIAL_CONFIRMED_RISK") or not report.get("issues"):
        raise ValueError("OVERRIDE_REQUIRES_PROVEN_OFFICIAL_RISK")
    if phrase!=ACK or not operator.strip() or len(operator)>128:
        raise ValueError("EXPLICIT_INFORMED_CONSENT_REQUIRED")
    entry={"timestamp_utc":datetime.now(timezone.utc).isoformat(),
           "operator":operator,"printer_model":report["printer_model"],
           "gcode_sha256":report["gcode_sha256"],
           "official_gcode_sha256":report.get("official_gcode_sha256"),
           "risks":report["issues"],"consent_phrase":phrase,
           "scope":"one specific unmodified G-code SHA; not permanent manufacturer homologation"}
    audit_log.parent.mkdir(parents=True,exist_ok=True)
    # Append-only in the application; production integration must add
    # authenticated user identity, atomic persistence and audit retention.
    with audit_log.open("a",encoding="utf-8") as handle:
        handle.write(json.dumps(entry,ensure_ascii=False,sort_keys=True)+"\n")
        handle.flush()
        os.fsync(handle.fileno())
    report=dict(report)
    report["status"]="USER_ACCEPTED_RISK"
    report["send_allowed"]=True
    report["explicit_risk_ack"]={"gcode_sha256":entry["gcode_sha256"],
                                  "timestamp_utc":entry["timestamp_utc"],"operator":operator}
    return report


def main() -> int:
    a=argparse.ArgumentParser(description="Read-only Filuna OEM spatial G-code QA gate")
    a.add_argument("--gcode",type=Path,required=True)
    a.add_argument("--vendor",type=Path,required=True)
    a.add_argument("--official",type=Path)
    a.add_argument("--origin",choices=("official","filuna","synthetic"),default="filuna")
    a.add_argument("--scope",choices=("critical","all"),default="critical")
    a.add_argument("--clearance",type=float,default=0.20)
    a.add_argument("--report",type=Path)
    a.add_argument("--operator")
    a.add_argument("--ack-phrase")
    a.add_argument("--audit-log",type=Path)
    x=a.parse_args()
    vendor=policy_from_yaml(x.vendor)
    before=x.gcode.read_bytes()
    if x.official:
        result=paired_audit(before,x.official.read_bytes(),vendor,clearance=x.clearance)
    else:
        result=validate_gcode(before,vendor,source=x.origin,scan=x.scope,clearance=x.clearance)
        if x.origin=="official":
            result["origin"]="OEM_ORIGINAL"
    if x.ack_phrase or x.operator:
        if not x.audit_log:
            a.error("--audit-log is required for informed risk acceptance")
        result=acknowledge_risk(result,x.operator or "",x.ack_phrase or "",x.audit_log)
    if digest(x.gcode.read_bytes())!=result["gcode_sha256"]:
        raise RuntimeError("GCODE_CHANGED_DURING_VALIDATION")
    output=json.dumps(result,ensure_ascii=False,sort_keys=True,indent=2)+"\n"
    if x.report:
        x.report.parent.mkdir(parents=True,exist_ok=True)
        x.report.write_text(output,encoding="utf-8")
    print(output)
    return 0 if result["send_allowed"] else 2


if __name__=="__main__":
    raise SystemExit(main())
