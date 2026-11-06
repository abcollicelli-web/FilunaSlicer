#!/usr/bin/env python3
"""Stage byte-exact OEM source deltas without mutating Filuna or the vendor checkout.

The source is a pinned, audited Git checkout. Only changed/additional files
are copied; identical blobs are shared by the Orca base. This creates SOURCE
OVERLAYS, not compiled engines, plugins, licenses to third-party binaries, or
evidence of an installed/working OEM module.

Example:
 python3 FILUNA/tools/stage_oem_source_deltas.py \
   --filuna-repo . --filuna-ref HEAD \
   --official-repo /path/BambuStudio-tag --brand bambu \
   --out /tmp/filuna-oem-source-bambu --copy

Never copy these over the active Orca tree. Compile each OEM engine with
its own isolated symbols/process; two incompatible Slic3r namespaces cannot
be linked into one executable simultaneously.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
import shutil
import stat
import subprocess
from pathlib import Path

SCOPES = ("src", "deps", "resources/profiles", "resources/printers")
def git(repo: Path, *args: str) -> bytes:
    p=subprocess.run(["git","-C",str(repo),*args],capture_output=True,check=True)
    return p.stdout

def tree(repo: Path, ref: str):
    raw=git(repo,"ls-tree","-r","-z",ref,*SCOPES)
    result={}
    for record in raw.split(b"\0"):
        if not record:continue
        meta,raw_path=record.split(b"\t",1)
        mode,kind,blob=meta.decode().split()
        result[raw_path.decode("utf-8","surrogateescape")]=(mode,kind,blob)
    return result

def git_blob_sha(raw: bytes) -> str:
    return hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest()

def classify(path):
    if path.startswith("src/libslic3r/"):return "engine"
    if path.startswith("src/slic3r/GUI/"):return "prepare_device_gui"
    if path.startswith("resources/profiles/"):return "profiles"
    if path.startswith("resources/printers/"):return "hardware_matrix"
    if path.startswith("deps/"):return "dependencies"
    if path.startswith("src/"):return "other_source"
    return "other"

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--filuna-repo",type=Path,required=True)
    ap.add_argument("--filuna-ref",required=True)
    ap.add_argument("--official-repo",type=Path,required=True)
    ap.add_argument("--brand",required=True)
    ap.add_argument("--out",type=Path,required=True)
    ap.add_argument("--copy",action="store_true")
    args=ap.parse_args()
    if not args.brand.isascii() or not args.brand.replace("-","").isalnum():
        raise SystemExit("invalid brand")
    filuna=args.filuna_repo.resolve()
    official=args.official_repo.resolve()
    out=args.out.resolve()
    if out==filuna or out==official or filuna in out.parents or official in out.parents:
        raise SystemExit("output must be independent from both Git checkouts")
    old=tree(filuna,args.filuna_ref)
    new=tree(official,"HEAD")
    manifest={"brand":args.brand,"filuna_commit":git(filuna,"rev-parse",args.filuna_ref).decode().strip(),
              "official_commit":git(official,"rev-parse","HEAD").decode().strip(),
              "status":"STAGED_SOURCE_NOT_COMPILED_NOT_HOMOLOGATED",
              "same_orca_engine_process":False,"scope":list(SCOPES),
              "files":[],"counts":{"identical":0,"modified":0,"added":0,"unsupported_mode":0},
              "source_bytes":0,"identical_bytes_not_copied":0,
              "limitations":["Does not discover proprietary OEM network libraries",
                "Does not verify redistributability of third-party deps/binaries",
                "No runtime loader or OEM engine compiled",
                "No G-code equality evidenced by these snapshots"]}
    if args.copy:(out/"source").mkdir(parents=True,exist_ok=True)
    for path,(mode,kind,blob) in sorted(new.items()):
        prev=old.get(path)
        is_same=prev==(mode,kind,blob)
        if kind!="blob" or mode not in ("100644","100755"):
            manifest["counts"]["unsupported_mode"]+=1
            continue
        if is_same:
            manifest["counts"]["identical"]+=1
            continue
        source=official/path
        # Do not follow symlinks or special files even if the Git metadata is
        # misleading. All copied content must match its pinned Git blob.
        if not source.is_file() or source.is_symlink():
            raise RuntimeError(f"Not a regular file: {path}")
        data=source.read_bytes()
        if git_blob_sha(data)!=blob:raise RuntimeError(f"Checksum mismatch: {path}")
        state="modified" if prev else "added"
        manifest["counts"][state]+=1
        manifest["source_bytes"]+=len(data)
        sha256=hashlib.sha256(data).hexdigest()
        manifest["files"].append({"path":path,"category":classify(path),"status":state,
                                  "git_blob":blob,"sha256":sha256,"bytes":len(data),
                                  "filuna_blob":prev[2] if prev else None,"mode":mode})
        if args.copy:
            target=out/"source"/path
            target.parent.mkdir(parents=True,exist_ok=True)
            target.write_bytes(data)
            target.chmod(0o755 if mode=="100755" else 0o644)
    out.mkdir(parents=True,exist_ok=True)
    (out/"manifest.json").write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print(json.dumps({"brand":args.brand,"official_commit":manifest["official_commit"],
           "filuna_commit":manifest["filuna_commit"],"counts":manifest["counts"],
           "source_bytes":manifest["source_bytes"],"copy":args.copy,
           "manifest":str(out/"manifest.json")},ensure_ascii=False))
if __name__=="__main__":
    main()
