#!/usr/bin/env python3
"""Compare Filuna G-code against an untouched official OEM output.

This gate is intentionally strict: structural mismatch blocks equivalence.
It does NOT certify that the output is safe to print or that the input presets
are identical; the fixture manifest must establish input parity separately.
"""
from __future__ import annotations
import argparse
import collections
import hashlib
import json
import pathlib
import re
import sys

MOVE = {"G0", "G1", "G2", "G3"}
CRITICAL = {"G0", "G1", "G2", "G3", "G28", "G90", "G91", "G92", "M82", "M83",
            "M104", "M109", "M140", "M190", "M106", "M107", "M204", "M205",
            "M220", "M221", "M400", "M600", "M900", "M960", "M970", "M971",
            "M975", "M976", "M977", "M980", "M981", "M983", "M984",
            "SET_VELOCITY_LIMIT", "SET_PRESSURE_ADVANCE", "BED_MESH_CALIBRATE"}

def scan(path: pathlib.Path) -> dict:
    data = path.read_bytes()
    counts = collections.Counter()
    normalized = hashlib.sha256()
    critical = hashlib.sha256()
    line_count = 0
    layer_markers = 0
    opening = []
    for raw in data.decode("utf-8", errors="replace").splitlines():
        line_count += 1
        stripped = raw.strip()
        if stripped.startswith(";"):
            if re.search(r"(?:change_layer|layer_change|^;\s*layer\s*:)", stripped, re.I):
                layer_markers += 1
            continue
        code = stripped.split(";", 1)[0].strip()
        if not code:
            continue
        code = " ".join(code.split())
        cmd = code.split(" ", 1)[0].upper()
        counts[cmd] += 1
        normalized.update(code.encode("utf-8") + b"\n")
        if cmd in CRITICAL or cmd in MOVE:
            critical.update(code.encode("utf-8") + b"\n")
        if len(opening) < 12:
            opening.append(code[:150])
    return {
        "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(),
        "lines": line_count, "layer_markers": layer_markers,
        "normalized_commands_sha256": normalized.hexdigest(),
        "critical_sequence_sha256": critical.hexdigest(),
        "commands": dict(sorted(counts.items())),
        "first_commands": opening
    }

def compare(official: pathlib.Path, filuna: pathlib.Path) -> dict:
    o = scan(official)
    f = scan(filuna)
    differences = []
    for cmd in sorted(CRITICAL | MOVE):
        if o["commands"].get(cmd, 0) != f["commands"].get(cmd, 0):
            differences.append({"command": cmd, "official": o["commands"].get(cmd, 0),
                                "filuna": f["commands"].get(cmd, 0)})
    signature_equal = o["critical_sequence_sha256"] == f["critical_sequence_sha256"]
    normalized_equal = o["normalized_commands_sha256"] == f["normalized_commands_sha256"]
    return {"test": "oem_gcode_equivalence_preliminary",
            "official_file_untouched": True,
            "requires_identical_fixture_and_effective_presets": True,
            "official": o, "filuna": f,
            "different_critical_command_counts": differences,
            "critical_sequence_identical": signature_equal,
            "normalized_commands_identical": normalized_equal,
            "status": "PRELIMINARY_MATCH_NOT_HARDWARE_PASS" if normalized_equal else "MISMATCH",
            "hardware_homologated": False, "module_autoload_tested": False}

def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--official", type=pathlib.Path, required=True)
    p.add_argument("--filuna", type=pathlib.Path, required=True)
    p.add_argument("--out", type=pathlib.Path)
    args = p.parse_args()
    result = compare(args.official, args.filuna)
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k not in ("official", "filuna")},
                     ensure_ascii=False))
    return 0 if result["status"] == "PRELIMINARY_MATCH_NOT_HARDWARE_PASS" else 1

if __name__ == "__main__":
    sys.exit(main())
