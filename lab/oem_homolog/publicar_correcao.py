#!/usr/bin/env python3
"""Publica uma correção de DEFEITO no código do Filuna Slicer (OrcaSlicer.dll) para a versão da Microsoft Store.

O pacote da loja não muda. A DLL corrigida vai para uma pasta do usuário e o lançador do pacote só a carrega se o
documento tiver a assinatura ECDSA P-256 da Filuna, for da mesma compilação do pacote, estiver dentro da validade e o
SHA-256 da DLL bater (src/FilunaCorrecao.hpp). Esta ferramenta é a porteira: ela aplica as regras de
FILUNA/ATUALIZACAO-REMOTA.md e recusa o que tem de ir pela loja.

  sudo python3 publicar_correcao.py --repo /tmp/tgc --base <commit da versão que está na loja> --ref <commit da correção> \\
      --artefato <pasta do executável compilado do --ref> --versao 1 --motivo "Queda ao escolher a mesma impressora" --canal beta

Regras (recusa com o motivo):
  - a correção parte da versão da loja (--base é ancestral de --ref) e já está no ramo do cliente (vai na próxima versão);
  - só código do programa (src/); recurso, dependência, empacotamento ou número de versão mudam pela loja;
  - áreas que nunca vêm por aqui: lançador e conferidor, atualização e assinaturas, sincronização da conta;
  - nenhum endereço de internet novo no código;
  - tamanho de correção: até 15 arquivos e 400 linhas mexidas;
  - validade de no máximo 30 dias; versão sobe; a DLL é a compilada do --ref.
Canal "beta": só o administrador recebe. Canal "todos": todos os clientes daquela compilação.
"""
from __future__ import annotations

import argparse
import fcntl
import hashlib
import json
import re
import subprocess
import sys
import zipfile
from datetime import datetime, timedelta, timezone
from pathlib import Path

CANAIS = ("beta", "todos")
MAX_ARQUIVOS, MAX_LINHAS, MAX_DIAS = 15, 400, 30
RAMO_DO_CLIENTE = "origin/filuna/v1-teste-fisico"

# Mexeu aqui, vai pela loja: quem decide que código roda, quem confere assinatura e o que sai da máquina do cliente.
SO_PELA_LOJA = (
    r"src/OrcaSlicer_app_msvc\.cpp", r"src/FilunaCorrecao\.hpp",                        # lançador e conferidor
    r"src/slic3r/GUI/FilunaManutencao\.", r"src/slic3r/GUI/FilunaAtualizacao\.",         # atualização
    r"src/slic3r/Filuna/FilunaRecursos\.", r"src/slic3r/Utils/FilunaModuleStore\.",      # assinatura e módulos
    r"src/slic3r/Utils/PresetUpdater\.", r"src/libslic3r/AppConfig\.",                   # perfis assinados e endereços
    r"src/slic3r/GUI/FilunaSync\.",                                                      # o que sai da conta do cliente
    r"src/slic3r/Filuna/FilunaVersoes\.hpp",                                             # geração das telas
)
# Fora de src/, isto muda o pacote: só pela loja. O resto fora de src/ (lab/, FILUNA/, docs) não vai no programa.
MUDA_O_PACOTE = (r"resources/", r"deps/", r"deps_src/", r"cmake/", r"CMakeLists\.txt$", r"version\.inc$", r"build_win\.bat$",
                 r"\.github/workflows/", r".*CMakeLists\.txt$")


def git(repo: Path, *args: str, ok: bool = True) -> subprocess.CompletedProcess:
    r = subprocess.run(["git", "-C", str(repo), *args], capture_output=True, text=True)
    if ok and r.returncode != 0:
        raise SystemExit(f"RECUSADO: git {' '.join(args[:2])}: {r.stderr.strip()}")
    return r


def der_para_rs(der: bytes) -> str:
    """Assinatura do openssl (DER: SEQUENCE{INTEGER r, INTEGER s}) -> R e S de 32 bytes em hexadecimal."""
    i = 2 + (der[1] & 0x7F if der[1] & 0x80 else 0)
    partes = []
    while i < len(der):
        n = der[i + 1]
        partes.append(int.from_bytes(der[i + 2:i + 2 + n], "big"))
        i += 2 + n
    return partes[0].to_bytes(32, "big").hex() + partes[1].to_bytes(32, "big").hex()


def conferir_regras(repo: Path, base: str, ref: str) -> dict:
    if git(repo, "merge-base", "--is-ancestor", base, ref, ok=False).returncode != 0:
        raise SystemExit("RECUSADO: a correção não parte da versão que está na loja (--base não é ancestral de --ref)")
    if git(repo, "merge-base", "--is-ancestor", ref, RAMO_DO_CLIENTE, ok=False).returncode != 0:
        raise SystemExit(f"RECUSADO: o commit da correção ainda não está em {RAMO_DO_CLIENTE}; sem isso ela se perde na próxima versão da loja")
    linhas = [l.split("\t") for l in git(repo, "diff", "--numstat", f"{base}..{ref}").stdout.splitlines() if l.strip()]
    no_programa, mexidas = [], 0
    for mais, menos, nome in linhas:
        if any(re.match(p, nome) for p in MUDA_O_PACOTE):
            raise SystemExit(f"RECUSADO: {nome} muda o pacote (recurso, dependência, empacotamento ou versão): vai pela loja")
        if not nome.startswith("src/"):
            continue  # lab/, FILUNA/, documentação: não entram no programa
        if any(re.match(p, nome) for p in SO_PELA_LOJA):
            raise SystemExit(f"RECUSADO: {nome} é área que só muda pela loja (atualização, assinatura, lançador ou conta)")
        if mais == "-":
            raise SystemExit(f"RECUSADO: {nome} é arquivo binário dentro de src/")
        no_programa.append(nome)
        mexidas += int(mais) + int(menos)
    if not no_programa:
        raise SystemExit("RECUSADO: não há mudança de código do programa entre --base e --ref")
    if len(no_programa) > MAX_ARQUIVOS or mexidas > MAX_LINHAS:
        raise SystemExit(f"RECUSADO: {len(no_programa)} arquivos e {mexidas} linhas mexidas passam do tamanho de uma correção "
                         f"(até {MAX_ARQUIVOS} arquivos e {MAX_LINHAS} linhas): vai pela loja")
    antes = set(re.findall(r"https?://[A-Za-z0-9.-]+", git(repo, "grep", "-hoE", r"https?://[A-Za-z0-9.-]+", base, "--", "src").stdout))
    novos = set()
    for l in git(repo, "diff", "-U0", f"{base}..{ref}", "--", "src").stdout.splitlines():
        if l.startswith("+") and not l.startswith("+++"):
            novos.update(u for u in re.findall(r"https?://[A-Za-z0-9.-]+", l) if u not in antes)
    if novos:
        raise SystemExit(f"RECUSADO: a correção traz endereço de internet novo ({', '.join(sorted(novos))}): vai pela loja")
    return {"arquivos": no_programa, "linhas": mexidas}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--repo", type=Path, required=True, help="git do Filuna Slicer")
    ap.add_argument("--base", required=True, help="commit da versão que está na loja (o do filuna-build.json do pacote)")
    ap.add_argument("--ref", required=True, help="commit da correção")
    ap.add_argument("--artefato", type=Path, required=True, help="pasta do executável compilado do --ref (OrcaSlicer.dll e filuna-build.json)")
    ap.add_argument("--versao", type=int, required=True, help="1, 2, 3… dentro da mesma versão da loja")
    ap.add_argument("--motivo", required=True, help="o defeito que a correção resolve, em uma frase")
    ap.add_argument("--validade-dias", type=int, default=MAX_DIAS)
    ap.add_argument("--canal", choices=CANAIS, default="beta")
    ap.add_argument("--destino", type=Path, default=Path("/var/lib/filuna-v2/correcao"))
    ap.add_argument("--chave", type=Path, default=Path("/root/.filuna-correcao/p256.pem"))
    ns = ap.parse_args()

    if not 1 <= ns.validade_dias <= MAX_DIAS:
        raise SystemExit(f"RECUSADO: a validade é de 1 a {MAX_DIAS} dias; depois disso a correção tem de estar na versão da loja")
    if ns.versao < 1 or len(ns.motivo.strip()) < 10:
        raise SystemExit("RECUSADO: informe a versão (1 em diante) e o defeito corrigido")
    base = git(ns.repo, "rev-parse", ns.base).stdout.strip()
    ref = git(ns.repo, "rev-parse", ns.ref).stdout.strip()
    build = base[:9]
    regras = conferir_regras(ns.repo, base, ref)

    dll = ns.artefato / "OrcaSlicer.dll"
    try:
        feito_de = json.loads((ns.artefato / "filuna-build.json").read_text(encoding="utf-8")).get("build")
    except (OSError, ValueError):
        feito_de = None
    if feito_de != ref[:9]:
        raise SystemExit(f"RECUSADO: o artefato foi compilado de {feito_de}, não do commit da correção ({ref[:9]})")
    dados = dll.read_bytes() if dll.is_file() else b""
    if dados[:2] != b"MZ" or len(dados) < 10_000_000:
        raise SystemExit("RECUSADO: OrcaSlicer.dll ausente ou não é uma DLL do programa")
    sha = hashlib.sha256(dados).hexdigest()

    raiz = ns.destino / build
    raiz.mkdir(parents=True, exist_ok=True)
    trava = open(ns.destino / ".publicar.lock", "w")
    try:
        fcntl.flock(trava, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        print("RECUSADO: outra publicação de correção está em andamento", file=sys.stderr)
        return 3
    for canal in CANAIS:
        atual = raiz / f"{canal}.txt"
        if atual.is_file():
            publicada = int(re.search(r"^versao=(\d+)$", atual.read_text(encoding="utf-8"), re.M).group(1))
            # no canal pedido a versão tem de subir; levar a mesma versão do beta para todos é permitido
            if ns.versao < publicada or (ns.versao == publicada and canal == ns.canal):
                raise SystemExit(f"RECUSADO: a versão {ns.versao} não é maior que a publicada no canal {canal} ({publicada})")

    pasta = raiz / str(ns.versao)
    pasta.mkdir(exist_ok=True)
    pacote, doc_arq, sig_arq = pasta / "OrcaSlicer.zip", pasta / "correcao.txt", pasta / "correcao.sig"
    if doc_arq.is_file():  # levar do beta para todos: o mesmo documento, a mesma DLL
        if f"dll_sha256={sha}\n" not in doc_arq.read_text(encoding="utf-8"):
            raise SystemExit(f"RECUSADO: a versão {ns.versao} já foi publicada com outra DLL; use a versão seguinte")
    else:
        with zipfile.ZipFile(pacote, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
            z.writestr(zipfile.ZipInfo("OrcaSlicer.dll", date_time=(2026, 1, 1, 0, 0, 0)), dados, zipfile.ZIP_DEFLATED)
        validade = (datetime.now(timezone.utc) + timedelta(days=ns.validade_dias)).strftime("%Y-%m-%d")
        doc_arq.write_text(f"filuna-correcao-1\nbuild={build}\ndll_sha256={sha}\nversao={ns.versao}\nvalidade={validade}\n", encoding="ascii", newline="\n")
        der = subprocess.run(["openssl", "dgst", "-sha256", "-sign", str(ns.chave), str(doc_arq)], capture_output=True, check=True).stdout
        sig_arq.write_text(der_para_rs(der) + "\n", encoding="ascii")
    texto, assinatura = doc_arq.read_text(encoding="ascii"), sig_arq.read_text(encoding="ascii")
    # "todos" também atualiza o beta: o administrador nunca fica atrás dos clientes
    for canal in (CANAIS if ns.canal == "todos" else ("beta",)):
        (raiz / f"{canal}.sig.novo").write_text(assinatura, encoding="ascii")
        (raiz / f"{canal}.txt.novo").write_text(texto, encoding="ascii", newline="\n")
        (raiz / f"{canal}.sig").unlink(missing_ok=True)   # sem assinatura o site não entrega: nunca texto novo com assinatura velha
        (raiz / f"{canal}.txt.novo").replace(raiz / f"{canal}.txt")
        (raiz / f"{canal}.sig.novo").replace(raiz / f"{canal}.sig")
    with open(ns.destino / "publicacoes.log", "a", encoding="utf-8") as log:
        log.write(json.dumps({"em": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"), "build": build, "versao": ns.versao,
                              "canal": ns.canal, "base": base, "commit": ref, "sha256": sha, "motivo": ns.motivo, **regras},
                             ensure_ascii=False) + "\n")
    print(f"publicado: correção {ns.versao} da compilação {build} no canal {ns.canal} "
          f"({len(regras['arquivos'])} arquivos, {regras['linhas']} linhas, DLL {len(dados) // 1048576} MB, pacote {pacote.stat().st_size // 1048576} MB)")
    return 0


def retirar() -> int:
    """sudo python3 publicar_correcao.py retirar <build> [beta|todos]  — o servidor deixa de entregar e os programas tiram a correção."""
    destino = Path("/var/lib/filuna-v2/correcao")
    build, canais = sys.argv[2], (sys.argv[3],) if len(sys.argv) > 3 else CANAIS
    if not re.fullmatch(r"[0-9a-f]{9}", build) or any(c not in CANAIS for c in canais):
        raise SystemExit("uso: publicar_correcao.py retirar <build de 9 dígitos> [beta|todos]")
    for canal in canais:
        for fim in ("sig", "txt"):
            (destino / build / f"{canal}.{fim}").unlink(missing_ok=True)
    with open(destino / "publicacoes.log", "a", encoding="utf-8") as log:
        log.write(json.dumps({"em": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"), "build": build, "retirada": list(canais)}) + "\n")
    print(f"retirada: correção da compilação {build} nos canais {', '.join(canais)}")
    return 0


if __name__ == "__main__":
    sys.exit(retirar() if len(sys.argv) > 1 and sys.argv[1] == "retirar" else main())
