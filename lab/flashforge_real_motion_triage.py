#!/usr/bin/env python3
"""Evidence-first Flashforge OEM-vs-Filuna G-code motion triage.

Reports exact XY/Z path edits separately from changed extrusion and feedrate.
Never normalizes, rewrites, or certifies G-code; a single sample remains a
diagnostic even when byte identical. Reads two existing G-code files only.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from difflib import SequenceMatcher
import hashlib
import json
from pathlib import Path
import re

MOTION = re.compile(r"^(G[0123])(?:\s|$)", re.I)
VALUE = re.compile(r"(?:^|\s)([XYZEFIJPR])([+-]?(?:\d+\.?\d*|\.\d+)(?:[Ee][+-]?\d+)?)")
OTHER = ("M106", "M73", "SET_VELOCITY_LIMIT", "M104", "M109", "M140", "M190")


def digest(blob: bytes) -> str:
    return hashlib.sha256(blob).hexdigest()


def parse(blob: bytes) -> tuple[dict[int, list[dict]], Counter]:
    layers = defaultdict(list)
    family_counts: Counter = Counter()
    layer = -1
    for line_number, raw in enumerate(blob.decode("utf-8", "replace").splitlines(), 1):
        line = raw.strip()
        if line.startswith(";LAYER_CHANGE"):
            layer += 1
        cmd = line.split(";", 1)[0].strip()
        if not cmd:
            continue
        token = cmd.split(None, 1)[0].upper()
        if token in OTHER:
            family_counts[token] += 1
        if not MOTION.match(cmd):
            continue
        attrs = {a.upper(): b for a, b in VALUE.findall(cmd)}
        # XY/Z comparison uses literal values, no tolerance or rounding.
        spatial = (token, attrs.get("X"), attrs.get("Y"), attrs.get("Z"),
                   attrs.get("I"), attrs.get("J"), attrs.get("P"), attrs.get("R"))
        layers[layer].append({
            "line": line_number, "gcode": cmd, "geometry": spatial,
            "e": attrs.get("E"), "f": attrs.get("F")
        })
        family_counts[token] += 1
    return layers, family_counts


def triage(official: bytes, filuna: bytes, max_examples: int = 12) -> dict:
    report = {
        "kind": "FLASHFORGE_FILUNA_MOTION_FORENSICS",
        "homologation": "NOT_PROVEN",
        "no_source_or_gcode_rewriting": True,
        "official": {"sha256": digest(official), "bytes": len(official)},
        "filuna": {"sha256": digest(filuna), "bytes": len(filuna)}
    }
    if not official or not filuna:
        return {**report, "status": "BLOCKED_EMPTY_OR_MISSING_GCODE"}
    a, ca = parse(official)
    b, cb = parse(filuna)
    counts = Counter()
    first = []
    per_layer = []
    for layer in sorted(set(a) | set(b)):
        left = a.get(layer, [])
        right = b.get(layer, [])
        matcher = SequenceMatcher(None,
                                  [r["geometry"] for r in left],
                                  [r["geometry"] for r in right],
                                  autojunk=False)
        this = Counter()
        for operation, lo, hi, fo, fi in matcher.get_opcodes():
            if operation == "equal":
                for x, y in zip(left[lo:hi], right[fo:fi]):
                    if x["e"] != y["e"]:
                        this["extrusion_different"] += 1
                        if len(first) < max_examples:
                            first.append({"layer": layer, "kind": "extrusion",
                                          "official": x, "filuna": y})
                    if x["f"] != y["f"]:
                        this["feedrate_different"] += 1
                        if len(first) < max_examples:
                            first.append({"layer": layer, "kind": "feedrate",
                                          "official": x, "filuna": y})
            else:
                # insert/delete/replace means actual spatial motion mismatch.
                this["geometry_edit_groups"] += 1
                this["official_unaligned_moves"] += hi - lo
                this["filuna_unaligned_moves"] += fi - fo
                if len(first) < max_examples:
                    first.append({
                        "layer": layer, "kind": "geometry",
                        "opcode": operation,
                        "official": left[lo:min(hi, lo + 3)],
                        "filuna": right[fo:min(fi, fo + 3)]
                    })
        counts.update(this)
        if this or len(left) != len(right):
            per_layer.append({
                "layer": layer,
                "official_motion_count": len(left),
                "filuna_motion_count": len(right),
                "differences": dict(this)
            })
    motion_a = sum(map(len, a.values()))
    motion_b = sum(map(len, b.values()))
    report.update({
        "status": ("DIAGNOSTIC_ONE_SAMPLE_RAW_IDENTICAL_NOT_HOMOLOGATED"
                   if official == filuna else "FAIL_FUNCTIONAL_OR_METADATA_MISMATCH"),
        "motion_counts": {"official": motion_a, "filuna": motion_b},
        "command_counts": {key: {"official": ca[key], "filuna": cb[key]}
                           for key in sorted(set(ca) | set(cb))
                           if ca[key] != cb[key]},
        "total_differences": dict(counts),
        "mismatching_layers": len(per_layer),
        "per_layer_differences": per_layer,
        "examples": first,
        "conclusion": "This diagnostic cannot confer Filuna native OEM parity."
    })
    return report


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--official", required=True, type=Path)
    ap.add_argument("--filuna", required=True, type=Path)
    ap.add_argument("--out", required=True, type=Path)
    args = ap.parse_args()
    if not args.official.is_file() or not args.filuna.is_file():
        report = {"status": "BLOCKED_REQUIRED_REAL_GCODE_NOT_FOUND",
                  "homologation": "NOT_PROVEN"}
    else:
        report = triage(args.official.read_bytes(), args.filuna.read_bytes())
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                        encoding="utf-8")
    print(json.dumps({k: report.get(k) for k in
                      ("status", "homologation", "motion_counts",
                       "total_differences", "mismatching_layers")},
                     ensure_ascii=False))
    return 0  # Lab diagnostics succeed independently of parity, which stays FAIL.


if __name__ == "__main__":
    raise SystemExit(main())
