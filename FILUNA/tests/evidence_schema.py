#!/usr/bin/env python3
"""Fail-closed validator for FILUNA/qa/evidence.schema.json records."""
from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
SCHEMA_PATH = ROOT / "FILUNA/qa/evidence.schema.json"

SHA40 = re.compile(r"^[0-9a-f]{40}$")
SHA64 = re.compile(r"^[0-9a-f]{64}$")
GATES = {"G0", "G1", "G2", "G3", "G4", "G5", "G6"}
RESULTS = {"PASS", "FAIL", "BLOCKED", "NOT_RUN"}
KINDS = {"static", "unit", "integration", "document", "hardware"}


class EvidenceValidationError(ValueError):
    pass


def load_schema() -> dict[str, Any]:
    if not SCHEMA_PATH.is_file():
        raise EvidenceValidationError(f"schema missing: {SCHEMA_PATH}")
    try:
        return json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        raise EvidenceValidationError(f"schema unreadable: {exc}") from exc


def validate_evidence(doc: Any, *, require_schema_file: bool = True) -> list[str]:
    """Return list of error strings; empty means OK. Fail-closed: any doubt = error."""
    errors: list[str] = []
    if require_schema_file:
        try:
            load_schema()
        except EvidenceValidationError as exc:
            return [str(exc)]

    if not isinstance(doc, dict):
        return ["evidence must be a JSON object"]

    required = [
        "schema_version", "case_id", "req_id", "gate", "commit_sha",
        "result", "platform", "evidence_kind", "created_at_utc",
    ]
    for key in required:
        if key not in doc:
            errors.append(f"missing required field: {key}")

    if doc.get("schema_version") != 1:
        errors.append("schema_version must be 1")

    for key in ("case_id", "req_id", "platform", "created_at_utc"):
        val = doc.get(key)
        if key in doc and (not isinstance(val, str) or len(val) < 3):
            errors.append(f"{key} must be non-trivial string")

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

    # Fail-closed: PASS cannot be claimed with pending/blocked language alone
    if doc.get("result") == "PASS":
        kind = doc.get("evidence_kind")
        url = doc.get("evidence_url")
        if kind == "hardware" and not url:
            errors.append("PASS+hardware requires evidence_url")
        if doc.get("gate") == "G4":
            if not url:
                errors.append("G4 PASS requires evidence_url")
            hw = doc.get("hardware")
            if not isinstance(hw, dict) or not hw:
                errors.append("G4 PASS requires non-empty hardware object")

    # Absent evidence_url is OK for static/unit/document PASS, but explicit null hash mismatch fields
    if "extra_forbidden" in doc:
        errors.append("additionalProperties not allowed: extra_forbidden")

    # Unknown keys = fail closed (additionalProperties: false)
    allowed = {
        "schema_version", "case_id", "req_id", "gate", "commit_sha", "result",
        "platform", "evidence_kind", "created_at_utc", "fixture_sha256",
        "notes", "evidence_url", "hardware",
    }
    for key in doc:
        if key not in allowed:
            errors.append(f"unknown field: {key}")

    return errors


def validate_evidence_file(path: Path) -> list[str]:
    if not path.is_file():
        return [f"evidence file missing: {path}"]
    try:
        doc = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        return [f"evidence unreadable/invalid JSON: {exc}"]
    return validate_evidence(doc)
