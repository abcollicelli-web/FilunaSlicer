#!/usr/bin/env python3
"""Flashforge 1.7.15 source-pinned installer unpack probe — no install / no build.

This is an extraction experiment, NOT a manufacturer-published portable build.
Do not claim CLI, parity or hardware/network PASS from extraction alone.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import shutil
import urllib.request

VERSION="1.7.15"
ASSET="Flash_Studio_Desktop_1.7.15_x64.exe"
URL="https://github.com/FlashForge/Orca-Flashforge/releases/download/1.7.15/"+ASSET
SHA256="b3cc8804e0426e3f4bb2c0953667e27512c0047af51099c0e1af5c5d1b3ef8f3"
MAX_BYTES=400*1024*1024

def digest(path):
 h=hashlib.sha256()
 with open(path,"rb") as f:
  for chunk in iter(lambda:f.read(1024*1024),b""):h.update(chunk)
 return h.hexdigest()

def main():
 p=argparse.ArgumentParser()
 p.add_argument("--cache",type=Path,default=Path(".flashforge-official-cache"))
 p.add_argument("--out",type=Path,default=Path("qa-flashforge-extract"))
 a=p.parse_args();cache=a.cache.resolve();out=a.out.resolve()
 cache.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
 rows=[]
 def add(stage,status,detail):
  assert status in ("PASS","FAIL","PENDING")
  rows.append({"brand":"flashforge","version":VERSION,"stage":stage,"status":status,"detail":detail})
  print(stage,status,detail,flush=True)
 try:
  pkg=cache/ASSET
  if not pkg.is_file() or digest(pkg)!=SHA256:
   pkg.unlink(missing_ok=True)
   partial=cache/(ASSET+".partial");partial.unlink(missing_ok=True)
   try:
    req=urllib.request.Request(URL,headers={"User-Agent":"Filuna-OEM-QA/1"})
    with urllib.request.urlopen(req,timeout=90) as src,partial.open("wb") as dest:
     size=0
     for block in iter(lambda:src.read(1024*1024),b""):
      size+=len(block)
      if size>MAX_BYTES:raise RuntimeError("ARCHIVE_TOO_LARGE")
      dest.write(block)
    if digest(partial)!=SHA256:raise RuntimeError("OFFICIAL_SHA256_MISMATCH "+digest(partial))
    partial.replace(pkg)
   finally:partial.unlink(missing_ok=True)
  if digest(pkg)!=SHA256:raise RuntimeError("SHA256_MISMATCH_CACHED")
  add("OFFICIAL_INSTALLER_SHA256","PASS",SHA256)
  (out/"package-lock.json").write_text(json.dumps({
     "source":URL,"sha256":SHA256,"bytes":pkg.stat().st_size,
     "publisher_format":"Windows installer EXE, not official portable ZIP",
     "experiment":"extract without installing, no compilation"},indent=2)+"\n")
  unpack=cache/"extracted"
  unpack.mkdir(exist_ok=True)
  opts=["innoextract","--extract","--output-dir",str(unpack),str(pkg)]
  with (out/"innoextract.log").open("w") as f:
   try:rc=subprocess.run(opts,stdout=f,stderr=subprocess.STDOUT,timeout=240,check=False).returncode
   except subprocess.TimeoutExpired:rc=124;f.write("TIMEOUT\n")
   except OSError as exc:rc=127;f.write(repr(exc)+"\n")
  extractor="innoextract"
  if rc!=0:
   # NSIS/7z installers are not Inno; do not mask the failure or mix
   # partial extraction trees from two different tools.
   shutil.rmtree(unpack)
   unpack.mkdir()
   cmd=["7z","x","-y","-o"+str(unpack),str(pkg)]
   with (out/"7zip-extract.log").open("w") as f:
    try:rc=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,timeout=240,check=False).returncode
    except subprocess.TimeoutExpired:rc=124;f.write("TIMEOUT\\n")
    except OSError as exc:rc=127;f.write(repr(exc)+"\\n")
   extractor="7zip"
  if rc!=0:
   add("PORTABLE_EXTRACTION","FAIL","innoextract unsupported; 7zip exit="+str(rc)+"; inspect both extraction logs")
  else:
   filepaths=sorted(x for x in unpack.rglob("*") if x.is_file())
   executables=[str(x.relative_to(unpack)) for x in filepaths if x.suffix.lower()==".exe"]
   resources=[str(x.relative_to(unpack)) for x in filepaths if "resources" in x.parts or "profiles" in x.parts]
   (out/"extracted-inventory.json").write_text(json.dumps({
     "file_count":len(filepaths),"exe_files":executables[:120],
     "resource_file_count":len(resources),"resource_samples":resources[:120],
     "definition":"archive extraction inventory, NOT executable provenance or functionality proof"
     },indent=2)+"\n",encoding="utf-8")
   if not executables or not resources:
    add("PORTABLE_EXTRACTION","FAIL",
        "EXTRACTED_TREE_MISSING_EXECUTABLE_OR_RESOURCES exe_count="+str(len(executables))
        +" resource_count="+str(len(resources)))
   else:
    add("PORTABLE_EXTRACTION","PASS",
        "OFFICIAL_INSTALLER_EXTRACTED_WITH_"+extractor+" without installation/compilation; exe_count="+str(len(executables))
        +" resources="+str(len(resources)))
  add("PORTABLE_CLI","PENDING","not yet proven runnable without install; needs verified official profile and G-code")
  add("FILUNA_PARITY","PENDING","Filuna native adapter has not been executed")
  add("NETWORK","PENDING","No real Filuna Flashforge device protocol evidence")
 except Exception as exc:add("RUNNER","FAIL",type(exc).__name__+": "+str(exc))
 (out/"results.json").write_text(json.dumps(rows,indent=2)+"\n",encoding="utf-8")
 (out/"RESUMO.md").write_text(
 "# Filuna Flashforge 1.7.15 — extração do instalador OFICIAL\n\n"
 "**Não é uma release portátil oficial, não instala e não compila.**\n\n"
 "| Gate | Estado | Evidência |\n|---|---|---|\n"
 + "".join(f"| {r['stage']} | {r['status']} | {r['detail']} |\n" for r in rows)
 +"\nNenhum PASS de extração autoriza afirmar compatibilidade Filuna, fatiamento, rede ou homologação.\n",encoding="utf-8")
 return 1 if any(r["status"]=="FAIL" for r in rows) else 0

if __name__=="__main__":raise SystemExit(main())
