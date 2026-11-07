# Filuna QA — Snapmaker U1 (0.4 mm), fonte oficial v2.4.1

**Modelo seguinte escolhido após Bambu A1, Elegoo CC2, QIDI Q2 e Creality K2 Plus.**
**Estado:** laboratório iniciado; até haver G-code A/B comprovado, **NÃO HOMOLOGADO**.

## Fontes originais verificadas

- Release portátil oficial: `Snapmaker/OrcaSlicer`, tag `v2.4.1`, `Snapmaker_Orca_Windows_V2.4.1_portable.zip`, SHA-256 `1f002e2168c576f800bcfd0509b5e164b5ad65a0963abc2bef8317d929d48c9d`.
- `machine.json` da versão oficial: `resources/profiles/Snapmaker/machine/Snapmaker U1 (0.4 nozzle).json` — blob Git `d9a6ab12cb872af25275fab6045b05cfc4531e2f`.
- `process.json` da versão oficial: `resources/profiles/Snapmaker/process/0.20mm Standard @Snapmaker U1 (0.4 nozzle).json` — blob Git `932a52d485a20ea2042716b366733f48f40facfc`.
- `filament.json` da versão oficial: `resources/profiles/Snapmaker/filament/Snapmaker PLA Basic @U1.json` — blob Git `8e4ba5984b072578b82c842b7b465947bf82cfb2`.
- Extrusoras declaradas no perfil de máquina: quatro bicos de 0,4 mm. O ensaio de cubo usa as quatro posições com o **mesmo filamento PLA oficial**, sem afirmar validação de troca de ferramentas ou multicor.

## Atenção à divergência dos perfis antigos

A árvore `resources/profiles/Snapmaker` do Filuna herdado contém valores e nomes de perfis que divergem da release `v2.4.1` (inclusive nome do processo). **Não misturar essas árvores** no teste atual. A versão original fixada em `fixtures/oem/snapmaker/official-profiles` é a fonte do ensaio, com blob Git byte-exato.

## Gates de QA interno

1. Origem, ZIP, SHA-256 e três blobs oficiais do GitHub.
2. Confirmação do modelo U1 e bico 0,4 e compatibilidade de processo/filamento.
3. CLI do Snapmaker portátil **real**, duas execuções independentes (A/B), cada uma com diretório, log, arquivo G-code e SHA próprios; nenhuma normalização autorizada sem prova A/B.
4. Exportação opcional do 3MF pelo executável oficial (independente do G-code).
5. Próximas fases internas: integração do módulo de fatiamento Filuna × OEM, simulação dos protocolos de dispositivos, UI Preparar→Fatiar→Enviar e regressões Windows.

**Não confundir a conclusão de uma CLI oficial com `PARIDADE_FILUNA_PASS` nem com `INTERNAL_QA_PASS`.**

## Regra de lançamento aprovada em 07/11/2026

`FILUNA/REGRA-FASES-HOMOLOGACAO-INTERNA-BETA-FISICA-20261107.md`: todos os testes internos primeiro; **depois** build final; **depois** beta vigiado, quando será feita validação física dos dispositivos. Somente Bambu com Bambuddy foi intencionalmente adiado. Nenhuma prova física é exigida na fase portátil inicial, mas todo resultado físico permanece não testado até o beta.

## Execução

- Script: `lab/snapmaker_u1_official_qa.py`
- Workflow: `.github/workflows/filuna-snapmaker-u1-qa.yml`
- Primeira execução: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42311663149

Nenhum merge, release ou instalação de teste autorizado por esta evidência.

## Diagnóstico real do Windows — 07/11/2026

- Execução `#42311663149`: ZIP e binário oficial validados. Falha no check de SHA do perfil de máquina no **arquivo de trabalho Windows**: o checkout converteu fim de linha, sem alterar o objeto Git original. Corrigido usando `git show HEAD:<path>` com bytes originais para validar os blobs e alimentar a CLI.
- Execução `#42311921546`: `OFFICIAL_PORTABLE_ARCHIVE=PASS`, `PORTABLE_EXECUTABLE=PASS`, `U1_OFFICIAL_PROFILE_PROVENANCE=PASS` e `CUBE_FIXTURE=PASS`. O binário original retornou **exit 3221225477 = 0xC0000005 (access violation)** ao carregar o `verified-process.json`, antes da geração de G-code. `OFFICIAL_A=FAIL`, `OFFICIAL_A_B=PENDENTE`, `U1_PROJECT_EXPORT=PENDENTE`. Logs: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42311921546/artifacts/13548229696 .
- O `process.json` original possui BOM UTF-8, mas **a causa do crash ainda não está comprovada**. O runner foi estendido com testes isolados (help, só máquina, só processo, processo sem BOM e máquina+processo original) para determinar a origem sem alterar binário OEM. Essa matriz nunca é promovida automaticamente a homologação.

**Esta não é uma falha de impressora física**, e nenhum teste com hardware será realizado até o beta vigiado, conforme regra vigente.

## Isolamento complementar e resolução da herança original

A execução `#42312556473` demonstrou `HELP=0`; somente máquina chega à verificação de compatibilidade sem crash (`exit=-17` em signed), mas só processo, máquina+processo e **processo mínimo** resultaram em `0xC0000005`. Assim, não é comprovado defeito de BOM nem de uma única opção extra. Matriz: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42312556473/artifacts/13549247169 .

O programa seguinte de diagnóstico faz a derivação **efetiva** dos perfis por árvore de herança OEM completa, sem reescrever nenhum blob upstream:

- Máquina: `Snapmaker U1 (0.4 nozzle)` → `fdm_U1` → `fdm_toolchanger` → `fdm_klipper`.
- Processo: `0.20mm Standard @Snapmaker U1 (0.4 nozzle)` → `fdm_process_U1_0.20` → `fdm_process_U1_common` → `fdm_process_U1`.
- Filamento: `Snapmaker PLA Basic @U1` → `Snapmaker PLA Basic @U1 base` → `fdm_filament_pla_category` → `fdm_filament_common`.

Os **nove arquivos-pai** foram copiados da tag oficial `v2.4.1` para `fixtures/oem/snapmaker/official-profiles/parents/`, com SHA Git por arquivo e manifesto `parents.lock.json`. O script `lab/snapmaker_u1_inherit.py` valida os 12 blobs (folhas + pais), deriva temporariamente uma configuração por tipo com precedência do filho, registra `effective-profile-derivation.json` e testa a CLI oficial A/B de novo. A saída é categorizada como `OFFICIAL_EFFECTIVE_A_B`, **não** paridade Filuna. Sem G-code real e hashes, não há PASS.

A fase de homologação física permanece reservada exclusivamente ao beta vigiado posterior ao build final, sem nenhuma impressão de teste remota automática.


## Resultado efetivo Windows — run #42313078703 (06/11/2026 UTC)

Prova: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42313078703

- ZIP oficial v2.4.1 e executável: PASS.
- Perfis-pai oficiais originais (9) e folhas (3), configuração efetiva derivada: PASS na verificação de origem e herança.
- Ao fatiar cubo, a CLI oficial A retornou **3221225477 (0xC0000005, access violation)**, nenhum arquivo G-code.
- Com todas as heranças efetivas, a CLI oficial A novamente retornou **3221225477 (0xC0000005)**, nenhum G-code. Portanto, a herança incompleta sozinha NÃO explica a falha.
- OFFICIAL_A_B e OFFICIAL_EFFECTIVE_A_B: PENDING (G-code não gerado).
- PARIDADE_FILUNA, DISPOSITIVOS/REDE, HOMOLOGACAO_INTERNA: PENDING.
- Artefatos dos logs e dos perfis efetivos: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42313078703/artifacts/13548907984

**Diagnóstico obrigatório:** comparar as opções CLI suportadas por esse executável e a formação de configuração efetiva; investigar access violation em Windows com caso mínimo ou dump, sem alterar os perfis oficiais nem atribuir causa sem prova. Teste físico somente no beta vigiado após validação interna e compilação final. Snapmaker U1 permanece NÃO HOMOLOGADA; a investigação continua em paralelo à Prusa.


## Hipótese de vetor de processo isolada, mas NÃO confirmada — run #42314656389

Análise do código original Snapmaker `src/Snapmaker_Orca.cpp` na tag `v2.4.1` apontou possível acesso sem guarda a `different_settings_to_system[0]`, quando a lista poderia estar vazia. O diagnóstico criou uma **cópia temporária** do processo efetivo, acrescendo **apenas** `"different_settings_to_system": [""]`, com as configurações OEM originais intactas.

Execução Windows: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42314656389

- ZIP/binário verificados: PASS.
- 12 fontes oficiais da árvore de herança: PASS.
- Cópia de campo único com hash próprio: PASS.
- Controle com processo original efetivo: reproduziu `0xC0000005` sem G-code: PASS como reprodução de bug.
- **Cópia diagnóstica com vetor preenchido: também apresentou `0xC0000005`**, sem G-code: FAIL do diagnóstico; portanto **não foi solução**.
- `OFFICIAL_A_B`, `FILUNA_PARITY`, `NETWORK`: PENDENTE; U1 continua NÃO HOMOLOGADA.

Artefato: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42314656389/artifacts/13550241939

Próximo foco técnico: isolar `ConfigBase::load_from_json` e `DynamicPrintConfig::normalize_fdm()` no binário v2.4.1 mediante matriz mínima de processos ou depurador disponível no runner. Não afirmar causalidade do campo `different_settings_to_system` sem backtrace. Nenhuma alteração de perfis ativos OEM, compilação ou impressão física.


## Diagnóstico causal e fatiamento de controle com bicos OEM — runs #42316614470 e #42316895543 (07/11/2026)

A auditoria da tag oficial `v2.4.1`, commit `bac6fd5c1e1c23ddf057cf6471ba4ed7ed621c4c`, revelou uma falha de segurança de acesso em `src/libslic3r/PrintConfig.cpp`, função `DynamicPrintConfig::normalize_fdm`: se um preset de **processo** define `wipe_tower_filament`, a rotina dereferencia `this->opt<ConfigOptionFloats>("nozzle_diameter")->size()` sem checar a ausência de `nozzle_diameter`. No fluxo CLI, **cada** arquivo `--load-settings` é normalizado isoladamente antes da união de máquina+processo, então o bico presente no perfil da máquina ainda não está no processo.

**Reprodução causal em Windows original** — https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42316614470:

| Caso no binário oficial, com cópia temporária de processo | CLI exit | `loaded process config` | G-code |
|---|---:|---|---|
| Perfil original completo | `3221225477 / 0xC0000005` | Não | Não |
| Perfil mínimo sem `wipe_tower_filament` | 0 | Sim | Sim |
| Mesmo mínimo com `wipe_tower_filament` | `0xC0000005` | Não | Não |
| Completo removendo somente `wipe_tower_filament` | 0 | Sim | Sim |
| Completo acrescentando somente `nozzle_diameter` da máquina | 0 | Sim | Sim |

A prova isolou a condição e confirmou o defeito de carregamento da CLI da fonte oficial. Não altera nem aprova o binário original, não simula impressão física. A hipótese anterior `different_settings_to_system` já foi descartada; não é a causa.

**Workaround de ensaio genérico e preservador de comportamento:** `lab/snapmaker_runtime_nozzle_guard_qa.py` identifica automaticamente modelo/compatibilidade e **lê todos os valores e a quantidade de bicos do preset efetivo da máquina**, não fixa U1 nem quatro extrusoras. Cria cópia temporária do processo com só `nozzle_diameter` acrescentado e mantém `wipe_tower_filament` original. Se identidade, tipo, compatibilidade ou medidas forem inválidos, bloqueia. Seis testes unitários positivos/negativos PASS.

Execução: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42316895543

- ZIP portátil oficial SHA-256 `1f002e2168c576f800bcfd0509b5e164b5ad65a0963abc2bef8317d929d48c9d`: **PASS**.
- Perfis 12 blobs originais + quatro bicos de 0,4 obtidos por herança OEM verificada: **PASS**.
- G-code U1 cubo A e B: **250.183 bytes cada**, SHA-256 bruto idêntico `ae4551a3274e3015247007fb57a6c0d8231ba7c1e1b989118508bf361926d4a6`: **PASS para cópia de processo de laboratório**, em saídas independentes.
- O G-code contém sequência OEM `PRINT_START`, preparação de extrusoras, `SM_PRINT_CHECK_SWITCH_EXTRUDER`, `BED_MESH_CALIBRATE` e `PRINT_END`. A presença de comandos não comprova execução física.
- **Limitação:** as duas chamadas geraram cabeçalho no mesmo segundo. Um controle separado temporalmente está em `lab/snapmaker_u1_temporal_ab_qa.py`: deve permitir remover somente a linha de geração situada em `HEADER_BLOCK` após confirmar que **todas as outras linhas** são idênticas.
- Patch OEM nativo: **candidato não aplicado**, `FILUNA/fabricantes/snapmaker/patches/0001-guard-nozzle-diameter-process-load.patch`. Não editar a raiz Orca nem compilar OEM neste ciclo.

**Nenhum dos seguintes gates mudou para PASS:** `OFFICIAL_UNMODIFIED_A_B`, `FILUNA_NATIVE_PARITY`, `NATIVE_NETWORK_ACCESS`, `SEND_ACK`, `MULTICOLOR`, `HOMOLOGACAO_INTERNA`, `FISICA_BETA`.

**Escopo de marca corrigido:** a ferramenta `FILUNA/tools/oem_universal_inventory.py` auditou a fonte oficial completa, **100 perfis de máquina/variantes, 112 de impressão, 312 de filamento** e 2.115 registros de fonte/recurso no run https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42316033247. Há 101 itens em revisão; isso é inventário, não integração. O `FILUNA/fabricantes/snapmaker/manifest.json` usa descoberta de todos os modelos por árvore OEM e não uma whitelist de U1.


## Prova temporal e novo bloqueio de reprodutibilidade entre sessões (07/11/2026)

- Execução Windows https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42317221768: ZIP oficial, geração com a cópia temporária de bicos e **6 controles automatizados de normalização** PASS.
- Dois processos reais separados: `2026-11-07T00:18:08Z` e `2026-11-07T00:18:11Z`. G-codes separados de **249.098 bytes** cada; hashes brutos diferentes apenas pela linha `; generated by Snapmaker Orca 2.4.1 on ...` no `HEADER_BLOCK`. Sem remover nenhuma outra linha, o SHA-256 normalizado é **`06687a6b84ee16d0d7ceb75f8c631272b649f452313ce4395864893e372247d7`**. Comandos OEM `PRINT_START`, `SM_PRINT_CHECK_SWITCH_EXTRUDER`, `BED_MESH_CALIBRATE` e `PRINT_END` presentes (sem provar execução no hardware).
- **Atenção:** ao comparar com a execução Windows anterior https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42316895543, o G-code foi de **250.183 bytes** no run anterior, embora o ZIP oficial, o perfil de máquina, o de processo, o de filamento e a cópia `nozzle_diameter` sejam byte-idênticos. Uma comparação direta dos artefatos encontrou diferenças efetivas em extrusão/retração, movimentos e posição de `M73`, **não só no cabeçalho**. Não é autorizado normalizar tais linhas; isso impede declarar repetibilidade entre sessões.
- Para separar variação por horário/minuto de variação entre runners, criou-se `lab/snapmaker_u1_cross_minute_qa.py`, workflow `.github/workflows/filuna-snapmaker-u1-cross-minute.yml`; a comparação exige dois processos no mesmo Windows atravessando limite de minuto e falha se qualquer byte fora do timestamp explícito diferir.
- **Estado:** `GUARDED_CLI_SHORT_INTERVAL_A_B=PASS`; `CROSS_SESSION_GCODE_REPRODUCIBILITY=BLOCKED` até explicação e teste demonstrado; `ORIGINAL_UNMODIFIED_AB`, `FILUNA_PARITY`, `DEVICE_NETWORK` e `INTERNAL_HOMOLOGATION` continuam pendentes. O aparente sucesso da cópia temporária não dispensa o restante.


### Controle adicional entre minutos — run #42317564346

- Duas execuções independentes do **mesmo** ZIP OEM original e dos mesmos três perfis efetivos, com o workaround temporário que preserva `wipe_tower_filament` e obtém bicos da máquina.
- Horários de geração confirmados no `HEADER_BLOCK`: `2026-11-07T00:21:00Z` e `2026-11-07T00:22:02Z` (aproximadamente 61 s de intervalo).
- Arquivos de saída separados: **250.183 bytes cada**; hashes brutos distintos pelo timestamp. **Tudo fora da linha de timestamp idêntico**, SHA-256 normalizado no critério estrito `2774becee1246529474db12c7439cc1d46a501bfe797c31a5e53936db705db4a`.
- Execução Windows e artefatos: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42317564346
- **Conclusão delimitada:** mudou o minuto e a geração permaneceu idêntica, portanto **a mudança de minuto sozinha NÃO explica a divergência entre jobs**.

### Comparação estrita entre três jobs oficiais (mesmos arquivos de entrada; ainda divergentes)

| Windows job | G-code bytes | Observação |
|---|---:|---|
| #42316895543 | 250.183 | Variante A |
| #42317221768 | 249.098 | Variante B, diferenças de movimento e retração |
| #42317564346 | 250.183 | Variante A, idêntica ao primeiro fora do timestamp |

Conferidos os arquivos efetivos **byte a byte**: perfis de máquina, processo e filamento, além da cópia temporária de processo com o bico, são idênticos nos artefatos dos três jobs. O ZIP/executável está pinado ao mesmo SHA. Uma comparação textual A×B revelou diferenças em pequenos segmentos de trajetória, `WIPE_START`/retrações e posicionamento de `M73`, além da data; **não podem ser normalizadas como ruído de timestamp**. A identidade de geometria/modelo é conservada, mas ainda não há causa comprovada da variação entre jobs (ambiente do runner, caminho da saída, concorrência ou outro fator requerem isolamento adicional).

**Gate adicional `CROSS_JOB_GCODE_REPRODUCIBILITY=FAIL`, não PASS.** O protocolo nativo Snapmaker também está pendente: fontes oficiais localizadas e contratos de gateway documentados em `FILUNA/qa/SNAPMAKER-U1-REDE-CONTROLE-FONTES-20261107.md`, sem compilar nem ativar rede.

Manter `G4-I / HOMOLOGACAO_INTERNA_U1=BLOCKED` até verificar integração nativa do Filuna, acesso/controle em laboratório e explicar/solucionar a divergência de G-code. Não gerar compilação final, não imprimir no hardware nem promover PR #6.


### Repetição exata do workflow de origem — PASS restrito à mesma configuração (07/11/2026)

O job `#42316895543`, tentativa **2**, foi reexecutado sem alterar script, parâmetros nem commit do workflow inicial. Fonte: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42316895543

- Tentativa 1: G-code 250.183 bytes; A/B na mesma sessão PASS.
- Tentativa 2: G-code 250.183 bytes; A/B na mesma sessão PASS.
- Comparando **os dois artefatos reais** e removendo **só** a linha explícita de timestamp do `HEADER_BLOCK`, ambos têm SHA-256 normalizado **`2774becee1246529474db12c7439cc1d46a501bfe797c31a5e53936db705db4a`**.
- A execução independente **entre minutos** `#42317564346` também tem esse mesmo hash normalizado. Portanto, a afirmação genérica anterior `CROSS_JOB_REPRODUCIBILITY=FAIL` deve ser lida com a presente **retificação de escopo**: `SAME_WORKFLOW_CROSS_ATTEMPT=PASS`, `CROSS_MINUTE=PASS`.
- O workflow **diferente** `#42317221768` manteve ZIP OEM e perfis byte-idênticos mas produziu outro G-code válido de 249.098 bytes, SHA normalizado `06687a6b84ee16d0d7ceb75f8c631272b649f452313ce4395864893e372247d7`, com diferenças de movimento/retração. Isso mantém `CROSS_WORKFLOW_TOOLPATH_VARIANCE=UNEXPLAINED/BLOCKED`, sem autorizar normalização de comandos e sem concluir que o mesmo job seja não determinístico.

**Próxima verificação indicada:** comparar exatamente a invocação CLI, nomes/paths de perfis temporários e ambiente de runner dos dois workflows distintos, isolando um fator por vez. Enquanto não houver paridade do motor **Filuna** e implementação real de gateway/controle da U1, a homologação interna permanece **BLOQUEADA**. No PR só existe o patch de fonte **candidato**, não aplicado ao motor comum nem compilado.


## Validação do patch de código-fonte OEM com Git apply (07/11/2026)

A ferramenta `FILUNA/fabricantes/snapmaker/patches/0001-guard-nozzle-diameter-process-load.patch` foi convertida em diff Git válido e checada **contra o commit oficial exato** `bac6fd5c1e1c23ddf057cf6471ba4ed7ed621c4c` no GitHub Actions Linux.

- [Execução #42319819540](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42319819540): **SUCCESS** `git apply --check`, `git apply`, verificação de alteração **somente** em `src/libslic3r/PrintConfig.cpp`, verificação da presença da guarda de `nozzle_diameter`, `git diff --check`, `git apply -R` e confirmação de árvore OEM limpa. Sem build, instalador ou rede/print.
- **Patch candidato válido** para correção nativa isolada da carga de processo; não afeta todo Orca até homologação de ABI/integração.
- O A/B oficial temporário da U1 permaneceu PASS nos ensaios anteriores, mas as variações de G-code entre workflows, o build do Filuna, rede/controle do fabricante e comparação Filuna×OEM continuam pendentes.

**`SOURCE_PATCH_APPLIES_EXACT_UPSTREAM=PASS`**, **`NATIVE_FILUNA_CODE_LINKED=NOT_RUN`**, **`HOMOLOGACAO_INTERNA=BLOCKED`**. Não usar sucesso de `git apply` como prova de execução de motor.

## Evidência decisiva — deriva de G-code SEM mudança de perfis nem de caminhos (07/11/2026)

**Duas campanhas novas com o executável original Snapmaker Orca 2.4.1**, pacote ZIP SHA `1f002e2168c576f800bcfd0509b5e164b5ad65a0963abc2bef8317d929d48c9d`, perfis OEM pinados e guard provisório **idêntico** (apenas injeta os quatro bicos reais `nozzle_diameter` na cópia de processo para evitar o acesso inválido já provado). Nenhum código-fonte OEM foi compilado ou porta de impressora acionada.

1. [#42320900067](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42320900067): modificação do caminho dos arquivos de processo ou filamento coincidiu com mudanças reais nos movimentos G-code. **Hipótese inicialmente formulada: caminhos influenciam a geração; não foi comprovada causa.**
2. [#42321507580](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321507580): **11 invocações independentes**, com nomes/caminhos de perfis trocados, produziram **11 G-codes funcionais idênticos fora do timestamp**, hash normalizado `2774becee1246529474db12c7439cc1d46a501bfe797c31a5e53936db705db4a`. Logo, mudança de caminho **NÃO É suficiente** para causar a falha.
3. [#42321603426](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321603426): **10 invocações**, mesmas versões de todos os perfis e binário; **o mesmo conjunto exato de arquivos nos mesmos caminhos** gerou dois hashes normalizados para o caso `base` (250.183 e 249.098 bytes). Outro alias de filamento produziu uma terceira variante de 249.212 bytes. Gate FAIL.
4. [#42321783683](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321783683): **40 invocações independentes** alternando originais e aliases de nomes dos perfis. A maior parte dos resultados é `250183 bytes / 2774becee1...`, mas em execuções de **exatamente os mesmos perfis/caminhos** apareceram variantes de 249.212 e 249.098 bytes, com hashes diferentes após remover somente o timestamp; também observadas alterações de movimentos e retrações. O stress deu `FAIL_MULTIPLE_NON_TIMESTAMP_TOOLPATH_VARIANTS`. Três exemplos: `base` run 7 = 249.212 bytes, run 8 = 250.183 bytes; `machine-alias` run 27 = 249.098 bytes e run 29 = 250.183 bytes.

**Conclusão nova que prevalece sobre as hipóteses anteriores:** há **instabilidade intermitente de G-code de movimento/retração no executável OEM 2.4.1** sob o mesmo projeto e mesmos parâmetros, não apenas nomes de arquivo ou timestamps. Não normalizar diferenças de trajetórias, não declarar `CROSS_WORKFLOW` como causa, não publicar U1 como homologada. Necessário isolar o componente/planner que causa deriva e depois verificar a correção no módulo Filuna por comparador estrito sob repetição, além do gateway de rede/AMS/upload/ACK. **Causador técnico exato ainda não provado.** A correção `wipe_tower_filament` é apenas para falha de carregamento de preset, não resolve a deriva de G-code.

**Estado:** `SOURCE_PATCH_APPLIES=PASS`; `OFFICIAL_GUARDED_CLI_RUNS=PASS_WITH_INTERMITTENT_DIFFERENCES`; `IDENTICAL_INPUT_GCODE_REPEATABILITY=FAIL`; `FILUNA_NATIVE_PARITY=NOT_RUN`; `NETWORK_DEVICE_GATEWAY=NOT_RUN`; `INTERNAL_HOMOLOGATION=BLOCKED`; `BETA_PHYSICAL=NOT_RUN`.


## Atualização crítica — falha REAL e reproduzida de G-code sob testes de estresse (07/11/2026)

**Não promover Snapmaker U1 à homologação interna.** As amostras A/B inicialmente aprovadas não provavam estabilidade sustentada do binário original v2.4.1.

1. [Teste de quatro caminhos #42320900067](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42320900067): G-code real de 250.183 e 249.098 bytes para os mesmos hashes de perfis; dois caminhos modificados coincidiram com versão diferente de trajetórias. A hipótese provisória de dependência de nome de arquivo foi levantada, **mas não confirmada**.
2. [Teste de 11 caminhos/repetições #42321507580](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321507580): **PASS**, 11/11 G-codes de 250.183 bytes e mesmo hash normalizado \`2774becee1246529474db12c7439cc1d46a501bfe797c31a5e53936db705db4a\`. Este ensaio isolado exclui afirmar que alterar path necessariamente altera o G-code; **não exclui falha rara do slicer**.
3. [Teste independente repetido de 10 invocações #42321603426](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321603426): **FAIL**, inclusive com **mesmo path repetido**, 249.098, 249.212 e 250.183 bytes, evidência concreta contra a tese de causalidade obrigatória do nome do arquivo.
4. [Estresse original Windows 40 CLI #42321783683](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321783683): **FAIL legítimo** (não falha da infraestrutura). Mesmo ZIP original SHA \`1f002e2168c576f800bcfd0509b5e164b5ad65a0963abc2bef8317d929d48c9d\`, mesmas configurações de máquina/processo/filamento, mesmo cubo e apenas **uma cópia temporária de processo com bicos herdados** usada em todas as execuções, **40 processos CLI independentes**. Os hashes normalizados do G-code agrupam:
   - 35/40: **250.183 bytes**, hash \`2774becee1246529474db12c7439cc1d46a501bfe797c31a5e53936db705db4a\`;
   - 3/40: **249.098 bytes**, hash \`06687a6b84ee16d0d7ceb75f8c631272b649f452313ce4395864893e372247d7\`;
   - 1/40: **249.212 bytes**, hash \`c122c77d994d157b7d7fd8e9f1e3282775c351627ab4ee595349ee8c05d00f4d\`;
   - 1/40: **249.212 bytes**, hash \`9bf76f58cb59d678c3f2630d71ade1df77f94dfff4b07b8b33b9541b97b6a8fb\`.
   - **Quatro hashes de G-code não equivalentes**, mesmo descontando apenas a linha gerada de data no cabeçalho. O diff bruto mostra linhas extras de movimento, variações de wipe/retração, arco Z e posição M73; diferenças **SEMÂNTICAS E REAIS** que não podem ser normalizadas.

**Diagnóstico com limites:** A reprodução de variações no **mesmo** comando e perfil confirma **não repetibilidade ocasional do caminho de ferramenta no binário OEM original v2.4.1 nas condições testadas**; ainda não prova se a causa é concorrência interna, cache, algoritmos de costura, estado do processo, dependência runtime ou bug upstream. O nome de arquivo não é causa necessária. A guarda \`nozzle_diameter\` é uma correção de acesso nulo distinta e não resolve essa variação. Sem build Filuna, não sabemos se o Filuna nativo teria o mesmo comportamento.

**Próximo gate bloqueador**: investigar fonte/configurações do planejador/wipe/seam e concorrência, estabelecer um conjunto de configuração de produção determinístico e teste de regressão de longa duração; comparar caminhos de verdade contra OEM original como referência de conformidade, sem ocultar divergências. **\`STRESS_STRICT_GCODE=FAIL\`**, **\`ROOT_CAUSE=NOT_IDENTIFIED\`**, **\`FILUNA_NATIVE_PARITY=NOT_RUN\`**, **\`GATE_REDE_U1=NOT_RUN\`**, **\`HOMOLOGACAO_INTERNA=BLOCKED\`**, **\`BETA_FISICO=NOT_RUN\`**. Nenhum build ou hardware iniciado por este diagnóstico.


## Bloqueadores P0 de segurança e estabilidade — diagnóstico adicional de 07/11/2026

**1. Bug upstream de colisão possível com peça finalizada.** Issue oficial [Snapmaker/OrcaSlicer #1149](https://github.com/Snapmaker/OrcaSlicer/issues/1149) **ABERTA** na consulta de 07/11/2026. Snapmaker Orca **2.4.1** pode omitir Z-hop/retração no deslocamento final à torre de purga U1 em impressão multicolor. Relato físico descreve arranhão na peça; a análise aponta **\`src/libslic3r/GCode.cpp\`, \`WipeTowerIntegration::append_tcr2()\`**, condição alterada no commit \`b626b4be6\`. **Não é o mesmo problema do acesso nulo em \`PrintConfig.cpp\`**, nem se deve declará-lo corrigido pelo patch \`0001\`. O fabricante ainda não publicou correção homologada nesta rodada. <https://forum.snapmaker.com/t/43865>

Patch candidato novo, isolado e **não ativado**: \`FILUNA/fabricantes/snapmaker/patches/0002-candidate-final-purge-z-lift-safety.patch\`; [CI fonte #42323500007](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42323500007) **SUCCESS de \`git apply --check\`/apply/revert em upstream exato**; **NÃO** testou G-code, firmware, segurança física, binário compilado ou Filuna nativo. Verificação verdadeira de trajetória no firmware real ou simulado permanece **P0 obrigatório**.

**2. Afinidade a um único processador não elimina G-code diferente.** [CI #42323435503](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42323435503) executou **24 subprocessos originais no modo CPU padrão e 24 fixados por máscara de afinidade Windows a um único núcleo**. No grupo normal houve **três hashes normalizados distintos** (22/24 resultado majoritário); no de um núcleo, **dois hashes distintos** (23/24 majoritário). \`AFFINITY_SINGLE_CORE_FIX=REFUTED\`; não alterar a execução em produção para um núcleo como correção.

**3. Alterações isoladas de seam e wipe também não eliminam variação.** [CI #42323700744](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42323700744): original fixado, **4 grupos × 20 fatiamentos oficiais**, mudanças controladas apenas em cópias temporárias dos perfis. G-code estrito (única normalização: data original do cabeçalho) produziu:
- OEM original: **2 variantes**;
- \`wipe=["0"]×4\` em vez do padrão: **3 variantes**;
- \`seam_position=nearest\` em vez de \`aligned\`: **2 variantes**;
- ambos: **3 variantes**.

**Nenhum desses perfis experimentais** pode ser entregue ao cliente ou registrado como compatível; todos falharam na repetibilidade, o que demonstra que desligar limpeza e/ou mudar costura não constitui solução. Estes testes não provam causalidade nem isolam bug de concorrência, RNG, geometria ou outro subsistema; exigem auditoria upstream adicional e eventual correção fonte nativa segura.

**Veredito inalterável até prova:** \`SNAPMAKER_U1_GCODE_STABILITY=FAIL\`, \`UPSTREAM_FINAL_PURGE_Z_HOP_SAFETY=OPEN_BLOCKER\`, \`SOURCE_PATCH_APPLIES=PASS_NOT_FUNCTIONAL\`, \`FILUNA_NATIVE_COMPARISON=NOT_RUN\`, \`NETWORK_GATEWAY_ACK=NOT_RUN\`, \`HOMOLOGACAO_INTERNA=BLOCKED\`, \`BETA_FISICO=NOT_RUN\`. **Não compilar instalador final, ativar U1 ou disparar impressões por causa desses patches.**


## Referência regressiva independente — Snapmaker Orca 2.4.0 também varia (07/11/2026)

[Execução #42324349445](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42324349445) **FAIL legítimo de estabilidade**, **30 subprocessos CLI Windows reais**, pacote portátil oficial \`Snapmaker_Orca_Windows_V2.4.0_portable.zip\` SHA-256 \`089b7ca20ccfc4be57ffb5a7f9f81e3a2b55db3a9622755fee2b20208bfcb8c2\`. **Perfis máquina/processo/Generic PLA originais da própria versão 2.4.0**, com herança resolvida da mesma árvore do ZIP, e guarda temporária de acesso nulo idêntica em todas as execuções; **não** se misturaram perfis v2.4.1 com motor 2.4.0. O STL, invocações e parâmetros de fatiamento permaneceram fixados. Teste mantém somente a exceção de data de geração \`Snapmaker Orca 2.4.0\` em cabeçalho.

**Histograma:** 27/30 fatiamentos de 248.825 bytes iguais após normalização; 3/30 divergentes, cada um com SHA funcional distinto e 248.197, 247.867 e 247.789 bytes. **Quatro toolpaths diferentes** no total. Diff real aponta mudanças em feedrate \`G1 F...\`, segmentos de wipe/retração, posicionamento de \`M73\` e outros movimentos; **não** são só comentários/time stamps.

**Conclusão verificável:** a instabilidade no CLI oficial **não foi introduzida exclusivamente na 2.4.1**. Trocar automaticamente para 2.4.0 **não** seria solução homologada. O bug de falta de Z-hop final multi-cor [#1149](https://github.com/Snapmaker/OrcaSlicer/issues/1149) é **outro problema distinto** e tem evidência de regressão na 2.4.1. Os dois continuam P0. Necessário rastrear determinismo/custos/ordenação/perímetros sob fonte e provar paridade do **Filuna nativo** antes de qualquer liberação de impressão.

\`U1_VENDOR_240_STRESS=FAIL\`, \`U1_VENDOR_241_STRESS=FAIL\`, \`FILUNA_NATIVE_PARITY=NOT_RUN\`, \`U1_FINAL_PURGE_SAFETY=NOT_RUN\`, \`HOMOLOGACAO_INTERNA=BLOCKED\`.
