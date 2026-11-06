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
import argparse, json, os, re, sys
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "FILUNA/tests"))
from evidence_schema import validate_evidence  # noqa: E402

CONTRACT = ROOT / "FILUNA/qa/required-cases.json"
RUN_URL = re.compile(r"^https://github\.com/[\w.-]+/[\w.-]+/actions/runs/\d+$")
SHA64 = re.compile(r"^[0-9a-f]{64}$")



def verify_github_run(record: dict) -> list[str]:
    """Corroborate CI run/artifact via GitHub. Never infer hardware from a CI run.

    A successful Actions job and its artifact are only technical provenance:
    a human must still verify device, firmware, physical logs and signoff.
    """
    errors: list[str] = []
    ev = record.get("evidence", {})
    m = RUN_URL.fullmatch(str(ev.get("evidence_url", "")))
    if not m:
        return ["invalid canonical Filuna GitHub Actions run URL"]
    token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
    if not token:
        return ["no GitHub token available for remote run/artifact verification"]

    def request(path: str) -> dict:
        req = urllib.request.Request(
            "https://api.github.com/repos/abcollicelli-web/FilunaSlicer/" + path,
            headers={
                "Authorization": f"Bearer {token}",
                "Accept": "application/vnd.github+json",
                "User-Agent": "Filuna-quality-gate",
                "X-GitHub-Api-Version": "2022-11-28",
            },
        )
        with urllib.request.urlopen(req, timeout=10) as reply:
            if reply.status != 200:
                raise ValueError(f"GitHub returned HTTP {reply.status}")
            blob = reply.read(2_000_000)
        return json.loads(blob)

    try:
        run_id = m.group(1)
        run = request(f"actions/runs/{run_id}")
        if run.get("status") != "completed" or run.get("conclusion") != "success":
            errors.append("GitHub run not completed/success")
        if run.get("head_sha") != ev.get("commit_sha"):
            errors.append("GitHub run head_sha != evidence commit_sha")
        art_id = str(record.get("artifact_id", ""))
        if not art_id.isdigit() or int(art_id) < 1:
            errors.append("invalid numeric artifact_id")
        else:
            artifact = request(f"actions/artifacts/{art_id}")
            if str(artifact.get("workflow_run", {}).get("id")) != str(run_id):
                errors.append("artifact workflow_run.id != GitHub evidence run")
            if artifact.get("expired") is True or artifact.get("size_in_bytes", 0) <= 0:
                errors.append("artifact expired or empty")
            if not artifact.get("name"):
                errors.append("artifact has no name")
    except (urllib.error.URLError, OSError, ValueError, TypeError,
            json.JSONDecodeError) as exc:
        errors.append(f"failed to corroborate GitHub run/artifact: {type(exc).__name__}")
    return errors

def evaluate(records: list[dict], *, contract: dict, gate: str, commit_sha: str,
             signoff: dict | None, verify_external=None) -> tuple[str, list[str]]:
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
        if verify_external is None:
            errs.append(f"{cid}: no external CI verification supplied; promotion denied")
        else:
            errs.extend(f"{cid}: {e}" for e in verify_external(r))
        if ev.get("commit_sha") != commit_sha:
            errs.append(f"{cid}: commit mismatch")
        if spec["kind_required"] == "integration" and "windows" in cid and \
                not str(ev.get("platform", "")).startswith("windows"):
            errs.append(f"{cid}: platform mismatch")
    if not signoff or not signoff.get("owner") or not signoff.get("at_utc"):
        errs.append("missing owner signoff for gate")
    return ("PASS" if not errs else "BLOCKED"), errs


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Filuna gate: report is informational; enforce is fail-closed."
    )
    parser.add_argument("--mode", choices=("report", "enforce", "assert-blocked"),
                        default="report")
    parser.add_argument("--gate", action="append", choices=("G2", "G3", "G4"))
    args = parser.parse_args()
    contract = json.loads(CONTRACT.read_text(encoding="utf-8"))
    ev_dir = ROOT / "FILUNA/qa/evidence"
    records = [json.loads(p.read_text(encoding="utf-8"))
               for p in sorted(ev_dir.glob("*.json"))] if ev_dir.is_dir() else []
    import subprocess
    head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT,
                                   text=True).strip()
    gates = args.gate or ["G2", "G3", "G4"]
    blocked = 0
    for gate in gates:
        state, errs = evaluate(records, contract=contract, gate=gate,
                               commit_sha=head, signoff=None,
                               verify_external=verify_github_run
                               if args.mode == "enforce" else None)
        print(f"{gate}: {state} ({len(errs)} bloqueio(s))")
        for reason in errs[:16]:
            print("  -", reason)
        blocked += state != "PASS"
    if args.mode == "assert-blocked":
        if blocked != len(gates):
            print("FAIL: a gate unexpectedly passed without promotion evidence")
            return 1
        print("PASS: all requested unapproved gates remain blocked")
        return 0
    if args.mode == "enforce":
        if contract.get("global_promotion_allowed") is not True:
            print("BLOCKED: contract is only a foundation subset; global promotion disabled")
            return 1
        return 1 if blocked else 0
    print("REPORT ONLY: an exit 0 is NOT approval; use --mode enforce to promote.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
