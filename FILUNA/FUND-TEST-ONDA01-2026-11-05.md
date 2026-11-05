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
| FUND-T07 gcode pin↔Filuna binary | **PASS (1 fixture×1 preset, Windows Actions)** | run 42124288641; CORE-04 global BLOCKED |
| FUND-UNIT-19 | **PASS** | `python3 FILUNA/tests/run_fund_tests.py` → 19 OK |
| FUND-CMP-7 | checks documentais PASS; pacote PARTIAL | cmp_legal 7/7 ≠ certificação do instalador |
| FUND-DATA-8 | **PASS** | data_isolation 8/8 |
| FUND-T09 coexistência URL Windows | **NOT_RUN** | G4 |
| FUND-T08 start sem confirmação (impl runtime) | **NOT_RUN** | contrato; sem DeviceBridge |

Plataforma dos PASS unitários: **Linux box**. Não equivale a instalação Windows.

Commit implementação: `7ee410ec25e65d85727b96f021cadb38f11970e0`.


## Gates globais (obrigatório)

| Gate | Estado | Nota |
|---|---|---|
| G1 fundação | AUTHORIZED | escopo dono; sem visual |
| G2 global Onda01 | **NOT_RUN/BLOCKED** | unitários individuais ≠ G2 global |
| G3 global Onda01 | **NOT_RUN/BLOCKED** | baseline binária Windows pendente/em curso |
| G4 | **NOT_RUN** | físico intacto |

Atualizado UTC 2026-11-05T22:45:00Z.

## 19. VEREDITO DO DONO t19u — 2026-11-05 23:33 Europe/Rome (22:33 UTC) — IMUTÁVEL

Registro: `FILUNA/approvals/FUND-VERDICT-2026-11-05-t19u.json`. **Aprovação com ressalvas, avanço bloqueado.** Autoriza só correção/testes da fundação no gate atual; **não** homologa etapa/release.

| Item | Estado |
|---|---|
| LEGAL-02 | checks documentais 7/7 PASS · **auditoria do pacote PARTIAL** (`FILUNA/legal/PACKAGE-INVENTORY-run42102469888.json`: 4062 arquivos, 75 binários, 26 sem proveniência verificada, FFmpeg avcodec/avformat BLOCKED até auditoria; relação instalador↔portable NOT_RUN) |
| QA-03 | validador + `gate_governance.py` (contrato `FILUNA/qa/required-cases.json` v2026-11-05.3): hoje G2/G3/G4 = **BLOCKED** |
| CORE-04 | **BLOCKED** (baseline binário Windows em execução/sem PASS) |
| DATA-05 | **BLOCKED** (AppData Windows real NOT_RUN) |
| Onda 01 | **BLOCKED**; nenhuma onda avança |
| G4 | NOT_RUN |

check 113/113, CMP 7/7 e testes unitários **não** são autorização para progredir. Dados sintéticos provam só o validador.

## 20. FUND-T07 baseline binário REAL Windows — 2026-11-05 22:47 UTC (23:47 Europe/Rome)

- Run: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42124288641 (success) · artifact `Filuna_baseline_gcode_windows` id 13458549645 · commit testado `6c844fb8`.
- Orca pin: run upstream 42037889437 @ a620d804 (head_sha verificado no job), orca-slicer.exe b51efb83…, OrcaSlicer.dll 372aeb32…
- Filuna: run 42102469888 @ 2dace0db (verificado), orca-slicer.exe 4b822731…, OrcaSlicer.dll f0914911… (binários distintos).
- CLI (igual aos testes upstream `tests/cli`): `--datadir <TEMP> --load-settings "BBL P1S 0.4;0.20mm Standard @BBL X1C" --load-defaultfila --slice 0 --outputdir <TEMP> 20mm_cube.obj`; exit 0/0; sem GUI, sem impressora.
- G-code bruto: 272882 B cada; diff = **1 linha** (`; generated by … 22:47:09` vs `22:47:10`). Normalizado idêntico sha 1884525d…
- Negativo real: E50→E50.5 na linha 836 → compare FAIL (detectado).
- **Caso FUND-T07 (1 fixture × 1 preset) = PASS com evidência.** CORE-04 permanece **BLOCKED**: STL/3MF/STEP, outros presets e signoff do dono NOT_RUN.
- Tentativas anteriores (falhas reais, corrigidas): 42122763283 (import path), 42123026960 e 42123239839 (fixture CRLF → hash mismatch bloqueou corretamente), 42124068177 (`--export-gcode` inválido nesta versão). Rebuild do pin 42122933198 **cancelado** (desnecessário: download do upstream funcionou com GITHUB_TOKEN). Branch `filuna/pin-win-a620d804` ficou criada, sem uso.
- Limitação conhecida: `gate_governance` exige commit da evidência == HEAD; cada commit novo invalida a evidência até nova execução (fail-closed, intencional).

## 21. CORE-04 matriz real + auditoria de pacote — 2026-11-05 ~23:10 UTC (00:10 Europe/Rome, 06/11)

Veredito t19u preservado: **CORE-04, DATA-05, Onda 01 = BLOCKED.** 1 caso/preset PASS ≠ CORE-04 total PASS.

**Matriz** `FILUNA/qa/baseline/cases.json` v2026-11-05.1 (só arquivos reais do pin; nenhuma fixture sintética) — run https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42125523328 · subject commit `affbea51` (código/inputs idênticos a `b3e19fe8`; objetos git gravados em `provenance.inputs_git_objects`) · resultado `FILUNA/qa/runs/CORE04-matrix-run42125523328.json`.

| Fixture \ Preset | BBL P1S 0.4 (ref.) | Elegoo Centauri Carbon 2 0.4 | Anycubic Kobra 3 V2 0.4 |
|---|---|---|---|
| OBJ 20mm_cube | PASS | PASS (G-code bruto idêntico) | BLOCKED: exit -18 em ambos, `retraction_distances_when_cut: 0 not in range [10, 18]` |
| STL 20mmbox-LF | PASS | PASS | BLOCKED: idem -18 |
| STEP named_single_body | BLOCKED: exit -6 em ambos, CLI aceita só .stl/.obj/.amf | BLOCKED -6 | BLOCKED -6 |
| 3MF OrcaSliced (handy_models) | BLOCKED: exit -101 `CLI_GCODE_PATH_CONFLICTS` em ambos | BLOCKED -101 | BLOCKED -18 |

Totais: **4 PASS, 0 FAIL, 8 BLOCKED**. Cada PASS: exit 0/0, compare normalizado rc 0, negativo funcional (E+0.5) detectado. Binários: Orca pin exe b51efb83…/dll 372aeb32…, Filuna exe 4b822731…/dll f0914911….
Pendências reais: K3V2 via `--load-settings` falha validação nos DOIS binários (não é regressão Filuna; causa a investigar: preset isolado sem herança completa); STEP não suportado pela CLI desta versão (GUI/G4 seria outro teste); 3MF requer fixture sem conflito de caminho ou ajuste documentado; nenhum preset/fixture foi alterado para "passar".

**Auditoria do pacote** (`FILUNA/legal/PACKAGE-INVENTORY-run42102469888.json`, status PARTIAL):
- Instalador NSIS extraído com 7z no runner, **sem executar** (run https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42125762040): 4073 arquivos; todos os 4062 do portable têm sha256 idêntico no instalador; instalador acrescenta 10 (plugins NSIS, Uninstall.exe, 2 `resources/tooltip/.md`).
- Origem dos 75 binários: **PROVEN 9** (FFmpeg ×5 == zip pinado em `deps/FFMPEG/FFMPEG.cmake` sha 85da19da…; GMP/MPFR/WebView2Loader == blobs vendorizados no pin; uv.exe == zip oficial 0.11.21 cujo sha == `ORCA_UV_SHA256` no pin), **CORROBORATED 60** (OCCT TK* por PE VersionInfo 8.0.1 + receita V8_0_1; Python 3.12.13 por PE + receita; VC++ runtime por PE, assinatura não verificada), **SELF_BUILD 2**, **UNKNOWN 4** (libcrypto-3/libssl-3 3.0.16, libffi-8, sqlite3 do build CPython — não pinados; deps/OpenSSL do pin é 1.1.1w).
- **FFmpeg** (ctypes no runner, sem mídia/rede): todas as libs declaram **"GPL version 3 or later"** (`--enable-gpl --enable-version3`, sem nonfree). Obrigações de redistribuição/compatibilidade = **BLOCKED_PENDING_REVIEW**. Não é parecer jurídico.
- LEGAL-02: checks documentais 7/7 PASS · auditoria de pacote PARTIAL · licenças de terceiros BLOCKED.

Execuções canceladas externamente (não por mim): 42125236040, 42125277991 (`The operation was canceled`); refeitas uma vez com sucesso.
ADR proposta (não vigente): `FILUNA/FUND-ADR-SUBJECT-VS-REPORTING-COMMIT-PROPOSTA.md`. Regra vigente commit==HEAD mantida → `gate_governance` continua BLOCKED.
