#!/usr/bin/env python3
"""Official Snapmaker U1 2.4.1 CLI: repeated path-controlled A/B/A/B trial.

Zero OEM compilation, zero physical printer. All input bytes and vendor ZIP
SHA pinned. Detect whether altered filenames or CLI-run order correlate with
the two real, different toolpath hashes discovered in run #42320900067.
"""
from __future__ import annotations
import argparse
from collections import defaultdict
import json
from pathlib import Path
import shutil
from portable_first_qa import OFFICIAL,digest,obtain,extract_executable
from snapmaker_u1_inherit import build_effective_original_presets
from snapmaker_runtime_nozzle_guard_qa import guarded_runtime_process
from snapmaker_u1_guard_probe import run_cli
from snapmaker_u1_temporal_ab_qa import header_date_only_normalization
ROOT=Path(__file__).resolve().parents[1]
SEQUENCE=("base","base","process_b","process_b","base",
          "filament_b","filament_b","base","process_b","filament_b")

def main():
    pa=argparse.ArgumentParser()
    pa.add_argument("--cache",type=Path,default=Path(".oem-portable-cache"))
    pa.add_argument("--out",type=Path,default=Path("qa-snapmaker-u1-path-stability"))
    x=pa.parse_args();out=x.out.resolve();cache=x.cache.resolve()
    out.mkdir(parents=True,exist_ok=True);cache.mkdir(parents=True,exist_ok=True)
    events=[];results=defaultdict(list)
    def log(step,status,detail):
        events.append({"stage":step,"status":status,"detail":detail})
        print("SNAPMAKER_U1_PATH_REPEAT",step,status,detail,flush=True)
    try:
        archive,_=obtain(OFFICIAL["snapmaker"],cache)
        exe=extract_executable(archive,OFFICIAL["snapmaker"]["exe"],
                               cache/"snapmaker-portable-extracted")
        log("OFFICIAL_PACKAGE_SHA","PASS",digest(archive))
        files=build_effective_original_presets(out)
        original_process=out/"original-name-process.json"
        lock=guarded_runtime_process(files["machine"],files["process"],original_process)
        (out/"official-runtime-lock.json").write_text(json.dumps(lock,indent=2)+"\n")
        process_b=out/"different-name-process.json"
        filament_b=out/"different-name-filament.json"
        shutil.copyfile(original_process,process_b)
        shutil.copyfile(files["filament"],filament_b)
        assert digest(original_process)==digest(process_b)
        assert digest(files["filament"])==digest(filament_b)
        log("PROFILES_BYTE_IDENTICAL","PASS","process and filament duplicates SHA identical")
        for n,which in enumerate(SEQUENCE,1):
            proc=process_b if which=="process_b" else original_process
            fil=filament_b if which=="filament_b" else files["filament"]
            label=f"{n:02d}-{which}"
            info=run_cli(exe,files["machine"],proc,fil,
                         ROOT/"fixtures/cube20.stl",out,label,timeout=180)
            if info["exit_code"]!=0 or len(info["gcodes"])!=1:
                log("OEM_CLI_"+label,"FAIL",f"exit={info['exit_code']} count={len(info['gcodes'])}")
                break
            item=info["gcodes"][0]
            g=out/item["path"]
            norm=header_date_only_normalization(g.read_bytes())
            record={"step":n,"case":which,"gcode":item["path"],
                    "raw_sha256":item["sha256"],"bytes":item["bytes"],
                    "normalized_sha256":norm["normalized_sha256"],
                    "generated_at":norm["timestamp"],
                    "machine_sha256":digest(files["machine"]),
                    "process_sha256":digest(proc),"filament_sha256":digest(fil)}
            results[which].append(record)
            log("OEM_CLI_"+label,"PASS",
                f"bytes={record['bytes']} normalized={record['normalized_sha256']}")
        allhash=set();groups={}
        for case,recs in results.items():
            hashes=sorted({q["normalized_sha256"] for q in recs})
            groups[case]={"runs":len(recs),"normalized_hashes":hashes,
                          "intra_case_stable":len(hashes)==1}
            allhash.update(hashes)
            log("SAME_PATH_REPEAT_"+case,"PASS" if len(hashes)==1 and len(recs)>=3 else "FAIL",
                f"runs={len(recs)} variants={len(hashes)} "+str(hashes))
        stable=len(groups)==3 and all(g["intra_case_stable"] and g["runs"]>=3 for g in groups.values())
        cross_equal=stable and len(allhash)==1
        if stable and not cross_equal:
            log("PATH_DEPENDENCE_DIAGNOSIS","PASS",
                "same paths repeat stably, different input file names change functional G-code; NOT validated for printing")
        elif cross_equal:
            log("PATH_DEPENDENCE_DIAGNOSIS","PASS","all original runs produced identical normalized G-code")
        else:
            log("PATH_DEPENDENCE_DIAGNOSIS","FAIL",
                "same-path repeats also vary; possible nondeterminism, not file-name causality")
        data={"package_sha256":digest(archive),
              "one_key_runtime_guard_only":lock,"sequence":list(SEQUENCE),
              "results":dict(results),"groups":groups,"all_hashes":sorted(allhash),
              "stable_by_case":stable,"cross_case_equal":cross_equal,
              "status":"DIAGNOSTIC_NOT_NATIVE_FILUNA_HOMOLOGATION",
              "normalization":"only exact generated-at line in verified header; no toolpath changes"}
        (out/"path-stability-proof.json").write_text(json.dumps(data,indent=2,ensure_ascii=False)+"\n")
        log("FILUNA_ENGINE_PARITY","PENDING","no Filuna-native generated G-code")
        log("SNAPMAKER_NETWORK_ACCESS","PENDING","no native live gateway/device ACK simulation")
        log("BETA_PHYSICAL","PENDING","hardware later when supervised beta opens")
    except Exception as e:
        log("RUNNER","FAIL",f"{type(e).__name__}: {e}")
    (out/"results.json").write_text(json.dumps(events,indent=2,ensure_ascii=False)+"\n")
    (out/"RESUMO.md").write_text("# Snapmaker U1 2.4.1 path stability controlled repetition\n\n"
      "Official binary only; no manufacturer recompilation, Filuna parity or hardware.\n\n"
      "| Gate | Status | Details |\n|---|---|---|\n"+
      "".join("| "+x["stage"]+" | "+x["status"]+" | "+x["detail"].replace("|","/")+" |\n" for x in events))
    return 1 if any(e["status"]=="FAIL" for e in events) else 0

if __name__=="__main__":raise SystemExit(main())
