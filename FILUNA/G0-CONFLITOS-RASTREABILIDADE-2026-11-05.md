# FILUNA G0 — Rastreabilidade de requisitos e conflitos

**ID** FLN-G0-CF-2026-11-05.3  
**Atualizado** 2026-11-05 23:24:00 CET (Europe/Rome)  
**Gate G0** **PASS DOCUMENTAL** — escopo: inventário/genealogia Onda 00. **Não** é G0 “funcional”, release, QA Windows nem perfeição.  
**Base normativa** tip quality-master `d1935aefa91a1eda86f57b67b342a3857247294e` · REGRA-MESTRE · PLANO Onda 00 · t17u/t18u.

> Cada conflito/REQ herdado: fonte → status → resolução. Não inventar decisões só do proprietário.  
> Estados de teste: **PASS / FAIL / BLOCKED / NOT RUN** apenas (sem “PASS parcial”).

## Legenda de status de conflito/REQ

| Status | Significado |
|---|---|
| **RESOLVIDO** | Decisão já dada (dono/plano/REGRA) com efeito no fork F |
| **SUPERADO** | Texto legado contradiz decisão atual; legado não é normativa F |
| **DOCUMENTADO** | Inventariado; fase futura nomeada; não é órfão G0 |
| **FORA_G0** | Pertence a G1+ / DeviceBridge / SRV impl. / G4 |

## Critérios EXATOS de G0 (rastreio à norma — sem derivados inventados)

### A. REGRA-MESTRE-QUALIDADE.md — tabela Portões, linha **G0 Inventário**

Texto normativo: *“Ler fontes mestras, identificar conflitos, baseline upstream/Filuna, riscos, arquivo por arquivo no escopo”*  
Evidência: *“Mapa de fontes versionado com caminhos, hashes/SHAs e status sem itens obrigatórios órfãos”*.

| # | Trecho REGRA | Critério | Resultado | Evidência |
|---|---|---|---|---|
| G0-R1 | Ler fontes mestras | Fontes mestras/obrigatórias lidas | **PASS** | [G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md](G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md) |
| G0-R2 | Identificar conflitos | Conflitos rastreados | **PASS** | CF-01..11 neste arquivo |
| G0-R3 | Baseline upstream/Filuna | Pin + mirror + base rebrand | **PASS** | UPSTREAM_PIN; mirror=`a620d804…`; rebrand=`2dace0db…` |
| G0-R4 | Riscos | Riscos documentados c/ tratamento | **PASS** | PLANO §8 + [G0-RISCOS-OEM-PROVENIENCIA-2026-11-05.md](G0-RISCOS-OEM-PROVENIENCIA-2026-11-05.md) |
| G0-R5 | Arquivo por arquivo + mapa path/hash/status | 141 mapeados; 0 UNKNOWN; sem órfãos obrigatórios | **PASS** | MANIFESTO + [G0-MAPA-141-STATUS-2026-11-05.md](G0-MAPA-141-STATUS-2026-11-05.md) |

**Explicitamente NÃO é G0 (REGRA mesma tabela):** G1 visual; G2–G3 implementação; **G4 Windows e hardware**; G5 pacote; G6 liberação.

### B. PLANO-MESTRE — Onda **00 — Genealogia** (linha da tabela de ondas)

Texto normativo da entrega: *“Inventário do site e do Windows antigos, documentos, contradições, testes, fonte por feature, matriz de migração, SBOM inicial”* · Gate: *“G0 100%; fontes obrigatórias esgotadas”*.

| # | Trecho PLANO Onda 00 | Resultado | Evidência |
|---|---|---|---|
| G0-P1 | Inventário site (S) e Windows antigos (W) | **PASS** | MANIFESTO 61S+80W; MIGRACAO §1 |
| G0-P2 | Documentos | **PASS** | dossiê FILUNA/ + legado indexado |
| G0-P3 | Contradições | **PASS** | CF-01..11; MIGRACAO §4 |
| G0-P4 | Testes (do inventário G0) | **PASS** | [G0-TESTES-CASOS-2026-11-05.md](G0-TESTES-CASOS-2026-11-05.md) G0-T01..T10 |
| G0-P5 | Fonte por feature | **PASS** | MIG-01..26 + [G0-MIG-DAG-ALINHAMENTO-2026-11-05.md](G0-MIG-DAG-ALINHAMENTO-2026-11-05.md) |
| G0-P6 | Matriz de migração | **PASS** | MIGRACAO-LEGADO §3 |
| G0-P7 | **SBOM inicial** | **PASS** | [sbom/README.md](sbom/README.md) `PARTIAL_NOT_COMPLETE` = inicial verificável (completo **não** exigido pela Onda 00) |
| G0-P8 | Fontes obrigatórias esgotadas | **PASS** | G0-FONTES-OBRIGATORIAS (justificativa: 141 históricos ≠ obrigatório normativo) |

### Correção de critérios derivados indevidos (revogados)

| Derivado indevido (lote anterior) | Por que é inválido | Correção |
|---|---|---|
| Exigir QA Windows / “N8 funcional Windows” para G0 | REGRA coloca Windows/hardware em **G4** | Removido de G0; G4 = **NOT RUN**/BLOCKED até teste do dono |
| Exigir SBOM **completo** / fechar lacunas DLL artifact para G0 | PLANO pede SBOM **inicial** | SBOM inicial PASS; completo = fase QA/REL posterior |
| Estado “PASS parcial” | REGRA: PASS/FAIL/BLOCKED/NOT RUN | Eliminado da checklist G0 |
| Pergunta ao dono “nenhum legado bloqueia G0?” como bloqueio | REGRA hierarquia: legado W = evidência, não homologação automática | CF-07 **RESOLVIDO** por hierarquia; pergunta **não** necessária |

## Conflitos rastreados

| ID | Descrição | Fontes | Precedência / decisão | Status | Precisa dono? |
|---|---|---|---|---|---|
| CF-01 | Vários motores Orca+Prusa+Cura+Kiri | S `04-MOTORES`, W ENGINE_CONTRACT | Orca **único** fatiamento; OEM = adaptadores | **SUPERADO** | Não |
| CF-02 | Updates classe D multi-motor | W UPDATE-RELEASE | Classe D F = runtime/pin Orca | **SUPERADO** | Não |
| CF-03 | Home/UI legado | S 18/19 | Reimplementar G1+ | **FORA_G0** | Não para G0 |
| CF-04 | Impressão com confirmação | REGRA + plano | Mantém | **RESOLVIDO** | Não |
| CF-05 | Admin SEM controle impressoras | t18u + REGRA servidor | Diagnósticos read-only | **RESOLVIDO** | Não |
| CF-06 | UPDATE lista 4 motores | W | = CF-01 | **SUPERADO** | Não |
| CF-07 | BLOQUEANTEs legado W 0.2.x | W PENDENCIAS | Hierarquia REGRA: W ≠ base nem homologação F; defeitos = referência para REQ futuros revalidados, **não** FAIL automático G0 | **RESOLVIDO** | Não |
| CF-08 | Google/Stripe cofre | FILUNA-ID | Inventário; não provisionar | **FORA_G0** | Segredos na etapa própria |
| CF-09 | DLLs/SDK OEM fechados | PLANO riscos | Sem bundling sem direito | **RESOLVIDO** política | Jurídico se OEM exigir SDK (LEGAL-02) |
| CF-10 | 113/113 ≠ release | AUDITORIA | Documental estático | **RESOLVIDO** | Não |
| CF-11 | “141 integrais” indevido | inventário | Profundidades A/B/C | **RESOLVIDO** | Não |
| CF-12 | Checklist G0 pedia Windows/SBOM completo | este arquivo rev .2 | Contradizia REGRA/PLANO | **RESOLVIDO** (revogado neste rev .3) | Não |

## CF-07 — resolução por hierarquia (sem nova pergunta)

REGRA-MESTRE §Hierarquia de fontes: (1) ordem do proprietário / regra homologada; (2) contratos S; (3) estado/código W = **evidência de implementação, não homologação por si**; (4) código F em SHA; (5) AGENTS/licença Orca.

Portanto itens “BLOQUEANTE” em `docs/PENDENCIAS.md` do legado W **não** são critérios PASS/FAIL do G0 documental F. Relevantes para o produto (ex.: Anycubic start, multi-peça) permanecem na matriz MIG/DAG das ondas futuras e devem ser **revalidados com evidência F** — não apagados.

Lista resumida (referência, não checklist G0): várias peças/pintura cor; paramétricos árvore/luminária; Anycubic início; planos/preço; estoque mobile; KB assistente; Meta/ML; Authenticode — ver inventário detalhado no histórico rev .2 / PENDENCIAS W.

## REQ produto (amostra) — status

| REQ | Status |
|---|---|
| Motor único Orca | RESOLVIDO |
| Pasta `%APPDATA%\Filuna` | RESOLVIDO |
| AGPL público + créditos | RESOLVIDO |
| Sem plugins rede proprietários no pacote | RESOLVIDO |
| Admin sem controle impressoras | RESOLVIDO |
| Impressão só com confirmação | RESOLVIDO (impl. FORA_G0) |
| Tokens HEX / aceite visual | **FORA_G0 → G1 NOT RUN** |

## Declaração de escopo — G0 PASS DOCUMENTAL

**PASS** significa exclusivamente:

1. Critérios G0-R1..R5 e G0-P1..P8 acima = **PASS** com evidência versionada.  
2. Sem itens obrigatórios órfãos no mapa.  
3. Casos G0-T01..T10 = **PASS** em [G0-TESTES-CASOS-2026-11-05.md](G0-TESTES-CASOS-2026-11-05.md).

**NÃO significa:** release; G1 visual aprovado; DeviceBridge; SRV provisionado; QA Windows/impressoras; SBOM supply-chain completo; “sem erros futuros”.

| Gate | Estado após este lote |
|---|---|
| G0 | **PASS** (documental / Onda 00) |
| G1 | **NOT RUN** (aceitação visual do dono obrigatória) |
| G2–G3 | **NOT RUN** |
| G4 | **BLOCKED**/NOT RUN até Windows+hardware do dono |
| G5–G6 | **NOT RUN** |

## Próximo passo (sem saltar implementação de produto)

1. **G1:** SPEC/contrato visual + tokens HEX + aceite explícito do proprietário para aparência (REGRA G1).  
2. Não mergear PR #3 sem ordem do dono.  
3. Windows/impressoras quando o dono testar → evidências no gate **G4**.  
4. SBOM completo / DLLs de artifact = trabalho de QA/REL (não reabre G0 inicial).
