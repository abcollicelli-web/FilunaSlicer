# Filuna Slicer — diário contínuo de homologação OEM e G4-I
**Rodada:** 07/11/2026 · **Fonte de verdade:** [PR #6](https://github.com/abcollicelli-web/FilunaSlicer/pull/6), branch `filuna/oem-portable-first-qa-20261106` (DRAFT) · **Estado:** NÃO HOMOLOGADO.

## 1. Regra vinculante e alcance do PASS
- O proprietário pediu documentação integral e continuidade até conclusão. Esta ordem autoriza **investigar e corrigir/testar fontes e CI na branch isolada**, não equivale a autorização para publicar, instalar, comandar impressoras, alterar firmware, efetuar merge ou atestar testes físicos inexistentes.
- Unidade de aprovação = **modelo + bico + firmware + capacidades + perfil OEM e respectivas versões**, não a marca inteira.
- **PASS de testes unitários ou OEM original ≠ PASS de G-code Filuna ≠ PASS de DeviceBridge/rede/ACK ≠ PASS físico ≠ liberação estável**.
- Arquivos G-code OEM permanecem imutáveis; diferenças funcionais G0/G1/G2/G3, E/F, M-codes, purga, toolchange, macros e ordem são divergências reais, jamais normalizáveis para disfarçar FAIL.
- Pacote final: motor Orca único + deltas necessários e licenciados OEM; perfis assinados/carregados por modelo sob demanda. Nada de vários fatiadores embutidos.
- Somente integração autenticada **Bambu + Bambuddy** foi expressamente adiada. QA independente de Bambu prossegue. Físicos só em beta autorizado, três casos auditáveis por modelo/coorte e atualização de estado conforme regra vigente.

## 2. Evidência nova desta rodada (origem e resultado exatos)

| ID | Evidência | Resultado | Interpretação restrita |
|---|---|---|---|
| U1-SAFE-01 | [Actions #42393531370](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42393531370) | **PASS** — compilação C++17 isolada, 8/8 contratos de `PrintHost`, 4 G-codes OEM pinados | Geometria e contrato de código, **não** compilação GUI Windows, impressora ou ACK |
| U1-SAFE-02 | `1a97e2e` + `5f54179e` | **PASS** — WARN geométrico agora impede `PrintHost::upload` sem autorização auditável | Antes um evento GUI assíncrono permitia envio antecipado; agora fail-closed. Arquivo idêntico |
| WIN-CACHE-01 | [Actions #42365045634](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42365045634) | **FAIL de CI** — compilou dependências MSVC, falhou ao calcular `hashFiles('deps/**')` na etapa de cache, limite 120s | **Não** é falha do código GUI. Build GUI não ocorreu |
| WIN-CACHE-02 | commit `709c7e1`, [Actions #42394738274](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42394738274) | **PENDENTE** no checkpoint | Novo cache usa apenas arquivos de receita CMake versionados, cache opcional; compila destino GUI sem instalador. Conferir conclusão |
| U1-CORE-01 | `13690b6` + `d99f693` + `d382e4e`, [Actions #42394922180](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42394922180) | **PASS 2/2** da fixture sintética OPC/3MF: 8 vértices, 12 triângulos, manifold, 1 build item | Teste de formato e topologia, não teste da CLI OEM |
| U1-CORE-02 | mesma execução; artifact `13583545231` `minimal-core-3mf-control.log` | **FAIL da CLI** — `4294967290` (status de rejeição), `Loading of a model file failed`, 0 G-codes | 3MF Core com estrutura sintética testada; **não** é crash `0xC0000005` e não comprova suporte OEM a esse pacote |
| U1-STL-01 | mesmo artifact `13583545231` | **PASS isolado do OEM STL**, saída de 250183 bytes, SHA-256 `86e61b359fa2f25aa6a1296400b6638b274956c344fb9d28a230a940f26ce6ad` | Apenas fatiamento oficial de cubo, não paridade com Filuna, nem prova de repetibilidade; execução anterior SHA distinto reforça investigação de deriva |
| U1-3MF-01 | [#42393665358](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42393665358) e [#42394922180](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42394922180) | **FAIL real** — 3MF distribuído pelo próprio fabricante termina em `0xC0000005` após importação | Incompatibilidade/crash do CLI OEM nas condições testadas, **não** atribuível a 4 ferramentas |
| U1-3MF-02 | [#42393665358](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42393665358) | **FAIL real** — exportar 3MF de STL que fatia termina em `0xC0000005` | Última fase logada `render_thumbnail`/GLFW/OpenGL Mesa; relação causal com driver/thumbnail NÃO comprovada |
| U1-4T-01 | [#42394922180](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42394922180) | **BLOCKED** — 3MF QA com 4 objetos e extrusoras 1–4 foi importado, mas CLI original crash `0xC0000005`, sem G-code | Sem prova 4 cores, purga final ou equivalência |
| U1-SRC-01 | [Snapmaker Orca fonte pinada](https://github.com/Snapmaker/OrcaSlicer/blob/bac6fd5c1e1c23ddf057cf6471ba4ed7ed621c4c/src/Snapmaker_Orca.cpp) | **AUDITADO** | Código `export_to_3mf` chama GLFW/OpenGL para miniaturas; próximo passo é obter crash stack/controle de contexto, não pular para alteração de G-code |

**Artefatos brutos da U1:** [#42394922180](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42394922180) e [#42393665358](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42393665358); contêm logs, hashes, originais preservados e arquivos de QA. A saída do mesmo STL original pode variar semanticamente entre execuções do OEM (já comprovado em 40 rodadas), logo um sucesso isolado **não** substitui stress e paridade.

## 3. Status por fabricante e por gate
| Família / amostra | Fatiador oficial A/B | Código Filuna nativo comparado ao OEM | Rede/registro/ACK real | Homologação interna |
|---|---|---|---|---|
| Anycubic S1, Kobra 3 V2, Kobra 4 | **PASS em amostras oficiais**, 60 execuções adicionais | **NOT_PROVEN**; zero de retração corrigido somente em fonte candidata | **NOT_RUN** | **BLOCKED** |
| Flashforge AD5X, Adventurer 5M | **PASS em amostras oficiais**, 40 execuções adicionais | **FAIL PARITY** no Filuna pré-OEM; 100 camadas com diferenças reais; deltas fonte candidatos | **NOT_RUN** | **BLOCKED** |
| Snapmaker U1 | **FAIL** repetibilidade v2.4.0 e v2.4.1; 3MF crash, risco de purga final | **NOT_PROVEN**; validador C++ isolado PASS | **NOT_RUN** | **BLOCKED** |
| Bambu A1 | Histórico oficial e paridade precisam ser revalidados no motor final | **MISMATCH/NOT_PROVEN** | Autenticação Bambuddy **OWNER_DEFERRED** | **BLOCKED** |
| Elegoo Centauri Carbon 2 | CLI oficial executou; Filuna pré-OEM divergente | **MISMATCH/NOT_PROVEN** | **NOT_RUN** | **BLOCKED** |
| Creality, QIDI, Prusa e demais variantes | Apenas subconjuntos oficiais com provas pontuais | **NOT_PROVEN** | **NOT_RUN** | **BLOCKED** |

Base documental: `FILUNA/qa/ESTADO-SNAPMAKER-ANYCUBIC-FLASHFORGE-20261107.md`, `FILUNA/qa/PORTABLE-FIRST-8-OEM-20261106.md`, `FILUNA/REGISTRO-CONTINUIDADE-ETAPAS.md`, `FILUNA/qa/U1-GATE-NATIVO-CXX-FILA-ENVIO-WINDOWS-QA-20261107.md`.

**Gates globais atuais:** `G4-I=BLOCKED`, `INTERNAL_3_OEM=0/3`, `FILUNA_NATIVE_GCODE_PARITY=NOT_PROVEN`, `WINDOWS_FULL_GUI_BUILD=NOT_PROVEN`, `DEVICE_NETWORK_ACK=NOT_RUN`, `PHYSICAL_BETA=NOT_RUN`, `STABLE_RELEASE=BLOCKED`.

## 4. Próximas provas técnicas — nenhuma promoção por mera execução
1. **Windows:** ler conclusão real de #42394738274. Se falhar, obter primeiro erro do log, corrigir somente causa, reconstruir `libslic3r_gui` em QA sem gerar instalador, e reexecutar testes C++.
2. **U1/3MF:** provar a semântica do arquivo Core, isolar importar versus renderizar miniaturas com o binário oficial e, se necessário, capturar minidump/stack da CLI. Não injetar correção no G-code OEM. Atribuir acidente ao fabricante somente quando fonte e execução comparáveis sustentarem.
3. **Snapmaker U1:** obter 3MF 4 ferramentas realmente aceito pela CLI OEM, comparar A/B estrito repetido, validar purga final contra firmware real e risco geométrico, jamais liberar upload com risco sem aprovação autenticada por SHA/usuário. Envolver DeviceBridge de verdade e testar ACK separado.
4. **Anycubic/Flashforge:** quando houver executável QA Filuna com deltas nativos compilados, gerar G-code real com a mesma entrada e presets OEM pinados, comparar movimento/E/F/macros/camadas e corrigir motor, não preset oficial. Integrar rede e catálogo assinado sob demanda.
5. **Ampliação:** passar demais modelos e perfis pela mesma matriz, sem assumir que 2–3 amostras cobrem catálogo; executar hardware físico somente no beta supervisionado, conforme exigido.
6. **Promoção:** quando cada gate interno aplicável tiver evidência verificável, seguir Plano Mestre G4-I → build final/regressão → G5 → G6-B → G4-F → G6-S com aprovação expressa do proprietário.

**Política operacional nesta rodada:** nenhum merge, release, instalação, alteração de produção/servidor/firmware, comando LAN a impressora, impressão física ou aprovação automática. O caráter DRAFT da PR #6 permanece obrigatório.
