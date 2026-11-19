#!/usr/bin/env python3
"""Build effective U1 presets from ONLY pinned official Snapmaker v2.4.1 JSON.

This is a temporary QA materialization, not a change to OEM source or to
Filuna's user profile tree. Every source blob is verified byte-exactly.
"""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
TAG="v2.4.1"
UPSTREAM="Snapmaker/OrcaSlicer"
LEAF_GIT_SHAS={
 "machine":"d9a6ab12cb872af25275fab6045b05cfc4531e2f",
 "process":"932a52d485a20ea2042716b366733f48f40facfc",
 "filament":"8e4ba5984b072578b82c842b7b465947bf82cfb2",
}

def git_original(relative_path:str,expected_sha:str)->bytes:
 result=subprocess.run(["git","show","HEAD:"+relative_path],cwd=ROOT,
                       stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False)
 if result.returncode!=0:
  raise ValueError("GIT_OBJECT_MISSING "+relative_path+" "+result.stderr.decode("utf-8","replace")[:140])
 data=result.stdout
 got=hashlib.sha1(b"blob "+str(len(data)).encode("ascii")+b"\0"+data).hexdigest()
 if got!=expected_sha:
  raise ValueError("OFFICIAL_PROFILE_GIT_SHA_MISMATCH "+relative_path+" expected="+expected_sha+" actual="+got)
 return data

def build_effective_original_presets(output:Path)->dict[str,Path]:
 """Returns machine/process/filament JSON paths with upstream inheritance resolved."""
 output.mkdir(parents=True, exist_ok=True)
 fixture=ROOT/"fixtures/oem/snapmaker/official-profiles"
 lock=json.loads((fixture/"parents.lock.json").read_text(encoding="utf-8-sig"))
 if lock.get("upstream_tag")!=TAG or lock.get("upstream_repository")!=UPSTREAM:
  raise ValueError("PARENT_LOCK_NOT_PINNED_TO_SAME_OFFICIAL_RELEASE")
 index={(item["type"],item["name"]):item for item in lock.get("parents",[])}
 result={};proof={}
 for kind,expected_leaf_sha in LEAF_GIT_SHAS.items():
  leaf_relative=f"fixtures/oem/snapmaker/official-profiles/{kind}.json"
  raw=git_original(leaf_relative,expected_leaf_sha)
  leaf=json.loads(raw.decode("utf-8-sig"))
  if leaf.get("type")!=kind or leaf.get("from")!="system":
   raise ValueError("LEAF_PRESET_IDENTITY_ERROR "+kind)
  lineage=[]
  parent_name=leaf.get("inherits")
  visited=set()
  while parent_name:
   if parent_name in visited:raise ValueError("UPSTREAM_INHERITANCE_CYCLE "+kind+" "+parent_name)
   visited.add(parent_name)
   item=index.get((kind,parent_name))
   if not item:raise ValueError("UNPINNED_OFFICIAL_PARENT "+kind+" "+parent_name)
   payload=git_original(item["fixture_path"],item["git_blob_sha"])
   parent=json.loads(payload.decode("utf-8-sig"))
   if parent.get("type")!=kind or parent.get("name")!=parent_name or (parent.get("inherits") or None)!=(item.get("inherits") or None):
    raise ValueError("PINNED_PARENT_METADATA_MISMATCH "+parent_name)
   lineage.append((parent_name,parent,item["git_blob_sha"]))
   parent_name=parent.get("inherits")
  effective={}
  for _,parent,_ in reversed(lineage):
   effective.update(parent)
  effective.update(leaf)
  # These are identity/renaming metadata only, not slicing parameters.
  effective.pop("inherits",None)
  effective.pop("renamed_from",None)
  effective["from"]="system"
  if effective.get("type")!=kind or effective.get("name")!=leaf.get("name"):
   raise ValueError("EFFECTIVE_CONFIG_ID_CHANGED "+kind)
  dest=output/("effective-"+kind+".json")
  dest.write_text(json.dumps(effective,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
  result[kind]=dest
  proof[kind]={"leaf_blob_sha":expected_leaf_sha,
    "parents":[{"name":name,"git_blob_sha":sha} for name,_,sha in lineage],
    "effective_sha256":hashlib.sha256(dest.read_bytes()).hexdigest(),
    "effective_key_count":len(effective),
    "operation":"recursive dictionary merge, oldest parent first, leaf last; omit only inherits/renamed_from",
    "provenance":"official source v2.4.1, immutable original Git objects"}
 (output/"effective-profile-derivation.json").write_text(
   json.dumps(proof,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
 return result
