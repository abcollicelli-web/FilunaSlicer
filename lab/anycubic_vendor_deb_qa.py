#!/usr/bin/env python3
"""Anycubic Slicer Next original vendor Debian-package provenance + model inventory.

This is NOT an official portable ZIP, nor a Filuna integration, nor a model
homologation. Extract vendor package in isolated CI; no install or build.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import urllib.request

APT_ROOT = "https://cdn-universe-slicer.anycubic.com/prod/"
PACKAGES = APT_ROOT + "dists/noble/main/binary-amd64/Packages"
VERSION = "1:2.0.0.6"
APP_VERSION = "2.0.0.6"
FILENAME = "pool/main/a/anycubicslicernext/AnycubicSlicerNext_linux-v2.0.0.6-20261030105113.deb"
SHA256 = "fdb134a4e11fad7e795c8d425520ce1305321ccc0bf1375a651a1513f8854678"
BYTES = 153586194


def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as fp:
        for block in iter(lambda: fp.read(1024*1024), b""):
            h.update(block)
    return h.hexdigest()


def apt_records(source):
    """Fail closed when indexes have duplicate field names or invalid records."""
    records = []
    for para in re.split(r"\r?\n\r?\n+", source.strip()):
        fields = {}
        for line in para.splitlines():
            if line.startswith((" ", "\t")):
                continue
            if ":" not in line:
                continue
            key, value = line.split(":", 1)
            if key in fields:
                raise ValueError("DUPLICATED_APT_FIELD " + key)
            fields[key] = value.strip()
        if fields:
            records.append(fields)
    return records


def find_exact_package(records):
    rows = [r for r in records if r.get("Package", "").lower() == "anycubicslicernext"]
    if len(rows) != 1:
        raise ValueError("EXPECTED_ONE_VENDOR_PACKAGE_FOUND_" + str(len(rows)))
    item = rows[0]
    expected = {"Version": VERSION, "SHA256": SHA256,
                "Filename": FILENAME, "Size": str(BYTES)}
    for key, value in expected.items():
        if item.get(key) != value:
            raise ValueError("VENDOR_APT_METADATA_MISMATCH_" + key
                             + "_expected=" + value + "_actual=" + str(item.get(key)))
    return item


def download(url, path, max_bytes):
    request = urllib.request.Request(url, headers={"User-Agent": "Filuna-OEM-Provenance-QA/1"})
    temp = path.with_name(path.name+".partial")
    temp.unlink(missing_ok=True)
    try:
        with urllib.request.urlopen(request, timeout=100) as response, temp.open("wb") as out:
            amount=0
            for block in iter(lambda: response.read(1024*1024), b""):
                amount+=len(block)
                if amount > max_bytes:
                    raise ValueError("DOWNLOAD_SIZE_LIMIT")
                out.write(block)
        temp.replace(path)
    finally:
        temp.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cache", type=Path, default=Path(".anycubic-vendor-cache"))
    parser.add_argument("--out", type=Path, default=Path("qa-anycubic-original-deb"))
    args=parser.parse_args()
    cache=args.cache.resolve(); out=args.out.resolve()
    cache.mkdir(parents=True, exist_ok=True);out.mkdir(parents=True, exist_ok=True)
    rows=[]
    def add(stage,status,detail,evidence=""):
        assert status in ("PASS","FAIL","PENDING")
        rows.append({"stage":stage,"status":status,"detail":detail,"evidence":evidence})
        print("ANYCUBIC",stage,status,detail,flush=True)
    try:
        request=urllib.request.Request(PACKAGES,headers={"User-Agent":"Filuna-OEM-QA/1"})
        with urllib.request.urlopen(request, timeout=60) as response:
            index=response.read(5*1024*1024+1)
        if len(index)>5*1024*1024:
            raise ValueError("APT_INDEX_SIZE_LIMIT")
        records=apt_records(index.decode("utf-8"))
        current=[r for r in records if r.get("Package","").lower()=="anycubicslicernext"]
        print("ANYCUBIC_APT_CANDIDATE="+json.dumps([{
            key:r.get(key) for key in ("Package","Version","Filename","SHA256","Size")}
            for r in current]),flush=True)
        package=find_exact_package(records)
        add("VENDOR_APT_INDEX","PASS","official vendor Packages pins expected version/file/SHA/size",PACKAGES)
        pkg=cache/Path(FILENAME).name
        if not pkg.exists() or pkg.stat().st_size!=BYTES or sha(pkg)!=SHA256:
            pkg.unlink(missing_ok=True)
            download(APT_ROOT+FILENAME,pkg,BYTES+1)
        if pkg.stat().st_size!=BYTES or sha(pkg)!=SHA256:
            raise ValueError("VENDOR_DEB_SHA256_OR_SIZE_MISMATCH")
        add("OFFICIAL_DEB_SHA256","PASS",SHA256,APT_ROOT+FILENAME)
        (out/"official-package.lock.json").write_text(json.dumps({
            "source_packages":PACKAGES,"source_deb":APT_ROOT+FILENAME,
            "package_name":package["Package"],"package_version":VERSION,
            "application_version_expected":APP_VERSION,"bytes":BYTES,
            "sha256":SHA256,"package_kind":"original vendor Debian .deb, NOT portable",
            "no_installer_or_compiler_invoked":True
        },indent=2)+"\n",encoding="utf-8")
        tree=cache/"extracted"
        tree.mkdir(parents=True,exist_ok=True)
        log=out/"dpkg-extract.log"
        with log.open("w",encoding="utf-8") as fp:
            code=subprocess.run(["dpkg-deb","-x",str(pkg),str(tree)],stdout=fp,
                                stderr=subprocess.STDOUT,timeout=150,check=False).returncode
        if code:
            raise ValueError("OFFICIAL_DEB_EXTRACT_FAILED_"+str(code))
        add("ORIGINAL_PACKAGE_EXTRACTION","PASS","dpkg-deb -x; no install/build","dpkg-extract.log")
        versions=list(tree.rglob("build-version.txt"))
        versions=[p for p in versions if "resources" in p.parts]
        if not versions or APP_VERSION not in [v.read_text(encoding="utf-8",errors="replace").strip() for v in versions]:
            raise ValueError("OFFICIAL_APP_VERSION_IN_RESOURCES_NOT_CONFIRMED")
        add("OFFICIAL_APPLICATION_VERSION","PASS",APP_VERSION)
        exes=[p for p in tree.rglob("*") if p.is_file() and p.name in
              ("AnycubicSlicerNext","anycubic-slicer-next","AnycubicSlicer")]
        resources=[p for p in tree.rglob("*") if p.is_dir() and p.name=="profiles"]
        profile_files=[]
        matches={"Kobra S1":[],"Kobra 3 V2":[]}
        for folder in resources:
            for p in folder.rglob("*.json"):
                if not p.is_file() or p.stat().st_size>700000:
                    continue
                profile_files.append(p)
                data=p.read_text(encoding="utf-8-sig",errors="replace")
                for model in matches:
                    if model.casefold() in p.name.casefold() or model.casefold() in data.casefold():
                        matches[model].append(str(p.relative_to(tree)))
        (out/"resources-inventory.json").write_text(json.dumps({
            "version_files":[str(p.relative_to(tree)) for p in versions],
            "executable_files":[str(p.relative_to(tree)) for p in exes],
            "profiles_count":len(profile_files),
            "model_text_mentions":matches,
            "warning":"text mentions and file presence do NOT prove correct model preset/CLI",
        },indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
        if not exes or not resources:
            add("EXE_AND_PROFILES","FAIL","MISSING_EXECUTABLE_OR_PROFILES","resources-inventory.json")
        else:
            add("EXE_AND_PROFILES","PASS","exe="+str(len(exes))+" profiles="+str(len(profile_files)),
                "resources-inventory.json")
        for model in matches:
            add("MODEL_PROFILE_CANDIDATE_"+model.upper().replace(" ","_"),
                "PASS" if matches[model] else "PENDING",
                "text mentions="+str(len(matches[model]))+" (candidate only)",
                "resources-inventory.json")
        add("OFFICIAL_A_B","PENDING","No exact effective OEM model+process+filament CLI G-code A/B proof")
        add("FILUNA_PARITY","PENDING","Filuna application has not produced separate matched G-code")
        add("NETWORK","PENDING","No device bridge simulator evidence")
        add("PHYSICAL_BETA","PENDING","Only after all internal QA and final Filuna compilation")
    except Exception as exc:
        add("RUNNER","FAIL",type(exc).__name__+": "+str(exc))
    (out/"results.json").write_text(json.dumps(rows,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    (out/"RESUMO.md").write_text("# Anycubic Slicer Next — Original .deb internal provenance QA\n\n"
        "NO official portable ZIP, no install/build, no Filuna parity, no physical test.\n\n"
        "| Gate | State | Details |\n|---|---|---|\n"
        + "".join("| "+r["stage"]+" | "+r["status"]+" | "+r["detail"].replace("|","/")+" |\n"
                  for r in rows),encoding="utf-8")
    return 1 if any(r["status"]=="FAIL" for r in rows) else 0


if __name__=="__main__":
    raise SystemExit(main())
