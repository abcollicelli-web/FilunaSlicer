"""Raio-x dos casos DIVERGE de uma régua: separa diferença de COMANDO (o que a impressora executa:
toda linha que não é comentário) de diferença só em comentário/metadados, e conta os tipos de
comentário que mudam. Não muda veredito nenhum — serve para o proprietário decidir o critério quando
a fonte publicada não é a do binário oficial.

Uso: python3 comparar_comandos.py <pasta-da-paridade> [saida.json]
"""
from __future__ import annotations

import difflib
import json
import re
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from paridade import normalizado  # noqa: E402

TIPO = re.compile(r"^;\s*([A-Za-z_ ]+?)\s*[:=]|^;\s*([A-Za-z_]+)")


def linhas(p: Path) -> list[str]:
    return normalizado(p).decode("utf-8", "replace").splitlines()


def main() -> int:
    raiz = Path(sys.argv[1])
    d = json.loads((raiz / "prova.json").read_text(encoding="utf-8"))
    out, tipos = [], Counter()
    for i, r in enumerate(d["resultados"], 1):
        for c in r.get("casos", []):
            if c.get("veredito") != "DIVERGE":
                continue
            caso = raiz / "casos" / f"{i:03d}"
            comp = next((caso / f"{c['modelo']}-compilado").glob("*.gcode"), None)
            ofs = sorted((caso).glob(f"{c['modelo']}-oficial-*/*.gcode"))
            if not comp or not ofs:
                continue
            b = linhas(comp)
            melhor = None
            for o in ofs:
                a = linhas(o)
                ca = [l for l in a if l and not l.startswith(";")]
                cb = [l for l in b if l and not l.startswith(";")]
                dc = [l for l in difflib.unified_diff(ca, cb, lineterm="", n=0) if l[:1] in "+-" and l[:3] not in ("+++", "---")]
                if melhor is None or len(dc) < len(melhor[1]):
                    dm = [l for l in difflib.unified_diff(a, b, lineterm="", n=0) if l[:1] in "+-" and l[:3] not in ("+++", "---")]
                    melhor = (o, dc, dm)
            _, dc, dm = melhor
            t = Counter()
            for l in dm:
                m = TIPO.match(l[1:])
                if m:
                    t[(m.group(1) or m.group(2)).strip()] += 1
            tipos.update(t)
            out.append({"maquina": r["maquina"], "modelo": c["modelo"], "comandos_diferentes": len(dc),
                        "exemplo_comando": dc[:4], "comentarios_diferentes": dict(t.most_common(8))})
    resumo = {"casos_divergentes": len(out),
              "com_comando_diferente": sum(1 for x in out if x["comandos_diferentes"]),
              "so_comentario": sum(1 for x in out if not x["comandos_diferentes"]),
              "tipos_de_comentario": dict(tipos.most_common(12))}
    res = {"resumo": resumo, "casos": out}
    if len(sys.argv) > 2:
        Path(sys.argv[2]).write_text(json.dumps(res, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(json.dumps(resumo, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    sys.exit(main())
