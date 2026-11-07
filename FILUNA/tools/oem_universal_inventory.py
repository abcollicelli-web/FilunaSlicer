#!/usr/bin/env python3
"""Universal OEM source + profile inventory: ALL models, profiles and protocols.

Run over exact, separately checked-out official vendor Git sources. Records all
source candidates and changed OEM deltas against Filuna's pinned Orca tree.
Never installs engines, enables access, prints or advertises homologation.

The complete source tree is inspected, not hard-coded model names or a few
selected network files. Other OEMs' profile branches are NOT imported.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re
import subprocess

BRAND_PROFILE_ROOTS = {
    "bambu": ("BBL",),
    "elegoo": ("Elegoo",),
    "creality": ("Creality",),
    "flashforge": ("Flashforge",),
    "qidi": ("Q Series", "X 3 Series", "X 4 Series", "X 5 Series", "QIDI"),
    "snapmaker": ("Snapmaker",),
    "anycubic": ("Anycubic",),
    "prusa": ("Prusa", "PrusaResearch"),
    "ultimaker": ("Ultimaker", "UltiMaker"),
    "raise3d": ("Raise3D", "Raise"),
}
SOURCE_ROOTS = ("src", "deps", "cmake", "resources/profiles", "resources/printers")
TYPES = {"machine", "printer", "process", "print", "filament"}
NETWORK_TOKENS = ("network", "lan", "socket", "websocket", "mqtt", "mdns", "ssdp", "udp",
                  "tcp", "http", "ftp", "printeragent", "print_host", "print_host",
                  "moonraker", "octoprint", "prusalink", "serialport", "bonjour",
                  "discovery", "discover", "protocol", "wireless", "connectivity")
ACCESS_TOKENS = ("auth", "token", "credential", "certificate", "pairing", "tls", "ssl",
                 "password", "accesscode", "access_code", "login")
TRANSFER_TOKENS = ("upload", "filetransfer", "file_transfer", "sendtoprinter", "printjob",
                   "print_job", "spooler", "transfer")
CONTROL_TOKENS = ("device", "printer", "machine", "camera", "status", "monitor",
                  "control", "temperature", "filamentmms", "multimaterial",
                  "materialsystem")
SLICING_TOKENS = ("slic", "gcode", "g_code", "purge", "flush", "planner",
                  "layer", "extrud", "printconfig", "print_config", "pathplanning")
LICENSE_TOKENS = ("license", "licence", "copying", "third_party", "copyright")
SOURCE_EXTS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hxx", ".ipp", ".inl",
               ".cmake", ".py", ".js", ".ts", ".java", ".cs", ".proto"}
RESOURCE_EXTS = {".json", ".ini", ".cfg", ".yaml", ".yml", ".xml", ".proto"}
SENSITIVE = ("secrets/", ".env", "private_key", "account_key", ".p12", ".pfx")
NORMALIZE_TYPE = {"printer": "machine", "print": "process"}


def sha256(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def git(root: Path, *args: str) -> bytes:
    return subprocess.run(["git", "-C", str(root), *args], stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, check=True, timeout=120).stdout


def read_tree(root: Path, revision: str) -> dict[str, tuple[str, str, str]]:
    raw = git(root, "ls-tree", "-r", "-z", revision, *SOURCE_ROOTS)
    result = {}
    for line in raw.split(b"\0"):
        if not line:
            continue
        metadata, raw_path = line.split(b"\t", 1)
        mode, kind, blob = metadata.decode("ascii").split()
        path = raw_path.decode("utf-8", errors="surrogateescape")
        if Path(path).is_absolute() or ".." in Path(path).parts:
            raise ValueError("DANGEROUS_VENDOR_PATH")
        result[path] = (mode, kind, blob)
    return result


def own_profile(path: str, brand: str) -> bool:
    prefix = "resources/profiles/"
    if not path.startswith(prefix):
        return False
    rest = path[len(prefix):]
    head = rest.split("/", 1)[0]
    aliases = BRAND_PROFILE_ROOTS.get(brand, (brand,))
    return any(head.casefold() == root.casefold() or
               head.casefold() == (root+".json").casefold()
               for root in aliases)


def classify(path: str, brand: str) -> tuple[str, str]:
    """Classify conservatively; unknown sources go to REVIEW, never disappear."""
    low = path.casefold()
    name = Path(path).name.casefold()
    if path.startswith("resources/profiles/"):
        return ("ALL_BRAND_PROFILES", "brand_only") if own_profile(path, brand) \
            else ("OTHER_BRAND_PROFILES", "not_imported")
    if path.startswith("resources/printers/"):
        return "HARDWARE_AND_MODEL_MATRIX", "all_candidate"
    if any(token in low for token in SENSITIVE):
        return "REVIEW_SECURITY", "blocked_from_staging"
    if any(token in name for token in LICENSE_TOKENS):
        return "LICENSE_NOTICE", "review_required"
    if path.startswith(("deps/", "cmake/")):
        return "REVIEW_TRANSITIVE_DEPENDENCY", "dependency_analysis_required"
    if not path.startswith("src/"):
        return "REVIEW_UNCLASSIFIED", "review_required"
    if path.startswith("src/libslic3r/"):
        return "SLICING_ENGINE", "all_diff_candidate"
    if any(token in low for token in ACCESS_TOKENS):
        return "ACCESS_AND_AUTH", "all_diff_candidate"
    if any(token in low for token in TRANSFER_TOKENS):
        return "FILE_TRANSFER_AND_SEND", "all_diff_candidate"
    if any(token in low for token in NETWORK_TOKENS):
        return "NETWORK_AND_DISCOVERY", "all_diff_candidate"
    if any(token in low for token in CONTROL_TOKENS):
        return "PRINTER_CONTROL_AND_MONITORING", "all_diff_candidate"
    if any(token in low for token in SLICING_TOKENS):
        return "SLICING_AND_GCODE", "all_diff_candidate"
    if low.endswith(tuple(SOURCE_EXTS)):
        return "REVIEW_TRANSITIVE_SOURCE", "dependency_analysis_required"
    return "REVIEW_UNCLASSIFIED", "review_required"


def git_blob(root: Path, blob: str) -> bytes:
    data = git(root, "cat-file", "blob", blob)
    actual = hashlib.sha1(b"blob "+str(len(data)).encode("ascii")+b"\0"+data).hexdigest()
    if actual != blob:
        raise ValueError("OFFICIAL_GIT_BLOB_HASH_MISMATCH")
    return data


def survey_source(official: Path, base: Path, brand: str, out: Path,
                  copy: bool = False) -> dict:
    old, new = read_tree(base, "HEAD"), read_tree(official, "HEAD")
    rows = []
    counters = Counter()
    skipped = Counter()
    for path, (mode, kind, blob) in sorted(new.items()):
        category, policy = classify(path, brand)
        if category == "OTHER_BRAND_PROFILES":
            skipped["other_brand_profiles"] += 1
            continue
        baseline = old.get(path)
        status = "SHARED_WITH_ORCA" if baseline == (mode, kind, blob) \
            else ("MODIFIED_FROM_ORCA" if baseline else "NEW_OEM_SOURCE")
        record = {"path": path, "category": category, "policy": policy,
                  "origin_git_blob": blob, "base_blob": baseline[2] if baseline else None,
                  "status": status, "mode": mode,
                  "copy_status": "NOT_REQUESTED", "sha256": None}
        counters[category+":"+status] += 1
        if mode not in {"100644", "100755"} or kind != "blob":
            record["copy_status"] = "BLOCKED_SPECIAL_GIT_OBJECT"
        elif status == "SHARED_WITH_ORCA":
            record["copy_status"] = "SHARED_NOT_DUPLICATED"
        elif copy and policy != "blocked_from_staging":
            data = git_blob(official, blob)
            target = out / "source" / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            target.chmod(0o755 if mode == "100755" else 0o644)
            record["sha256"] = sha256(data)
            record["copy_status"] = "STAGED_FOR_AUDIT_NOT_INTEGRATED"
        elif policy == "blocked_from_staging":
            record["copy_status"] = "REVIEW_SECURITY"
        else:
            record["copy_status"] = "NOT_COPIED"
        rows.append(record)
    return {
        "brand": brand, "base_orca_commit": git(base, "rev-parse", "HEAD").decode().strip(),
        "official_vendor_commit": git(official, "rev-parse", "HEAD").decode().strip(),
        "all_models_not_hardcoded": True,
        "status": "SOURCE_INVENTORY_ONLY_NOT_INTEGRATED_NOT_HOMOLOGATED",
        "scope": list(SOURCE_ROOTS), "totals": dict(sorted(counters.items())),
        "other_brand_profiles_excluded": skipped["other_brand_profiles"],
        "files": rows,
        "review_blockers": [x["path"] for x in rows
                            if x["status"] != "SHARED_WITH_ORCA"
                            and (x["category"].startswith("REVIEW")
                                 or x["copy_status"].startswith("BLOCKED"))],
        "limitations": [
            "Git paths and blobs are inventoried, but transitive CMake/include dependency closure is not certified",
            "LICENSE_NOTICE is not redistributability approval; require file and dependency license audit",
            "A staged file is not linked, compiled, loaded, executed, or homologated",
            "No proprietary binaries, vendor cloud/private secrets or device commands are permitted",
            "Capabilities, network and device access must be proven separately through Filuna contract",
        ],
    }


def normalize_type(value: str) -> str:
    return NORMALIZE_TYPE.get(value, value)


def read_catalog(root: Path) -> dict:
    """Discover every OEM printer/process/filament JSON, no model selection."""
    root = root.resolve()
    if not root.is_dir():
        raise ValueError("VENDOR_PROFILES_DIR_MISSING")
    profiles = {}
    sources = {}
    extras = []
    invalid = []
    for path in sorted(root.rglob("*.json")):
        if path.is_symlink() or not path.is_file():
            invalid.append({"path": str(path.relative_to(root)), "issue": "UNSAFE_FILE"})
            continue
        try:
            value = json.loads(path.read_text(encoding="utf-8-sig"))
        except (UnicodeError, json.JSONDecodeError) as e:
            invalid.append({"path": str(path.relative_to(root)),
                            "issue": "INVALID_JSON", "message": str(e)[:200]})
            continue
        kind = normalize_type(str(value.get("type", ""))) if isinstance(value, dict) else ""
        name = value.get("name") if isinstance(value, dict) else None
        if kind not in {"machine", "process", "filament"} or not isinstance(name, str) or not name:
            extras.append(str(path.relative_to(root)))
            continue
        key = (kind, name)
        if key in profiles:
            invalid.append({"path": str(path.relative_to(root)), "issue": "DUPLICATE_PRESET",
                            "key": [kind, name], "previous": sources[key]})
            continue
        profiles[key] = value
        sources[key] = str(path.relative_to(root))
    index = {}
    pending = []
    resolved = {}
    def resolve(key, stack=()):
        if key in resolved:
            return resolved[key]
        if key in stack:
            raise ValueError("INHERITANCE_CYCLE " + repr(stack+(key,)))
        if key not in profiles:
            raise ValueError("MISSING_PROFILE_PARENT "+repr(key))
        value = profiles[key]
        parent_spec = value.get("inherits", "")
        if isinstance(parent_spec, str):
            parent_names = [s.strip() for s in parent_spec.split(";") if s.strip()]
        elif isinstance(parent_spec, list) and all(isinstance(x, str) for x in parent_spec):
            parent_names = [s.strip() for s in parent_spec if s.strip()]
        else:
            raise ValueError("INVALID_INHERITANCE_TYPE "+repr(key))
        merged = {}
        for parent in parent_names:
            merged.update(resolve((key[0], parent), stack+(key,)))
        merged.update(value)
        resolved[key] = merged
        return merged

    machines = []
    kinds = Counter()
    for key in sorted(profiles):
        kind, name = key
        kinds[kind] += 1
        item = {"kind": kind, "name": name, "file": sources[key],
                "source_sha256": sha256(json.dumps(profiles[key],ensure_ascii=False,
                           sort_keys=True,separators=(",",":")).encode("utf-8")),
                "parents": profiles[key].get("inherits", ""),
                "status": "UNRESOLVED"}
        try:
            effective = resolve(key)
            item["status"] = "RESOLVED"
            item["effective"] = effective
        except ValueError as err:
            item["error"] = str(err)
            pending.append({"kind": kind, "name": name, "error": str(err)})
        index[kind+":"+name] = item
        if kind == "machine":
            model = item.get("effective", {}).get("printer_model")
            machine = {
                "name": name, "source": sources[key], "model_id": model,
                "printer_variant": item.get("effective", {}).get("printer_variant"),
                "nozzle_diameter": item.get("effective", {}).get("nozzle_diameter"),
                "status": item["status"],
                "model_identity_status": "IDENTIFIED" if model else "NOT_DECLARED",
            }
            machines.append(machine)
    return {"status": "SOURCE_CATALOG_NOT_FILUNA_INTEGRATED",
            "all_models_discovered": True, "files_total":len(profiles)+len(extras)+len(invalid),
            "profiles_per_type":dict(kinds), "machines":machines, "profiles":index,
            "non_profile_json":extras, "issues":pending+invalid,
            "capability_proof":"NOT_RUN",
            "compatibility_note":"compatible_printers_condition and dynamic expressions are NOT evaluated. "
                "Presence of profiles never implies compatibility; the OEM predicate engine must approve it."}


def main():
    p=argparse.ArgumentParser()
    p.add_argument("--official", type=Path, required=True, help="pinned vendor Git checkout")
    p.add_argument("--base", type=Path, required=True, help="pinned Filuna Orca Git checkout")
    p.add_argument("--brand", type=str, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--copy", action="store_true", help="stage original changed Git blobs into isolated folder")
    p.add_argument("--profiles", type=Path, help="vendor original profiles folder; all models")
    args=p.parse_args()
    if not re.fullmatch("[a-z][a-z0-9_-]{1,31}",args.brand):
        p.error("INVALID_BRAND")
    official,base,out=args.official.resolve(),args.base.resolve(),args.out.resolve()
    if out==official or out==base or official in out.parents or base in out.parents:
        p.error("OUTPUT_MUST_BE_OUTSIDE_REPOSITORIES")
    out.mkdir(parents=True,exist_ok=True)
    source=survey_source(official,base,args.brand,out,args.copy)
    (out/"source-manifest.json").write_text(json.dumps(source,indent=2,ensure_ascii=False)+"\n",
                                            encoding="utf-8")
    summary={"source_inventory":"PASS", "source_review_blockers":len(source["review_blockers"]),
             "files_total":len(source["files"]), "brand":args.brand,
             "source_staged":sum(x["copy_status"]=="STAGED_FOR_AUDIT_NOT_INTEGRATED"
                                 for x in source["files"]),
             "models":"NOT_AUDITED" if not args.profiles else "AUDITED"}
    if args.profiles:
        catalog=read_catalog(args.profiles)
        (out/"all-models-all-profiles.json").write_text(
            json.dumps(catalog,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
        summary.update({"models_discovered":len(catalog["machines"]),
                        "profiles_per_type":catalog["profiles_per_type"],
                        "profile_issues":len(catalog["issues"])})
    (out/"SUMMARY.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False)+"\n")
    print(json.dumps(summary,ensure_ascii=False))
    # REVIEW is blocking for promotional approval, not for creating an inventory.
    return 0


if __name__=="__main__":
    raise SystemExit(main())
