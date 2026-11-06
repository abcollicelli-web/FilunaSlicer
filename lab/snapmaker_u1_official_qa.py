#!/usr/bin/env python3
"""Snapmaker U1 0.4 official v2.4.1 Windows portable QA, without OEM build.

This is a real CLI smoke and A/B runner. PASS of the official standalone
executable is *not* evidence of integration with Filuna, network or hardware.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time
import zipfile

from portable_first_qa import OFFICIAL, obtain, extract_executable, digest

ROOT=Path(__file__).resolve().parents[1]
TAG="v2.4.1"
MODEL="Snapmaker U1"
VARIANT="0.4"
PROFILE_SOURCE="Snapmaker/OrcaSlicer"
OFFICIAL_PROFILE_BLOBS={
 "machine":"d9a6ab12cb872af25275fab6045b05cfc4531e2f",
 "process":"932a52d485a20ea2042716b366733f48f40facfc",
 "filament":"8e4ba5984b072578b82c842b7b465947bf82cfb2",
}
FIELDS=("stage","status","details","evidence")

def git_blob_sha(path:Path)->str:
 data=path.read_bytes()
 return hashlib.sha1(b"blob "+str(len(data)).encode("ascii")+b"\0"+data).hexdigest()

def command(exe:Path,args:list[str],cwd:Path,log:Path,timeout:int=190)->int:
 log.parent.mkdir(parents=True,exist_ok=True)
 cmd=[str(exe),*args]
 with log.open("w",encoding="utf-8",errors="replace") as f:
  f.write("COMMAND_JSON="+json.dumps(cmd,ensure_ascii=False)+"\n")
  f.flush()
  try:
   return subprocess.run(cmd,cwd=cwd,stdout=f,stderr=subprocess.STDOUT,
                         timeout=timeout,check=False).returncode
  except subprocess.TimeoutExpired:
   f.write("\nTIMEOUT_AFTER_SECONDS="+str(timeout)+"\n")
   return 124
  except OSError as e:
   f.write("\nOSError="+repr(e)+"\n")
   return 127

def generated_gcodes(folder:Path)->list[Path]:
 found=[]
 for p in folder.rglob("*"):
  if not p.is_file():continue
  if p.suffix.lower() in (".gcode",".gco") and p.stat().st_size>0:
   found.append(p)
  elif p.suffix.lower()==".3mf":
   try:
    with zipfile.ZipFile(p) as z:
     gc=[x for x in z.namelist() if x.lower().endswith((".gcode",".gco"))]
     for member in gc:
      output=p.parent/(p.stem+"_"+Path(member).name)
      output.write_bytes(z.read(member))
      if output.stat().st_size>0:found.append(output)
   except zipfile.BadZipFile:pass
 return sorted(set(found))

def main():
 ap=argparse.ArgumentParser()
 ap.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
 ap.add_argument("--out",type=Path,default=Path("qa-snapmaker-u1"))
 ap.add_argument("--timeout",type=int,default=190)
 args=ap.parse_args()
 cache=args.cache.resolve();out=args.out.resolve()
 cache.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
 rows=[]
 def add(stage,status,details,evidence=""):
  assert status in ("PASS","FAIL","PENDING")
  rows.append(dict(stage=stage,status=status,details=details,evidence=evidence))
  print(f"SNAPMAKER_U1 {stage}: {status} — {details}",flush=True)

 try:
  cfg=OFFICIAL["snapmaker"]
  archive,url=obtain(cfg,cache)
  add("OFFICIAL_PORTABLE_ARCHIVE","PASS","sha256="+digest(archive)+" version="+cfg["version"],url)
  exe=extract_executable(archive,cfg["exe"],cache/"snapmaker-portable-extracted").resolve()
  add("PORTABLE_EXECUTABLE","PASS",str(exe.relative_to(cache)))
  profiles=(ROOT/"fixtures/oem/snapmaker/official-profiles").resolve()
  resolved={}
  manifest={"source_repository":PROFILE_SOURCE,"source_tag":TAG,"printer_model":MODEL,
            "printer_variant":VARIANT,"asset_sha256":cfg["sha256"],"official_profiles":{}}
  for kind,gitsha in OFFICIAL_PROFILE_BLOBS.items():
   path=profiles/(kind+".json")
   if not path.is_file():raise ValueError("CHECKOUT_PROFILE_MISSING "+str(path))
   # git checkout on Windows can turn LF into CRLF, changing the worktree
   # hash. Read the ORIGINAL GIT OBJECT bytes; never weaken provenance checks.
   gitpath=path.relative_to(ROOT).as_posix()
   blob=subprocess.run(["git","show","HEAD:"+gitpath],cwd=ROOT,
                       stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False)
   if blob.returncode!=0:
    raise ValueError("ORIGINAL_PROFILE_GIT_OBJECT_NOT_FOUND "+gitpath+" "+
                     blob.stderr.decode("utf-8","replace")[:300])
   source=blob.stdout
   actual=hashlib.sha1(b"blob "+str(len(source)).encode("ascii")+b"\0"+source).hexdigest()
   if actual!=gitsha:
    raise ValueError("OFFICIAL_PROFILE_PROVENANCE_MISMATCH "+kind+" expected="+gitsha+" actual="+actual)
   source_path=out/("verified-"+kind+".json")
   source_path.write_bytes(source)
   info=json.loads(source.decode("utf-8-sig"))
   if info.get("type")!=kind or info.get("from")!="system":
    raise ValueError("OFFICIAL_PROFILE_TYPE_MISMATCH "+kind)
   resolved[kind]=source_path
   manifest["official_profiles"][kind]={"git_blob_sha":gitsha,
       "sha256":digest(source_path),"name":info["name"],"inherits":info.get("inherits"),
       "git_object_used_for_execution":gitpath,
       "checkout_worktree_sha256":digest(path),
       "origin":"resources/profiles/Snapmaker/"+kind+"/"+{
         "machine":"Snapmaker U1 (0.4 nozzle).json",
         "process":"0.20mm Standard @Snapmaker U1 (0.4 nozzle).json",
         "filament":"Snapmaker PLA Basic @U1.json"}[kind]}
  m=json.loads(resolved["machine"].read_text(encoding="utf-8-sig"))
  if m.get("printer_model")!=MODEL or m.get("printer_variant")!=VARIANT:
   raise ValueError("WRONG_SELECTED_PRINTER_MODEL")
  pr=json.loads(resolved["process"].read_text(encoding="utf-8-sig"))
  fi=json.loads(resolved["filament"].read_text(encoding="utf-8-sig"))
  if m["name"] not in pr.get("compatible_printers",[]) or m["name"] not in fi.get("compatible_printers",[]):
   raise ValueError("PROCESS_OR_FILAMENT_NOT_COMPATIBLE_WITH_U1_04")
  (out/"upstream.lock.json").write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
  add("U1_OFFICIAL_PROFILE_PROVENANCE","PASS","three upstream Git blobs byte-exact; U1 0.4 compatible",
      "upstream.lock.json")
  cube=ROOT/"fixtures/cube20.stl"
  if not cube.is_file():raise ValueError("20MM_CUBE_MISSING")
  add("CUBE_FIXTURE","PASS","sha256="+digest(cube))
  # The U1 has four independent toolheads; use the same official U1 PLA
  # preset for each tool in the single-material cube control.
  profile_arg=str(resolved["machine"])+";"+str(resolved["process"])
  filament_arg=";".join([str(resolved["filament"])]*4)
  commands=[
    ("direct-slice",["--debug","3","--slice","0",
         "--load-settings",profile_arg,"--load-filaments",filament_arg]),
    ("export-profiled-3mf",["--debug","3",
         "--load-settings",profile_arg,"--load-filaments",filament_arg])
  ]
  official_outputs=[]
  # Perform independent runs in distinct output directories; do NOT accept
  # the same file counted twice and do NOT rewrite official G-code.
  for letter in ("A","B"):
   folder=out/("run-"+letter)
   if folder.exists():shutil.rmtree(folder)
   folder.mkdir(parents=True)
   argv=[*commands[0][1],"--outputdir",str(folder),str(cube.resolve())]
   rc=command(exe,argv,exe.parent,out/("official-"+letter+".log"),args.timeout)
   gcodes=generated_gcodes(folder)
   (out/("official-"+letter+"-inventory.json")).write_text(json.dumps({
     "argv":[str(exe),*argv],"exit_code":rc,"files":[{"path":str(x.relative_to(folder)),
        "bytes":x.stat().st_size,"sha256":digest(x)} for x in folder.rglob("*") if x.is_file()]},
        indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
   if rc!=0 or len(gcodes)!=1:
    add("OFFICIAL_"+letter,"FAIL",f"EXIT={rc} GCODE_COUNT={len(gcodes)}; see CLI log/inventory",
        "official-"+letter+".log")
    add("OFFICIAL_A_B","PENDING","OFFICIAL_CLI_NOT_YET_SUCCESSFUL")
    break
   g=gcodes[0]
   official_outputs.append(g)
   add("OFFICIAL_"+letter,"PASS","gcode_bytes="+str(g.stat().st_size)+" sha256="+digest(g),
       str(g.relative_to(out)))
  if len(official_outputs)==2:
   first,second=official_outputs
   if first.resolve()==second.resolve():
    add("OFFICIAL_A_B","FAIL","SAME_GCODE_FILE_REUSED")
   elif digest(first)!=digest(second):
    add("OFFICIAL_A_B","FAIL","GCODE_RAW_SHA_DIFFERENT; do not normalize without official A/B proof "
       +"A="+digest(first)+" B="+digest(second))
   else:
    add("OFFICIAL_A_B","PASS","RAW_SHA256_IDENTICAL "+digest(first))
  # This optional export validates a profiled 3MF independent of direct slicing;
  # absence is a limitation, not a substitution for G-code proof.
  project_path=out/"exported-U1-0.4-project.3mf"
  argv=[*commands[1][1],"--export-3mf",str(project_path),str(cube.resolve())]
  rc=command(exe,argv,exe.parent,out/"export-profiled-3mf.log",args.timeout)
  if rc==0 and project_path.is_file() and zipfile.is_zipfile(project_path):
   add("U1_PROJECT_EXPORT","PASS","official project 3mf sha256="+digest(project_path))
  else:
   add("U1_PROJECT_EXPORT","PENDING","CLI_EXPORT_NOT_PROVEN exit="+str(rc)
       +" file_present="+str(project_path.is_file()))
  add("FILUNA_PARITY","PENDING","No separately executing Filuna OEM engine; official A/B is not Filuna parity")
  add("DEVICE_NETWORK","PENDING","Simulator-only U1 OEM contract not yet executed")
  add("PHYSICAL_BETA","PENDING","By owner: ONLY after all internal QA, final Filuna build and supervised beta")
 except Exception as e:
  add("RUNNER","FAIL",type(e).__name__+": "+str(e))
 (out/"results.json").write_text(json.dumps(rows,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
 (out/"RESUMO.md").write_text("# Filuna OEM Snapmaker U1 — CLI portátil oficial 2.4.1\n\n"
   "Sem compilação OEM, instalação, deploy ou impressão física.\n\n"
   "| Gate | Estado | Evidência |\n|---|---|---|\n"
   +"".join("| "+r["stage"]+" | "+r["status"]+" | "+r["details"].replace("|","/")+" |\n" for r in rows)
   +"\nEste teste não equivale a paridade Filuna nem à homologação física.\n",encoding="utf-8")
 return 1 if any(r["status"]=="FAIL" for r in rows) else 0

if __name__=="__main__":
 raise SystemExit(main())
