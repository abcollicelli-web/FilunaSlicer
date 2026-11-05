# FILUNA G0 — Rastreabilidade de requisitos e conflitos

**ID** FLN-G0-CF-2026-11-05.2  
**Atualizado** 2026-11-05 23:12:40 CET (Europe/Rome)  
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
| N1 | Fontes mestras lidas (obrigatórias esgotadas) | **PASS** | Conjunto normativo = MIGRACAO §2 + PLANO §3 + REGRA + dossiê F — esgotado semanticamente neste lote. Evidência: [G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md](G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md). **Não** exige 141 cláusula-a-cláusula. |
| N2 | Conflitos identificados e sem órfãos obrigatórios | **PASS** (processo) | CF-01..11 rastreados; CF-07 BLOQUEANTEs W enumerados abaixo com política **não herdar** → não são órfãos obrigatórios G0 F. Pergunta única ao dono permanece opcional para reforço. |
| N3 | Baseline upstream/Filuna | **PASS** | pin `a620d804…`; base `2dace0db…`; UPSTREAM_PIN.md |
| N4 | Riscos documentados | **PASS** parcial | MIGRACAO + inventário + riscos OEM/DLL/admin-controle |
| N5 | Mapa versionado path+hash+status | **PASS** parcial | MANIFESTO 141 + inventário; status de *profundidade de leitura* corrigido neste lote |
| N6 | Matriz migração / fonte por feature | **PASS** parcial | MIG-01..26 existem; alinhamento fino a nodes DAG ainda iterativo |
| N7 | SBOM inicial | **PASS** parcial | [sbom/README.md](sbom/README.md) PARTIAL verificável — **não** completo |
| N8 | Testes G0 de fundação documentados | **PASS** estático | check_quality_gate foundation; check_master_plan 113/113 ≠ G0 funcional |

**Conclusão:** N1/N2 **PASS** neste lote. G0 global permanece **PENDING** enquanto N4–N7 forem só parciais e não houver QA Windows (N8 estático ≠ funcional).

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

### Inventário explícito CF-07 — BLOQUEANTEs legado W (`docs/PENDENCIAS.md`, blob lido 2026-11-05)

Fonte: `abcollicelli-web/filuna-slicer` `docs/PENDENCIAS.md` @ `61339ae…` (conteúdo lido integralmente neste lote).

| Item legado (resumo) | Tipo no W | Tratamento no fork F |
|---|---|---|
| Várias peças / booleanas / montagem no fatiador | BLOQUEANTE 🧪 | **Não herdar** como G0; futuro REQ-F sob Orca nativo (MIG-01/04) |
| Pintura de cor multi-filamento 3MF | BLOQUEANTE 🧪 | Não herdar; validar no core Orca 2.5 F |
| Paramétricos árvore/luminária/quadro | BLOQUEANTE 🧪 | Não herdar; Onda 10 |
| Anycubic início automático (só envia) | BLOQUEANTE 🧪 | Não herdar como PASS W; **reabrir** como REQ DeviceBridge piloto Kobra (Onda 06) com evidência F |
| Planos R$40 / emergência 5d / offline 3d | BLOQUEANTE 💡 | Não herdar preço; Orca-livre é invariante; valores = PENDENTE_DONO comercial (FORA_G0) |
| Estoque/E-commerce site celular | BLOQUEANTE 🧪 | Não herdar; Ondas 11–12 / SRV |
| Teste Windows instalador 0.2.3 + approve | BLOQUEANTE 👤 | Não herdar binário W; G4 F usa build `42102469888` + hardware dono |
| Revisar 65 artigos KB assistente | BLOQUEANTE 👤 | Não herdar; FORA_G0 |
| Ligar faturas site ↔ Slicer | BLOQUEANTE 💡 | Não herdar; SRV/ID |
| Testar Claude/Gemini/Grok MCP | BLOQUEANTE 👤 | Não herdar; Onda 12 |
| Mercado Livre etiqueta real | BLOQUEANTE 👤 | Não herdar; Onda 11/12 |
| Meta WhatsApp/IG/Messenger | BLOQUEANTE 👤 | Não herdar |
| Convite Meta `filuna_convite_beta_v5` | BLOQUEANTE 👤 | Não herdar |
| Homologação-site checklist BLOQUEANTE | (ref. HOMOLOGACAO-SITE) | Não herdar publicação W como G0 F |

**Efeito N2:** nenhum desses itens é “órfão obrigatório” do G0 F sob a política documentada. Se o dono quiser promover algum a REQ-F obrigatório **antes** de G0 PASS, deve dizê-lo explicitamente (pergunta única abaixo).


## REQ adicionais extraídos neste lote (amostra B — semânticos)

| REQ | Fonte (seção) | Texto condensado | Precedência F | Status |
|---|---|---|---|---|
| REQ-G0-UPD-03 | S/25 §Classificação | Classes A/B dados; C UI/DLL Filuna; D motor/runtime só com consentimento/UAC/Windows real | Mantém (updater F futuro C++) | RESOLVIDO política |
| REQ-G0-UPD-04 | S/25 + W/D5-D6 | Pacote remoto não pode trazer `.exe`/scripts/Orca; targeted OFF por default | Mantém | RESOLVIDO política |
| REQ-G0-PEI-03 | S/23 + W/QA-Lua + ENGINE | Adaptador/CLI deve usar `--arrange 0` e inspecionar nº de placas antes de sucesso | Mantém no F (GUI Orca nativa: validar equivalente) | DOCUMENTADO |
| REQ-G0-DEV-01 | W/PROTOCOLOS §1 | Filuna **lê** (status/filamento/câmera); Orca **imprime** (host_type); elo = mapeamento filamento→perfil | Alinha DeviceBridge F | DOCUMENTADO / FORA impl G0 |
| REQ-G0-DEV-02 | W/QA-0.1.9 + FUP | Discovery só IPv4 privado; “Identificada” ≠ cadastrada ≠ impressão liberada | Mantém | DOCUMENTADO |
| REQ-G0-DEV-03 | W/QA-0.1.9 | Envio G-code **sem** auto-start; pause/resume/cancel com confirmação | CF-04 | RESOLVIDO |
| REQ-G0-PROD-01 | W/PRODUCAO | Fila/agenda só avisam; `printSend` só pela UI com confirmação | CF-04 | RESOLVIDO |
| REQ-G0-CAT-03 | S/REGRA-CATALOGO | Catálogo Filuna: sem Baixar/Export STL/3MF; externos/paramétricos próprios mantêm export | Mantém | RESOLVIDO |
| REQ-G0-SEC-03 | W/AUDITORIA | Diagnostics sanitizados (sem caminhos/G-code/credenciais); retenção limitada | Mantém SRV | DOCUMENTADO / FORA impl |
| REQ-G0-BIZ-02 | W/GUIA-DONO | Google/Stripe reuso site; Bambu SDK = ação do dono; LAN até lá | Não provisionar G0 | DOCUMENTADO |
| REQ-G0-ENG-04 | contracts/engine-job | Schema legado enum 4 engines | **SUPERADO** — F só `orca` | SUPERADO |
| REQ-G0-HOME-01 | S/18 + ESTRUTURA | Home Grok 03/11: faixas Recentes/Modelos/Ferramentas; dados reais; Imprimir não envia fictício | G1+ | FORA_G0 impl |

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

1. N1/N2 fechados neste lote (definição normativa). Opcional: aprofundar COVER **não obrigatórios** sob demanda.  
2. Apertar N4–N7: riscos OEM remanescentes; alinhar MIG↔DAG; enriquecer SBOM com DLLs do artifact `42102469888` (read-only) **sem** novo build.  
3. Não iniciar Home/DeviceBridge/SRV/G1 UI; sem merge PRs.  
4. G0 PASS global só quando N3–N8 estiverem comprovados sem “parcial” bloqueante — N8 funcional Windows continua NOT RUN (dono: testar depois).
