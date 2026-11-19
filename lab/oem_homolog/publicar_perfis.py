#!/usr/bin/env python3
"""Publica uma correção de perfis (impressora, filamento, processo) de um fabricante, assinada pela Filuna.

O Filuna Slicer consulta https://filuna3d.com/api/slicer/v1/perfis?vendor=<Fabricante>, confere a assinatura
Ed25519 do documento (chave pública fixa no programa) e o SHA-256 do pacote, e só então troca os perfis. O programa
instalado (inclusive o da Microsoft Store) não muda: o que chega é dado.

Os perfis saem de um commit do repositório do Filuna Slicer (resources/profiles/<Fabricante>.json e a pasta dele),
então a correção é revisada e homologada no git antes de ir ao ar.

  sudo python3 publicar_perfis.py --repo /srv/filuna-homolog/repo --ref <commit> --fabricante BBL \\
      --versao 02.08.00.17 --notas "A1: temperatura da primeira camada do PETG" --canal beta

Canal "beta": só a conta de administrador recebe (para testar). Canal "todos": todos os clientes.
Recusa (saída 4): versão que não sobe, perfil que não é JSON válido, fabricante sem pasta.
Outra publicação em andamento: saída 3. Registro em <destino>/perfis/publicacoes.log.
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
import zipfile
from datetime import datetime, timezone
from pathlib import Path

CANAIS = ("beta", "todos")


def versao_tupla(v: str) -> tuple[int, ...]:
    if not re.fullmatch(r"\d{1,4}(\.\d{1,4}){2,3}", v):
        raise SystemExit(f"RECUSADO: versão '{v}' inválida (use números, ex.: 02.08.00.17)")
    return tuple(int(p) for p in v.split("."))


def arquivos_do_commit(repo: Path, ref: str, fabricante: str) -> dict[str, bytes]:
    """Lê do git (sem tocar a pasta de trabalho) o <Fabricante>.json e a pasta <Fabricante>/ de resources/profiles."""
    base = "resources/profiles"
    tar = subprocess.run(["git", "-C", str(repo), "archive", ref, f"{base}/{fabricante}.json", f"{base}/{fabricante}"],
                         capture_output=True)
    if tar.returncode != 0:
        raise SystemExit(f"RECUSADO: {fabricante} não existe em {base} no commit {ref}: {tar.stderr.decode().strip()}")
    saida: dict[str, bytes] = {}
    with tarfile.open(fileobj=io.BytesIO(tar.stdout)) as t:
        for m in t.getmembers():
            if m.isfile():
                saida[m.name[len(base) + 1:]] = t.extractfile(m).read()
    return saida


def conferir(arquivos: dict[str, bytes], fabricante: str) -> None:
    indice = json.loads(arquivos[f"{fabricante}.json"])
    faltando = []
    for lista in ("machine_model_list", "machine_list", "filament_list", "process_list"):
        for item in indice.get(lista, []):
            if f"{fabricante}/{item['sub_path']}" not in arquivos:
                faltando.append(item["sub_path"])
    if faltando:
        raise SystemExit(f"RECUSADO: o índice cita arquivos que não existem: {faltando[:5]}")
    for nome, dados in arquivos.items():
        if nome.endswith(".json"):
            try:
                json.loads(dados)
            except ValueError as e:
                raise SystemExit(f"RECUSADO: {nome} não é JSON válido: {e}") from e


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--repo", type=Path, required=True, help="git do Filuna Slicer")
    ap.add_argument("--ref", required=True, help="commit com os perfis corrigidos")
    ap.add_argument("--fabricante", required=True, help="pasta em resources/profiles (BBL, Elegoo, Anycubic…)")
    ap.add_argument("--versao", required=True, help="versão dos perfis; tem de ser maior que a publicada")
    ap.add_argument("--notas", required=True, help="o que a correção muda, em uma frase para o cliente")
    ap.add_argument("--canal", choices=CANAIS, default="beta")
    ap.add_argument("--destino", type=Path, default=Path("/var/lib/filuna-v2/modulos"))
    ap.add_argument("--chave", type=Path, default=Path("/root/.filuna-modulos/ed25519.pem"))
    ns = ap.parse_args()

    fab = ns.fabricante
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9 _-]{0,39}", fab):
        raise SystemExit("RECUSADO: nome de fabricante inválido")
    nova = versao_tupla(ns.versao)
    raiz = ns.destino / "perfis"
    raiz.mkdir(parents=True, exist_ok=True)
    trava = open(raiz / ".publicar.lock", "w")
    try:
        fcntl.flock(trava, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        print("RECUSADO: outra publicação de perfis está em andamento", file=sys.stderr)
        return 3

    commit = subprocess.run(["git", "-C", str(ns.repo), "rev-parse", ns.ref], capture_output=True, text=True).stdout.strip()
    arquivos = arquivos_do_commit(ns.repo, ns.ref, fab)
    conferir(arquivos, fab)

    indice = json.loads(arquivos[f"{fab}.json"])
    base = versao_tupla(str(indice.get("version", "0.0.0")))
    if nova <= base:
        raise SystemExit(f"RECUSADO: {ns.versao} não é maior que a versão do commit ({indice.get('version')}); o programa não trocaria")
    for canal in CANAIS:
        atual = raiz / fab / f"{canal}.json"
        if atual.is_file():
            publicada = json.loads(atual.read_text(encoding="utf-8"))["versao"]
            # no canal pedido a versão tem de subir; promover a mesma versão de beta para todos é permitido
            if nova < versao_tupla(publicada) or (nova == versao_tupla(publicada) and canal == ns.canal):
                raise SystemExit(f"RECUSADO: {ns.versao} não é maior que a publicada no canal {canal} ({publicada})")

    # A versão nova e a instalação sem pergunta vão no índice; o resto é o commit, byte a byte.
    indice["version"], indice["force_update"] = ns.versao, "1"
    arquivos[f"{fab}.json"] = (json.dumps(indice, ensure_ascii=False, indent=4) + "\n").encode("utf-8")

    pasta = raiz / fab / ns.versao
    pasta.mkdir(parents=True, exist_ok=True)
    pacote = pasta / f"{fab}.zip"
    if not pacote.is_file():  # promoção de beta para todos reaproveita o mesmo pacote (mesmo SHA-256)
        # O programa só cria uma pasta quando o zip traz a entrada dela (extract_archive_confined): sem as entradas
        # "Fabricante/", "Fabricante/machine/"… a extração para no primeiro arquivo de dentro da pasta.
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
    doc = {"tipo": "perfis", "fabricante": fab, "versao": ns.versao, "sha256": hashlib.sha256(dados).hexdigest(),
           "bytes": len(dados), "url": f"/api/slicer/v1/perfis/{fab}/{ns.versao}/{fab}.zip", "notas": ns.notas,
           "commit": commit, "chave": "filuna-modulos-ed25519-1",
           "gerado_em": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")}
    texto = json.dumps(doc, ensure_ascii=False, sort_keys=True)
    assinar = pasta / "documento.json"
    assinar.write_text(texto, encoding="utf-8")
    assinatura = subprocess.run(["openssl", "pkeyutl", "-sign", "-inkey", str(ns.chave), "-rawin", "-in", str(assinar)],
                                capture_output=True, check=True).stdout
    sig = base64.b64encode(assinatura).decode() + "\n"
    # "todos" também atualiza o beta: o administrador nunca fica com versão mais velha que a dos clientes
    for canal in (CANAIS if ns.canal == "todos" else ("beta",)):
        for nome, conteudo in ((f"{canal}.json.novo", texto), (f"{canal}.json.sig.novo", sig)):
            (raiz / fab / nome).write_text(conteudo, encoding="utf-8")
        (raiz / fab / f"{canal}.json.sig.novo").replace(raiz / fab / f"{canal}.json.sig")
        (raiz / fab / f"{canal}.json.novo").replace(raiz / fab / f"{canal}.json")
    with open(raiz / "publicacoes.log", "a", encoding="utf-8") as log:
        log.write(json.dumps({"em": doc["gerado_em"], "fabricante": fab, "versao": ns.versao, "canal": ns.canal,
                              "commit": commit, "sha256": doc["sha256"], "notas": ns.notas}, ensure_ascii=False) + "\n")
    print(f"publicado: {fab} {ns.versao} no canal {ns.canal} ({len(dados)} bytes, {len(arquivos)} arquivos, commit {commit[:9]})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
