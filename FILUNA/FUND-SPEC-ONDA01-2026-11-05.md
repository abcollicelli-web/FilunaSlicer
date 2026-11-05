# FILUNA — SPEC Onda 01 Fundação (contrato, não implementação)

**ID** FLN-FUND-SPEC-2026-11-05.1  
**Atualizado** 2026-11-05 23:30:00 CET (Europe/Rome)  
**Estado documental** **ESCOPO APROVADO** pelo proprietário 2026-11-05 23:24 Europe/Rome (widget t18s14). Registro: `FILUNA/approvals/FUND-SPEC-2026-11-05.1-owner-approval.json` (não é assinatura criptográfica). Implementação mínima §9 **em andamento/neste lote**.  
**Não é** G1 visual/Home/tokens HEX (Onda 03 / BRAND-08).  
**Pré-requisito** Onda 00 / G0 **PASS DOCUMENTAL** @ `ee025544…` (PR #3).  
**Branch** `filuna/continuidade-g0-20261105` · tip trabalho `ee02554401c026370f7bac19b4a30ba4c8e97aaf`

## 1. Mapeamento ao plano e ao DAG

| Fonte | Texto / nó | Uso neste contrato |
|---|---|---|
| PLANO Onda **01 — Fundação** | CI, checks de verdade, baseline G-code, instalador privado, isolamento de dados, rastreamento de versão, logs · saída *G1+G2+G3 da fundação* · pré = 00 PASS | Escopo funcional da onda |
| REGRA portões | G2 implementação local; G3 integração/regressão; G4 Windows/hardware **fora** desta onda como PASS | Gates técnicos |
| DAG | Ver §2 | Ordem de nós |

**Interpretação de “G1 da fundação”:** critérios **mensuráveis do contrato de fundação** (este SPEC + REQ/TEST), **não** o G1 visual da REGRA (protótipo/identidade/Home). Identidade/Home = Onda 03 após Onda 02.

## 2. Primeiro nó elegível e dependências

| Ordem | Nó DAG | Deps | exit_gate | Estado JSON atual | Papel na Onda 01 |
|---|---|---|---|---|---|
| 0 (feito documental) | GOV-00, SRC-01 | — / GOV-00 | G0 | `IN_PROGRESS` (*) | Genealogia/G0 |
| **1 — próximo elegível** | **LEGAL-02** | GOV-00, SRC-01 | G0 | `NOT_STARTED` | Fechar CMP/licenças remanescentes da fundação (complementa G0-OEM) |
| 2 | **QA-03** | GOV-00, LEGAL-02 | G2 | `NOT_STARTED` | Pipeline testes/evidências/SBOM; checks verdadeiros |
| 3 | **CORE-04** | SRC-01, QA-03 | G3 | `NOT_STARTED` | Baseline G-code / regressão STL·3MF·STEP |
| 4 | **DATA-05** | CORE-04, LEGAL-02 | G3 | `NOT_STARTED` | Isolamento `%APPDATA%\Filuna`, backup/config |

(\*) O checker `check_master_plan.py` **proíbe** `state=PASS` nos nós; GOV/SRC permanecem `IN_PROGRESS` no JSON mesmo com G0 PASS documental no REGISTRO.

**SRV-00+** e **BRAND-08/SHELL-09** estão **fora** do escopo mínimo desta SPEC.

## 3. Escopo mínimo (o que entra / o que não entra)

### Entra (contrato + casos medíveis)
1. LEGAL-02: matriz CMP restante, NOTICE/créditos, política sem plugin proprietário (já em G0-OEM — consolidar checklist CMP-*).  
2. QA-03: schema de evidência; falha fechada de release; extensão documentada dos checks; **não** assinar/publicar.  
3. CORE-04: protocolo de baseline G-code Orca pin ↔ Filuna (fixtures, hashes, tolerâncias, normalização só metadados).  
4. DATA-05: requisitos de isolamento de dados Filuna vs Orca; negativos de associação `orcaslicer://` / pasta.  
5. Instalador privado / versão: tratar run [42102469888](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42102469888) como **candidato** existente (evidência de artefato), sem novo CI caro neste lote.  
6. Logs: contrato de campos sanitizados (sem modelos/tokens) — implementação depois do aceite.

### Não entra
- Tokens HEX, Home, shell visual, mascote derivados (Onda 03).  
- DeviceBridge / OEM hardware (Ondas 05–06).  
- Provisionar SRV, secrets, assinatura Authenticode, publish.  
- Alterar `src/libslic3r/**` ou `upstream-mirror`.  
- Inventar resultados de G-code sem binário executável comparável.

## 4. Baseline atual (read-only, evidência)

| Item | Valor medido neste lote | Limite |
|---|---|---|
| Upstream pin | `a620d804efbe74162c96ad29d755cdab2b823530` | UPSTREAM_PIN.md; mirror API |
| Base rebrand instalável | `2dace0db86d305bec5b9627179f265012e4f5069` · run 42102469888 **success** | Artefato ≠ QA Windows (G4) |
| Pasta dados | `SetAppName(FILUNA_DATA_DIR_NAME)` → `%APPDATA%\Filuna` | GUI_App.cpp (já no rebrand) |
| ProgID / URL | `Filuna.Slicer.1`; associa `orcaslicer://` | Coexistência com Orca = teste G4 |
| Fixture exemplo | `tests/data/20mm_cube.obj` sha256 `15943d9ad0548b654199c4e8eecdfc937964eaac80aad0237bf70fac75725b80` | Licença: árvore testes Orca AGPL |
| Unit tests G-code | Existem em `tests/fff_print/*` (Catch2) | Requer **build** + artifact CI; **não** executados neste lote no box sem binário |
| Box Linux agora | `check_master_plan` / `check_quality_gate` / leitura docs | Sem `orca-slicer` local no PATH deste ambiente |
| Brand originais | Presentes: 105206.jpg(99392B), 105207.jpg(95048B), 105208.jpg(184358B), 99421.webp(38980B), 99422.jpg(311466B), FILUNA-BRAND-ICONE-F-contexto-original.jpg(184358B), FILUNA-BRAND-LOGO-original.jpg(95048B), FILUNA-BRAND-PALETA-original.jpg(99392B), FILUNA-MASCOTE-CIRCULAR-original.jpg(311466B), FILUNA-MASCOTE-TRANSPARENTE-original.webp(38980B) | Tokens HEX ainda não homologados (Onda 03) |

## 5. Protocolo proposto — comparação G-code (CORE-04)

**Objetivo:** mesmo input + presets + seed → comparar saída Orca pin vs Filuna rebrand **sem** mascarar geometria/volumetria/movimentos.

| Campo | Regra |
|---|---|
| Input | Fixtures versionadas sob `tests/data/` ou `FILUNA/qa/fixtures/` (quando criadas pós-aceite) com sha256 + origem/licença |
| Presets | JSON de `resources/profiles/` pinados por path+hash do commit sob teste |
| CLI | Flags documentadas; para multi-placa `--arrange 0` quando aplicável (REQ legado PEI) |
| Normalização permitida | Remover/estabilizar **apenas** metadados não funcionais (timestamps, paths absolutos de máquina, UUIDs de sessão, cabeçalhos de User-Agent variáveis se não afetam movimento) |
| Normalização **proibida** | Alterar E/F/G0–G3 movimentos, temperaturas efetivas de trajetória, purga, Z, ordenação de extrusão para “passar” diff |
| Tolerâncias | Declarar por fixture (ex.: identidade byte-a-byte após normalização de metadados **ou** diff estrutural de moves com Δ máximo documentado). Sem tolerância = exigir igualdade pós-normalização |
| Ambientes | (A) CI Windows x64 com artifact Filuna + Orca pin build; (B) unit `tests/fff_print` no mesmo runner; (C) G4 físico N/A para baseline de arquivo |
| Falha | Divergência fora da tolerância = **FAIL** CORE; não rebaseline silencioso |

**Estado agora:** normalizador allowlist estrita + fixtures **sintéticas** (separadas do binário). Baseline binária via `filuna-windows.yml` input `baseline-gcode-only` (Windows runner). Até run+artifacts de comparação: **NOT_RUN/BLOCKED**.

## 6. Requisitos negativos (fundação)

| ID | Requisito | Gate |
|---|---|---|
| FUND-NEG-01 | Nenhuma impressão/envio automático sem confirmação explícita do usuário | G2/G3 contrato; G4 prova |
| FUND-NEG-02 | Release mode sem `FILUNA/QA/release-evidence.json` deve **falhar fechado** | Já coberto foundation-check |
| FUND-NEG-03 | Dados de usuário Filuna não gravam em `%APPDATA%\OrcaSlicer` por padrão | DATA-05 |
| FUND-NEG-04 | Associação `orcaslicer://` não pode corromper projetos Orca existentes (teste coexistência) | G4 |
| FUND-NEG-05 | Logs/telemetria sem caminhos de modelo, G-code, tokens, senhas | QA-03 / SAFETY |

## 7. Onde rodar testes (inventário)

| Classe | Exemplos | Box Linux agora | CI existente | G4 físico |
|---|---|---|---|---|
| Docs/DAG | `check_master_plan.py`, `check_quality_gate.py --mode foundation` | **Sim** | `filuna-foundation-check.yml` | N/A |
| Unit Catch2 slice | `tests/fff_print`, `tests/libslic3r` | Não (sem build local) | `unit_tests.yml` via `build_all` | N/A |
| Baseline G-code pin↔Filuna | Script futuro pós-aceite | Não | Windows runner (caro) — só após aceite + necessidade | N/A arquivo |
| Instalador/AppData/URL | Isolamento pasta, ProgID | Não | Parcial | **Sim** |
| Impressão CC2/Kobra | — | Não | Não | **Sim** |

## 8. Critério de saída da Onda 01 (fundação) — não marcar agora

Para declarar Onda 01 pronta (futuro): LEGAL-02 checklist CMP PASS; QA-03 evidências+CI verdadeiros PASS; CORE-04 baseline com casos obrigatórios PASS no ambiente declarado; DATA-05 isolamento PASS nos casos aplicáveis; instalador privado candidato verificado (hash); logs conforme contrato; **sem** exigir G4 hardware nem G1 visual.

**G1 contrato fundação:** escopo aprovado pelo dono (sem mudança visual). **G2/G3 globais da Onda01:** **NOT_RUN/BLOCKED** até critérios de saída §8; casos individuais (ex. unitários Linux) podem PASS com plataforma/evidência próprias — **não** elevam G2/G3 globais. Baseline binária Windows e G4 = **NOT_RUN** até evidência.

## 9. Implementação mínima (autorizada) — estado neste head

1. LEGAL-02: `FILUNA/legal/CMP-CHECKLIST-LEGAL-02.json` + `FILUNA/tests/cmp_legal.py` — **PASS 7/7** no box.  
2. QA-03: `FILUNA/qa/evidence.schema.json` + `evidence_schema.py` + testes negativos fail-closed — **PASS**.  
3. CORE-04: fixtures + `normalize_gcode.py`/`compare_gcode.py` + unitários sintéticos — **PASS**; pin↔Filuna Windows **NOT_RUN**.  
4. DATA-05: `data_isolation.py` (fonte + temp) — **PASS**; AppData/registro real **NOT_RUN** (G4).  
5. Sem Home/DeviceBridge/SRV; `libslic3r` intocado.

## 10. Pergunta única de aceite (dono)

> Homologa o escopo **FLN-FUND-SPEC-2026-11-05.1** (Onda 01 Fundação: LEGAL-02→QA-03→CORE-04→DATA-05; sem UI/Home/tokens) para autorizar a implementação mínima descrita no §9?
