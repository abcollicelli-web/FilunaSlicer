#!/usr/bin/env python3
"""FlashForge 1.7.9 official Linux AppImage, immutable portable-first diagnostics.

Official upstream asset and SHA are published by FlashForge/Orca-Flashforge:
GitHub release 1.7.9, AppImage sha256:96578656466416adc1ccd2e7ec656a0b217560edadeeb29642259e67922adf45.
It is an *older* official Linux release useful for standalone QA; not the
vendor's current Windows 1.7.20, nor proof of Filuna parity.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import urllib.request

VERSION="1.7.9"
UPSTREAM_COMMIT="70f2359ba8b4bc8ff9e54818a4bb1d05e56e7856"
ASSET="Flash.Studio_ubuntu24.04_V1.7.9.AppImage"
URL="https://github.com/FlashForge/Orca-Flashforge/releases/download/1.7.9/"+ASSET
SHA256="96578656466416adc1ccd2e7ec656a0b217560edadeeb29642259e67922adf45"
BYTES=153561592
ROOT=Path(__file__).resolve().parents[1]


def digest(path):
    h=hashlib.sha256()
    with path.open("rb") as fp:
        for buf in iter(lambda:fp.read(1024*1024),b""):
            h.update(buf)
    return h.hexdigest()


def obtain(cache):
    cache.mkdir(parents=True,exist_ok=True)
    archive=cache/ASSET
    if not archive.is_file() or archive.stat().st_size!=BYTES or digest(archive)!=SHA256:
        archive.unlink(missing_ok=True)
        temp=archive.with_name(archive.name+".partial")
        temp.unlink(missing_ok=True)
        try:
            req=urllib.request.Request(URL,headers={"User-Agent":"Filuna-OEM-Portable-QA/1"})
            with urllib.request.urlopen(req,timeout=100) as response,temp.open("wb") as out:
                size=0
                for block in iter(lambda:response.read(1024*1024),b""):
                    size+=len(block)
                    if size>BYTES+1:
                        raise ValueError("OFFICIAL_APPIMAGE_DOWNLOAD_TOO_LARGE")
                    out.write(block)
            if temp.stat().st_size!=BYTES or digest(temp)!=SHA256:
                raise ValueError("OFFICIAL_APPIMAGE_SHA_OR_SIZE_MISMATCH")
            temp.replace(archive)
        finally:
            temp.unlink(missing_ok=True)
    archive.chmod(archive.stat().st_mode|0o111)
    return archive


def launch(argv, logfile, cwd, timeout=120):
    with logfile.open("w",encoding="utf-8") as f:
        f.write("COMMAND_JSON="+json.dumps(argv)+"\n")
        f.flush()
        try:
            return subprocess.run(argv,cwd=cwd,stdout=f,stderr=subprocess.STDOUT,
                                  timeout=timeout,check=False).returncode
        except subprocess.TimeoutExpired:
            f.write("\nQA_TIMEOUT\n")
            return 124
        except OSError as exc:
            f.write("\nQA_OSERROR="+repr(exc)+"\n")
            return 127


def discover_profiles(root):
    found=[]
    for folder in root.rglob("Flashforge"):
        if not folder.is_dir() or folder.parent.name!="profiles":
            continue
        if all((folder/x).is_dir() for x in ("machine","process","filament")):
            found.append(folder)
    return sorted(found,key=lambda d:len(str(d)))


def main():
    p=argparse.ArgumentParser()
    p.add_argument("--cache",type=Path,default=Path(".flashforge-appimage-cache"))
    p.add_argument("--out",type=Path,default=Path("qa-flashforge-appimage-179"))
    a=p.parse_args()
    cache=a.cache.resolve();out=a.out.resolve()
    cache.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
    rows=[]
    def add(stage,status,details,evidence=""):
        assert status in ("PASS","FAIL","PENDING")
        rows.append({"stage":stage,"status":status,"details":details,"evidence":evidence})
        print("FLASHFORGE_ORIGINAL",stage,status,details,flush=True)
    try:
        archive=obtain(cache)
        add("OFFICIAL_APPIMAGE_SHA256","PASS",SHA256+" bytes="+str(archive.stat().st_size),URL)
        app=cache/"squashfs-root"/"AppRun"
        if not app.is_file():
            code=launch([str(archive),"--appimage-extract"],out/"extract.log",cache,180)
            if code!=0:
                raise ValueError("ORIGINAL_APPIMAGE_EXTRACT_FAILED_"+str(code))
        if not app.is_file():
            raise ValueError("OFFICIAL_APPRUN_MISSING_AFTER_EXTRACTION")
        app.chmod(app.stat().st_mode|0o111)
        add("OFFICIAL_ORIGINAL_EXTRACTION","PASS","AppRun found; no installer or compiler")
        profiles=discover_profiles(cache/"squashfs-root")
        artifacts={"source_url":URL,"source_sha256":SHA256,"source_bytes":BYTES,
                   "tag":"1.7.9","source_commit":UPSTREAM_COMMIT,
                   "profile_roots":[str(x.relative_to(cache/"squashfs-root")) for x in profiles],
                   "source_kind":"original published Linux AppImage (not Windows 1.7.20)"}
        if not profiles:
            add("VENDOR_PROFILE_TREE","FAIL","No complete Flashforge machine/process/filament directory")
        else:
            chosen=profiles[0]
            counts={typ:len(list((chosen/typ).glob("*.json")))
                    for typ in ("machine","process","filament")}
            artifacts["profile_counts"]=counts
            aliases={}
            for kind in ("machine","process","filament"):
                aliases[kind]=[p.name for p in sorted((chosen/kind).glob("*.json"))
                               if ("AD5X" in p.name or "Adventurer 5M" in p.name)][:50]
            artifacts["ad5x_ad5m_candidates"]=aliases
            add("VENDOR_PROFILE_TREE","PASS",
                "all original model candidates scanned "+json.dumps(counts),
                "inventory.json")
        (out/"inventory.json").write_text(json.dumps(artifacts,indent=2,ensure_ascii=False)+"\n")
        binary=cache/"squashfs-root"/"bin"
        bfiles=[x for x in binary.glob("*") if x.is_file()] if binary.is_dir() else []
        (out/"binaries-inventory.json").write_text(json.dumps({
             "bin_files":[x.name for x in bfiles],
             "AppRun_type":"symlink" if app.is_symlink() else "regular",
             "AppRun_target":os.readlink(app) if app.is_symlink() else None
        },indent=2)+"\n")
        run=launch(["xvfb-run","-a","-s","-screen 0 1600x900x24",
                    str(app),"--help"],out/"cli-help.log",
                   cache/"squashfs-root",35)
        add("PORTABLE_CLI_HELP",
            "PASS" if run==0 else "PENDING",
            "exit="+str(run)+"; view cli-help.log (help != slicing proof)",
            "cli-help.log")
        add("OFFICIAL_GCODE_A_B","PENDING",
            "exact OEM model/process/material and 2 original CLI G-codes not proven")
        add("FILUNA_PARITY","PENDING",
            "no separate Filuna engine output or native module link")
        add("NETWORK","PENDING",
            "no Flashforge LAN/IFS native integration, ACK, or device test")
        add("HARDWARE_BETA","PENDING","hardware testing later with supervised beta")
    except Exception as exc:
        add("RUNNER","FAIL",type(exc).__name__+": "+str(exc))
    (out/"results.json").write_text(json.dumps(rows,indent=2,ensure_ascii=False)+"\n")
    (out/"RESUMO.md").write_text("# Flashforge original Linux AppImage 1.7.9 pilot QA\n\n"
        "Source official release (old version); not current 1.7.20 Windows, "
        "nor native Filuna/hardware homologation.\n\n"
        "| Gate | Status | Detail |\n|---|---|---|\n"
        + "".join("| "+r["stage"]+" | "+r["status"]+" | "+
                  r["details"].replace("|","/")+" |\n" for r in rows),encoding="utf-8")
    return 1 if any(x["status"]=="FAIL" for x in rows) else 0


if __name__=="__main__":
    raise SystemExit(main())
