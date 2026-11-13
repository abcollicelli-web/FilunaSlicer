#!/usr/bin/env python3
"""Filuna OEM first-stage portable verification; no vendor builds and no installs.

Proof levels:
  PACKAGE_OK: verified upstream download SHA-256 and executable presence.
  CLI_OK: subprocess returned 0 for an explicit caller-selected command.
  A_B_EQUAL: two independent official CLI runs produced exactly identical G-code.
  FILUNA_EQUAL: separate Filuna build executed and output compared (NOT implemented here).

A verified archive never implies successful slicing or a homologated Filuna OEM module.
"""
from __future__ import annotations
import argparse
import base64
import csv
import hashlib
import json
import os
from pathlib import Path
import shutil
import re
import subprocess
import sys
import urllib.request
import zipfile

ROOT=Path(__file__).resolve().parents[1]
BRANDS=("bambu","elegoo","flashforge","qidi","creality","snapmaker","prusa","anycubic")
# SHA-256 digests are the GitHub Release asset digests at the pinned official tags.
OFFICIAL={
 "bambu":{
  "version":"02.09.01.70","repo":"bambulab/BambuStudio",
  "asset":"Bambu_Studio_win-v02.09.01.70-20261103183404.zip",
  "tag":"v02.09.01.70","sha256":"a4c0d5f94109cb2c2c07fb02ca1f740e96b586fbf3ae557a1647eeb5503a15b8",
  "exe":"bambu-studio.exe"},
 "elegoo":{
  "version":"1.5.3.5","repo":"elegooofficial/ElegooSlicer",
  "asset":"ElegooSlicer_Windows_V1.5.3.5_portable.zip","tag":"v1.5.3.5",
  "sha256":"0215cd958d6a52ae1bd33a5ad7509ecd1b156baf63fbe04333e04cf0305793e5",
  "exe":"elegoo-slicer.exe"},
 "qidi":{
  "version":"2.07.02.73","repo":"QIDITECH/QIDIStudio",
  "asset":"QIDIStudio_v02.07.02.73_Windows.zip","tag":"v2.07.02.73",
  "sha256":"30f276c5390e4ca755c0a08d2115d6ea5feea2a2ee7ec7a3a7e3a729d0094224",
  "exe":"qidi-studio.exe"},
 "snapmaker":{
  "version":"2.4.1","repo":"Snapmaker/OrcaSlicer",
  "asset":"Snapmaker_Orca_Windows_V2.4.1_portable.zip","tag":"v2.4.1",
  "sha256":"1f002e2168c576f800bcfd0509b5e164b5ad65a0963abc2bef8317d929d48c9d",
  "exe":"snapmaker-orca.exe"},
 "prusa":{
  "version":"2.9.7","repo":"prusa3d/PrusaSlicer",
  "asset":"PrusaSlicer-2.9.7.zip","tag":"version_2.9.7",
  "sha256":"5d4f395aeb6c344b300e0e1be37a0e1931622b642c3c3085eb19cdae211bc008",
  "exe":"prusa-slicer-console.exe"},
}
FIXTURE_SHA256={
 "bambu":"212202ece5982aaceccdbe57359c070ecec3c4ddb00d4baf211c80556f50baa4",
 "elegoo":"c5a6bb377548c129cb1c14d2b72ea1cc49197ecccc339da99c3c37b75ea68450",
 "qidi":"2e5a8d293bac8846d547e16a0d4d04ee07e67b63ed1eea6f3b1844b38266ac04",
}
BLOCKED={
 "flashforge":"NO_PINNED_OFFICIAL_PORTABLE_ARCHIVE_1.7.15 (installer is not portable)",
 "creality":"NO_PINNED_OFFICIAL_PORTABLE_ARCHIVE_7.3.1 (existing EXE is installer)",
 "anycubic":"NO_VERSIONED_PINNED_PORTABLE_RELEASE_AND_DIGEST",
}
FIELDS=("brand","version","stage","status","detail","evidence")
MAX_ARCHIVE_BYTES=900*1024*1024

def digest(p:Path)->str:
 h=hashlib.sha256()
 with p.open("rb") as f:
  for chunk in iter(lambda:f.read(1024*1024),b""):h.update(chunk)
 return h.hexdigest()

def record(rows,brand,version,stage,status,detail,evidence=""):
 assert status in ("PASS","FAIL","PENDING")
 rows.append(dict(brand=brand,version=version,stage=stage,status=status,detail=detail,evidence=evidence))

def obtain(cfg:dict,cache:Path)->tuple[Path,str]:
 url="https://github.com/"+cfg["repo"]+"/releases/download/"+cfg["tag"]+"/"+cfg["asset"]
 cache.mkdir(parents=True,exist_ok=True)
 archive=cache/cfg["asset"]
 expected=cfg["sha256"]
 if not archive.is_file() or digest(archive)!=expected:
  if archive.exists(): archive.unlink()
  partial=archive.with_suffix(archive.suffix+".part")
  if partial.exists():partial.unlink()
  request=urllib.request.Request(url,headers={"User-Agent":"FilunaOEM-Portable-QA/1"})
  try:
   with urllib.request.urlopen(request,timeout=90) as response,partial.open("wb") as out:
    total=0
    while True:
     chunk=response.read(1024*1024)
     if not chunk:break
     total+=len(chunk)
     if total>MAX_ARCHIVE_BYTES:raise ValueError("ARCHIVE_EXCEEDS_MAX_BYTES")
     out.write(chunk)
   got=digest(partial)
   if got!=expected:raise ValueError("SHA256_MISMATCH expected="+expected+" actual="+got)
   partial.replace(archive)
  finally:
   if partial.exists(): partial.unlink()
 return archive,url

def extract_executable(archive:Path,exe_name:str,extract_dir:Path)->Path:
 extract_dir.mkdir(parents=True,exist_ok=True)
 # Extract whole portable tree (DLLs/resources are part of CLI correctness).
 with zipfile.ZipFile(archive) as z:
  root=extract_dir.resolve()
  for entry in z.infolist():
   target=(extract_dir/entry.filename).resolve()
   if not target.is_relative_to(root):raise ValueError("ZIP_PATH_TRAVERSAL "+entry.filename)
   # Windows NTFS reparse/symlink members are not permitted.
   if (entry.external_attr>>16)&0o170000 == 0o120000:
    raise ValueError("ZIP_SYMLINK_REJECTED "+entry.filename)
  if not (extract_dir/".filuna-extraction-ok").exists():
   z.extractall(extract_dir)
   (extract_dir/".filuna-extraction-ok").write_text(digest(archive),encoding="ascii")
  elif (extract_dir/".filuna-extraction-ok").read_text(encoding="ascii")!=digest(archive):
   shutil.rmtree(extract_dir)
   return extract_executable(archive,exe_name,extract_dir)
 candidates=sorted(x for x in extract_dir.rglob("*") if x.is_file() and x.name.lower()==exe_name.lower())
 if len(candidates)==0 and "snapmaker" in exe_name.lower():
  # Snapmaker upstream ZIP filename may differ from its previously extracted Windows package.
  candidates=sorted(x for x in extract_dir.rglob("*.exe") if x.is_file() and "snapmaker" in x.stem.lower() and all(w not in x.stem.lower() for w in ("uninstall","setup","update","crash")))
 if len(candidates)>1 and "snapmaker" in exe_name.lower():
  # A real GUI slicer can coexist with its console/validator/support tools.
  # Prefer exactly one main executable; fail closed if still ambiguous.
  main=[p for p in candidates if not any(word in p.stem.lower() for word in ("console","validator","profile","updater","update","test","crash"))]
  if len(main)==1:candidates=main
 if len(candidates)!=1:
  names=[p.relative_to(extract_dir).as_posix() for p in candidates]
  raise ValueError("PORTABLE_EXE_NOT_UNIQUE count="+str(len(candidates))+" file="+exe_name+" candidates="+repr(names[:20]))
 return candidates[0]

def run_cli(exe:Path,project:Path,out:Path,logfile:Path,timeout:int)->tuple[int,list[Path]]:
 out=out.resolve()
 project=project.resolve()
 exe=exe.resolve()
 out.mkdir(parents=True,exist_ok=True)
 if not project.is_file():
  with logfile.open("w",encoding="utf-8") as f:f.write("ERROR: PROFILED_PROJECT_NOT_FOUND "+str(project)+"\\n")
  return 127,[]
 cmd=[str(exe),"--debug","3","--slice","0","--outputdir",str(out),str(project)]
 with logfile.open("w",encoding="utf-8",errors="replace") as log:
  log.write("ARGV="+json.dumps(cmd,ensure_ascii=False)+"\n")
  log.flush()
  try:
   p=subprocess.run(cmd,cwd=exe.parent,stdout=log,stderr=subprocess.STDOUT,timeout=timeout,check=False)
   code=p.returncode
  except subprocess.TimeoutExpired:
   log.write("ERROR: PORTABLE_SLICE_TIMEOUT\n");code=124
  except OSError as error:
   log.write("ERROR: PORTABLE_LAUNCH_FAILED "+repr(error)+"\n");code=127
 outputs=sorted(p for p in out.rglob("*") if p.is_file() and p.suffix.lower() in (".gcode",".gco"))
 return code,outputs

def compare_official_a_b(brand:str,version:str,a:Path,b:Path,folder:Path)->tuple[str,str]:
 """Fail closed. Allow ONLY the Elegoo 1.5.3.5 timestamp, proven in official A/B."""
 sa,sb=digest(a),digest(b)
 la=a.read_bytes().splitlines(keepends=True)
 lb=b.read_bytes().splitlines(keepends=True)
 proof={"brand":brand,"official_version":version,"file_A":str(a.name),"file_B":str(b.name),
        "raw_sha256_A":sa,"raw_sha256_B":sb,"line_count_A":len(la),"line_count_B":len(lb),
        "allowed_dynamic_lines":[],"unexpected_differences":[]}
 allowed={}
 if brand=="elegoo" and version=="1.5.3.5":
  allowed[2]=re.compile(rb"; generated by ElegooSlicer 1\.5\.3\.5 on \d{4}-\d{2}-\d{2} at \d{2}:\d{2}:\d{2}")
 aa=list(la);bb=list(lb)
 if len(la)!=len(lb):
  proof["unexpected_differences"].append("GCODE_LINE_COUNT_DIFFERENT")
 else:
  for line,(xa,xb) in enumerate(zip(la,lb),start=1):
   if xa==xb:continue
   rx=allowed.get(line)
   # A/B itself must establish the only difference and both line formats must
   # match. Do not normalize other comments, UUIDs, G-code or toolpath.
   if rx is not None and rx.fullmatch(xa.rstrip(b"\r\n")) and rx.fullmatch(xb.rstrip(b"\r\n")):
    proof["allowed_dynamic_lines"].append({
     "line":line,"A":xa.rstrip(b"\r\n").decode("utf-8","replace"),
     "B":xb.rstrip(b"\r\n").decode("utf-8","replace"),
     "policy":"OFFICIAL_A_B_TIMESTAMP_ONLY"})
    aa[line-1]=bb[line-1]=b"; FILUNA PROVEN OEM TIMESTAMP HEADER\n"
   else:
    proof["unexpected_differences"].append({
     "line":line,"A":xa[:180].decode("utf-8","replace"),
     "B":xb[:180].decode("utf-8","replace")})
 if not proof["unexpected_differences"] and len(aa)==len(bb):
  proof["normalized_sha256_A"]=hashlib.sha256(b"".join(aa)).hexdigest()
  proof["normalized_sha256_B"]=hashlib.sha256(b"".join(bb)).hexdigest()
  proof["normalization_required"]=bool(proof["allowed_dynamic_lines"])
 else:
  proof["normalization_required"]=False
 (folder/"official-A-B-proof.json").write_text(json.dumps(proof,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
 if proof["unexpected_differences"]:
  difference=proof["unexpected_differences"][0]
  return "FAIL","OFFICIAL_AB_DIFFERENCE_NOT_AUTHORIZED "+str(difference)[:300]
 if proof.get("normalized_sha256_A")!=proof.get("normalized_sha256_B"):
  return "FAIL","NORMALIZED_OUTPUT_HASH_DIFFERENT"
 if not proof["normalization_required"]:
  return "PASS","RAW_SHA_IDENTICAL "+sa
 return "PASS",("OFFICIAL_AB_TIMESTAMP_ONLY_NORMALIZATION lines="
                +str([x["line"] for x in proof["allowed_dynamic_lines"]])
                +" raw_A="+sa+" raw_B="+sb
                +" normalized_sha256="+proof["normalized_sha256_A"])

def test_brand(brand,cache,out,run_slicing,timeout,rows):
 cfg=OFFICIAL.get(brand)
 if cfg is None:
  record(rows,brand,"unknown","PORTABLE","PENDING",BLOCKED[brand]);return
 version=cfg["version"]
 bdir=out/brand;bdir.mkdir(parents=True,exist_ok=True)
 metadata={k:cfg[k] for k in ("version","repo","tag","asset","sha256","exe")}
 (bdir/"upstream-lock.json").write_text(json.dumps(metadata,indent=2)+"\n",encoding="utf-8")
 try:
  archive,url=obtain(cfg,cache)
  real=digest(archive)
  record(rows,brand,version,"PACKAGE_SHA256","PASS",real,url)
  exe=extract_executable(archive,cfg["exe"],cache/(brand+"-portable-extracted"))
  record(rows,brand,version,"PORTABLE_EXE","PASS",exe.relative_to(cache).as_posix(),str(bdir/"upstream-lock.json"))
 except Exception as e:
  record(rows,brand,version,"PORTABLE","FAIL",str(e));return
 # Profiled OEM .3mf is REQUIRED. STL/default fallbacks would prove the wrong
 # selected device/profile and must never be called homologation.
 fixture=ROOT/"fixtures"/"oem"/brand/"cube-profiled.3mf"
 encoded=fixture.with_suffix(fixture.suffix+".b64")
 if not fixture.is_file() and encoded.is_file():
  # The historical official 3MF test fixture is checked in as portable
  # base64 text because GitHub's text-only API cannot store binary blobs.
  try:
   fixture=bdir/"cube-profiled.3mf"
   fixture.write_bytes(base64.b64decode(encoded.read_text(encoding="ascii").strip(),validate=True))
   if not zipfile.is_zipfile(fixture):raise ValueError("NOT_A_3MF_ZIP")
   actual_fixture_sha=digest(fixture)
   expected_fixture_sha=FIXTURE_SHA256.get(brand)
   if expected_fixture_sha is not None and actual_fixture_sha!=expected_fixture_sha:
    raise ValueError("HISTORICAL_FIXTURE_HASH_MISMATCH expected="+expected_fixture_sha+" actual="+actual_fixture_sha)
   record(rows,brand,version,"PROFILED_FIXTURE","PASS",f"sha256={actual_fixture_sha}",str(encoded.relative_to(ROOT)))
  except Exception as err:
   record(rows,brand,version,"PROFILED_FIXTURE","FAIL",repr(err),str(encoded))
   return
 if not fixture.is_file():
  record(rows,brand,version,"OFFICIAL_A_B","PENDING","PROFILED_3MF_NOT_PRESENT "+str(fixture))
  record(rows,brand,version,"FILUNA_PARITY","PENDING","FILUNA_PORTABLE_OUTPUT_NOT_PROVIDED")
  record(rows,brand,version,"NETWORK","PENDING","FILUNA_DEVICE_PROTOCOL_NOT_EXERCISED")
  return
 if not run_slicing:
  record(rows,brand,version,"OFFICIAL_A_B","PENDING","SLICE_FLAG_NOT_ENABLED")
  return
 outcomes=[]
 for letter in ("A","B"):
  directory=bdir/("run-"+letter)
  if directory.exists():shutil.rmtree(directory)
  rc,gcodes=run_cli(exe,fixture,directory,bdir/("official-"+letter+".log"),timeout)
  if rc!=0 or len(gcodes)!=1 or not gcodes[0].stat().st_size:
   record(rows,brand,version,"OFFICIAL_"+letter,"FAIL",f"CLI_EXIT={rc} GCODE_COUNT={len(gcodes)}",str(bdir/("official-"+letter+".log")))
   record(rows,brand,version,"OFFICIAL_A_B","PENDING","OFFICIAL_CLI_FAILED");return
  outcomes.append((gcodes[0],digest(gcodes[0])))
  record(rows,brand,version,"OFFICIAL_"+letter,"PASS",f"SHA256={outcomes[-1][1]} BYTES={gcodes[0].stat().st_size}",str(gcodes[0]))
 state,proof=compare_official_a_b(brand,version,outcomes[0][0],outcomes[1][0],bdir)
 record(rows,brand,version,"OFFICIAL_A_B",state,proof,str(bdir/"official-A-B-proof.json"))
 record(rows,brand,version,"FILUNA_PARITY","PENDING","SEPARATE_FILUNA_EXECUTION_NOT_YET_COMPARED")
 record(rows,brand,version,"NETWORK","PENDING","NO_REAL_FILUNA_DEVICE_FLOW_EVIDENCE")

def main():
 ap=argparse.ArgumentParser()
 ap.add_argument("--output",type=Path,default=Path("qa-portable"))
 ap.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
 ap.add_argument("--run-slicing",action="store_true",default=False)
 ap.add_argument("--timeout",type=int,default=180)
 ap.add_argument("--brands",nargs="*",choices=BRANDS,default=list(BRANDS))
 args=ap.parse_args()
 args.output.mkdir(parents=True,exist_ok=True)
 args.cache.mkdir(parents=True,exist_ok=True)
 rows=[]
 for brand in args.brands:
  try:test_brand(brand,args.cache,args.output,args.run_slicing,args.timeout,rows)
  except Exception as e:record(rows,brand,OFFICIAL.get(brand,{}).get("version","unknown"),"RUNNER","FAIL",repr(e))
  if brand=="bambu":
   # Owner decision: only Bambuddy integration is deliberately deferred.
   # Official Bambu portable tests remain active and must keep their evidence.
   record(rows,brand,OFFICIAL["bambu"]["version"],"BAMBUDDY_NETWORK","PENDING",
          "OWNER_DEFERRED_BAMBUDDY; DO_NOT_EXECUTE_UNTIL_OWNER_REQUEST")
 with (args.output/"results.csv").open("w",newline="",encoding="utf-8") as f:
  w=csv.DictWriter(f,fieldnames=FIELDS);w.writeheader();w.writerows(rows)
 (args.output/"results.json").write_text(json.dumps(rows,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
 summary=["# Filuna OEM QA — portáteis oficiais, SEM COMPILAR",
          "Este teste não aprova módulos OEM do Filuna. SHA verificado prova somente pacote.",
          "",
          "| Marca | Pacote SHA | Binário portátil | Oficial A×B | Filuna vs OEM | Rede |",
          "|---|---|---|---|---|---|"]
 for b in BRANDS:
  b_rows=[r for r in rows if r["brand"]==b]
  def state(stage):
   a=[r["status"] for r in b_rows if r["stage"]==stage]
   return a[-1] if a else "PENDING"
  summary.append("| "+b+" | "+state("PACKAGE_SHA256")+" | "+state("PORTABLE_EXE")+" | "+state("OFFICIAL_A_B")+" | "+state("FILUNA_PARITY")+" | "+state("NETWORK")+" |")
 summary+=["","**ADIADO POR DECISÃO DO PROPRIETÁRIO:** Bambu + Bambuddy = PENDING. Somente esse teste foi adiado; os demais gates e fabricantes continuam em validação.",
           "PASS de pacote não significa PASS de fatiamento, rede ou homologação.",
           "Não há instalação, compilação de fatiador, acionamento de impressora, deploy ou publicação.",
           "Sem 3MF perfilado original para cada marca, a comparação A/B NÃO é executada."]
 (args.output/"RESUMO.md").write_text("\n".join(summary)+"\n",encoding="utf-8")
 print("\n".join(summary),flush=True)
 return 1 if any(r["status"]=="FAIL" for r in rows) else 0

if __name__=="__main__":raise SystemExit(main())
