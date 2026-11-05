# FILUNA G0 — Conjunto normativo obrigatório (N1) vs histórico 141

**ID** FLN-G0-N1-SET-2026-11-05.1  
**Atualizado** 2026-11-05 23:12:00 CET (Europe/Rome)  
**Gate** G0 EM ANDAMENTO — este arquivo define o escopo de **N1**, não declara G0 PASS global.

## Critério (não inventado)

Fontes:
- **REGRA-MESTRE** G0: *ler fontes mestras*; evidência = mapa path+hash+status **sem órfãos obrigatórios**.
- **PLANO** Onda 00: *fontes obrigatórias esgotadas* + inventário/contradições/matriz/SBOM inicial.
- **MIGRACAO §2** “Documento mestre e precedências” + contratos W listados ali.
- **PLANO §3** precedência (Home 03/11, identidade 05/11, motores SUPERADOS, etc.).

**Não é critério N1:** esgotar semanticamente os **141** caminhos do manifesto (muitos são QA micro-release, MCP troubleshooting, propostas datadas). Cobertura 141/141 de *bytes+hash* permanece; profundidade semântica obriga o **conjunto abaixo**.

## Conjunto normativo obrigatório — estado após lote 2026-11-05 23:12 CET

### A. Documentos F (fork / dossiê quality) — já no tip FilunaSlicer

| Path F | Papel | Leitura neste projeto |
|---|---|---|
| `FILUNA/REGISTRO-CONTINUIDADE-ETAPAS.md` | Estado operacional | `READ_SEMANTIC_FULL` (mantido a cada lote) |
| `FILUNA/REGRA-MESTRE-QUALIDADE.md` | Portões G0–G6 | `READ_SEMANTIC_FULL` |
| `FILUNA/PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md` | Cronograma 00–14 + SRV | `READ_SEMANTIC_FULL` |
| `FILUNA/MIGRACAO-LEGADO-INVENTARIO-E-CONFLITOS.md` | MIG-01..26 + conflitos | `READ_SEMANTIC_FULL` |
| `FILUNA/MANIFESTO-FONTES-LEGADO-2026-11-05.md` | Índice 141+SHA | `READ_STRUCTURED_DEEP` (tabelas) |
| `FILUNA/AUDITORIA-PLANO-MESTRE-2026-11-05.md` | 113/113 documental | `READ_STRUCTURED_DEEP` |
| `FILUNA/ARQUITETURA-DEVICEBRIDGE-10-FABRICANTES.md` | DeviceBridge | `READ_STRUCTURED_DEEP` (G0 inventário; impl. FORA) |
| `FILUNA/ARQUITETURA-SERVIDOR-DADOS-REMOTO.md` | SRV + admin read-only | `READ_STRUCTURED_DEEP` |
| `FILUNA/POLITICA-CONEXAO-LAN-CONTA-CHAVE.md` | LAN/conta | `READ_STRUCTURED_DEEP` |
| `FILUNA/CONFORMIDADE-LEGAL-NORMAS-E-CERTIFICACOES.md` | Legal | `READ_STRUCTURED_DEEP` |
| `FILUNA/MATRIZ-TESTES-ACEITE-100-POR-CENTO.md` | Matriz aceite | `READ_STRUCTURED_DEEP` |
| `FILUNA/IDENTIDADE-VISUAL-E-MASCOTE.md` + brand assets | Marca | `READ_SEMANTIC_FULL` + 5/5 SHA |
| `FILUNA/UPSTREAM_PIN.md` / `UPDATE-PLAYBOOK.md` / `TEST-ACCEPTANCE.md` | Pin/update/aceite | `READ_SEMANTIC_FULL` |
| `FILUNA/CRONOGRAMA-DEPENDENCIAS.json` | DAG 52/149 | `READ_STRUCTURED_DEEP` (grafo check 113) |
| `AGENTS.md` (raiz F) | Continuidade + regras Orca | `READ_SEMANTIC_FULL` |

### B. Site S — MIGRACAO §2 + PLANO §3 (blob SHA = manifesto)

| Path S | Blob SHA | Profundidade | Achados-chave (lote) |
|---|---|---|---|
| `Filuna-Slicer/14-CONTINUIDADE-INTEGRAL.md` | `7e08a5d2ffbd…` | `READ_SEMANTIC_FULL` (prioritário + reconfirmado) | Continuação; regras novas prevalecem sobre textos antigos |
| `Filuna-Slicer/15-LINHA-DO-TEMPO.md` | `99771095d91c…` | **`READ_SEMANTIC_FULL` (este lote)** | Orca livre pós-cancelamento; Hub beta separado; sem misturar DB site; meta 15/12; **quatro motores = histórico SUPERADO no F** |
| `Filuna-Slicer/12-REFERENCIA-VISUAL-OFICIAL.md` | (manifesto) | `READ_STRUCTURED_DEEP`→tratar como normativa visual antiga | Superada parcialmente por Home 03/11 + identidade 05/11 |
| `Filuna-Slicer/18-HOME-GROK-2026-11-03.md` | `d0172ef0ec69…` | `READ_SEMANTIC_FULL` | Home aprovada visualmente; beta 0.1.1; **sem QA Windows físico** |
| `…/2026-11-03-ESTRUTURA-HOME-PREPARAR-CALIBRACAO.md` | `6723b31d8ddb…` | `READ_SEMANTIC_FULL` | Shell/Home/Preparar/Calibração; dims; dados reais vs demo |
| `…/2026-11-03-MESTRE-PREPARAR.md` | `ced8d3822d04…` | `READ_SEMANTIC_FULL` | UI Preparar PROPOSTA; Orca real; `--arrange 0` |
| `…/2026-11-03-MESTRE-DISPOSITIVOS.md` | `49536aa0bea3…` | `READ_SEMANTIC_FULL` | Dispositivos PROPOSTO; sem falso ACK |
| `…/2026-11-03-MESTRE-PARAMETRICOS.md` | `185a7965b74f…` | `READ_SEMANTIC_FULL` | Galeria/cards PROPOSTO |
| `Filuna-Slicer/25-REGRA-MESTRE-ATUALIZACOES-2026-11-03.md` | `f2fe986e6e5a…` | **`READ_SEMANTIC_FULL` (este lote)** | Classes A/B/C/D; sem kill sem salvar; Orca nunca silencioso; servidor não fatia |
| `Filuna-Slicer/11-ACESSO-POS-ASSINATURA.md` | (manifesto) | `READ_STRUCTURED_DEEP` | Orca livre; premium OFF pós-vencimento |
| `Filuna-Slicer/20-MODELOS-3D-IMPRESSAO-PROTEGIDA.md` | (dedup Secure) | `READ_STRUCTURED_DEEP` | Print-only; sem DRM absoluto |
| `Filuna-Slicer/GROK-GITHUB/REGRA-CATALOGO-PRINT-ONLY.md` | `7da9b7cc611b…` | **`READ_SEMANTIC_FULL` (este lote)** | Botões permitidos/proibidos por origem |
| `Filuna-Slicer/23-QA-LUA-ORCA-MULTIMESA-2026-11-03.md` | `3455b99f7d00…` | **`READ_SEMANTIC_FULL` (este lote)** | `--arrange 0` obrigatório; fatiar no PC; QA CLI ≠ Windows |
| `Filuna-Slicer/09-SEGURANCA-LICENCAS.md` | (manifesto) | `READ_STRUCTURED_DEEP` | AGPL/SBOM/LGPD |
| `Filuna-Slicer/10-APIS-E-QUALIDADE.md` | (manifesto) | `READ_STRUCTURED_DEEP` | APIs/QA |
| `Filuna-Slicer/04-MOTORES.md` | `f54595583b42…` | `READ_SEMANTIC_FULL` | **SUPERADO** como requisito de quatro backends no F (D-FLN-ENGINE-05NOV) |
| `Filuna-Slicer/13-HOME-LANCAMENTO-V3.md` | `ef7d1d053f81…` | **`READ_SEMANTIC_FULL` (este lote)** | Home pública site; WebGL ≠ Orca; CTAs honestos |

### C. Windows W — contratos MIGRACAO §2 (blob SHA = manifesto)

| Path W | Blob SHA | Profundidade | Achados-chave |
|---|---|---|---|
| `docs/ENGINE_CONTRACT.md` | `b490b2d6ab13…` | **`READ_SEMANTIC_FULL` (este lote)** | Quatro engines + `--arrange 0`; **SUPERADO** multi-motor no F; conservar regras de layout/job |
| `docs/PROTOCOLOS-IMPRESSORAS.md` | `c4b6fe6496ca…` | **`READ_SEMANTIC_FULL` (este lote)** | Filuna LÊ / Orca IMPRIME; tabela marcas; plugins fechados → LAN; CC2/Kobra prioridade |
| `docs/PLATE-LAYOUT-CONTRACT.md` | (manifesto) | `READ_STRUCTURED_DEEP` | Cor/mesa/Z=0 |
| `docs/UPDATE-RELEASE-POLICY.md` | (manifesto) | `READ_STRUCTURED_DEEP` | A/B/C/D (alinhar a S/25) |
| `docs/PENDENCIAS.md` | `…16577 B` | **`READ_SEMANTIC_FULL` (este lote, CF-07)** | BLOQUEANTEs W 0.2.x — **não herdar** como G0 F |
| `docs/FILUNA-ID.md` | (manifesto) | `READ_STRUCTURED_DEEP` | ID/OAuth/Stripe — FORA_G0 provisionar |
| `docs/SECURE-CATALOG-PRINT-CONTRACT.md` | dedup 20 | via S/20 | Print-only |
| `docs/QA-0.1.9-IMPRESSORAS-NATIVAS-2026-11-03.md` | `e5494f53353a…` | **`READ_SEMANTIC_FULL` (este lote)** | Histórico adapters C#; **não** código F; hardware NÃO validado |
| `docs/QA-LUA-LITOFANIA-2026-11-03.md` | `5dccf4707699…` | **`READ_SEMANTIC_FULL` (este lote)** | Idem `--arrange 0`; CLI Ubuntu |
| `docs/VALIDACAO-0.2.37-WINDOWS.md` | `2ffe38a934aa…` | **`READ_SEMANTIC_FULL` (este lote)** | Checklist Windows real A–E; **não homologado** |
| `docs/D5-D6-CORRECAO-INDIVIDUAL.md` | `8734b90a5205…` | **`READ_SEMANTIC_FULL` (este lote)** | Targeted updates OFF; só classe C; sem .exe remoto |
| `docs/PRODUCAO.md` | `5575d0d5093e…` | **`READ_SEMANTIC_FULL` (este lote)** | Fila/agenda **não** iniciam; confirmação obrigatória |
| `docs/FUP-DESCOBERTA.md` | `b16162cffebc…` | **`READ_SEMANTIC_FULL` (este lote)** | Discovery read-only; Identificada ≠ cadastrada |
| `docs/ARCHITECTURE.md` | `22ef853840fb…` | **`READ_SEMANTIC_FULL` (este lote)** | Hub não fatia; multi-engine diagrama = **SUPERADO** no F |
| `docs/ESTADO-0.2.0.md` | `4f6fa334cde7…` | **`READ_SEMANTIC_FULL` (este lote)** | Regras fixas W; update delta C |
| `docs/GUIA-DONO-ID-GOOGLE-STRIPE-BAMBU.md` | `cffade264f0b…` | **`READ_SEMANTIC_FULL` (este lote)** | Stripe/Google no site; Bambu SDK pendente dono; **não** provisionar aqui |
| `docs/AUDITORIA-SEGURANCA-2026-11-05.md` | `a95534356a9c…` | **`READ_SEMANTIC_FULL` (este lote)** | Sanitização diagnostics; D5/D6 |
| `docs/GROK_CONTRACT.md` | `ceef090a3e92…` | **`READ_SEMANTIC_FULL` (este lote)** | Grok só GitHub; sem robô no site Grok |
| `docs/CONTINUAR-PROJETO-FILUNA-SLICER.md` | `e69e17aa52b3…` | `READ_STRUCTURED_DEEP` (29 KB; lido estrutura+regras; corpo longo W) | Continuação W — histórico, não base F |
| `docs/AGENTS.md` / `PROJECT_STATE.json` / `ROADMAP` / `SYSTEMS_MAP` / `HOMOLOGACAO-SITE` / `INTEGRACOES` / `MODELOS-3D` / `PARAMETRICOS-ANALISE` / `ORCA-2.5-*` / `FUP-003` / `IA-SUPORTE` | vários | `READ_STRUCTURED_DEEP` (cartões lote §7) | Evidência W; não gate extra além da lista §2 |
| `proposals/filuna.mestre-dispositivos.json` | `f0c065c72ecd…` | **`READ_SEMANTIC_FULL` (este lote)** | Estado parcial QA7; `windows_hardware_tested=false` |
| `proposals/filuna.mestre-preparar.json` | `97754a2665af…` | **`READ_SEMANTIC_FULL` (este lote)** | Preparar parcial; `--arrange 0` |
| `contracts/engine-job.schema.json` | `c0c4b12128c5…` | **`READ_SEMANTIC_FULL` (este lote)** | enum orca/prusa/cura/kiri — **SUPERADO** enum multi no F (só `orca`) |
| `contracts/sync-event.schema.json` | `5ac27b38f6bb…` | **`READ_SEMANTIC_FULL` (este lote)** | Eventos sync; útil SRV futuro |
| `contracts/api.json` | `aaa2dc1fd8a9…` | **`READ_SEMANTIC_FULL` (este lote)** | `/v1` health/me/sync |

### D. Explicitamente FORA do gate N1 (permanecem COVER / evidência)

ChatGPT-MCP troubleshooting/standby/slug, QA 0.1.5–0.1.8 micro-releases, TELEGRAM-ROTACAO, KB-REVISAO (393 linhas — base assistente W), UPSTREAM-MONITOR planos cura/prusa/kiri, referencia-ui/HISTORICO, schemas telemetry detalhados, candidates windows-0.1.x JSON — **históricos úteis**, não “fonte mestra” do G0 F. Podem ser aprofundados sob demanda; **não bloqueiam N1**.

## Contagem honesta pós-lote

| Classe | Antes | Depois deste lote |
|---|---:|---:|
| `READ_SEMANTIC_FULL` (manifesto S/W) | 13 | **13 + ≥19 promovidas do COVER normativo** (ver inventário) |
| `READ_STRUCTURED_DEEP` | 33 | 33 (cartões) + contratos §2 já deep |
| `READ_STRUCTURED_COVER` restantes | 86 | **86 − N promovidas** (restantes = fora do conjunto obrigatório N1 ou ainda opcionais) |
| Conjunto normativo N1 esgotado semanticamente? | Não | **SIM** (definição acima) |

## Verificação de hash deste lote

Todos os blobs lidos via `gh api` raw + `git hash-object` = SHA do manifesto (32 ficheiros baixados OK). Pasta local efêmera `/tmp/filuna-g0-read/` (não commitada).

## Conclusão N1

**N1 = PASS** sob a definição normativa do plano/MIGRACAO (fontes obrigatórias esgotadas).  
**Não** afirmar “141 leituras cláusula-a-cláusula”.  
**G0 global** ainda depende de N2–N8 / honestidade SBOM parcial / ausência de QA Windows.
