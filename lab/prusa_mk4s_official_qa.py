#!/usr/bin/env python3
"""Prusa MK4S 0.4 official v2.9.7 portable Windows internal QA.

Uses an SHA-pinned *official* config bundle, derives temporary effective
settings and runs the official CLI twice. This does not prove Filuna parity.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import urllib.request

from portable_first_qa import OFFICIAL, digest, extract_executable, obtain

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ("https://raw.githubusercontent.com/prusa3d/PrusaSlicer/"
          "version_2.9.7/resources/profiles/PrusaResearch.ini")
BLOB = "b7872a7c735329153b8508cfa53a3461a3a2bf7d"
MODEL = "Original Prusa MK4S 0.4 nozzle"
PRINT = "0.20mm SPEED @MK4S 0.4"
FILAMENT = "Prusament PLA @MK4S"
METADATA = {
    "inherits", "renamed_from", "compatible_printers_condition",
    "compatible_prints_condition", "default_print_profile",
    "default_filament_profile",
    "printer_preset", "print_preset", "filament_preset"
}


def git_blob(data: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


def sections(text: str) -> dict[tuple[str, str], dict[str, str]]:
    result = {}
    active = None
    for line in text.splitlines():
        match = re.fullmatch(r"\[(printer|print|filament):(.+)\]", line.strip())
        if match:
            active = (match.group(1), match.group(2))
            if active in result:
                raise ValueError("DUPLICATE_OFFICIAL_SECTION " + repr(active))
            result[active] = {}
        elif line.strip().startswith("[") and line.strip().endswith("]"):
            active = None
        elif active and line.strip() and not line.lstrip().startswith(("#", ";")):
            if "=" in line:
                key, value = line.split("=", 1)
                key = key.strip()
                if key in result[active]:
                    raise ValueError("DUPLICATE_OFFICIAL_KEY " + repr((active, key)))
                result[active][key] = value.strip()
    return result


def resolve(index, kind: str, name: str, stack=()):
    key = (kind, name)
    if key in stack:
        raise ValueError("OFFICIAL_INHERITANCE_CYCLE " + repr(stack + (key,)))
    if key not in index:
        raise ValueError("UNRESOLVED_OFFICIAL_PARENT " + repr(key))
    entry = index[key]
    parents = [x.strip() for x in entry.get("inherits", "").split(";") if x.strip()]
    merged, lineage = {}, []
    for parent in parents:
        config, chain = resolve(index, kind, parent, stack + (key,))
        merged.update(config)
        lineage.extend(chain)
    merged.update(entry)
    merged.pop("inherits", None)
    return merged, list(dict.fromkeys(lineage + [name]))


def effective_config(index):
    printer, pc = resolve(index, "printer", MODEL)
    process, qc = resolve(index, "print", PRINT)
    filament, fc = resolve(index, "filament", FILAMENT)
    if printer.get("printer_model") != "MK4S" or printer.get("printer_variant") != "0.4":
        raise ValueError("WRONG_OFFICIAL_MODEL_OR_NOZZLE")
    if printer.get("default_print_profile") != PRINT:
        raise ValueError("PRINT_DEFAULT_MISMATCH")
    if printer.get("default_filament_profile", "").strip('"') != FILAMENT:
        raise ValueError("FILAMENT_DEFAULT_MISMATCH")
    if printer.get("nozzle_diameter", "").split(";")[0] != "0.4":
        raise ValueError("NOZZLE_DIAMETER_NOT_0_4")
    merged, collisions = {}, []
    for name, part in (("print", process), ("filament", filament), ("printer", printer)):
        for key, value in part.items():
            if key in METADATA:
                continue
            if key in merged and merged[key] != value:
                collisions.append({"field": key, "earlier": merged[key], "later": value, "from": name})
            merged[key] = value
    if not merged.get("start_gcode") or not merged.get("bed_shape"):
        raise ValueError("EFFECTIVE_PRINTER_REQUIRED_FIELDS_ABSENT")
    return merged, {"printer": pc, "print": qc, "filament": fc}, collisions


def command(exe, argv, logfile, cwd, timeout):
    with logfile.open("w", encoding="utf-8", errors="replace") as out:
        out.write("COMMAND_JSON=" + json.dumps([str(exe), *argv], ensure_ascii=False) + "\n")
        out.flush()
        try:
            return subprocess.run([str(exe), *argv], cwd=cwd, stdout=out,
                                  stderr=subprocess.STDOUT, timeout=timeout, check=False).returncode
        except subprocess.TimeoutExpired:
            out.write("\nQA_TIMEOUT\n")
            return 124
        except OSError as error:
            out.write("\nQA_OSERROR=" + repr(error) + "\n")
            return 127


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cache", type=Path, default=Path(".oem-portable-cache"))
    parser.add_argument("--out", type=Path, default=Path("qa-prusa-mk4s"))
    parser.add_argument("--timeout", type=int, default=180)
    args = parser.parse_args()
    cache, out = args.cache.resolve(), args.out.resolve()
    cache.mkdir(parents=True, exist_ok=True)
    out.mkdir(parents=True, exist_ok=True)
    rows = []

    def add(stage, status, detail, evidence=""):
        assert status in ("PASS", "FAIL", "PENDING")
        rows.append({"stage": stage, "status": status, "detail": detail, "evidence": evidence})
        print("PRUSA_MK4S", stage, status, detail, flush=True)

    try:
        cfg = OFFICIAL["prusa"]
        archive, url = obtain(cfg, cache)
        add("OFFICIAL_ZIP", "PASS", "sha256=" + digest(archive), url)
        exe = extract_executable(archive, cfg["exe"], cache / "prusa-portable-extracted").resolve()
        add("OFFICIAL_CONSOLE_EXE", "PASS", str(exe))
        request = urllib.request.Request(SOURCE, headers={"User-Agent": "FilunaOEM-QA/1"})
        with urllib.request.urlopen(request, timeout=90) as response:
            raw = response.read(4 * 1024 * 1024 + 1)
        if len(raw) > 4 * 1024 * 1024 or git_blob(raw) != BLOB:
            raise ValueError("OFFICIAL_PRUSA_2_9_7_BUNDLE_BLOB_MISMATCH")
        bundle = out / "PrusaResearch-2.9.7-official.ini"
        bundle.write_bytes(raw)
        index = sections(raw.decode("utf-8-sig"))
        settings, lineage, collisions = effective_config(index)
        effective = out / "derived-MK4S-0.4.ini"
        effective.write_text(
            "# Derived only from the SHA-pinned PrusaResearch.ini; not an OEM source file\n"
            + "\n".join(key + " = " + val for key, val in sorted(settings.items())) + "\n",
            encoding="utf-8")
        evidence = {
            "model": MODEL, "print": PRINT, "filament": FILAMENT,
            "official_repo": "prusa3d/PrusaSlicer", "official_tag": "version_2.9.7",
            "official_bundle_url": SOURCE, "official_bundle_git_blob": BLOB,
            "official_bundle_sha256": hashlib.sha256(raw).hexdigest(),
            "derived_config_sha256": digest(effective),
            "inheritance": lineage, "overridden_keys": collisions,
            "note": "Original bundle immutable; derivation is not Filuna parity"}
        (out / "upstream.lock.json").write_text(
            json.dumps(evidence, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        add("OFFICIAL_MK4S_04_INHERITANCE", "PASS",
            "verified official profile; derived temporary effective config", "upstream.lock.json")

        cube = ROOT / "fixtures/cube20.stl"
        if not cube.is_file():
            raise ValueError("MISSING_CUBE20_FIXTURE")
        add("CUBE_20MM", "PASS", "sha256=" + digest(cube))
        outputs = []
        for letter in ("A", "B"):
            directory = out / ("run-" + letter)
            if directory.exists():
                shutil.rmtree(directory)
            directory.mkdir()
            gc = directory / "mk4s-0.4.gcode"
            argv = ["--printer-technology", "FFF", "--load", str(effective),
                    "--export-gcode", "--output", str(gc), str(cube.resolve())]
            rc = command(exe, argv, out / ("official-" + letter + ".log"),
                         exe.parent, args.timeout)
            found = gc.is_file() and gc.stat().st_size > 0
            (out / ("official-" + letter + "-inventory.json")).write_text(
                json.dumps({"exit_code": rc, "output": str(gc), "file_present": found,
                            "bytes": gc.stat().st_size if found else 0,
                            "sha256": digest(gc) if found else None}, indent=2) + "\n")
            if rc != 0 or not found:
                add("OFFICIAL_" + letter, "FAIL",
                    "CLI_EXIT=" + str(rc) + " GCODE_PRESENT=" + str(found),
                    "official-" + letter + ".log")
                break
            add("OFFICIAL_" + letter, "PASS",
                "bytes=" + str(gc.stat().st_size) + " sha256=" + digest(gc),
                "official-" + letter + "-inventory.json")
            outputs.append(gc)
        if len(outputs) != 2:
            add("OFFICIAL_A_B", "PENDING", "OFFICIAL_CLI_FAILED")
        elif outputs[0].resolve() == outputs[1].resolve():
            add("OFFICIAL_A_B", "FAIL", "REUSED_SAME_FILE")
        elif digest(outputs[0]) != digest(outputs[1]):
            left = outputs[0].read_text(encoding="utf-8", errors="replace").splitlines()
            right = outputs[1].read_text(encoding="utf-8", errors="replace").splitlines()
            changes = [{"line": i + 1, "A": a[:240], "B": b[:240]}
                       for i, (a, b) in enumerate(zip(left, right)) if a != b][:30]
            (out / "A-B-differences.json").write_text(json.dumps(
                {"A": digest(outputs[0]), "B": digest(outputs[1]),
                 "lines_A": len(left), "lines_B": len(right),
                 "first_differences": changes}, indent=2) + "\n")
            add("OFFICIAL_A_B", "FAIL", "RAW_GCODE_SHA_MISMATCH; no normalization allowed",
                "A-B-differences.json")
        else:
            add("OFFICIAL_A_B", "PASS", "RAW_SHA256_IDENTICAL " + digest(outputs[0]))
        add("FILUNA_PARITY", "PENDING", "NO_SEPARATE_FILUNA_GCODE_EXECUTION")
        add("NETWORK", "PENDING", "NO_SIMULATOR_OR_DEVICE_BRIDGE_EVIDENCE")
    except Exception as err:
        add("RUNNER", "FAIL", type(err).__name__ + ": " + str(err))

    (out / "results.json").write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n")
    summary = [
        "# Prusa MK4S 0.4 — Official portable Windows internal QA",
        "Official PrusaSlicer 2.9.7 CLI only; no Filuna parity, network or physical certification.",
        "",
        "| Stage | Result | Detail |", "|---|---|---|"
    ]
    summary += ["| " + r["stage"] + " | " + r["status"] + " | "
                + r["detail"].replace("|", "/") + " |" for r in rows]
    (out / "RESUMO.md").write_text("\n".join(summary) + "\n", encoding="utf-8")
    return 1 if any(x["status"] == "FAIL" for x in rows) else 0


if __name__ == "__main__":
    raise SystemExit(main())
