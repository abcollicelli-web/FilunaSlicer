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


def run_slice(exe: Path, work: Path, model: Path, label: str, preset: dict) -> tuple[int, Path | None, Path]:
    datadir = work / f"datadir-{label}"
    outdir = work / f"out-{label}"
    datadir.mkdir(parents=True, exist_ok=True)
    outdir.mkdir(parents=True, exist_ok=True)
    machine = ROOT / preset["machine"]
    process = ROOT / preset["process"]
    if not machine.is_file() or not process.is_file():
        raise FileNotFoundError("pinned presets missing from checkout")
    cmd = [str(exe), "--datadir", str(datadir), "--load-settings", f"{machine};{process}"]
    if preset.get("filament"):
        fil = ROOT / preset["filament"]
        if not fil.is_file():
            raise FileNotFoundError(f"filament preset missing: {fil}")
        cmd += ["--load-filaments", str(fil)]
    else:
        cmd += ["--load-defaultfila"]
    cmd += ["--slice", "0", "--outputdir", str(outdir), str(model)]
    log = work / f"slice-{label}.log"
    t0 = time.time()
    kwargs = {}
    if os.name == "nt":
        kwargs["creationflags"] = getattr(subprocess, "CREATE_NO_WINDOW", 0)
    proc = subprocess.run(cmd, cwd=str(exe.parent), capture_output=True, text=True, timeout=900, **kwargs)
    rc = proc.returncode if proc.returncode < 2**31 else proc.returncode - 2**32
    log.write_text(
        f"cmd={cmd}\nexit={rc}\nelapsed_s={time.time()-t0:.1f}\n"
        f"--- stdout ---\n{proc.stdout}\n--- stderr ---\n{proc.stderr}\n",
        encoding="utf-8", errors="replace")
    gcodes = sorted(outdir.rglob("*.gcode"))
    return rc, (gcodes[0] if (rc == 0 and len(gcodes) == 1) else None), log


def inputs_tree(paths: list[str]) -> dict:
    out = {}
    for p in paths:
        try:
            out[p] = subprocess.check_output(["git", "rev-parse", f"HEAD:{p}"], cwd=ROOT, text=True).strip()
        except Exception:
            out[p] = "UNAVAILABLE"
    return out


def negative_probe(ref: Path, cand: Path, work: Path, tag: str) -> bool:
    import re
    text = cand.read_text(encoding="utf-8", errors="replace")
    lines, mutated = [], False
    for line in text.splitlines(True):
        if not mutated and line.startswith("G1") and " E" in line:
            nl = re.sub(r"E(-?\d+(?:\.\d+)?)", lambda m: f"E{float(m.group(1)) + 0.5:.5f}", line, count=1)
            if nl != line:
                lines.append(nl); mutated = True; continue
        lines.append(line)
    neg = work / f"{tag}-neg-functional.gcode"
    neg.write_text("".join(lines), encoding="utf-8")
    return mutated and compare(ref, neg) == 1


def main() -> int:
    if os.name != "nt" and not os.environ.get("FILUNA_BASELINE_ALLOW_NON_WINDOWS"):
        print("FAIL: real binary baseline requires Windows runner", file=sys.stderr)
        return 2
    ap = argparse.ArgumentParser()
    ap.add_argument("--orca-root", type=Path, required=True)
    ap.add_argument("--filuna-root", type=Path, required=True)
    ap.add_argument("--work", type=Path, required=True)
    ap.add_argument("--evidence-out", type=Path, required=True)
    ap.add_argument("--commit-sha", required=True, help="subject commit (code+inputs tested)")
    args = ap.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    matrix = json.loads((ROOT / "FILUNA/qa/baseline/cases.json").read_text(encoding="utf-8"))

    orca_exe, filuna_exe = find_exe(args.orca_root), find_exe(args.filuna_root)
    prov = {"orca_pin_sha": ORCA_PIN, "orca_upstream_run": ORCA_RUN, "filuna_build_sha": FILUNA_HEAD,
            "filuna_run": FILUNA_RUN, "orca_exe_sha256": sha256_file(orca_exe),
            "filuna_exe_sha256": sha256_file(filuna_exe), "matrix_version": matrix["matrix_version"],
            "tolerance": "identity_after_metadata_allowlist_normalize"}
    for lbl, r in (("orca", orca_exe.parent), ("filuna", filuna_exe.parent)):
        d = r / "OrcaSlicer.dll"
        if d.is_file():
            prov[f"{lbl}_OrcaSlicer.dll_sha256"] = sha256_file(d)
    if prov["orca_exe_sha256"] == prov["filuna_exe_sha256"] and \
            prov.get("orca_OrcaSlicer.dll_sha256") == prov.get("filuna_OrcaSlicer.dll_sha256"):
        print("FAIL: Orca and Filuna binaries identical - provenance invalid", file=sys.stderr)
        return 2
    paths = ["FILUNA/qa/baseline", "FILUNA/tests/evidence_schema.py"] + \
        [f["path"] for f in matrix["fixtures"]] + \
        [p[k] for p in matrix["presets"] for k in ("machine", "process", "filament") if p.get(k)]
    prov["subject_commit"] = args.commit_sha
    prov["inputs_git_objects"] = inputs_tree(paths)
    (args.work / "provenance.json").write_text(json.dumps(prov, indent=2) + "\n", encoding="utf-8")

    cases, any_fail = [], False
    for fx in matrix["fixtures"]:
        model = ROOT / fx["path"]
        if not model.is_file() or sha256_file(model) != fx["sha256"]:
            cases.append({"fixture": fx["id"], "preset": "*", "result": "FAIL", "cause": "fixture missing/hash mismatch"})
            any_fail = True
            continue
        for pr in matrix["presets"]:
            tag = f"{fx['id']}__{pr['id']}"
            c = {"case": tag, "fixture": fx["id"], "format": fx["format"], "preset": pr["id"], "fixture_sha256": fx["sha256"]}
            try:
                rc_o, g_o, _ = run_slice(orca_exe, args.work, model, f"{tag}-orca", pr)
                rc_f, g_f, _ = run_slice(filuna_exe, args.work, model, f"{tag}-filuna", pr)
            except Exception as exc:  # fail-closed
                c.update(result="FAIL", cause=f"exception: {exc}"); cases.append(c); any_fail = True; continue
            c.update(orca_exit=rc_o, filuna_exit=rc_f)
            if g_o is None and g_f is None:
                c.update(result="BLOCKED", cause=f"ambos sem G-code (exit orca={rc_o} filuna={rc_f}); ver logs")
                c["divergent_exit"] = rc_o != rc_f
                if rc_o != rc_f:
                    c["result"] = "FAIL"; any_fail = True
            elif (g_o is None) != (g_f is None):
                c.update(result="FAIL", cause="só um binário gerou G-code"); any_fail = True
            else:
                ro, rf = args.work / f"{tag}-orca.gcode", args.work / f"{tag}-filuna.gcode"
                shutil.copy2(g_o, ro); shutil.copy2(g_f, rf)
                c["raw_sha256"] = {"orca": sha256_file(ro), "filuna": sha256_file(rf)}
                rc = compare(ro, rf)
                neg = negative_probe(ro, rf, args.work, tag)
                c.update(compare_rc=rc, negative_detected=neg,
                         result="PASS" if (rc == 0 and neg) else "FAIL")
                if c["result"] == "FAIL":
                    any_fail = True
            cases.append(c)
            print(json.dumps(c))
    summary = {k: sum(1 for c in cases if c.get("result") == k) for k in ("PASS", "FAIL", "BLOCKED")}
    created = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    out = {"summary": summary, "cases": cases, "provenance": prov, "created_at_utc": created,
           "evidence_url": os.environ.get("FILUNA_EVIDENCE_URL", ""),
           "scope_note": "Cada caso PASS vale só para seu fixture x preset; CORE-04 total exige matriz completa + signoff."}
    args.evidence_out.parent.mkdir(parents=True, exist_ok=True)
    args.evidence_out.write_text(json.dumps(out, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary))
    return 1 if any_fail else 0


if __name__ == "__main__":
    raise SystemExit(main())
