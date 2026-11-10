#!/usr/bin/env python3
"""Clona a fonte oficial fixada de um modulo de fabricante e confere a integridade.

Uso:
    python FILUNA/fabricantes/tools/fetch_oem_source.py FILUNA/fabricantes/snapmaker/module.json [--dest DIR]

Le `module.json`, faz clone raso e esparso do commit fixado, e confere:
  * o commit obtido e exatamente o declarado;
  * o hash git de cada arvore/arquivo declarado em source.trees / source.blobs;
  * a existencia de cada arquivo e arvore listados em components.

Nao edita nada da fonte clonada. Sai com codigo != 0 em qualquer divergencia.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path


def git(args: list[str], cwd: Path | None = None) -> str:
    out = subprocess.run(["git", *args], cwd=cwd, check=True, capture_output=True, text=True)
    return out.stdout.strip()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("manifest", type=Path)
    ap.add_argument("--dest", type=Path, default=None,
                    help="pasta de destino (padrao: <pasta do manifest>/oem-src)")
    ns = ap.parse_args()

    manifest = json.loads(ns.manifest.read_text(encoding="utf-8"))
    src = manifest["source"]
    dest = ns.dest or ns.manifest.parent / "oem-src"
    errors: list[str] = []

    if not (dest / ".git").exists():
        dest.mkdir(parents=True, exist_ok=True)
        git(["init", "-q"], cwd=dest)
        git(["remote", "add", "origin", src["repository"]], cwd=dest)
        git(["sparse-checkout", "set", "--no-cone", *manifest["sparse_paths"]], cwd=dest)
        git(["fetch", "-q", "--depth", "1", "origin", src["commit"]], cwd=dest)
        git(["-c", "advice.detachedHead=false", "checkout", "-q", "FETCH_HEAD"], cwd=dest)

    head = git(["rev-parse", "HEAD"], cwd=dest)
    if head != src["commit"]:
        errors.append(f"COMMIT_MISMATCH got={head} want={src['commit']}")

    for kind in ("trees", "blobs"):
        for path, want in src.get(kind, {}).items():
            try:
                got = git(["rev-parse", f"HEAD:{path}"], cwd=dest)
            except subprocess.CalledProcessError:
                got = "MISSING"
            if got != want:
                errors.append(f"HASH_MISMATCH {path} got={got} want={want}")

    files = trees = 0
    for name, comp in manifest["components"].items():
        for rel in comp.get("files", []):
            files += 1
            if not (dest / rel).is_file():
                errors.append(f"FILE_MISSING component={name} {rel}")
        for rel in comp.get("trees", []):
            trees += 1
            if not (dest / rel).is_dir():
                errors.append(f"TREE_MISSING component={name} {rel}")

    print(f"MODULE={manifest['id']} COMMIT={head} FILES_CHECKED={files} TREES_CHECKED={trees}")
    for e in errors:
        print(e)
    print("SOURCE_STATUS=" + ("FAIL" if errors else "PASS"))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
