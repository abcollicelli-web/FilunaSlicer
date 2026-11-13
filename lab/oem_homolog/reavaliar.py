"""Recalcula, sem fatiar de novo, os casos DIVERGE de uma prova.json da régua Prusa com a normalização
atual (paridade.normalizado): compara o G-code do compilado com as saídas do oficial já gravadas.
Uso: python3 reavaliar.py <pasta-da-paridade>"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from paridade import normalizado, sha  # noqa: E402

raiz = Path(sys.argv[1])
d = json.loads((raiz / "prova.json").read_text(encoding="utf-8"))
mud = 0
for i, r in enumerate(d["resultados"], 1):
    caso = raiz / "casos" / f"{i:03d}"
    for c in r.get("casos", []):
        if c.get("veredito") != "DIVERGE":
            continue
        comp = caso / f"{c['modelo']}-compilado" / "saida.gcode"
        ofs = sorted(caso.glob(f"{c['modelo']}-oficial-*/saida.gcode"))
        if comp.is_file() and ofs and sha(normalizado(comp)) in {sha(normalizado(o)) for o in ofs}:
            c["veredito_antes"], c["veredito"] = c["veredito"], "IDENTICO"
            c["reavaliado"] = "normalização atual (rótulos M486/objects_info)"
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
print(f"casos reavaliados: {mud}; RESUMO {json.dumps(res)}")
