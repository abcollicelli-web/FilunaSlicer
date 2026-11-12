#!/usr/bin/env python3
"""Filuna — pacote de perfis de um fabricante para download sob demanda.

1. Confere que os perfis da FONTE fixada (clone no commit do module.json) são byte a byte os
   mesmos que o binário OFICIAL daquela versão carrega (pasta resources/profiles extraída).
2. Resolve o fechamento da herança: outras pastas de fabricante de que os presets da marca
   herdam (ex.: OrcaFilamentLibrary) entram no pacote; nada é editado.
3. Audita o catálogo (pais ausentes, ciclos) e lista as impressoras selecionáveis.
4. Grava um .tar.gz determinístico + manifest.json com o SHA-256 de cada arquivo.

Uso:
  pacote_perfis.py --id qidi --vendor "Q Series,X 3 Series" --fonte SRC/resources/profiles \
                   --oficial OFICIAL/resources/profiles --versao 2.07.02.73 --saida DIR
"""
from __future__ import annotations

import argparse
import gzip
import hashlib
import io
import json
import sys
import tarfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from perfis import Catalogo  # noqa: E402


def sha_arq(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def arquivos_vendor(raiz: Path, vendor: str) -> dict[str, str]:
    out = {}
    j = raiz / f"{vendor}.json"
    if j.is_file():
        out[j.name] = sha_arq(j)
    d = raiz / vendor
    if d.is_dir():
        for p in sorted(d.rglob("*")):
            if p.is_file():
                out[str(p.relative_to(raiz))] = sha_arq(p)
    return out


def fechamento(cat: Catalogo, vendors: list[str]) -> set[str]:
    """Pastas necessárias: as marcas pedidas + bibliotecas comuns de que algum preset delas herda
    (a herança é resolvida dentro da marca primeiro, como o fatiador)."""
    pastas = set(vendors)
    for (v, t, n), (_, d) in cat.por_vendor.items():
        if v not in vendors:
            continue
        atual, visto = d.get("inherits"), set()
        while atual and atual not in visto:
            visto.add(atual)
            onde, item = cat._busca(v, t, atual)
            if not item:
                break
            pastas.add(onde)
            atual = item[1].get("inherits")
    return pastas


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--id", required=True)
    ap.add_argument("--vendor", required=True)
    ap.add_argument("--fonte", type=Path, required=True)
    ap.add_argument("--oficial", type=Path, required=True)
    ap.add_argument("--versao", required=True)
    ap.add_argument("--saida", type=Path, required=True)
    ns = ap.parse_args()
    vendors = [v for v in ns.vendor.split(",") if v]

    cat = Catalogo(ns.oficial)
    pastas = sorted(fechamento(cat, vendors))
    comparacao, ok = {}, True
    for v in pastas:
        a, b = arquivos_vendor(ns.fonte, v), arquivos_vendor(ns.oficial, v)
        so_f = sorted(set(a) - set(b))
        so_o = sorted(set(b) - set(a))
        dif = sorted(k for k in set(a) & set(b) if a[k] != b[k])
        comparacao[v] = {"arquivos_fonte": len(a), "arquivos_oficial": len(b),
                         "so_na_fonte": so_f[:50], "so_no_oficial": so_o[:50], "diferentes": dif[:50],
                         "identico": not (so_f or so_o or dif)}
        ok &= comparacao[v]["identico"]

    aud = cat.auditoria()
    imps = cat.impressoras(",".join(vendors))
    # erros de auditoria só das pastas do pacote
    erros = [e for e in aud["erros"] if any(e.startswith(f"[{v}]") for v in vendors)]

    ns.saida.mkdir(parents=True, exist_ok=True)
    nome = f"perfis-{ns.id}-{ns.versao}.tar.gz"
    manifesto_arquivos = {}
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode="w", format=tarfile.PAX_FORMAT) as tar:
        for v in pastas:
            for rel, h in sorted(arquivos_vendor(ns.oficial, v).items()):
                p = ns.oficial / rel
                ti = tarfile.TarInfo("profiles/" + rel)
                dados = p.read_bytes()
                ti.size, ti.mtime, ti.mode, ti.uid, ti.gid, ti.uname, ti.gname = len(dados), 0, 0o644, 0, 0, "", ""
                tar.addfile(ti, io.BytesIO(dados))
                manifesto_arquivos["profiles/" + rel] = h
    gz = io.BytesIO()
    with gzip.GzipFile(fileobj=gz, mode="wb", mtime=0, filename="") as g:
        g.write(buf.getvalue())
    (ns.saida / nome).write_bytes(gz.getvalue())
    manifesto = {
        "schema": 1, "id": ns.id, "componente": "profiles", "versao": ns.versao,
        "marcas": vendors, "pastas": pastas, "pacote": nome,
        "pacote_sha256": hashlib.sha256(gz.getvalue()).hexdigest(), "pacote_bytes": len(gz.getvalue()),
        "fonte_igual_oficial": ok, "comparacao": comparacao,
        "auditoria": {"contagem": aud["contagem"], "erros_no_pacote": erros},
        "impressoras": [{"maquina": i["maquina"], "processo": i.get("processo"), "filamento": i.get("filamento"),
                         "notas": i.get("notas", []), "erro": i.get("erro")} for i in imps],
        "arquivos": manifesto_arquivos,
    }
    (ns.saida / f"perfis-{ns.id}-{ns.versao}.manifest.json").write_text(
        json.dumps(manifesto, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(json.dumps({"id": ns.id, "pastas": pastas, "fonte_igual_oficial": ok, "impressoras": len(imps),
                      "sem_erro": sum(1 for i in imps if not i.get("erro")), "erros_no_pacote": len(erros),
                      "pacote_sha256": manifesto["pacote_sha256"]}, ensure_ascii=False))
    return 0 if ok else 3


if __name__ == "__main__":
    sys.exit(main())
