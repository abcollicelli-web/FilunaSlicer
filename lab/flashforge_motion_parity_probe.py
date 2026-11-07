#!/usr/bin/env python3
"""Strict Flashforge motion diagnosis against the immutable official 1.7.9 G-code.

Never rewrites G-code, normalizes movement, calls a printer, or claims native
Filuna parity. This diagnoses the *source port* still needed on the shared Orca
engine. Comments and firmware-specific metadata may carry semantics and are
still covered by raw-file equivalence.
"""
from __future__ import annotations
import argparse
from collections import Counter
from difflib import SequenceMatcher
import hashlib
import json
from pathlib import Path
import re

MOVE = re.compile(r"^G([0123])(?:[ 	]|$)")
AXES = re.compile(r"(?<![A-Z])([XYZEF])(-?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+))")
LAYER = re.compile(r"^;LAYER_CHANGE(?:\s|$)")


CONFIG_ENTRY = re.compile(r"^;\s*([a-zA-Z_][a-zA-Z_0-9]*)\s*=\s*(.*?)\s*$")


def runtime_config(data: bytes) -> dict[str, str | list[str]]:
    """Read exact ordered config occurrences; do not hide repeated OEM keys.

    Invalid/incomplete blocks fail closed in the diagnostic report rather
    than silently returning a partial configuration as if it were complete.
    """
    from oem_exported_config_defaults_audit import read_config
    return read_config(data.decode("utf-8", "replace"))


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def movements(data: bytes) -> list[dict]:
    result = []
    layer = 0
    for lineno, line in enumerate(data.decode("utf-8", "replace").splitlines(), 1):
        if LAYER.match(line):
            layer += 1
        cmd = line.split(";", 1)[0].strip()
        m = MOVE.match(cmd)
        if not m:
            continue
        axes = dict(AXES.findall(cmd))
        # Preserve command opcode and coordinates. Ignore *nothing* in raw
        # parity; this key is solely for localizing divergent geometry.
        result.append({
            "line": lineno, "layer": layer, "code": cmd,
            "opcode": "G" + m.group(1),
            "xyz": tuple(axes.get(x) for x in "XYZ"),
            "e": axes.get("E"), "feedrate": axes.get("F"),
        })
    return result


def first_motion_delta(official: bytes, candidate: bytes, max_samples: int = 12) -> dict:
    if not official or not candidate:
        return {"status": "FAIL_EMPTY_GCODE", "native_parity": "NOT_PROVEN"}
    a, b = movements(official), movements(candidate)
    akeys = [(x["opcode"], *x["xyz"]) for x in a]
    bkeys = [(x["opcode"], *x["xyz"]) for x in b]
    matcher = SequenceMatcher(None, akeys, bkeys, autojunk=False)
    ops = matcher.get_opcodes()
    equal_moves = sum(i2-i1 for op,i1,i2,_,_ in ops if op == "equal")
    changed_blocks = [entry for entry in ops if entry[0] != "equal"]
    delta_counts = Counter()
    for op, i, j, k, l in ops:
        if op != "equal":
            continue
        for x, y in zip(a[i:j], b[k:l]):
            if x["e"] != y["e"]:
                delta_counts["extrusion"] += 1
            if x["feedrate"] != y["feedrate"]:
                delta_counts["feedrate"] += 1
    examples = []
    for op, i, j, k, l in changed_blocks[:max_samples]:
        examples.append({
            "kind": op,
            "first_official_layer": a[i]["layer"] if i < len(a) else None,
            "first_filuna_layer": b[k]["layer"] if k < len(b) else None,
            "official_move_index": i,
            "filuna_move_index": k,
            "official": a[i]["code"] if i < len(a) and i < j else None,
            "filuna": b[k]["code"] if k < len(b) and k < l else None,
        })
    try:
        official_settings = runtime_config(official)
        filuna_settings = runtime_config(candidate)
        config_evidence = "VALID"
    except ValueError as exc:
        official_settings = {}
        filuna_settings = {}
        config_evidence = "INVALID_OR_MISSING_CONFIG_BLOCK: " + str(exc)
    shared_deltas = {
        key: {"official": official_settings[key], "filuna": filuna_settings[key]}
        for key in sorted(official_settings.keys() & filuna_settings.keys())
        if official_settings[key] != filuna_settings[key]
    }
    raw_equal = official == candidate
    movement_equal = [x["code"] for x in a] == [x["code"] for x in b]
    geometry_equal = akeys == bkeys
    # A single matching print is also NOT proof of a connected native OEM port.
    return {
        "status": ("DIAGNOSTIC_SINGLE_SAMPLE_IDENTICAL_NOT_HOMOLOGATED"
                   if raw_equal else "FAIL_REAL_GCODE_PARITY"),
        "native_parity": "NOT_PROVEN",
        "raw_equal": raw_equal,
        "embedded_runtime_config_differences": shared_deltas,
        "embedded_runtime_config_evidence": config_evidence,
        "embedded_runtime_config_common_count": len(official_settings.keys() & filuna_settings.keys()),
        "movement_equal": movement_equal,
        "geometry_equal": geometry_equal,
        "official_sha256": digest(official),
        "filuna_sha256": digest(candidate),
        "official_move_count": len(a),
        "filuna_move_count": len(b),
        "geometry_equal_subsequence_moves": equal_moves,
        "geometry_changed_blocks": len(changed_blocks),
        "matching_geometry_different_e": delta_counts["extrusion"],
        "matching_geometry_different_f": delta_counts["feedrate"],
        "first_differences": examples,
        "note": "Never treat geometric subsequence, skipped metadata or rounded E/F as G-code parity.",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--official", type=Path, required=True)
    parser.add_argument("--filuna", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if not args.official.is_file() or not args.filuna.is_file():
        report = {"status": "BLOCKED_MISSING_REAL_GCODE", "native_parity": "NOT_PROVEN",
                  "official_exists": args.official.is_file(),
                  "filuna_exists": args.filuna.is_file()}
    else:
        report = first_motion_delta(args.official.read_bytes(), args.filuna.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    print(json.dumps({k: report.get(k) for k in
                      ("status", "native_parity", "official_move_count",
                       "filuna_move_count", "geometry_changed_blocks")},
                     ensure_ascii=False))
    # Diagnostic job success must not be mistaken for G-code parity.
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
