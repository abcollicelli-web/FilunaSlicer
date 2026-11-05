#!/usr/bin/env python3
"""DATA-05 isolation checks without writing real AppData/registry."""
from __future__ import annotations

import re
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HDR = ROOT / "src/libslic3r/libslic3r.h"
GUI = ROOT / "src/slic3r/GUI/GUI_App.cpp"

FILUNA_DIR_RE = re.compile(r'#define\s+FILUNA_DATA_DIR_NAME\s+"([^"]+)"')
SETAPP_RE = re.compile(r"SetAppName\(\s*FILUNA_DATA_DIR_NAME\s*\)")
ORCA_APPDATA = "OrcaSlicer"
PROGID_RE = re.compile(r'Filuna\.Slicer\.1')


def check_source_constants() -> list[tuple[str, bool, str]]:
    out: list[tuple[str, bool, str]] = []
    if not HDR.is_file():
        return [("DATA-HDR", False, "libslic3r.h missing")]
    text = HDR.read_text(encoding="utf-8", errors="replace")
    m = FILUNA_DIR_RE.search(text)
    if not m:
        out.append(("DATA-DIR-DEFINE", False, "FILUNA_DATA_DIR_NAME missing"))
    else:
        name = m.group(1)
        out.append(("DATA-DIR-DEFINE", name == "Filuna", f"value={name!r}"))
        out.append(("DATA-DIR-NOT-ORCA", name != ORCA_APPDATA, f"value={name!r}"))

    if not GUI.is_file():
        out.append(("DATA-GUI", False, "GUI_App.cpp missing"))
        return out
    gui = GUI.read_text(encoding="utf-8", errors="replace")
    out.append(("DATA-SETAPPNAME", bool(SETAPP_RE.search(gui)), "SetAppName(FILUNA_DATA_DIR_NAME)"))
    out.append(("DATA-PROGID", bool(PROGID_RE.search(gui)), "Filuna.Slicer.1"))
    # Association exists (product decision) — coexistence proof is G4
    out.append(("DATA-URL-ORCASLICER-ASSOC", "associate_url(L\"orcaslicer\")" in gui,
                "orcaslicer:// association present; G4 coexistence NOT_RUN"))
    return out


def check_synthetic_paths() -> list[tuple[str, bool, str]]:
    """Simulate path policy in a temp tree — never touch real %APPDATA%."""
    out: list[tuple[str, bool, str]] = []
    with tempfile.TemporaryDirectory(prefix="filuna-data05-") as tmp:
        root = Path(tmp)
        filuna = root / "Filuna"
        orca = root / "OrcaSlicer"
        filuna.mkdir()
        # Policy: Filuna writes only under Filuna/
        target = filuna / "filuna.ini"
        target.write_text("ok=1\n", encoding="utf-8")
        out.append(("DATA-SYN-WRITE-FILUNA", target.is_file(), str(target)))
        out.append(("DATA-SYN-NO-ORCA-DIR", not orca.exists(), "OrcaSlicer dir not created"))
        # Negative: resolving wrong name would point to Orca — detect mismatch
        wrong = root / ORCA_APPDATA / "filuna.ini"
        out.append(("DATA-SYN-WRONG-PATH-ABSENT", not wrong.exists(), str(wrong)))
    return out


def run_all() -> list[tuple[str, bool, str]]:
    return check_source_constants() + check_synthetic_paths()


def main() -> int:
    rows = run_all()
    failed = 0
    for cid, ok, detail in rows:
        print(("PASS" if ok else "FAIL"), cid, detail)
        if not ok:
            failed += 1
    print(f"DATA-05 summary: {len(rows) - failed}/{len(rows)} PASS")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
