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
