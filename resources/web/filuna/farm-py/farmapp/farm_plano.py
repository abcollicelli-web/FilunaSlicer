"""Plano da Farm: como dividir um pedido entre as impressoras, e o porquê de cada decisão.

Funções puras (sem banco): recebem as peças, as respostas do questionário, as impressoras, as bobinas e o
fatiamento real de cada arquivo, e devolvem as mesas, os prazos, as comparações e o que precisa ser corrigido.

O que é medido e o que é estimado:
- tempo e gramas por peça vêm do fatiamento real do arquivo (dividido entre as peças pelo volume);
- peças por mesa é conta de encaixe em grade com folga — o Filuna Slicer faz a arrumação definitiva;
- purga e tempo de troca de cor são estimativas por troca (parâmetros abaixo) até o G-code real chegar.
"""
from __future__ import annotations

import math

PADRAO = {
    "folga_mm": 5.0,           # espaço entre peças na mesa
    "borda_mm": 5.0,           # faixa livre nas bordas da mesa
    "min_mesa_extra": 6.0,     # minutos parados por mesa (aquecer, retirar, limpar)
    "purga_g": {"ams": 0.6, "cabecas": 0.05},   # gramas jogadas fora por troca de cor
    "troca_s": {"ams": 75, "cabecas": 12},       # segundos por troca de cor
    "ocupacao_mista": 0.72,    # fração da mesa aproveitada quando peças diferentes vão juntas
    "altura_camada": 0.2,
}

CORES_NOME = [("Preto", "#1E1E1E"), ("Branco", "#FFFFFF"), ("Cinza", "#8E8E8E"), ("Vermelho", "#D62828"),
              ("Laranja", "#F77F00"), ("Amarelo", "#F6C90E"), ("Verde", "#2A9D4B"), ("Azul", "#2563EB"),
              ("Roxo", "#7B4BFF"), ("Rosa", "#FF5CA8"), ("Marrom", "#7A4A2A"), ("Bege", "#E3CDA4")]


def _rgb(h: str) -> tuple[int, int, int]:
    h = (h or "#000000").lstrip("#")
    try:
        return int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)
    except ValueError:
        return 0, 0, 0


def distancia_cor(a: str, b: str) -> float:
    ra, ga, ba = _rgb(a)
    rb, gb, bb = _rgb(b)
    return math.sqrt((ra - rb) ** 2 + (ga - gb) ** 2 + (ba - bb) ** 2)


def cor_perto(a: str | None, b: str | None) -> bool:
    return bool(a and b) and distancia_cor(a, b) <= 60


def nome_cor(h: str | None) -> str:
    if not h:
        return "Sem cor"
    return min(CORES_NOME, key=lambda c: distancia_cor(c[1], h))[0]


def duracao(seg) -> str:
    seg = int(seg or 0)
    h, m = seg // 3600, round(seg % 3600 / 60)
    if m == 60:
        h, m = h + 1, 0
    return f"{h} h {m:02d}" if h else f"{m} min"


def gramas(g) -> str:
    return f"{round(g):,} g".replace(",", " ")


def reais(v) -> str:
    return "R$ " + f"{v:,.2f}".replace(",", "X").replace(".", ",").replace("X", ".")


def cabem(px: float, py: float, pz: float, mesa: dict, p: dict) -> tuple[int, int]:
    """Quantas peças iguais cabem numa mesa: (melhor arrumação, do jeito que o arquivo veio)."""
    if pz > mesa["h"]:
        return 0, 0
    g, w, d = p["folga_mm"], mesa["w"] - 2 * p["borda_mm"], mesa["d"] - 2 * p["borda_mm"]

    def grade(a, b):
        if a > w or b > d:
            return 0
        return int((w + g) // (a + g)) * int((d + g) // (b + g))
    como_veio = grade(px, py)
    return max(como_veio, grade(py, px)), como_veio


def _mesa(imp: dict) -> dict:
    return {"w": imp["mesa_w"], "d": imp["mesa_d"], "h": imp["mesa_h"]}


def _unidades(pecas: list[dict], fatias: dict) -> dict:
    """Tempo e gramas de uma unidade de cada peça, repartindo o fatiamento do arquivo pelo volume."""
    por_arquivo: dict[str, float] = {}
    for pc in pecas:
        por_arquivo[pc["arquivo"]] = por_arquivo.get(pc["arquivo"], 0.0) + pc["volume_mm3"] * pc["copias"]
    saida = {}
    for pc in pecas:
        f = fatias.get(pc["arquivo"])
        total = por_arquivo[pc["arquivo"]]
        if not f or total <= 0:
            saida[pc["id"]] = None
            continue
        parte = pc["volume_mm3"] / total
        saida[pc["id"]] = {"tempo_s": f["tempo_s"] * parte, "gramas": f["gramas"] * parte,
                           "altura_camada": f.get("altura_camada") or 0}
    return saida


def material_da_cor(r: dict, cor: str | None) -> str:
    """Material confirmado para uma cor no wizard (`materiais` = {hex: material}); sem resposta, o padrão do pedido."""
    for hx, mat in (r.get("materiais") or {}).items():
        if mat and cor_perto(hx, cor):
            return mat
    return r.get("material") or "PLA"


def _slots(impressoras: list[dict], material: str, cor: str):
    for imp in impressoras:
        for s in imp["slots"]:
            if s["material"] == material and cor_perto(s["cor_hex"], cor):
                yield imp, s
                break


def planejar(pecas: list[dict], r: dict, impressoras: list[dict], bobinas: list[dict], fatias: dict,
             params: dict | None = None, precos_kg: dict | None = None, densidades: dict | None = None) -> dict:
    p = {**PADRAO, **(params or {})}
    material = r["material"]
    conjuntos = max(1, int(r.get("conjuntos") or 1))
    un = _unidades(pecas, fatias)
    medido = all(un[pc["id"]] for pc in pecas)
    bob = {b["id"]: b for b in bobinas}
    preco_kg = lambda b_id, mat=material: (bob[b_id]["preco_kg"] if b_id in bob and bob[b_id]["preco_kg"] else
                                           (precos_kg or {}).get(mat, 100.0))
    # o fatiamento é feito no material padrão; peça de outro material tem o peso corrigido pela densidade
    dens = densidades or {}
    base = r.get("_base") or material
    r = {**r, "_fator": {m: (dens[m] / dens[base]) if dens.get(m) and dens.get(base) else 1.0
                         for m in set((r.get("materiais") or {}).values()) | {material}}}
    correcoes: list[dict] = []
    multicor = [pc for pc in pecas if len(pc["cores"]) > 1]
    simples = [pc for pc in pecas if len(pc["cores"]) <= 1]
    cores_pedido = []
    for pc in pecas:
        for c in (pc["cores"] or [pc.get("cor")]):
            if c and not any(cor_perto(c, x) for x in cores_pedido):
                cores_pedido.append(c)

    sep = _separadas(simples, multicor, material, conjuntos, r, impressoras, bob, un, p, correcoes, preco_kg)
    jun = _juntas(pecas, cores_pedido, material, conjuntos, impressoras, bob, un, p, preco_kg)
    usar_juntas = r.get("modo_cores") == "juntas" and len(cores_pedido) > 1
    escolhido = jun if usar_juntas else sep
    if usar_juntas:
        correcoes = list(jun["correcoes"])

    if not impressoras:
        correcoes.insert(0, _c("sem_impressora", "Nenhuma impressora cadastrada", True,
                               "a Farm precisa saber quais impressoras você tem e qual cor está em cada uma.",
                               "cadastre suas impressoras, ou conecte o Filuna Slicer para ele informar sozinho.",
                               "o pedido passa a ser dividido automaticamente entre elas.",
                               {"tipo": "link", "href": "/farm/impressoras", "rotulo": "Cadastrar impressoras"}))
    if not medido:
        correcoes.append(_c("sem_medida", "Tempo e filamento ainda não medidos", False,
                            "o fatiamento deste arquivo não terminou ou não foi possível no servidor.",
                            "o Filuna Slicer informa o tempo e o filamento reais ao fatiar cada mesa.",
                            "a fila funciona, mas o prazo e a conferência de filamento só aparecem depois da medida."))
    if not usar_juntas and len({pc["id"] for pc in simples}) > 1 and len(cores_pedido) > 1:
        lista = " + ".join(f"{pc['copias']} {pc['nome']}" for pc in pecas)
        correcoes.append(_c("montagem", "Montagem depois da impressão", False,
                            "cada cor sai de uma impressora diferente, então as peças saem soltas.",
                            f"junte {lista} em cada conjunto.",
                            f"{conjuntos} conjunto{'s' if conjuntos > 1 else ''} sem nenhuma purga de troca de cor.",
                            resolvido=True, rotulo="para você saber"))

    comparacoes = {}
    if len(cores_pedido) > 1:
        comparacoes["cores"] = {
            "separadas": {"purga_g": 0.0, "prejuizo": 0.0, "mesas": len(sep["mesas"]), "prazo_s": sep["prazo_s"],
                          "possivel": True},
            "juntas": {"purga_g": jun["purga_g"], "prejuizo": jun["prejuizo"], "mesas": len(jun["mesas"]),
                       "prazo_s": jun["prazo_s"], "possivel": jun["possivel"], "trocas": jun["trocas"],
                       "pct": (round(100 * jun["purga_g"] / jun["gramas"]) if jun["gramas"] else 0),
                       "impressoras": jun["n_impressoras"]},
        }
    total_pecas = sum(pc["copias"] for pc in pecas) * conjuntos
    mesas_cheias = _mesas_iguais(simples, conjuntos, impressoras, p, False)
    comparacoes["iguais"] = {
        "juntas": {"mesas": mesas_cheias},
        "uma": {"mesas": sum(pc["copias"] for pc in simples) * conjuntos,
                "extra_s": max(0, sum(pc["copias"] for pc in simples) * conjuntos - mesas_cheias) * p["min_mesa_extra"] * 60},
    }
    sugestoes = [] if usar_juntas else _sugestoes(sep, simples, material, conjuntos, r, impressoras, bob, un, p)
    custo = sum(m["gramas"] / 1000 * preco_kg(m["bobina_id"], m["material"]) for m in escolhido["mesas"])
    custo += escolhido.get("prejuizo") or 0.0
    return {
        "modo": "juntas" if usar_juntas else "separadas", "medido": medido, "mesas": escolhido["mesas"],
        "grupos": escolhido["grupos"], "correcoes": correcoes, "comparacoes": comparacoes, "sugestoes": sugestoes,
        "resumo": {"pecas": total_pecas, "mesas": len(escolhido["mesas"]), "gramas": escolhido["gramas"],
                   "purga_g": escolhido.get("purga_g", 0.0), "sequencial_s": escolhido["sequencial_s"],
                   "prazo_s": escolhido["prazo_s"], "impressoras": escolhido["n_impressoras"],
                   "custo": round(custo, 2),
                   "pendentes": sum(1 for c in correcoes if c["precisa_agir"] and not c["resolvido"])},
    }


def _c(tipo, titulo, precisa_agir, porque, como, resultado, acao=None, resolvido=False, rotulo=None) -> dict:
    return {"tipo": tipo, "titulo": titulo, "precisa_agir": precisa_agir, "porque": porque, "como": como,
            "resultado": resultado, "acao": acao, "resolvido": resolvido,
            "rotulo": rotulo or ("você precisa agir" if precisa_agir else "já corrigido pelo sistema")}


def _mesas_iguais(pecas, conjuntos, impressoras, p, uma_por_mesa) -> int:
    total = 0
    for pc in pecas:
        n = max([cabem(pc["x"], pc["y"], pc["z"], _mesa(i), p)[0] for i in impressoras] or [0])
        qtd = pc["copias"] * conjuntos
        total += qtd if (uma_por_mesa or n < 1) else math.ceil(qtd / n)
    return total


def _separadas(simples, multicor, material, conjuntos, r, impressoras, bob, un, p, correcoes, preco_kg) -> dict:
    uma = r.get("iguais") == "uma_por_mesa"
    prioridade = r.get("prioridade") or "prazo"
    carga = {i["id"]: float(i.get("carga_s") or 0) for i in impressoras}
    inicio = dict(carga)
    reservado: dict[str, float] = {}
    mesas: list[dict] = []
    grupos: list[dict] = []
    por_cor: dict[str, list] = {}
    for pc in simples:
        cor = (pc["cores"][0] if pc["cores"] else pc.get("cor")) or ""
        chave = next((k for k in por_cor if cor_perto(k, cor)), cor)
        por_cor.setdefault(chave, []).append(pc)

    padrao = material
    for cor, lista in por_cor.items():
        material = material_da_cor(r, cor) if cor else padrao
        fator = (r.get("_fator") or {}).get(material, 1.0)
        cand = list(_slots(impressoras, material, cor)) if cor else []
        nome = next((s["cor_nome"] for _, s in cand if s.get("cor_nome")), None) or nome_cor(cor)
        g = {"cor": cor, "cor_nome": nome, "material": material, "pecas": [pc["nome"] for pc in lista], "mesas": [],
             "impressoras": [], "tempo_s": 0.0, "gramas": 0.0}
        for pc in lista:
            total = pc["copias"] * conjuntos
            u = un[pc["id"]]
            cabe = {imp["id"]: cabem(pc["x"], pc["y"], pc["z"], _mesa(imp), p) for imp in impressoras}
            aptos = [(imp, s) for imp, s in cand if cabe[imp["id"]][0] >= 1]
            if aptos and prioridade != "prazo":
                if prioridade == "filamento":
                    aptos = [max(aptos, key=lambda t: (bob.get(t[1]["bobina_id"]) or {}).get("restante_g") or 0)]
                else:
                    usadas = {m["impressora_id"] for m in mesas}
                    aptos = [min(aptos, key=lambda t: (t[0]["id"] not in usadas, carga[t[0]["id"]]))]
            maior = max([c[0] for c in cabe.values()] or [0])
            if maior < 1:
                correcoes.append(_c("nao_cabe", f"{pc['nome']} não cabe em nenhuma mesa", True,
                                    f"a peça mede {_cm(pc['x'])} × {_cm(pc['y'])} × {_cm(pc['z'])} cm, maior que as suas mesas.",
                                    "divida a peça em partes no Filuna Slicer, ou cadastre uma impressora maior.",
                                    "a peça entra na fila assim que couber em alguma mesa."))
            restante, numero = total, 0
            while restante > 0:
                if aptos:
                    imp, slot = min(aptos, key=lambda t: carga[t[0]["id"]])
                    n = 1 if uma else cabe[imp["id"]][0]
                else:
                    imp, slot, n = None, None, (1 if uma else max(maior, 1))
                q = min(n, restante)
                restante -= q
                numero += 1
                t = (u["tempo_s"] * q) if u else 0.0
                gr = (u["gramas"] * q * fator) if u else 0.0
                bloqueio = None if imp else ("nao_cabe" if maior < 1 else "falta_cor")
                bid = slot["bobina_id"] if slot else None
                if imp and u and bid in bob:
                    livre = (bob[bid]["restante_g"] or 0) - (bob[bid].get("reservado_g") or 0) - reservado.get(bid, 0.0)
                    if gr > livre + 1e-6:
                        bloqueio = "filamento"
                    reservado[bid] = reservado.get(bid, 0.0) + gr
                m = {"peca_id": pc["id"], "peca": pc["nome"], "arquivo": pc["arquivo"], "quantidade": q,
                     "numero": numero, "impressora_id": imp["id"] if imp else None,
                     "impressora": imp["nome"] if imp else None, "posicao": slot["posicao"] if slot else None,
                     "bobina_id": bid, "material": material, "cor": cor, "cor_nome": nome,
                     "tempo_s": round(t), "gramas": round(gr, 1), "bloqueio": bloqueio, "cabem": n}
                if imp:
                    carga[imp["id"]] += t + p["min_mesa_extra"] * 60
                    if imp["nome"] not in g["impressoras"]:
                        g["impressoras"].append(imp["nome"])
                mesas.append(m)
                g["mesas"].append(m)
                g["tempo_s"] += t
                g["gramas"] += gr
            for m in g["mesas"]:
                if m["peca_id"] == pc["id"]:
                    m["total"] = numero
            melhor, veio = max(cabe.values(), key=lambda c: c[0], default=(0, 0))
            if not uma and melhor > veio >= 1 and math.ceil(total / veio) > math.ceil(total / melhor):
                menos = math.ceil(total / veio) - math.ceil(total / melhor)
                correcoes.append(_c("reorganizado", f"{pc['nome']}: peças giradas para caber mais", False,
                                    f"do jeito que o arquivo veio cabiam {veio} por mesa.",
                                    "o sistema girou as peças; nada para você fazer.",
                                    f"{melhor} por mesa: {menos} mesa{'s' if menos > 1 else ''} a menos"
                                    + (f" e cerca de {duracao(menos * p['min_mesa_extra'] * 60)} parados a menos." if menos else "."),
                                    resolvido=True))
        if cor and not cand:
            evitar = {i["id"] for c2 in por_cor if c2 and c2 != cor
                      for i, _ in _slots(impressoras, material_da_cor(r, c2), c2)}
            fc = _falta_cor(cor, nome, material, lista, impressoras, bob, g, evitar,
                            (r.get("troca") or {}).get(cor.upper()), list(por_cor))
            fc["cor"] = cor
            correcoes.append(fc)
        _confere_filamento(g, bob, correcoes)
        grupos.append(g)

    for pc in multicor:
        correcoes.append(_c("peca_multicor", f"{pc['nome']} tem {len(pc['cores'])} cores na mesma peça", True,
                            "peça pintada ou com partes coladas de cores diferentes não pode ser separada por impressora.",
                            "responda “Todas as cores na mesma mesa” e use uma impressora que troca de cor.",
                            "a peça sai pronta, com a purga de troca de cor mostrada antes de imprimir."))
    usadas = {m["impressora_id"] for m in mesas if m["impressora_id"]}
    return {"mesas": mesas, "grupos": grupos, "gramas": round(sum(m["gramas"] for m in mesas), 1),
            "sequencial_s": round(sum(m["tempo_s"] for m in mesas)),
            "prazo_s": round(max([carga[i] - inicio[i] for i in usadas] or [0])), "n_impressoras": len(usadas)}


def _cm(mm) -> str:
    v = (mm or 0) / 10
    t = f"{v:.2f}" if v < 1 else f"{v:.1f}"
    return t.rstrip("0").rstrip(".").replace(".", ",") or "0"


def posicao_livre(imp: dict, cores_do_pedido=()) -> dict:
    """A posição da impressora que pode receber outra bobina: a que não guarda nenhuma cor do pedido."""
    for s in imp["slots"]:
        if not s.get("cor_hex") or not any(cor_perto(s["cor_hex"], c) for c in cores_do_pedido if c):
            return s
    return imp["slots"][0]


def _falta_cor(cor, nome, material, lista, impressoras, bob, grupo, evitar=(), preferida=None, cores_do_pedido=()) -> dict:
    carregadas = {s["bobina_id"] for i in impressoras for s in i["slots"] if s["bobina_id"]}
    em_estoque = sorted([b for b in bob.values() if b["material"] == material and cor_perto(b["cor_hex"], cor)
                         and (b["restante_g"] or 0) > 0 and b["id"] not in carregadas],
                        key=lambda b: -(b["restante_g"] or 0))
    pecas = ", ".join(pc["nome"] for pc in lista)
    porque = f"{pecas} precisa de {material} {nome} e nenhuma impressora está com essa cor. Sem a cor, não entra na fila."
    com_slot = [i for i in impressoras if i["slots"] and i["id"] not in evitar] or [i for i in impressoras if i["slots"]]
    livres = [i for i in com_slot if i.get("estado") == "livre"] or com_slot
    escolhida = next((i for i in impressoras if i["id"] == preferida and i["slots"]), None)   # escolhida no wizard
    if escolhida:
        livres = [escolhida]
    if em_estoque and livres:
        b, imp = em_estoque[0], livres[0]
        pos = posicao_livre(imp, cores_do_pedido)
        return _c("falta_cor", f"Falta cor na impressora: {material} {nome}", True, porque,
                  f"coloque a bobina {material} {b['cor_nome'] or nome} nº {b['numero']} ({gramas(b['restante_g'])}) na {imp['nome']}"
                  + (f", no lugar da posição {pos['posicao']}." if len(imp["slots"]) > 1 else "."),
                  f"{pecas} entra na fila da {imp['nome']}"
                  + (f" e leva {duracao(grupo['tempo_s'])}." if grupo["tempo_s"] else "."),
                  {"tipo": "carregar", "impressora_id": imp["id"], "posicao": pos["posicao"],
                   "bobina_id": b["id"], "rotulo": f"Coloquei a bobina na {imp['nome']}"})
    return _c("falta_cor", f"Falta cor na impressora: {material} {nome}", True, porque,
              f"não há bobina de {material} {nome} no estoque. Cadastre a bobina e coloque numa impressora.",
              f"{pecas} entra na fila assim que a cor estiver numa impressora.",
              {"tipo": "link", "href": "/farm/estoque", "rotulo": "Cadastrar bobina"})


def _confere_filamento(g: dict, bob: dict, correcoes: list) -> None:
    por_bobina: dict = {}
    for m in g["mesas"]:
        if m["bobina_id"] and m["impressora_id"]:
            por_bobina.setdefault((m["impressora_id"], m["bobina_id"]), []).append(m)
    for (_, bid), ms in por_bobina.items():
        b = bob.get(bid)
        if not b or not any(m["bloqueio"] == "filamento" for m in ms):
            continue
        precisa = sum(m["gramas"] for m in ms)
        tem = max(0.0, (b["restante_g"] or 0) - (b.get("reservado_g") or 0))
        primeira = next(m for m in ms if m["bloqueio"] == "filamento")
        resto = sum(m["gramas"] for m in ms if m["bloqueio"] == "filamento")   # o que as mesas paradas precisam
        outras = sorted([x for x in bob.values() if x["id"] != bid and x["material"] == b["material"]
                         and cor_perto(x["cor_hex"], b["cor_hex"]) and (x["restante_g"] or 0) > 0],
                        key=lambda x: ((x["restante_g"] or 0) < resto, x["restante_g"] or 0))
        antes = sum(1 for m in ms if m["bloqueio"] != "filamento")
        if outras:
            o = outras[0] if (outras[0]["restante_g"] or 0) >= resto else outras[-1]
            como = (f"troque pela bobina nº {o['numero']} ({gramas(o['restante_g'])}) "
                    + (f"depois da mesa {antes}" if antes else "antes de começar")
                    + ". As mesas seguintes ficam aguardando a troca."
                    + ("" if (o["restante_g"] or 0) >= resto else
                       f" Mesmo assim ainda faltam {gramas(resto - (o['restante_g'] or 0))}: cadastre mais uma bobina."))
            acao = {"tipo": "carregar", "impressora_id": primeira["impressora_id"], "posicao": primeira["posicao"],
                    "bobina_id": o["id"], "rotulo": "Troquei a bobina"}
        else:
            como = f"faltam {gramas(precisa - tem)}. Cadastre outra bobina de {b['material']} {g['cor_nome']} e coloque na impressora."
            acao = {"tipo": "link", "href": "/farm/estoque", "rotulo": "Ver estoque"}
        correcoes.append({**_c("filamento", f"Filamento insuficiente: {b['material']} {g['cor_nome']} na {primeira['impressora']}", True,
                               f"{primeira['peca']} ({primeira['arquivo']}) precisa de {gramas(precisa)} e a bobina nº {b['numero']} "
                               f"tem {gramas(tem)}. Acabaria na mesa {primeira['numero']}.",
                               como, "nenhuma peça perdida por acabar o filamento no meio.", acao),
                          "cor": g["cor"], "precisa_g": round(precisa, 1), "tem_g": round(tem, 1)})


def _juntas(pecas, cores, material, conjuntos, impressoras, bob, un, p, preco_kg) -> dict:
    """Todas as cores na mesma mesa: só em impressora que troca de cor. Purga estimada por troca."""
    correcoes: list[dict] = []
    area = sum((pc["x"] + p["folga_mm"]) * (pc["y"] + p["folga_mm"]) * pc["copias"] for pc in pecas)
    alto = max(pc["z"] for pc in pecas)

    def serve(imp):
        if imp.get("multicor") not in ("ams", "cabecas") or alto > imp["mesa_h"]:
            return None
        achados = []
        for c in cores:
            s = next((s for s in imp["slots"] if s["material"] == material and cor_perto(s["cor_hex"], c)), None)
            if not s:
                return None
            achados.append(s)
        return achados

    def por_mesa(imp):
        if any(cabem(pc["x"], pc["y"], pc["z"], _mesa(imp), p)[0] < 1 for pc in pecas):
            return 0
        util = (imp["mesa_w"] - 2 * p["borda_mm"]) * (imp["mesa_d"] - 2 * p["borda_mm"]) * p["ocupacao_mista"]
        return max(1, int(util // area)) if area else 0

    aptas = [(i, serve(i)) for i in impressoras]
    aptas = [(i, s) for i, s in aptas if s and por_mesa(i) >= 1]
    tem_multicor = any(i.get("multicor") in ("ams", "cabecas") for i in impressoras)
    possivel = bool(aptas)
    if aptas:
        tipo = aptas[0][0]["multicor"]
    else:
        tipo = next((i["multicor"] for i in impressoras if i.get("multicor") in ("ams", "cabecas")), "ams")
    alt = next((u["altura_camada"] for u in un.values() if u and u["altura_camada"]), 0) or p["altura_camada"]
    camadas = max(1, math.ceil(alto / alt))
    trocas_mesa = 0
    for i in range(camadas):
        z = i * alt
        k = len({next((c for c in cores if cor_perto(c, x)), x)
                 for pc in pecas if pc["z"] > z for x in (pc["cores"] or [pc.get("cor")]) if x})
        trocas_mesa += max(0, k - 1)
    t_conj = sum((un[pc["id"]]["tempo_s"] if un[pc["id"]] else 0) * pc["copias"] for pc in pecas)
    g_conj = sum((un[pc["id"]]["gramas"] if un[pc["id"]] else 0) * pc["copias"] for pc in pecas)
    referencia = aptas[0][0] if aptas else max(impressoras, key=lambda i: i["mesa_w"] * i["mesa_d"], default=None)
    n = (por_mesa(referencia) if referencia else 0) or 1
    carga = {i["id"]: float(i.get("carga_s") or 0) for i, _ in aptas}
    inicio = dict(carga)
    mesas, restante, numero = [], conjuntos, 0
    total_mesas = math.ceil(conjuntos / n)
    while restante > 0:
        q = min(n, restante)
        restante -= q
        numero += 1
        t = t_conj * q + trocas_mesa * p["troca_s"][tipo]
        if aptas:
            imp, _ = min(aptas, key=lambda t_: carga[t_[0]["id"]])
            carga[imp["id"]] += t + p["min_mesa_extra"] * 60
        else:
            imp = None
        mesas.append({"peca_id": None, "peca": "Conjunto completo", "arquivo": pecas[0]["arquivo"], "quantidade": q,
                      "numero": numero, "total": total_mesas, "impressora_id": imp["id"] if imp else None,
                      "impressora": imp["nome"] if imp else None, "posicao": None, "bobina_id": None,
                      "material": material, "cor": cores[0] if cores else "", "cor_nome": "Várias cores",
                      "tempo_s": round(t), "gramas": round(g_conj * q + trocas_mesa * p["purga_g"][tipo], 1),
                      "purga_g": round(trocas_mesa * p["purga_g"][tipo], 1), "trocas": trocas_mesa,
                      "bloqueio": None if imp else "sem_multicor", "cabem": n})
    purga = round(sum(m["purga_g"] for m in mesas), 1)
    kg = sum(preco_kg(s["bobina_id"]) for s in (aptas[0][1] if aptas else [])) / max(1, len(aptas[0][1]) if aptas else 0) \
        if aptas else preco_kg(None)
    if not possivel:
        if not tem_multicor:
            como = "cadastre uma impressora que troca de cor (com AMS ou várias cabeças), ou responda “Uma cor por impressora”."
        else:
            como = f"coloque as {len(cores)} cores em {material} na mesma impressora que troca de cor, ou responda “Uma cor por impressora”."
        correcoes.append(_c("sem_multicor", "Nenhuma impressora pronta para várias cores na mesma mesa", True,
                            "imprimir todas as cores juntas exige uma impressora que troca de cor, com todas as cores carregadas.",
                            como, "as mesas ficam aguardando até existir uma impressora assim."))
    usadas = {m["impressora_id"] for m in mesas if m["impressora_id"]}
    return {"mesas": mesas, "grupos": [], "correcoes": correcoes, "possivel": possivel, "trocas": trocas_mesa * len(mesas),
            "purga_g": purga, "prejuizo": round(purga / 1000 * kg, 2),
            "gramas": round(sum(m["gramas"] for m in mesas), 1), "sequencial_s": round(sum(m["tempo_s"] for m in mesas)),
            "prazo_s": round(max([carga[i] - inicio[i] for i in usadas] or [0])) or round(sum(m["tempo_s"] for m in mesas)),
            "n_impressoras": len(usadas) or 1}


def _sugestoes(sep, simples, material, conjuntos, r, impressoras, bob, un, p) -> list[dict]:
    """Se uma impressora livre receber a cor que mais demora, quanto o prazo cai?"""
    if not sep["grupos"] or not sep["prazo_s"] or (r.get("prioridade") or "prazo") != "prazo":
        return []
    usadas = {m["impressora_id"] for m in sep["mesas"]}
    livres = [i for i in impressoras if i["id"] not in usadas and i.get("estado") == "livre" and i["slots"]]
    carregadas = {s["bobina_id"] for i in impressoras for s in i["slots"] if s["bobina_id"]}
    gargalo = max(sep["grupos"], key=lambda g: g["tempo_s"] / max(1, len(g["impressoras"])))
    if not livres or not gargalo["impressoras"] or not gargalo["cor"]:
        return []
    material = gargalo.get("material") or material
    b = next((x for x in sorted(bob.values(), key=lambda x: -(x["restante_g"] or 0))
              if x["material"] == material and cor_perto(x["cor_hex"], gargalo["cor"]) and x["id"] not in carregadas
              and (x["restante_g"] or 0) > 0), None)
    if not b:
        return []
    alvo = livres[0]
    teste = [dict(i, slots=([{"posicao": i["slots"][0]["posicao"], "material": material, "cor_hex": b["cor_hex"],
                              "cor_nome": b["cor_nome"], "bobina_id": b["id"]}] + i["slots"][1:]) if i["id"] == alvo["id"]
                  else i["slots"]) for i in impressoras]
    novo = _separadas(simples, [], material, conjuntos, r, teste, bob, un, p, [], lambda _: 0)
    if novo["prazo_s"] >= sep["prazo_s"] - 300:
        return []
    return [{"texto": f"a {alvo['nome']} está livre. Colocando {material} {gargalo['cor_nome']} (bobina nº {b['numero']}) nela, "
                      f"o prazo cai de {duracao(sep['prazo_s'])} para {duracao(novo['prazo_s'])}.",
             "acao": {"tipo": "carregar", "impressora_id": alvo["id"], "posicao": alvo["slots"][0]["posicao"],
                      "bobina_id": b["id"], "rotulo": f"Coloquei a bobina na {alvo['nome']}"}}]
