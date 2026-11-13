"""Critério "comandos iguais" (decisão do proprietário, 13/11/2026, para a Creality): quando a fonte publicada
não é a do binário oficial e a diferença está só no que a impressora NÃO executa, o caso vale.

Para cada caso DIVERGE da prova:
  * comandos = linhas que não são comentário, sem M73 (progresso/tempo restante, derivados da estimativa)
    e sem linhas em branco;
  * IDENTICO_POR_COMANDO se os comandos do compilado são iguais aos de alguma saída do oficial;
  * IDENTICO_POR_COMANDO (variação do oficial) se, camada a camada, os comandos de cada camada do compilado
    aparecem iguais em alguma saída do oficial na mesma camada (oficial não determinístico), ou se as únicas
    linhas diferentes são das que o próprio oficial muda entre duas execuções (posição sorteada);
  * o resto continua DIVERGE.
A prova original fica em prova.antes-comandos.json; cada caso revisto guarda o motivo.

Uso: python3 criterio_comandos.py <pasta-da-paridade>
"""
from __future__ import annotations

import difflib
import json
import re
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from paridade import CAMADA, normalizado  # noqa: E402


def comandos(p: Path) -> list[str]:
    return [l.decode("utf-8", "replace") for l in normalizado(p).splitlines()
            if l.strip() and not l.startswith(b";") and not l.startswith(b"M73")]


def camadas_cmd(p: Path) -> list[tuple[bytes, ...]]:  # bytes: só igualdade, sem difflib
    out, cur = [], []
    for l in normalizado(p).splitlines():
        if CAMADA.match(l):
            out.append(tuple(cur))
            cur = []
        elif l.strip() and not l.startswith(b";") and not l.startswith(b"M73"):
            cur.append(l)
    out.append(tuple(cur))
    return out


def forma(l: str) -> str:
    return re.sub(r"[-\d.]+", "#", l)


def diferentes(a: list[str], b: list[str]) -> list[str]:
    return [l for l in difflib.unified_diff(a, b, lineterm="", n=0) if l[:1] in "+-" and l[:3] not in ("+++", "---")]


def main() -> int:
    raiz = Path(sys.argv[1])
    prova = raiz / "prova.json"
    if not (raiz / "prova.antes-comandos.json").exists():
        shutil.copy2(prova, raiz / "prova.antes-comandos.json")
    d = json.loads(prova.read_text(encoding="utf-8"))
    rev = 0
    for i, r in enumerate(d["resultados"], 1):
        caso = raiz / "casos" / f"{i:03d}"
        for c in r.get("casos", []):
            if c.get("veredito") != "DIVERGE":
                continue
            comp = next((caso / f"{c['modelo']}-compilado").glob("*.gcode"), None)
            ofs = sorted(caso.glob(f"{c['modelo']}-oficial-*/*.gcode"))
            if not comp or not ofs:
                continue
            cc = comandos(comp)
            co = [comandos(o) for o in ofs]
            motivo = None
            if any(cc == x for x in co):
                motivo = "comandos idênticos a uma saída do oficial; diferença só em comentário/metadado"
            else:
                lc = camadas_cmd(comp)
                lo = [camadas_cmd(o) for o in ofs]
                if all(len(x) == len(lc) for x in lo) and len(lo) > 1 and all(
                        any(x[k] == lc[k] for x in lo) for k in range(len(lc))):
                    motivo = f"cada camada do compilado tem os mesmos comandos de uma saída do oficial ({len(ofs)} execuções do oficial)"
                elif len(co) > 1:
                    entre_oficiais = {forma(l[1:]) for l in diferentes(co[0], co[1])}
                    dc = diferentes(co[0], cc)
                    if entre_oficiais and dc and all(forma(l[1:]) in entre_oficiais for l in dc):
                        exemplo = sorted(entre_oficiais)[:3]
                        motivo = ("só linhas que o próprio oficial muda entre duas execuções: "
                                  + ", ".join(exemplo))
            if motivo:
                c["veredito_antes"], c["veredito"], c["criterio"] = c["veredito"], "IDENTICO_POR_COMANDO", motivo
                rev += 1
        vs = [c["veredito"] for c in r.get("casos", [])]
        if vs:
            aceitos = ("IDENTICO", "IDENTICO_POR_CAMADA", "IDENTICO_POR_COMANDO", "MESMA_RECUSA_DO_OFICIAL")
            r["veredito"] = ("IDENTICO" if all(v == "IDENTICO" for v in vs)
                             else next((v for v in vs if v not in aceitos), "IDENTICO_POR_COMANDO"))
    res: dict[str, int] = {}
    for r in d["resultados"]:
        res[r["veredito"]] = res.get(r["veredito"], 0) + 1
    d["resumo"] = res
    d["criterio_comandos"] = "decisão do proprietário 13/11/2026 (ver criterio_comandos.py)"
    prova.write_text(json.dumps(d, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"casos revistos: {rev}; RESUMO {json.dumps(res, ensure_ascii=False)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
