#!/usr/bin/env python3
"""Read-only four-way ElegooSlicer/Filuna G-code cross-audit, not hardware simulation.

Compares real executable instructions, layer counts, motion/extrusion totals, printer
commands, and the impact of swapping official/Filuna presets. A different G-code
is BLOCKED_EQUIVALENCE, not a defect proof, and matching instructions are not a
physical safety certificate.
"""
from __future__ import annotations
import argparse, collections, hashlib, json, re
from pathlib import Path
COMMAND = re.compile(r"^\s*((?:[GMT]\d+(?:\.\d+)?)|(?:[A-Z_][A-Z0-9_]+))\b")
FIELDS = re.compile(r"(?<![A-Za-z])([XYZEFSP])([-+]?(?:\d+(?:\.\d*)?|\.\d+))")
def audit(path:Path)->dict:
    raw=path.read_bytes()
    lines=raw.decode("utf-8",errors="replace").splitlines()
    counts=collections.Counter(); layers=[]; type_counts=collections.Counter()
    positive_extrusion=0.0; max_feed=0.0; exec_lines=[]
    temps=collections.defaultdict(set);fan=collections.defaultdict(set)
    startup=[]; end=[]; first_layer=False; macros=set()
    for line in lines:
        if line.startswith(";LAYER:"):
            try:layers.append(int(line.split(":",1)[1].strip()))
            except ValueError:pass
            first_layer=True
        if line.startswith(";TYPE:"):type_counts[line.split(":",1)[1].strip()]+=1
        instruction=line.split(";",1)[0].strip()
        if not instruction or instruction.startswith("("):continue
        mat=COMMAND.match(instruction)
        if not mat:continue
        cmd=mat.group(1)
        counts[cmd]+=1
        exec_lines.append(instruction)
        if not first_layer:startup.append(instruction)
        words={k:float(v) for k,v in FIELDS.findall(instruction[len(cmd):])}
        if cmd in ("G0","G1"):
            max_feed=max(max_feed,words.get("F",0))
            positive_extrusion+=max(0.0,words.get("E",0))
        if cmd in ("M104","M109","M140","M190") and "S" in words:
            temps[cmd].add(words["S"])
        if cmd in ("M106","M107"):
            fan[cmd].add(tuple((k,v) for k,v in sorted(words.items()) if k in ("S","P")))
        if cmd not in ("G0","G1") and not cmd[0:1] in ("G","M","T"):
            macros.add(cmd)
    statlines=[s.strip() for s in lines if s.startswith("; estimated printing time") or s.startswith("; filament used [g]") or s.startswith("; total filament used [g]")]
    return {
        "sha256":hashlib.sha256(raw).hexdigest(),"bytes":len(raw),"line_count":len(lines),
        "layer_markers":len(layers),"unique_layer_markers":len(set(layers)),
        "command_total":len(exec_lines),"commands":dict(sorted(counts.items())),
        "positive_extrusion_words_mm":round(positive_extrusion,4),
        "max_explicit_feed_mm_min":max_feed,
        "macros":sorted(macros),"temps_C":{k:sorted(v) for k,v in temps.items()},
        "fan_commands":{k:sorted([list(t) for t in v]) for k,v in fan.items()},
        "feature_type_counts":dict(type_counts),
        "startup_first_35":startup[:35],"statistics_comments":statlines,
        "executable_sha256":hashlib.sha256("\n".join(exec_lines).encode()).hexdigest(),
    }
def main():
    p=argparse.ArgumentParser()
    for arg in ("official","filuna","filuna_official_presets","official_filuna_presets"):
        p.add_argument("--"+arg.replace("_","-"),type=Path,required=True)
    p.add_argument("--output",type=Path,required=True)
    a=p.parse_args()
    records={key:audit(getattr(a,key)) for key in
             ("official","filuna","filuna_official_presets","official_filuna_presets")}
    same_preset_core=records["official"]["command_total"]==records["official_filuna_presets"]["command_total"] and (
        records["official"]["commands"]==records["official_filuna_presets"]["commands"])
    same_preset_filuna=records["filuna"]["command_total"]==records["filuna_official_presets"]["command_total"] and (
        records["filuna"]["commands"]==records["filuna_official_presets"]["commands"])
    o,f=records["official"],records["filuna"]
    deltas={key:{"official":o[key],"filuna":f[key]} for key in
            ("bytes","line_count","command_total","positive_extrusion_words_mm",
             "max_explicit_feed_mm_min","layer_markers")
            if o[key]!=f[key]}
    delta_counts={k:{"official":o["commands"].get(k,0),"filuna":f["commands"].get(k,0)}
                  for k in sorted(set(o["commands"])|set(f["commands"]))
                  if o["commands"].get(k,0)!=f["commands"].get(k,0)}
    result={
        "scope":"REAL Windows ElegooSlicer v1.5.3.5 vs Filuna portable under Wine; offline CLI-only",
        "model":"Orca tests/data/20mm_cube.obj sha256:15943d9ad0548b654199c4e8eecdfc937964eaac80aad0237bf70fac75725b80",
        "printer":"Elegoo Centauri Carbon 2 0.4 nozzle",
        "filament":"Elegoo PLA @ECC2","process":"0.20mm Standard @Elegoo CC2 0.4 nozzle",
        "verification":"Two distinct binaries and four independent slicing runs (crossed presets)",
        "machine_safety_certified":False,"firmware_emulated":False,"physical_printer_connected":False,
        "results":records,
        "crossed_presets_same_command_counts":{"official":same_preset_core,"filuna":same_preset_filuna},
        "main_differences":deltas,"command_count_differences":delta_counts,
        "status":"PARTIAL_COMPARISON_MISMATCH" if (
            o["executable_sha256"]!=f["executable_sha256"]) else "PARTIAL_COMMANDS_MATCH",
        "equivalence_certified":False,"release_approved":False,
        "next_action":"Investigate effective inherited defaults/engine differences, then rerun; hardware final only.",
    }
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+"\n")
    print(result["status"])
    print("official:",o["sha256"],o["bytes"],"G1",o["commands"].get("G1"))
    print("filuna:",f["sha256"],f["bytes"],"G1",f["commands"].get("G1"))
    print("crossed same command counts:",result["crossed_presets_same_command_counts"])
    print("command difference count:",len(delta_counts))
if __name__=="__main__":main()
