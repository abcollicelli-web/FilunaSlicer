#!/usr/bin/env python3
"""LEGAL-02 CMP checklist runner — fail-closed."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CHECKLIST = ROOT / "FILUNA/legal/CMP-CHECKLIST-LEGAL-02.json"


def run_cmp() -> list[tuple[str, bool, str]]:
    if not CHECKLIST.is_file():
        return [("CMP-MISSING", False, f"checklist absent: {CHECKLIST}")]
    try:
        data = json.loads(CHECKLIST.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        return [("CMP-BADJSON", False, str(exc))]

    results: list[tuple[str, bool, str]] = []
    for item in data.get("items", []):
        cid = item.get("id", "?")
        paths = item.get("paths") or []
        rule = item.get("status_rule")
        ok = True
        detail = "ok"
        if rule == "file_exists_and_contains":
            needles = item.get("must_contain") or []
            if item.get("must_contain_any_per_file"):
                # each file must contain at least one needle
                for rel in paths:
                    fp = ROOT / rel
                    if not fp.is_file():
                        ok = False
                        detail = f"missing {rel}"
                        break
                    text = fp.read_text(encoding="utf-8", errors="replace")
                    if not any(n.lower() in text.lower() for n in needles):
                        ok = False
                        detail = f"{rel} missing any of {needles}"
                        break
            else:
                for rel in paths:
                    fp = ROOT / rel
                    if not fp.is_file():
                        ok = False
                        detail = f"missing {rel}"
                        break
                    text = fp.read_text(encoding="utf-8", errors="replace")
                    for n in needles:
                        if n not in text:
                            ok = False
                            detail = f"{rel} missing `{n}`"
                            break
                    if not ok:
                        break
        elif rule == "policy_doc_forbids_bundle":
            for rel in paths:
                fp = ROOT / rel
                if not fp.is_file():
                    ok = False
                    detail = f"missing {rel}"
                    break
                text = fp.read_text(encoding="utf-8", errors="replace").lower()
                # Document must discuss proprietary/forbid bundling — not claim files are shipped
                if "propriet" not in text and "plugin" not in text:
                    ok = False
                    detail = f"{rel} does not state proprietary/plugin policy"
                    break
                for banned in item.get("must_not_claim_bundled") or []:
                    # Fail if doc claims these are packaged as part of Filuna
                    claim = f"empacota {banned}".lower()
                    if claim in text or f"bundle {banned}".lower() in text:
                        ok = False
                        detail = f"{rel} claims bundling {banned}"
                        break
        else:
            ok = False
            detail = f"unknown rule {rule}"
        results.append((cid, ok, detail))
    return results


def main() -> int:
    rows = run_cmp()
    failed = 0
    for cid, ok, detail in rows:
        print(("PASS" if ok else "FAIL"), cid, detail)
        if not ok:
            failed += 1
    print(f"CMP summary: {len(rows) - failed}/{len(rows)} PASS")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
