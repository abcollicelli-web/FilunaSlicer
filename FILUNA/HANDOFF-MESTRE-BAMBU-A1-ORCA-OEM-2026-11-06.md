# FILUNA — HANDOFF MESTRE / RETOMADA EXATA DO PILOTO OEM BAMBU A1

**Data:** 2026-11-06 (Europe/Rome). **Objetivo:** permitir que outro chat ou engenheiro continue exatamente do ponto alcançado, sem refazer decisões, inventar PASS ou perder dados.

> **LEIA PRIMEIRO ESTE DOCUMENTO, os arquivos citados e a branch em seu HEAD ATUAL, ANTES DE ALTERAR CÓDIGO.**
> Repo NOVO e correto: `abcollicelli-web/FilunaSlicer` (Filuna Windows nativo sobre Orca). NÃO confundir com `abcollicelli-web/filuna-slicer` (.NET legado) nem `abcollicelli-web/Filuna` (site).
> PR candidata: https://github.com/abcollicelli-web/FilunaSlicer/pull/5 — branch `filuna/oem-gcode-routing-qa-20261106`; base `filuna/qa-gates-hardening-20261105`. Ainda em rascunho, sem merge nem release. Como podem existir outras automações/agentes, **ler HEAD/PR/working tree novamente** e evitar sobrescrever trabalho concorrente.

## 1. O QUE O PROPRIETÁRIO DETERMINOU — ARQUITETURA FINAL, NÃO DEBATER NOVAMENTE

- **Um único Orca como motor/coluna vertebral**, única interface visual Filuna/Orca e único aplicativo Windows Filuna já compilado. Não empacotar um Bambu Studio, Orca, Anycubic, Flashforge etc. completos nem duplicar binários gigantes.
- Pasta `FILUNA/fabricantes/<marca>/` com **módulo de compatibilidade OEM COMPLETO** de cada fabricante, isto é: somente as diferenças **realmente necessárias** em relação à base Orca fixada, com dependências e licença auditadas. Compartilhar o código comum com Orca. Podem existir subpastas `modelos/`, `engine/`, `transport/`, `profiles/`, `gcode/`.
- **Dispositivos:** ao reconhecer/registrar impressora de rede, vincular automaticamente módulo completo correto de fabricante, modelo, firmware, versão/ABI e capacidades. **Preparar:** mesma seleção de módulo internamente, mantendo GUI Filuna e motor Orca. **Fatiar:** a parte OEM efetivamente integra o pipeline e produz G-code correto para hardware. **Imprimir:** apenas impressoras registradas do usuário, com autorização e ACK real.
- Prioridade de descoberta/protocolo: tentar primeiro portas/serviços locais acessíveis e documentados (SSDP, mDNS, MQTT/TLS, FTPS etc.), sempre respeitando autenticação requerida; **só tentar alternativa LAN Code quando recursos anteriores forem insuficientes e o equipamento permitir**. Importante: **para Bambu o LAN Access Code pode ser a senha necessária para o MQTT/FTPS**, não necessariamente um transporte independente; não tentar conexão anônima com porta só porque ela responde. Sem contornar firmware, usar nuvem privada, copiá-la fechada, chaves ou segredos.
- Padrão universal: todas as marcas e futuras impressoras; depois de homologado um módulo, futuras versões Bambu/Orca/OEM exigem apenas **diff dos arquivos/algoritmos relevantes**, port compatível de novos deltas, regressão e homologação incremental, manifesto SHA/licenças, assinatura, rollback. Não publicar atualizado sem evidência.
- Prioridade **AGORA**: **Bambu Lab A1**. Proprietário quer TESTES REAIS DE FATIAMENTO, não apenas mais documentos. Próximos fabricantes: Anycubic Kobra 3 V2 (confirmar nome/código antes, não confundir com “Kobra V3”), Flashforge AD5X e Elegoo Centauri Carbon 2. Depois aplicar a todos.
- Nunca confundir arquivo/preset com módulo funcional. “Módulo carregado” exige evidência de código em execução no motor Filuna; “homologado” exige comparação de impressão/G-code com referências oficiais, condições idênticas e depois hardware físico, respeitando limitações do simulador.
- **Perguntas pendentes do proprietário e que a próxima sessão deve responder COM EVIDÊNCIAS:** 1. Tamanho **final incorporado** (módulo, todos os arquivos e acréscimo real no pacote Windows); 2. Módulo carregado cobre **toda a linha Bambu** ou somente A1? 3. Existe uma **regra homologada** replicável para todos os fabricantes? 4. Comparativo real `Filuna / Orca / módulo OEM` vs `Bambu Studio oficial` com mesmo modelo e parâmetros efetivos e resultado de G-code. Não afirmar números de build/release sem medir nem validar família inteira baseado na A1.

## 2. REGRAS CANÔNICAS JÁ REGISTRADAS

- `FILUNA/ARQUITETURA-ORCA-UNICO-FABRICANTES-OVERLAY-2026-11-06.md`
- `FILUNA/REGRA-MESTRE-ATUALIZACAO-INCREMENTAL-OEM-ORCA-2026-11-06.md`
- `FILUNA/REGRA-MESTRE-QUALIDADE.md`
- `FILUNA/ARQUITETURA-COMPATIBILIDADE-FABRICANTES.md`
- `FILUNA/REGISTRO-CONTINUIDADE-ETAPAS.md`
- `FILUNA/HOMOLOGACAO-01-FLUXO-OEM-DISPOSITIVOS-PREPARAR-2026-11-06.md`
- `FILUNA/qa/BAMBU-A1-HOMOLOGACAO-01-RESULTADOS-2026-11-06.md`
- `FILUNA/qa/gcode/bambu_a1_source_manifest.json` (SHA blobs de componentes oficiais, estado individual).
- `FILUNA/fabricantes/bambu/manifest.json` e demais manifestos candidatos (Anycubic, Flashforge, Elegoo).

**Hierarquia:** decisão atual Orca único + delta completo por fabricante prevalece sobre qualquer texto antigo de carregar todos os motores ou vários fatiadores inteiros.

## 3. CÓDIGO EFETIVAMENTE CRIADO/ALTERADO NO PR #5

**Pasta de fabricante e fonte Bambu:**
- `FILUNA/fabricantes/bambu/modelos/a1/N2S.json`: snapshot de `bambulab/BambuStudio/resources/printers/N2S.json`, blob oficial `3a5929b98244d7800ee06a7fe41662f0141306ce`. Fonte oficial confirma **Bambu Lab A1 = N2S** e prefixo do serial **039**. Matriz por firmware contém capacidades de impressão; **não habilitar automaticamente diferenças de hardware**.
- `FILUNA/fabricantes/bambu/transport/BambuLanDiscovery.hpp`: parser C++17 da resposta SSDP, valida modelo `N2S`, série prefixo `039`, endereço e identificador; **não faz handshake MQTT/TLS, FTPS, pareamento ou envio de impressão**.
- `FILUNA/fabricantes/bambu/engine/BambuA1Module.hpp`: objeto C++17 `A1Module`, aceita identidade A1/`N2S`, método `activate_for_prepare` devolve `MathCandidateOnly`, utiliza `BambuSpeedMapping.hpp`; **ainda não contém o planner Bambu inteiro**.
- `FILUNA/modules/bambu-a1/BambuSpeedMapping.hpp`: funções de mapeamento de velocidade derivadas do código AGPL `bambulab/BambuStudio/src/libslic3r/GCode.cpp` blob `31cfddddeef4a0f972e8c60b42bf0d24d35017e6`. Compiladas e testadas isoladamente; **não conectadas ao emissor principal de G-code**.
- `src/slic3r/GUI/DeviceCore/FilunaOEMOverlay.hpp`: `resolve_oem_for_stage(Device|Prepare, ...)` aponta ao mesmo módulo. Para a A1, reconhece aliases `N2S`, `BBL-A1`, nome comercial.
- `src/slic3r/GUI/DeviceCore/FilunaVendorRegistry.hpp`: `model_for_name("N2S") -> BBL-A1`.
- `src/slic3r/GUI/DeviceCore/DevManager.cpp`: hook na seleção de dispositivo, aponta `A1Module`, com log; **não prova conexão real de rede OEM**.
- `src/slic3r/GUI/Plater.cpp::reslice()`: hook no fluxo Preparar/Reslice chama entrada do módulo A1, **ainda não comprovado por compilação/reexecução do aplicativo Windows**. Sua chamada só registra estado matemático candidato; não injeta o planner.
- `resources/printers/N2S.json`: já existia no fork; **não idêntico** à matriz Bambu oficial. O snapshot oficial fica em `FILUNA/fabricantes/bambu/modelos/a1/N2S.json`; preservar conservadorismo até testar.
- `FILUNA/fabricantes/{anycubic,flashforge,elegoo}/manifest.json`: apenas candidatos/estrutura; **não módulos prontos**.

**Verificadores e infraestrutura QA criados:**
- `FILUNA/qa/test_bambu_a1_ssdp.cpp`
- `FILUNA/qa/test_bambu_a1_module.cpp`
- `FILUNA/qa/test_oem_overlay_selection.cpp`
- `FILUNA/qa/gcode/compare_oem_gcode.py`
- `FILUNA/qa/gcode/audit_oem_preset_parity.py`
- `FILUNA/qa/gcode/verify_bambu_a1_official_macros.py`
- `FILUNA/qa/gcode/test_oem_parity.py`
- `FILUNA/qa/gcode/test_bambu_speed_mapping.cpp`
- `FILUNA/qa/gcode/check_bambu_orca_upstream.py`
- `FILUNA/qa/gcode/test_bambu_orca_upstream.py`
- `FILUNA/qa/verify_bambu_a1_capabilities.py`
- `FILUNA/qa/virtual-lab/bambu_a1_side_by_side.py`
- Workflows `.github/workflows/filuna-oem-gcode-regression.yml`, `filuna-oem-upstream-watch.yml`, `filuna-oem-windows-qa.yml`. Nem todo workflow foi comprovado em CI; workflow Windows nova branch **ainda não validado como build PASS**.

## 4. O SERVIDOR, REPRODUÇÃO E CAMINHOS DE EVIDÊNCIA

Conector SentinelX, host_id `host_585dce92e0030ae5`, máquina **Ubuntu 26.04.1 LTS** (não CentOS).
- Git checkout preservado: `/var/tmp/filuna-qa-wine/qa-repo`; **working branch** `filuna/qa-gates-hardening-20261105`, NÃO confundir com PR #5. Para ler a candidata usar `git -C /var/tmp/filuna-qa-wine/qa-repo fetch origin filuna/oem-gcode-routing-qa-20261106` e `git show FETCH_HEAD:<arquivo>`, ou clone separado. NÃO fazer checkout/reset destrutivo no tree compartilhado e não sobrescrever estado de outras automações.
- Executável oficial pré-extraído: `/var/tmp/filuna-qa-wine/official-bambu-v0290170/unpacked` (Bambu Studio 0.29.0.170 em Wine).
- Fatiamento oficial anterior: `/var/tmp/filuna-qa-wine/qa-official-bambu-a1/out/plate_1.gcode`, resultado `.../result.json`.
- Fatiamento Filuna anterior: `/var/tmp/filuna-qa-wine/qa-four-bambu-a1/out/plate_1.gcode`.
- Cruzamento Bambu oficial + perfil Filuna: `/var/tmp/filuna-qa-wine/qa-cross-bambu-official-filuna/out/plate_1.gcode`.
- Cruzamento Filuna + perfil Bambu oficial: `/var/tmp/filuna-qa-wine/qa-cross-bambu-filuna-official/run.log`, falhou `Vendor bundle contains invalid presets`.
- Rede isolada já disponível: `10.77.0.0/24`, Bambu mock `10.77.0.12`, servidor `10.77.0.1`, UDP M-SEARCH 1990, resposta 2021.
- Scripts/fixtures temporários: `/var/tmp/filuna-qa-wine/bambu-ssdplab-20261106/`, `bambu-integrated-module-qa-20261106/`, `bambu-side-by-side-20261106/`, `a1_regressions_20261106/`.
- **Relatório comparativo concreto existente**: `/var/tmp/filuna-qa-wine/bambu-side-by-side-20261106/result.json`.
- Comparação de capacidades N2S: `.../bambu-side-by-side-20261106/N2S-capabilities-compare.json`.
- Não prometer preservação perpétua de `/var/tmp`; os textos/código do GitHub são a referência persistente. Recomenda-se anexar evidências JSON sem credenciais ao PR ou artifacts.
- Wine pode estar em `/usr/bin/wine64`; ausência de `/usr/bin/wine` não prova que Wine não está instalado.

## 5. RESULTADOS REALMENTE EXECUTADOS — NÃO CONFUNDIR PASS

**C++ e rede simulada:**
- `g++ -std=c++17 -Wall -Wextra -Werror`, teste nativo do parser SSDP Bambu: PASS; recebeu por UDP resposta do emulador com `DevModel.bambu.com: N2S`, `USN: 039LAB0000001`, host `10.77.0.12`.
- Teste da entrada C++ `A1Module` Dispositivos/Preparar: PASS, identidade/mesmo overlay; **teste isolado**, não binário Windows Filuna completo.
- Teste do candidato matemático `BambuSpeedMapping.hpp`: compilação + execução PASS isolado.
- Verificador dos cinco scripts G-code A1 oficiais vs os já presentes em Filuna: **5/5 PASS de conteúdo**; templates start/end, change filament, layer change, timelapse são IGUAIS. **Não duplicar macros iguais.**
- Unidade de comparador G-code: 5 casos PASS; verificador de upstream: 4 casos PASS.
- Simulador de protocolos anterior `network-results.json`: 14 PASS, 0 FAIL, 6 NOT RUN; **testa simuladores/protocolo, NÃO DeviceBridge Windows ou firmware físico**.
- Workflow `Filuna OEM G-code regression (non-release)`, run `42146262821`: sucesso em controles offline e compilação dos testes isolados. Workflow `Filuna virtual LAN protocol regression`, run `42146262724`: sucesso em emuladores. **Não equivale a novo Windows build**.

**G-code OFICIAL vs Filuna anteriores** (cubo `20mm_cube.obj`, resultados com camada 0,20mm):
| Métrica | Bambu Studio oficial | Filuna Orca |
|---|---:|---:|
| Tamanho G-code | 214953 bytes | 372272 bytes |
| SHA256 | `df954ca85031bc7b8f451db8b4f326243ce03e5045a46a58a6c9261a2aa723bf` | `f549436593d381d9a0a2118ba200084ffcf133f138a4259eade6c5dc95c62379` |
| G1 | 6429 | 6100 |
| G92 | 0 | 205 |
| M204 | 856 | 762 |
| Resultado | referência imutável | **MISMATCH**, NÃO homologado |

**Atenção:** arquivos anteriores foram gerados **ANTES** do hook Bambu recém-criado; compará-los de novo não comprova novo engine. A condição para o próximo teste é obter **NOVO G-code do Filuna Windows atualizado** e novamente o oficial com input e presets efetivos realmente equivalentes.

**Divergências encontradas na herança de presets:**
- Máquina `Bambu Lab A1 0.4 nozzle`: **6** campos efetivos divergentes.
- Filamento `Bambu PLA Basic @BBL A1`: **8** campos.
- Processo `0.20mm Standard @BBL A1`: **19** campos.
- `PrintConfig.cpp` Filuna ignora `smooth_coefficient`; Bambu possui opção e planner de suavização; `GCode.hpp/.cpp` Filuna não contém o mesmo `smooth_speed_discontinuity_area`, `mapping_speed` e `m_smooth_coefficient`. O candidato matemático sozinho NÃO reproduz o planner.
- `resources/printers/N2S.json` runtime Orca difere de Bambu oficial em **5 campos**: `print.support_ams_ext_mix_print`, `print.timelapse_slow_down`, `air_print_detection_position`, `filament_load_image`, `sn_prefix`. Não habilitar automaticamente por trocar arquivo.
- O teste cruzado Filuna+bundle oficial falhou com **`Vendor bundle contains invalid presets`**. Corrigir compatibilidade real de parser/config/engine antes de aceitar G-code.
- O último diff Orca upstream de `a620d804efbe74162c96ad29d755cdab2b823530` a `c78272f58338d12cb42e62afde767d2a08b4484c` alterou apenas 3 arquivos UI/docs (Gizmos + PR template), sem delta diretamente no engine; não mexer nos componentes Bambu por isso.

## 6. TAREFA IMEDIATA — DEVE SER IMPLEMENTAÇÃO + TESTE, NÃO OUTRO PLANO

**Prioridade P0, seguir nesta ordem:**
1. Atualizar HEAD da PR #5 e preservar histórico/branches concorrentes. Medir **tamanho exato por subpasta e total incremental** usando diff Git `git diff --numstat`/blobs/bytes, e **tamanho real compilado/instalador** apenas após build Windows. Distinguir modelo A1 de “linha Bambu completa”: atualmente **somente A1/N2S candidato; toda linha Bambu NÃO**.
2. Revisar linha de código em `src/slic3r/GUI/Plater.cpp::reslice` e `DevManager.cpp`, caminhos de include e CMake; disparar ou observar **build Windows completo** da branch PR, resolver erros reais. Workflow `filuna-oem-windows-qa.yml` não substitui resultado comprovado; não declarar que o novo módulo funciona no app se apenas g++ unitário passou.
3. **Incorporar de verdade os deltas Bambu Studio AGPL necessários ao planner Orca Filuna**, não só logger/selector: comparar códigos e configurações fonte por versão e licença; portar `smooth_coefficient` + `smooth_speed_discontinuity_area` e dependências corretas, arrumar `Vendor bundle contains invalid presets`, ajustar diferenças críticas dos presets herdados sem silenciar opções e preservar Orca como único motor. Usar módulo `fabricantes/bambu/` com interfaces estáveis; pode exigir build assinado de binário.
4. Produzir G-code **novo de Filuna atualizado** vs **Bambu Studio oficial imutável** da **mesma fixture, mesmo bico, camada, filamento, origem, mesa, orientação, macros, perfis efetivos**. Comparar geometria/trajetórias e G-code crítico além de hashes; diferenças devem ser examinadas por relevância, não forçar byte equality caso metadados/ordenamento não críticos divirjam. Não remover comandos essenciais à segurança.
5. Exercitar **adapter Filuna real**, não somente socket Python, com Bambu A1 virtual: SSDP N2S, MQTT/TLS autenticado, FTPS transfer, falha de credencial, desconexão, timeouts, upload sem impressão automática. Validar fallback de autenticação usando LAN Access Code fornecido pela impressora onde exigido. Sem firmware físico, manter gate de hardware NOT RUN.
6. Registrar/commit resultados e manifestos com upstream commit/hash/versão/ABI. Definir contratualmente as regras para **todos** os fabricantes sem declarar que A1 cobre toda Bambu, Elegoo, Flashforge ou Anycubic.
7. Responder ao proprietário com **tamanho final medido**, **cobertura exata**, **estado homologado e G-code real atual**. Se incompleto, fornecer números parciais e bloqueios concretos, nunca prometer continuar em background.

## 7. RESTRIÇÕES/SEGURANÇA/QUALIDADE
- Código OEM aberto pode ser reutilizado **conforme licença por arquivo/dependência**, preservando copyright e obrigações AGPL/GPL. Não copiar DLL de networking Bambu fechada, tokens, chaves, endpoints secretos ou plugin privado. `BambuStudio/src/slic3r/Utils/NetworkAgent.cpp` carrega biblioteca separada `BambuSource`; isso não autoriza redistribuí-la.
- Qualquer aplicação de firmware, impressão real ou conexão com equipamento físico exige condições de segurança/pareamento e testes explícitos. Emulador não substitui hardware.
- Preserve o Filuna compilado existente, sem instalador novo ou deploy desnecessário; updates por módulo ou binário assinado somente após QA e rollback.
- Não promova `MathCandidateOnly` para `ReadyForValidatedGCode` apenas porque houve chamada `reslice`.

## 8. COMANDO INICIAL PARA NOVA CONVERSA
“Continue a homologação Bambu A1 do novo Filuna Slicer no GitHub `abcollicelli-web/FilunaSlicer`, PR #5, branch `filuna/oem-gcode-routing-qa-20261106`. Leia primeiro `FILUNA/HANDOFF-MESTRE-BAMBU-A1-ORCA-OEM-2026-11-06.md` e todos os contratos relacionados. Use SentinelX Ubuntu `host_585dce92e0030ae5` e os testes em `/var/tmp/filuna-qa-wine/`. Não documente apenas: compile o novo Filuna, integre o módulo Bambu A1 ao motor G-code, faça novo fatiamento idêntico ao Bambu Studio oficial e compare; informe tamanho incremental do módulo, cobertura da linha Bambu e homologação. Não afirme PASS antes da prova.”

**STATUS CONCLUSIVO NO MOMENTO DESTE HANDOFF:** descoberta N2S no emulador e testes isolados nativos PASS; 5 scripts A1 iguais PASS; matriz OEM e presets divergentes; G-code Filuna vs oficial **MISMATCH**; build Windows atualizado, pipeline completo OEM, MQTT/TLS/FTPS nativo, impressão e toda linha Bambu **AINDA NÃO HOMOLOGADOS**. Nenhuma publicação/merge.


## DIAGNÓSTICO E DETERMINAÇÃO ATUAL DO PROPRIETÁRIO — MOTOR OEM NÃO ESTÁ INTEGRALMENTE ATIVO
O proprietário destacou corretamente: o hook atual `BambuA1Module::activate_for_prepare()` em `Plater::reslice()` **não substitui nem parametriza o planner/emissor do Orca**, apenas devolve `MathCandidateOnly`. O teste de módulo passou em C++ isolado, mas **não significa fatiamento realizado pelo módulo Bambu**. É obrigatório que a parte OEM específica, completa e compatível do Bambu Studio entre na execução real do motor compartilhado Orca, preservando apenas o núcleo comum que não diverge. Não basta carregar nome, JSON, log, perfil ou math helper isolado. Ao selecionar a impressora, a identidade/firmware devem vincular a unidade OEM; ao preparar e fatiar, a unidade OEM deve exercer controle **comprovável e instrumentado** sobre configuração, planner, pós-processamento, purga/multimaterial/macros e conexão pertinente. Se uma extensão depender de mudança do código Orca, portar e compilar a diferença no engine e registrar versão/licença/diff e testes. Não criar um segundo executável de fatiamento nem retirar funções comuns Orca.

**Novo ensaio experimental executado no Ubuntu/Wine nesta conversa:** fatiamento oficial com scripts A1 + presets efetivos achatados após herança e Bambu Studio 02.09.01.70, cubo 20 mm, gerou `/var/tmp/filuna-qa-wine/qa-a1-bambu-fully-resolved/out/plate_1.gcode` 343563 bytes, SHA256 `538c6cecf55163529ca9792fbc358ec7e77caa518a369d1225609adacd481588`, G1 6018 e tempo total estimado 19m19s, filamento density 1.26; Filuna anterior 372272 bytes e 19m53s, ou seja, **desvio 28709 bytes, 34s**, ainda não é teste do novo build OEM. Ensaios isolados em Bambu Studio oficial: `precise_outer_wall=1` gerou 354275 bytes (+10712 bytes e +273 G1), `accel_to_decel_enable=1` manteve 343563 bytes (mudou hash), `internal_solid_infill_pattern=monotonic` 343567 bytes. Essas variantes demonstram efeito de parâmetros, mas não provam qual é o único parâmetro culpado; diferenças de código e configuração efetiva permanecem. Verificar as variantes em `/var/tmp/filuna-qa-wine/qa-a1-bambu-fully-resolved/variants/`.

**Critério seguinte:** ligar o planner OEM real ao Orca compilado no Filuna; acrescentar trace obrigatória `model -> module version -> effective preset hash -> OEM planner selected -> slicing output hash`; compilar e fatiar novo 3MF/STL na mesma fixture contra Bambu original. Até isso: módulo Bambu completo **NÃO CARREGADO**, equivalência **FAIL**, homologação física **NOT RUN**. Não divulgar como módulo completo.


## REGRA PRIORITÁRIA MAIS RECENTE — 06/11/2026
O proprietário exigiu **100% do módulo OEM de REDE em Dispositivos e 100% do módulo OEM de FATIAMENTO em Preparar**, com o Orca apenas como núcleo comum único; são **dois módulos distintos, completos e selecionados automaticamente pelo dispositivo/firmware**, nunca fatiadores duplicados. A regra normativa integral está em `FILUNA/REGRA-PRIORITARIA-100-PORCENTO-MODULOS-OEM-REDE-FATIAR-2026-11-06.md`. **Prevalece sobre descrições anteriores de seleção ou módulos parciais.** A Bambu A1 atual NÃO está 100% integrada em nenhuma das etapas; não declarar conclusão por PASS isolado.
