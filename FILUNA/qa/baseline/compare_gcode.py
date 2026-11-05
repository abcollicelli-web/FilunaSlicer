#!/usr/bin/env python3
"""Compare two G-code files after strict metadata normalization. Fail-closed."""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from normalize_gcode import normalize_file, sha256_text


def compare(left: Path, right: Path) -> int:
    try:
        a = normalize_file(left)
        b = normalize_file(right)
    except (OSError, ValueError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2
    if not a.strip() or not b.strip():
        print("FAIL: normalized output empty", file=sys.stderr)
        return 2
    sa, sb = sha256_text(a), sha256_text(b)
    if sa == sb:
        print(f"PASS: normalized match sha256={sa}")
        return 0
    # Show first differing line for diagnosis (no silent pass)
    al, bl = a.splitlines(), b.splitlines()
    n = max(len(al), len(bl))
    for i in range(n):
        la = al[i] if i < len(al) else "<EOF>"
        lb = bl[i] if i < len(bl) else "<EOF>"
        if la != lb:
            print(f"FAIL: first diff at line {i+1}")
            print(f"  left : {la}")
            print(f"  right: {lb}")
            break
    print(f"left_sha={sa} right_sha={sb}")
    return 1


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("left", type=Path)
    p.add_argument("right", type=Path)
    args = p.parse_args(argv)
    if not args.left.is_file() or not args.right.is_file():
        print("FAIL: missing input file(s)", file=sys.stderr)
        return 2
    return compare(args.left, args.right)


if __name__ == "__main__":
    raise SystemExit(main())
