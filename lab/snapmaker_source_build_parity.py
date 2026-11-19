#!/usr/bin/env python3
"""Snapmaker U1 — paridade do portatil compilado da fonte AGPL vs portatil oficial.

Fatia o mesmo cubo de 20 mm com dois executaveis, usando presets oficiais U1
identicos (resolvidos de HEAD por snapmaker_u1_inherit), e compara o G-code
normalizando apenas a linha de data/gerador do cabecalho (mesma regra do
compare_oem_exact_sha). Nao altera presets nem G-code.

  --built-exe        snapmaker-orca.exe compilado da fonte pinada
  --official-cache   pasta de cache do portatil oficial (zip verificado por SHA)
  --out              pasta de saida (proof.json, G-codes, logs)

Saida: proof.json com SHA RAW e SHA normalizado dos dois, veredito e primeira
linha divergente. Sai !=0 se qualquer fatia falhar; o veredito de paridade
(IDENTICAL / DIVERGES) nao derruba o processo por si so — quem decide e o dono.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

from portable_first_qa import OFFICIAL, obtain, extract_executable
from snapmaker_u1_guard_probe import run_cli
from snapmaker_u1_inherit import build_effective_original_presets
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "FILUNA" / "qa" / "gcode"))
from compare_oem_exact_sha import TIMESTAMP  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
CUBE = ROOT / "fixtures" / "cube20.stl"


def normalized_lines(path: Path) -> list[bytes]:
    out = []
    with path.open("rb") as fp:
        for line in fp:
            if TIMESTAMP.match(line.rstrip(b"\r\n")):
                continue
            out.append(line.replace(b"\r\n", b"\n"))
    return out


def sha_raw(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sha_norm(path: Path) -> str:
    h = hashlib.sha256()
    for ln in normalized_lines(path):
        h.update(ln)
    return h.hexdigest()


def first_divergence(a: Path, b: Path):
    la, lb = normalized_lines(a), normalized_lines(b)
    for i, (x, y) in enumerate(zip(la, lb), 1):
        if x != y:
            return {"line": i,
                    "built": x.decode("utf-8", "replace").rstrip(),
                    "official": y.decode("utf-8", "replace").rstrip()}
    if len(la) != len(lb):
        return {"line": min(len(la), len(lb)) + 1,
                "built_lines": len(la), "official_lines": len(lb)}
    return None


def one_gcode(inv: dict, out: Path) -> Path | None:
    if inv["exit_code"] != 0 or len(inv["gcodes"]) != 1:
        return None
    return out / inv["gcodes"][0]["path"]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--built-exe", type=Path, required=True)
    ap.add_argument("--official-cache", type=Path, default=Path(".oem-portable-cache"))
    ap.add_argument("--out", type=Path, default=Path("qa-u1-source-parity"))
    ns = ap.parse_args()

    out = ns.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    built = ns.built_exe.resolve()
    if not built.is_file():
        print("BUILT_EXE_NOT_FOUND", built)
        return 2
    if not CUBE.is_file():
        print("CUBE_FIXTURE_MISSING", CUBE)
        return 2

    ns.official_cache.resolve().mkdir(parents=True, exist_ok=True)
    archive, url = obtain(OFFICIAL["snapmaker"], ns.official_cache.resolve())
    official = extract_executable(archive, OFFICIAL["snapmaker"]["exe"], ns.official_cache.resolve() / "extracted")

    presets = build_effective_original_presets(out / "presets")
    m, p, f = presets["machine"], presets["process"], presets["filament"]

    inv_built = run_cli(built, m, p, f, CUBE, out, "built-from-source", timeout=240, arrange=0)
    inv_official = run_cli(official, m, p, f, CUBE, out, "official-portable", timeout=240, arrange=0)

    g_built = one_gcode(inv_built, out)
    g_official = one_gcode(inv_official, out)

    proof: dict = {
        "fixture": "fixtures/cube20.stl single-material",
        "official_source_url": url,
        "official_exe": str(official.relative_to(ns.official_cache.resolve())),
        "built_exe_sha256": sha_raw(built),
        "slice_built": {k: inv_built[k] for k in ("exit_code", "gcodes")},
        "slice_official": {k: inv_official[k] for k in ("exit_code", "gcodes")},
    }

    if g_built and g_official:
        nb, no = sha_norm(g_built), sha_norm(g_official)
        proof["gcode_built_sha256_raw"] = sha_raw(g_built)
        proof["gcode_official_sha256_raw"] = sha_raw(g_official)
        proof["gcode_built_sha256_normalized"] = nb
        proof["gcode_official_sha256_normalized"] = no
        if nb == no:
            proof["verdict"] = "IDENTICAL_AFTER_HEADER_NORMALIZATION"
        else:
            proof["verdict"] = "DIVERGES"
            proof["first_divergence"] = first_divergence(g_built, g_official)
        code = 0
    else:
        proof["verdict"] = "SLICE_FAILED"
        code = 1

    (out / "proof.json").write_text(json.dumps(proof, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("PARITY_VERDICT=" + proof["verdict"])
    for k in ("gcode_built_sha256_normalized", "gcode_official_sha256_normalized"):
        if k in proof:
            print(k + "=" + proof[k])
    if proof.get("first_divergence"):
        print("FIRST_DIVERGENCE=" + json.dumps(proof["first_divergence"], ensure_ascii=False))
    return code


if __name__ == "__main__":
    sys.exit(main())
