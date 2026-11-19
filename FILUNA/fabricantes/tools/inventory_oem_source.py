#!/usr/bin/env python3
"""Gera o module.json preliminar de um fabricante a partir da fonte oficial fixada.

Uso:
    python FILUNA/fabricantes/tools/inventory_oem_source.py --id elegoo \
        --repo https://github.com/elegooofficial/ElegooSlicer --ref refs/tags/v1.5.3.5 \
        --slicer "ElegooSlicer 1.5.3.5" --profiles Elegoo --work /tmp/oem --filuna-root .

Clona (raso e esparso) motor, GUI/rede e perfis no commit indicado, registra os
hashes git de arvore e classifica, POR NOME DE ARQUIVO, os arquivos de
src/slic3r que o fabricante criou ou alterou em relacao ao Filuna. A
classificacao e preliminar: serve de ponto de partida para revisao, nao de prova.
Nao edita a fonte clonada.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

RULE = "FILUNA/REGRA-CANONICA-MODULOS-FABRICANTE-SOB-DEMANDA-20261110.md"
SPARSE = ["/src/libslic3r/", "/src/slic3r/", "/resources/profiles/", "/version.inc", "/LICENSE.txt", "/LICENSE"]
SRC_EXT = {".cpp", ".hpp", ".h", ".c", ".mm", ".cc"}
# ordem importa: a primeira classe que casar fica com o arquivo
CLASSES = [
    ("camera", r"camera(popup|utils|ctrl)|media(play|file|ctrl)|video|player|stream|liveview|timelapse"),
    ("colors", r"ams|filament|colou?r|canvas|acepro|\bace\b|cfs|spool|materialbox|mapping|\bifs\b|multicolor|box"),
    ("network", r"bonjour|mdns|ssdp|discover|mqtt|websocket|printhost|link|connect|network|gateway|moonraker|octo|"
                r"klipper|lan|cloud|http|socket|tcp|udp|ftp|upload|sendto|send"),
    ("control", r"device|monitor|status|control|printer|job|task|sswcp|upgrade|calibration|hms|bind"),
]


def git(args, cwd=None) -> str:
    return subprocess.run(["git", *args], cwd=cwd, check=True, capture_output=True, text=True).stdout.strip()


def sha(p: Path) -> str:
    return hashlib.sha256(p.read_bytes().replace(b"\r\n", b"\n")).hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--id", required=True)
    ap.add_argument("--repo", required=True)
    ap.add_argument("--ref", required=True, help="refs/tags/<tag> ou commit completo")
    ap.add_argument("--slicer", required=True)
    ap.add_argument("--profiles", required=True, help="regex dos nomes em resources/profiles")
    ap.add_argument("--work", type=Path, required=True)
    ap.add_argument("--filuna-root", type=Path, default=Path("."))
    ap.add_argument("--note", default="")
    ns = ap.parse_args()

    dest = ns.work / ns.id
    if not (dest / ".git").exists():
        dest.mkdir(parents=True, exist_ok=True)
        git(["init", "-q"], cwd=dest)
        git(["remote", "add", "origin", ns.repo], cwd=dest)
        git(["sparse-checkout", "set", "--no-cone", *SPARSE], cwd=dest)
        git(["fetch", "-q", "--depth", "1", "--filter=blob:none", "origin", ns.ref], cwd=dest)
        git(["-c", "advice.detachedHead=false", "checkout", "-q", "FETCH_HEAD"], cwd=dest)
    commit = git(["rev-parse", "HEAD^{commit}"], cwd=dest)

    trees, blobs, prof_trees, prof_files = {}, {}, [], []
    for t in ("src/libslic3r", "src/slic3r"):
        if (dest / t).is_dir():
            trees[t] = git(["rev-parse", f"HEAD:{t}"], cwd=dest)
    pdir = dest / "resources/profiles"
    if pdir.is_dir():
        for e in sorted(pdir.iterdir()):
            if re.search(ns.profiles, e.name):
                rel = f"resources/profiles/{e.name}"
                h = git(["rev-parse", f"HEAD:{rel}"], cwd=dest)
                if e.is_dir():
                    trees[rel] = h
                    prof_trees.append(rel)
                else:
                    blobs[rel] = h
                    prof_files.append(rel)

    def changed(sub: str):
        """arquivos-fonte do fabricante ausentes ou diferentes no Filuna"""
        only, diff, total = [], [], 0
        base = dest / sub
        for p in sorted(base.rglob("*")) if base.is_dir() else []:
            if not p.is_file() or p.suffix.lower() not in SRC_EXT:
                continue
            total += 1
            rel = p.relative_to(dest).as_posix()
            q = ns.filuna_root / rel
            if not q.is_file():
                only.append(rel)
            elif sha(p) != sha(q):
                diff.append(rel)
        return total, only, diff

    e_total, e_only, e_diff = changed("src/libslic3r")
    g_total, g_only, g_diff = changed("src/slic3r")

    # "files": criados pelo fabricante; "shared_modified": existem no Filuna com conteudo diferente
    comps = {name: [] for name, _ in CLASSES}
    shared = {name: [] for name, _ in CLASSES}
    unclassified = 0
    for group, target in ((g_only, comps), (g_diff, shared)):
        for rel in group:
            stem = Path(rel).stem.lower()
            for name, rx in CLASSES:
                if re.search(rx, stem):
                    target[name].append(rel)
                    break
            else:
                unclassified += 1

    desc = {
        "network": "Descoberta em rede, conexao e envio",
        "control": "Controle, status e monitoramento",
        "camera": "Video do dispositivo",
        "colors": "Sistema de cores / multimaterial",
    }
    module = {
        "schema": 1,
        "id": ns.id,
        "status": "FONTE_FIXADA__INVENTARIO_PRELIMINAR_POR_NOME__MOTOR_NAO_COMPILADO__NAO_HOMOLOGADO",
        "rule": RULE,
        "source": {
            "repository": ns.repo,
            "ref": ns.ref,
            "commit": commit,
            "license": "AGPL-3.0 (conferir LICENSE da fonte antes de distribuir)",
            "official_slicer": ns.slicer,
            "trees": trees,
            "blobs": blobs,
        },
        "sparse_paths": SPARSE,
        "delta_vs_filuna": {
            "engine_source_files": e_total,
            "engine_only_in_oem": len(e_only),
            "engine_different": len(e_diff),
            "gui_source_files": g_total,
            "gui_only_in_oem": len(g_only),
            "gui_different": len(g_diff),
            "gui_changed_not_classified": unclassified,
        },
        "components": {
            "engine": {"description": "Motor de fatiamento e G-code do fabricante, clonado inteiro",
                       "trees": ["src/libslic3r"] if "src/libslic3r" in trees else []},
            "profiles": {"description": "Perfis oficiais de maquina, filamento e processo",
                         "trees": prof_trees, "files": prof_files},
            **{k: {"description": desc[k], "classification": "PRELIMINAR_POR_NOME_DE_ARQUIVO",
                   "files": v, "shared_modified": shared[k]}
               for k, v in comps.items()},
        },
        "pending": [
            "Revisar a classificacao preliminar de rede/controle/camera/cores lendo o codigo",
            "Compilar o motor (libslic3r pinado) sem interface no Windows",
            "Ligar a carga e a troca de modulo no Filuna (Dispositivos e Preparar)",
            "Prova de G-code identico: motor do modulo x fatiador oficial na mesma versao",
            "Prova de descoberta e conexao com o codigo clonado",
        ],
    }
    if ns.note:
        module["source"]["note"] = ns.note
    out = ns.filuna_root / "FILUNA/fabricantes" / ns.id / "module.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(module, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    d = module["delta_vs_filuna"]
    print(f"{ns.id}: commit={commit[:10]} trees={len(trees)} profiles={len(prof_trees)}+{len(prof_files)} "
          f"engine_changed={d['engine_only_in_oem'] + d['engine_different']}/{e_total} "
          f"gui_changed={len(g_only) + len(g_diff)}/{g_total} "
          + " ".join(f"{k}={len(v)}+{len(shared[k])}" for k, v in comps.items()) + f" unclassified={unclassified}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
