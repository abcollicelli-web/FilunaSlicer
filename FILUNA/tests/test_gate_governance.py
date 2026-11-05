#!/usr/bin/env python3
import copy, json, sys, unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "FILUNA/tests"))
from gate_governance import evaluate  # noqa: E402

C = json.loads((ROOT / "FILUNA/qa/required-cases.json").read_text())
SHA = "b" * 40
SIGN = {"owner": "dono", "at_utc": "2026-11-05T22:40:00Z"}


def rec(cid, kind="integration", platform="windows-actions-x64", **over):
    r = {
        "case_id": cid, "contract_version": C["contract_version"], "synthetic": False,
        "artifact_id": "123", "binary_sha256": "c" * 64,
        "evidence": {"schema_version": 1, "case_id": cid, "req_id": "REQ-FUND-004", "gate": "G3",
                     "commit_sha": SHA, "result": "PASS", "platform": platform, "evidence_kind": kind,
                     "created_at_utc": "2026-11-05T22:40:00Z", "fixture_sha256": "d" * 64,
                     "evidence_url": "https://github.com/o/r/actions/runs/1"},
    }
    r.update(over)
    return r


def full_g3():
    return [rec("FUND-T07-gcode-bin-windows"), rec("FUND-DATA05-appdata-windows")]


class T(unittest.TestCase):
    def ev(self, recs, sign=SIGN, gate="G3"):
        return evaluate(recs, contract=C, gate=gate, commit_sha=SHA, signoff=sign)

    def test_positive_complete(self):
        self.assertEqual(self.ev(full_g3())[0], "PASS")

    def test_empty_blocks(self):
        self.assertEqual(self.ev([])[0], "BLOCKED")

    def test_missing_case_blocks(self):
        self.assertEqual(self.ev(full_g3()[:1])[0], "BLOCKED")

    def test_duplicate_blocks(self):
        self.assertEqual(self.ev(full_g3() + full_g3()[:1])[0], "BLOCKED")

    def test_unknown_id_blocks(self):
        self.assertEqual(self.ev(full_g3() + [rec("FUND-X-UNKNOWN")])[0], "BLOCKED")

    def test_synthetic_blocks(self):
        r = full_g3(); r[0]["synthetic"] = True
        self.assertEqual(self.ev(r)[0], "BLOCKED")

    def test_text_only_url_blocks(self):
        r = full_g3(); r[0]["evidence"]["evidence_url"] = "ver REGISTRO.md"
        self.assertEqual(self.ev(r)[0], "BLOCKED")

    def test_no_binary_hash_blocks(self):
        r = full_g3(); r[0]["binary_sha256"] = ""
        self.assertEqual(self.ev(r)[0], "BLOCKED")

    def test_commit_mismatch_blocks(self):
        r = full_g3(); r[0]["evidence"]["commit_sha"] = "e" * 40
        self.assertEqual(self.ev(r)[0], "BLOCKED")

    def test_linux_as_windows_blocks(self):
        r = full_g3(); r[0]["evidence"]["platform"] = "linux-box"
        self.assertEqual(self.ev(r)[0], "BLOCKED")

    def test_na_without_acceptance_blocks(self):
        r = full_g3(); r[1]["evidence"]["result"] = "N/A"
        self.assertEqual(self.ev(r)[0], "BLOCKED")

    def test_no_signoff_blocks(self):
        self.assertEqual(self.ev(full_g3(), sign=None)[0], "BLOCKED")

    def test_contract_version_mismatch_blocks(self):
        r = full_g3(); r[0]["contract_version"] = "old"
        self.assertEqual(self.ev(r)[0], "BLOCKED")

    def test_not_run_blocks(self):
        r = full_g3(); r[0]["evidence"]["result"] = "NOT_RUN"
        self.assertEqual(self.ev(r)[0], "BLOCKED")

    def test_repo_state_today_is_blocked(self):
        # Real repo: no product evidence registered -> every gate BLOCKED
        for g in ("G2", "G3", "G4"):
            self.assertEqual(evaluate([], contract=C, gate=g, commit_sha=SHA, signoff=None)[0], "BLOCKED")


if __name__ == "__main__":
    unittest.main(verbosity=2)
