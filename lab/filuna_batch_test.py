#!/usr/bin/env python3
"""Eight-brand evidence-first Filuna OEM batch. Never invents a native PASS."""
from __future__ import annotations
import argparse
import csv
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import urllib.request
import zipfile

BRANDS=("bambu","elegoo","flashforge","qidi","creality","snapmaker","prusa","anycubic")
STEPS=("T1","T3","T4","T5","T6","T7","T8","T9")
REPO=Path(__file__).resolve().parents[1]
FIELDS=("marca","teste","status","mensagem","comando","correcao","log")
PRESERVED={
 "bambu":{
   "evidence":"FILUNA/qa/HOMOLOGACAO-LAB-8-FABRICANTES-R3-20261106.md",
   "cube_sha":"f907c93683ae5208fe47a9d22f3b9b125e51bbbaf87d8947b7a0e94c026950a4",
   "multicolor_sha":"bafae92490dd04a924f7ffa9312ba7a1ddb4ea79aebbae21a64f628e009569e1"
 }
}
def sha(path):
 h=hashlib.sha256()
 with Path(path).open("rb") as f:
  for b in iter(lambda:f.read(1024*1024),b""):h.update(b)
 return h.hexdigest()
def run(argv, log, timeout=120):
 with Path(log).open("w",encoding="utf-8",errors="replace") as out:
  out.write("$ "+" ".join(str(t) for t in argv)+"\n")
  out.flush()
  try:
   p=subprocess.run(argv,stdout=out,stderr=subprocess.STDOUT,timeout=timeout,check=False)
   return p.returncode
  except subprocess.TimeoutExpired:
   out.write("\nERROR: TIMEOUT\n");return 124
  except OSError as exc:
   out.write("\nERROR: "+str(exc)+"\n");return 127
def emit(rows,brand,step,status,message,command="",fix="",log=""):
 assert status in ("PASS","FALHA","PENDENTE")
 rows.append(dict(marca=brand,teste=step,status=status,mensagem=message,comando=command,correcao=fix,log=log))
def resolve_manifest(brand):
 p=REPO/"fabricantes"/brand/"vendor.yaml"
 try:return json.loads(p.read_text(encoding="utf-8"))
 except Exception as exc:raise ValueError(str(exc))
def safe_extract_zip(path,out):
 with zipfile.ZipFile(path) as z:
  root=out.resolve()
  for member in z.infolist():
   resolved=(out/member.filename).resolve()
   if not resolved.is_relative_to(root):raise RuntimeError("archive path traversal: "+member.filename)
  z.extractall(out)
def acquire(cfg, cache, log):
 official=cfg["official"]
 url=official.get("download_url")
 expected=official.get("archive_sha256")
 if not url or not expected or not re.fullmatch("[0-9a-f]{64}",expected):
  raise RuntimeError("DOWNLOAD_NOT_PINNED: official URL and real SHA-256 required in vendor.yaml")
 archive=cache/(cfg["brand"]+"-"+Path(url.split("?")[0]).name)
 if not archive.exists():
  tmp=archive.with_suffix(archive.suffix+".partial")
  with urllib.request.urlopen(url,timeout=90) as r, tmp.open("wb") as out:shutil.copyfileobj(r,out)
  tmp.replace(archive)
 got=sha(archive)
 if got!=expected:raise RuntimeError("ARCHIVE_SHA256_MISMATCH expected="+expected+" actual="+got)
 out=cache/(cfg["brand"]+"-extracted")
 if not out.exists():
  out.mkdir(parents=True)
  fmt=official.get("archive_kind")
  if fmt=="zip":safe_extract_zip(archive,out)
  elif fmt in ("tar.gz","tgz"):
   with tarfile.open(archive,"r:gz") as t:
    for m in t.getmembers():
     if not (out/m.name).resolve().is_relative_to(out.resolve()):raise RuntimeError("archive path traversal")
    t.extractall(out,filter="data")
  else:raise RuntimeError("EXTRACTOR_NOT_CONFIGURED: "+repr(fmt))
 exe=out/(official.get("exe_relative_path") or "")
 if not exe.is_file():raise RuntimeError("OFFICIAL_EXE_NOT_FOUND "+str(exe))
 return exe
def clean_dir(p):
 if p.exists():shutil.rmtree(p)
 p.mkdir(parents=True)
def files_in(p):
 return sorted(q for q in p.rglob("*") if q.suffix.lower() in (".gcode",".gco") and q.is_file())
def formatted(argv, exe, source, output):
 return [str(x).format(exe=str(exe),input=str(source),out=str(output)) for x in argv]
def compare_normalized(a,b,rules,prooflog):
 aa=Path(a).read_bytes().splitlines(keepends=True)
 bb=Path(b).read_bytes().splitlines(keepends=True)
 if len(aa)!=len(bb):return False,"GCODE_LINE_COUNT_DIFFERENT"
 permitted={}
 for r in rules:
  num=int(r["line"])
  if num<1 or num>len(aa):return False,f"NORMALIZATION_LINE_MISSING line={num}"
  rx=re.compile(r["regex"].encode("utf-8"))
  if not rx.fullmatch(aa[num-1].rstrip(b"\r\n")) or not rx.fullmatch(bb[num-1].rstrip(b"\r\n")):
   return False,f"NORMALIZATION_REGEX_MISMATCH line={num}"
  permitted[num]=rx
 diffs=[]
 for i,(x,y) in enumerate(zip(aa,bb),start=1):
  if x!=y:
   diffs.append(i)
   if i not in permitted:return False,f"UNPROVEN_DIFFERENCE line={i} officialA={x[:180]!r} officialB={y[:180]!r}"
  if i in permitted:
   aa[i-1]=bb[i-1]=b"; OEM PROVEN VARIABLE\n"
 with Path(prooflog).open("a",encoding="utf-8") as f:f.write("VARIABLE_LINES="+repr(diffs)+"\n")
 return b"".join(aa)==b"".join(bb),"NORMALIZED_SHA="+hashlib.sha256(b"".join(aa)).hexdigest()
def t1(brand,cfg,store,cache,os_name,rows):
 # Historical proof is not a new run and is labeled as such.
 if brand=="bambu":
  p=PRESERVED["bambu"]
  emit(rows,brand,"T1","PASS","HISTORICAL_PROVEN_NOT_REEXECUTED cube="+p["cube_sha"]+" four_color="+p["multicolor_sha"],p["evidence"],"Conservar evidências")
  return
 owner_multicolor=cfg.get("flags",{}).get("owner_multicolor_pending",False)
 if not cfg.get("cli",{}).get("verified"):
  msg="OEM_CLI_ARGUMENTS_NOT_VERIFIED"
  if owner_multicolor:msg+="; OWNER_OFFICIAL_MULTICOLOR_3MF_NOT_RECEIVED"
  emit(rows,brand,"T1","PENDENTE",msg,str(REPO/"fabricantes"/brand/"vendor.yaml"),"Verificar CLI do cubo e receber o 3MF multicor oficial")
  return
 fixture=REPO/cfg.get("fixtures",{}).get("cube","fixtures/cube20.stl")
 if not fixture.is_file():
  emit(rows,brand,"T1","PENDENTE","CUBE_FIXTURE_MISSING",str(fixture),"Adicionar fixture")
  return
 log=str(store/"T1.log")
 try:
  exe=acquire(cfg,cache,log)
 except Exception as exc:
  emit(rows,brand,"T1","PENDENTE",str(exc),"download+SHA256","Fixar URL oficial, SHA, formato do pacote e caminho do binário",log)
  return
 cmdcfg=cfg["cli"]["slice"]
 if not cmdcfg:
  emit(rows,brand,"T1","PENDENTE","OEM_SLICE_TEMPLATE_MISSING",str(REPO/"fabricantes"/brand/"vendor.yaml"),"Preencher CLI comprovada")
  return
 output=store/"official-execution"
 archived=[]
 for name in ("A","B"):
  clean_dir(output)
  cmd=formatted(cmdcfg,exe,fixture,output)
  if os_name=="ubuntu" and Path(cmd[0]).suffix.lower()==".exe":
   cmd=["xvfb-run","-a","-s","-screen 0 1920x1080x24 +extension GLX +render -noreset","wine",*cmd]
  env=dict(os.environ)
  env.update(LIBGL_ALWAYS_SOFTWARE="1",GALLIUM_DRIVER="llvmpipe",__GLX_VENDOR_LIBRARY_NAME="mesa")
  logfile=store/("official-"+name+".log")
  old=os.environ.copy();os.environ.update(env)
  try:rc=run(cmd,logfile,180)
  finally:os.environ.clear();os.environ.update(old)
  g=files_in(output)
  if rc!=0 or not g:
   emit(rows,brand,"T1","FALHA",f"OFFICIAL_{name}_EXIT_{rc}_GCODE_COUNT_{len(g)}"," ".join(cmd),"Inspecionar log da CLI oficial",str(logfile));return
  dest=store/("official-"+name+".gcode");shutil.copy2(g[0],dest);archived.append(dest)
 valid,detail=compare_normalized(archived[0],archived[1],cfg.get("normalization",[]),store/"normalization-proof.log")
 if not valid:
  emit(rows,brand,"T1","FALHA",detail,str(store/"official-A.log"),"Investigar divergência oficial A×B antes de normalizar",str(store/"normalization-proof.log"));return
 filuna=cfg.get("filuna",{})
 if not filuna.get("slice") or not filuna.get("exe_relative_path"):
  emit(rows,brand,"T1","PENDENTE","FILUNA_NATIVE_CLI_NOT_CONNECTED "+detail,"", "Vincular executável nativo do aplicativo e repetir comparação")
  return
 emit(rows,brand,"T1","PENDENTE","FILUNA_NATIVE_CLI_COMPARISON_NOT_IMPLEMENTED"+("; OWNER_MULTICOLOR_PENDING" if owner_multicolor else ""),"","Completar comparação oficial×Filuna com os mesmos perfis e SHA normalizado")
def contract(brand,cfg,store,native_binary,rows):
 # Owner decision: ONLY the authenticated Bambu/Bambuddy validation is
 # intentionally deferred. Other Bambu OEM network/GUI/gcode tests remain
 # in the active QA queue; never label all T3-T9 as "deferred".
 if brand=="bambu" and cfg.get("flags",{}).get("defer_bambuddy_authenticated"):
  emit(rows,brand,"BAMBUDDY_AUTH","PENDENTE",
       "OWNER_DEFERRED_ONLY_BAMBU_BAMBUDDY_AUTHENTICATED",
       "", "Executar validação autenticada com Bambuddy na etapa posterior definida pelo proprietário")
 if not native_binary or not native_binary.is_file():
  message="NATIVE_CPP_BINARY_NOT_COMPILED"
 elif not cfg.get("network",{}).get("contract_ready"):
  message="NATIVE_OEM_CONTRACT_NOT_IMPLEMENTED_FOR_APP"
 else:
  message="NATIVE_CONTRACT_RUNNER_NOT_READY"
 for step in STEPS[1:]:
  emit(rows,brand,step,"PENDENTE",message,"filuna_batch_test --list (linkage only)","Portar C++ oficial ao adapter e executar contrato com ACK e evidência")
def main():
 p=argparse.ArgumentParser()
 p.add_argument("--all",action="store_true",required=True)
 p.add_argument("--platform",choices=("ubuntu","windows"),required=True)
 p.add_argument("--output",default=None)
 p.add_argument("--native-binary",default="")
 a=p.parse_args()
 name="batch-"+dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")+"-"+a.platform
 output=Path(a.output) if a.output else Path.cwd()/name
 output.mkdir(parents=True,exist_ok=True)
 cache=Path(os.environ.get("FILUNA_OEM_CACHE",str(Path.cwd()/".oem-cache")))
 cache.mkdir(parents=True,exist_ok=True)
 rows=[];owners=[]
 for brand in BRANDS:
  folder=output/brand;folder.mkdir(parents=True,exist_ok=True)
  try:
   cfg=resolve_manifest(brand)
   (folder/"vendor-snapshot.json").write_text(json.dumps(cfg,indent=2,ensure_ascii=False),encoding="utf-8")
   t1(brand,cfg,folder,cache,a.platform,rows)
   contract(brand,cfg,folder,Path(a.native_binary) if a.native_binary else None,rows)
   if cfg.get("flags",{}).get("owner_multicolor_pending"):owners.append(brand+": 3MF multicor da interface oficial")
  except Exception as exc:
   emit(rows,brand,"SETUP","FALHA",f"{type(exc).__name__}: {exc}","","Corrigir vendor.yaml/runner, reexecutar lote")
   for step in STEPS:
    if not any(x["marca"]==brand and x["teste"]==step for x in rows):
     emit(rows,brand,step,"PENDENTE","SETUP_EXCEPTION","","Corrigir erro registrado em SETUP")
  bybrand=[x for x in rows if x["marca"]==brand]
  (folder/"result.log").write_text("\n".join(f"{x['teste']} {x['status']}: {x['mensagem']}" for x in bybrand)+"\n",encoding="utf-8")
 # Infrastructure failures must appear in RESUMO instead of hiding in artifact logs.
 for filename,key in (("native-build.status","CMAKE_BUILD_EXIT"),("app-build.status","FILUNA_APP_CONFIG_EXIT")):
  fp=output.parent/filename
  if not fp.exists():
   emit(rows,"infra","BUILD","PENDENTE",f"BUILD_STATUS_MISSING {filename}",str(fp),"Analisar runner e CMake")
   continue
  text=fp.read_text(encoding="utf-8",errors="replace")
  matches=re.findall(r"(?:^|\\s)"+key+r"=(\\d+)",text)
  code=int(matches[-1]) if matches else -1
  if code!=0:
   logfile=output.parent/("cmake-build.log" if filename=="native-build.status" else "app-configure.log")
   tail=""
   if logfile.is_file():
    candidates=[z.strip() for z in logfile.read_text(encoding="utf-8",errors="replace").splitlines() if "CMake Error" in z or "Could NOT find" in z or "error:" in z]
    tail="; ".join(candidates[-3:])[:700]
   emit(rows,"infra","BUILD","FALHA",f"{key}={code}; {tail}",str(logfile),"Instalar deps oficiais do Filuna, configurar toolchain e reexecutar")
 simulator=output.parent/"simulators-config.log"
 if a.platform=="ubuntu" and simulator.is_file() and "PENDENTE" in simulator.read_text(errors="replace"):
  emit(rows,"infra","SIMULADORES","PENDENTE","SIMULATORS_NOT_PINNED",str(simulator),"Fixar imagens oficiais por digest e configuração de rede isolada")
 with (output/"results.csv").open("w",encoding="utf-8",newline="") as f:
  w=csv.DictWriter(f,fieldnames=FIELDS);w.writeheader();w.writerows(rows)
 with (output/"RESUMO.md").open("w",encoding="utf-8") as f:
  f.write(f"# Filuna OEM batch — {a.platform}\n\n")
  f.write("Resultados reais + evidência histórica identificada. PENDENTE nunca equivale a PASS.\n\n")
  f.write("| Marca | PASS | FALHA | PENDENTE |\n|---|---:|---:|---:|\n")
  for b in BRANDS:
   items=[x for x in rows if x["marca"]==b]
   f.write("| "+b+" | "+" | ".join(str(sum(x["status"]==s for x in items)) for s in ("PASS","FALHA","PENDENTE"))+" |\n")
  f.write("\n## Erros e testes pendentes\n\n| Marca | Teste | Mensagem exata | Comando/arquivo | Correção proposta |\n|---|---|---|---|---|\n")
  grouped={}
  for x in rows:
   if x["status"]!="PASS":
    key=(x["marca"],x["mensagem"],x["comando"],x["correcao"])
    grouped.setdefault(key,[]).append(x["teste"])
  for (brand,message,command,fix),steps in grouped.items():
   values=(brand,",".join(steps),message,command,fix)
   f.write("| "+" | ".join(v.replace("|","/").replace("\n"," ") for v in values)+" |\n")
  f.write("\n## Pendências do proprietário\n\n")
  for line in sorted(set(owners)):f.write("- "+line+"\n")
  f.write("- Bambu A1 MQTT/FTPS autenticação/contrato: adiado por decisão do proprietário.\n")
  f.write("- URLs e SHA256 oficiais dos pacotes sem referência verificável (não supor hashes).\n")
  f.write("\n**SEM merge, deploy, publicação ou homologação adicional.**\n")
 print(output/"RESUMO.md")
 return 2 if any(x["status"] in ("FALHA","PENDENTE") for x in rows) else 0
if __name__=="__main__":raise SystemExit(main())
