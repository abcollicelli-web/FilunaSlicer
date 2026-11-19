# FILUNA — Correções nativas candidatas dos 3 OEMs e próxima etapa mestre
**Data:** 2026-11-07 · **Branch:** `filuna/oem-portable-first-qa-20261106` · **PR:** #6 (DRAFT)

## Governança e escopo
Leitura prévia: `FILUNA/REGISTRO-CONTINUIDADE-ETAPAS.md`,
`FILUNA/REGRA-MESTRE-QUALIDADE.md`,
`FILUNA/PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md` §§1.1, 4–5, 13,
`FILUNA/qa/ESTADO-SNAPMAKER-ANYCUBIC-FLASHFORGE-20261107.md`,
e `FILUNA/qa/FLASHFORGE-AD5X-ENGINE-MOTION-CAUSE-20261107.md`.

O proprietário pediu corrigir **Snapmaker U1, Anycubic e Flashforge** antes de passar à etapa seguinte. Mudanças nesta rodada são **apenas na branch de QA**, sem merge, build final, instalador, publicação, acesso a impressoras ou alteração do servidor. **Bambuddy autenticado continua adiado**. O único motor futuro é Orca nativo, com deltas OEM auditados; nunca empacotar fatiadores inteiros.

## Alterações efetivamente aplicadas
| Requisito | Fabricante | Código | Modificação | Prova atual / limite |
|---|---|---|---|---|
| OEM-FLN-FF-AUTO | Flashforge | `src/libslic3r/PrintConfig.cpp` | Removida a migração legada `wall_direction=auto -> ccw` incompatível com o modo Auto já adicionado ao enum. Default Orca global permanece CCW. | Contrato de fonte PASS, **sem G-code do Filuna recompilado**. Persistem diferenças geométricas/E/F em 100/100 camadas e defaults de fabricante ausentes. |
| OEM-FLN-ANY-FINITE | Anycubic | `src/libslic3r/Print.cpp` | Bloqueia NaN/Inf para distância de retração quando o corte longo está ativo. Preserva sentinela `0` do OEM quando desativado, com intervalo 10–18 mm quando ativo. | Fonte PASS; não executado no motor recompilado nem comprovada paridade com Kobra. |
| OEM-FLN-SNAP-PURGE-GATE | Snapmaker | `src/libslic3r/GCode.cpp` | **Bloqueio conservador** da purga final do perfil com macro `chamber_cooling_mode` quando não existe troca de ferramenta (`new_extruder_id=-1`): evita gerar G-code de caminho com Z-hop ainda não validado. Não reconhece modelo por nome fixo. | Guard de fonte PASS; **não é correção de trajetória nem homologação de segurança**. Risco upstream #1149 e variação funcional 2.4.0/2.4.1 continuam abertos. |
| OEM-FLN-SOURCE-REG | Todos | `lab/test_oem_native_source_contract.py` | Atualizados checks Anycubic; novas regressões contra perda de Auto, liberação indevida da purga final e falsa homologação Snapmaker. | **15/15 PASS**, GitHub Actions [#42342020907](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42342020907). Inicial [#42341898566](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42341898566) FAIL por menção textual ao nome de modelo no comentário, corrigida em `f02e1e43`. |

**Commits:** Flashforge `8a83ec36`; Anycubic `2c56bee6`; Snapmaker `8f1a5247` e ajuste `f02e1e43`; testes `08b7ab39`.

## O que NÃO está corrigido/homologado
- **Snapmaker:** ainda falta provar repetibilidade do OEM sob mesmo input (divergência real em 40 runs), causa de variação, correção dinâmica segura de purga e Z-hop, comparar Filuna nativo e obter provas de firmware. O guard atual **impede a exportação insegura** de um caso, não gera um G-code correto em seu lugar.
- **Anycubic:** ainda falta um executável Filuna que aceite o perfil OEM real, carregue o catálogo por modelo sem cópia User temporária e produza G-code comparável ao original, além da revisão de código/perfis da versão fonte versus binário.
- **Flashforge:** auto agora é preservado quando explicitamente presente no perfil; ainda falta refletir defaults OEM auditados (sem alterar perfil oficial), portar só diferenças de planejamento comprovadamente causais e demonstrar paridade de movimento, extrusão, ventilação e macros em AD5X/AD5M.
- Para os três: **`FILUNA_NATIVE_PARITY=FAIL_NOT_READY`, `G4-I=BLOCKED`, `DEVICE_NETWORK=NOT_RUN`, `PHYSICAL_BETA=NOT_RUN`, `INTERNAL_HOMOLOGATION=0/3`**. PASS de testes Python é apenas QA de fonte. Não autoriza merge/release/instalador nem impressão física.
- O trabalho restante exige C++ nativo executado e comparado, revisão de licenças/ABI e provas DeviceBridge. Por ordem do proprietário, **nenhum build intermediário OEM** para mascarar falhas; antes do build final, manter gates honestos. O gate de paridade só poderá passar com executável de QA efetivamente validado.

## Etapa seguinte conforme Plano Mestre
**Dentro da homologação OEM:** resolver todas as divergências funcionais e safety gates; integração real do dispositivo e perfis sob demanda; testes internos **G4-I** (todos os critérios PASS), depois **um build final único** e regressão do binário, pacote/assinatura/rollback (**G5**), beta vigiado autorizado (**G6-B**), três provas físicas independentes por modelo/coorte (**G4-F**) e liberação estável somente com decisão expressa (**G6-S**).

**Cronograma global de criação:** a próxima onda ainda pendente é **01 — Fundação** (CI, baseline de G-code, isolamentos, logs, instalador privado), seguida de **02 — Qualidade nativa Orca**, **03 — Identidade/Home**, **04 — Preparar/Visualização/Calibração**, **05 — DeviceBridge**, **06–09 — pilotos OEM**, **10 — Catálogo/Paramétricos**, **11 — Ateliê/Estoque/Produção**, **12 — Serviços/IA**, **13 — Distribuição** e **14 — Operação permanente**. A correção pontual de três OEMs não conclui nem substitui essas ondas.
