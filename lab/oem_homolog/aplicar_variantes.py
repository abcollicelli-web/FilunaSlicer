"""Junta à prova.json da régua os resultados de mais_variantes.py (<caso>/<modelo>-mais-variantes.json).
Um caso DIVERGE vira IDENTICO_POR_CAMADA só se, com mais execuções do próprio oficial, cada camada do
compilado apareceu byte a byte numa saída do oficial; a evidência fica registrada no caso.

Uso: python3 aplicar_variantes.py <pasta-da-paridade>
"""
from __future__ import annotations

import json
import sys
from pathlib import Path


def main() -> int:
    raiz = Path(sys.argv[1])
    prova = raiz / "prova.json"
    d = json.loads(prova.read_text(encoding="utf-8"))
    mudou = 0
    for i, r in enumerate(d.get("resultados", []), 1):
        caso = raiz / "casos" / f"{i:03d}"
        for c in r.get("casos", []):
            mv = caso / f"{c.get('modelo')}-mais-variantes.json"
            if c.get("veredito") != "DIVERGE" or not mv.is_file():
                continue
            ev = json.loads(mv.read_text(encoding="utf-8"))
            c["mais_variantes"] = ev
            if ev.get("veredito") == "IDENTICO_POR_CAMADA":
                c["veredito_antes"] = c["veredito"]
                c["veredito"] = "IDENTICO_POR_CAMADA"
                mudou += 1
        vs = [c["veredito"] for c in r.get("casos", [])]
        if vs and all(v in ("IDENTICO", "IDENTICO_POR_CAMADA", "MESMA_RECUSA_DO_OFICIAL") for v in vs):
            r["veredito"] = ("IDENTICO" if all(v == "IDENTICO" for v in vs)
                             else "IDENTICO_COM_RECUSA_OFICIAL" if "MESMA_RECUSA_DO_OFICIAL" in vs else "IDENTICO_POR_CAMADA")
    resumo: dict[str, int] = {}
    for r in d.get("resultados", []):
        resumo[r["veredito"]] = resumo.get(r["veredito"], 0) + 1
    d["resumo"] = resumo
    prova.write_text(json.dumps(d, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"casos revistos: {mudou}; RESUMO {json.dumps(resumo)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
