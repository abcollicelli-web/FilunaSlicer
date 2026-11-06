#!/usr/bin/env python3
"""Stage narrowly scoped OEM source overlays using pinned Git blobs.

No compilation, loading, functional parity, or redistribution clearance is
implied. Categories never include other manufacturers' profile trees.
"""
import argparse,collections,hashlib,json,subprocess
from pathlib import Path

ROOTS={
"bambu":["BBL"],
"elegoo":["Elegoo"],
"creality":["Creality"],
"flashforge":["Flashforge"],
"qidi":["Q Series","X 3 Series","X 4 Series","X 5 Series"],
"snapmaker":["Snapmaker"],
"anycubic":["Anycubic"],
"elegoo-link":[],
}
CLI={
"bambu":"BambuStudio","elegoo":"ElegooSlicer","creality":"CrealityPrint",
"flashforge":"Orca-Flashforge","qidi":"QIDIStudio","snapmaker":"Snapmaker_Orca",
"anycubic":"OrcaSlicer",
}
PREPARE_NAMES={
"Plater","PartPlate","BackgroundSlicingProcess","Tab","ParamsPanel",
"PresetComboBoxes","UnsavedChangesDialog","SavePresetDialog",
"ProjectDirtyStateManager","SyncPresetWithPrinterDialog",
}
# Directories tied to manufacturer-specific device interfaces; ordinary
# unrelated GUI and navigation pages are intentionally excluded.
NETWORK={
"bambu":("src/slic3r/GUI/DeviceCore/","src/slic3r/GUI/SelectMachine",
  "src/slic3r/GUI/SendToPrinter","src/slic3r/Utils/NetworkAgent",
  "src/slic3r/Utils/FileTransferObject"),
"elegoo":("src/slic3r/Utils/Elegoo/","src/slic3r/GUI/Elegoo/",
  "src/slic3r/Utils/ElegooLink","src/slic3r/Utils/PrinterUploadManager"),
"creality":("src/slic3r/GUI/print_manage/","src/slic3r/GUI/SendToPrinter",
  "src/slic3r/Utils/NetworkAgent"),
"flashforge":("src/slic3r/GUI/FlashForge/","src/slic3r/Utils/FlashNetwork"),
"qidi":("src/slic3r/GUI/SendToPrinter","src/slic3r/Utils/NetworkAgent",
  "src/slic3r/Utils/Udp","src/slic3r/Utils/FileTransfer"),
"snapmaker":("src/slic3r/Utils/MoonRaker","src/slic3r/Utils/GatewayService",
  "src/slic3r/Utils/ConnectionProcessManager","src/slic3r/Utils/MQTT",
  "src/mqtt/"),
"anycubic":(), "elegoo-link":("src/lan/","src/discovery/","src/"),
}
WEB_ROOTS={
"bambu":("src/slic3r/GUI/DeviceCore/DeviceWeb/", "src/slic3r/GUI/DeviceWeb/"),
"elegoo":("src/slic3r/GUI/Elegoo/ElegooPrinterWebView",),
"creality":("src/slic3r/GUI/print_manage/","src/slic3r/GUI/DeviceTab/"),
"flashforge":("src/slic3r/GUI/FlashForge/","src/slic3r/GUI/DeviceTab/"),
"qidi":("src/slic3r/GUI/DeviceWeb/",),"snapmaker":("src/slic3r/GUI/DeviceWeb/",),
"anycubic":(), "elegoo-link":(),
}
ENGINE_THIRDPARTY=("src/clipper/","src/clipper2/","src/admesh/","src/libnest2d/",
 "src/qhull/","src/eigen/","src/libigl/","src/mcut/","src/boost/",
 "src/nlopt/","src/ankerl/","src/agg/","src/fast_float/",
 "src/libzip/","src/embree/","src/meshoptimizer/")
DROP=("telemetry","sentry","aliyun","breakpad","localization",
 "translation","/i18n/","/locale/","artificialintelligence","ai_plugin",
 "crashreport","reporter","telemetryevents","/simple/bridge/")
IMAGE_SUFFIX={".png",".jpg",".jpeg",".webp",".svg",".gif",".ico",".bmp"}
def run(root,*args):
 r=subprocess.run(["git","-C",str(root),*args],capture_output=True,check=True)
 return r.stdout
def git_tree(root,ref):
 d={}
 raw=run(root,"ls-tree","-r","-z",ref,"src","deps","cmake","resources/profiles","resources/printers")
 for item in raw.split(b"\0"):
  if not item:continue
  meta,path=item.split(b"\t",1);mode,typ,sha=meta.decode().split()
  d[path.decode("utf-8","surrogateescape")]=(mode,typ,sha)
 return d
def category(name,p):
 low=p.lower()
 if p.startswith("resources/profiles/"):
  rest=p[len("resources/profiles/"):]
  return "Perfis" if any(rest==label+".json" or rest.startswith(label+"/") for label in ROOTS[name]) else None
 if p.startswith("resources/printers/"):
  return "Perfis" if p.lower().endswith(".json") else None
 if any(x in low for x in DROP):return None
 if p.startswith("src/libslic3r/") or p.startswith(("deps/","cmake/")):return "Motor"
 if any(p.startswith(x) for x in ENGINE_THIRDPARTY):return "Motor"
 if name in CLI and (p=="src/"+CLI[name]+".cpp" or p=="src/"+CLI[name]+"_app_msvc.cpp"):return "Motor"
 if p.startswith("src/slic3r/GUI/"):
  basename=Path(p).stem
  if basename in PREPARE_NAMES or ("/SyncPresetWithPrinterDialog" in p):return "Preparar"
  if any(p.startswith(x) for x in WEB_ROOTS[name]) and (
     p.endswith((".html",".js",".css",".json")) or "WebView" in p or "DeviceWeb/" in p):return "Web"
 if any(p.startswith(prefix) for prefix in NETWORK[name]):
  if p.endswith(tuple(IMAGE_SUFFIX)):return None
  return "Rede"
 return None
def main():
 ap=argparse.ArgumentParser()
 ap.add_argument("--brand",choices=sorted(ROOTS),required=True)
 ap.add_argument("--filuna",type=Path,required=True)
 ap.add_argument("--ref",default="HEAD")
 ap.add_argument("--official",type=Path,required=True)
 ap.add_argument("--out",type=Path,required=True)
 ap.add_argument("--copy",action="store_true")
 a=ap.parse_args();a.out.mkdir(parents=True,exist_ok=True)
 base=git_tree(a.filuna,a.ref);off=git_tree(a.official,"HEAD")
 rows=[];counts={k:collections.Counter() for k in ("Motor","Perfis","Preparar","Rede","Web")}
 for path,(mode,kind,blob) in sorted(off.items()):
  cat=category(a.brand,path)
  if not cat:continue
  if mode not in ("100644","100755") or kind!="blob":continue
  old=base.get(path)
  status="idêntico" if old==(mode,kind,blob) else ("modificado" if old else "novo")
  counts[cat][status]+=1
  if status=="idêntico":continue
  f=a.official/path
  if not f.is_file() or f.is_symlink():raise RuntimeError("Nonregular "+path)
  body=f.read_bytes()
  actual=hashlib.sha1(b"blob "+str(len(body)).encode()+b"\0"+body).hexdigest()
  if actual!=blob:raise RuntimeError("Git content differs "+path)
  rows.append({"path":path,"category":cat,"status":status,"sha256":hashlib.sha256(body).hexdigest(),
               "git_blob":blob,"bytes":len(body),"mode":mode})
  if a.copy:
   target=a.out/"source"/path
   target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(body)
 for cat in counts:
  for val in ("idêntico","modificado","novo"):counts[cat][val]+=0
 result={"brand":a.brand,"official_commit":run(a.official,"rev-parse","HEAD").decode().strip(),
         "filuna_commit":run(a.filuna,"rev-parse",a.ref).decode().strip(),
         "scope":"filtered per owner 2026-11-06","state":"SOURCE_ONLY_NOT_COMPILED_NOT_LOADED",
         "counts":{x:dict(v) for x,v in counts.items()},"rows":rows}
 (a.out/"manifest.json").write_text(json.dumps(result,ensure_ascii=False,indent=2)+"\n")
 print(json.dumps({"brand":a.brand,"counts":result["counts"],"files_staged":len(rows),
  "bytes_staged":sum(x["bytes"] for x in rows),"manifest":str(a.out/"manifest.json")},ensure_ascii=False))
if __name__=="__main__":main()
