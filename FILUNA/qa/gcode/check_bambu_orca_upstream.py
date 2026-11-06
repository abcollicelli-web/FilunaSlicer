#!/usr/bin/env python3
"""Read-only upstream change detector for Bambu OEM and Orca base.

Records source changes for human engineering review; NEVER ports code,
updates deployed clients, merges branches or promotes release gates.
"""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import urllib.error
import urllib.parse
import urllib.request

API="https://api.github.com"
def request_json(endpoint: str) -> dict:
    req=urllib.request.Request(API+endpoint,headers={
        "Accept":"application/vnd.github+json",
        "User-Agent":"FilunaSlicer-OEM-upstream-audit",
        **({"Authorization":"Bearer "+os.environ["GITHUB_TOKEN"]} if os.getenv("GITHUB_TOKEN") else {})
    })
    with urllib.request.urlopen(req,timeout=12) as response:
        return json.load(response)

def observe(manifest: dict, getter=request_json) -> dict:
    repo=manifest["upstream"]["repository"]
    branch=manifest["upstream"].get("branch","master")
    rows=[]
    paths=[{"path":r["path"],"expected":r["upstream_blob_sha"],"role":"A1_macro"} for r in manifest["components"]]
    paths.extend({"path":r["path"],"expected":r["blob_sha"],"role":"Bambu_source_candidate"} for r in manifest["upstream"].get("tracked_code_candidates",[]))
    for item in paths:
        endpoint=f"/repos/{repo}/contents/{urllib.parse.quote(item['path'],safe='/')}?ref={urllib.parse.quote(branch)}"
        try:
            remote=getter(endpoint)
            observed=remote.get("sha")
            status="SAME" if observed==item["expected"] else "UPDATED_REVIEW_REQUIRED"
        except Exception as exc:
            observed=None
            status="CHECK_FAILED"
            reason=type(exc).__name__+": "+str(exc)[:120]
        row={**item,"observed":observed,"status":status}
        if status=="CHECK_FAILED":row["error"]=reason
        rows.append(row)
    try:
        sha=getter("/repos/OrcaSlicer/OrcaSlicer/commits/main")["sha"]
        pin=manifest["filuna"]["upstream_orca_pin"]
        record={"role":"Orca_upstream","path":"main HEAD","expected":pin,"observed":sha,
                "status":"SAME" if sha==pin else "UPDATED_REVIEW_REQUIRED"}
        if sha != pin:
            # A version difference is not automatically an engine change.
            # Compare filenames to select targeted QA without granting a PASS.
            try:
                diff=getter(f"/repos/OrcaSlicer/OrcaSlicer/compare/{pin}...{sha}")
                changed=[item["filename"] for item in diff.get("files",[]) if "filename" in item]
                engine=any(name.startswith(("src/libslic3r/","resources/profiles/","src/slic3r/Utils/",
                                             "src/slic3r/GUI/DeviceCore/")) for name in changed)
                record["changed_files"]=changed
                record["review_scope"]="ENGINE_DEVICE_OR_PRESETS" if engine else "UI_DOC_OR_OTHER"
                record["compare_complete"]=bool(diff.get("status")) and not diff.get("too_large",False)
            except Exception as exc:
                record["review_scope"]="UNKNOWN_FULL_REVIEW"
                record["compare_error"]=type(exc).__name__+": "+str(exc)[:120]
        rows.append(record)
    except Exception as exc:
        rows.append({"role":"Orca_upstream","path":"main HEAD","status":"CHECK_FAILED",
                     "error":type(exc).__name__+": "+str(exc)[:120]})
    return {"kind":"FILUNA_OEM_UPSTREAM_DIFF_ONLY","rows":rows,
            "needs_review":any(x["status"]!="SAME" for x in rows),
            "does_not_auto_update":True,"hardware_homologated":False}

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--manifest",type=Path,default=Path(__file__).with_name("bambu_a1_source_manifest.json"))
    parser.add_argument("--out",type=Path,default=Path("/tmp/filuna-oem-upstream-review.json"))
    args=parser.parse_args()
    manifest=json.loads(args.manifest.read_text())
    result=observe(manifest)
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(result,indent=2,ensure_ascii=False)+"\n")
    print(json.dumps({"needs_review":result["needs_review"],"checks":[{"path":r["path"],"status":r["status"]} for r in result["rows"]]},ensure_ascii=False))
    return 1 if result["needs_review"] else 0

if __name__=="__main__":
    raise SystemExit(main())
