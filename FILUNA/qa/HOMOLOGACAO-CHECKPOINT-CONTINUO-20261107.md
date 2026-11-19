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


## 5. Releitura independente de artefato e andamento do Windows — 07/11/2026 17:16 UTC

**WIN-CACHE-02** — [#42394738274](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42394738274): `in_progress`, job `128469957670` ainda na etapa **Build pinned Orca Windows deps on this QA branch when the cache is unavailable**. Nenhum erro novo de compilação e **nenhum PASS** da GUI até o fechamento do job. Não reexecutar nem disparar builds concorrentes enquanto esse run estiver ativo.

**U1-CORE-02 / U1-3MF-01 / U1-3MF-02** — artefato bruto do run [#42394922180](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42394922180), ID `13583545231`, reaberto e comparado diretamente, sem depender apenas do status geral:
- `fourtool-proof.json`: `minimal_core_3mf_slice_control.exit_code=4294967290`, `gcodes=[]`. O log termina em `_BBS_3MF_Importer::_load_model_from_file:1687 IMPORT_STAGE_READ_FILES`, depois `Loading of a model file failed`. Isso é **rejeição durante importação**, não violação de acesso. O teste de integridade da fixture verifica ZIP/OPC, fechamento geométrico e malha, **não garante** que o importer BBS aceite todos os metadados/extensões. Causa detalhada ainda **NOT_PROVEN**.
- `official-unmodified-3mf-control.exit_code=3221225477` = `0xC0000005`. O log do 3MF oficial chega a `IMPORT_STAGE_FINISH`, `plate count 0`; crash ocorre **depois da importação**, diferentemente da fixture Core. O arquivo OEM original permaneceu intacto (SHA-256 `302cb4d3b1583aa06bb56cad94265b3faf6d16dae82f9f2a882a39b27aafe8b0`).
- `oem_exported_3mf_control.exit_code=3221225477` = `0xC0000005`. A última etapa logada é `render_thumbnail` com Mesa OpenGL, após `glfwInit Success` e `glewInit Sucess`. **Correlação temporal**, não prova de culpa do driver. A saída 3MF não foi criada.
- `raw_stl_control.exit_code=0`: um G-code original produzido com `250183` bytes, SHA-256 `86e61b359fa2f25aa6a1296400b6638b274956c344fb9d28a230a940f26ce6ad`; o resultado **não prova repetibilidade** e não permite promover Snapmaker.
- `four-tool-1.exit_code=3221225477`: quatro extrusoras **não testadas com G-code**. Crash do 3MF de referência sem alterações demonstra que a presença de quatro ferramentas não é causa necessária da falha, mas não identifica a causa suficiente.

**Conclusão diagnóstica:** há pelo menos **dois modos de falha distintos** no caminho 3MF da CLI oficial Snapmaker 2.4.1 no runner Windows: rejeição de importação Core e access violation em projetos BBS/exportação. Nenhum deles autoriza editar G-code OEM, omitir testes negativos, concluir bug de firmware ou considerar o binário Filuna homologado. Próximo ensaio seguro: controle de importação Core com metadados mínimos do padrão BBS, crash stack ou dump em ambiente QA, e comparação da execução sem aceleração de renderização; qualquer intervenção deve permanecer somente no runner/fixture e documentar hashes.

## 5. Novo avanço reproduzível — importação 3MF padrão corrigida, fatiamento OEM funciona

**[Actions #42395512419](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42395512419), artifact `13582648165`:**

- A primeira fixture Core 3MF foi rejeitada com `4294967290` no [run #42394922180](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42394922180). Investigação comparou `_rels/.rels` e `[Content_Types].xml` com a embalagem oficial: o gerador de testes usava prefixos `ns0:Relationships`/`ns0:Relationship` em vez de serializar o namespace OPC como padrão.
- **Causa demonstrada na fonte aberta Snapmaker** `src/libslic3r/Format/bbs_3mf.cpp`, pin `bac6fd5c1e1c23ddf057cf6471ba4ed7ed621c4c`, `RELATIONSHIP_TAG="Relationship"` (linha aproximada 259), `strcmp(RELATIONSHIP_TAG,name)` (~4641). O SAX Expat retorna literalmente `ns0:Relationship` quando há prefixo, logo o relacionamento principal não é associado. Isto descreve a compatibilidade do **importador OEM com a fixture**, não certificação de todo formato 3MF.
- Correção em `lab/snapmaker_u1_fourtool_original_qa.py` commit `ac87dcb` e regressão de raiz OPC em `lab/test_snapmaker_minimal_core_3mf.py` commit `c0871d5`: gera `<Relationships xmlns="...">` e `<Types xmlns="...">` e preserva elementos de 3MF Core com namespace padrão. **Testes estruturais 2/2 PASS.**
- **Novo resultado funcional positivo e real do OEM sem alteração de executável nem preset:** o mesmo Snapmaker Orca Windows **2.4.1 oficial** executou a fixture Core 3MF de cubo e retornou `0`, gerando **249381 bytes** de G-code, SHA-256 `01568d58335e3767575e530814a7de220f16ce96f1444cfdad785c9fef6c80b4`. Status **`MINIMAL_3MF_OEM_SLICE=PASS`**. A fase de importação Core fica diferenciada da falha de projeto OEM.
- O **3MF intocado `OrcaCube_v2.3mf` do fabricante** continua causando `0xC0000005`. A cópia BBS quatro ferramentas também falha com `0xC0000005`; `--export-3mf` do OEM continua falhando no caminho de miniaturas. Portanto o job completo **FAIL legítimo**, `U1_ORIGINAL_BBS_3MF=FAIL`, `U1_FOUR_TOOLS=BLOCKED`.
- Novo passo de isolamento, fonte e fixture QA apenas: `bed46c6`/`eecbd29`/`a3d3c9a` criam **quatro malhas Core fechadas distintas** a partir do arquivo simples que efetivamente fatia, com `model_settings.config` atribuindo extrusoras 1,2,3,4; validam topologia e tokens no G-code oficial. [Run #42395857307](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42395857307) **aguardando resultado** no momento deste checkpoint. Se vier G-code, ainda falta comparar movimentos/purga/fonte/firmware e repetir; sucesso deste piloto não homologa o modelo.

**Sem alteração de G-code original, software OEM, firmware, servidor, aplicação instalada ou impressora.** `G4-I`, `3_OEM`, `SEND_ACK`, `PHYSICAL_BETA` seguem bloqueados.

## 6. Quatro ferramentas OEM em 3MF sintético — A/B genuíno e bloqueio de colisão confirmado

**Amostra real do motor oficial Snapmaker Orca Windows 2.4.1** com ZIP verificado e preset process efetivo copiado exclusivamente em QA com o campo `nozzle_diameter` herdado da máquina original; nenhum G-code, fonte ou perfil OEM foi alterado. A entrada é o arquivo sintético `qa-core-four-distinct-tools.3mf` (quatro objetos + `Metadata/model_settings.config`, extrusoras distintas), **não** um projeto oficial gerado pela UI nem prova física.

### Funcionamento confirmado, só no escopo controlado
- [CI #42395857307](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42395857307), artifact `13583129173`: G-code real com **2638447 bytes**, `T0/T1/T2/T3` presentes e **301 blocos `CP TOOLCHANGE START`**, 100 camadas. A fixture passa em 3 testes sintéticos de estrutura; o binário OEM efetivamente gera movimentos. `FOUR_TOOL_SYNTHETIC_OEM_CLI=PASS`.
- [CI #42396105609](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42396105609), artifact `13583204316`: dois processos OEM de fatiamento independentes com mesmo 3MF/perfis/executável, horários **17:25:05 e 17:25:09 UTC**, **2638447 bytes** em cada, hashes brutos diferentes apenas pela linha única `generated by Snapmaker Orca 2.4.1` dentro de `HEADER_BLOCK`. Após remover **somente essa linha** de ambas as saídas, SHA-256 idêntico **`1fffba21d2cce0e0456f69836e3e8b5f22aaa35e955ddcb8305627457aca12d1`**. Todas as outras **107257 linhas** preservadas. `FOUR_TOOL_SYNTHETIC_OEM_TEMPORAL_AB=PASS_SINGLE_PAIR`; ainda não é estresse prolongado nem paridade Filuna.

### Bloqueio real e reproduzível: purga final pode atravessar peça impressa
- O mesmo G-code OEM foi avaliado em Python `lab/snapmaker_spatial_gcode_validator.py`: `BLOCK`, **linha 106623, camada 99**, travessia sem clearance no ponto **X132.044 Y141.691 Z20.05 mm** sobre geometria com topo Z20.05 mm, depositada anteriormente na linha 104537. Contexto `FINAL_PURGE_OR_END`. O G-code OEM permanece intocado.
- A nova [CI de dupla prova nativa #42396347068](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42396347068) concluiu **SUCCESS** de teste: faz SHA pin do G-code oficial, exige BLOCK do validador Python, compila `lab/test_snapmaker_native_gcode_safety.cpp` em C++17 com `-Wall -Wextra -Wpedantic -Werror`, executa o **mesmo arquivo** no scanner nativo e exige também `NATIVE_STATUS=BLOCK` com **a mesma linha 106623, camada 99, XYZ 132.044/141.691/20.05**. C++: `ERRORS=1`, `WARNINGS=0`; ver [workflow](../../.github/workflows/filuna-snapmaker-four-core-block-proof.yml). **SUCCESS significa DETECÇÃO CORRETA DO BLOQUEIO, nunca G-code seguro.**
- `U1_FOUR_TOOL_SAFETY=BLOCKED`, `U1_FIRMWARE_MACRO_LIVE=NOT_RUN`, `U1_PRINT_HOST_ACK=NOT_RUN`, `FILUNA_NATIVE_GENERATOR_PARITY=NOT_RUN`. O risco está no **arquivo original gerado pelo OEM** com este projeto sintético e este motor/preset, não foi criado por pós-processamento Filuna.
- O 3MF `resources/handy_models/OrcaCube_v2.3mf` do próprio pacote continua com `0xC0000005` no fatiamento via CLI nas condições estudadas, assim como o antigo 3MF QA clonado. Isso permanece FAIL separado, mesmo com a amostra Core compatível. **Não reclassificar o job inteiro em PASS para ocultar a falha.**

**Decisão operacional:** o scanner integrado ao caminho clássico `PrintHost` precisa continuar bloqueando esta trajetória (e o fluxo DeviceBridge ainda requer integração/compilação Windows real). Nada de contornar o bloqueio, inserir Z-hop em G-code original ou homologar Snapmaker por existência de quatro ferramentas. Corrigir/validar a fonte/projeto OEM e provar firmware/controle de envio, ou manter a impressora como não homologada. Permanecem `G4-I=BLOCKED`, `HOMOLOGADOS_OEM=0/3`, `G4-F=NOT_RUN`.

## 7. Estresse oficial Snapmaker U1 quatro ferramentas: FAIL real em 20 invocações — 07/11/2026

**Fonte imutável:** [GitHub Actions #42396616586](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42396616586), artifact `13583613457`, `stress-proof.json`, G-codes A/B/20 e logs originais. Python de QA `lab/snapmaker_u1_four_core_stress_qa.py`, workflow `.github/workflows/filuna-snapmaker-four-core-stress.yml`. O ZIP portátil Snapmaker Orca Windows v2.4.1, perfis efetivos, quatro objetos/extrusoras e configuração temporária de bicos herdada da máquina permaneceram fixos; nenhuma edição no fabricante ou nos G-codes. As 20 chamadas independentes terminaram `exit_code=0`, com T0–T3 e 301 blocos de toolchange.

### Resultado estatístico (amostra, não certificação)

| Grupo de saída OEM original | Ocorrências | Bytes / linhas | SHA funcional (normaliza exclusivamente a única linha `generated by` do HEADER_BLOCK) |
|---|---:|---|---|
| Caminho predominante | **18/20** | **2.638.447 / 107.257** | `1fffba21d2cce0e0456f69836e3e8b5f22aaa35e955ddcb8305627457aca12d1` |
| Caminho divergente | **2/20** (iterações 09 e 14) | **2.634.118 / 107.066** | `b42e9a25f5440cf7e0b7d422f36855506c8ed71a2a73e7f66dd1b479d401fa7a` |

**Veredito correto:** `OEM_FOUR_TOOL_STRESS_REPEATABILITY=FAIL_NON_TIMESTAMP_TOOLPATH_VARIANCE`. Mesmo 3MF/arquivo/presets/versão gerou duas trajetórias G-code funcionais diferentes em 20 execuções. O 2/2 anterior tinha PASS verdadeiro apenas da amostra limitada, mas **não passou no estresse**.

Comparação independente dos dois G-codes originais (`stress-00` e `stress-09`) via `difflib.SequenceMatcher` com igualdade exata de linha identificou **407 grupos de edição**, **531 linhas no A e 340 linhas no B dentro dos grupos**, e **79.629 versus 79.477 comandos G0–G3**. Exemplos verificáveis:
- Mudanças de **G1 E**: `E-1.47479 F1800` versus `E-1.20437 F1800` (retração).
- Mudanças de **G1 F**: `F11552.663` versus `F12177.387` (velocidades).
- Arco **G3 Z20.45 I-1.215 J.075 P1` versus `G3 Z20.45 I-1.216 J.053 P1` (geometria).
- Posições de comandos M73 também mudam, mas **não explicam ou autorizam desconsiderar as diferenças de movimento, extrusão e arco**.
Os grupos de edição foram calculados sobre dados originais, sem alterar, subtrair movimentos ou reescrever qualquer arquivo. Contagens são métricas de comparação, não equivalência.

### Segurança reprovada em **ambos** os resultados OEM

| Grupo | Decisão Python | Linha/camada original | Cruzamento de risco, topo depositado |
|---|---|---|---|
| 18/20 predominante | **BLOCK** | **106.623 / 99** | X132.044 Y141.691 Z20.05 mm, topo Z20.05 (impresso na linha 104.537) |
| 2/20 divergente | **BLOCK** | **106.432 / 99** | **mesmo XYZ**, topo Z20.05 (impresso na linha 104.346) |

O QA de dupla prova [#42396347068](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42396347068) já demonstrou Python+C++17 com BLOCK para o primeiro caminho; recomenda-se repetir o C++ nativo no **segundo** caminho de G-code para manter cobertura de ambos. **Um CI verde que detecta corretamente risco nunca torna o G-code seguro para executar.**

### Limites e bloqueadores remanescentes

- O erro de leitura/exportação do 3MF `handy_models/OrcaCube_v2.3mf` OEM original `0xC0000005` permanece **independente** do fatiamento Core 3MF sintético, que funciona.
- A causa raiz das divergências funcionais no algoritmo OEM (ordenação, concorrência, memória, planner, costura, retração, arcos ou outros) **não foi isolada**; não atribuir um desses motivos como fato sem controle causal.
- `FILUNA_NATIVE_GCODE_PARITY=NOT_PROVEN`, `SNAPMAKER_4_TOOL_SAFETY=BLOCKED`, `DEVICE_NETWORK_ACK=NOT_RUN`, `FIRMWARE_LIVE=NOT_RUN`, `G4-I=BLOCKED`, `INTERNAL_OEM_HOMOLOGATION=0/3`, `PHYSICAL_BETA=NOT_RUN`.
- Preservar PR DRAFT, sem merge, release, instalador, upload de G-code ou impressão. O workflow de compilação GUI MSVC [#42394738274](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42394738274) permanecia em execução na verificação deste checkpoint; sua conclusão ainda precisa ser lida.


## Checkpoint 07/11/2026 19:23 UTC — erro concreto MSVC/CMake
- [Windows Actions #42394738274](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42394738274), job `128469957670`, fonte `709c7e1dac974f775225e2e68436e04ddacc046a`: **FAIL**, etapa 8 `Compile shared engine and GUI`. Etapas de dependências MSVC e cache **PASS**; o erro ocorreu na configuração CMake, antes de compilar a GUI.
- **Primeiro erro exato:** `CMake Error at fabricantes/CMakeLists.txt:25 (target_link_libraries): The plain signature for target_link_libraries has already been used with the target "libslic3r_gui". All uses ... must be either all-keyword or all-plain.` Referências `src/slic3r/CMakeLists.txt:948,974,986,1052`. Não é erro do scanner de G-code nem falha de paridade.
- **Correção mínima somente nesta branch:** commit [`d7762d13ae915387623ad0787bf2a9d1af90adf0`](https://github.com/abcollicelli-web/FilunaSlicer/commit/d7762d13ae915387623ad0787bf2a9d1af90adf0), substitui `target_link_libraries(libslic3r_gui PRIVATE ${FILUNA_OEM_TARGETS})` por assinatura plain `target_link_libraries(libslic3r_gui ${FILUNA_OEM_TARGETS})` em `fabricantes/CMakeLists.txt`, compatível com as chamadas existentes. Nenhum arquivo G-code ou OEM alterado.
- **Gates:** `WINDOWS_MSVC_DEPS=PASS` no run anterior; `WINDOWS_CMAKE_CONFIG=FAIL` no SHA anterior; `WINDOWS_CMAKE_CONFIG_FIXED=NOT_RUN` no novo SHA até resultado de Actions; `WINDOWS_FULL_GUI_BUILD=NOT_RUN`; `FILUNA_NATIVE_GCODE_PARITY=NOT_PROVEN`; `SNAPMAKER_4_TOOL_SAFETY=BLOCKED`; `DEVICE_NETWORK_ACK=NOT_RUN`; `PHYSICAL_BETA=NOT_RUN`; `G4-I=BLOCKED`; `INTERNAL_OEM_HOMOLOGATION=0/3`.
- Próxima prova: execução Windows no SHA corrigido, confirmar CMake configure, compilação real e regressões. Não reexecutar o run antigo como prova da correção porque ele referencia o SHA anterior.


## Rodada adicional — auditoria do adaptador Snapmaker e teste de prontidão (07/11/2026)
- Fonte real `fabricantes/snapmaker/adapter/snapmaker_adapter.cpp`: `create_snapmaker()` retorna `PendingVendorAdapter`; não há contrato de rede/G-code U1 implementado nesse adaptador. `fabricantes/snapmaker/vendor.yaml` mantém `network.compiled_into_app=false`, `contract_ready=false` e `filuna.slice=null`. Portanto **SNAPMAKER_NATIVE_ADAPTER=BLOCKED**, não declarar paridade ou rede pronta.
- Em `lab/filuna_batch_test.cpp`, o teste anterior chamava `status_stream()` para detectar pendência; isso podia invocar rede num smoke test. Commit [29a56863](https://github.com/abcollicelli-web/FilunaSlicer/commit/29a56863f6bcc52cc5bcfe8e7ec7f6cf80b4ab6a) substitui por identificação RTTI de `PendingVendorAdapter`, sem descoberta, pareamento ou I/O. `QA_PENDING_ADAPTER_DETECTION=IMPLEMENTED_NOT_RUN` até compilar/executar CI na revisão nova.
- A alteração **não** implementa a Snapmaker U1, não altera motor Orca, não transforma placeholder em SDK OEM, não edita G-code oficial. Não é correção definitiva de G-code/rede. Gates `SNAPMAKER_GCODE_PARITY=NOT_PROVEN`, `SNAPMAKER_NETWORK_ACK=NOT_RUN`, `U1_FOUR_TOOL_SAFETY=BLOCKED`, `G4-I=BLOCKED` preservados.
- Nenhum merge, deploy, release, instalador, servidor, Bambuddy, dispositivo físico ou impressão.
