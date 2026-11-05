# FILUNA — REGISTRO DE CONTINUIDADE E ETAPAS REALIZADAS

**ID** FLN-CONT-2026-11-05.1  
**Fuso** Europe/Rome (CET/CEST)  
**Atualizado** 2026-11-05 22:48:30 CET  
**Estado do gate G0** EM ANDAMENTO (não PASS)  
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
- **Escopo deste lote autorizado:** documentação de continuidade + execução parcial de **G0/Onda 00**; sem saltar para implementação grande de UI/DeviceBridge/remoto; sem merge de PR #1/#2; sem mudar branch padrão.

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
| 00 Genealogia / **G0** | **PENDING / EM ANDAMENTO** | Manifesto 141 fontes + conflitos principais registrados; leitura integral de todas as fontes **não** esgotada; assets binários de marca ausentes; SBOM completo do tree C++ **não** consolidado |
| 01 Fundação / G1–G3 fundação | PENDING | Checks estáticos existem; baseline G-code/Windows real não |
| 02 Qualidade Orca nativa | PENDING | |
| 03 Identidade/Home | BLOCKED até originais de marca no repo + G0/G2 | |
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
| G0-11 | Binários de identidade/mascote no git | **FAIL/PENDING** | só `FILUNA/brand/REFERENCIAS-MANIFESTO.json`; originais NÃO commitados → bloqueia G1 visual |
| G0-12 | Leitura integral das 141 fontes | **NOT RUN / PENDING** | manifesto indexa; auditoria semântica incompleta (já declarado no plano/diário) |
| G0-13 | SBOM/hash integral do tree C++ do fork | **PENDING** | milhares de fontes; fora deste lote |
| G0-14 | Teste Windows real / impressoras | **NOT RUN** | autorização atual: testes Windows ficam para depois |
| G0-15 | Criar este registro + vínculo no plano | **PASS** ao push deste PR | URLs na seção 7 após publish |

**Conclusão G0 deste lote:** progresso real de inventário/verificação, **G0 NÃO PASS**. Promoção para Onda 01/G1 **bloqueada**.

## 5. Decisões confirmadas vs dúvidas

**Confirmadas (dono):** Orca motor único 2.5-dev pinado; AGPL público; pasta `Filuna`; rebrand em fork; Elegoo Centauri Carbon 2 + Anycubic Kobra 3 V2 como piloto; `orcaslicer://` a assumir; quality docs em PR draft; iniciar cronograma sobre base atual; documentar continuidade para qualquer IA; Windows físico depois; sem merge automático dos PRs.

**Abertas / bloqueantes:** versionar 5 assets de marca; completar leitura normativa restante; tokens HEX finais; firmware exato das duas impressoras; quem assina instalador; política Bambu/Flashforge LAN-only vs plugin; escopo Hub/remoto (default OFF até gates); se importar perfis Orca do usuário no 1º run.

## 6. Última tarefa e próxima ação exata

- **Última tarefa:** criar registro de continuidade, vincular no plano mestre/AGENTS/README, executar checklist G0 acima, abrir PR draft para `filuna/quality-master-20261105`.
- **Próxima ação executável (lote seguinte, sem releitura de chat):**  
  1. Reler remoto deste REGISTRO + plano (podem ter avançado).  
  2. Continuar G0: ler na íntegra o lote prioritário S (`14-CONTINUIDADE-INTEGRAL`, `18-HOME-GROK`, `25-REGRA-MESTRE-ATUALIZACOES`, MESTRE-PREPARAR/DISPOSITIVOS/PARAMETRICOS, `04-MOTORES` para registro de superação) e W (`CONTINUAR-PROJETO`, `GROK_CONTRACT`, `ENGINE_CONTRACT`, `PROTOCOLOS-IMPRESSORAS`, `AUDITORIA-SEGURANCA-2026-11-05`) e registrar REQ/conflitos por arquivo no inventário.  
  3. Pedir ao dono o commit dos 5 originais de marca (ou caminho autorizado) — sem isso G1 permanece BLOCKED.  
  4. **Não** implementar DeviceBridge/UI Home nem mergear PR #1/#2 neste passo.

**Critérios para marcar G0 PASS (ainda não atingidos):** fontes obrigatórias esgotadas com precedência resolvida; mapa de conflitos sem item CONFLITANTE órfão no escopo G0; baseline pin/SBOM inicial aceito; dono ciente do que ficou fora; evidências no REGISTRO com PASS/URL.

## 7. Commits / PRs deste lote (preencher no push)

| Campo | Valor |
|---|---|
| Branch | `filuna/continuidade-g0-20261105` |
| Base no momento da criação | `d1935aef` (`filuna/quality-master-20261105`) |
| Commit tip (checkpoint) | `2818f403251d04a0abf7503c125c6ab4ad7a6cd7` (série c209f19d…2818f403) |
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
