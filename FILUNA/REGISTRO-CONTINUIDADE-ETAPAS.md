# FILUNA — REGISTRO DE CONTINUIDADE E ETAPAS REALIZADAS

**ID** FLN-CONT-2026-11-05.1  
**Fuso** Europe/Rome (CET/CEST)  
**Atualizado** 2026-11-05 23:02:32 CET  
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
| 00 Genealogia / **G0** | **PENDING / EM ANDAMENTO** | Manifesto **141/141 caminhos lidos** (119 blobs novos + 13 prévios; 9 dedup). Assets marca versionados. SBOM F apenas inicial. Conflitos CF-06..08 registrados. G0 **não PASS** |
| 01 Fundação / G1–G3 fundação | PENDING | Checks estáticos existem; baseline G-code/Windows real não |
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
| G0-12 | Leitura integral das 141 fontes | **NOT RUN / PENDING** | manifesto indexa; auditoria semântica incompleta (já declarado no plano/diário) |
| G0-13 | SBOM/hash integral do tree C++ do fork | **PENDING** | milhares de fontes; fora deste lote |
| G0-14 | Teste Windows real / impressoras | **NOT RUN** | autorização atual: testes Windows ficam para depois |
| G0-15 | Criar este registro + vínculo no plano | **PASS** ao push deste PR | URLs na seção 7 após publish |
| G0-16 | Leitura integral lote prioritário S/W (13) | **PASS** (leitura) | hashes = manifesto; ver inventário G0 |
| G0-17 | Confirmar tip plano final 52/149/SRV/113 | **PASS** (documental) | tip `d1935aef`; 113/113 ≠ G0 funcional |
| G0-18 | Incorporar 5 assets marca do ZIP dono | **PASS** | ZIP `02b3c842…915e`; 5/5 SHA match; nomes estáveis+alias conversa |
| G0-19 | Mapear SRV prioridades dono sem provisionar | **PASS** (inventário) | SRV-00..11; admin sem controle impressoras |


**Conclusão G0 deste lote:** progresso real de inventário/verificação, **G0 NÃO PASS**. Promoção para Onda 01/G1 **bloqueada**.

## 5. Decisões confirmadas vs dúvidas

**Confirmadas (dono):** Orca motor único 2.5-dev pinado; AGPL público; pasta `Filuna`; rebrand em fork; Elegoo Centauri Carbon 2 + Anycubic Kobra 3 V2 como piloto; `orcaslicer://` a assumir; quality docs em PR draft; iniciar cronograma sobre base atual; documentar continuidade para qualquer IA; Windows físico depois; sem merge automático dos PRs.

**Abertas / bloqueantes:** completar leitura normativa restante (G0); tokens HEX finais e QA visual (G1); firmware exato das duas impressoras; quem assina instalador; política Bambu/Flashforge LAN-only vs plugin; escopo Hub/remoto (default OFF até gates); se importar perfis Orca do usuário no 1º run. **Assets de marca:** versionados (não reabrir pedido).

## 6. Última tarefa e próxima ação exata

- **Última tarefa (2026-11-05 23:02:32 CET):** (a) 5 originais de marca validados/incorporados; (b) leitura integral dos **128** caminhos restantes do manifesto (119 blobs únicos, 9 dedup), inventário atualizado.
- **Próxima ação:** consolidar conflitos órfãos + SBOM F real de deps Windows; G1 tokens HEX; sem Home/DeviceBridge/SRV; sem merge PRs.


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
- G0 continua **EM ANDAMENTO**.


## 11. Checkpoint assets de marca (2026-11-05 23:00:49 CET)

ZIP dono `01M6PVH7P1FCG8M8SC5MB3JEB9.zip` (sha256 `02b3c8426ebdf481754f3658c42a147ce915906f52d36f2476032deeb4f2915e`), 711+ KB.
Validação segura (zipfile, sem traversal/symlink): **5/5 SHA-256 = manifesto**. Nomes no ZIP estáveis; alias de conversa preservados via hardlink.
Incorporados **somente** os 5 originais em `FILUNA/brand/assets/original/` (ZIP e `MANIFESTO-IMAGENS.json` extras **não** commitados).
**G1 não PASS:** faltam tokens HEX homologados, derivados e QA Windows. G0 continua independente da marca.


## 12. Checkpoint leitura manifesto 141 (2026-11-05 23:02:32 CET)

- 141/141 caminhos do manifesto cobertos por leitura de conteúdo (texto); 9 pares S↔W deduplicados por SHA.
- Inventário: [G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md](G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md) rev .3.
- G0 **ainda não PASS** (SBOM completo, aceite de conflitos herdados, evidências Windows).
