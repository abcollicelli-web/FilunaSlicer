#!/usr/bin/env python3
"""Publica uma versão do Filuna Slicer para a atualização automática — só o que mudou.

Cada versão publicada vira um "build": a lista de arquivos com SHA-256 (builds/<id>/manifesto.json) e os
arquivos guardados uma vez só, pelo conteúdo (blobs/<sha>). Para cada build anterior é gerado um pacote
só com os arquivos diferentes (pacotes/<de>__<para>.zip) e um documento assinado (docs/<de>__<para>.json
+ .sig, Ed25519, mesma chave do catálogo de módulos). Quem tem um build desconhecido recebe o pacote completo.

Três jeitos de publicar:
  --origem PASTA        pasta do programa compilado (a que tem FilunaSlicer.exe)
  --run ID              baixa o artefato "-exe" desse run do GitHub Actions
  --base BUILD --sobrepor PASTA
                        NÃO compila nada: parte do build BASE e troca/acrescenta os arquivos de PASTA
                        (mesma estrutura do programa, ex.: resources/web/filuna/dispositivo.html).
                        É a correção rápida de tela, texto, perfil e dado.

  python3 publicar_app.py --run 123 --build 440d178 --versao 2.5.0-beta.3 --nota "Conexão da Elegoo corrigida" \
      --interno "update de recursos internos" --canal beta
"""
from __future__ import annotations
import argparse, base64, datetime, hashlib, json, os, shutil, subprocess, sys, tempfile, zipfile
from pathlib import Path

def sha(p: Path) -> str:
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()

def seguro(s: str) -> bool:
    return bool(s) and len(s) <= 80 and all(c.isalnum() or c in "-._" for c in s)

def raiz_do_programa(p: Path) -> Path:
    """A pasta que contém o executável (o artefato pode trazer um nível a mais)."""
    for cand in [p] + sorted(x for x in p.rglob("*") if x.is_dir()):
        if any((cand / n).is_file() for n in ("FilunaSlicer.exe", "filuna-slicer.exe", "OrcaSlicer.exe")):
            return cand
    return p

def zipar(destino: Path, arquivos: dict[str, Path]):
    """Pasta ANTES dos arquivos dela: o extrator do Filuna só cria pasta a partir da entrada de pasta."""
    pastas = set()
    for rel in arquivos:
        partes = rel.split("/")
        for k in range(1, len(partes)):
            pastas.add("/".join(partes[:k]) + "/")
    with zipfile.ZipFile(destino, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for p in sorted(pastas):
            zi = zipfile.ZipInfo(p, date_time=(2026, 1, 1, 0, 0, 0)); zi.external_attr = 0x10
            z.writestr(zi, b"")
        for rel in sorted(arquivos):
            z.write(arquivos[rel], rel)

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--origem", type=Path); ap.add_argument("--run")
    ap.add_argument("--base"); ap.add_argument("--sobrepor", type=Path)
    ap.add_argument("--repo-gh", default="abcollicelli-web/FilunaSlicer")
    ap.add_argument("--build", required=True); ap.add_argument("--versao", required=True)
    ap.add_argument("--nota", action="append", default=[]); ap.add_argument("--interno", action="append", default=[])
    ap.add_argument("--canal", default="beta", choices=["beta", "todos", "nenhum"])
    ap.add_argument("--destino", type=Path, default=Path("/var/lib/filuna-v2/app"))
    ap.add_argument("--chave", type=Path, default=Path("/root/.filuna-modulos/ed25519.pem"))
    ap.add_argument("--deltas", type=int, default=8, help="quantos builds anteriores recebem pacote só do que mudou")
    ns = ap.parse_args()
    if not seguro(ns.build):
        print("build inválido"); return 2
    D = ns.destino
    for s in ("builds", "blobs", "docs", "pacotes"):
        (D / s).mkdir(parents=True, exist_ok=True)
    tmp = Path(tempfile.mkdtemp(prefix="filuna-app-", dir=str(D)))
    try:
        arquivos: dict[str, Path] = {}   # caminho relativo -> arquivo de origem
        if ns.base:
            if not ns.sobrepor or not ns.sobrepor.is_dir():
                print("--base pede --sobrepor PASTA"); return 2
            man = json.loads((D / "builds" / ns.base / "manifesto.json").read_text())
            for rel, info in man["arquivos"].items():
                arquivos[rel] = D / "blobs" / info["sha256"]
            n = 0
            for f in sorted(ns.sobrepor.rglob("*")):
                if f.is_file():
                    arquivos[f.relative_to(ns.sobrepor).as_posix()] = f; n += 1
            print(f"base {ns.base}: {len(man['arquivos'])} arquivos; sobrepostos {n}")
        else:
            origem = ns.origem
            if ns.run:
                baixar = tmp / "artefato"; baixar.mkdir()
                subprocess.run(["gh", "run", "download", str(ns.run), "-R", ns.repo_gh, "-p", "*-exe-*", "-D", str(baixar)], check=True)
                origem = baixar
            if not origem or not origem.is_dir():
                print("informe --origem, --run ou --base/--sobrepor"); return 2
            origem = raiz_do_programa(origem)
            for f in sorted(origem.rglob("*")):
                if f.is_file():
                    arquivos[f.relative_to(origem).as_posix()] = f
            print(f"origem {origem}: {len(arquivos)} arquivos")
        # identidade do build dentro do próprio programa (o cliente lê para saber o que tem instalado)
        ident = tmp / "filuna-build.json"
        ident.write_text(json.dumps({"build": ns.build, "versao": ns.versao}, ensure_ascii=False), encoding="utf-8")
        arquivos["filuna-build.json"] = ident
        # guarda os arquivos pelo conteúdo e escreve o manifesto
        man = {"build": ns.build, "versao": ns.versao, "arquivos": {}}
        for rel, f in arquivos.items():
            h = sha(f); blob = D / "blobs" / h
            if not blob.is_file():
                shutil.copyfile(f, str(blob) + ".tmp"); os.replace(str(blob) + ".tmp", blob)
            man["arquivos"][rel] = {"sha256": h, "bytes": blob.stat().st_size}
        (D / "builds" / ns.build).mkdir(exist_ok=True)
        (D / "builds" / ns.build / "manifesto.json").write_text(json.dumps(man, ensure_ascii=False), encoding="utf-8")
        agora = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        (D / "builds" / ns.build / "publicado_em").write_text(agora)

        def documento(nome: str, de: str, escolhidos: dict[str, Path], remover: list[str], tipo: str):
            pacote = D / "pacotes" / f"{nome}.zip"
            zipar(pacote, escolhidos)
            doc = {"schema": 1, "de": de, "para": ns.build, "versao": ns.versao, "tipo": tipo, "publicado_em": agora,
                   "notas": ns.nota, "internos": ns.interno, "arquivos": len(escolhidos), "remover": remover,
                   "pacote": {"arquivo": pacote.name, "sha256": sha(pacote), "bytes": pacote.stat().st_size}}
            texto = json.dumps(doc, ensure_ascii=False, sort_keys=True)
            (D / "docs" / f"{nome}.json").write_text(texto, encoding="utf-8")
            assin = subprocess.run(["openssl", "pkeyutl", "-sign", "-inkey", str(ns.chave), "-rawin", "-in",
                                    str(D / "docs" / f"{nome}.json")], check=True, capture_output=True).stdout
            (D / "docs" / f"{nome}.json.sig").write_text(base64.b64encode(assin).decode() + "\n", encoding="ascii")
            print(f"  {nome}: {len(escolhidos)} arquivos, {pacote.stat().st_size / 1048576:.1f} MB, remover {len(remover)}")

        blobs = {rel: D / "blobs" / i["sha256"] for rel, i in man["arquivos"].items()}
        documento(f"completo__{ns.build}", "*", blobs, [], "completo")
        anteriores = sorted((p for p in (D / "builds").iterdir() if p.is_dir() and p.name != ns.build and (p / "manifesto.json").is_file()),
                            key=lambda p: (p / "publicado_em").read_text() if (p / "publicado_em").is_file() else "", reverse=True)
        for p in anteriores[: ns.deltas]:
            velho = json.loads((p / "manifesto.json").read_text())["arquivos"]
            mudou = {rel: blobs[rel] for rel, i in man["arquivos"].items() if velho.get(rel, {}).get("sha256") != i["sha256"]}
            remover = sorted(rel for rel in velho if rel not in man["arquivos"])
            documento(f"{p.name}__{ns.build}", p.name, mudou, remover, "delta")
        if ns.canal != "nenhum":
            cf = D / "canal.json"
            canal = json.loads(cf.read_text()) if cf.is_file() else {}
            canal[ns.canal] = ns.build
            cf.write_text(json.dumps(canal, ensure_ascii=False), encoding="utf-8")
            print(f"canal {ns.canal} -> {ns.build}")
        subprocess.run(["chmod", "-R", "a+rX", str(D)], check=False)
        return 0
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

if __name__ == "__main__":
    sys.exit(main())
