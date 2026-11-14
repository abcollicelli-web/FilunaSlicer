#!/usr/bin/env python3
"""Audit *fully resolved* exported G-code configuration against OEM reference.

Pure diagnostic on existing files. Never edits G-code, never assumes equality
from filtered metadata, never certifies model/firmware/network or native parity.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re


KEY = re.compile(r"^;\s+([A-Za-z][A-Za-z0-9_]*)\s*=\s*(.*?)\s*$")


def read_config(text: str) -> dict[str, str]:
    blocks = []
    active = None
    for line in text.splitlines():
        if line.strip() == "; CONFIG_BLOCK_START":
            if active is not None:
                raise ValueError("NESTED_CONFIG_BLOCK")
            active = {}
        elif line.strip() == "; CONFIG_BLOCK_END":
            if active is None:
                raise ValueError("UNMATCHED_CONFIG_BLOCK_END")
            blocks.append(active)
            active = None
        elif active is not None:
            m = KEY.match(line)
            if not m:
                continue  # multiline comments in OEM configs are not parsed as keys
            key = m.group(1)
            # OEM exports may legally print one config key twice (e.g.
            # wipe_tower_x = 15.000 followed by wipe_tower_x = 15).
            # Keep every exact occurrence in order; never silently choose
            # the last value or treat conflicting values as equivalent.
            value = m.group(2)
            if key not in active:
                active[key] = value
            else:
                previous = active[key]
                if isinstance(previous, list):
                    previous.append(value)
                else:
                    active[key] = [previous, value]
    if active is not None or len(blocks) != 1 or not blocks[0]:
        raise ValueError("ONE_COMPLETE_NONEMPTY_CONFIG_BLOCK_REQUIRED")
    return blocks[0]


def compare_gcode_config(oem: str, filuna: str) -> dict:
    try:
        official = read_config(oem)
        candidate = read_config(filuna)
    except ValueError as e:
        return {"status": "BLOCKED_INVALID_OR_MISSING_CONFIG_BLOCK",
                "reason": str(e), "native_parity": "NOT_PROVEN"}
    shared = sorted(set(official) & set(candidate))
    conflicts = {
        k: {"oem": official[k], "filuna": candidate[k]}
        for k in shared if official[k] != candidate[k]
    }
    missing = sorted(set(official) - set(candidate))
    new = sorted(set(candidate) - set(official))
    return {
        "status": ("BLOCKED_EFFECTIVE_DEFAULTS_CONFLICT"
                   if conflicts or missing or new
                   else "DIAGNOSTIC_FULL_CONFIG_IDENTICAL_NOT_HOMOLOGATED"),
        "native_parity": "NOT_PROVEN",
        "oem_config_key_count": len(official),
        "filuna_config_key_count": len(candidate),
        "conflicting_shared_values": conflicts,
        "oem_keys_missing_from_filuna": missing,
        "filuna_only_keys": new,
        "notes": [
            "Different defaults may alter geometry, cooling, flow or multimaterial.",
            "The official OEM version and Filuna core can have different option types.",
            "Do not auto-coerce wall_direction=auto to ccw, remap speed or postprocess G-code.",
            "A same exported config block does not prove matching algorithms or G-code."
        ]
    }


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--oem", required=True, type=Path)
    p.add_argument("--filuna", required=True, type=Path)
    p.add_argument("--out", required=True, type=Path)
    args = p.parse_args()
    if not args.oem.is_file() or not args.filuna.is_file():
        result = {"status": "BLOCKED_NO_TWO_REAL_GCODES", "native_parity": "NOT_PROVEN"}
    else:
        result = compare_gcode_config(args.oem.read_text(encoding="utf-8", errors="replace"),
                                      args.filuna.read_text(encoding="utf-8", errors="replace"))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n",
                        encoding="utf-8")
    print(json.dumps({"status": result["status"], "native_parity": result["native_parity"],
                      "default_conflicts": len(result.get("conflicting_shared_values", {})),
                      "missing_from_filuna": len(result.get("oem_keys_missing_from_filuna", []))}))
    return 0  # diagnostic CI collection, not promotion


if __name__ == "__main__":
    raise SystemExit(main())
