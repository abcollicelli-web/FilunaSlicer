# FILUNA — REGISTRO DE CONTINUIDADE E ETAPAS REALIZADAS

## Checkpoint atual — integração Windows ainda bloqueada (08/11/2026, Europe/Rome)

O proprietário reafirmou literalmente às17:47: «entao faça tudo perfeito e completo». Isso autoriza continuar o escopo de integração nativa e assinatura, mas não constitui prova de implementação, aceite visual derivado ou homologação. A tentativa subsequente não alterou nenhum arquivo do aplicativo Windows, não gerou patch de código, PR, compilação, instalador, assinatura ou release. A autenticação GitHub foi confirmada; o impedimento é o caminho de implementação disponível nesta execução, não falta de acesso ao repositório. Não anunciar implementação em curso enquanto esse impedimento persistir.

A pesquisa de caminhos suportados realizou três buscas e cinco leituras públicas, sem encontrar uma rota efetivamente utilizável nesta execução. As rotas oficiais documentadas são os [agentes na nuvem](https://cursor.com/docs/cloud-agent), o [painel de agentes](https://cursor.com/agents) e a [CLI do Cursor](https://cursor.com/docs/cli/overview); a existência dessas opções não significa que estejam habilitadas ou autorizadas neste fluxo. A tabela pública de [planos](https://cursor.com/pricing) não confirma o plano individual nem garante a habilitação das ferramentas desta conversa. Nenhum plano, conta ou acesso foi modificado. O usuário pode verificar a elegibilidade no painel oficial ou esclarecer a divergência com o suporte; não prometer que apenas trocar o plano resolverá esta sessão.

O último resultado funcional permanece a Fase05.2 privada: 196 testes PASS em temporários, sem ativação operacional. Esta tentativa não reexecutou esses testes. Integração nativa Windows, assinatura real GitHub/Windows/manifesto e primeiro download beta/admin continuam PENDENTES; nenhuma versão antiga deve ser apresentada como integrada. Migrações006/007 não foram aplicadas operacionalmente, não houve restart/deploy do serviço ou exclusão de instaladores. Nenhuma frente de implementação permanece ativa neste checkpoint. Gates G2/G3 BLOCKED, G4 NOT RUN e G5/G6 BLOCKED permanecem. Homologação geral continua condicionada à prova da integração nativa.


## Estado mais recente — Fase05.2: lifecycle privado real, SOMENTE TEMP

08/11/2026 Europe/Rome: biblioteca interna de promoção, retenção e recuperação implementada em `/srv/filuna-new-site`; **196 PASS / 0 FAIL = 173 baseline + 23 novos** (baseline também reexecutado separadamente). Dez interrupções reais de processo, concorrência, falhas FS/SQL e HTTP privado em testes isolados. Limite global de **cinco VERSÕES físicas**, inclusive staging/trash, preservando todo histórico; active/rollback/pins/leases protegidos e sexta promoção negada se todas estiverem protegidas. Uma falha após poda pode deixar quatro versões: não há promessa de atomicidade FS+SQL ou restauração de bytes já removidos.

**Nada ativado/publicado/excluído operacionalmente:** DB continua001–005,006/007 apenasTEMP; zero dados reais; processo2089150 UID33 inalterado; sem trust/authconfig/root de releases operacional. Sem novo endpoint HTTP mutante, sem restart/deploy, frontend/app/auth/storage intactos. ECDSA efêmera real deTEST; provas GitHub/Windows/QA continuam fixtures, não validação operacional. Assinatura GitHub não substitui assinatura Windows/Filuna nem manifesto ECDSA. Windows updater/helper/consent/save/close/reopen/rollback e corte físico de energia NOTRUN. GatesG2/G3BLOCKED,G4NOTRUN,G5/G6BLOCKED inalterados. Autoridade continua t36/t37/t38+t43; publicação documental anterior não acrescenta autorização.

[Implementação, evidências, hashes e limites](WEB-FASE-05-2-LIFECYCLE-EVIDENCIAS-20261108.md). Os checkpoints antigos abaixo são históricos e não devem ser lidos como estado atual.


## Estado mais recente — Fase05.1 privada implementada/testada, NÃO ativada

08/11/2026 Europe/Rome, após recuperação autorizada t43 às17:04: código real de catálogo/ledger candidato, bytesstreaming privados e planner retenção existem em `/srv/filuna-new-site`, **173 testesPASS/0FAIL =148baseline+25novos**. As afirmações abaixo de bloqueioSSH/ausência total de implementação são checkpoints HISTÓRICOS, superados somente nesta subfatia. Não foi publicada primeira versão integrada, nem criada confiança operacional;006 só em DBtemporário, processo operacional NÃO reiniciado e domínio estático intacto. [Evidência, rotas efetivas xcarregadas e limitações](WEB-FASE-05-IMPLEMENTACAO-EVIDENCIAS-20261108.md). GatesG2/G3BLOCKED,G4NOTRUN,G5/G6BLOCKED permanecem; nenhuma homologação/assinatura/certificação/updaterWindows inferida.


## Prioridade vigente — ordens t36–t38 de 08/11/2026

O proprietário autoriza iniciar AGORA a integração nativa de atualizações Slicer/servidor e o fluxo de assinatura pelo GitHub conforme regras. Primeira distribuição só beta/admin; até CINCO versões de instaladores, preservando o histórico documental; compilar somente quando necessário. A retomada geral/homologação fica condicionada à integração comprovada. Esta ordem atualiza a pausa histórica somente nesse escopo; não promove gates nem aprova telas derivadas.

**Estado neste checkpoint: especificação proposta, código e testes novos NOT RUN, nenhum instalador beta integrado publicado.** A sessão SSH antiga deixou de responder; uma conexão normal nova alcança o mesmo servidor, mas requer autenticação. Nenhuma chave/configuração foi alterada. Ler [pedido literal e limites](approvals/ATUALIZACAO-NATIVA-BETA-T36-T38-20261108.json) e [contrato pré-código](WEB-FASE-05-RELEASE-CONTRATO-20261108.md) antes de continuar.


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


## Direção visual C — referência aprovada em 08/11/2026

Antes de criar ou alterar qualquer tela Filuna, consultar `FILUNA/design/visual-c/documentacao/DESIGN-SYSTEM-C.md`, a aprovação literal em `FILUNA/design/visual-c/documentacao/APROVACAO-VISUAL-C.json`, os dois modelos C aprovados e a galeria/matriz em `FILUNA/design/visual-c/README.md` e `FILUNA/design/visual-c/MATRIZ-COBERTURA.md`. O proprietário aprovou a direção visual; as demais telas e detalhamentos são propostas em revisão. O inventário consolidado contém 123 pranchas e 208 itens rastreados, sem equivaler à homologação de software.

Preparação permanece sem painel direito de informações de arquivo; Orca é o motor único. A regra vigente de G-code imutável e a distinção entre risco OEM e divergência de motor permanecem intactas. Esta aprovação não retoma implementação, CI, hardware, distribuição, servidor ou website. No escopo pausado pelo proprietário, G2/G3 permanecem BLOCKED, G4 NOT RUN e G5/G6 BLOCKED. Nenhum requisito funcional ou registro histórico foi revogado.

## Ordem expressa — fidelidade visual obrigatória (08/11/2026 14:16 Europe/Rome)

Texto do proprietário, ipsis litteris:

> adicione isso ao documento mestre do Filuna Slicer onde deve ser obrigatorio o resutaldo 100% identifo visual seguindo as imagens e regras de montagem e recursos para fazer com essa estrutura e visual que voce vai deixar pronto, nenhuma ia pode construir o designer do aplicativo diferente do modelo aprovado

Decisão: [FLN-VISUAL-C-100 no plano](PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md#fln-visual-c-100), [regra operacional](design/visual-c/documentacao/REGRA-FIDELIDADE-VISUAL-C.md), [recursos e montagem](design/visual-c/documentacao/CONTRATO-MONTAGEM-VISUAL-C.md) e [registro da ordem](design/visual-c/documentacao/ORDEM-FIDELIDADE-VISUAL-C-20261108.json). Baseline 720291b8263e0898f7d6723bc904112ba2c6b3eb; âncoras e aprovação t26u preservadas. Proibida reinterpretação por qualquer IA; resultado 100% idêntico exige evidência real e aceite humano, sem tolerância autoconcedida. Derivadas e lacunas mantêm pendência e bloqueiam o item. Apenas documentação preparada neste lote; implementação/QA visual da aplicação NOT RUN. G2/G3 BLOCKED, G4 NOT RUN, G5/G6 BLOCKED; nenhuma retomada funcional.


## Checkpoint web — autorização e inventário inicial (08/11/2026, Europe/Rome)

- Solicitação expressa de home/admin/serviços + **«esta liberado»**, registrada às 14:57 Europe/Rome. [Texto integral e autorização](approvals/WEB-SERVICOS-AUTORIZACAO-20261108.json); [contrato](WEB-SERVICOS-ESCOPO-20261108.md). Não é novo aceite das 206 derivadas nem retomada geral do desktop.
- Base documental remota confirmada em `filuna/visual-c-20261108`: `d6074987e090f87fc65cbfff6d1c60013a5692af`; baseline artes `720291b8263e0898f7d6723bc904112ba2c6b3eb`, sem alteração.
- Lidos neste checkpoint: AGENTS, este registro, plano mestre, regra de qualidade, arquitetura servidor, README Filuna, DAG; módulos/atualização, regra três camadas, prioridade OEM; DESIGN-SYSTEM-C, regra de fidelidade e guia de montagem C. Não se afirma leitura integral do restante do dossiê.
- Ambiente novo: os quatro caminhos exatos fornecidos não existem no filesystem disponível; não criados artificialmente. NEW-SITE e guias VAULT/RESTORE **não lidos**. Host/método autorizado de acesso remoto ainda não identificado neste contexto; não se inferiu SSH pelo domínio público nem se buscou valor de credencial.
- Mestres Slicer e servidor ampliados em continuidade neste commit documental, com ownership/árvore a verificar no guia real, contratos/versões/rollback e testes de aceite. A publicação registra o contrato e a autorização; não comprova acesso ou implementação no servidor.
- **Executado neste lote:** leitura, confirmação remota e checks documentais somente. **NOT RUN:** home, shell admin, backend, API executável, schemas/migrações, testes web, preview/deploy e toda integração desktop. Cofre, legado, runtime, domínio vivo e impressoras não tocados. G2/G3 BLOCKED, G4 NOT RUN, G5/G6 BLOCKED no desktop.
- **Próxima ação concreta:** pelo caminho remoto autorizado que detém a área nova, ler `/srv/filuna-new-site/docs/NEW-SITE.md` e listar somente raiz/estrutura nova; comprovar stack, ownership e isolamento. Em seguida implementar e testar a primeira fatia: home candidata C + admin negado por padrão + contrato API/schema versionado conforme stack real. Não inventar segunda árvore local nem implantar mock em produção. Política comercial core livre/offline versus chave deve ser resolvida antes de sua ativação; investigação/contratos e home candidata independentes podem avançar.


## Checkpoint — acesso recuperado e primeira fatia web real (08/11/2026, Europe/Rome)

Acesso autenticado pelo proprietário permitiu ler integralmente NEW-SITE e somente os dois guias permitidos do cofre. A limitação de acesso registrada no marco anterior é **superada neste marco**, sem apagar a história. Área nova estática/independente confirmada; cofre/legado não restaurados nem explorados. Home C candidata, shell não conectado a auth, prévia Python stdlib/SQLite vazio, política/contratos/schema e migração iniciais implementados **somente em `/srv/filuna-new-site`**. Nenhum código web privado é incorporado ao repo público.

[Primeira fatia e evidências reais](WEB-FASE-01-EVIDENCIAS-20261108.md): 33 testes PASS/0 FAIL no subconjunto especificado; HTTP preview real loopback; HTTPS público conserva home de transição, admin 401 e API/rotas novas 404. Nenhuma modificação de Nginx/SSH/OS/serviço alheio. Modelo web candidato ainda sem captura/aceite visual; auth de aplicação/signature verifier ausentes, APIs privadas negadas com 503, não falso sucesso.

**Decisão literal posterior de 15:18 Europe/Rome:** [registro](approvals/WEB-ACESSO-BASE-ORCA-20261108.json). Básico Orca e modelo-base livres/offline; sem assinatura válida, não carregar perfis/modelos homologados Filuna e usar MESMA engine em modo base limpo. Sem apagar arquivos/criações/dados nem revogar licenças livres. Dúvida core/premium resolvida; prazos offline/cache/revogação ainda pendentes. Dois mestres e contrato incorporam esta decisão e a política executável da prévia.

Próxima ação: revisão browser/humana da home candidata, protocolo visual e evolução de identidade/autorização/assinatura com testes; não promover tráfego/API pública antes da prontidão/fluxo aprovado. Não retomar desktop, CI geral ou hardware. G2/G3 BLOCKED, G4 NOT RUN, G5/G6 BLOCKED do Slicer preservados. Mestre local do servidor em `docs/PLANO-MESTRE-SERVIDOR-WEB.md`, NEW-SITE e evidências atualizados a este marco.


<a id="central-cliente-identidade-20261108"></a>
## Continuidade — Central do cliente e identidade verificável (08/11/2026, Europe/Rome)

**Novo pedido e aceite visual condicional do proprietário, t33u, 15:36 Europe/Rome — ipsis litteris:**
> esta bom a unica coisa que precisa ter precisa ser um acesso ao cliente para ele contratar, assinar, acessar seus backup, seus recursos, seus download, e isso sera expandido conforme novas ferrasmentas para o cliente serao criadas

[Registro novo da decisão e limites](approvals/WEB-CENTRAL-CLIENTE-T33-20261108.json). O visual apresentado foi aceito com a adição de acesso claro à **Central do cliente**. Manter hero, paleta C, Inter e marca; não redesenhar. Isso NÃO é homologação funcional, pixelperfect 100%, aprovação de derivadas, cobrança ou deploy geral. A entrada superior foi adicionada à home candidata; nova central tem navegação canônica `/conta/` e `/conta/acesso/`, com Contratar, Assinatura, Backups, Meus recursos e Downloads. Registro versionado de módulos governa ID/rota/owner/capability/handler; evoluir sem menus/pastas paralelos. Client e operador admin são separados.

**Estado real da fase02:** implementação somente na área nova; verificação Ed25519 real com PyJWT/cryptography, issuer/audience/datas/alg/tipo/kid controlados, identidade resolvida server-side, RBAC/membership por tenant e dados por conta; role admin exige MFA e não libera dados de clientes. Rotação/revogação de chaves/tokens e revisões anti-rollback testadas; bearer não é one-time/DPoP. Licença do recurso é verificação separada da assinatura. Manifesto JWS é validado criptograficamente, mas a API continua somente metadados, sem alegar instalação/bytes verificados no cliente.

**Operação ainda fechada:** provedor de login, configuração pública legítima de confiança e signer NÃO instalados/configurados; nenhum usuário ou direito real emitido. APIs privadas seguem 503 AUTH_NOT_CONFIGURED na prévia normal. Login browser/BFF/sessão interativa pendentes. Página pública da central explica isso em HTML navegável, sem senha/token fake, checkout inventado ou 503 bruto. Com chaves efêmeras somente em memória e DBs sintéticos temporários, testes HTTP positivos e negativos exercitaram leitura privada somente da própria conta. Não apresentar fixture como integração de produção.

[Resultados, arquivos e limites da fase02](WEB-FASE-02-EVIDENCIAS-20261108.md): **76 testes PASS** (33 baseline + 28 de segurança + 15 da central), dependências fixadas com SHA256 em venv da área nova; migrações 002/003 aditivas após backup apenas do DB novo, 001 intacta, integridade/foreign keys conferidas e tabelas operacionais vazias. Nginx/OS/tráfego/legado/cofre não alterados; nova central NÃO publicada no domínio. Código privado web e configurações não integram este repositório público.

Decisão t32 preservada: básico Orca e modelo-base livres/offline na MESMA engine; assinatura inválida impede carregar perfis/modelos homologados premium Filuna, sem apagar arquivos próprios nem direitos livres. Prazo offline/cache/revogação comercial, preços/planos/provedor e limites continuam sem defaults inventados. Storage/sync/restauração, entrega de downloads, contratação/cobrança e integração desktop não operacionais. A captura anterior em Chrome GUI offline prova apenas os trechos vistos; inspeção da nova entrada/central e mobile continuam pendentes. FLN-VISUAL-C-100, evidência humana e G2–G6/veto à retomada do Slicer permanecem.

Este marco supera somente as afirmações históricas de ausência de verificador/revisão da fase01: agora há verificação criptográfica implementada/testada e aceite visual condicional t33 da captura, mas ainda NÃO há provedor/signer operacionais nem sessão browser. Mestre local, NEW-SITE, ledger de migração, continuidade e evidências atualizados em `/srv/filuna-new-site/docs/`. Próxima ação concreta: inspecionar export home+central e preparar integração de provedor de identidade/signer legítimos por configuração segura; não promover tráfego ou exigir segredo em chat. Nenhum teste Slicer, build, hardware ou gate de release realizado.


<a id="storage-restore-bytes-20261108"></a>
## Continuidade — storage, restore e entrega autorizada de bytes (08/11/2026, Europe/Rome)

Subfatia efetivamente implementada SOMENTE em `/srv/filuna-new-site`: objetos privados opacos, versões imutáveis próprias, upload explícito por consentimento, SHA256/tamanho, CAS entre PCs, idempotência e ACK após fsync/commit; restore cria NOVA versão, preserva história. Identidade/membership server-side revalidadas depois da leitura e dentro da transação. Nenhuma assinatura suspensa apaga/bloqueia arquivos próprios; básico Orca livre/offline na mesma engine preservado.

Rotas de prévia: `/api/v1/tenants/{tenant}/files`, `.../files/{file}/versions`, `.../versions/{revision}/bytes`, `.../files/{file}/restore`; download de catálogo em `.../content/{content}/bytes`, exigindo publicação aprovada/JWS real, licença independente, grant premium quando aplicável, engine/compatibilidade e bytes/hash/tamanho verificados. Não expor URL de filesystem. Nenhum artefato real/instalador foi publicado; testes usam exclusivamente conteúdo sintético temporário e chaves efêmeras em memória.

**111 PASS / 0 FAIL** (76 baseline + 35 storage), pip check PASS, sem ResourceWarning. Migração 004 aditiva JÁ aplicada uma vez após backup somente do DB novo, versões 001–003 intactas, integridade/foreign keys ok. Estado operacional: zero contas/versões/bindings/objetos, provedor/signer/configuração legítima AUSENTES, privado continua 503 AUTH_NOT_CONFIGURED. Login email/Google/browser não implementado/configurado por esta fatia; nenhum Google/SMTP/serviço externo criado. Não confundir teste positivo com concessão real.

Filesystem+SQLite **não são transação distribuída atômica**: falha antes do commit pode deixar objeto órfão privado, reportado sem deletar; referência ausente/corrompida bloqueia bytes/restore/retry. Snapshot de operador copia DB+objetos referenciados com writers excluídos, verifica integridade e só conclui manifesto após fsync. Restore efetivamente testado em diretório isolado, nunca sobre DB ativo. É backup no MESMO host, não offsite/PITR/DR comercial nem RPO/RTO comprovados; não houve corte físico de energia. Falhas IO/crash foram injetadas em teste.

Transporte inicial: default 8 MiB, configurável com teto técnico 128 MiB, timeout de body 10s; não é quota/plano/janela offline. Sem streaming/resume grande ainda. Compatibilidade firmware aceita literal `==...`; range complexo bloqueia 409, declaração do cliente não é atestação. Sem Range/ETag/CDN; Range/If-None-Match rejeitados 400. Content-Digest/hash calculados dos bytes verificados. Downloads não instalam/controlam desktop/impressora nem executam/geram/fatiam objetos.

[Arquivos, hashes, testes e lacunas](WEB-FASE-03-EVIDENCIAS-20261108.md). Contrato privado canônico `contracts/object-storage.v1.json`, OpenAPI e bindings dos módulos atualizados; mestre/NEW-SITE/ledger/continuidade locais atualizados. Cofre/legado/Nginx/OS/tráfego/domínio/Slicer/hardware não alterados; nenhum gate promovido. Sem política comercial/retenção/GC/preço inventados.

### Revisão visual posterior t34u — 15:57 Europe/Rome
> minha colaboracao as paginas estao muito com cara de wordpress travado layout pronto precisa ser moderno e perfeito como o visual do filuna pagina premiada

[Registro literal e limites](approvals/WEB-VISUAL-REVISAO-T34-20261108.json). Palavra WordPress é comparação, não autorização para instalar CMS/plugin. O pedido posterior torna o aceite visual anterior condicionado à NOVA revisão DESIGN: **home/central NÃO homologadas**. Backend não altera hero/CSS/markup e não copia template para login. Frente DESIGN candidata separada, sem segundo backend; futura integração só após revisão coordenada. CU anterior navegou Central, cinco módulos e Acesso seguro no export offline, desktop sem recortes inesperados nos trechos vistos; não expôs dados privados nem comprovou mobile/HTTP vivo/100%. Isso não anula a crítica posterior nem promove FLN-VISUAL-C-100.


## Fase04 web — dual login implementado, não ativado (08/11/2026 Europe/Rome)

**Registro literal obrigatório** — decisão do dono no widget **t33s249**, **08/11/2026 15:52 Europe/Rome**: «Quero login com e-mail e senha e também com conta Google.» Esta escolha autoriza login da aplicação por ambos os métodos, não criar Google Project/OAuth app/SMTP externo, cobrar ou enviar email real nesta execução. Corrige lacuna de registro anterior.

Backend exclusivo `/srv/filuna-new-site`: Argon2id/email verificado/reset, sessões opacas BFF/cookie Secure HttpOnly SameSite, CSRF/origem, rotação/revogação, Google authorization-code+PKCE/state/nonce e **RS256** oficial (não confundir Ed25519 de entitlements). Identity estável issuer/sub, sem auto-link por email; vínculo exige provas independentes/recentes, sem mesclar outra conta/duplicar backups. Conta/tenant/roles/subscription nunca vêm como autoridade do cliente. Google/password não emitem grant premium; básico Orca/base continua livre/offline, premium só entitlement/licença legítimos separados. Nenhum hardware/Slicer retomado.

**148 testes PASS / 0 FAIL**, incluindo111 baseline e37 novos. HTTP real loopback com RSA/Argon2 reais efêmeros, email/JWKS em memória de TESTE; não é login real Google, TLS/browser, email operacional, pentest, MFA ou produção. Config auth/trust AUSENTE, DB operacional com zero contas/credenciais/sessões/fluxos; APIs privadas503. GET `/api/v1/auth/options` público200 só explica disponibilidade; nenhum token real/usuário admin/permissão inventados. Migração005 aditiva já aplicada UMA vez após backup somenteDBnovo, integridade/FK ok. Venv/dependências13 fixadas+hashes, pip check PASS. Sem operações no cofre/legado/nginx/OS/tráfego neste workstream.

Contratos canônicos: `contracts/auth-bff.v1.json`, `auth-config.v1.schema.json`, OpenAPI/índice. Front integra JSON/viewmodels; esta fase não fez UI/CSS/home/client_portal. Requisitos/rotas/config/limites e evidência reproduzível: [FASE04](WEB-FASE-04-EVIDENCIAS-20261108.md). Escolha literal estruturada: [registro](approvals/WEB-DUAL-LOGIN-ESCOLHA-20261108.json).

**Cronologia visual separada:** crítica t34 às15:57 exigiu revisão; t35 às16:19 Europe/Rome, «aprove e publique», aprovou candidata `/workspace/filuna-web-redesign-C-20261108/` e publicação pelo executor DESIGN/deploy separado. ZIP SHA256 `874b50d79ba3b1c301f77e19455e452cb03fe808870999fd1985e47307a78839`. Este aceite visual não ativa auth/pagamento nem prova deploy; executor backend não altera frontend/processos após esta coordenação. Publicação terá evidência própria, não é afirmada neste checkpoint.

Ativação futura só com origin HTTPS/BFF/TLS exatos; cliente Google WEB legítimo client_id/client_secret/callback registrada+consent; SMTP legítimo host465 TLS/sender verificado/username/credential; config privada canônica via Vault/Form/Handoff seguro pelo pai, nunca segredos em chat. Sem esses insumos a implementação permanece fechada, não paralisada nem simulada. SMTP fila memória limitada não é outbox durável. DB SQLite/devHTTP/snapshot mesmo host não são prontidão de produção/offsiteDR. Parâmetros de sessão/anti-abuso são segurança, não política comercial offline. Nenhum gate desktop/hardware/comercial promovido.


<a id="web-publicacao-c-t35-20261108"></a>
## Publicação visual web C — t35 (08/11/2026 Europe/Rome)

**Ordem literal do proprietário, t35u,16:19 Europe/Rome:** «aprove e publique». [Aceite estruturado](approvals/WEB-VISUAL-PUBLICACAO-T35-20261108.json). A candidata refeita após t34, ZIP SHA256 `874b50d79ba3b1c301f77e19455e452cb03fe808870999fd1985e47307a78839`, foi aprovada e **publicada realmente16:23:45 Europe/Rome** em https://filuna3d.com/, `/conta/`, `/conta/acesso/` e cinco áreas canônicas. [Evidências de publicação, hashes, limites e rollback](WEB-PUBLICACAO-C-T35-EVIDENCIAS-20261108.md). Registros anteriores de não publicação/visual pendente são históricos; a fase04 de autenticação permanece íntegra.

Publicação estática allowlisted via Nginx/TLS existente na mesma raiz nova, releaseversionada/symlink atômico. CSS/JS/Inter/logo/geometria mantêm bytes aprovados; somente links/paths canônicos, CSP de publicação e nota factual de aprovação adaptados. Home e Central agora usam a composição revista, sem reinterpretar o modelo aprovado nem alterar âncorasSlicer. Backend/app/client_portal/auth/storage/admin/migrações não alterados/reiniciados; contratos/índice recebem apenas registro aditivo da publicação. Nenhum segundo backend/CMS ou cofre/legado.

**Evidência:** 45 verificações públicasPASS com HTTPS/certificado normal,16 arquivos publicados por hash e145 referênciasallowlisted;2 consultasHTTP do box500Pingora, enquanto origem301HTTPS, limitação registrada sem bypass/DNS/proxy alterados. Admin anônimo401; arquivos internos/traversal/APIsprivadas404 no domínio. Backend148testesPASS reexecutados após deploy; design115estáticos+11doublesDOMPASS. Nginx-t/reloadPASS; snapshotnovo/config anterior e rollbackpreparados. No checkpoint inicial, Browser/CU público e execução real de rollback estavam NOT RUN neste executor; desktop público foi conferido posteriormente pelo coordenador, no adendo abaixo. Rollback executado e mobile público continuam NOT RUN. Não chamar47/47PASS ou auditoriaAA.

**Publicar visual NÃO ativa serviços:** e-mail/senha disabled, Google explicativo, nenhum cadastro/checkout/backup/download/premium simulado. Auth/trust operacionais continuam ausentes; privado loopback503, API pública404 por não publicação. Básico/baseOrca livres/offline na MESMAengine, sem carregar catálogoFiluna homologado sem direito válido; dados próprios preservados. G2/G3BLOCKED,G4NOTRUN,G5/G6BLOCKED e FLN-VISUAL-C-100 desktop/hardware não promovidos. Próximo passo do coordenador: inspeçãoCU no domínio e integração segura de serviços somente com configuração legítima/escopo específico.


### QA público desktop posterior — 08/11/2026 16:30 Europe/Rome

O coordenador executou Chrome público real, desktop1280×800/zoom100%, sem emulação e HTTPS normal sem aviso de certificado. Abriu a home, clicou **Central do cliente** para `/conta/` e **Acessar conta** para `/conta/acesso/`. Logo e estudo3D renderizaram; não houve fallback de fonte evidente nem texto/botões sobrepostos nas áreas vistas. Acesso mostrou conexão não configurada e campos desativados. Google não foi clicado, nenhuma credencial submetida. Executor inspecionou as três capturas brutas, sem segredos aparentes.

**Estado atual:** desktop público conferido no escopo acima; mobile público, comparação pixel/diff, auditoriaAA e homologação funcional/global **NOT RUN**. Captura1280×800 não fixa a equivalência CSS/device/DPI. Os45 checksHTTP/HTTPS anteriores continuam45; os2HTTPsimples500Pingora permanecem registrados, sem contarCU como mediçãoHTTP adicional. Não ativaGoogle/SMTP/login/pagamento nem altera gates/aplicativo/hardware.

[Capturas brutas e manifesto SHA256](evidencias/web-t35-public-20261108/MANIFEST-SHA256.json): home, Central e acesso, sem resize/crop/máscara. SHA256 do manifesto `f9680c1019474669fd6ae57c2c4b6f97e5c44f249d0d6fdc119cfdcf064ba790`. NOTRUN anterior do browser é histórico do checkpoint do executor, superado SOMENTE por esta inspeção desktop delimitada.


## Fase05 privada — releases/integridade/retenção: contrato proposto, implementação bloqueada
08/11/2026 Europe/Rome. t36/t37 resumidos pelo pai exigem integração nativa desde primeira versão, resumo/reabertura seguros e histórico completo; download inicial só beta/admin, máximo CINCO VERSÕES de instaladores, novo build só necessário. t38 literal: «a assinatira da priimeira versao sera feita pelo github conforme regras». Não equivale a dispensar ECDSA manifesto, SHA256, assinatura real de código/identidadeFiluna, consentimento/salvamento ou QAWindows; attestation GitHub é proveniência separada. [Especificação pré-código e testes NOT RUN](WEB-FASE-05-RELEASE-CONTRATO-20261108.md).

**Checkpoint honesto:** somente especificação/plano local e patch documental; código/rotas/migração/serviço privado novo NÃO implementados nesta fatia. Canal SSH existente deixou de responder a comandos, embora check local do master tenha sucesso; nenhuma mutação remota, acesso alternativo, build/CI/assinatura/publicação/pruning/remoção. Fase04 148 PASS é histórica, não novo baseline executado. Estado atual DB/zero dados NÃO reverificado. Config/trust não devem ser inventados, frontend aprovado/t35/APIs públicas/nginx/cofre/legado/desktop preservados por não atuação. Retomar leitura de NEW-SITE/mestres atuais pelo mesmo acesso existente antes de código.


## Continuidade executável — Fase05.1 privada (08/11/2026 Europe/Rome)
T36–t38 literais continuam no [registro autorizado](approvals/ATUALIZACAO-NATIVA-BETA-T36-T38-20261108.json); backend agora implementou primeira subfatia REAL, não outra proposta: ledger imutável de candidatos/história, ECDSAcapacidade de verificação, streaming privado por64KiB, autorização tenant/conta/beta/adminMFA/licença e plano retenção5VERSÕES global/pins/leases/snapshotconflito. Nenhum executor de pruning/publicação, nenhum artefato real removido/importado/assinado. ECDSAcurva/envelope/raiz NÃO escolhidos operacionalmente; testes P256/SHA256/DER são só capacidade. Adaptadores GitHubprovenance/Windowscode/ownerQA apenas fixtures em memória, não provas reais.

173PASS de unidade/integradosHTTP/cripto não equivalem aos22 requisitos globais aceitos: matriz19PASS estritamente escopados/3NOTRUN com subcasos incompletos explícitos. NativeWindows/confirm/save/helper/reopen/rollback, cadeiaGitHub/Windows e recovery/promotion/pruning reais continuamNOTRUN. Auth/trust config AUSENTES; DBoperacional intacto1–5/integridadeFK/zero contas e catálogo, tabela006 não existe nele. Código novo NÃO carregado no PID2089150, nenhum restart/deploy; domínio/Nginx/publict35 preservados. Antes de aplicar006 futuramente exigir backupDBnovo/revisão/coordenação. Não executar --migrate-only cegamente.

[Arquivos/hashes, testes, contratos, matriz e próximas lacunas](WEB-FASE-05-IMPLEMENTACAO-EVIDENCIAS-20261108.md). Nada de cofre/legado/OS/SSHconfig/CI/build/pagamentos/GoogleSMTP/hardware. Operação atual permanece fechada e catálogo real vazio, básicoOrca/offline e arquivos próprios preservados.
