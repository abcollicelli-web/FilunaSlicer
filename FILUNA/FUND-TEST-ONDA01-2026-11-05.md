# FILUNA — TEST Onda 01 Fundação (casos medíveis)

**ID** FLN-FUND-TEST-2026-11-05.2 · **Atualizado** 2026-11-05 23:35:00 CET  
**SPEC/REQ** FUND-SPEC / FUND-REQ  
**Plataforma destes resultados exploratórios** Linux box · commit `ee02554401c026370f7bac19b4a30ba4c8e97aaf`  
**Hardware** N/A (G4)

## Inventário de execução

| Ambiente | Pode rodar agora? | Notas |
|---|---|---|
| Box Linux docs checks | **Sim** | FUND-T01..T04 |
| CI `filuna-foundation-check` | Sim em push `filuna/quality-*` ou PR regras do yml | Não disparar só por este doc se branch já coberta por PR #3 paths |
| Unit Catch2 / slice | Não neste box sem build | CI `build_all`/`unit_tests` — caro |
| Baseline G-code pin↔Filuna | **NOT RUN** | Aguarda aceite + script/fixture job |
| Windows AppData/URL/instalador | **NOT RUN** | G4 / runner Windows |
| Impressoras | **NOT RUN** | G4 |

## Casos

| case_id | req_id | gate | setup | expected | actual (2026-11-05 23:30:00 CET) | result | evidence |
|---|---|---|---|---|---|---|---|
| FUND-T01 | REQ-FUND-005 | G2 | `python3 FILUNA/tests/check_quality_gate.py --mode foundation` | 0 falhas | reexecutado neste lote | **PASS** | stdout local |
| FUND-T02 | REQ-FUND-005 | G2 | `check_quality_gate.py --mode release` sem dossiê | falha fechada | reexecutado; bloqueia sem release-evidence | **PASS** | foundation workflow espelha |
| FUND-T03 | REQ-FUND-001/010 | G0/G2 | `check_master_plan.py` | 113/113 | reexecutado | **PASS** | stdout local |
| FUND-T04 | REQ-FUND-001 | G3 | `gh api` upstream-mirror sha | = `a620d804efbe74162c96ad29d755cdab2b823530` | conferido | **PASS** | API |
| FUND-T05 | REQ-FUND-003 | G3 | grep `SetAppName(FILUNA_DATA_DIR_NAME)` | presente no rebrand tip | presente em GUI_App.cpp | **PASS** (código) | path src |
| FUND-T06 | REQ-FUND-009 | G2 | run 42102469888 | success + artifacts Filuna | success @ `2dace0db86d305bec5b9627179f265012e4f5069` | **PASS** (estado) | Actions URL |
| FUND-T07 | REQ-FUND-004 | G3 | comparar G-code pin↔Filuna fixture `15943d9ad054…` | dentro da tolerância | **não executado** (sem binário/CI) | **NOT RUN** | SPEC §5 |
| FUND-T08 | REQ-FUND-007 | G2 | revisão contrato: start sem confirmação | proibido | só documental | **NOT RUN** (impl) | — |
| FUND-T09 | REQ-FUND-012 | G4 | coexistência Orca+Filuna URL | sem corrupção | — | **NOT RUN** | G4 |
| FUND-T10 | REQ-FUND-014 | — | SPEC não lista Home/tokens | ausentes do escopo | SPEC §3 | **PASS** (doc) | SPEC |

## Comandos exploratórios deste lote
```text
python3 FILUNA/tests/check_master_plan.py
python3 FILUNA/tests/check_quality_gate.py --mode foundation
python3 FILUNA/tests/check_quality_gate.py --mode release   # expect fail-closed
gh api repos/abcollicelli-web/FilunaSlicer/commits/upstream-mirror --jq .sha
```


## Execução pós-aprovação (2026-11-05 23:35:00 CET)

Aprovação: `FILUNA/approvals/FUND-SPEC-2026-11-05.1-owner-approval.json`.

| case_id | result | evidência |
|---|---|---|
| FUND-T01 foundation | **PASS** | check_quality_gate foundation 0 FAIL |
| FUND-T02 release fail-closed | **PASS** | modo release bloqueia sem dossiê |
| FUND-T03 master plan | **PASS** | 113/113 |
| FUND-T04 pin mirror | **PASS** | UPSTREAM_PIN + CMP-002 |
| FUND-T05 SetAppName Filuna | **PASS** | data_isolation + CMP-006 |
| FUND-T06 artifact 42102469888 | **PASS** (estado) | Actions success prévio |
| FUND-T07 gcode pin↔Filuna binary | **NOT_RUN** | sem binário; só sintético PASS |
| FUND-UNIT-19 | **PASS** | `python3 FILUNA/tests/run_fund_tests.py` → 19 OK |
| FUND-CMP-7 | **PASS** | cmp_legal 7/7 |
| FUND-DATA-8 | **PASS** | data_isolation 8/8 |
| FUND-T09 coexistência URL Windows | **NOT_RUN** | G4 |
| FUND-T08 start sem confirmação (impl runtime) | **NOT_RUN** | contrato; sem DeviceBridge |

Plataforma dos PASS unitários: **Linux box**. Não equivale a instalação Windows.

Commit implementação: `7ee410ec25e65d85727b96f021cadb38f11970e0`.
