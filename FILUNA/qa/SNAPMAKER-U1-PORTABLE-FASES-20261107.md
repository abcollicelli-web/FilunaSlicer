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
