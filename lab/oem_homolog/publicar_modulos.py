#!/usr/bin/env python3
"""Filuna — monta o catálogo assinado dos módulos de fabricante servidos sob demanda.

Lê, para cada fabricante, o module.json do repositório, o pacote de perfis gerado por
pacote_perfis.py, o motor empacotado (quando houver) e as provas (paridade de G-code e
rede). Copia os artefatos para a pasta que o site serve e grava catalogo.json + assinatura
Ed25519 (catalogo.json.sig, base64). O estado de cada componente vem SÓ das provas: sem
prova, o componente fica PENDENTE. Impressão física fica sempre a cargo do proprietário.

Uso (no servidor):
  publicar_modulos.py --repo /srv/filuna-homolog/repo --artefatos /srv/filuna-homolog/modulos \
      --provas /srv/filuna-homolog/res --destino /var/lib/filuna-v2/modulos \
      --chave /root/.filuna-modulos/ed25519.pem
"""
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import shutil
import subprocess
import sys
import time
from pathlib import Path

FABRICANTES = ("snapmaker", "bambu", "elegoo", "creality", "flashforge", "qidi", "anycubic", "prusa")
NOMES = {"snapmaker": "Snapmaker", "bambu": "Bambu Lab", "elegoo": "Elegoo", "creality": "Creality",
         "flashforge": "Flashforge", "qidi": "QIDI", "anycubic": "Anycubic", "prusa": "Prusa"}
ACEITOS_MOTOR = ("IDENTICO", "IDENTICO_COM_RECUSA_OFICIAL")


def sha(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def estado_motor(prova: Path) -> dict:
    if not prova.is_file():
        return {"estado": "PENDENTE", "motivo": "sem prova de paridade do binário compilado"}
    d = json.loads(prova.read_text(encoding="utf-8"))
    if d.get("oficial") == d.get("compilado"):
        return {"estado": "PENDENTE", "motivo": "só régua oficial × oficial", "resumo": d.get("resumo")}
    resumo = d.get("resumo", {})
    total = sum(resumo.values())
    bons = sum(v for k, v in resumo.items() if k in ACEITOS_MOTOR)
    return {"estado": "HOMOLOGADO" if total and bons == total else "REPROVADO", "impressoras": total,
            "aceitas": bons, "resumo": resumo, "prova_sha256": sha(prova)}


def estado_simples(prova: Path, chave_ok: str = "aprovado") -> dict:
    if not prova.is_file():
        return {"estado": "PENDENTE", "motivo": "sem prova"}
    d = json.loads(prova.read_text(encoding="utf-8"))
    return {"estado": "HOMOLOGADO" if d.get(chave_ok) else "REPROVADO", "resumo": d.get("resumo"),
            "prova_sha256": sha(prova)}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", type=Path, required=True)
    ap.add_argument("--artefatos", type=Path, required=True)
    ap.add_argument("--provas", type=Path, required=True)
    ap.add_argument("--destino", type=Path, required=True)
    ap.add_argument("--chave", type=Path, required=True)
    ns = ap.parse_args()

    ns.destino.mkdir(parents=True, exist_ok=True)
    modulos = []
    for fid in FABRICANTES:
        mj = ns.repo / "FILUNA/fabricantes" / fid / "module.json"
        mod = json.loads(mj.read_text(encoding="utf-8")) if mj.is_file() else {}
        src = mod.get("source", {})
        art = ns.artefatos / fid
        componentes = []
        versao = None
        for man in sorted(art.glob("*.manifest.json")) if art.is_dir() else []:
            m = json.loads(man.read_text(encoding="utf-8"))
            pacote = art / m["pacote"]
            if not pacote.is_file() or sha(pacote) != m["pacote_sha256"]:
                print(f"AVISO {fid}: pacote {m['pacote']} ausente ou com hash diferente do manifesto", file=sys.stderr)
                continue
            versao = versao or m["versao"]
            alvo = ns.destino / fid / m["versao"]
            alvo.mkdir(parents=True, exist_ok=True)
            shutil.copy2(pacote, alvo / pacote.name)
            componentes.append({
                "tipo": m["componente"], "arquivo": pacote.name, "sha256": m["pacote_sha256"],
                "bytes": m["pacote_bytes"], "url": f"/api/slicer/v1/modulos/{fid}/{m['versao']}/{pacote.name}",
                "fonte_igual_oficial": m.get("fonte_igual_oficial"),
                "impressoras": [i["maquina"] for i in m.get("impressoras", []) if not i.get("erro")]})
        perfis = next((c for c in componentes if c["tipo"] == "profiles"), None)
        homolog = {
            "motor": estado_motor(ns.provas / f"paridade-{fid}" / "prova.json"),
            "perfis": ({"estado": "HOMOLOGADO" if perfis["fonte_igual_oficial"] else "HOMOLOGADO_DO_BINARIO_OFICIAL",
                        "nota": None if perfis["fonte_igual_oficial"] else
                        "perfis publicados na fonte diferem do binário oficial; o pacote leva os do binário oficial"}
                       if perfis else {"estado": "PENDENTE"}),
            "cores": estado_motor(ns.provas / f"paridade-{fid}" / "prova.json") | {"nota": "caso de 4 cores da régua"},
            "rede": estado_simples(ns.provas / f"rede-{fid}" / "prova.json"),
            "preparar_enviar_controlar": estado_simples(ns.provas / f"rede-{fid}" / "prova.json"),
            "impressao_fisica": {"estado": "PENDENTE_TESTE_DO_PROPRIETARIO"},
        }
        internos = [v["estado"] for k, v in homolog.items() if k != "impressao_fisica"]
        estado = "HOMOLOGADO_INTERNO" if all(e.startswith("HOMOLOGADO") for e in internos) else "EM_TESTE"
        modulos.append({
            "id": fid, "nome": NOMES[fid], "versao": versao, "estado": estado,
            "fonte": {"repositorio": src.get("repository"), "ref": src.get("tag") or src.get("branch"),
                      "commit": src.get("commit"), "licenca": src.get("license")},
            "conectividade": mod.get("connectivity", {}).get("path"),
            "componentes": componentes, "homologacao": homolog})

    catalogo = {"schema": 1, "gerado_em": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "chave": "filuna-modulos-ed25519-1", "modulos": modulos}
    dados = (json.dumps(catalogo, ensure_ascii=False, indent=1) + "\n").encode("utf-8")
    (ns.destino / "catalogo.json").write_bytes(dados)
    assin = subprocess.run(["openssl", "pkeyutl", "-sign", "-inkey", str(ns.chave), "-rawin",
                            "-in", str(ns.destino / "catalogo.json")], capture_output=True, check=True).stdout
    (ns.destino / "catalogo.json.sig").write_text(base64.b64encode(assin).decode() + "\n", encoding="ascii")
    for m in modulos:
        print(f"{m['id']:11} {m['estado']:18} " + " ".join(f"{k}={v['estado']}" for k, v in m["homologacao"].items()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
