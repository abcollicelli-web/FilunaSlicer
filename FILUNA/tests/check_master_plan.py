#!/usr/bin/env python3
"""Auditoria ESTÁTICA do plano de construção Filuna.

Comprova coerência de documentos, DAG e regras de bloqueio;
NÃO comprova qualidade do software, conformidade jurídica, certificação ou hardware.
"""
from __future__ import annotations

import json
import re
import sys
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FILUNA = ROOT / "FILUNA"
ERRORS: list[str] = []
CHECKS = 0


def check(ok: bool, label: str) -> None:
    global CHECKS
    CHECKS += 1
    if ok:
        print(f"PASS [{CHECKS:03d}] {label}")
    else:
        ERRORS.append(label)
        print(f"FAIL [{CHECKS:03d}] {label}")


def required_text(name: str) -> str:
    f = FILUNA / name
    check(f.is_file(), "documento obrigatório existe: " + name)
    return f.read_text(encoding="utf-8") if f.is_file() else ""


def is_gate_pass(results: list[str], has_evidence: bool) -> bool:
    return bool(results) and has_evidence and all(r == "PASS" for r in results)


def audit() -> None:
    required = [
        "REGRA-MESTRE-QUALIDADE.md",
        "MAPA-MIGRACAO-PERSONALIZACAO.md",
        "PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md",
        "ARQUITETURA-DEVICEBRIDGE-10-FABRICANTES.md",
        "POLITICA-CONEXAO-LAN-CONTA-CHAVE.md",
        "REFERENCIAS-COMPATIBILIDADE-SIMPLYPRINT.md",
        "MATRIZ-TESTES-ACEITE-100-POR-CENTO.md",
        "MIGRACAO-LEGADO-INVENTARIO-E-CONFLITOS.md",
        "CONFORMIDADE-LEGAL-NORMAS-E-CERTIFICACOES.md",
        "IDENTIDADE-VISUAL-E-MASCOTE.md",
        "TEST-ACCEPTANCE.md",
        "UPDATE-PLAYBOOK.md",
    ]
    docs = {k: required_text(k) for k in required}
    master = docs["PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md"]
    legal = docs["CONFORMIDADE-LEGAL-NORMAS-E-CERTIFICACOES.md"]
    migration = docs["MIGRACAO-LEGADO-INVENTARIO-E-CONFLITOS.md"]
    qa = docs["MATRIZ-TESTES-ACEITE-100-POR-CENTO.md"]
    devices = docs["ARQUITETURA-DEVICEBRIDGE-10-FABRICANTES.md"]
    for key in ("G0", "G1", "G2", "G3", "G4", "G5", "G6"):
        check(key in master and key in qa, f"Gate {key} rastreado no plano/QA")
    check("100%" in qa and "NOT RUN" in qa and "BLOCKED" in qa and "FAIL" in qa,
          "aprovação de 100% com estados negativos reconhecidos")
    for result in ("FAIL", "NOT RUN", "BLOCKED", "SKIP", "UNKNOWN"):
        check(not is_gate_pass(["PASS", result], True),
              f"gate recusa {result} mesmo com outro PASS")
    check(not is_gate_pass([], True), "gate recusa lista vazia")
    check(not is_gate_pass(["PASS"], False), "gate recusa ausência de evidências")
    check(is_gate_pass(["PASS", "PASS"], True), "gate só aprova 100% PASS com evidências")
    for i in range(1, 27):
        check(f"MIG-{i:02d}" in migration, f"domínio histórico MIG-{i:02d} inventariado")
    for brand in ("Elegoo", "Anycubic", "Bambu", "Creality", "Flashforge",
                  "QIDI", "Snapmaker", "Prusa", "UltiMaker", "Raise3D"):
        check(brand.lower() in devices.lower(), f"fabricante {brand} no DeviceBridge")
    for standard in ("ISO 9001:2026", "ISO/IEC 25010:2023",
                     "ISO/IEC 27001:2022", "NIST SP 800-218",
                     "IEC 62443", "GDPR", "CRA",
                     "11/09/2026", "11/12/2027", "AGPL",
                     "AI Act", "OWASP", "ISO/IEC 29147"):
        check(standard.lower() in legal.lower(), f"referência normativa {standard}")
    check("Não" in legal and "certificado" in legal.lower(),
          "auditoria não concede certificação")
    check("simplyprint" in docs["REFERENCIAS-COMPATIBILIDADE-SIMPLYPRINT.md"].lower(),
          "pesquisa independente documentada")
    check("LAN" in docs["POLITICA-CONEXAO-LAN-CONTA-CHAVE.md"]
          and "conta" in docs["POLITICA-CONEXAO-LAN-CONTA-CHAVE.md"]
          and "chave" in docs["POLITICA-CONEXAO-LAN-CONTA-CHAVE.md"],
          "escada LAN/conta/chave preservada")
    check("OrcaSlicer" in master and "não" in master,
          "motor principal Orca e limites documentados")

    graph_file = FILUNA / "CRONOGRAMA-DEPENDENCIAS.json"
    check(graph_file.is_file(), "cronograma machine-readable existe")
    if not graph_file.is_file():
        return
    try:
        graph = json.loads(graph_file.read_text(encoding="utf-8"))
    except (ValueError, OSError):
        check(False, "JSON de dependências válido")
        return
    check(graph.get("schema_version") == 1, "versão de schema definida")
    nodes = graph.get("nodes", [])
    check(isinstance(nodes, list) and len(nodes) >= 40,
          "pelo menos 40 entregas ligadas no grafo")
    ids = [x.get("id") for x in nodes if isinstance(x, dict)]
    check(len(set(ids)) == len(ids), "IDs únicos")
    root = graph.get("root")
    index = {x.get("id"): x for x in nodes if isinstance(x, dict)}
    check(root in index, "raiz existe")
    check({x["id"] for x in nodes if not x["depends_on"]} == {root},
          "somente governança pode nascer sem dependência")
    check(all(
        x.get("priority") in {"P0", "P1", "P2", "P3"}
        and x.get("exit_gate") in {"G0", "G1", "G2", "G3", "G4", "G5", "G6"}
        and x.get("tests", "").strip()
        and x.get("source_contract", "").strip()
        and isinstance(x.get("depends_on"), list)
        and x.get("acceptance_required") is True
        for x in nodes), "cada entrega tem prioridade, teste, fonte e gate")
    missing = [(x["id"], y) for x in nodes for y in x["depends_on"] if y not in index]
    check(not missing, f"nenhuma dependência desconhecida: {missing[:5]}")
    indegree = {k: 0 for k in index}
    graph_children = {k: [] for k in index}
    for x in nodes:
        for y in x["depends_on"]:
            if y in index:
                indegree[x["id"]] += 1
                graph_children[y].append(x["id"])
    queue = deque([k for k, deg in indegree.items() if deg == 0])
    order = []
    while queue:
        node = queue.popleft()
        order.append(node)
        for child in graph_children[node]:
            indegree[child] -= 1
            if indegree[child] == 0:
                queue.append(child)
    check(len(order) == len(nodes), "grafo acíclico (DAG)")
    reachable = {root}
    walk = [root]
    while walk:
        p = walk.pop()
        for nxt in graph_children[p]:
            if nxt not in reachable:
                reachable.add(nxt)
                walk.append(nxt)
    check(len(reachable) == len(nodes),
          f"todas entregas ligadas à raiz (alcançáveis: {len(reachable)})")
    release = graph.get("release_node")
    check(release in index and index[release]["exit_gate"] == "G6",
          "release tem gate G6 e nó real")
    check(all(index[k]["state"] != "PASS" for k in index),
          "nenhuma entrega declarada aprovada sem prova")
    check(len([k for k in ids if k.startswith("OEM-")]) == 10,
          "dez integrações OEM estão previstas")
    pre_release = set()
    def visit(k: str) -> None:
        if k in pre_release:
            return
        pre_release.add(k)
        for dep in index[k]["depends_on"]:
            visit(dep)
    if release in index and not missing:
        visit(release)
    check(all(k in pre_release for k in ids if k.startswith("OEM-")),
          "todos fabricantes bloqueiam release das dez marcas")
    check(all(k in pre_release for k in
              ("LEGAL-02", "QA-03", "CORE-04", "BRAND-08", "SAFETY-13",
               "TRUST-11", "CAMERAS-26", "FILAMENTS-27", "ATELIE-33")),
          "release depende de conformidade, qualidade, core, marca e negócio")
    remote = required_text("ARQUITETURA-SERVIDOR-DADOS-REMOTO.md")
    check("admin Filuna" in remote or "administrador Filuna" in remote,
          "servidor distingue administrador Filuna do dono da impressora")
    check("NUNCA" in remote and "SEM" not in "",
          "controle do suporte proibido por contrato")
    check("PITR" in remote and "restaura" in remote.lower()
          and "offline" in remote.lower(),
          "proteção de dados, recuperação e offline no contrato servidor")
    check("SRV-QA-020" in remote and "SRV-QA-001" in remote,
          "vinte casos de testes servidor/remoto catalogados")
    server_nodes = [k for k in ids if k.startswith("SRV-")]
    check(len(server_nodes) == 12,
          "doze fases de servidor e acesso remoto registradas")
    check(all(index[k]["state"] != "PASS" for k in server_nodes),
          "nenhuma fase remota marcada homologada sem teste")
    check(all(k in index for k in ("SRV-07","SRV-08","SRV-10","SRV-11")),
          "fases remoto OFF, piloto, release e operação presentes")
    check("SRV-07" in index["SRV-08"]["depends_on"]
          and "OEM-ELEGOO-16" in index["SRV-08"]["depends_on"]
          and "OEM-ANYCUBIC-17" in index["SRV-08"]["depends_on"],
          "piloto remoto condicionado a impressoras LAN físicas homologadas")
    check("RELEASE-38" in index["SRV-10"]["depends_on"]
          and "SRV-09" in index["SRV-10"]["depends_on"],
          "release remoto só ocorre depois de release local e QA remoto")
    check("SRV-10" in index["SRV-11"]["depends_on"],
          "operações remotas só depois da homologação remota")
    check("SRV-03" in index["SYNC-28"]["depends_on"],
          "sincronização vinculada ao banco/backup do Hub")
    check("SRV-04" in index["RELEASE-38"]["depends_on"],
          "release local vinculado ao diagnóstico seguro do servidor")
    check("OPERATE-39" not in pre_release and "RELEASE-38" in index["OPERATE-39"]["depends_on"],
          "operação ocorre depois do release")
    print(f"GRAFO: {len(nodes)} entregas, "
          f"{sum(len(n['depends_on']) for n in nodes)} vínculos, "
          f"ordem sem ciclo: {' → '.join(order[:6])} ...")


if __name__ == "__main__":
    audit()
    print(f"RESULTADO DOCUMENTAL: {CHECKS - len(ERRORS)}/{CHECKS} PASS, "
          f"{len(ERRORS)} FAIL")
    if ERRORS:
        print("ERROS:", "; ".join(ERRORS))
    else:
        print("ATENÇÃO: validação estática aprovada; G0/G1 e QA Windows/Hardware NÃO foram homologados.")
    sys.exit(1 if ERRORS else 0)
