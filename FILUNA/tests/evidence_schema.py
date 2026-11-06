#!/usr/bin/env python3
"""Fail-closed validator for FILUNA/qa/evidence.schema.json records.

Applies the JSON Schema constraints in code (no silent skip if jsonschema
package is absent): required fields, enums, sha patterns, UTC timestamp,
additionalProperties false, provenance filters, G4 PASS rules.
"""
from __future__ import annotations

import json
import re
from datetime import datetime
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
SCHEMA_PATH = ROOT / "FILUNA/qa/evidence.schema.json"

SHA40 = re.compile(r"^[0-9a-f]{40}$")
SHA64 = re.compile(r"^[0-9a-f]{64}$")
# Require explicit Z or +00:00 UTC offset (reject naive / local-only)
UTC_TS = re.compile(
    r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+]00:00)$"
)
GATES = {"G0", "G1", "G2", "G3", "G4", "G5", "G6"}
RESULTS = {"PASS", "FAIL", "BLOCKED", "NOT_RUN"}
KINDS = {"static", "unit", "integration", "document", "hardware"}
RUN_URL = re.compile(r"^https://github[.]com/abcollicelli-web/FilunaSlicer/actions/runs/[1-9][0-9]*$")
PLACEHOLDERS = {"tbd", "todo", "unknown", "placeholder", "none", "n/a", "test", "example"}

# Provenance / false-evidence filters
FALSE_EVIDENCE_MARKERS = (
    "invented", "placeholder", "TODO_PASS", "fake", "assumed pass",
)


class EvidenceValidationError(ValueError):
    pass


def load_schema() -> dict[str, Any]:
    if not SCHEMA_PATH.is_file():
        raise EvidenceValidationError(f"schema missing: {SCHEMA_PATH}")
    try:
        return json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        raise EvidenceValidationError(f"schema unreadable: {exc}") from exc


def _check_utc(ts: str) -> list[str]:
    errs: list[str] = []
    if not isinstance(ts, str) or not UTC_TS.match(ts):
        errs.append("created_at_utc must be ISO-8601 with Z or +00:00")
        return errs
    try:
        datetime.fromisoformat(ts.replace("Z", "+00:00"))
    except ValueError:
        errs.append("created_at_utc not parseable as datetime")
    return errs


def validate_evidence(doc: Any, *, require_schema_file: bool = True) -> list[str]:
    """Return list of error strings; empty means OK. Fail-closed."""
    errors: list[str] = []
    schema: dict[str, Any] | None = None
    if require_schema_file:
        try:
            schema = load_schema()
        except EvidenceValidationError as exc:
            return [str(exc)]

    if not isinstance(doc, dict):
        return ["evidence must be a JSON object"]

    # additionalProperties: false from schema
    allowed = set(schema.get("properties", {}).keys()) if schema else {
        "schema_version", "case_id", "req_id", "gate", "commit_sha", "result",
        "platform", "evidence_kind", "created_at_utc", "fixture_sha256",
        "notes", "evidence_url", "hardware",
    }
    for key in doc:
        if key not in allowed:
            errors.append(f"unknown field: {key}")

    required = list(schema.get("required", [])) if schema else [
        "schema_version", "case_id", "req_id", "gate", "commit_sha",
        "result", "platform", "evidence_kind", "created_at_utc",
    ]
    for key in required:
        if key not in doc:
            errors.append(f"missing required field: {key}")

    if doc.get("schema_version") != 1:
        errors.append("schema_version must be 1")

    for key in ("case_id", "req_id", "platform"):
        val = doc.get(key)
        if key in doc and (not isinstance(val, str) or len(val) < 3):
            errors.append(f"{key} must be non-trivial string")

    if "created_at_utc" in doc:
        errors.extend(_check_utc(doc.get("created_at_utc", "")))

    if "gate" in doc and doc.get("gate") not in GATES:
        errors.append(f"invalid gate: {doc.get('gate')}")
    if "result" in doc and doc.get("result") not in RESULTS:
        errors.append(f"invalid result: {doc.get('result')}")
    if "evidence_kind" in doc and doc.get("evidence_kind") not in KINDS:
        errors.append(f"invalid evidence_kind: {doc.get('evidence_kind')}")

    sha = doc.get("commit_sha", "")
    if "commit_sha" in doc and not (isinstance(sha, str) and SHA40.fullmatch(sha)):
        errors.append("commit_sha must be 40 lowercase hex")

    fx = doc.get("fixture_sha256")
    if fx is not None and fx != "":
        if not (isinstance(fx, str) and SHA64.fullmatch(fx)):
            errors.append("fixture_sha256 must be 64 hex or empty")

    # Provenance filters — notes/url must not claim fake PASS
    blob = " ".join(
        str(doc.get(k, "")) for k in ("notes", "evidence_url", "platform")
    ).lower()
    if doc.get("result") == "PASS":
        for marker in FALSE_EVIDENCE_MARKERS:
            if marker in blob:
                errors.append(f"PASS blocked by false-evidence marker: {marker}")

    if doc.get("result") == "PASS":
        kind = doc.get("evidence_kind")
        url = doc.get("evidence_url")
        if kind in ("integration", "hardware"):
            if not isinstance(url, str) or not RUN_URL.fullmatch(url):
                errors.append(f"PASS+{kind} requires canonical Filuna GitHub Actions run URL")
        if doc.get("gate") == "G4":
            if kind != "hardware":
                errors.append("G4 PASS requires hardware evidence_kind")
            hw = doc.get("hardware")
            if not isinstance(hw, dict):
                errors.append("G4 PASS requires structured hardware object")
            else:
                needed = ("manufacturer", "model", "firmware", "device_fingerprint",
                          "performed_at_utc")
                for key in needed:
                    val = hw.get(key)
                    if not isinstance(val, str) or len(val.strip()) < 3 or val.strip().lower() in PLACEHOLDERS:
                        errors.append(f"G4 hardware {key} missing or placeholder")
                fingerprint = hw.get("device_fingerprint", "")
                if not (isinstance(fingerprint, str) and
                        fingerprint.startswith("sha256:") and SHA64.fullmatch(fingerprint[7:])):
                    errors.append("G4 hardware device_fingerprint must be pseudonymous SHA-256")
                errors.extend("hardware " + err for err in _check_utc(hw.get("performed_at_utc", "")))
                if "operator" in hw and str(hw["operator"]).strip().lower() in PLACEHOLDERS:
                    errors.append("G4 hardware operator is placeholder")
        # Binary baseline claims need commit + fixture hash
        if kind == "integration" and "gcode" in str(doc.get("case_id", "")).lower():
            if not fx:
                errors.append("gcode integration PASS requires fixture_sha256")

    return errors


def validate_evidence_file(path: Path) -> list[str]:
    if not path.is_file():
        return [f"evidence file missing: {path}"]
    try:
        doc = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        return [f"evidence unreadable/invalid JSON: {exc}"]
    return validate_evidence(doc)


if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2:
        print("usage: evidence_schema.py <file.json>", file=sys.stderr)
        raise SystemExit(2)
    errs = validate_evidence_file(Path(sys.argv[1]))
    if errs:
        print("FAIL:")
        for e in errs:
            print(" -", e)
        raise SystemExit(1)
    print("PASS")
