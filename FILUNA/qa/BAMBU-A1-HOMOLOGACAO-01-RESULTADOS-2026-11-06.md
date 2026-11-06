# Bambu Lab A1 — relatório de homologação 01, 06/11/2026

**Escopo:** novo Filuna Slicer já compilado, usando laboratório existente Ubuntu 26.04.1 LTS/Wine (não CentOS). Não houve instalação, mudança de produção, impressão em hardware real nem integração OEM concluída nesta rodada.

## Regra de arquitetura, válida para todos os fabricantes

1. Descoberta em **Dispositivos** por interfaces/protocolos abertos e permitidos conforme modelo/firmware, sem presumir que porta aberta dispensa autenticação. Identificar fabricante, modelo, firmware, capacidades e dispositivo estável.
2. Usar primeiro recursos/protocolos disponíveis (por exemplo MQTT/TLS, SSDP, HTTP/WS, FTPS quando suportados) e cumprir autenticação exigida. **LAN Code somente quando os primeiros recursos forem insuficientes**, e se disponível/autorizado. O código pode autenticar o próprio MQTT, não necessariamente indicar outro transporte.
3. Registrar apenas dispositivos reais pareados; a lista **Imprimir** só mostra os dispositivos registrados do cliente, com status real.
4. **Preparar** mantém GUI Orca integrada; seleciona internamente módulo de compatibilidade reutilizável/licenciado por modelo e firmware, com vínculo executável ao pipeline de slicing, perfis efetivos, macros, multimaterial e transporte. Não invocar fatiador externo nem alterar aparência.
5. Seguir atualizações modulares verificadas por versão/ABI/assinatura/hash/rollback e manter Orca livre/offline. Módulo OEM não equivale automaticamente a hardware seguro; exigir testes específicos.

## Execução observada

- `/var/tmp/filuna-qa-wine/module-selection-test` reexecutado: `BBL-A1:filuna-bambu-a1:ABI=1:hardware-locked`. Seleção de metadados **PASS**, carregamento real/Preparar **NOT RUN**.
- `/var/tmp/filuna-qa-wine/vendor-registry-test`: `Bambu Lab A1 PARTIAL_SIMULATED`; não significa conexão real.
- `network_regression.test_bambu_discovery()` reexecutado com emulador na rede de laboratório `10.77.0.0/24`: **PASS SIMULATED**; resposta SSDP de A1 simulada em UDP 2021. MQTT/TLS 8883, FTPS 990, LAN Code, DeviceBridge no Wine e firmware físico **NOT RUN**. Nada foi enviado a impressoras reais.
- Comparador novo `FILUNA/qa/gcode/compare_oem_gcode.py`, commit `1cd83e84fb54ccffb8727d9eaecb48dec405a5a9`, recuperado do GitHub e **executado efetivamente no servidor**. O comparador não edita nenhum G-code e exige equivalência da sequência normalizada para um PASS preliminar. Ele não substitui inspeção de parâmetros herdados, geometria e firmware.

| Caso comparado | Execução | Resultado |
|---|---|---|
| Bambu Studio oficial × Filuna prévio | comparador exit 1 | **MISMATCH** |
| Bambu Studio oficial × Bambu Studio oficial usando presets Filuna | comparador exit 1 | **MISMATCH** |
| Controle positivo: Bambu oficial × mesmo arquivo | comparador exit 0 | **PRELIMINARY_MATCH**, não hardware PASS |
| Filuna com presets oficiais Bambu | fatiamento anterior abortou: `Vendor bundle contains invalid presets` | **FAIL de compatibilidade de preset** |

Arquivos oficiais e Filuna examinados sem modificação:

| Métrica | Oficial Bambu Studio | Filuna |
|---|---:|---:|
| Tamanho (bytes) | 214953 | 372272 |
| `G1` | 6429 | 6100 |
| `G92` | 0 | 205 |
| `M204` | 856 | 762 |
| Comentários de camada identificados | 102 | 104 |
| SHA256 | `df954ca85031bc7b8f451db8b4f326243ce03e5045a46a58a6c9261a2aa723bf` | `f549436593d381d9a0a2118ba200084ffcf133f138a4259eade6c5dc95c62379` |

No cruzamento de presets usando executável oficial, gerou-se arquivo de 215539 bytes (`G1=6454`, `G92=0`, `M204=856`): divergência bem menor que Filuna x oficial, mas ainda **MISMATCH**. O cruzamento inverso falhou por validação de bundles, não por impressora incompatível demonstrada.

## Inspeção das fontes

- Repositório público `bambulab/BambuStudio`: `src/slic3r/Utils/NetworkAgent.cpp` carrega biblioteca de rede separada (`BambuSource`); componentes fechados não devem ser copiados presumindo licença AGPL. Usar recursos LAN permitidos e implementação própria quando indispensável.
- Fork Filuna: já possui `src/slic3r/Utils/NetworkAgent.cpp`, `BBLNetworkPlugin.cpp`, `NetworkAgentFactory.cpp`, `DevManager.cpp` e perfis `resources/profiles/BBL`. O `FilunaModuleSelection.hpp` declara expressamente que o seletor **não executa binários de terceiros nem faz loader**. Seleção por modelo é metadado; vínculo verdadeiro com Preparar requer implementação/teste.

## Próximas correções de engenharia (sem inventar PASS)

1. Auditar o código OEM AGPL por arquivo/commit e compatibilidade com Orca pinado; separar motor/macros/perfis do transporte proprietário.
2. Corrigir o erro de `Vendor bundle contains invalid presets` com matriz de herança/versão, sem alterar o G-code oficial de referência.
3. Criar ligação instrumentada Dispositivos → registro → Preparar → engine/configuração Bambu, validando execução do módulo no processo Filuna.
4. Simular MQTT/TLS/FTPS e exceções de autenticação, depois testar através do **Filuna DeviceBridge real** em Wine. Sem impressão automática.
5. Reexecutar a mesma fixture (mesmo STL/3MF, geometria, transformações, bico, filamento, processos, versões de motores/presets efetivos) no binário oficial imutável e no Filuna instrumentado; comparar G-code, movimento, extrusão, temperaturas, macros, purga e tempo.
6. Windows real e Bambu A1 com firmware real só contam após QA e aprovação, mantendo gates bloqueados antes disso.

**Estado final desta rodada:** política universal atualizada, testes de seleção e descoberta simulada passaram, comparador regressivo testado com controle positivo e mismatches reais. **Módulo completo Bambu ainda NÃO incorporado/carregado em Preparar; equivalência oficial NÃO demonstrada; sem homologação física.**


## Rodada adicional: auditoria efetiva dos presets e gate automático

Executado no Ubuntu via SentinelX: `FILUNA/qa/gcode/audit_oem_preset_parity.py` do commit `264dae727b60229711869633ad521a43f703683d`, contra os diretórios `resources/profiles/BBL` do Bambu oficial e do Filuna. O teste **terminou exit 1**, corretamente, após resolver `inherits` de máquina, filamento e processo:

| Arquivo/preset | Valores efetivos divergentes | Resultado |
|---|---:|---|
| Bambu Lab A1 0.4 nozzle | 6 | MISMATCH |
| Bambu PLA Basic @BBL A1 | 8 | MISMATCH |
| 0.20mm Standard @BBL A1 | 19 | MISMATCH |

Divergências influentes incluem `default_wipe_tower_pos`, `extruder_clearance_radius`, `use_3mf`, identidade do filamento, `filament_pre_heating_enabled`, `adaptive_layer_height`, `reduce_infill_retraction_mode`, `smooth_coefficient` e parâmetros `prime_tower_*`. Os arquivos de referência oficiais **não foram modificados**.

O Bambu Studio oficial pertence a uma linha de código/versão diferente do fork Orca fixado. Alinhar arquivos JSON sem portar opções/algoritmos correspondentes não garante correspondência do G-code; a mensagem anterior `Vendor bundle contains invalid presets` deve ser tratada como bloqueio real de compatibilidade do parser/motor. O caminho correto é mapear cada opção incompatível para sua implementação específica e testar no pipeline, em vez de inserir valores silenciosamente ou falsificar equivalência.

**Condição de aceite do módulo:** um trace no processo Filuna deve demonstrar: máquina registrada A1 → módulo implementado e carregado automaticamente em Preparar → parâmetros efetivos resolvidos → execução de código de slicing compatível e versão registrada → comparação de G-code oficial e Filuna sem modificação do oficial. Nenhuma dessas provas de execução end-to-end foi concluída. Simulador SSDP PASS não prova módulo carregado nem conexão MQTT/TLS.


## Resultado de testes automatizados após a auditoria

Foi criado `FILUNA/qa/gcode/test_oem_parity.py`, commit `2d40bf0bfcda78057823a6d4fd2fcd6f33a041ee`, e EXECUTADO por `python3 ... -v` no host Ubuntu: **5 testes unitários PASS, 0 FAIL** (controle positivo de arquivo idêntico, diferença de macro crítica, preset ausente, ciclo de herança e diferença herdada). Também foi criado workflow `.github/workflows/filuna-oem-gcode-regression.yml`, commit `4d75af8600cdb9c95bb71191bc5f9651ce196c1c`. Estes PASS comprovam somente o comparador/gate; **não** carregamento real do módulo em Preparar, binário OEM incorporado, MQTT/TLS ou print físico. G-code Bambu A1 oficial x Filuna permanece **MISMATCH**.

**Bloqueio técnico para portar módulo completo:** as duas árvores de perfil e seus algoritmos executam versões distintas de código. O Filuna rejeita parte do bundle Bambu oficial, e dezenas de opções efetivas divergem. A integração completa exige revisão e port dos componentes AGPL reutilizáveis do engine por arquivo, suporte do parser para essas opções, wiring no fluxo Preparar e build Windows/Wine reproduzível antes da homologação. Nunca interpretar simples inclusão de JSON ou seleção `filuna-bambu-a1` como conclusão.
