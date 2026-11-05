#!/usr/bin/env python3
"""Windows-only: slice same fixture with Orca-pin and Filuna portables; compare G-code.

Fail-closed. No GUI. No printer connection. No print start.
Uses --datadir under TEMP only.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(ROOT / "FILUNA/tests"))
from compare_gcode import compare  # noqa: E402
from evidence_schema import validate_evidence  # noqa: E402

ORCA_PIN = "a620d804efbe74162c96ad29d755cdab2b823530"
FILUNA_HEAD = "2dace0db86d305bec5b9627179f265012e4f5069"
ORCA_RUN = "42037889437"
FILUNA_RUN = "42102469888"

MACHINE = "resources/profiles/BBL/machine/Bambu Lab P1S 0.4 nozzle.json"
PROCESS = "resources/profiles/BBL/process/0.20mm Standard @BBL X1C.json"
FIXTURE_META = ROOT / "FILUNA/qa/fixtures/FUND-CUBE-20MM.json"


def sha256_file(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def find_exe(root: Path) -> Path:
    candidates = list(root.rglob("orca-slicer.exe"))
    if not candidates:
        raise FileNotFoundError(f"orca-slicer.exe not found under {root}")
    # Prefer shallowest
    candidates.sort(key=lambda p: len(p.parts))
    return candidates[0]


def run_slice(exe: Path, work: Path, model: Path, label: str) -> Path:
    datadir = work / f"datadir-{label}"
    outdir = work / f"out-{label}"
    datadir.mkdir(parents=True, exist_ok=True)
    outdir.mkdir(parents=True, exist_ok=True)
    machine = ROOT / MACHINE
    process = ROOT / PROCESS
    if not machine.is_file() or not process.is_file():
        raise FileNotFoundError("pinned presets missing from checkout")
    cmd = [
        str(exe),
        "--datadir", str(datadir),
        "--load-settings", f"{machine};{process}",
        "--load-defaultfila",
        "--arrange", "0",
        "--slice", "0",
        "--export-gcode",
        "--outputdir", str(outdir),
        str(model),
    ]
    log = work / f"slice-{label}.log"
    t0 = time.time()
    # CREATE_NO_WINDOW on Windows if available
    kwargs = {}
    if os.name == "nt":
        kwargs["creationflags"] = getattr(subprocess, "CREATE_NO_WINDOW", 0)
    proc = subprocess.run(
        cmd,
        cwd=str(exe.parent),
        capture_output=True,
        text=True,
        timeout=900,
        **kwargs,
    )
    log.write_text(
        f"cmd={cmd}\nexit={proc.returncode}\nelapsed_s={time.time()-t0:.1f}\n"
        f"--- stdout ---\n{proc.stdout}\n--- stderr ---\n{proc.stderr}\n",
        encoding="utf-8",
        errors="replace",
    )
    if proc.returncode != 0:
        raise RuntimeError(f"slice {label} failed exit={proc.returncode}; see {log}")
    gcodes = list(outdir.rglob("*.gcode"))
    if not gcodes:
        raise FileNotFoundError(f"no gcode produced for {label}; see {log}")
    gcodes.sort(key=lambda p: p.stat().st_mtime, reverse=True)
    return gcodes[0]


def main() -> int:
    if os.name != "nt" and not os.environ.get("FILUNA_BASELINE_ALLOW_NON_WINDOWS"):
        print("FAIL: real binary baseline requires Windows runner", file=sys.stderr)
        return 2

    ap = argparse.ArgumentParser()
    ap.add_argument("--orca-root", type=Path, required=True, help="Extracted Orca pin portable root")
    ap.add_argument("--filuna-root", type=Path, required=True, help="Extracted Filuna portable root")
    ap.add_argument("--work", type=Path, required=True)
    ap.add_argument("--evidence-out", type=Path, required=True)
    ap.add_argument("--commit-sha", required=True, help="Continuity branch tip being tested")
    args = ap.parse_args()

    args.work.mkdir(parents=True, exist_ok=True)
    meta = json.loads(FIXTURE_META.read_text(encoding="utf-8"))
    model = ROOT / meta["path"]
    if not model.is_file():
        print(f"FAIL: fixture missing {model}", file=sys.stderr)
        return 2
    fx = sha256_file(model)
    if fx != meta["sha256"]:
        print(f"FAIL: fixture hash mismatch {fx} != {meta['sha256']}", file=sys.stderr)
        return 2

    provenance = {
        "orca_pin_sha": ORCA_PIN,
        "orca_upstream_run": ORCA_RUN,
        "filuna_build_sha": FILUNA_HEAD,
        "filuna_run": FILUNA_RUN,
        "machine": MACHINE,
        "process": PROCESS,
        "fixture": meta["path"],
        "fixture_sha256": fx,
        "tolerance": "identity_after_metadata_allowlist_normalize",
    }
    (args.work / "provenance.json").write_text(json.dumps(provenance, indent=2) + "\n", encoding="utf-8")

    orca_exe = find_exe(args.orca_root)
    filuna_exe = find_exe(args.filuna_root)
    print(f"orca_exe={orca_exe}")
    print(f"filuna_exe={filuna_exe}")
    provenance["orca_exe_sha256"] = sha256_file(orca_exe)
    provenance["filuna_exe_sha256"] = sha256_file(filuna_exe)
    for dll in ("OrcaSlicer.dll",):
        for lbl, r in (("orca", orca_exe.parent), ("filuna", filuna_exe.parent)):
            d = r / dll
            if d.is_file():
                provenance[f"{lbl}_{dll}_sha256"] = sha256_file(d)
    if provenance["orca_exe_sha256"] == provenance["filuna_exe_sha256"] and \
            provenance.get("orca_OrcaSlicer.dll_sha256") == provenance.get("filuna_OrcaSlicer.dll_sha256"):
        print("FAIL: Orca and Filuna binaries identical - provenance invalid", file=sys.stderr)
        return 2
    (args.work / "provenance.json").write_text(json.dumps(provenance, indent=2) + "\n", encoding="utf-8")

    g_orca = run_slice(orca_exe, args.work, model, "orca-pin")
    g_filuna = run_slice(filuna_exe, args.work, model, "filuna")
    # Copy outputs for artifact
    shutil.copy2(g_orca, args.work / "orca-pin.gcode")
    shutil.copy2(g_filuna, args.work / "filuna.gcode")

    rc = compare(args.work / "orca-pin.gcode", args.work / "filuna.gcode")
    result = "PASS" if rc == 0 else "FAIL"

    # Negative: mutate Filuna gcode extrusion and ensure compare fails
    neg = args.work / "filuna-neg-functional.gcode"
    text = (args.work / "filuna.gcode").read_text(encoding="utf-8", errors="replace")
    mutated = False
    lines = []
    for line in text.splitlines(True):
        if (not mutated) and line.startswith("G1") and " E" in line:
            # bump first E value slightly
            import re
            def bump(m):
                return f"E{float(m.group(1)) + 0.5:.5f}"
            newline = re.sub(r"E(-?\d+(?:\.\d+)?)", bump, line, count=1)
            if newline != line:
                lines.append(newline)
                mutated = True
                continue
        lines.append(line)
    neg.write_text("".join(lines), encoding="utf-8")
    rc_neg = compare(args.work / "orca-pin.gcode", neg)
    neg_ok = rc_neg == 1
    if not mutated or not neg_ok:
        print("FAIL: negative functional probe did not detect difference", file=sys.stderr)
        result = "FAIL"
        rc = 1

    created = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    evidence = {
        "schema_version": 1,
        "case_id": "FUND-T07-gcode-bin-windows",
        "req_id": "REQ-FUND-004",
        "gate": "G3",
        "commit_sha": args.commit_sha,
        "result": result,
        "platform": "windows-actions-x64",
        "evidence_kind": "integration",
        "created_at_utc": created,
        "fixture_sha256": fx,
        "notes": (
            f"orca_pin={ORCA_PIN} run={ORCA_RUN}; filuna={FILUNA_HEAD} run={FILUNA_RUN}; "
            f"orca_dll={provenance.get('orca_OrcaSlicer.dll_sha256','?')[:16]} filuna_dll={provenance.get('filuna_OrcaSlicer.dll_sha256','?')[:16]}; compare_rc={rc}; neg_detected={neg_ok}"
        ),
        "evidence_url": "",  # filled by workflow with run URL
    }
    # Temporarily set URL placeholder for validation of other fields when PASS
    if result == "PASS":
        evidence["evidence_url"] = os.environ.get(
            "FILUNA_EVIDENCE_URL", "https://github.com/abcollicelli-web/FilunaSlicer/actions"
        )
    errs = validate_evidence(evidence)
    if errs and result == "PASS":
        print("FAIL: evidence invalid:", errs, file=sys.stderr)
        result = "FAIL"
        evidence["result"] = "FAIL"
        rc = 1
    args.evidence_out.parent.mkdir(parents=True, exist_ok=True)
    args.evidence_out.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"result": result, "compare_rc": rc, "neg_ok": neg_ok}, indent=2))
    return 0 if result == "PASS" else 1


if __name__ == "__main__":
    # Fix import order: evidence from tests
    raise SystemExit(main())
