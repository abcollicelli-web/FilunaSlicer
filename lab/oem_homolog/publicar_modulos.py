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
import re
import shutil
import subprocess
import tempfile
import zipfile
import sys
import time
from pathlib import Path

FABRICANTES = ("snapmaker", "bambu", "elegoo", "creality", "flashforge", "qidi", "anycubic", "prusa")
NOMES = {"snapmaker": "Snapmaker", "bambu": "Bambu Lab", "elegoo": "Elegoo", "creality": "Creality",
         "flashforge": "Flashforge", "qidi": "QIDI", "anycubic": "Anycubic", "prusa": "Prusa"}
ACEITOS_CASO = ("IDENTICO", "IDENTICO_POR_CAMADA", "MESMA_RECUSA_DO_OFICIAL")


def sha(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def estado_motor(prova: Path, modelo: str) -> dict:
    """Paridade do G-code por caso da régua ('cubo' = motor, 'cores' = sistema de cores)."""
    if not prova.is_file():
        return {"estado": "PENDENTE", "motivo": "sem prova de paridade do binário compilado"}
    d = json.loads(prova.read_text(encoding="utf-8"))
    if d.get("oficial") == d.get("compilado"):
        return {"estado": "PENDENTE", "motivo": "só régua oficial × oficial"}
    resumo, n, trocas = {}, 0, 0
    for r in d.get("resultados", []):
        casos = r.get("casos") or []
        if r.get("veredito") == "PERFIL_INCOMPLETO" and not casos:
            resumo["PERFIL_INCOMPLETO_DO_FABRICANTE"] = resumo.get("PERFIL_INCOMPLETO_DO_FABRICANTE", 0) + 1
            continue
        for c in casos:
            # variantes do caso contam no mesmo grupo ("cores-sem-torre" é sistema de cores)
            if (c.get("modelo") or "").split("-")[0] != modelo:
                continue
            n += 1
            resumo[c["veredito"]] = resumo.get(c["veredito"], 0) + 1
            if (c.get("trocas_ferramenta") or 0) > 2:
                trocas += 1
    if not n:
        return {"estado": "PENDENTE", "motivo": f"sem caso '{modelo}' na prova"}
    ok = sum(v for k, v in resumo.items() if k in ACEITOS_CASO)
    estado = "HOMOLOGADO" if ok == n else "REPROVADO"
    if modelo == "cores" and estado == "HOMOLOGADO" and trocas == 0:
        # o caso de cores só prova o sistema de cores se o oficial de fato trocou de filamento
        estado = "PENDENTE"
    out = {"estado": estado, "casos": n, "aceitos": ok, "resumo": resumo, "prova_sha256": sha(prova)}
    if modelo == "cores":
        out["maquinas_com_troca_de_cor"] = trocas
        if estado == "PENDENTE":
            out["motivo"] = "a fixture de 4 cores não gerou troca de filamento neste fatiador"
    return out


def estado_rede(provas: Path, fid: str) -> dict:
    """Junta as sondas de rede da marca (um protocolo por pasta rede-<id>[-protocolo])."""
    pastas = sorted(p for p in provas.glob(f"rede-{fid}*") if (p / "prova.json").is_file())
    if not pastas:
        return {"estado": "PENDENTE", "motivo": "sem prova de rede"}
    protocolos, ok = {}, True
    for p in pastas:
        d = json.loads((p / "prova.json").read_text(encoding="utf-8"))
        protocolos[d.get("agente", p.name)] = {"aprovado": bool(d.get("aprovado")), "etapas": len(d.get("etapas", [])),
                                               "nao_suportado_pelo_protocolo": d.get("nao_suportado_pelo_protocolo", [])}
        ok &= bool(d.get("aprovado"))
    return {"estado": "HOMOLOGADO" if ok else "REPROVADO", "protocolos": protocolos,
            "nota": "ensaio contra emulador do protocolo; impressora real fica para o teste do proprietário"}



# Recursos do próprio Slicer servidos pelo mesmo catálogo (Fase 7): id -> (nome, pasta no fork).
# O id é o mesmo do motor em FilunaVersoes.hpp; a versão vem de lá, no mesmo ref.
RECURSOS_SLICER = {"telas": ("Filuna Telas", "resources/web/filuna")}


def versao_motor(repo: Path, ref: str, motor: str) -> str | None:
    try:
        txt = subprocess.run(["git", "-C", str(repo), "show", f"{ref}:src/slic3r/Filuna/FilunaVersoes.hpp"],
                             capture_output=True, check=True, text=True).stdout
    except subprocess.CalledProcessError:
        return None
    m = re.search(r'\{"%s",\s*"[^"]*",\s*"([^"]+)"\}' % re.escape(motor), txt)
    return m.group(1) if m else None


def recursos_slicer(repo: Path, ref: str, destino: Path) -> list[dict]:
    """Empacota os recursos do Slicer (ex.: telas) a partir do git do fork. O zip leva os
    arquivos na raiz, como o FilunaRecursos espera em <dados>/filuna/recursos/<id>/<versao>/.
    Datas fixas no zip: o pacote só muda quando o conteúdo muda."""
    mods = []
    commit = subprocess.run(["git", "-C", str(repo), "rev-parse", ref],
                            capture_output=True, check=True, text=True).stdout.strip()
    for rid, (nome, subdir) in RECURSOS_SLICER.items():
        versao = versao_motor(repo, ref, rid)
        if not versao:
            print(f"AVISO {rid}: versao nao encontrada em FilunaVersoes.hpp ({ref})", file=sys.stderr)
            continue
        with tempfile.TemporaryDirectory() as td:
            tar = subprocess.run(["git", "-C", str(repo), "archive", ref, subdir],
                                 capture_output=True, check=True).stdout
            subprocess.run(["tar", "-x", "-C", td], input=tar, check=True)
            raiz = Path(td) / subdir
            arquivos = sorted(q for q in raiz.rglob("*") if q.is_file())
            if not arquivos:
                print(f"AVISO {rid}: {subdir} vazio em {ref}", file=sys.stderr)
                continue
            alvo = destino / rid / versao
            alvo.mkdir(parents=True, exist_ok=True)
            pacote = alvo / f"{rid}-{versao}.zip"
            with zipfile.ZipFile(pacote, "w") as z:
                for q in arquivos:
                    z.writestr(zipfile.ZipInfo(str(q.relative_to(raiz))), q.read_bytes(),
                               zipfile.ZIP_DEFLATED)
        mods.append({"id": rid, "nome": nome, "versao": versao, "estado": "HOMOLOGADO_INTERNO",
                     "fonte": {"repositorio": "https://github.com/abcollicelli-web/FilunaSlicer",
                               "ref": ref, "commit": commit, "licenca": "AGPL-3.0"},
                     "componentes": [{"tipo": "recurso", "arquivo": pacote.name, "sha256": sha(pacote),
                                      "bytes": pacote.stat().st_size,
                                      "url": f"/api/slicer/v1/modulos/{rid}/{versao}/{pacote.name}"}],
                     "homologacao": {"recurso": {"estado": "HOMOLOGADO",
                                                 "nota": "arquivos do proprio fork, os mesmos do instalador"}}})
    return mods


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", type=Path, required=True)
    ap.add_argument("--artefatos", type=Path, required=True)
    ap.add_argument("--provas", type=Path, required=True)
    ap.add_argument("--destino", type=Path, required=True)
    ap.add_argument("--chave", type=Path, required=True)
    ap.add_argument("--slicer-repo", type=Path, help="git do fork FilunaSlicer para empacotar recursos (telas)")
    ap.add_argument("--slicer-ref", default="FETCH_HEAD")
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
            "motor": estado_motor(ns.provas / f"paridade-{fid}" / "prova.json", "cubo"),
            "perfis": ({"estado": "HOMOLOGADO" if perfis["fonte_igual_oficial"] else "HOMOLOGADO_DO_BINARIO_OFICIAL",
                        "nota": None if perfis["fonte_igual_oficial"] else
                        "perfis publicados na fonte diferem do binário oficial; o pacote leva os do binário oficial"}
                       if perfis else {"estado": "PENDENTE"}),
            "cores": estado_motor(ns.provas / f"paridade-{fid}" / "prova.json", "cores"),
            "rede": estado_rede(ns.provas, fid),
            "preparar_enviar_controlar": estado_rede(ns.provas, fid),
            "impressao_fisica": {"estado": "PENDENTE_TESTE_DO_PROPRIETARIO"},
        }
        bloqueio = mod.get("homologation_block")
        if bloqueio:
            # motor/cores não têm como provar identidade: a fonte publicada não é a do binário oficial.
            # O resultado medido fica junto, para ninguém confundir bloqueio com falta de teste.
            for k in ("motor", "cores"):
                homolog[k] = {"estado": bloqueio["estado"], "motivo": bloqueio["motivo"], "medido": homolog[k]}
        internos = [v["estado"] for k, v in homolog.items() if k != "impressao_fisica"]
        estado = ("HOMOLOGADO_INTERNO" if all(e.startswith("HOMOLOGADO") for e in internos)
                  else bloqueio["estado"] if bloqueio else "EM_TESTE")
        modulos.append({
            "id": fid, "nome": NOMES[fid], "versao": versao, "estado": estado,
            "fonte": {"repositorio": src.get("repository"), "ref": src.get("tag") or src.get("branch"),
                      "commit": src.get("commit"), "licenca": src.get("license")},
            "conectividade": mod.get("connectivity", {}).get("path"),
            "componentes": componentes, "homologacao": homolog})

    if ns.slicer_repo:
        modulos += recursos_slicer(ns.slicer_repo, ns.slicer_ref, ns.destino)

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
