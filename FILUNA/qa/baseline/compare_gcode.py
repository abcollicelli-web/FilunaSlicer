#!/usr/bin/env python3
"""Compare two G-code files after metadata-only normalization.

Exit 0 = match; 1 = functional mismatch; 2 = usage/IO error.
Never invents PASS when files missing.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from normalize_gcode import normalize_file, sha256_text


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("left", type=Path)
    p.add_argument("right", type=Path)
    args = p.parse_args(argv)
    for path in (args.left, args.right):
        if not path.is_file():
            print(f"FAIL: missing {path}", file=sys.stderr)
            return 2
    a = normalize_file(args.left)
    b = normalize_file(args.right)
    ha, hb = sha256_text(a), sha256_text(b)
    if ha == hb:
        print(f"PASS: normalized match sha256={ha}")
        return 0
    # Show first differing line for diagnostics (functional or unexpected meta)
    la, lb = a.splitlines(), b.splitlines()
    for i, (x, y) in enumerate(zip(la, lb), 1):
        if x != y:
            print(f"FAIL: first diff at line {i}")
            print(f"  left : {x}")
            print(f"  right: {y}")
            break
    else:
        print(f"FAIL: length differs {len(la)} vs {len(lb)}")
    print(f"left_sha={ha} right_sha={hb}")
    return 1


if __name__ == "__main__":
    # Allow running as script from this directory
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    raise SystemExit(main())
