# FILUNA — REGISTRO DE CONTINUIDADE E ETAPAS REALIZADAS

**ID** FLN-CONT-2026-11-05.1  
**Fuso** Europe/Rome (CET/CEST)  
**Atualizado** 2026-11-05 23:45:00 CET
**Estado do gate G0** **PASS DOCUMENTAL** (Onda 00 / REGRA G0). G1 NOT RUN · G2–G3 NOT RUN · G4 BLOCKED/NOT RUN (Windows/hardware) · G5–G6 NOT RUN. **Não** é release nem QA funcional.
**Autor deste checkpoint** agente autorizado pelo proprietário (t17u), lote continuidade+G0  

> **Obrigatório para qualquer IA ou colaborador:** ler este arquivo **antes** de alterar código, docs, CI, UI ou releases. Em seguida ler os documentos normativos listados abaixo. Não depender de chat, memória local, `/workspace` ou histórico de sessão.

## 0. Ordem de leitura obrigatória

1. Este registro: [REGISTRO-CONTINUIDADE-ETAPAS.md](REGISTRO-CONTINUIDADE-ETAPAS.md)
2. [REGRA-MESTRE-QUALIDADE.md](REGRA-MESTRE-QUALIDADE.md) — portões G0–G6
3. [PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md](PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md) — cronograma e escopo
4. [MANIFESTO-FONTES-LEGADO-2026-11-05.md](MANIFESTO-FONTES-LEGADO-2026-11-05.md) — 141 fontes com blob SHA
5. [MIGRACAO-LEGADO-INVENTARIO-E-CONFLITOS.md](MIGRACAO-LEGADO-INVENTARIO-E-CONFLITOS.md)
6. [UPSTREAM_PIN.md](UPSTREAM_PIN.md) + [UPDATE-PLAYBOOK.md](UPDATE-PLAYBOOK.md)
7. [TEST-ACCEPTANCE.md](TEST-ACCEPTANCE.md) + [MATRIZ-TESTES-ACEITE-100-POR-CENTO.md](MATRIZ-TESTES-ACEITE-100-POR-CENTO.md)
8. [AUDITORIA-PLANO-MESTRE-2026-11-05.md](AUDITORIA-PLANO-MESTRE-2026-11-05.md) e [DIARIO-QUALIDADE-2026-11-05.md](DIARIO-QUALIDADE-2026-11-05.md)
9. Demais contratos do dossiê sob `FILUNA/` conforme a onda ativa

**Precedência:** ordem expressa atual do proprietário > REGRA-MESTRE > PLANO-MESTRE > este registro (estado factual) > contratos Grok mais recentes e específicos > legado S/W (histórico) > código em SHA determinado. Segurança/AGPL não se anulam por desenho.

## 1. Objetivo e escopo vigentes

- **Produto:** Filuna Slicer Windows = fork nativo **OrcaSlicer 2.5.0-dev** (C++17/wxWidgets) com camada Filuna e módulos de fabricante **separados**.
- **Motor único:** OrcaSlicer. Não embutir Prusa/Cura/Kiri como backends de fatiamento.
- **Licença:** AGPL-3.0; créditos Slic3r → PrusaSlicer → Bambu Studio → OrcaSlicer → Filuna.
- **Sem** plugins de rede proprietários empacotados (Bambu/Flashforge) sem direito específico.
- **Sem** iniciar impressão sem confirmação do usuário.
- **Pasta de dados:** `%APPDATA%\Filuna` (própria; não misturar com Orca).
- **Testes físicos Windows/impressoras:** ainda **NÃO feitos** neste projeto F; build verde ≠ homologação.
- **Camada visual:** provisória / identidade referenciada; **não** produto final perfeito.
- **Escopo vigente:** G0 PASS documental; Onda 01 FUND contrato PROPOSTO; sem UI/DeviceBridge/remoto/impl prematura; sem merge PR #3 sem ordem; branch padrão inalterada.

## 2. Software-base, pin e branches (fonte de verdade)

| Item | Valor | Evidência |
|---|---|---|
| Repo produto | https://github.com/abcollicelli-web/FilunaSlicer | default_branch `filuna/main` |
| Pin upstream Orca | `a620d804efbe74162c96ad29d755cdab2b823530` (main 2.5.0-dev) | [UPSTREAM_PIN.md](UPSTREAM_PIN.md); `upstream-mirror` = mesmo SHA (verificado API 05/11 2026-11-05 22:48:30 CET) |
| Base funcional instalável (rebrand) | branch `filuna/rebrand-1` @ `2dace0db86d305bec5b9627179f265012e4f5069` | PR #1 aberto → `filuna/main` (não merged) |
| Documentação/QA/plano | branch `filuna/quality-master-20261105` (evolui; tip lida neste lote abaixo) | PR #2 **draft** → `filuna/rebrand-1` |
| Branch deste lote | `filuna/continuidade-g0-20261105` | PR draft → quality-master (não mergear sozinho) |
| Legado site (S) | `abcollicelli-web/Filuna` `main` @ `74aaf19311094265b893f2a5dc0f0d78731b9cc7` | docs `Filuna-Slicer/` |
| Legado Windows .NET (W) | `abcollicelli-web/filuna-slicer` `main` @ `61339ae22ace4abe7899ce9d5b03b54d96ffa762` | 0.2.42; **não** é o fork C++ atual |
| Plano mestre lido neste lote | blob `71957a312f2c405dafda09555f6c76070121069d` no commit base `d1935aefa91a1eda86f57b67b342a3857247294e` | reler remoto antes de novo lote |

### Build Windows conhecido (verde ≠ teste real)

| Run | Resultado | SHA | Artefatos Filuna | URL |
|---|---|---|---|---|
| 42102469888 | **success** | `2dace0db` (rebrand-1) | `Filuna_Windows_x64_installer` (~168 864 208 B), `Filuna_Windows_x64_portable` (~220 470 524 B); também artefacts OrcaSlicer_* / PDB / tests | https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42102469888 |

**NÃO feito:** instalação/QA em Windows 10/11 real pelo agente; impressão; firmware Elegoo/Anycubic; DeviceBridge; assinatura de instalador; coexistência com Orca via `orcaslicer://`.

CI fundação (estático, quality branch): runs documentados no diário; local neste lote: `check_quality_gate.py` → 0 FAIL (foundation); `check_master_plan.py` → 113/113 PASS. Isso **não** homologa G0/G1/G4.

## 3. Estado das ondas e gates

| Onda / Gate | Status | Notas |
|---|---|---|
| 00 Genealogia / **G0** | **PASS DOCUMENTAL** | Critérios REGRA G0 + Onda 00 fechados (rev conflitos .3); G4 Windows separado |
| 01 Fundação / gates fundação | **BLOCKED (t19u)** | só correção/testes; CMP docs PASS, pacote PARTIAL; G-code binário NOT_RUN; G4 NOT_RUN |
| 02 Qualidade Orca nativa | PENDING | |
| 03 Identidade/Home | BLOCKED até G0 PASS + tokens/QA visual (originais **já no repo**) | |
| 04–14 e G2–G6 produto | PENDING / BLOCKED por predecessor | |
| DeviceBridge / 10 marcas | PENDING — `FILUNA/modules/` **inexistente** no tip | |
| Windows hardware G4 | **NOT RUN** | |
| Release G5/G6 | BLOCKED | modo release do checker deve falhar sem dossiê |

## 4. Checklist G0 executada neste lote (05/11/2026, Europe/Rome)

| ID | Item | Resultado | Evidência |
|---|---|---|---|
| G0-01 | Confirmar pin upstream e `upstream-mirror` | **PASS** | ambos `a620d804…823530`; commit Orca 2026-11-05T13:40:41Z |
| G0-02 | Confirmar software-base rebrand e PR #1 | **PASS** (estado) | `filuna/rebrand-1` `2dace0db`; PR https://github.com/abcollicelli-web/FilunaSlicer/pull/1 aberto |
| G0-03 | Confirmar branch/plano quality e PR #2 draft | **PASS** (estado) | tip base deste lote `d1935aef`; PR https://github.com/abcollicelli-web/FilunaSlicer/pull/2 draft |
| G0-04 | Contar manifesto 141 (61 S + 80 W) | **PASS** | regex tabelas = 141; claims batem |
| G0-05 | Verificar blob amostra S `04-MOTORES.md` | **PASS** | blob `f5459558…` size 5443 via API |
| G0-06 | Conflito multi-motor vs Orca único documentado | **PASS** (documental) | MIGRACAO §4 SUPERADO; plano D-FLN-ENGINE-05NOV |
| G0-07 | Build 42102469888 success + artefactos Filuna | **PASS** | installer+portable presentes, `expired:false` |
| G0-08 | Rodar `check_quality_gate.py` foundation | **PASS** estático | 0 falhas; perfis Elegoo Centauri Carbon 2 e Anycubic Kobra 3 V2 presentes como JSON |
| G0-09 | Rodar `check_master_plan.py` | **PASS** estático | 113/113; aviso explícito G0/G1/Windows NÃO homologados |
| G0-10 | `FILUNA/modules/` fabricante | **PASS** como “ainda inexistente” | API 404 — esperado na Onda 00 |
| G0-11 | Binários de identidade/mascote no git | **PASS** (bytes) | 5 originais em `FILUNA/brand/assets/original/` SHA=manifesto; G1 visual ainda PENDING (tokens/QA) |
| G0-12 | Leitura das 141 fontes (profundidade honesta) | **NOT_RUN/BLOCKED** (casos unitários individuais ≠ gate global) | Cobertura path+hash 141; semântica N1 = conjunto obrigatório (não 141); ver G0-FONTES-OBRIGATORIAS |
| G0-13 | SBOM inicial Windows (receitas) | **NOT_RUN/BLOCKED** (casos unitários individuais ≠ gate global) | `FILUNA/sbom/` PARTIAL; completo ainda PENDING |
| G0-14 | Teste Windows real / impressoras | **NOT RUN** | autorização atual: testes Windows ficam para depois |
| G0-15 | Criar este registro + vínculo no plano | **PASS** ao push deste PR | URLs na seção 7 após publish |
| G0-16 | Leitura integral lote prioritário S/W (13) | **PASS** (leitura) | hashes = manifesto; ver inventário G0 |
| G0-17 | Confirmar tip plano final 52/149/SRV/113 | **PASS** (documental) | tip `d1935aef`; 113/113 ≠ G0 funcional |
| G0-18 | Incorporar 5 assets marca do ZIP dono | **PASS** | ZIP `02b3c842…915e`; 5/5 SHA match; nomes estáveis+alias conversa |
| G0-19 | Mapear SRV prioridades dono sem provisionar | **PASS** (inventário) | SRV-00..11; admin sem controle impressoras |
| G0-20 | Consolidar conflitos CF-01..11 com rastreio | **PASS** documental | [G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md](G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md); CF-07 política não-herança |
| G0-21 | Critérios exatos G0 N1–N8 vs REGRA/PLANO | **PASS** (mapa) | N1/N2 PASS neste lote; G0 global ainda PENDING (N4–N7 parcial, Windows NOT RUN) |


**Conclusão G0 (histórico do checklist inicial deste arquivo):** na primeira redação G0 ainda não PASS. **Fato corrente (§15):** G0 **PASS DOCUMENTAL**; Onda 01 contrato PROPOSTO (§16).

## 5. Decisões confirmadas vs dúvidas

**Confirmadas (dono):** Orca motor único 2.5-dev pinado; AGPL público; pasta `Filuna`; rebrand em fork; Elegoo Centauri Carbon 2 + Anycubic Kobra 3 V2 como piloto; `orcaslicer://` a assumir; quality docs em PR draft; iniciar cronograma sobre base atual; documentar continuidade para qualquer IA; Windows físico depois; sem merge automático dos PRs.

**Abertas (não bloqueiam G0):** aceite de escopo Onda 01 (FUND-SPEC); depois implementação fundação. **Fora da Onda 01:** tokens HEX/QA visual (Onda 03); firmware CC2/Kobra e Windows (G4); assinatura instalador (G5); Hub/remoto (SRV). **Assets de marca:** versionados.

## 6. Última tarefa e próxima ação exata

- **Última tarefa (2026-11-05 23:45:00 CET / 2026-11-05T22:45:00Z):** gates G2/G3 globais corrigidos para NOT_RUN/BLOCKED; normalizador/evidence adversarial; `filuna-windows.yml` + runner baseline binário; tentativa de execução Windows sob demanda.
- **Próxima ação:** acompanhar run baseline/pin-build; publicar evidence.json + hashes; G4 permanece NOT_RUN; sem Home/SRV/DeviceBridge.


## 7. Commits / PRs deste lote (preencher no push)

| Campo | Valor |
|---|---|
| Branch | `filuna/continuidade-g0-20261105` |
| Base no momento da criação | `d1935aef` (`filuna/quality-master-20261105`) |
| Commit tip (checkpoint) | último commit de `filuna/continuidade-g0-20261105` (PR #3) |
| PR draft | [#3](https://github.com/abcollicelli-web/FilunaSlicer/pull/3) |
| Arquivos tocados | `FILUNA/REGISTRO-CONTINUIDADE-ETAPAS.md` (novo); `FILUNA/PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md`; `FILUNA/README.md`; `FILUNA/DIARIO-QUALIDADE-2026-11-05.md`; `AGENTS.md` |

## 8. Comandos executados neste lote (amostra)

```text
gh auth status   # ok, conta abcollicelli-web; token omitido
gh api .../upstream-mirror → a620d804…
gh api OrcaSlicer/.../a620d804… → confere mensagem/data
gh run view 42102469888 → success; artefacts Filuna installer+portable
gh api .../FILUNA/modules?ref=filuna/rebrand-1 → 404
python3 FILUNA/tests/check_quality_gate.py → 0 FAIL foundation
python3 FILUNA/tests/check_master_plan.py → 113/113 PASS estático
```

## 9. Checkpoint

Persistido neste arquivo no GitHub. Qualquer IA deve atualizar **este mesmo arquivo** (append de seção datada) a cada lote — não criar segundo “registro mestre” paralelo. Diário de qualidade permanece para narrativa QA; o plano mestre permanece normativo; este registro é o **estado operacional executável**.


## 10. Checkpoint lote leitura prioritária (2026-11-05 22:55:50 CET)

- Plano final remoto: `d1935aefa91a1eda86f57b67b342a3857247294e` — sem tip mais nova; PR #3 só acrescenta continuidade/G0.
- Anúncio dono t18u alinhado ao DAG/auditoria do tip (52/149/12 SRV; 113/113 documental ≠ G0 PASS).
- Evidência: [G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md](G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md).
- Admin Filuna sem controle de impressoras reafirmado; SRV inventariado, não provisionado.
- G0 continua **EM ANDAMENTO**. *(histórico do checkpoint 10; superado pelo §15 PASS DOCUMENTAL)*


## 11. Checkpoint assets de marca (2026-11-05 23:00:49 CET)

ZIP dono `01M6PVH7P1FCG8M8SC5MB3JEB9.zip` (sha256 `02b3c8426ebdf481754f3658c42a147ce915906f52d36f2476032deeb4f2915e`), 711+ KB.
Validação segura (zipfile, sem traversal/symlink): **5/5 SHA-256 = manifesto**. Nomes no ZIP estáveis; alias de conversa preservados via hardlink.
Incorporados **somente** os 5 originais em `FILUNA/brand/assets/original/` (ZIP e `MANIFESTO-IMAGENS.json` extras **não** commitados).
**G1 não PASS:** faltam tokens HEX homologados, derivados e QA Windows. G0 continua independente da marca.


## 12. Checkpoint leitura manifesto 141 (2026-11-05 23:02:32 CET)

- 141/141 caminhos do manifesto cobertos por leitura de conteúdo (texto); 9 pares S↔W deduplicados por SHA.
- Inventário: [G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md](G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md) rev .3.
- G0 **ainda não PASS** (SBOM completo, aceite de conflitos herdados, evidências Windows).

## 13. Checkpoint conflitos + SBOM parcial (2026-11-05 23:07:34 CET)

- Corrigida honestidade: **não** 141 leituras integrais; classes A/B/C no inventário .4.
- Conflitos: [G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md](G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md).
- SBOM inicial parcial: [sbom/README.md](sbom/README.md) + CDX parcial.
- *(histórico §13)* N1/N2 depois fechados; G0 PASS DOCUMENTAL no §15. Sem salto para Home/SRV.


## 14. Checkpoint N1/N2 semântico (2026-11-05 23:12:40 CET)

- Definição N1 publicada: [G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md](G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md) — **não** exige 141 cláusula-a-cláusula.
- ≥19 fontes COVER normativas + reconfirmação §2 promovidas/lidas com SHA=manifesto (`gh api` raw + `git hash-object`).
- Conflitos rev .2: N1 **PASS**, N2 **PASS** (CF-07 lista BLOQUEANTE W + não-herdar).
- REQ novos: UPD-03/04, PEI-03, DEV-01..03, PROD-01, CAT-03, SEC-03, BIZ-02, ENG-04 SUPERADO, HOME-01 FORA_G0.
- Checks: `check_master_plan.py` 113/113; `check_quality_gate.py --mode foundation` 0 FAIL (estático ≠ G0 funcional).
- **Pergunta única prioritária ao dono (opcional):** confirma CF-07 — nenhum BLOQUEANTE do legado W 0.2.x é critério G0 do FilunaSlicer atual, salvo REQ que você reabrir explicitamente? (exceção candidata já mapeada: Anycubic start → DeviceBridge Onda 06, não G0.)
- G0 global **PENDING**. Sem merge PR #3.

## 15. Checkpoint G0 PASS DOCUMENTAL (2026-11-05 23:24:00 CET)

- Corrigida checklist derivada: **não** exige QA Windows (G4) nem SBOM completo (Onda 00 = SBOM inicial).
- Estados só PASS/FAIL/BLOCKED/NOT RUN.
- CF-07 **RESOLVIDO** por hierarquia REGRA (legado W ≠ homologação F); sem nova pergunta ao dono.
- Evidências: `G0-MAPA-141-STATUS`, `G0-MIG-DAG-ALINHAMENTO`, `G0-RISCOS-OEM-PROVENIENCIA`, `G0-TESTES-CASOS`, conflitos rev .3.
- Checks: `check_master_plan.py` 113/113; `check_quality_gate.py --mode foundation` 0 FAIL (Linux).
- Escopo do PASS: inventário/genealogia apenas. **Próximo:** Onda **01 Fundação** (FUND-SPEC), não Onda 03 visual.

## 16. Checkpoint Onda 01 FUND contrato (2026-11-05 23:30:00 CET)

- Docs: [FUND-SPEC-ONDA01-2026-11-05.md](FUND-SPEC-ONDA01-2026-11-05.md), [FUND-REQ-ONDA01-2026-11-05.md](FUND-REQ-ONDA01-2026-11-05.md), [FUND-ADR-BASELINE-DADOS-2026-11-05.md](FUND-ADR-BASELINE-DADOS-2026-11-05.md), [FUND-TEST-ONDA01-2026-11-05.md](FUND-TEST-ONDA01-2026-11-05.md).
- 1º nó elegível: **LEGAL-02** (deps GOV-00/SRC-01); em seguida **QA-03** → CORE-04 → DATA-05.
- G1 visual/tokens/Home **não** são o próximo passo (Onda 03).
- **G1/G2/G3 fundação:** não marcados PASS; SPEC estado PROPOSTO.
- **Pergunta única ao dono:** homologa o escopo FLN-FUND-SPEC-2026-11-05.1 para autorizar implementação mínima §9 do SPEC?
- Correções: plano §3.2 marca no git; §12 cobertura sem “G0 EM ANDAMENTO” corrente; índice 0d/0e/0f/0g.

## 17. Checkpoint implementação FUND pós-aprovação (2026-11-05 23:35:00 CET)

- **Aprovação dono:** 2026-11-05 23:24 Europe/Rome / 22:24 UTC · widget t18s14 · texto em `FILUNA/approvals/FUND-SPEC-2026-11-05.1-owner-approval.json`.
- **G1 contrato fundação:** escopo autorizado **sem** mudança visual.
- **Implementado:** CMP LEGAL-02, evidence schema QA-03, gcode normalize/compare + fixtures, DATA-05 isolation tests, workflows foundation+manual.
- **Resultados box Linux:** unittest **19 OK**; CMP **7/7**; DATA **8/8**; master **113/113**; foundation **0 FAIL**.
- **NOT_RUN / BLOCKED:** comparação G-code Orca pin↔Filuna com artifact Windows; AppData/registro real; impressoras; release.
- **Adversarial:** confirmar que sample_c (X21) falha compare (exit 1); meta-only passa; schema rejeita G4 PASS sem hardware; `libslic3r` não diffado neste lote.
- Base pré-lote: `5349c06ff94953268a6690472d2b3af399296ad6`.

### Push/commit lote (2026-11-05 23:40:00 CET)

- Commit: `7ee410ec25e65d85727b96f021cadb38f11970e0` (`7ee410ec`)
- Branch: `filuna/continuidade-g0-20261105` · draft PR #3
- Base anterior: `5349c06f`
- Sem merge/force-push/rebase.

- CI: `filuna-foundation-check` agora também dispara em push `filuna/continuidade-*` (paths FILUNA); base/default/merge intocados.

### CI foundation no tip (2026-11-05 23:42:00 CET)

- Tip remoto = local: `17112fae551bc8510e062d9583f9e6f1cddfb5de` (`17112fae`)
- Actions: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42121804935 — **success** (foundation + CMP + DATA + unit + release fail-closed)
- Draft PR #3: https://github.com/abcollicelli-web/FilunaSlicer/pull/3 (sem merge)
- Próxima ação: baseline G-code **binário** pin↔Filuna Windows permanece **NOT_RUN** até artifacts/job dedicado; G4 físico intacto; sem Home/DeviceBridge/SRV.


## 18. Baseline binário Windows — lote (2026-11-05 23:45:00 CET / UTC 2026-11-05T22:45:00Z)

- G1 fundação = escopo aprovado (t18s14). **G2/G3 globais = NOT_RUN/BLOCKED** (19 unit Linux ≠ gate global).
- Fontes: Orca run `42037889437` @ `a620d804…`; Filuna run `42102469888` @ `2dace0db…`.
- Workflow: `filuna-windows.yml` input `baseline-gcode-only=true` despachável `--ref` desta branch.
- Sintético ≠ binário; G4/instWindows NOT_RUN.

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


## 23. Onda 01 — regressão adversarial da LAN virtual (06/11/2026)

**Ambiente:** Ubuntu VPS, bridge isolada 10.77.0.0/24; Wine/CLI e testes do motor são trilhas separadas. **Escopo exclusivo: emuladores.**

- Primeira execução adversarial: **6 PASS, 1 FAIL**. A Elegoo CC2 virtual aceitava comando MQTT desconhecido `987654` com `error_code=0`; falha registrada, não ocultada.
- Correção: `FILUNA/qa/virtual-lab/lab.py` passa a devolver `error_code=404` para método não suportado, sem alterar o motor Orca nem dispositivos físicos. Commit `a40553801ce1de5ab65ae56643ebe919b28cc500`.
- Reexecução VPS: **7 PASS, 0 FAIL** (senha incorreta, DISCONNECT, pacote MQTT inválido, dois uploads acima do limite, endpoint desconectado, comando desconhecido). A suíte original continuou **14 PASS, 0 FAIL, 6 NOT_RUN**.
- CI obrigatória: [run 42134823573](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42134823573), workflow `filuna-virtual-lan-regression.yml`, job `protocol-lab` SUCCESS, inclusive etapa adversarial e limpeza da LAN. Evidência [JSON](https://github.com/abcollicelli-web/FilunaSlicer/blob/filuna/qa-gates-hardening-20261105/FILUNA/qa/virtual-lab/adversarial-results-2026-11-06-vps.json).
- **Limite:** PASS aqui significa resposta correta do emulador. Não demonstra que o código Filuna conectou, que o firmware real responde assim, nem que TLS/FTPS/WS foi implementado. Nenhum portão G3/G4 liberado.
- **Próxima etapa obrigatória:** ampliar Bambu MQTT/TLS e FTPS, Elegoo original SDCP/WS e Moonraker WS; criar testes de injeção de falha de transferência e, só depois, executar o cliente Filuna/DeviceBridge sob Wine. Manter a distinção `PASS MOCK` / `PASS FILUNA` / `PASS HARDWARE`.



## 24. Diretriz do proprietário — simulação parcial e homologação física final (06/11/2026)

**Decisão:** onde houver dependência de impressora física, simular em laboratório e classificar exclusivamente como `PARTIAL_SIMULATED`; reservar a prova real para homologação final. Avançar tarefas independentes da integração física, sem declarar G3/G4 aprovados, sem alterar regras de segurança, sem liberar comandos remotos nem publicação.

**Execução:**
- Novo `FILUNA/qa/virtual-lab/physical_homologation.py`: lê os resultados efetivamente executados (14 PASS mock + 7 PASS adversariais), verifica consistência e gera JSON com 30 casos físicos `NOT_RUN`, separados por cinco famílias.
- Executado na VPS: Bambu A1, Elegoo Carbon original, Elegoo Carbon 2 e Moonraker = `PARTIAL_SIMULATED` (escopo parcial de protocolos); OctoPrint = `NOT_RUN` (nenhum emulador ainda).
- `filuna_devicebridge_tested=false`; `release_g3=BLOCKED`, `release_g4=BLOCKED`; `physical_checks_pass=0`.
- CI `filuna-virtual-lan-regression.yml` agora gera o checklist automaticamente e verifica que a homologação física não pode ser marcada PASS por dados simulados.
- **Próximos passos:** implementar emulador OctoPrint; ampliar MQTT/TLS, FTPS e WebSocket com certificados de teste; conectar DeviceBridge do Filuna no Wine; registrar PASS/FAIL/PARTIAL de cada protocolo; deixar ensaios reais de firmware, autenticação, monitoramento, comandos e transferência na fila de homologação final.

**Regra de continuidade:** avançar em implementação e testes sintéticos é permitido; aprovar gate de integração/hardware ou release com evidência sintética é proibido.


## 25. Onda 01 — OctoPrint simulado e automação (06/11/2026)

- Implementado `FILUNA/qa/virtual-lab/octoprint_mock.py`, **somente loopback 127.0.0.1**, sem acesso a impressoras reais, chave sintética `FILUNA_LAB_ONLY_KEY`.
- Implementado `test_octoprint_mock.py` com 11 verificações: ausência/chave incorreta rejeitadas, identificação, leitura de estado, consulta de trabalho, comando desconhecido rejeitado, iniciar/pausar/cancelar simulados, mudança de estado e endpoint inexistente.
- Execução na VPS: **11 PASS / 0 FAIL**, exclusivamente do emulador HTTP. Nenhuma transferência de arquivo OctoPrint foi implementada; não alegar esse PASS.
- O workflow `filuna-virtual-lan-regression.yml` passou a executar os 11 testes OctoPrint e anexar JSON de evidência. Verificar o status da nova execução antes de marcar CI PASS.
- **Classificação:** `PARTIAL_SIMULATED` para o subconjunto HTTP OctoPrint; DeviceBridge/Filuna Wine e firmware real continuam `NOT_RUN`. A matriz física gerada permanece com 30 ensaios reais `NOT_RUN`; G3/G4 `BLOCKED`.
- **Próximo:** completar os protocolos WS/TLS/FTPS e transferência OctoPrint no laboratório; depois testar o cliente Filuna real no Wine. Em paralelo podem avançar tarefas sem dependência de hardware.


## 26. Regra do proprietário — perfis oficiais + Orca + camada Filuna (06/11/2026)

**Decisão normativa:** a compatibilidade de hardware do Filuna deve utilizar **dados oficiais dos fatiadores dos fabricantes**, combinados com o motor Orca e a camada de integração Filuna. Não homologar impressora com perfil genérico aproximado quando existir referência oficial adequada. Respeitar licenças, proveniência e versão do firmware/perfil; não redistribuir componentes proprietários sem permissão.

**Método por impressora:**
1. Obter e registrar versão, origem e licença dos perfis/configurações e fatiador oficial do fabricante; preservar checksum e associação com modelo, revisão de hardware e firmware.
2. Mapear no Orca os limites e os comandos do perfil oficial (mesa, bico, material, temperaturas, velocidades, aceleração, G-code inicial/final, purga, ventilação e comandos especiais), sem assumir equivalência de semântica.
3. Gerar o mesmo modelo 3D nos dois fatiadores com a mesma impressora, bico, material, altura de camada e parâmetros equivalentes. Comparar movimentos, extrusão, temperaturas, ventilação, acelerações, limites, comandos específicos e simulação visual; divergências exigem explicação e teste.
4. Emular o protocolo e a interpretação de comandos em laboratório, registrando `PARTIAL_SIMULATED` quando aplicável; não promover automaticamente a homologação física.
5. Registrar checklist de firmware e impressora real para homologação final do proprietário.

**Primeiro alvo:** Elegoo Centauri Carbon 2. O G-code gerado anteriormente com perfil **Bambu P1S** não constitui referência de Elegoo nem comparação oficial. O comparador `FILUNA/qa/gcode/elegoo_official_trace_compare.py` é apenas ferramenta inicial de análise offline; bloqueia comandos desconhecidos, trajetórias fora de limites e proveniência sintética, e **não substitui simulador real de firmware nem prova de segurança**. A primeira execução em G-code Bambu P1S registrou comandos proprietários e limites que impedem equivalência; isso é resultado de diagnóstico, não falha homologada da Elegoo.

**Portões:** comparação documental e sintética podem avançar como `PARTIAL_SIMULATED`; homologação final de hardware permanece `PENDING` até ensaio real com firmware e dispositivo identificados.


## 27. Auditoria Elegoo CC2 — fonte oficial fixada e comparação dos perfis (06/11/2026)

- Referência real do fabricante localizada: `elegooofficial/ElegooSlicer`, commit `2d507e39a96ab9562b35d06b23ef5df8fe613297` (23/09/2026), perfil Centauri Carbon 2 bico 0,4 mm.
- Três arquivos oficiais foram comparados com o Filuna: máquina, processo 0,20 mm Standard e filamento Elegoo PLA.
- **Oito divergências:** seis na máquina (`setting_id`, tempos de carga/descarga, `default_bed_type`, `support_mms`, `support_multi_filament`), uma no `setting_id` do processo e uma no `setting_id` do filamento. **Não alterar valores automaticamente sem validar herança e semântica.**
- Comandos oficiais específicos identificados: `M6211`, `G180`, `BED_MESH_CALIBRATE`, `SET_PRINT_STATS_INFO`. Comparador genérico precisa tratá-los explicitamente; não chamar comandos específicos de inválidos por padrão.
- Relatório: [ECC2-OFFICIAL-PROFILE-AUDIT-2026-11-06.md](https://github.com/abcollicelli-web/FilunaSlicer/blob/filuna/qa-gates-hardening-20261105/FILUNA/qa/gcode/ECC2-OFFICIAL-PROFILE-AUDIT-2026-11-06.md).
- Estado: **PARTIAL_PROFILE_AUDIT**, não é equivalência de G-code; binário oficial ElegooSlicer e G-code independente ainda `NOT_RUN`. Homologação física `PENDING`.
- Próxima etapa: validar herança dos perfis e, no laboratório Wine, executar **ambos** os fatiadores sobre o mesmo modelo/mesmos parâmetros para gerar G-codes independentes e comparar simulação por camada, sem usar o G-code Bambu como referência Elegoo.

## 28. Regra oficial: Orca + adaptadores Filuna por fabricante — 06/11/2026

O proprietário reafirma que o Filuna não pode ser apenas um Orca renomeado com perfis genéricos. Orca segue motor-base; integrações por fabricante/modelo implementam descoberta, identificação, autenticação, estado, câmera, comandos, transferência, filamento, recursos, perfis, G-code e segurança. Fontes oficiais são prioritárias; código oficial só pode ser reutilizado após auditoria de licença/AGPL e dependências. Onde a licença não permitir cópia, criar implementação interoperável própria baseada em documentação/APIs permitidas.

**Especificação versionada:** [FILUNA/ARQUITETURA-COMPATIBILIDADE-FABRICANTES.md](https://github.com/abcollicelli-web/FilunaSlicer/blob/filuna/qa-gates-hardening-20261105/FILUNA/ARQUITETURA-COMPATIBILIDADE-FABRICANTES.md). Famílias: Bambu, Elegoo, Creality, Flashforge, QIDI, Moonraker, OctoPrint. Compatibilidade e homologação são POR MODELO/FIRMWARE, não por marca inteira.

**Evidência CC2 agora executada**, corrigindo o estado da etapa 27: ElegooSlicer oficial v1.5.3.5 portátil Windows (SHA256 ZIP `0215cd958d6a52ae1bd33a5ad7509ecd1b156baf63fbe04333e04cf0305793e5`), DLL oficial `cb5d66cc79b1b8d9940d2cb2a0fad287d2400457f6a1a1dd50830c4b066305b3`; Filuna DLL `f0914911be85fbd1809a739f5b1865e82dd4f6708f5b3f5645476f72b092aa9f`. Dois executáveis distintos geraram G-code com sucesso no Wine com CC2 0.4, PLA Elegoo, 0.20mm e OBJ 20mm (4 runs incluindo inversão de perfis). **Diferenças persistem:** 5.424 vs 7.532 G1; 209.800 vs 255.165 bytes; 428 vs 201 `SET_VELOCITY_LIMIT`; M204 S1000 vs S5000. Classificação **PARTIAL_COMPARISON_MISMATCH**, sem equivalência e sem homologação física.

**Próxima execução:** comparar parâmetros efetivos após herança, avançar um adaptador CC2 sobre o código real do DeviceBridge (não apenas emuladores) e testar no Wine; posteriormente aplicar o mesmo contrato aos demais fabricantes. G3/G4 permanecem bloqueados para promoção/release, sem impedir desenvolvimento independente ou simulação parcial.


## 29. Inicialização adaptativa + atualização modular por fabricante — 06/11/2026

**Ordem do proprietário:** ao abrir Filuna, reconhecer modelo e firmware do cliente, resolver automaticamente módulo compatível, aproveitar código e recursos livres do fatiador oficial somados ao Orca interno. Atualizações futuras de fatiadores devem gerar incrementos em módulos/perfis, sem refazer o aplicativo completo ou exigir instalador pesado, salvo necessidade técnica comprovada.

**Contrato de projeto:** [FILUNA/MODULOS-FABRICANTES-ATUALIZACAO-INTELIGENTE.md](https://github.com/abcollicelli-web/FilunaSlicer/blob/filuna/qa-gates-hardening-20261105/FILUNA/MODULOS-FABRICANTES-ATUALIZACAO-INTELIGENTE.md). Inclui identificação, manifesto com versão/ABI/firmware/hashes/assinatura, atualizações por modelo, verificação de regressão de G-code, rollback, funcionamento offline e restrição contra uso de serviços/login próprios de fabricante.

**Código já incorporado na branch QA:** registro nativo dos quatro modelos, seletor `FilunaModuleSelection.hpp` por modelo + ABI e consulta no `DeviceManager.cpp` ao restaurar dispositivo. **Teste local:** compilação C++17 de seletor com 4 casos de seleção, incompatibilidade ABI rejeitada e operações de hardware bloqueadas por ausência de comprovação. **Status PARTIAL_NATIVE_IMPLEMENTATION**; *não* declarar carregamento de módulos baixados, integração real de protocolos, comparação de G-code das quatro máquinas ou compilação Windows homologados.

**Próximos bloqueadores concretos:** resolver identidade e firmware reais, portar componentes OEM AGPL com rastreabilidade, integrá-los ao DeviceBridge existente, construir atualizador assinado/rollback, compilar Windows e rodar a matriz real/simulada. Sem merge, deploy ou comando em impressora física.


## ADENDO VINCULANTE — POLÍTICA UNIVERSAL DE DISPOSITIVOS E MÓDULOS OEM (06/11/2026)

**Decisão expressa do proprietário: aplica-se a TODAS as impressoras, marcas, famílias e futuros adaptadores Filuna.** Preservar a aplicação Windows Filuna Slicer já compilada, a GUI e o motor-base Orca nativo. Não iniciar outro aplicativo de fatiamento instalado nem popular a lista de impressão com todos os presets disponíveis.

**Ordem de execução obrigatória por identidade/firmware:**
1. Dispositivos descobre a impressora por protocolos e portas locais suportados/documentados (mDNS/SSDP/UDP, HTTP, MQTT/MQTT-TLS, WebSocket etc. conforme modelo). Porta aberta não implica acesso não autenticado. Detectar capacidades de maneira permitida; sem scan amplo indiscriminado e sem autenticação presumida.
2. Verificar fabricante/modelo/revisão/firmware e autorização/pareamento exigidos; persistir registro privado do cliente, credenciais no cofre apropriado e relação módulo+ABI+versão. Sem dispositivo físico não alegar descoberta real, conexão, firmware ou ACK.
3. **Primeiro aproveitar os protocolos e recursos de rede disponíveis** para descoberta/telemetria/transferência/comandos suportados, com a autenticação que cada serviço exigir. **Somente quando esses recursos forem insuficientes**, tentar o modo LAN com código fornecido pela impressora, quando o firmware permitir. LAN Code pode ser credencial do próprio MQTT/FTPS, não necessariamente um segundo transporte; não contornar autenticação, certificado, firmware ou controles privados.
4. Preparar mantém GUI Orca; ao selecionar um equipamento registrado, deve ativar implementação OEM compatível **efetivamente conectada ao pipeline**, com capacidades, macros, perfis e código livre reutilizável conforme licença por arquivo. Seleção de um ID ou preset JSON, isoladamente, não cumpre a regra. Se recurso incompatível/fechado, declarar indisponibilidade ou fallback identificado, sem equivalência presumida.
5. Imprimir oferece somente máquinas registradas; offline permanece rotulada offline. Enviar apenas com impressão autorizada, integridade de arquivo e ACK real. Nenhum comando de impressão automática na descoberta.
6. Atualizar módulos/firmware-matrix/perfis independentemente quando tecnicamente possível, via manifesto assinado, SHA, versão, ABI, rollback e reinício seguro quando necessário; motor-base Orca permanece acessível offline sem assinatura.

**Primeiro piloto:** Bambu Lab A1. **Próximos:** Flashforge AD5X, Anycubic Kobra 3 V2 (não confundir com nome informal Kobra V3 sem confirmação de identidade) e Elegoo Centauri Carbon 2. A política também vale às demais marcas e novos dispositivos.

**Estado de prova:** apenas seleção candidato por modelo/ABI em C++ e emuladores parciais; nenhum módulo OEM completo comprovado no pipeline Preparar. Wine CLI não valida GUI/firmware/hardware. Não declarar gates G2-G6 PASS nem release; preservar evidências e testes negativos.


## Decisão vinculante adicional — atualizações diferenciais OEM/Orca (06/11/2026)
Por ordem do proprietário, a política válida para **todos os fabricantes e modelos** é [REGRA-MESTRE-ATUALIZACAO-INCREMENTAL-OEM-ORCA-2026-11-06.md](REGRA-MESTRE-ATUALIZACAO-INCREMENTAL-OEM-ORCA-2026-11-06.md). Em atualizações Bambu Studio, Orca ou outros OEM, comparar SHA/versão/licença por arquivo, reutilizar apenas diferenças necessárias, compilar, testar a ligação real com Preparar, G-code oficial inalterado, rede simulada, Windows e hardware. Sem promover módulo não testado e sem update ou merge automático. Primeiro piloto A1: cinco macros já idênticos, port C++ do candidato matemático testado isoladamente, integração ao pipeline **NOT RUN**, G-code oficial×Filuna **MISMATCH**. A atualização de fontes é um alerta para revisão, nunca aprovação.


## Evidência de incorporação parcial real — Bambu A1 / N2S (06/11/2026)
- Fonte oficial: `bambulab/BambuStudio/resources/printers/N2S.json` (blob `3a5929b98244d7800ee06a7fe41662f0141306ce`) identifica **Bambu A1 = N2S**, SN prefixo 039. Snapshot mantido em `FILUNA/fabricantes/bambu/modelos/a1/N2S.json`, sem habilitar campos de firmware automaticamente.
- Código de descoberta SSDP nativo `FILUNA/fabricantes/bambu/transport/BambuLanDiscovery.hpp` valida `N2S`, prefixo 039 e resposta do protocolo. Entrada C++ `engine/BambuA1Module.hpp` compartilhada por registro de Dispositivos e Preparar via `FilunaOEMOverlay.hpp`; correções no `DevManager.cpp` e chamada em `Plater.cpp::reslice`. Ambos os caminhos **inseridos no código-fonte**, sem UI substituta nem outro fatiador instalado.
- Teste C++17 compilado e executado no Ubuntu: módulo/inicialização Dispositivos-Preparar PASS; resposta UDP real do **emulador LAN** 10.77.0.12:1990 → 10.77.0.1:2021, N2S serial 039LAB0000001 PASS. Esta prova não confirma compilação Windows integral nem impressão física.
- `FILUNA/qa/virtual-lab/bambu_a1_side_by_side.py` rodou no servidor e produziu `/var/tmp/filuna-qa-wine/bambu-side-by-side-20261106/result.json`: seleção nativa PASS, SSDP emulado PASS, **G-code Bambu Studio oficial anterior vs Filuna Orca anterior MISMATCH**, sem recompilar aplicativos nessa comparação. Estado global HOMOLOGATION_BLOCKED.
- Auditor `FILUNA/qa/verify_bambu_a1_capabilities.py` encontrou 5 campos diferentes entre a matriz oficial N2S e a matriz runtime Orca (AMS externo, timelapse, posição de detecção, imagem, SN prefixo). Nenhuma capacidade foi ativada automaticamente por causa deste delta.
- Geração de arquivos de impressão equivalente ao binário oficial e transporte MQTT/TLS 8883, FTPS 990, autenticação com código LAN, envio e ACK físico ainda **NOT RUN** no Filuna integrado. Na Bambu, o LAN Code é a credencial exigida pelos serviços MQTT/FTPS quando ativados, não um transporte independente. Sem credencial real fornecida pela própria impressora, não afirmar pareamento.
- Workflow dedicado `.github/workflows/filuna-oem-windows-qa.yml` foi registrado para tentar compilar a branch candidata; **não considerar build PASS até receber resultado verificável**. Nenhum merge/deploy autorizado ou executado.


## PONTO DE RETOMADA OBRIGATÓRIO — 06/11/2026
Para continuar a arquitetura Orca único + módulos completos apenas de diferenças OEM, começando pela Bambu A1, **abrir primeiro** `FILUNA/HANDOFF-MESTRE-BAMBU-A1-ORCA-OEM-2026-11-06.md`. Contém comandos, artefatos, testes realmente executados, números de comparação, proveniência, PR #5, pendências de compilação, escopo por fabricante e regra para não chamar um teste simulado de homologação real. A tarefa prioritária é concluir integração C++ real no pipeline e fatiamento novo Filuna x Bambu Studio oficial, medir tamanho final incorporado e confirmar cobertura da linha Bambu.


## REGRA PRIORITÁRIA MAIS RECENTE — 06/11/2026
O proprietário exigiu **100% do módulo OEM de REDE em Dispositivos e 100% do módulo OEM de FATIAMENTO em Preparar**, com o Orca apenas como núcleo comum único; são **dois módulos distintos, completos e selecionados automaticamente pelo dispositivo/firmware**, nunca fatiadores duplicados. A regra normativa integral está em `FILUNA/REGRA-PRIORITARIA-100-PORCENTO-MODULOS-OEM-REDE-FATIAR-2026-11-06.md`. **Prevalece sobre descrições anteriores de seleção ou módulos parciais.** A Bambu A1 atual NÃO está 100% integrada em nenhuma das etapas; não declarar conclusão por PASS isolado.


## GOVERNANÇA FINAL — ARQUITETURA PERMANENTE 3 CAMADAS, CONTRATO OEM, M1–M5/U1–U8 — 06/11/2026
**Documento normativo prioritário:** `FILUNA/REGRA-MESTRE-PERMANENTE-3-CAMADAS-CONTRATO-OEM-E-ATUALIZACOES-2026-11-06.md`. Camada Orca oficial imutável; Camada Filuna só acessa OEM via contrato versionado; camada fabricante com código oficial por marca/versão, isolamento e adaptador fino. Qualquer atualização Orca/OEM gera M1–M5, PR e conformance; só libera com PASS, conserva anterior. Código OEM não alterado, patches inevitáveis justificados; contrato retrocompatível e capacidades ausentes bloqueadas na UI. **Modificações OEM anteriores em Orca são débito de migração, NÃO nova arquitetura homologada**. G-code Bambu E5/B: comparar G-code real, SHA bruto + normalizado removendo somente data/hora do HEADER_BLOCK e registrando linha; ZIP 3MF por arquivos; cache deps oficial por hash. Último run observado `42165326354` QUEUED, B/C sem evidência PASS. PR #5 draft. Ler doc normativo + `FILUNA/HANDOFF-MESTRE-BAMBU-A1-ORCA-OEM-2026-11-06.md` ao iniciar novo chat.


## REGRA FUTURA DA FASE BETA — MONITORAMENTO AUTOMÁTICO NO SERVIDOR PARA HOMOLOGAÇÃO FÍSICA (07/11/2026)

**Nova decisão expressa do proprietário:** criar, para **quando abrir o beta vigiado**, uma ferramenta Filuna com coletor no aplicativo/DeviceBridge e **painel de homologação física no servidor Filuna**. A cobertura será automática e universal por fabricante/modelo/firmware/módulo, não apenas um modelo em whitelist: observar programa e arquivos abertos, perfis, fatiamento, AMS/multimaterial, G-code gerado, paridade com oficial, transferência/ACK e — somente quando o protocolo permitir — G-code recebido/executado; acompanhar início, progresso, erros, desconexão, cancelamento e finalização da impressão física. Relacionar eventos por job e reunir dossiê de evidências/alertas para aprovação manual de cada modelo e capacidade.

**Norma completa:** [REGRA-BETA-MONITORAMENTO-HOMOLOGACAO-FISICA-20261107.md](REGRA-BETA-MONITORAMENTO-HOMOLOGACAO-FISICA-20261107.md); vinculada a `REGRA-FASES-HOMOLOGACAO-INTERNA-BETA-FISICA-20261107.md` e `ARQUITETURA-SERVIDOR-DADOS-REMOTO.md`. O servidor do Filuna será **somente observador**; sem credenciais OEM, controle remoto administrativo, impressão automática, câmera ou envio indiscriminado de projetos privados. Opt-in granular dos participantes beta, revogação, criptografia, minimização e segregação são bloqueadores da ativação. Término de impressão não equivale a peça homologada; requer evidência física/revisão e aprovação do proprietário.

**Situação atual:** `BETA_MONITORAMENTO_FISICO = PLANEJADO_DOCUMENTADO_NAO_IMPLEMENTADO`; `BETA_ABERTO = NAO`; `COLETA_REAL = DESATIVADA`. Não confundir a criação deste registro com serviço ativo, testes reais ou homologação concluída. Abertura do beta somente depois dos gates internos, build final e prontidão da ferramenta, sem antecipar hardware.


## CONTINUIDADE OEM PRIORITÁRIA — SNAPMAKER + ANYCUBIC + FLASHFORGE (07/11/2026, lote de QA)

**Ordem do proprietário:** avançar estas três marcas para fechar o máximo de QA interno sem hardware, **sem instalar/compilar motores OEM nem liberar build final**. Todos os modelos/perfis/protocolos precisam de descoberta automatizada e módulo OEM completo; piloto com dois modelos NÃO define whitelist.

### Snapmaker U1
- Crise real do executável oficial `2.4.1` identificada e isolada: `wipe_tower_filament` presente em processo sem `nozzle_diameter`, acesso inválido durante `normalize_fdm` da CLI. Workaround temporário genérico usando bicos herdados do OEM gerou G-code real A/B idêntico (runs 42316614470, 42316895543, 42317564346).
- **Patch de fonte completo e válido**, somente `PrintConfig.cpp`, contra upstream commit `bac6fd5c1e1c23ddf057cf6471ba4ed7ed621c4c`; GitHub Actions [#42319819540](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42319819540) `SUCCESS`: apply/check/revert limpa, sem compilação.
- Inventário integral de fonte e perfis automatizado. **Pendentes:** port de código nativo à arquitetura OEM Filuna, testes do gateway/controle/ACK real em simulador, comparação G-code Filuna×OEM, explicação de variância de trajetórias entre dois workflows diferentes. `HOMOLOGACAO_INTERNA=BLOCKED`.

### Anycubic Slicer Next 2.0.0.6
- Origem oficial .deb pinada SHA `fdb134a4e11fad7e795c8d425520ce1305321ccc0bf1375a651a1513f8854678`; libcloud_mqtt carregada **somente da pasta original do próprio pacote** (não instalada nem distribuída no Filuna).
- [A/B real dois modelos #42319417880](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42319417880) `SUCCESS`: **Kobra S1 0,4/PLA 210.094 bytes**, SHA bruto idêntico nas duas passagens; **Kobra 3 V2 0,4/PLA 325.731 bytes**, SHA bruto idêntico. [Catálogo #42319568616](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42319568616) `SUCCESS`: **40 máquinas, 191 processos, 382 filamentos, zero ancestrais ausentes/ciclos** na árvore original do pacote.
- Inventário código-fonte Git oficial `6103ed8b...`: 19 máquinas, 42 processos, 45 filamentos; **diverge** do pacote .deb v2.0.0.6. 65 itens para revisão de código/dependências. [CI fonte #42319915138](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42319915138). Não misturar fontes e perfil de versões diferentes nem presumir módulo de rede/cloud distribuível.
- `OEM_AB_2_SAMPLE=PASS`; `FULL_NATIVE_FILUNA_PARITY`, `NETWORK_ACE_ACK`, `MATERIAL_MULTICOLOR` = `NOT_RUN`; `HOMOLOGACAO_INTERNA=BLOCKED`.

### Flashforge Flash Studio
- **Versão 1.7.20 Windows atual é um instalador EXE**, mesmo que o botão oficial traga texto `win64.zip`. O pacote 1.7.15 original já teve SHA verificado mas não entregou executável+resources portátil pela extração 7z/Inno; nenhum PASS CLI 1.7.15.
- Alternativa **portátil Linux oficial anterior 1.7.9** da release GitHub `Flash.Studio_ubuntu24.04_V1.7.9.AppImage`, SHA `96578656466416adc1ccd2e7ec656a0b217560edadeeb29642259e67922adf45`. [CI original #42320238666](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42320238666) `SUCCESS` de download/hash, AppRun e inventário **59 arquivos máquina, 130 processo, 636 filamento**; CLI ainda bloqueada inicialmente por runtime `libgstvideo-1.0.so.0`, **não** por formato ou preset.
- [CI fonte oficial completo #42319915138](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42319915138) `SUCCESS`: **26 perfis-máquina, 60 de processo, 123 de filamento** interpretáveis, 2.453 arquivos-fonte/recurso, 2.267 deltas preparados isoladamente; 64 itens em revisão.
- Correção de ambiente: provisionar somente `libgstreamer-plugins-base1.0-0`; novo CI da CLI e da geração G-code A/B **AD5X 0,4 + Adventurer 5M 0,4** em andamento. Checar logs antes de marcar PASS. Não afirmar IFS/câmera/LAN ou Filuna nativo sem testes.

**Regra de evidências:** pacote OEM e código-fonte integral pinados são `PASS_SOURCE`, G-code do **oficial** A/B é `PASS_OEM_ONLY`, **não** é `PASS_FILUNA_PARITY`, `PASS_NETWORK` ou `HOMOLOGACAO_INTERNA`. Nenhum dos três fabricantes foi promovido, mesclado, instalado ou habilitado fisicamente. Bambu × Bambuddy autenticado permanece único adiamento deliberado.


## DECISÃO DO PROPRIETÁRIO — INSTALADOR LEVE E HOMOLOGAÇÃO POR CHECKLIST EM TRÊS TESTES (07/11/2026)

**Fonte normativa de implantação:** [PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md §13](PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md) **§13** — `REGRA_MOTOR_BASE_ON_DEMAND_PROFILES_BETA_3_TESTES`.

**Nova regra prioritária e vinculante:** compilar para entrega apenas **um motor-base Filuna/Orca e os deltas OEM indispensáveis**, nunca os fatiadores oficiais completos ou todos os catálogos de perfis de todas as marcas no instalador. Perfis de máquina, processo e filamento vêm de catálogo oficial auditado por **download seletivo assinado na primeira conexão autorizada**, com herança/compatibilidade automática e cache offline. Cada impressora física recém-conectada executa assistente guiado: sistema mede rede/portas/identidade/segurança/G-code, cliente visualiza preview real de câmera local e confirma, sistema lê slots AMS/ACE/CFS/IFS e cliente confirma filamentos/cores. Não permitir que o cliente “aprove” uma porta inexistente.

**Mensagens de beta:** todos os testes locais PASS → **"Impressora liberada para teste de impressão"**. Primeira impressão física válida completa → **"Impressora parcialmente homologada"** no próximo vínculo do mesmo modelo/firmware, ainda com teste local obrigatório. Três impressões físicas distintas, com matrizes e evidências completas PASS, sem incidentes críticos → **"Impressora homologada — liberada para impressão"** no beta. Duplicidades não contam, novos firmwares/variações exigem gate correspondente. O proprietário ainda aprova releases estáveis; o servidor observa sem comandar máquinas dos clientes. As normas anteriores de homologação física e observabilidade continuam válidas em tudo que não contrariar expressamente esta regra mais recente.

**Status factual:** `NOVA_REGRA_DOCUMENTADA=PASS`, `NOVO_INSTALADOR=NOT_RUN`, `PROFILE_CATALOG_RUNTIME=NOT_RUN`, `FIRST_CONNECT_WIZARD=NOT_RUN`, `BETA_3_PRINTS=NOT_RUN`; sem publicação, compilação final ou ensaio físico por este registro. Retomar Snapmaker, Anycubic e Flashforge como QA OEM interno, sem confundir A/B oficial com engine Filuna.


## Checkpoint 07/11/2026 — três OEMs, correção de fonte e etapa seguinte
- **Autoridade/escopo:** ordem do proprietário de corrigir Snapmaker U1, Anycubic e Flashforge após leitura de `REGRA-MESTRE-QUALIDADE.md` e `PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md`; tudo isolado no PR draft #6.
- **Correções de fonte candidatas aplicadas:** `PrintConfig.cpp` agora preserva `wall_direction=auto` explícito da Flashforge; `Print.cpp` bloqueia NaN/Inf na retração ativa, mantendo o zero desativado da Anycubic; `GCode.cpp` falha fechado para purga final não validada de perfil com `chamber_cooling_mode`, sem transformar isso em promessa de segurança para a Snapmaker.
- **Prova real:** [Actions #42342020907](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42342020907), 15/15 testes **de fonte** PASS, commit `f02e1e43`; não há compilação do motor alterado, comparação nativa nova ou impressão. Falha transitória anterior #42341898566 foi corrigida sem afrouxar o guard.
- **Continua bloqueado:** `INTERNAL_HOMOLOGATION=0/3`; Snapmaker não reproduz consistentemente o G-code OEM e upstream #1149 segue aberto; Flashforge mantém diferenças reais de G-code; Anycubic ainda não tem paridade do binário corrigido; `G4-I=BLOCKED`, `G4-F=NOT_RUN`. Bambuddy autenticado permanece adiado.
- **Dossiê de continuidade:** [CORRECOES-CANDIDATAS-3-OEM-E-PROXIMA-ETAPA-20261107.md](qa/CORRECOES-CANDIDATAS-3-OEM-E-PROXIMA-ETAPA-20261107.md).
- **Próxima etapa técnica desse lote:** concluir paridade do motor nativo e demais gates internos por fabricante; só então compilação final e beta físico autorizado. **Próxima onda global do Plano Mestre ainda pendente:** **01 — Fundação** (CI, baseline G-code, isolamento de dados e logs), seguida de Onda 02 Qualidade Orca. Nenhuma onda foi marcada PASS por este lote.


## Checkpoint — confronto dos portáteis, laboratório Ubuntu e porte de FillRectilinear (07/11/2026)
- **Pedido do proprietário:** consultar o laboratório dos oito fabricantes no Ubuntu, comparar portáteis OEM reais, extrair do código-fonte original os trechos responsáveis por falhas e incorporar no **motor Orca compartilhado** do Filuna (não instalar motores OEM duplicados). Acesso/inspeção da pasta `/srv/site/incoming/filuna-local-oem-qa-20261106` foi somente leitura; nenhum script histórico foi alterado.
- **Ponto confirmado no servidor:** `Snapmaker Orca.exe` de `lab/filuna_lab_run.sh` **não é o nome real** do binário extraído `snapmaker-orca.exe`; o `official-manifest.json exists:false` foi falso negativo de caminho. Flashforge do servidor é instalador Windows **1.7.15**, mas a prova oficial GitHub Linux é **1.7.9**; Anycubic só tem CLI oficial executável na prova GitHub 2.0.0.6, não em R3. Dossiê: [LAB-SERVIDOR-PORTABLES-PORTE-MOTOR-3-OEM-20261107.md](qa/LAB-SERVIDOR-PORTABLES-PORTE-MOTOR-3-OEM-20261107.md).
- **Novo porte nativo C++ candidato (Flashforge):** `FillRectilinear.cpp` agora reconhece o algoritmo de ancoragem do preenchimento consistente FlashForge/Orca-Flashforge 1.7.9 **apenas** quando `PrintConfig::filuna_oem_legacy_infill_anchor` é explicitamente ativado; default `false`, algoritmo Orca sem mudança para demais perfis. `PrintConfig.hpp/.cpp` registram a chave na classe de impressão (não região). O source `wall_direction=auto` permanece preservado. **Nenhum loader assinado o ativa ainda.**
- **Prova fonte:** [17/17 PASS #42342812612](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42342812612). **Sem compilação da nova base, G-code da versão nova, regressão numérica ou simulação de dispositivo do próprio app.** Anycubic: zero desativado/10–18 ativo finito preservados. Snapmaker: guarda fail-closed até resolver issue #1149 e variação do próprio OEM.
- **Estado:** `FILUNA_NATIVE_OEM_PARITY=NOT_PROVEN`, `HOMOLOGACAO_INTERNA=0/3`, `G4-I=BLOCKED`, `PHYSICAL_BETA=NOT_RUN`; no merge, release, build final, deploy ou impressão. Bambu+Bambuddy autenticado permanece adiado.
- **Próxima ação:** mapear deltas restantes com origem/versão/licença, ligar perfis e defaults via módulo OEM verificável e executar **motor nativo de QA** com comparação real de G-code, testes negativos, depois rede/DeviceBridge; build final único apenas após gates internos. Etapas globais 01 Fundação, 02 Qualidade Orca permanecem pendentes.


## Checkpoint — Snapmaker U1 original imutável e validador geométrico (07/11/2026)
- **Ordem expressa posterior:** nunca corrigir fabricante por reescrita/inserção de movimentos G-code; preservar resultado do OEM. Reversão na branch QA do guard cego e da tentativa `filuna_oem_final_purge_lift` em `src/libslic3r/{GCode.cpp,PrintConfig.cpp,PrintConfig.hpp}`. Política vinculante agora em [REGRA-MESTRE-QUALIDADE.md](REGRA-MESTRE-QUALIDADE.md).
- **Firmware/prova:** perfil efetivo original Snapmaker Orca 2.4.1 mostra `G91/G1 Z1.5/G90` **apenas na troca normal de ferramenta**. `Snapmaker/u1-klipper@7ac453b`, `lava/fluidd.cfg` blob `dd8bf672`, macro `PRINT_END` eleva Z **antes de estacionamento**, **depois da purga final**, não protege esta. `/printer/objects/query?configfile` de máquina real **NOT_RUN**.
- **Validador geométrico externo ao motor:** `lab/snapmaker_spatial_gcode_validator.py` e `FILUNA/fabricantes/snapmaker/vendor.yaml`, sem alterações no G-code. Só macro com origem/escopo/hashes verificados tem entrada em `safe_macros`; sem imunidade genérica, sem presumir correção de bug OEM. Risco oficial pode ser conscientemente aceito com consentimento auditado por usuário/hash; divergência Filuna × original não pode receber aceite “fabricante”.
- **CI comprovado** [#42345082502](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42345082502) **4 jobs SUCCESS**: 16/16 testes geométricos, 18/18 contratos de fonte, cubo U1 mono real PASS, original U1 bicolor **BLOCK (risco no próprio Snapmaker Orca 2.4.1, camada índice 14, linha 38911, cruzamento X63.569/Y147.185/Z3.05)**; cubos oficiais Bambu A1 e Creality PASS para regressão geométrica (não certifica envelopes mecânicos de outras marcas).
- **Pendências inequívocas:** validador QA não integrado ao **DeviceBridge nativo, UI, autorização e envio**; sem compilação/paridade G-code Filuna atual nem teste U1 quatro cores/macros do firmware vivo. `G4-I=BLOCKED`, `SNAPMAKER_INTERNA=BLOCKED`, `G4-F=NOT_RUN`. NÃO anunciar que “só resta imprimir”; build final/merge/deploy/release não foram efetuados.
- **Dossiê:** [SNAPMAKER-U1-GCODE-IMUTAVEL-VALIDADOR-GEOMETRICO-20261107.md](qa/SNAPMAKER-U1-GCODE-IMUTAVEL-VALIDADOR-GEOMETRICO-20261107.md).


## Checkpoint — U1 validador nativo C++ e bloqueio real de upload (07/11/2026)
- **Código de aplicação alterado, somente na branch QA:** `src/slic3r/Utils/FilunaGCodeSafety.hpp` é validador geométrico **C++17 autônomo**, sem Python ou mutação de G-code. Integrado ao `PrintHostJobQueue::upload_job` em `src/slic3r/Utils/PrintHost.cpp` para **o perfil efetivo U1**, com bloqueio por colisão/inconclusão antes do transporte, 3MF não analisável bloqueado e aviso via `PrintHostQueueDialog::on_info` / wxWidgets. `PrintHostJob` carrega identidade do **preset selecionado**, não prova de impressora pareada. `src/slic3r/CMakeLists.txt` registra o header. **O fluxo DeviceBridge alternativo não foi testado.**
- **Real proof C++17:** [GitHub Actions #42362530764](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42362530764), 4 G-codes OEM originais verificados por SHA e 3 negativos/positivos sintéticos: U1 mono PASS, U1 bicolor BLOCK (linha 38911, camada 14), Bambu A1 e Creality PASS. [#42363101061](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42363101061): 7 contratos de ligação da fila/GUI PASS. **Isto ainda não é prova de compilação integral GUI Windows.**
- **Compilação Windows de QA sem instalador:** primeiro CI #42362859858 falhou por cache inacessível de dependências; o workflow agora reconstrói as dependências do QA. [#42363069593](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42363069593) estava na etapa de dependências ao último check — **não marcar GUI PASS sem log real de compilação**.
- **Quatro ferramentas U1:** nova fixture 3MF com 4 objetos e extrusoras 1..4, executada pelo **Snapmaker Orca 2.4.1 original** via CLI e perfis oficiais. Primeiro 3MF sintético falhou import, novo template original `resources/handy_models/OrcaCube_v2.3mf` expôs exigência de `plater_id` e de usar mesh real em vez de component wrapper; correções mantêm o OEM inalterado. Runs #42363527057, #42363769245 e #42364223761 registram **FAIL** em etapas do teste, nunca devem ser promovidas como homologação. [#42364395913](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42364395913) verifica fixture corrigida, ainda sem resultado confirmado neste checkpoint.
- **Gates:** `NATIVE_CPP17_GCODE_SAFETY=PASS`, `GCODE_READ_ONLY=PASS`, `UPLOAD_HOOK_SOURCE_QA=PASS`, `FULL_GUI_WINDOWS_COMPILE=NOT_PROVEN`, `NATIVE_GCODE_3_OEM_PARITY=NOT_PROVEN`, `LIVE_FIRMWARE_AND_DEVICEBRIDGE=NOT_RUN`, `G4-I=BLOCKED`, `G4-F=NOT_RUN`, `HOMOLOGACAO_INTERNA=0/3`. Sem merge/release/instalador final/deploy/print.
- Relatório detalhado: [U1-GATE-NATIVO-CXX-FILA-ENVIO-WINDOWS-QA-20261107.md](qa/U1-GATE-NATIVO-CXX-FILA-ENVIO-WINDOWS-QA-20261107.md). **A aceitação consciente auditada do risco OEM existe apenas no laboratório Python, não na UI do aplicativo.** Não dispensar integração de identidade real e comparação oficial.

## Checkpoint 07/11/2026 — continuidade ativa de homologação OEM (PR #6)
- Documento central atualizado: [HOMOLOGACAO-CHECKPOINT-CONTINUO-20261107.md](qa/HOMOLOGACAO-CHECKPOINT-CONTINUO-20261107.md). É o índice para histórico, logs, hashes, provas reais e pendências, não substitui o Plano Mestre.
- Segurança U1: `PrintHost` não envia G-code em `WARN` geométrico antes de aceite por usuário/arquivo auditável. Commits `1a97e2e` e `5f54179e`, [CI #42393531370](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42393531370) **PASS** do scanner C++17 isolado, 8 contratos estáticos e regressões com G-code oficial. UI Windows e ACK de rede **não comprovados**.
- Fatiador oficial Snapmaker v2.4.1: STL de cubo é fatiado, mas 3MF OEM imutável/QA quatro ferramentas causam `0xC0000005`; export 3MF OEM após `render_thumbnail` também causa `0xC0000005` e 3MF Core mínimo sintético retorna rejeição `4294967290`, **não o mesmo crash**. [CI #42394922180](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42394922180), artifact `13583545231` **FAIL de compatibilidade**, 2/2 testes de fixture **PASS**.
- Build Windows: [#42365045634](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42365045634) compilou deps em MSVC, falhou em hash recursivo `deps/**` após a compilação; correção de cache determinístico `709c7e1`, [#42394738274](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42394738274) **pendente de resultado**, sem instalador.
- **Nenhuma promoção**: `G4-I BLOCKED`, `3_OEM 0/3`, `FILUNA_NATIVE_PARITY NOT_PROVEN`, `NETWORK_ACK NOT_RUN`, `G4-F NOT_RUN`, `G6-S BLOCKED`. PR DRAFT, sem merge/implantação/hardware; somente Bambuddy autenticado explicitamente adiado.

### Subgate U1 quatro ferramentas — resultado atualizado 07/11/2026
- O **G-code oficial Snapmaker 2.4.1** com fixture sintética 3MF Core+BBS de quatro objetos/extrusoras foi gerado em duas execuções, T0–T3, 301 trocas e 2.638.447 bytes; SHA funcional (somente data do cabeçalho normalizada) **`1fffba21d2cce0e0456f69836e3e8b5f22aaa35e955ddcb8305627457aca12d1`**, [#42396105609](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42396105609). **PASS restrito ao A/B desta fixture**.
- Segurança **BLOCK**: purga final cruza peça na linha 106623, camada 99, X132.044/Y141.691/Z20.05. Python e C++ nativo fazem o mesmo bloqueio, [#42396347068](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42396347068) **PASS de DETECÇÃO, NÃO de impressão segura**. `G4-I BLOCKED`, `0/3` homologados; ainda falta paridade Filuna, risco/firmware real e ACK.
- Estresse de 20 processos OEM e compilação GUI MSVC continuam sendo avaliados; consultar [checkpoint §6](qa/HOMOLOGACAO-CHECKPOINT-CONTINUO-20261107.md) antes de qualquer classificação. Nenhum merge, deploy, release ou impressão.


## REGRA CANÔNICA VIGENTE — MÓDULOS DE FABRICANTE CLONADOS E CARREGADOS SOB DEMANDA (10/11/2026)
- **Decisão expressa do proprietário (10/11/2026, sob sua responsabilidade):** [REGRA-CANONICA-MODULOS-FABRICANTE-SOB-DEMANDA-20261110.md](REGRA-CANONICA-MODULOS-FABRICANTE-SOB-DEMANDA-20261110.md). Cada fabricante tem um módulo com **motor, perfis, rede, controle, câmera e sistema de cores clonados integralmente** da fonte oficial fixada; a base Filuna/Orca é leve e gratuita; módulos ficam no servidor e são baixados quando o cliente logado escolhe o fabricante em Dispositivos; em Preparar o módulo ativo troca conforme a impressora.
- **Substitui:** motor Orca único com overlays por fabricante; homologação física como condição de bloqueio. **Homologação = G-code idêntico ao oficial + rede/preparo/envio/controle pelo código clonado.** Impressão física não bloqueia.
- **Motivo técnico medido:** motor do Snapmaker Orca 2.4.1 (`bac6fd5c`) difere do motor Filuna em 571 arquivos de `libslic3r`; a linha de ajustes pontuais parou em `SAFE_BUT_DIVERGES_FROM_OEM`, similaridade ~0,14. Prova reexecutada em 10/11: [#42841651470](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42841651470) SUCCESS nos dois jobs, mesmo status; a falha pré-job de 09/11 (#42672418486) não se repetiu com o workflow inalterado.
- **Primeiro módulo — Snapmaker U1:** `FILUNA/fabricantes/snapmaker/module.json` fixa fonte, commit, hashes de árvore e inventário por componente (44 arquivos + 3 árvores). `FILUNA/fabricantes/tools/fetch_oem_source.py` clona e confere; execução local `SOURCE_STATUS=PASS`. Workflow `filuna-module-source-verify.yml` repete a conferência no CI.
- **Estado:** `SNAPMAKER_MODULE=FONTE_FIXADA_E_INVENTARIADA`; **motor do módulo não compilado, carga/troca de módulo no app não implementada, G-code idêntico NÃO provado, rede NÃO provada.** Nenhuma impressora homologada ainda. Sem merge, release ou deploy.
- **Próxima ação:** compilar o `libslic3r` pinado do módulo como motor sem interface no Windows e provar G-code idêntico ao Snapmaker Orca 2.4.1 portátil nas fixtures já existentes (cubo mono e quatro ferramentas, variantes OEM A/B).

### Módulos dos 8 fabricantes — fonte oficial fixada e inventário preliminar (10/11/2026)
- `FILUNA/fabricantes/<id>/module.json` criado para **bambu, elegoo, creality, flashforge, qidi, anycubic, prusa** (snapmaker já existia), gerado por `FILUNA/fabricantes/tools/inventory_oem_source.py` e conferido por `fetch_oem_source.py`: **8/8 `SOURCE_STATUS=PASS`** em execução local.

| id | Fonte fixada | Commit | Motor: arquivos diferentes do Filuna |
|---|---|---|---|
| snapmaker | Snapmaker/OrcaSlicer v2.4.1 | `bac6fd5c` | 571 (contagem por diff de diretório) |
| bambu | bambulab/BambuStudio v02.09.01.70 | `56a57d00` | 515 de 537 |
| elegoo | elegooofficial/ElegooSlicer v1.5.3.5 | `3f1042e9` | 459 de 501 |
| creality | CrealityOfficial/CrealityPrint v7.3.1 | `254250c0` | 669 de 699 |
| flashforge | FlashForge/Orca-Flashforge 1.7.9 | `70f2359b` | 450 de 491 |
| qidi | QIDITECH/QIDIStudio v2.07.02.73 | `403b619d` | 485 de 507 |
| anycubic | ANYCUBIC-3D/AnycubicSlicerNext (main) | `6103ed8b` | 434 de 466 |
| prusa | prusa3d/PrusaSlicer version_2.9.7 | `450d70dd` | 530 de 541 |

- **Leitura:** em todos os fabricantes o motor difere do motor Filuna na grande maioria dos arquivos; confirma a regra canônica (motor do fabricante dentro do módulo).
- **Limites:** rede/controle/câmera/cores dos 7 novos estão classificados **só por nome de arquivo** (`PRELIMINAR_POR_NOME_DE_ARQUIVO`), sem leitura do código; Snapmaker foi classificado manualmente. **Anycubic:** sem tag de fonte para 2.0.0.6; equivalência fonte × binário não provada. **Bambu:** plugin de rede proprietário fora da fonte. **Prusa:** não é fork do Orca; perfis `.ini`.
- **Estado:** 8/8 `FONTE_FIXADA`; **0/8 motor compilado, 0/8 G-code idêntico provado, 0/8 homologado.** Sem alteração em `src/`, sem merge, release ou deploy.

### Snapmaker U1 — build de paridade do portátil e componente fechado de conexão (10/11/2026)
- Workflow `filuna-snapmaker-source-build-parity.yml` + `lab/snapmaker_source_build_parity.py`: compila o portátil do Snapmaker a partir da fonte AGPL pinada (`Snapmaker/OrcaSlicer@bac6fd5c`) e compara o G-code do cubo 20 mm com o portátil oficial 2.4.1, presets U1 idênticos, normalizando só a linha de cabeçalho.
- **Descoberta:** o build oficial roda `scripts/packaging/stage_connect_bundle.py`, que **baixa um binário fechado** `snapmaker_control` (`Snapmaker/snapmaker_control`, lock `.github/connect-bundle.lock.json` tag v1.8.5) e o embute. `src/slic3r/Utils/ConnectionProcessManager.*` e `GUI/ConnectPrinter.*` só lançam esse binário. **Decisão mantida:** não empacotar o fechado; o portátil Filuna é compilado pulando o connect bundle (só motor/GUI AGPL). Rede/controle nativos da U1 pelo caminho aberto Moonraker/MQTT.
- **Primeira tentativa** ([run 42850605253](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42850605253)) falhou no passo de deps: a fonte do Snapmaker usa `build_release_vs2022.bat deps|slicer`, não `build_win.bat`. Workflow corrigido para o script real e para configurar/compilar o slicer sem o connect bundle.
- **Estado:** build de paridade disparado; resultado ainda não confirmado (build Windows longo). Sem alteração em `src/`, sem merge, release ou impressão.

### Política canônica de conectividade — fonte aberta, sem fechado (10/11/2026)
- [REGRA-CANONICA-CONECTIVIDADE-OPEN-20261110.md](REGRA-CANONICA-CONECTIVIDADE-OPEN-20261110.md): rede/controle/câmera/cores via código AGPL e SDKs abertos + protocolos locais faltantes; **nunca** empacotar componente fechado nem contornar autenticação; homologar por modelo. Descumprimento de AGPL por fabricante não transfere licença para redistribuir o fechado — por isso o fechado fica fora, decisão do dono.
- Caminho por marca gravado em cada `module.json` (`connectivity`): **Snapmaker=Moonraker/MQTT (DECIDIDO)**; Elegoo=Elegoo Link SDK aberto (`elegooofficial/Elegoo-Link`, existe; conferir licença); Anycubic=plugin comunitário ACE Pro (localizar/verificar); Bambu=LAN aberto + AMS comunitário, sem `bambu_networking` (localizar/verificar); Creality/Flashforge/QIDI/Prusa=a levantar.
- Bambu: controvérsia #10952 tratada como argumento, não veredito.

### Especificação integral da v1 + telas aprovadas Visual C (10/11/2026)
- Decisão do dono: v1 nasce completa e sólida; depois só correções. Fonte de verdade: [SPEC-V1-BASE-COMPLETA-20261110.md](SPEC-V1-BASE-COMPLETA-20261110.md), ancorada na Direção Visual C aprovada (telas Preparação e Dispositivo versionadas em `design/visual-c/telas-aprovadas-20261110/`).
- Cobre marca/splash/instalador/nome Filuna; tradução pt-BR (só strings, função intacta, ferramentas inalteradas); autoajuda liga/desliga com card+imagem por ferramenta e por parâmetro; login Orca + login Filuna ligado ao plano no servidor; auto-update diferencial contra o canal `site/deploy/slicer-releases` (recurso atualiza sem recompilar; binário só quando muda `hostVersion`); sistema de plugins do Orca; integração web server + cofre. Ordem de build por camadas com critério de "pronto".
- Servidor/site identificado: `abcollicelli-web/Filuna` → `site/` (FastAPI, cofre, OAuth `oauth_providers.py`, releases, `marca/tokens.css`). Tokens oficiais confirmados.
- **Bloqueios declarados:** acesso ao cofre/segredos e deploy (login Filuna, update assinado, publicação); verificação visual do GUI é do dono por camada; cada build Windows é longo. **Aguardando decisão do dono sobre acesso ao cofre/deploy antes da camada de servidor.**

### Guia de implementação da v1 para o modelo implementador (10/11/2026)
- Decisão do dono: o modelo atual não implementa; escreve o guia completo para o Opus executar sem errar. Guia em `FILUNA/guia-v1/` (00 a 04): regras, telas aprovadas e como aplicá-las (WebView para telas novas, restilização para telas nativas), modelo de dados único com fórmulas e exemplos (bobina física, fim de impressão, purga, desperdício, manutenção por horas, rankings, previsão de falta), servidor (contas de cliente, sincronização, vínculo de arquivo sem upload do 3D, auto-update em duas camadas) e cronograma em 12 fases com aceite.
- Requisitos novos do dono registrados: backup automático no servidor de tudo que o cliente faz; arquivo 3D não sobe (só o vínculo com a pasta do PC); sistema único interligado; estoque por bobina com confirmação nova/usada; produtos mais impressos, filamento mais gasto e previsão de falta.
- Telas: 9 telas em `FILUNA/design/visual-c/ui/` (HTML/CSS de referência, não é o app compilado).
- Snapmaker U1: portátil compilado da fonte aberta com sucesso (run 42851665029); comparação de G-code refeita no run 42893583289 após correção de import — veredito pendente.
- Fatos verificados que o guia registra: servidor só tem conta de administrador; Orca não traz SQLite; Orca 2.5 já tem plugins Python e ilustração em tooltip.


### V1 — Fase 0, base Filuna + SQLite (10/11/2026)
- **Branch de trabalho:** `filuna/v1-f00-base-sqlite-20261110` criada a partir de `filuna/oem-portable-first-qa-20261106`, sem alterar a branch principal.
- **Implementado:** `src/slic3r/Filuna/FilunaFoundation.{hpp,cpp}`; SQLite amalgamation oficial 3.50.4 em `src/sqlite3/` (CMake, sqlite3.c, sqlite3.h); inclusão em `src/slic3r/CMakeLists.txt` somente na GUI.
- **Proveniência:** fonte, versão e hashes em `FILUNA/guia-v1/VERIFICACOES.md`. Código de `src/libslic3r/**` intocado.
- **Workflow:** `filuna-windows-qa-exe.yml` incluído para branch da Fase 0 e alterado para executar `--run-tests`; alteração estritamente necessária para cumprir o aceite com build e CTest na branch de trabalho.
- **Testado:** compilação isolada de `sqlite3.c` com compilador C Linux, compilação de `FilunaFoundation.cpp` C++17, link e execução com versão esperada `3050004`; CMake/CTest isolado **1/1 PASS**; `git diff --check`.
- **NÃO TESTADO/AINDA PENDENTE:** build completo Windows, suíte CTest Windows, confirmação de ausência de regressões no executável. **Fase 0 não está aceita até o CI passar.**
- **Não iniciado:** fases 1 a 11, telas, servidor, contas, sincronização, auto-update, release ou deploy.


### V1 — Fase 0: aceite CI Windows + CTest (11/11/2026)
- **Escopo:** somente a fundação v1 da PR [#8](https://github.com/abcollicelli-web/FilunaSlicer/pull/8), branch `filuna/v1-f00-base-sqlite-20261110`; módulo C++ Filuna e SQLite 3.50.4 ligados ao alvo GUI, sem alterar `src/libslic3r/**`.
- **Causa da primeira falha:** [run 42899893475](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42899893475), assinatura mista em `target_link_libraries(libslic3r_gui)` → correção mínima SHA `45fce160ad172b38f381387bbca4a58b5ae57c04`, reproduzida em teste CMake isolado.
- **Causa da segunda falha:** [run 42911461477](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42911461477), compilação Windows OK mas CTest #1844 FAIL devido ao número de série sintético que não cumpria o prefixo Bambu P1P/C11 `01S`; corrigida **somente a fixture** em `tests/slic3rutils/test_device_manager_integration.cpp` no SHA `558c8866546a50d7e2bcf9bf024e4201817be56f`. Teste isolado de validade da identidade Bambu C++17 PASS; política de rejeição preservada.
- **Aceite técnico da Fase 0 — PASS** em [GitHub Actions Windows #42928933023](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42928933023), SHA exato `558c8866546a50d7e2bcf9bf024e4201817be56f`: `Build full Filuna QA executable (no installer)=SUCCESS`, `ctest --test-dir build/tests -C Release --output-on-failure` **100% testes PASS, 0 falhas, 3.003 registrados: 2.998 Passed e 5 Skipped** (suítes com Skip declaradas, não convertidas em execução efetiva). `actions/upload-artifact@v4=SUCCESS`.
- **Artefato verificado:** `filuna-qa-windows-exe-558c8866546a50d7e2bcf9bf024e4201817be56f`, [ID 13821062353](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42928933023/artifacts/13821062353); 219.667.410 bytes; ZIP SHA-256 `129b406d465e219c5252b427ebbb7ecdf517ee7dd0b8e0111ec4c18a2a7d7666` de acordo com o upload GitHub.
- **Limites:** só prova build Windows CI e ausência de falhas nos CTest efetivamente executados; não prova abertura em PC real, visual Filuna, instalador/atalho, paridade física/OEM, atualização, segurança de acesso, desempenho de hardware ou 5 testes marcados Skipped. Nenhum G-code novo/fatiamento manual executado nesta fase; apenas código de infraestrutura.
- **Promoção:** os critérios **específicos de aceite da Fase 0** no `FILUNA/guia-v1/04-CRONOGRAMA.md` estão satisfeitos. A Fase 1 deve permanecer em branch e PR DRAFT separados, com seu próprio aceite visual e de instalador antes da Fase 2; sem merge, release ou deploy na produção.


### V1 — Fase 1: início controlado (11/11/2026)
- **PR DRAFT:** [#9](https://github.com/abcollicelli-web/FilunaSlicer/pull/9), branch `filuna/v1-f01-identidade-20261111`, criada sobre o aceite Fase 0 `835442f272efa2f14ca2338a63c28aaac64312ff` para preservar o SQLite e a regressão aprovada. A branch de referência canônica continua `filuna/oem-portable-first-qa-20261106`; sem merge.
- **Implementado inicialmente:** SHA `fca23633162a2aa4a0cd34e88f65e685c66ab235`: nome visível `Filuna Slicer` em `version.inc`, **sem mudar `SLIC3R_APP_KEY=OrcaSlicer`** e sem migração AppData; cores centrais aprovadas `#C2185B`/`#8E1243`/`#FF2E8A` em StateColor, progresso do splash e links; nome de arquivo do instalador e fornecedor CPACK Filuna. Não alterados: `src/libslic3r/**`, G-code, controles de impressora, AGPL, Sobre.
- **Testado:** `git diff --check` PASS, inspeção das mudanças e preservação da chave. **NÃO TESTADO:** Windows CI/CTest deste SHA, instalador NSIS/atalho, abertura real do app, splash, Sobre, comparação visual, G-code, ausência de verde em toda a UI.
- **Pendências bloqueantes:** logotipo **Filuna 3D com fundo transparente aprovado** para `resources/images/OrcaSlicer*` e splash; não inventar, não converter JPG creme em aprovação. Decisão sobre migração AppData ainda **não aprovada**, portanto chave preservada. A alteração do nome do pacote CPACK **não é prova de instalador**: workflow de QA é explicitamente `NO installer`.
- **Aceite Fase 1:** PENDENTE até todos os critérios de `FILUNA/guia-v1/04-CRONOGRAMA.md` e `01-TELAS.md` serem provados. Nenhuma Fase 2 iniciada.

### V1 — Fase 1: diagnóstico Windows CTest (11/11/2026)
- **Run:** [#42948512564](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42948512564), SHA `2979577c2499d5ee91642be9aa92ed564114853d`, branch `filuna/v1-f01-identidade-20261111`, PR [#9](https://github.com/abcollicelli-web/FilunaSlicer/pull/9) DRAFT. Compilação das dependências e do executável avançou até CTest, mas o job terminou **FAIL** e o upload do executável foi **SKIPPED**.
- **CTest completo:** 3.003 testes; **2.994 Passed, 4 Failed, 5 Skipped**, exit code 8. Falhas: #1041 (Preset/Bundle), #2386 (ExclusionVolume/GCode), #2678 (GCodeProcessor/belt), #2920 (GCodeProcessor/Regression). Todos lançam `Not a G-code file generated by Filuna Slicer.` ao ler fixtures que contêm `; generated by OrcaSlicer`.
- **Hipótese causal sustentada pelo log e pelas fixtures:** mudança de identidade em `version.inc` fez a validação de origem do G-code rejeitar arquivos Orca existentes. **Não modificar fixtures para mascarar regressão**; **não modificar `src/libslic3r/**`**, nem a geração de G-code. Separação entre identidade visual e identificador técnico deve ser comprovada por implementação fora do motor e CTest integral.
- **Situação:** Fase 0 continua PASS no [run #42928933023](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42928933023) (2.998 Passed, 5 Skipped, 0 Failed); Fase 1 **FAIL/BLOQUEADA**, sem aceite visual ou instalador; Fase 2 **NÃO INICIADA**. Sem merge, release, deploy ou impressão.

### V1 — Fase 1: hipótese isolada para revalidar compatibilidade (11/11/2026)
- **Causa:** run #42948512564, SHA 2979577c, 4/3003 falhas por cabeçalho OrcaSlicer rejeitado quando `SLIC3R_APP_NAME=Filuna Slicer`.
- **Hipótese/alteração única:** restaurar **temporariamente** `SLIC3R_APP_NAME=OrcaSlicer` em `version.inc`, mantendo `SLIC3R_APP_KEY=OrcaSlicer`, cores e CPACK Filuna. Testar se os quatro CTest retornam PASS sem tocar em `src/libslic3r/**` ou nos testes. Não é solução final de marca; nome visível Filuna continua **PENDENTE** e fase 1 não pode ser aceita com esta configuração.
- **Teste solicitado:** workflow Windows QA completo e CTest de 3003 casos, sem dispensar falhas. Resultado: **NOT RUN** até o CI concluir. Sem instalador, tela real ou prova de G-code manual.

### V1 — Fase 1: revalidação Windows da hipótese técnica (11/11/2026)
- **Run:** [#43006095533](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/43006095533), SHA `1a95deb379abd9ba4a6fad1b736493d6dc574285`, PR [#9](https://github.com/abcollicelli-web/FilunaSlicer/pull/9) DRAFT: workflow Windows **SUCCESS**; etapa de compilação do executável **SUCCESS**; `ctest --test-dir build/tests -C Release --output-on-failure` **100% dos 3.003 testes sem falha**, 2.998 Passed, 5 Skipped (não executados). As quatro falhas do run anterior #42948512564 deixaram de ocorrer com a restauração temporária do identificador técnico Orca.
- **Artefato QA:** `filuna-qa-windows-exe-1a95deb379abd9ba4a6fad1b736493d6dc574285`, [ID 13851639002](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/43006095533/artifacts/13851639002), 219.667.310 bytes, `expired=false`; ZIP SHA-256 registrado no upload: `ec9fd09f2880ee0203d4a41922e637380e4e90aeefade159494941bb3b278798`.
- **Limite importante:** o identificador `SLIC3R_APP_NAME=OrcaSlicer` foi restaurado como diagnóstico e **não** satisfaz o requisito de nome visível Filuna Slicer. O workflow é **NO installer**: instalador NSIS, atalhos, abertura Windows real, comparação visual lado a lado, splash/logo transparente aprovado, créditos Sobre/AGPL e varredura completa de cores **NÃO TESTADOS/PENDENTES**. Não há evidência de impressão física, paridade G-code manual ou homologação de impressora. A Fase 1 **NÃO está aceita**, e a Fase 2 **NÃO está iniciada**.
- **Proteções:** nenhum arquivo `src/libslic3r/**`, teste ou G-code alterado por este registro; nenhum merge, release, deploy ou impressão.


### V1 — Fase 1: separar identificação técnica de nomes nos diálogos wx (11/11/2026)
- **Compatibilidade comprovada antes desta alteração:** [Windows #43006095533](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/43006095533), SHA `1a95deb379abd9ba4a6fad1b736493d6dc574285`: build Windows SUCCESS, CTest 3.003 cadastrados, **2.998 PASS / 0 FAIL / 5 SKIPPED**; artefato 13851639002, ZIP SHA-256 `ec9fd09f2880ee0203d4a41922e637380e4e90aeefade159494941bb3b278798`.
- **Identificador técnico:** `version.inc` mantém `SLIC3R_APP_NAME=OrcaSlicer` e `SLIC3R_APP_KEY=OrcaSlicer` para preservar leitura de G-code legado, perfis e AppData; `SLIC3R_APP_FULL_NAME=Filuna Slicer` já existe como macro de identidade pública, confirmada no header, sem modificação de `src/libslic3r/**`.
- **Correção GUI, Fase 1:** os títulos wx de uploads, Configurações e ação de saída em `src/slic3r/GUI/GUI_App.cpp` e `src/slic3r/GUI/MainFrame.cpp` usam `SLIC3R_APP_FULL_NAME`, preservando os identificadores técnicos. Não mudamos fixtures, parser, geração ou movimento de G-code.
- **Status:** verificação textual PASS; **build Windows + CTest no novo SHA PENDENTES**, além de instalador/atalho, inspeção visual das telas reais e revisão de todos os tokens/ícones. Fase 1 ainda não tem aceite.


### V1 — Fase 1: identidade oficial, varredura de cores e ensaio de instalador (11/11/2026)
- **Prova de regressão prévia:** [Windows run #43028878213](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/43028878213), SHA `7ef91b17b1fbdf25e5c7b0328c4b5053913806bd`, SUCESSO: 3.003 CTest, **2.998 aprovados / 0 falhas / 5 Skipped**; executável QA [artefato ID 13867570688](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/43028878213/artifacts/13867570688), ZIP SHA-256 `01defe0630a171c40c7edf73c18d8802ae25ff7d275568a4fca1af5869f509ce`. Não inclui prova de instalador.
- **Implementação candidata, ainda não homologada:** cores canônicas magenta `#C2185B`, rosa `#FF2E8A` e escuro `#8E1243` aplicadas somente à camada visual da GUI e SVG de recursos; ícones PNG/ICO/ICNS convertidos de arte já existente no repositório oficial `Filuna/site/slicer-source/branding`, logo com canal alfa em splash e Sobre, wordmark em Ajuda/Diagnóstico; CPack/NSIS aponta para `Filuna.ico`; arquivo fonte, hashes e limites em `FILUNA/qa/v1-f01/ASSETS-E-CORES.md`.
- **Compatibilidade protegida:** `SLIC3R_APP_NAME` e `SLIC3R_APP_KEY` preservados como `OrcaSlicer`; nome exibido `SLIC3R_APP_FULL_NAME=Filuna Slicer`; nenhuma mudança em `src/libslic3r/**`, ferramentas, perfis ou G-code.
- **Prova de instalador solicitada:** somente na branch F1, o workflow Windows QA tentará gerar pacote NSIS com `cpack` e enviar artefato de teste, sem release ou deploy; falta validar conclusão, atalho real e licença/credito na instalação.
- **NÃO TESTADO / BLOQUEADO:** build Windows e CTest desta alteração, captura do programa executando no Windows, comparação com os PNGs aprovados (conforme guia 01-TELAS §7), funções da Preparação/Dispositivo, teste de pacote instalável limpo e paridade do G-code do cubo. Fase 1 permanece NÃO ACEITA, Fase 2 NÃO INICIADA.


### V1 — Fechamento do visual: conta única, barra de abas e telas Meu espaço/Estoque/Dispositivo/rodapé (12/11/2026)
- **Branch e base:** `filuna/v1-base-unificada` (única branch válida por ordem do proprietário em 12/11/2026; as demais serão excluídas por ele). Base anterior: `bfc9315f`. Escopo desta etapa por ordem do proprietário: **somente o visual**; homologação, registro de impressoras, autonomia e demais etapas ficam para depois. Mudanças agrupadas num único pacote, para uma só compilação (pedido do proprietário).
- **Problemas apontados pelo proprietário nos prints do build `bfc9315f`:** telas não compartilhavam o login (Início logado, Minha loja pedindo login de novo); Orca Cloud, Bambu Cloud, OrcaCloud e Wiki no Início; aba extra "Painel da impressora" e ícone sem nome (Minha loja) na barra; Estoque vazio sem botão; Meu espaço abrindo em Ofertas com menu diferente da referência; Dispositivo mostrando o perfil "MyMarlin" para uma Elegoo CC2; sugestão de venda espremida na lateral.
- **Feito (código):**
  - Conta única: `FilunaPanel::account_changed` avisa todas as telas Filuna (evento `conta`) e o Início (`FlTokenDeFora`) quando o token muda em qualquer lugar; `ponte.js` passa a usar só o token guardado pelo Filuna Slicer (o `localStorage` da página vale apenas fora do programa) e confere a conta ao voltar para a tela. `GUI_App` só regrava e avisa quando o token realmente muda.
  - Barra de abas: logo "Filuna 3D" no início da barra (clique abre o Início); botões das abas Painel da impressora, Painel da impressora (Web), Multi-dispositivo, Calibração do Orca e Minha loja escondidos (`ButtonsListCtrl::ShowPageButton`), páginas continuam no livro e abrem por dentro das telas Filuna. Barra resultante: Início · Preparação · Visualização · Dispositivo · Projeto · Calibração · Meu espaço.
  - Preparação: o rodapé Filuna saiu da coluna lateral e virou a barra inferior de largura total (painel AUI `filuna_footer`, camada externa, não móvel), como em `telas-aprovadas-20261110/01-preparacao.png`: Sugestão de venda · Peso · Custo · Tempo · aviso · IA Filuna · Autoajuda.
  - Início: blocos Orca Cloud/Bambu Cloud ocultos (o HTML fica porque `home.js` o usa), itens OrcaCloud e Wiki removidos; Estoque abre Meu espaço › Filamentos (um estoque só) e entrou "Minha loja".
  - Meu espaço refeito pela referência `ui/meu-espaco.html`: lateral com perfil em uso e impressora; menu Meu espaço · Projetos · Perfis de impressão · Filamentos · Configurações e, separado, Ferramentas Filuna (Fila da Farm, Reparação, Ofertas, IA Filuna, Minha loja); visão geral com Atividade recente, Meus projetos, Perfis salvos, Biblioteca de filamentos, Status da conta e Links rápidos. Novas ações da ponte: `perfis.salvos`, `perfil.usar`, `secao.pendente` (abre a seção certa mesmo quando a página é criada só no clique).
  - Estoque (Filamentos) pela referência `ui/estoque.html`: tabela Filamento · Localização · Peso restante · Custo · alerta, ordenação, quatro totais (filamentos, peso restante, valor estimado, alerta).
  - Minha loja: aberta dentro de Meu espaço, com "‹ Meu espaço" para voltar.
  - Dispositivo pela referência `02-dispositivo.png`: "Dispositivo local", LAN local, Sessão do titular, Capacidades e saúde, Revisar envio, Configurar unidade; blocos Bico/Mesa; Controle local com Revisar pausa/cancelamento e aviso de confirmação; Diagnóstico e permissões; materiais pelos slots da impressora; rodapé Impressoras · Filamentos · Fila e histórico; aviso quando o perfil da Preparação não é da impressora (caso "MyMarlin" × CC2); lista lateral só com 2+ impressoras.
- **Diferenças da referência registradas (não inventar dado):** Bico/Mesa mostram "—  sem dado da impressora" porque a API da Farm não envia temperatura; etapa "Aquecido" da referência virou "Enviado" (único estado que existe); "Importar/Exportar" do Estoque e "Biblioteca"/"Documentação" de Meu espaço não entraram por não terem função; "Adicionar filamento" abre o cadastro no site; custo da bobina depende de `preco_kg`, que a API `/estoque` ainda não envia (mostra "sem preço cadastrado"). O plano do administrador aparece como "Equipe" porque o servidor (`site-v2/app/farm.py`, `limites`) devolve esse nome para admin — correção é no site, **não feita** nesta etapa. A barra de título da janela (Arquivo, salvar, desfazer) continua: é a moldura da janela Windows; a referência é um desenho sem moldura.
- **Não feito nesta etapa (próximo do visual):** cartão "Perfis" da Preparação (Impressora/perfil, Bico, Tipo, Mesa), lista de filamentos com cor e "Volumes de purga", modos Simples/Avançado/Especialista e aba Velocidade no painel nativo; área 3D com título da placa e zoom à direita; sino/ajuda/engrenagem na barra; Início (desenho ainda não aprovado pelo proprietário).
- **Testes:** telas WebView (Meu espaço, Filamentos, Dispositivo, rodapé) renderizadas no Chrome sem cabeça a 1600 px com dados de teste e comparadas com as referências — **PASS visual de disposição**, sem prova no executável. Sintaxe JS de todas as páginas alteradas — PASS. Build Windows e CTest deste SHA — **NOT RUN** até o CI concluir. Captura do executável lado a lado com a referência (01-TELAS §7) — **NOT RUN**, depende do proprietário instalar o build. Nenhuma alteração em `src/libslic3r/**`, perfis, G-code ou motor; nenhum merge, release, deploy ou impressão.


### Homologação interna dos 8 fabricantes — ramo `filuna/homologacao-oem-8` (12/11/2026, em andamento)
- **Ordem do proprietário:** homologar internamente Snapmaker, Bambu, Elegoo, Creality, Flashforge, QIDI, Anycubic e Prusa (motor/G-code, cores, rede, preparar, enviar, controlar, perfis), deixando só a impressão física para os testes dele; tudo nativo, no servidor, baixado só quando o cliente escolhe a impressora; trabalho em ramo separado do visual (outro Claude no `v1-base-unificada`), para unir depois.
- **Laboratório no servidor (`/srv/filuna-homolog`):** binários oficiais Linux com SHA (`oficial/manifest.txt`); imagem `filuna-oem-u24` (Ubuntu 24.04, mesmo ambiente dos AppImages; +libnss3 para a Creality); clones completos das 8 fontes nos commits do `module.json`; `build/compilar.sh` com a receita oficial de cada marca (Snapmaker pula só o `stage_connect_bundle.py` fechado, sem editar a fonte); compilações **uma por vez** (23 GB não aguentam duas GUIs: o servidor travou com duas + base, Elegoo ficou pausada até a Snapmaker terminar — `build/retoma.sh`).
- **Régua de paridade (`lab/oem_homolog/`):** `perfis.py` resolve a herança dentro do pacote da própria marca (como o fatiador; nomes iguais de outras marcas não se misturam) e, quando o padrão citado pela máquina não existe ou é incompatível, escolhe o compatível e registra `PADRAO_OEM_AUSENTE/INCOMPATIVEL`; `paridade.py` fatia o catálogo inteiro da marca no oficial (A, B e mais execuções quando ele varia) e no compilado, cubo 20 mm + 4 cores em toda máquina; `fixture_cores.py` gera o 3MF de 4 cores a partir de um 3MF exportado pelo **próprio oficial** (Bambu exige isso). Veredito só `IDENTICO` (bytes, fora a linha de data) a uma variante oficial, ou `MESMA_RECUSA_DO_OFICIAL`.
- **Régua oficial × oficial (prova de que a régua funciona):** Snapmaker 68 idênticas (+8 corrigidas depois pelo padrão ausente); Elegoo **73/73** cubo e 4 cores; Bambu: o próprio Bambu Studio 02.09.01.70 **não é determinístico** (larguras de linha/ordem nas camadas finais mudam entre execuções) — régua passou a juntar variantes; QIDI: CLI sem `--datadir` (régua usa HOME novo); Flashforge AD5X/Creator 5: perfil padrão de processo de outra máquina (defeito do perfil oficial). Rebase Flashforge/QIDI/Creality/Bambu em execução.
- **Perfis:** `pacote_perfis.py` (zip determinístico + manifesto SHA-256). Fonte = oficial byte a byte: Snapmaker, Bambu, Elegoo, Flashforge; QIDI igual fora fim de linha CRLF/LF. **Creality:** binário 7.3.1.6378 traz perfis `26.11.03.15`, a fonte publicada (tag e todos os ramos) só tem `26.10.29.20`. **Anycubic:** fonte publicada termina em 02/02/2026, binário 2.0.0.6 é de 30/10/2026. Nos dois casos o pacote leva os perfis do binário oficial e isso fica registrado no catálogo.
- **Módulos sob demanda:** catálogo assinado Ed25519 (`publicar_modulos.py`; chave privada em `/root/.filuna-modulos/` — não apagar; pública em `FILUNA/fabricantes/chave-modulos-ed25519-1.pub`); estado de cada parte vem só das provas. Site: `/admin/modulos` (canal nenhum/beta/todos) e `/api/slicer/v1/modulos` + download com `X-Filuna-SHA256` (site `5d349b0`/`829705c`, publicado). Slicer: `src/slic3r/Utils/FilunaModuleStore.*` (sem interface: confere assinatura, escolhe o módulo pela impressora, baixa, confere SHA-256, extrai sem caminho inseguro, reaproveita o instalado) + `tests/slic3rutils/test_filuna_module_store.cpp`. Gancho na escolha de impressora da GUI **não feito** (fica para unir com o visual).
- **Estado:** motores compilados 0/8 (Snapmaker e Elegoo compilando; fila: Flashforge, Anycubic, Bambu, QIDI, Creality, Prusa, Filuna com testes); paridade compilado × oficial roda sozinha ao fim de cada compilação (`auto-paridade.sh`) e republica o catálogo. Rede/controle/câmera/cores por protocolo: **não testados ainda** (próximo: suíte slic3rutils do Filuna em Linux + ensaios ponta a ponta por marca). **Nenhum fabricante homologado ainda.** Sem merge, release ou deploy do Slicer.
- **Atualização 12/11 21:40 UTC:**
  - **Motor compilado × oficial:** Snapmaker 2.4.1 **76/76** (71 idênticas no cubo e nas 4 cores, inclusive U1 0.2/0.6/0.8 com 154–754 trocas; 5 com a mesma recusa do oficial); Elegoo 1.5.3.5 **73/73** idênticas (cubo + 4 cores). Normalização acrescentada: id de objeto derivado de endereço de memória (`id:<n>`), que muda a cada execução do próprio oficial, vira ordem de aparição. Oficial não determinístico (Bambu/QIDI): aceito só se cada camada do compilado for byte a byte uma camada produzida pelo oficial (`IDENTICO_POR_CAMADA`). CrealityPrint 7 só fatia com `--cli`.
  - **Rede nativa nova (sem componente fechado), `src/slic3r/Utils/Filuna*Agent.*`, registradas na `NetworkAgentFactory`:** Bambu LAN (MQTT/TLS 8883 + FTPS 990 próprio com reuso de sessão TLS — o libcurl 7.75 das dependências truncava o envio em 2 MiB — + SSDP 2021), Elegoo CC2 (protocolo do SDK aberto Elegoo-Link, Apache-2.0), Elegoo Centauri Carbon (SDCP v3), Flashforge (API HTTP 8898, sem a biblioteca fechada FlashNetwork do Orca-Flashforge), Prusa (PrusaLink v1 com Digest). Cliente MQTT 3.1.1 próprio (`FilunaMqttClient`).
  - **Sondas × emuladores (`lab/oem_homolog/emuladores`, `lab/oem_homolog/sonda`):** Bambu, Elegoo CC2, Elegoo SDCP, Flashforge, Prusa — todas as etapas PASS (descoberta, recusa de código errado, conexão, estado, cores AMS/Canvas/IFS, envio com arquivo recebido idêntico, início, pausa, retomada, temperatura quando o protocolo tem, parada). Sem equivalente no protocolo: cores do MMU e temperatura na PrusaLink; senha no SDCP; temperatura na CC2/Flashforge.
  - **Família Klipper (Snapmaker U1, QIDI, Creality K com CFS, Anycubic com driver comunitário ACEPRO em `lane_data`, Elegoo Neptune):** agentes já existentes no Filuna; ajustados para não depender da janela (sem `wxTheApp`: sem preset, endereço ainda montado; cores entregues pelo callback de mensagem). Conexão, estado, 4 cores e envio PASS; início/pausa/retomada aguardavam o emulador aceitar `SDCARD_PRINT_FILE` (corrigido, reensaio em curso).
  - **Testes do Filuna em Linux (build do ramo):** `slic3rutils_tests` filtrado em módulos, Bambu LAN, Moonraker, Creality, QIDI e PrintHost: **75 casos / 509 asserções PASS**.
  - **Pendências declaradas:** motor de Flashforge, Anycubic, Bambu, QIDI, Creality e Prusa (compilando em fila, um por vez); fixture de 4 cores da Prusa não gera troca de filamento no PrusaSlicer oficial (fica PENDENTE, não aprovada); Anycubic: fonte publicada mais antiga que o binário 2.0.0.6; ensaio com impressoras reais e integração das telas da GUI ficam para depois. Nada foi unido ao `v1-base-unificada`; sem release.
- **Atualização 12/11 22:20 UTC:**
  - **Flashforge 1.7.9 — HOMOLOGADO_INTERNO** (motor, perfis, cores, rede, preparar/enviar/controlar; falta só a impressão física do proprietário). Régua compilado × oficial: 42 máquinas, 33 `IDENTICO`, 7 `IDENTICO_COM_RECUSA_OFICIAL`, 2 `PERFIL_INCOMPLETO` do próprio fabricante. Receita: o CMake do fabricante troca o binário `Flash Studio` por um link para `FlashStudio` (que não existe) no POST_BUILD; o link foi refeito para `FlashStudio` com o mesmo comando de link do ninja e a imagem gerada com `build_linux_image.sh` (fonte não editada). `paridade-auto.sh` passa a aceitar AppImage com espaço no nome.
  - **Prusa — régua consertada:** o `PrusaSlicer.ini` gerado não tinha o prefixo `model:` que o `AppConfig` lê, então a CLI caía na impressora `- default FFF -` (base anterior inválida, refeita). O G-code das MK4/XL/CORE é binário: `lab/oem_homolog/bgcode.py` lê `.bgcode` (heatshrink + MeatPack) sem componente externo, para a régua comparar o texto. Processo/filamento escolhidos pelo `--query-print-filament-profiles` do oficial quando o padrão da máquina é incompatível (ex.: XL 5T cita `Prusament PLA @PG`). **4 cores:** a CLI oficial 2.9.7 cai (SIGSEGV, rc 139) ao fatiar várias extrusoras com torre de limpeza — registrado como `MESMA_RECUSA_DO_OFICIAL`; a troca de cores é provada no caso `cores-sem-torre` (mesma configuração, `--no-wipe-tower`): MK4S MMU3 302 trocas e XL 5T 309 trocas, idênticas oficial × oficial. Base completa (217 máquinas) rodando.
  - **Placar:** HOMOLOGADO_INTERNO 3/8 (Snapmaker, Elegoo, Flashforge). Rede PASS nas 8. Compilando: Anycubic e Bambu; fila: QIDI, Creality, Prusa.
- **Atualização 13/11 00:00 UTC:**
  - **Anycubic — BLOQUEADO_PELO_FABRICANTE** (motor e cores): a fonte publicada (main `6103ed8`, 02/02/2026, motor 2.3.1) não é a do binário oficial 2.0.0.6 (30/10/2026), e não há outra tag ou ramo. O próprio script dela pede um alvo (`AnycubicSlicerNext`) que o CMake não define; compilada com o alvo existente (`OrcaSlicer`, fonte não editada), recusa os perfis do binário oficial em 35 de 39 máquinas (`retraction_distances_when_cut` fora da faixa) e diverge nas 4 Kobra 3 Max V2. Registrado em `FILUNA/fabricantes/anycubic/module.json` (`homologation_block`); o catálogo mostra o bloqueio com o resultado medido. Desbloqueio: Anycubic publicar a fonte do 2.0.0.6 ou decisão do proprietário sobre outro critério. Rede/controle e perfis continuam homologados.
  - **QIDI:** a fonte publicada (tag `v02.07.02.73`, igual ao binário) exclui por `.gitignore` a pasta `src/slic3r/QIDI` (nuvem QIDI Maker, MQTT, P2P, WebSocket, API HTTP) e não compila. Substitutos vazios escritos pela Filuna em `lab/oem_homolog/stubs/qidi` (só tipos e chamadas sem rede), copiados para a pasta ignorada, com `QDT_RELEASE_TO_PUBLIC=0` e inclusão forçada por flag de compilação — nenhum arquivo do fabricante editado; motor e CLI intocados. Não entram em módulo. Compilação na fila.
  - **Prusa:** a base oficial × oficial passou a falhar por falta de PIDs no contêiner (processos do D-Bus acumulados a cada execução do Flatpak); `oficial/prusa-oficial.sh` agora roda cada execução numa sessão própria e encerra o que ela ativou, e a régua marca `ERRO_DE_AMBIENTE` em vez de recusa. Base refeita do zero.
  - **Servidor travou (00:10–00:45 hora de Roma)** por falta de memória: três tarefas pesadas ao mesmo tempo (Bambu 6 núcleos, ensaio QIDI 6 núcleos, base Prusa). Reiniciado pelo proprietário no painel IONOS. Instalado: `earlyoom` (corta primeiro compilador/linker; protege SSH, site, banco e conector), `kernel.panic=10`, `softdog` + watchdog do systemd (reinício automático em travamento), limite de 15 GB por compilação, e o serviço `filuna-homolog-retomar` que no boot retoma a fila (uma compilação por vez) e as réguas como unidades próprias do systemd. Cofre, chaves e site não foram tocados.
  - **Placar:** HOMOLOGADO_INTERNO 3/8 (Snapmaker, Elegoo, Flashforge); Anycubic bloqueado pelo fabricante; Bambu, QIDI, Creality e Prusa compilando em fila.
- **Atualização 13/11 02:45 UTC:**
  - **Bambu 02.09.01.70 — HOMOLOGADO_INTERNO:** 60 máquinas (cubo + 4 cores): 37 `IDENTICO`, 20 com a mesma recusa do oficial, 3 `IDENTICO_POR_CAMADA` (P1S 0.2, P2S 0.8, X1E 0.2: o próprio oficial varia nas camadas finais; com 4–23 execuções extras do oficial, `mais_variantes.py`, cada camada do compilado apareceu byte a byte numa saída dele; `aplicar_variantes.py` registra a evidência na prova). **Placar: 4/8.**
  - **Correção do registro da Anycubic:** "não há outra tag ou ramo" estava errado — o clone só buscava `main`. O GitHub tem `develop`/`WL_Master` (`987a3c2b`, 02/02/2026, 67 arquivos do motor diferentes do `main`) e `release/v2.3.1`. Por proposta do proprietário: `develop@987a3c2b`, perfis do `.deb` oficial 2.0.0.6 no compilado (dado, não motor), régua nas 39 máquinas; resultado do `main` guardado em `res/paridade-anycubic.main-6103ed8`. O `develop` aponta o wxWidgets para um git da rede interna da Anycubic (`http://192.168.2.20/r/~liuchunlin/dep_orca_wxwidgets.git`, inalcançável) e pede wxWidgets 3.3: o `git` do contêiner troca o endereço (`insteadOf`) por um espelho local do wxWidgets público do Orca com HEAD em `orca-3.3.2` — só interface; não há como provar que é igual ao interno da Anycubic.
  - **QIDI:** além de `src/slic3r/QIDI`, a fonte publicada também não traz a biblioteca P2P fechada `deps/pgLibConnect` (ignorada no `.gitignore`, ligada sem condição): vira um `.a` vazio só para o link. Inclusão forçada dos substitutos só em C++; 4 tarefas (6 passavam de 15 GB).
  - **Creality:** o `BuildLinux.sh` precisa de `bc` (escolha da lista do 24.04) e `meson`; com GCC a fonte não compila (`build_result_from_mixed` declarada `extern` e depois `static`). O binário oficial foi feito com clang 18.1.3 (seção `.comment`), e o script de pacote do fabricante (`scripts/BuildLinux_Package.sh`) usa clang por padrão: a receita passou a ser esse script (`-cdsir`).
  - **Prusa:** receita oficial + pacotes do sistema da documentação (`libtool`, `texinfo`, `libudev-dev`, `libhidapi-dev`…). `bgcode.py`: referência do heatshrink antes do início da janela vale zero (como o decodificador original) — a base oficial × oficial parou na máquina 181 por isso e foi refeita.
- **Atualização 13/11 03:45 UTC — QIDI 02.07.02.73 HOMOLOGADO_INTERNO (5/8):** compilada da tag oficial com os substitutos da Filuna para a pasta fechada `src/slic3r/QIDI` e o `.a` vazio no lugar da biblioteca P2P fechada `pgLibConnect` (motor e CLI do fabricante intocados; nada disso vai para módulo). Régua: 40 máquinas, cubo + 4 cores; 35 `IDENTICO` e 5 `IDENTICO_POR_CAMADA` (Q2C 0.4 cubo; X-Max 4 0.2, X-Plus 4 0.4/0.6 e X-Smart 5 0.4 nas 4 cores): o QIDIStudio oficial, como o Bambu Studio, varia entre execuções nas marcações "Slow Down" das camadas finais; com 2 a 120 execuções extras do oficial (`variantes.sh`), cada camada do compilado apareceu byte a byte numa saída dele. `mais_variantes.py` ganhou a opção de reexecutar também o compilado (não foi preciso). Creality: o script de pacote exige os argumentos posicionais de versão (`7.3.1.6378 CrealityPrint Release 1`, como no binário oficial); na fila.
- **Decisão do proprietário (13/11 05:10 UTC) — entrega para o teste físico:** depois das homologações internas, uma compilação final do Filuna Slicer **só para Windows** (por agora). Tudo o que foi homologado por fabricante (motor/G-code, cores, perfis, rede, preparar/enviar/controlar) fica **no servidor** como módulo; o instalador do Filuna não leva motor de marca nenhuma. Ao escolher a impressora (Dispositivo ou Preparação), o Filuna baixa do servidor só o módulo daquela marca, por atualização interna pequena, sem reinstalar nem recompilar, e passa a fatiar **como o fatiador oficial**. Um cliente com 4, 5 ou 40 impressoras de marcas diferentes tem um módulo por marca no cache; trocar de impressora troca o motor sozinho. Forma escolhida para garantir isso: o módulo de motor é o fatiador da marca compilado da fonte (o mesmo binário que passou na régua), executado pelo Filuna por baixo; o Filuna entrega o projeto e recebe o G-code. Consequência: os motores precisam ser compilados para **Windows** e passar de novo pela régua contra os binários oficiais de Windows antes de ir ao servidor.
- **Anycubic `develop@987a3c2b` não compila fora da Anycubic:** wxWidgets num git da rede interna (192.168.2.20); o `Anycubic.cmake` fixa `wx_baseu_xml-3.1` enquanto o resto exige wxWidgets 3.3; e o próprio CMake falha (`target_link_libraries` com assinatura com e sem palavra-chave no mesmo alvo `libslic3r_gui`, `src/slic3r/CMakeLists.txt:703`), o que só se resolve editando a fonte. Continua BLOQUEADO_PELO_FABRICANTE; prova medida segue sendo a do `main`.
- **Creality 7.3.1.6378:** 99/99 DIVERGE na régua compilado × oficial. O binário oficial tem o recurso "time flush" (`time_flush_gcode`, `time_flush_interval`, comentários `;TIME_FLUSH_CANDIDATE`) que não existe em nenhuma ref publicada (`v7.3.1`, `publish/v7.3.1`, `main`). Na CR-10 0.4 os comandos executados são idênticos (6.596 no cubo, 26.863 nas 4 cores); só comentários/metadados diferem. Raio-x das 99 em `res/creality-raio-x.json` (`comparar_comandos.py`); critério fica para decisão do proprietário.
- **Atualização 13/11 05:40 UTC — Anycubic: bloqueio de compilação do `develop` desfeito (compilando):** o resumo "não compila fora da Anycubic" estava errado nas três causas; diagnóstico pelos logs `build/anycubic-0225..0457`:
  1. **wxWidgets:** o git interno `192.168.2.20/dep_orca_wxwidgets` é só um espelho do `SoftFever/Orca-deps-wxWidgets` (o diff do `develop` contra o Orca 2.3.1 troca apenas a URL). O `develop` (base Orca 2.3.1; `Anycubic.cmake` exige `wx_baseu_xml-3.1`) precisa da tag **`orca-3.1.5-1`**, não da 3.3.2 que o espelho tinha — daí a "mistura de versões" (`wx/gtk/private/string.h` inexistente, `wx_baseu_xml-3.1 not found`). Espelho `/espelhos/dep_orca_wxwidgets.git` com HEAD=master=`139e4f2a` (orca-3.1.5-1).
  2. **open-plugins:** `deps/plugins/plugins.cmake` puxa `ANYCUBIC-3D/open-plugins` pela branch flutuante `develop`, que em 08/05/2026 passou a exigir wxWidgets 3.3 (quem pedia 3.3 era ela, não o slicer). Espelho local `/espelhos/open-plugins.git` com `develop` fixada em **`0073895`** (26/02/2026, ponta da `Wl_Master`, ainda wx 3.1; corrige o macro `get_module_name` que em Release aceitava 1 argumento e era chamado com 3 — o commit de 04/02 `5a672ec` falhou por isso em `downloader/detail/impl/impl.cpp`). `insteadOf` no git do contêiner, como no wx.
  3. **"Erro no arquivo de compilação":** `src/slic3r/CMakeLists.txt:703` (link do `libspnav` na forma antiga) só é alcançado se `libspnav-dev` estiver instalado — o `build_linux.sh -u` instala pela `scripts/linux.d/debian`; a receita remove o pacote no contêiner efêmero. Alvo no Linux: `AnycubicSlicer` (sai como `AnycubicSlicerNext`).
  - Fonte do fabricante **não editada** (`git status` limpo); só espelhos, receita (`build/compilar.sh`) e limpeza das dependências wx 3.3. Compilação iniciada 05:33 UTC (`develop@987a3c2b`); wx 3.1.5 e open-plugins concluídos, slicer em andamento sem erros. Depois: `paridade-auto.sh` troca os perfis pelos do `.deb` oficial 2.0.0.6 (1.3.2610.08, 40 máquinas; o `develop` traz 1.3.2508.11 com 21) e roda a régua nas 39/40 máquinas. Se sobrar divergência de G-code, é a distância fev→out do motor (fonte de outubro não publicada) e vira decisão do proprietário.
- **Atualização 13/11 07:00 UTC — Prusa 2.9.7 HOMOLOGADO_INTERNO (6/8):** compilado da fonte (commit do Flathub 2.9.7) × PrusaSlicer oficial do Flathub, 217 máquinas: 152 `IDENTICO`, 52 `IDENTICO_COM_RECUSA_OFICIAL` (as MMU/XL nas 4 cores **com** torre: oficial e compilado caem igual, SIGSEGV), 13 `PERFIL_INCOMPLETO` do fabricante. 4 cores **sem** torre: 52 máquinas idênticas, com troca de filamento. Ajustes da régua (nenhum muda comando de impressão): código de saída por sinal normalizado (`-N` direto × `128+N` pelo shell do Flatpak; `rc_sinal.py`); rótulos de "cancelar objeto" (`M486 S<n>` e "; printing object … id:<n>") viram o polígono que o próprio `objects_info` do arquivo associa a cada rótulo, porque a numeração sai de um `std::map` indexado pelo endereço de memória do `ModelObject` (`LabelObjects::init`) e muda entre compilações do mesmo código; pares vazios "printing object/stop printing object" fora da comparação. Recalculado sem fatiar de novo (`reavaliar.py`). A imagem do contêiner do Flatpak ganhou `libhidapi-hidraw0` para o compilado rodar.
- **Creality (raio-x):** dos 197 casos com diferença, 185 só em comentário (recurso "time flush" do binário, ausente da fonte). Nos modelos com CFS-C o próprio oficial sorteia a posição X da limpeza do bico (`G1 X… Y288.100 F30000`): duas execuções do oficial diferem em ~590 linhas desse tipo, e o compilado difere do oficial no mesmo tipo de linha. K1 Max/K2/K2 Pro/Z2 0.8: só linha em branco. K3 e SPARKX i8 em análise. Critério com o proprietário.
- **Atualização 13/11 07:30 UTC — quadro final Creality 7.3.1.6378 (99 máquinas × cubo + 4 cores = 198 casos; 1 idêntico, 197 com diferença):** 185 só em comentário ("time flush": `;TIME_FLUSH_CANDIDATE`, `;TIME_ELAPSED`, `time_flush_*` no cabeçalho; `time_flush_interval = 0` em todos os perfis, recurso desligado); 4 só uma linha em branco (K1 Max 0.4, K2 0.8, K2 Pro 0.8, Z2 0.8); 6 CFS-C (K1/K1C/K1 SE/K1 Max, com e sem 2025) só a posição X da limpeza do bico `G1 X… Y288.100 F30000`, que o próprio oficial sorteia (duas execuções do oficial diferem ~590 linhas desse tipo); K3: o próprio oficial varia muito entre execuções (A×B ~97 mil linhas de comando), compilado × oficial A ~4,9 mil — dentro da variação do oficial; **SPARKX i8 0.4: diferença real de motor** — oficial determinístico, a torre de limpeza do compilado sai 0,021 mm mais para dentro (`G1  Y292.949` × `Y292.970`, E 0.1881 × 0.1889), ~36 mil linhas da torre; o resto igual. Critério para o proprietário.
- **Anycubic `develop@987a3c2b` (compilação de 05:33):** parou em 537/578 (`build/anycubic.log`, 05:50 UTC): o `easy_log/stackstrace.hxx` do open-plugins define, em Release (`NDEBUG`), a macro `get_module_name(name, ...)` (em `0073895`; `get_module_name(name)` em `52f0d33`), que colide com o método `DeviceManager::get_module_name(ModuleID)` quando `funcation.hxx` inclui `DeviceManager.hpp` depois dela. O binário oficial 2.0.0.6 usa `get_stack_depth` mas não importa `get_module_name` (foi compilado com a macro, sem colisão): a fonte dele é outra. Saída possível sem editar a fonte: compilar sem `NDEBUG` (só flag; ativa os `assert`) — decisão do proprietário. O binário oficial também traz bibliotecas fechadas (`libcloud_sdk_cpp`, `libMachMQTT`, `libWorkbench`, `libcloud_mqtt`…), que não entram em módulo.
