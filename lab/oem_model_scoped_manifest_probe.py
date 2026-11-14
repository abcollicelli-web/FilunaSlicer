#!/usr/bin/env python3
"""Probe *existing* Filuna CLI with model-scoped OEM system preset source trees.

Temporary QA ONLY. Repackage three already-resolved profile *values* without
inventing printer parameters. Preserves untouched original profiles and records
their SHA. This is not signed client distribution, native OEM integration,
G-code equivalence, a compiled Filuna patch or device homologation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

from filuna_baseline_three_oem_qacheck import h, model_files
from oem_functional_gcode_diff import analyze_bytes


def stable_digest(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def safe_name(s: object) -> str:
    if not isinstance(s, str) or not s or len(s) > 240 or any(
        c in s for c in ("/", "\\", "\0", "\r", "\n")
    ) or s in (".", "..") or s.startswith("."):
        raise ValueError("UNSAFE_OEM_PRESET_NAME")
    return s


def make_source_bundle(profiles: dict[str, dict], manufacturer: str,
                       out: Path) -> dict:
    """Effective presets already passed the source/inheritance review.

    Clearing 'inherits' is a metadata transport step only: all other effective
    values MUST stay identical, including zero and firmware macros. This is
    explicitly a one-model source-manifest experiment, not an installable
    profile update and not evidence that the full vendor inheritance works.
    """
    if not re.fullmatch(r"[A-Z][A-Za-z0-9_-]{1,39}", manufacturer):
        raise ValueError("INVALID_OEM_MANUFACTURER")
    if set(profiles) != {"machine", "process", "filament"}:
        raise ValueError("THREE_RESOLVED_PROFILE_TYPES_REQUIRED")
    for kind, data in profiles.items():
        if not isinstance(data, dict) or data.get("type") != kind:
            raise ValueError("INVALID_EFFECTIVE_OEM_PROFILE_KIND")
        safe_name(data.get("name"))
        if data.get("from") != "system":
            raise ValueError("ONLY_VERIFIED_OEM_SYSTEM_PRESETS_ALLOWED")
        if not isinstance(data.get("inherits", ""), str):
            raise ValueError("INVALID_INHERITS")
    machine = profiles["machine"]
    model_id = safe_name(machine.get("printer_model"))
    if not isinstance(machine.get("nozzle_diameter"), list) or not machine["nozzle_diameter"]:
        raise ValueError("OEM_NOZZLE_IDENTITY_MISSING")
    for kind in ("process", "filament"):
        eligible = profiles[kind].get("compatible_printers")
        if not isinstance(eligible, list) or machine["name"] not in eligible:
            raise ValueError("PROFILE_COMPATIBILITY_NOT_PROVEN")
        cond = profiles[kind].get("compatible_printers_condition", "")
        if cond not in ("", None):
            raise ValueError("OEM_COMPATIBILITY_PREDICATE_NOT_EVALUATED")
    if out.exists() and any(out.iterdir()):
        raise ValueError("SOURCE_BUNDLE_DESTINATION_NOT_EMPTY")
    out.mkdir(parents=True, exist_ok=True)
    vendor = out / manufacturer
    vendor.mkdir()
    manifest = {
        "name": manufacturer, "version": machine.get("version", "0.0.0.0"),
        "force_update": "0", "description": "Filuna ephemeral CLI QA model subset",
        "machine_model_list": [], "machine_list": [],
        "process_list": [], "filament_list": []
    }
    originals = {}
    for kind in ("machine", "process", "filament"):
        original = profiles[kind]
        transformed = dict(original)
        transformed["inherits"] = ""  # already-resolved effective inheritance
        # The only changed field is inheritance metadata; keep from=system,
        # manufacturer model, speed, temperature, material and every macro.
        if any(original.get(k) != transformed.get(k) for k in original if k != "inherits"):
            raise AssertionError("TUNING_PARAMETER_CHANGED")
        relative = Path(kind) / (safe_name(original["name"]) + ".json")
        target = vendor / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        raw = (json.dumps(transformed, ensure_ascii=False, indent=2) + "\n").encode()
        target.write_bytes(raw)
        manifest[kind + "_list"].append({
            "name": original["name"], "sub_path": relative.as_posix()
        })
        originals[kind] = {
            "name": original["name"],
            "source_effective_sha256": stable_digest(
                json.dumps(original, ensure_ascii=False, sort_keys=True,
                           separators=(",", ":")).encode()),
            "qa_source_sha256": stable_digest(raw),
            "inherits_original": original.get("inherits", ""),
            "only_metadata_changed": True
        }
    (out / (manufacturer + ".json")).write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8"
    )
    return {
        "scope": "EPHEMERAL_VENDOR_MANIFEST_PREBUILT_CLI_PROBE",
        "manufacturer": manufacturer,
        "model": machine["name"],
        "printer_model": model_id,
        "nozzle_diameter": machine["nozzle_diameter"],
        "profile_sources": originals,
        "qa_manifest": str(out / (manufacturer + ".json")),
        "native_device_module": "NOT_INTEGRATED",
        "native_gcode_parity": "NOT_PROVEN",
        "shipping_status": "FORBIDDEN_QA_ONLY"
    }


def run_one(vendor: str, model: str, artifact: Path, exe: Path, stl: Path,
            out: Path) -> dict:
    target = out / vendor
    target.mkdir(parents=True, exist_ok=True)
    result = {
        "vendor": vendor, "model": model, "test_kind": "OEM_MINIMAL_SOURCE_TREE_DIAGNOSTIC",
        "status": "NOT_RUN", "native_filuna_parity": "NOT_PROVEN",
        "device_connection": "NOT_RUN", "physical_beta": "NOT_RUN"
    }
    try:
        original_files, official, _ = model_files(vendor, artifact, model, target)
        for f in [*original_files.values(), official]:
            if not f.is_file():
                raise FileNotFoundError(str(f))
        sources = {k: json.loads(path.read_text(encoding="utf-8-sig"))
                   for k, path in original_files.items()}
        manufacturer = {"anycubic": "Anycubic", "flashforge": "Flashforge",
                        "snapmaker": "Snapmaker"}[vendor]
        tree = target / "source"
        result["source_bundle"] = make_source_bundle(sources, manufacturer, tree)
        paths = {k: (tree / manufacturer / k / (safe_name(v["name"]) + ".json")).resolve()
                 for k, v in sources.items()}
        work = target / "sliced"
        work.mkdir()
        filament_count = len(sources["machine"]["nozzle_diameter"])
        # For Snapmaker tests the OEM original needs the four tool slots; else
        # use one actual recorded nozzle. No additional material values invented.
        filament_slots = max(filament_count, 4 if vendor == "snapmaker" else 1)
        argv = [
            str(exe.resolve()), "--debug", "3", "--slice", "0",
            "--load-settings", ";".join(str(paths[k]) for k in ("machine", "process")),
            "--load-filaments", ";".join([str(paths["filament"])] * filament_slots),
            "--outputdir", str(work.resolve()), str(stl.resolve())
        ]
        with (target / "prebuilt-filuna-source-bundle.log").open(
                "w", encoding="utf-8", errors="replace") as log:
            log.write("MODEL_SCOPED_QA_MANIFEST_NOT_RELEASE\n")
            log.write("ARGV=" + json.dumps(argv) + "\n")
            log.flush()
            try:
                code = subprocess.run(
                    argv, cwd=exe.parent, stdout=log, stderr=subprocess.STDOUT,
                    timeout=180, check=False
                ).returncode
            except subprocess.TimeoutExpired:
                code = 124
        results = sorted(work.rglob("*.gcode"))
        result["returncode"] = code
        result["gcode_count"] = len(results)
        if code != 0 or len(results) != 1:
            result["status"] = "BLOCKED_PREBUILT_FILUNA_CLI"
        else:
            result["diagnostic"] = analyze_bytes(
                official.read_bytes(), results[0].read_bytes(), vendor
            )
            result["status"] = result["diagnostic"]["status"]
    except Exception as e:
        result["status"] = "BLOCKED_SOURCE_MANIFEST_PROBE"
        result["error"] = type(e).__name__ + ": " + str(e)
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    for name in ("base", "anycubic", "flashforge", "snapmaker", "stl", "out"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    exe = (args.base / "orca-slicer.exe").resolve()
    if not exe.is_file():
        raise ValueError("UNCHANGED_FILUNA_PORTABLE_NOT_FOUND")
    output = args.out.resolve()
    output.mkdir(parents=True, exist_ok=True)
    testcases = (
        ("anycubic", "Anycubic Kobra 3 V2 0.4 nozzle", args.anycubic),
        ("flashforge", "Flashforge AD5X 0.4 nozzle", args.flashforge),
        ("snapmaker", "Snapmaker U1 (0.4 nozzle)", args.snapmaker),
    )
    rows = [run_one(brand, machine, folder.resolve(), exe, args.stl,
                    output) for brand, machine, folder in testcases]
    (output / "verdicts.json").write_text(
        json.dumps(rows, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    for row in rows:
        print("FILUNA_MODEL_MANIFEST_PROBE", row["vendor"], row["status"], flush=True)
    # The diagnostic may PASS as a collection step, NEVER as OEM equivalence.
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
