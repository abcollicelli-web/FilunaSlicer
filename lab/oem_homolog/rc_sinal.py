"""Corrige numa prova.json da régua Prusa os casos em que o oficial (pelo shell do Flatpak, 128+N) e o
compilado (direto, -N) morreram pelo mesmo sinal, gravados antes da normalização do código de saída.
Uso: python3 rc_sinal.py <pasta-da-paridade>"""
import json
import sys
from pathlib import Path


def n(rc):
    return 128 - rc if isinstance(rc, int) and rc < 0 else rc


raiz = Path(sys.argv[1])
d = json.loads((raiz / "prova.json").read_text(encoding="utf-8"))
mud = 0
for r in d["resultados"]:
    for c in r.get("casos", []):
        if c.get("veredito") == "OFICIAL_NAO_FATIOU" and not c.get("variantes_oficiais") and "compilado" in c:
            ro, rcc = n(c["oficial"]["rc"]), n(c["compilado"]["rc"])
            if ro == rcc != 0 and not c["compilado"]["gcodes"]:
                c["oficial"]["rc"], c["compilado"]["rc"] = ro, rcc
                c["veredito"] = "MESMA_RECUSA_DO_OFICIAL"
                mud += 1
    vs = [c["veredito"] for c in r.get("casos", [])]
    if vs:
        aceitos = ("IDENTICO", "MESMA_RECUSA_DO_OFICIAL")
        r["veredito"] = "IDENTICO" if all(v == "IDENTICO" for v in vs) else next((v for v in vs if v not in aceitos), "IDENTICO_COM_RECUSA_OFICIAL")
res = {}
for r in d["resultados"]:
    res[r["veredito"]] = res.get(r["veredito"], 0) + 1
d["resumo"] = res
(raiz / "prova.json").write_text(json.dumps(d, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
print(f"casos corrigidos: {mud}; RESUMO {json.dumps(res)}")
