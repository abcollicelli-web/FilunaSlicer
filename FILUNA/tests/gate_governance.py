#!/usr/bin/env python3
"""Fail-closed gate governance: a gate/wave can only be PASS if EVERY required
case from the versioned contract (FILUNA/qa/required-cases.json) has exactly one
valid, externally-anchored evidence record. Self-declared PASS is never proof.

Blocks on: missing/duplicate cases, unknown IDs, contract version mismatch,
synthetic data presented as product evidence, text-only evidence, missing CI
run/artifact or binary digest, commit/platform mismatch, N/A without
justification+owner acceptance, missing signoff.
"""
from __future__ import annotations
import json, re, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "FILUNA/tests"))
from evidence_schema import validate_evidence  # noqa: E402

CONTRACT = ROOT / "FILUNA/qa/required-cases.json"
RUN_URL = re.compile(r"^https://github\.com/[\w.-]+/[\w.-]+/actions/runs/\d+$")
SHA64 = re.compile(r"^[0-9a-f]{64}$")


def evaluate(records: list[dict], *, contract: dict, gate: str, commit_sha: str,
             signoff: dict | None) -> tuple[str, list[str]]:
    errs: list[str] = []
    req = [c for c in contract["required_cases"] if c["gate"] == gate]
    if not req:
        return "BLOCKED", [f"no required cases for {gate} in contract"]
    req_ids = {c["case_id"]: c for c in req}
    all_ids = {c["case_id"] for c in contract["required_cases"]}
    seen: dict[str, int] = {}
    for r in records:
        cid = r.get("case_id")
        seen[cid] = seen.get(cid, 0) + 1
        if cid not in all_ids:
            errs.append(f"unknown case_id: {cid}")
        if r.get("contract_version") != contract["contract_version"]:
            errs.append(f"{cid}: contract_version mismatch")
    for cid, n in seen.items():
        if n > 1:
            errs.append(f"duplicate case_id: {cid}")
    for cid, spec in req_ids.items():
        recs = [r for r in records if r.get("case_id") == cid]
        if not recs:
            errs.append(f"missing required case: {cid}")
            continue
        r = recs[0]
        ev = r.get("evidence", {})
        res = ev.get("result")
        if res == "N/A":
            if not (r.get("na_justification") and r.get("na_owner_acceptance")):
                errs.append(f"{cid}: N/A without justification+owner acceptance")
            continue
        verr = validate_evidence(ev)
        if verr:
            errs.append(f"{cid}: schema: {verr}")
        if res != "PASS":
            errs.append(f"{cid}: result={res}")
            continue
        if r.get("synthetic") is not False:
            errs.append(f"{cid}: synthetic/unspecified data cannot prove product case")
        if ev.get("evidence_kind") != spec["kind_required"]:
            errs.append(f"{cid}: kind {ev.get('evidence_kind')} != {spec['kind_required']}")
        if not RUN_URL.match(ev.get("evidence_url", "")):
            errs.append(f"{cid}: evidence_url must be a CI run URL (text-only not accepted)")
        if not r.get("artifact_id"):
            errs.append(f"{cid}: missing CI artifact_id")
        if not SHA64.match(r.get("binary_sha256", "")):
            errs.append(f"{cid}: missing binary/installer sha256")
        if ev.get("commit_sha") != commit_sha:
            errs.append(f"{cid}: commit mismatch")
        if spec["kind_required"] == "integration" and "windows" in cid and \
                not str(ev.get("platform", "")).startswith("windows"):
            errs.append(f"{cid}: platform mismatch")
    if not signoff or not signoff.get("owner") or not signoff.get("at_utc"):
        errs.append("missing owner signoff for gate")
    return ("PASS" if not errs else "BLOCKED"), errs


def main() -> int:
    contract = json.loads(CONTRACT.read_text(encoding="utf-8"))
    ev_dir = ROOT / "FILUNA/qa/evidence"
    records = [json.loads(p.read_text(encoding="utf-8")) for p in sorted(ev_dir.glob("*.json"))] if ev_dir.is_dir() else []
    import subprocess
    head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    worst = 0
    for gate in ("G2", "G3", "G4"):
        st, errs = evaluate(records, contract=contract, gate=gate, commit_sha=head, signoff=None)
        print(f"{gate}: {st} ({len(errs)} bloqueio(s))")
        for e in errs[:12]:
            print("  -", e)
        worst = max(worst, 0 if st == "PASS" else 1)
    print("NOTA: BLOCKED aqui é o resultado correto enquanto houver casos obrigatórios sem execução.")
    return 0  # informational; tests assert behavior

if __name__ == "__main__":
    raise SystemExit(main())
