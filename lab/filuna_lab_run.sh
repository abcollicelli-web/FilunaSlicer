#!/usr/bin/env bash
# Single controlled Filuna QA runner. Never launches a physical print or deploy.
set -Eeuo pipefail
umask 077
ROOT=/srv/site/incoming/filuna-local-oem-qa-20261106
MODE=${1:---plan}
[[ "$MODE" == --plan || "$MODE" == --run ]] || exit 64
[[ "$MODE" != --run || "${FILUNA_LAB_OWNER_APPROVED:-}" == yes ]] || { echo OWNER_APPROVAL_REQUIRED; exit 78; }
cd "$ROOT"
python3 - "$MODE" "$ROOT" <<'PY'
import sys,os,json,hashlib,pathlib,datetime,subprocess,shlex
mode,root=sys.argv[1:];root=pathlib.Path(root)
run=root/('run-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ'))
run.mkdir(mode=0o700)
vendors=['bambu','elegoo','flashforge','qidi','creality','snapmaker','prusa','anycubic']
official={'bambu':'bambu-v02.09.01.70/bambu-studio.exe','elegoo':'elegoo-v1.5.3.5/elegoo-slicer.exe','flashforge':None,'qidi':'qidi-v2.07.02.73/QIDIStudio/qidi-studio.exe','creality':'creality-v7.3.1/CrealityPrint.exe','snapmaker':'snapmaker-v2.4.1/Snapmaker_Orca_Windows_V2.4.1_portable/Snapmaker Orca.exe','prusa':None,'anycubic':None}
bases=pathlib.Path('/srv/site/incoming/filuna-official-portables-20261106/unpacked')
steps=['make_cube_3mf','make_four_colors_3mf','slice_official_cube','slice_filuna_cube','slice_official_four_colors','slice_filuna_four_colors','contract_discover','contract_pair','contract_persist','contract_status','contract_send','contract_start','contract_pause','contract_resume','contract_stop','contract_camera','contract_materials']
cfgpath=root/'lab'/'filuna_lab_commands.json'
cfg=json.loads(cfgpath.read_text()) if cfgpath.exists() else {}
events=[]
def sha(path):
 with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def add(e):
 events.append(e)
 with (run/'events.jsonl').open('a') as f:f.write(json.dumps(e,ensure_ascii=False)+'\n')
for vendor in vendors:
 d=run/vendor;d.mkdir()
 exe=bases/official[vendor] if official[vendor] else None
 meta={'vendor':vendor,'official_executable':str(exe) if exe else None,'exists':bool(exe and exe.is_file()),'sha256':sha(exe) if exe and exe.is_file() else None}
 (d/'official-manifest.json').write_text(json.dumps(meta,indent=2))
 for step in steps:
  cmd=cfg.get(vendor,{}).get(step)
  e={'vendor':vendor,'step':step,'command':cmd,'exit_code':None,'status':'NÃO EXECUTADO'}
  if mode!='--run':e['reason']='PLAN_ONLY'
  elif not isinstance(cmd,list) or not cmd:e['reason']='COMMAND_NOT_CONFIGURED'
  elif not pathlib.Path(cmd[0]).is_absolute() or not pathlib.Path(cmd[0]).is_file():e['reason']='EXECUTABLE_MISSING'
  else:
   e['command_shell']=shlex.join(cmd)
   e['stdout']=str(d/(step+'.stdout.log'));e['stderr']=str(d/(step+'.stderr.log'))
   try:
    with open(e['stdout'],'wb') as o,open(e['stderr'],'wb') as err:
     p=subprocess.run(cmd,cwd=d,stdout=o,stderr=err,timeout=120,check=False)
    e['exit_code']=p.returncode;e['status']='PASS' if p.returncode==0 else 'FALHA'
    out=[x for x in d.iterdir() if x.is_file() and x.suffix.lower() in ('.gcode','.3mf')]
    if step.startswith(('make_','slice_')) and not out:e.update(status='FALHA',reason='NO_EXPECTED_OUTPUT')
    e['sha256']={x.name:sha(x) for x in out}
   except subprocess.TimeoutExpired:e.update(status='BLOQUEADO',reason='TIMEOUT_120_SECONDS_AFTER_ATTEMPT')
   except OSError as exc:e.update(status='BLOQUEADO',reason=str(exc))
  add(e)
summary={'mode':mode,'brand_order':vendors,'checks':len(events),'status_counts':{st:sum(e['status']==st for e in events) for st in ('PASS','FALHA','BLOQUEADO','NÃO EXECUTADO')},'note':'No commands defined means NOT EXECUTED; no manufacturer is homologated by this manifest.'}
(run/'manifest.json').write_text(json.dumps(summary,indent=2,ensure_ascii=False))
print(json.dumps({'run_dir':str(run),**summary},ensure_ascii=False,indent=2))
PY
