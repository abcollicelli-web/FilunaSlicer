#!/usr/bin/env python3
"""Publica uma correção de telas do Filuna Slicer (HTML, JS e CSS) pelo catálogo assinado, sem passar pela loja.

O programa já traz as telas dentro dele, com uma versão (FilunaVersoes.hpp: "telas" 1.3.0, "inicio" 1.0.0). Uma correção
é a mesma geração com o último número maior (1.3.1, 1.3.2…) e entra no catálogo com o id da geração ("telas-1-3"):
só o programa daquela geração procura por ela, confere a assinatura Ed25519 do catálogo e o SHA-256 do pacote, e passa
a abrir as páginas corrigidas. O executável assinado pela loja não muda: o que chega é página.

As páginas saem de um commit do repositório do Filuna Slicer, então a correção é revisada no git antes de ir ao ar.

  sudo python3 publicar_telas.py --repo /tmp/tgc --ref <commit> --recurso telas --versao 1.3.1 \\
      --notas "Estoque: botão de escanear não respondia"

Recursos: "telas" (resources/web/filuna: Meu espaço, Estoque, Dispositivos, Farm, Modelos 3D) e "inicio" (a tela
Início: resources/web/homepage, o que ela usa de web/include e as imagens que ela cita).
Módulo novo nasce no canal beta do site (só administrador); liberar para todos é em /admin/modulos.
Recusa: versão de outra geração ou que não sobe, recurso sem arquivos. Outra publicação em andamento: saída 3.
"""
from __future__ import annotations

import argparse
import base64
import fcntl
import hashlib
import io
import json
import re
import subprocess
import sys
import tarfile
import time
import zipfile
from pathlib import Path

RECURSOS = {"telas": "Filuna Telas", "inicio": "Filuna Início"}
TEXTO = (".html", ".js", ".css")


def git(repo: Path, *args: str) -> bytes:
    r = subprocess.run(["git", "-C", str(repo), *args], capture_output=True)
    if r.returncode != 0:
        raise SystemExit(f"RECUSADO: git {' '.join(args[:2])}: {r.stderr.decode().strip()}")
    return r.stdout


def arvore(repo: Path, ref: str, *caminhos: str) -> dict[str, bytes]:
    """Arquivos do commit (sem tocar a pasta de trabalho), com o caminho inteiro a partir da raiz do repositório."""
    saida: dict[str, bytes] = {}
    with tarfile.open(fileobj=io.BytesIO(git(repo, "archive", ref, *caminhos))) as t:
        for m in t.getmembers():
            if m.isfile():
                saida[m.name] = t.extractfile(m).read()
    return saida


def arquivos_do_recurso(repo: Path, ref: str, recurso: str) -> dict[str, bytes]:
    if recurso == "telas":  # as páginas ficam na raiz do pacote, como o programa as procura
        base = "resources/web/filuna/"
        return {n[len(base):]: d for n, d in arvore(repo, ref, base.rstrip("/")).items()}
    # Início: a página usa ../include e ../../images, então o pacote repete a árvore de resources.
    arqs = arvore(repo, ref, "resources/web/homepage", "resources/web/include")
    citadas = set()
    for nome, dados in arqs.items():
        if nome.startswith("resources/web/homepage/") and nome.endswith(TEXTO):
            citadas.update(re.findall(r"\.\./\.\./images/([A-Za-z0-9_.-]+)", dados.decode("utf-8", "replace")))
    if citadas:
        arqs.update(arvore(repo, ref, *(f"resources/images/{n}" for n in sorted(citadas))))
    return {n[len("resources/"):]: d for n, d in arqs.items()}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--repo", type=Path, required=True, help="git do Filuna Slicer")
    ap.add_argument("--ref", required=True, help="commit com as páginas corrigidas")
    ap.add_argument("--recurso", choices=sorted(RECURSOS), required=True)
    ap.add_argument("--versao", required=True, help="mesma geração da embutida, com o último número maior (ex.: 1.3.1)")
    ap.add_argument("--notas", required=True, help="o que a correção muda, em uma frase")
    ap.add_argument("--destino", type=Path, default=Path("/var/lib/filuna-v2/modulos"))
    ap.add_argument("--chave", type=Path, default=Path("/root/.filuna-modulos/ed25519.pem"))
    ns = ap.parse_args()

    if not re.fullmatch(r"\d{1,4}\.\d{1,4}\.\d{1,4}", ns.versao):
        raise SystemExit("RECUSADO: a versão tem três números (ex.: 1.3.1)")
    nova = tuple(int(p) for p in ns.versao.split("."))
    cab = git(ns.repo, "show", f"{ns.ref}:src/slic3r/Filuna/FilunaVersoes.hpp").decode("utf-8")
    m = re.search(r'\{"%s",\s*"[^"]*",\s*"(\d+)\.(\d+)\.(\d+)"\}' % ns.recurso, cab)
    if not m:
        raise SystemExit(f"RECUSADO: o commit {ns.ref} não tem o recurso \"{ns.recurso}\" em FilunaVersoes.hpp (programa antigo)")
    embutida = tuple(int(x) for x in m.groups())
    if nova[:2] != embutida[:2] or nova[2] <= embutida[2]:
        raise SystemExit(f"RECUSADO: {ns.versao} não é correção da geração {embutida[0]}.{embutida[1]} "
                         f"(embutida no programa: {'.'.join(map(str, embutida))})")
    mid = f"{ns.recurso}-{embutida[0]}-{embutida[1]}"

    trava = open(ns.destino / ".publicar-telas.lock", "w")
    try:
        fcntl.flock(trava, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        print("RECUSADO: outra publicação de telas está em andamento", file=sys.stderr)
        return 3

    texto = (ns.destino / "catalogo.json").read_text(encoding="utf-8")
    catalogo = json.loads(texto)
    atual = next((x for x in catalogo["modulos"] if x["id"] == mid), None)
    if atual and tuple(int(p) for p in atual["versao"].split(".")) >= nova:
        raise SystemExit(f"RECUSADO: {ns.versao} não é maior que a publicada ({atual['versao']})")

    commit = git(ns.repo, "rev-parse", ns.ref).decode().strip()
    arquivos = arquivos_do_recurso(ns.repo, ns.ref, ns.recurso)
    if not arquivos:
        raise SystemExit(f"RECUSADO: o recurso {ns.recurso} não tem arquivos no commit {ns.ref}")

    pasta = ns.destino / mid / ns.versao
    pasta.mkdir(parents=True, exist_ok=True)
    pacote = pasta / f"{mid}-{ns.versao}.zip"
    # O programa só cria uma pasta quando o zip traz a entrada dela (extract_archive_confined).
    pastas = {nome[:i + 1] for nome in arquivos for i, letra in enumerate(nome) if letra == "/"}
    with zipfile.ZipFile(pacote, "w", zipfile.ZIP_DEFLATED) as z:
        for nome in sorted(pastas | set(arquivos)):  # data fixa: o mesmo commit dá sempre o mesmo pacote
            entrada = zipfile.ZipInfo(nome, date_time=(2026, 1, 1, 0, 0, 0))
            if nome in pastas:
                entrada.external_attr = (0o40755 << 16) | 0x10
                z.writestr(entrada, b"")
            else:
                z.writestr(entrada, arquivos[nome], zipfile.ZIP_DEFLATED)
    dados = pacote.read_bytes()
    modulo = {"id": mid, "nome": RECURSOS[ns.recurso], "versao": ns.versao, "estado": "HOMOLOGADO_INTERNO",
              "notas": ns.notas,
              "fonte": {"repositorio": "https://github.com/abcollicelli-web/FilunaSlicer", "ref": ns.ref, "commit": commit,
                        "licenca": "AGPL-3.0"},
              "componentes": [{"tipo": "recurso", "arquivo": pacote.name, "sha256": hashlib.sha256(dados).hexdigest(),
                               "bytes": len(dados), "url": f"/api/slicer/v1/modulos/{mid}/{ns.versao}/{pacote.name}"}],
              "homologacao": {"recurso": {"estado": "HOMOLOGADO", "nota": "páginas do próprio repositório, no commit indicado"}}}
    catalogo["modulos"] = [x for x in catalogo["modulos"] if x["id"] != mid] + [modulo]
    catalogo["gerado_em"] = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
    novo = json.dumps(catalogo, ensure_ascii=False, indent=1) + "\n"

    carimbo = time.strftime("%Y%m%dT%H%M%SZ", time.gmtime())
    for nome in ("catalogo.json", "catalogo.json.sig"):  # cópia do que estava no ar, para voltar se precisar
        (ns.destino / f"{nome}.antes-{carimbo}").write_bytes((ns.destino / nome).read_bytes())
    (ns.destino / "catalogo.json.novo").write_text(novo, encoding="utf-8")
    assinatura = subprocess.run(["openssl", "pkeyutl", "-sign", "-inkey", str(ns.chave), "-rawin", "-in",
                                 str(ns.destino / "catalogo.json.novo")], capture_output=True, check=True).stdout
    (ns.destino / "catalogo.json.sig.novo").write_text(base64.b64encode(assinatura).decode() + "\n", encoding="ascii")
    (ns.destino / "catalogo.json.sig.novo").replace(ns.destino / "catalogo.json.sig")
    (ns.destino / "catalogo.json.novo").replace(ns.destino / "catalogo.json")
    with open(ns.destino / "telas-publicacoes.log", "a", encoding="utf-8") as log:
        log.write(json.dumps({"em": catalogo["gerado_em"], "id": mid, "versao": ns.versao, "commit": commit,
                              "sha256": modulo["componentes"][0]["sha256"], "notas": ns.notas}, ensure_ascii=False) + "\n")
    print(f"publicado: {mid} {ns.versao} ({len(dados)} bytes, {len(arquivos)} arquivos, commit {commit[:9]}); "
          f"canal no site: beta até liberar em /admin/modulos")
    return 0


if __name__ == "__main__":
    sys.exit(main())
