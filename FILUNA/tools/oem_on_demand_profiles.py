#!/usr/bin/env python3
"""Data-only per-printer OEM catalog snapshot for future Filuna on-demand profiles.

A QA/reference implementation of *data packaging* after genuine model
discovery. This is NOT an installer, signed release, device network module,
automatic online download, or proof of native G-code parity. It never packs
all vendor presets as one app-install payload.

No manufacturer or machine names are hardcoded; caller selects exact discovered
machine identity. Only explicitly compatible presets are eligible; unimplemented
OEM compatibility expressions fail closed and are flagged for engine evaluation.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import shutil

def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _bytes(payload: object) -> bytes:
    return (json.dumps(payload,ensure_ascii=False,sort_keys=True,
                       separators=(",",":"))+"\n").encode("utf-8")


def _guard_name(name: str) -> None:
    if not isinstance(name,str) or not name or len(name)>256 or "\x00" in name:
        raise ValueError("INVALID_PRESET_NAME")


def prepare_model_bundle(catalog:dict,manufacturer:str,machine_name:str,
                         upstream_commit:str,destination:Path) -> dict:
    if not re.fullmatch(r"[a-z][a-z0-9_-]{1,31}",manufacturer):
        raise ValueError("INVALID_MANUFACTURER_ID")
    _guard_name(machine_name)
    if not re.fullmatch("[0-9a-fA-F]{40}",upstream_commit):
        raise ValueError("IMMUTABLE_OFFICIAL_COMMIT_REQUIRED")
    machines={m["name"]:m for m in catalog["machines"] if m["status"]=="RESOLVED"}
    if machine_name not in machines:
        raise ValueError("MODEL_NOT_IN_VERIFIED_ORIGINAL_CATALOG")
    if catalog["issues"]:
        raise ValueError("VENDOR_CATALOG_HAS_UNRESOLVED_INHERITANCE")
    matrix=catalog["profiles"]
    machine=matrix.get("machine:"+machine_name)
    if not machine or machine["status"]!="RESOLVED":
        raise ValueError("MACHINE_PROFILE_NOT_RESOLVED")
    config=machine["effective"]
    nozzles=config.get("nozzle_diameter")
    if not isinstance(nozzles,list) or not nozzles:
        raise ValueError("MACHINE_NOZZLE_UNSUPPORTED")
    if config.get("printer_model")!=machines[machine_name]["model_id"]:
        raise ValueError("MACHINE_MODEL_IDENTITY_MISMATCH")
    records=[("machine",machine_name,config)]
    blocked=[]
    for key,item in sorted(matrix.items()):
        kind=key.split(":",1)[0]
        if kind not in ("process","filament"):
            continue
        name=item["name"]
        if item["status"]!="RESOLVED":
            blocked.append({"type":kind,"name":name,"reason":"UNRESOLVED"})
            continue
        effective=item["effective"]
        cond=effective.get("compatible_printers_condition","")
        eligible=effective.get("compatible_printers",[])
        if not isinstance(eligible,list):
            blocked.append({"type":kind,"name":name,"reason":"INVALID_COMPATIBILITY_LIST"})
            continue
        if machine_name not in eligible:
            continue
        # The AST for OEM compatibility expressions is not interpreted by this
        # source-only QA data packer. Only the actual OEM runtime may certify
        # it; no "all compatible" claims on unknown predicates.
        if cond and (not isinstance(cond,str) or cond.strip()):
            blocked.append({"type":kind,"name":name,"reason":"OEM_COMPATIBILITY_PREDICATE_NOT_EVALUATED"})
            continue
        records.append((kind,name,effective))
    if not any(k=="process" for k,_,_ in records):
        raise ValueError("NO_EXPLICITLY_COMPATIBLE_PROCESS")
    if not any(k=="filament" for k,_,_ in records):
        raise ValueError("NO_EXPLICITLY_COMPATIBLE_FILAMENT")
    allowed={(k,n) for k,n,_ in records}
    default_process=config.get("default_print_profile")
    default_materials=config.get("default_filament_profile",[])
    if not isinstance(default_materials,list):
        raise ValueError("DEFAULT_FILAMENTS_NOT_LIST")
    if default_process and ("process",default_process) not in allowed:
        blocked.append({"type":"process","name":default_process,"reason":"DEFAULT_PROCESS_NOT_PROVEN_COMPATIBLE"})
    for m in default_materials:
        if ("filament",m) not in allowed:
            blocked.append({"type":"filament","name":m,"reason":"DEFAULT_FILAMENT_NOT_PROVEN_COMPATIBLE"})
    dest=destination.resolve()
    if dest.exists() and any(dest.iterdir()):
        raise ValueError("BUNDLE_DESTINATION_MUST_BE_EMPTY")
    dest.mkdir(parents=True,exist_ok=True)
    files=[]
    for kind,name,payload in records:
        if payload.get("type")!=kind or payload.get("name")!=name:
            raise ValueError("OEM_EFFECTIVE_PRESET_IDENTITY_MISMATCH")
        body=_bytes(payload)
        path=Path("data")/kind/(digest(name.encode("utf-8"))+".json")
        target=dest/path
        target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes(body)
        files.append({"kind":kind,"name":name,"path":path.as_posix(),
                      "sha256":digest(body),"size":len(body)})
    manifest={
        "schema":1,"purpose":"OEM_ON_DEMAND_PROFILES_QA_DATA_ONLY",
        "manufacturer":manufacturer,"machine":machine_name,
        "model_identity":config["printer_model"],
        "nozzle_diameter":nozzles,
        "original_vendor_git_commit":upstream_commit.lower(),
        "files":files,"blocked_compatibility":blocked,
        "profile_counts":{k:sum(f["kind"]==k for f in files)
                          for k in ("machine","process","filament")},
        "all_models_hardcoded":False,
        "shipping_status":"NOT_SIGNED_NOT_ONLINE_NOT_INTEGRATED",
        "native_filuna_engine_parity":"NOT_RUN",
        "native_device_discovery":"NOT_RUN",
        "physical_beta":"NOT_RUN",
        "profile_package_requires_versioned_signature_before_client_distribution":True
    }
    (dest/"manifest.json").write_bytes(_bytes(manifest))
    verify_model_bundle(dest)
    return manifest


def verify_model_bundle(destination:Path)->dict:
    root=destination.resolve()
    manifest_path=root/"manifest.json"
    if manifest_path.is_symlink():
        raise ValueError("UNTRUSTED_MANIFEST_SYMLINK")
    d=json.loads(manifest_path.read_text(encoding="utf-8"))
    if d.get("purpose")!="OEM_ON_DEMAND_PROFILES_QA_DATA_ONLY":
        raise ValueError("MANIFEST_IDENTITY_INVALID")
    if not isinstance(d.get("files"),list):
        raise ValueError("PROFILE_FILES_MISSING")
    seen=set()
    for record in d["files"]:
        name=record.get("path","")
        path=Path(name)
        if (path.is_absolute() or ".." in path.parts or path.suffix!=".json"
                or path.parts[0]!="data" or len(path.parts)!=3 or name in seen):
            raise ValueError("UNSAFE_PROFILE_PATH")
        seen.add(name)
        p=root/path
        if p.is_symlink() or not p.is_file() or not p.resolve().is_relative_to(root):
            raise ValueError("PROFILE_MISSING_OR_SYMLINK")
        body=p.read_bytes()
        if len(body)!=record.get("size") or digest(body)!=record.get("sha256"):
            raise ValueError("PROFILE_HASH_OR_SIZE_MISMATCH")
        loaded=json.loads(body)
        if loaded.get("name")!=record.get("name") or loaded.get("type")!=record.get("kind"):
            raise ValueError("BUNDLE_PROFILE_IDENTITY_MISMATCH")
    if not any(r["kind"]=="machine" and r["name"]==d["machine"] for r in d["files"]):
        raise ValueError("BUNDLE_MACHINE_MISSING")
    return d
