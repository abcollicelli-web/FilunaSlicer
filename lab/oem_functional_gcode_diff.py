#!/usr/bin/env python3
"""Strict observational G-code differential for an existing Filuna portable vs OEM.

This is a diagnostic, not an equivalence test or certification. In particular,
ignoring comments for the secondary command-stream metric is NOT a valid
normalization for a homologation gate: some firmware consumes metadata comments.
No slicer is compiled or invoked by this module.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def commands_with_lines(data: bytes) -> tuple[list[tuple[int, str]], int]:
    """Preserve all executable text byte-for-byte except outer whitespace.

    The command-only *diagnostic* separates semicolon comments. It does not
    claim comments cannot affect vendor firmware or print behavior.
    """
    result = []
    comment_lines = 0
    for lineno, line in enumerate(data.decode("utf-8", "replace").splitlines(), 1):
        command = line.split(";", 1)[0].strip()
        if command:
            result.append((lineno, command))
        elif line.strip().startswith(";"):
            comment_lines += 1
    return result, comment_lines


def command_family(command: str) -> str:
    token = command.split(None, 1)[0].upper()
    if token in ("G0", "G1", "G2", "G3"):
        return "MOVEMENT_" + token
    if token in ("G10", "G11"):
        return "RETRACTION_" + token
    if token in ("M104", "M109", "M140", "M190", "M141", "M191"):
        return "TEMPERATURE_" + token
    if token in ("M204", "M205", "M201", "M203", "M220", "M221"):
        return "SPEED_ACCELERATION_" + token
    if token.startswith("T") and token[1:].isdigit():
        return "TOOLCHANGE_" + token
    return "OTHER_" + token


def analyze_bytes(official: bytes, candidate: bytes, vendor: str) -> dict:
    if not official or not candidate:
        return {"vendor": vendor, "status": "DIAGNOSTIC_EMPTY_GCODE",
                "filuna_parity": "NOT_PROVEN"}
    a, a_comments = commands_with_lines(official)
    b, b_comments = commands_with_lines(candidate)
    movement_a = [(ln, cmd) for ln, cmd in a if command_family(cmd).startswith("MOVEMENT_")]
    movement_b = [(ln, cmd) for ln, cmd in b if command_family(cmd).startswith("MOVEMENT_")]
    move_a = "\n".join(cmd for _, cmd in movement_a).encode("utf-8")
    move_b = "\n".join(cmd for _, cmd in movement_b).encode("utf-8")
    first_motion = next((i for i in range(min(len(movement_a), len(movement_b)))
                         if movement_a[i][1] != movement_b[i][1]),
                        min(len(movement_a), len(movement_b)))
    motion_difference = ({"index": first_motion,
                          "official": movement_a[first_motion:first_motion + 3],
                          "filuna": movement_b[first_motion:first_motion + 3]}
                         if first_motion < max(len(movement_a), len(movement_b)) else None)
    c_a = Counter(command_family(x) for _, x in a)
    c_b = Counter(command_family(x) for _, x in b)
    a_command_text = "\n".join(x for _, x in a).encode("utf-8")
    b_command_text = "\n".join(x for _, x in b).encode("utf-8")
    first = next((i for i in range(min(len(a), len(b)))
                  if a[i][1] != b[i][1]), min(len(a), len(b)))
    mismatched = first < max(len(a), len(b))
    context = {
        "index": first,
        "official": ([{"line": num, "text": cmd} for num, cmd in
                       a[first:first + 5]] if mismatched else []),
        "filuna": ([{"line": num, "text": cmd} for num, cmd in
                     b[first:first + 5]] if mismatched else [])
    }
    equal_raw = official == candidate
    equal_commands = a_command_text == b_command_text
    if equal_raw:
        status = "DIAGNOSTIC_ONE_SAMPLE_IDENTICAL_NOT_HOMOLOGATED"
    elif equal_commands:
        status = "DIAGNOSTIC_COMMANDS_EQUAL_METADATA_NOT_CERTIFIED"
    else:
        status = "FAIL_GCODE_COMMAND_STREAM_DIFFERENT"
    return {
        "vendor": vendor,
        "status": status,
        "filuna_parity": "NOT_PROVEN",
        "scope": "PREEXISTING_FILUNA_BINARY_VS_OFFICIAL_OEM_ONE_SAMPLE",
        "note": "Comment-stripped hashes are diagnostic ONLY; never promote on that basis.",
        "raw_bytes": {"official": len(official), "filuna": len(candidate)},
        "raw_sha256": {"official": sha256(official), "filuna": sha256(candidate)},
        "raw_equal": equal_raw,
        "noncomment_command_sha256": {
            "official": sha256(a_command_text), "filuna": sha256(b_command_text)},
        "noncomment_command_equal_diagnostic_only": equal_commands,
        "command_counts": {"official": len(a), "filuna": len(b)},
        "motion_counts": {"official": len(movement_a), "filuna": len(movement_b)},
        "motion_sha256": {"official": sha256(move_a), "filuna": sha256(move_b)},
        "motion_equal_diagnostic_only": move_a == move_b,
        "first_motion_difference": motion_difference,
        "comment_lines": {"official": a_comments, "filuna": b_comments},
        "families": {family: {"official": c_a.get(family, 0),
                              "filuna": c_b.get(family, 0)}
                     for family in sorted(set(c_a) | set(c_b))
                     if c_a.get(family, 0) != c_b.get(family, 0)},
        "first_command_difference": context if mismatched else None,
        "requires": [
            "Real Filuna C++ OEM module and compatible presets",
            "Multi-sample model/firmware/preset identity parity and negative tests",
            "Native device protocol, safety and beta-only physical evidence"
        ]
    }


def evaluate(vendor: str, official_file: Path, candidate_dir: Path) -> dict:
    result = {
        "vendor": vendor,
        "status": "NOT_RUN",
        "filuna_parity": "NOT_PROVEN",
        "official_file": str(official_file),
        "candidate_directory": str(candidate_dir)
    }
    if not official_file.is_file():
        result["status"] = "BLOCKED_OFFICIAL_GCODE_MISSING"
        return result
    candidate = sorted(candidate_dir.rglob("*.gcode")) if candidate_dir.is_dir() else []
    result["candidate_gcode_count"] = len(candidate)
    if len(candidate) != 1:
        result["status"] = (
            "BLOCKED_FILUNA_GCODE_NOT_GENERATED" if not candidate
            else "BLOCKED_MULTIPLE_FILUNA_GCODE_UNSCOPED"
        )
        return result
    result.update(analyze_bytes(official_file.read_bytes(),
                                candidate[0].read_bytes(), vendor))
    result["candidate_file"] = str(candidate[0])
    return result


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--vendor", required=True)
    p.add_argument("--official", type=Path, required=True)
    p.add_argument("--candidate-dir", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    args = p.parse_args()
    report = evaluate(args.vendor, args.official, args.candidate_dir)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n",
                           encoding="utf-8")
    print(json.dumps({"vendor": report["vendor"], "status": report["status"],
                      "parity": report["filuna_parity"]}, ensure_ascii=False))
    # A negative diagnostic is expected and MUST NOT become a native-parity gate.
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
