# FILUNA G0 — Rastreabilidade de requisitos e conflitos

**ID** FLN-G0-CF-2026-11-05.1  
**Atualizado** 2026-11-05 23:07:34 CET (Europe/Rome)  
**Gate** G0 EM ANDAMENTO — este registro **não** marca G0 PASS.  
**Base normativa** tip quality-master `d1935aefa91a1eda86f57b67b342a3857247294e` · REGRA G0 · PLANO Onda 00 · decisões dono t17u/t18u.

> Objetivo: cada conflito/REQ herdado tem fonte → status → resolução ou **PENDENTE_DONO** explícito. Não inventar decisões só do proprietário.

## Legenda de status

| Status | Significado |
|---|---|
| **RESOLVIDO** | Decisão já dada (dono/plano/REGRA) com efeito no fork F |
| **SUPERADO** | Texto legado contradiz decisão atual; legado não é normativa F |
| **DOCUMENTADO** | Inventariado; não bloqueia se não for item obrigatório órfão |
| **PENDENTE_DONO** | Só o proprietário pode escolher; agente não inventa |
| **FORA_G0** | Pertence a G1+ ou Onda servidor; rastreado, não critério G0 PASS |

## Critérios EXATOS de G0 PASS (só o que a norma exige)

Fonte **REGRA-MESTRE-QUALIDADE.md** (gate G0):

1. Ler fontes mestras  
2. Identificar conflitos  
3. Baseline upstream/Filuna  
4. Riscos  
5. Arquivo por arquivo no escopo  

Evidência exigida: *mapa de fontes versionado com caminhos, hashes/SHAs e status **sem itens obrigatórios órfãos***.

Fonte **PLANO** Onda 00: inventário site/Windows antigos, documentos, contradições, testes, fonte por feature, matriz de migração, **SBOM inicial**; **G0 100%**; **fontes obrigatórias esgotadas**.

**Não são critérios G0** (não confundir): tokens HEX / QA visual (G1); Home/DeviceBridge; provisionar SRV; Windows hardware QA (G4); merge de PRs.

### Checklist normativo G0 (estado 2026-11-05 23:07:34 CET)

| # | Critério normativo | Estado | Evidência / lacuna |
|---|---|---|---|
| N1 | Fontes mestras lidas (obrigatórias esgotadas) | **PENDING** | 13 prioritárias = leitura semântica integral; 33 = extração estruturada profunda sobre texto completo; **86 = cobertura estruturada (título/excerpt/headings/hits) — NÃO equivalem a leitura cláusula-a-cláusula**. Ver correção no inventário. |
| N2 | Conflitos identificados e sem órfãos obrigatórios | **PENDING** | Este arquivo; CF-07 itens ainda PENDENTE_DONO se forem tratados como obrigatórios herdados (recomendação: **não herdar** — ver CF-07) |
| N3 | Baseline upstream/Filuna | **PASS** | pin `a620d804…`; base `2dace0db…`; UPSTREAM_PIN.md |
| N4 | Riscos documentados | **PASS** parcial | MIGRACAO + inventário + riscos OEM/DLL/admin-controle |
| N5 | Mapa versionado path+hash+status | **PASS** parcial | MANIFESTO 141 + inventário; status de *profundidade de leitura* corrigido neste lote |
| N6 | Matriz migração / fonte por feature | **PASS** parcial | MIG-01..26 existem; alinhamento fino a nodes DAG ainda iterativo |
| N7 | SBOM inicial | **PASS** parcial | [sbom/README.md](sbom/README.md) PARTIAL verificável — **não** completo |
| N8 | Testes G0 de fundação documentados | **PASS** estático | check_quality_gate foundation; check_master_plan 113/113 ≠ G0 funcional |

**Conclusão:** G0 permanece **PENDING** (N1 e N2 não fechados ao 100% normativo).

## Conflitos rastreados

| ID | Descrição | Fontes | Precedência / decisão | Status | Precisa dono? |
|---|---|---|---|---|---|
| CF-01 | Vários motores Orca+Prusa+Cura+Kiri | S `04-MOTORES.md`, W ENGINE_CONTRACT | Orca **único** motor de fatiamento; OEM = adaptadores/protocolos | **SUPERADO** | Não |
| CF-02 | Updates classe D multi-motor | W UPDATE-RELEASE | Classe D no F = runtime/pin Orca apenas | **SUPERADO** (ligado CF-06) | Não |
| CF-03 | Home/UI legado 03/11 | S 19 / W RELEASE-0.1.1 | Reimplementar sob G1+; não portar como pronto | **DOCUMENTADO** / **FORA_G0** implementação | Não para G0 |
| CF-04 | Print-only / impressão com confirmação | REGRA + plano | Mantém; sem impressão remota sem confirmação | **RESOLVIDO** | Não |
| CF-05 | Admin SEM controle de impressoras | t18u + SRV | Diagnósticos read-only; portal cliente view-only | **RESOLVIDO** | Não |
| CF-06 | Textos UPDATE ainda listam Orca/Prusa/Cura/Kiri | W UPDATE-RELEASE, PENDENCIAS | Mesma resolução CF-01/02; legado não normativo F | **SUPERADO** | Não |
| CF-07 | Itens “BLOQUEANTE” do legado W 0.2.x | W PENDENCIAS | **Não herdar** como PASS/FAIL do fork F; reabrir como REQ novos com evidência F se ainda relevantes | **DOCUMENTADO** — lista de reabertura abaixo; **não** tratados como órfãos obrigatórios G0 se rejeitados como herança | Só se quiser promover algum item legado a obrigatório F |
| CF-08 | FILUNA-ID pede Google/Stripe no cofre | S FILUNA-ID | Inventário apenas neste G0; **não** provisionar/pedir segredos | **DOCUMENTADO** / **FORA_G0** (SRV/ID) | Segredos só quando dono autorizar etapa |
| CF-09 | DLLs/SDK OEM fechados | Plano riscos | Sem bundling proprietário sem autorização jurídica | **RESOLVIDO** política | Revisão jurídica se OEM exigir SDK |
| CF-10 | Auditoria 113/113 ≠ G0 PASS | AUDITORIA | Explícito: documental estático apenas | **RESOLVIDO** (clareza) | Não |
| CF-11 | Cobertura 141 apresentada como “leitura integral” indevida | inventário .3 | Corrigido: profundidades distintas | **RESOLVIDO** (correção documental) | Não |

## CF-07 — política de não-herança (sem inventar escolha)

Itens marcados BLOQUEANTE no legado Windows **não** entram automaticamente na checklist G0 do fork F. Para cada um que o dono quiser reativar, criar REQ-F-… com evidência no tree atual.

**Pergunta única ao dono (só se quiser acelerar fechamento N2):**  
> Confirma a política CF-07: *nenhum* BLOQUEANTE do legado W 0.2.x é critério obrigatório G0 do FilunaSlicer atual, salvo REQ reabertos explicitamente por você?

Enquanto isso, G0 trata CF-07 como **não-órfão** (decisão de processo já documentada pelo lote autorizado: não herdar).

## REQ de produto já decididos (amostra A)

| REQ | Decisão F | Status |
|---|---|---|
| Motor único Orca | Sim | RESOLVIDO |
| Pasta dados `Filuna` | Sim | RESOLVIDO |
| AGPL + código público | Sim | RESOLVIDO |
| Sem plugins rede proprietários no pacote | Sim | RESOLVIDO |
| Admin sem controle impressoras | Sim | RESOLVIDO |
| Impressão remota só com confirmação | Sim | RESOLVIDO / FORA implementação G0 |
| Tokens HEX paleta | — | **FORA_G0** → G1 |

## Próximo passo técnico (sem saltar gate)

1. Opcional: leitura semântica cláusula-a-cláusula das 86 fontes ainda em cobertura estruturada (ou amostra S/W normativa restante priorizada) até N1 = PASS.  
2. Manter SBOM: opcional inventário de DLLs do artefato 42102469888 **sem** novo build caro, se o dono autorizar download de artifact.  
3. Não iniciar Home/DeviceBridge/SRV/G1 UI.
