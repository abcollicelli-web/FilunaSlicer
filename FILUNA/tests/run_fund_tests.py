#!/usr/bin/env python3
"""Run Onda01 foundation unit suite and CMP/DATA summaries."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main() -> int:
    cmd = [sys.executable, "-m", "unittest", "FILUNA.tests.test_fundation_onda01", "-v"]
    # unittest module path: run file directly for simplicity
    cmd = [sys.executable, str(ROOT / "FILUNA/tests/test_fundation_onda01.py"), "-v"]
    rc = 0
    for f in ("test_fundation_onda01.py", "test_gate_governance.py", "test_core04_matrix.py"):
        c = [sys.executable, str(ROOT / "FILUNA/tests" / f), "-v"]
        print("+", " ".join(c), flush=True)
        rc |= subprocess.run(c, cwd=str(ROOT)).returncode
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
