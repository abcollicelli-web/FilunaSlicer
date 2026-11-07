# Flashforge AD5X — divergência geométrica real e mapa para correção nativa

**Data:** 07/11/2026. **Estado:** G-code do Filuna `FAIL_REAL_GCODE_PARITY`. Não homologado. Nenhum firmware, impressora, binário OEM, motor completo ou instalador foi modificado.

## Evidência reexecutada, sem simulação de paridade

- Runner [#42338655066](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42338655066): **SUCCESS como diagnóstico**, 8/8 testes negativos/positivos PASS.
- Fonte OEM imutável: FlashForge/Orca-Flashforge **1.7.9**, commit `70f2359ba8b4bc8ff9e54818a4bb1d05e56e7856`. Artefato G-code original [#42321095370](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321095370).
- Filuna utilizado: **portátil Windows PRÉ-integração**, originário de [#42102469888](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42102469888), G-code coletado em [#42336460665](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42336460665). **Não é um teste de versão contendo os novos patches OEM C++.**
- Artefato JSON do exame real: `filuna-flashforge-ad5x-motion-delta-root-cause-evidence`, ID `13560224300`. Algoritmo de auditoria `lab/flashforge_motion_parity_probe.py`; 8 casos de proteção em `lab/test_flashforge_motion_parity_probe.py`. Workflow `.github/workflows/filuna-flashforge-motion-delta-reference.yml`.

| Medida observada | OEM | Filuna pré-OEM |
|---|---:|---:|
| SHA256 | `68ed51e6e830ed7ad1d2159a26640b4d54d046f857851f3ad9054b3d045063d6` | `254904898873768d77bca7f2c03387c92fe6b8ac308120d330fcdf932c65cf70` |
| Bytes G-code | 209.677 | 214.319 |
| Movimentos G0–G3 | 5.759 | 5.753 |
| Blocos geométricos divergentes | colspan | **534** |
| Subsequência de movimentos com mesma geometria | colspan | **3.265** |
| Mesmo XYZ porém E diferente | colspan | **182 casos** |
| Mesmo XYZ porém F diferente | colspan | **374 casos** |

**Primeiro movimento geométrico divergente na camada 2**:
- Oficial: `G1 X119.191 Y117.103 F30000`
- Filuna: `G1 X119.191 Y117.317 F30000`

Há variação de posicionamento de segmentos internos nas camadas 2 e 3, não só de ordenação ou cabeçalho; camada 1 usa as mesmas coordenadas de movimentos principais, mas a extrusão também pode divergir. Não normalizar G0–G3, E, F, X, Y, Z, comandos de ventilador, purga ou aceleração.

## Hipótese técnica concreta a revisar antes de portar

Ao comparar as fontes fixadas `src/libslic3r/Fill/FillRectilinear.cpp`, função `FillRectilinear::fill_surface_by_lines`, o caminho do bounding box usado quando `has_consistent_pattern()` difere:

- **FlashForge/Orca-Flashforge**, blob `59c69a9d321042fef7a8a38a64df32a6006da8f9`: usa `this->extended_object_bounding_box()` diretamente no padrão consistente.
- **Filuna/Orca**, blob `142afbae1a8ea3cdbd04d9507a8eb92415b35ebd`: aplica transformação relativa à rotação do centro do bounding box antes de alinhar as linhas.

Essa diferença é uma **candidata a explicar parte das coordenadas XY**; ainda **NÃO é causa provada** sem executar ambas as variantes do mesmo algoritmo e correlacionar camadas/traçados. O caminho de preenchimento `FillGrid::fill_surface` e gerador de G-code/planejador também mudaram entre as versões e exigem auditoria. Não aplicar reversão genérica: quebraria os caminhos de outros fabricantes no motor Orca compartilhado.

## Gate de implementação correto

1. Incorporar apenas deltas OEM após identificar a chamada que realmente gerou o primeiro movimento distinto. Usar seleção explícita por **módulo OEM Flashforge verificado** e versão; não por nome de impressora hardcoded nem por um `G-code` pós-processado.
2. Capturar para os mesmos STL/3MF e perfis A/B: polígonos, `FillParams`, bounding box rotacionado, perímetros, fill layer 2/3, otimização de viagens, ordem de extrusão e `GCodeWriter`, com SHA de entrada. Demonstrar antes/depois e que fabricantes sem o módulo continuam sem regressão.
3. Confirmar se o caminho de ancoragem é causal; se não for, **não** portar a diferença. Investigar funções de Fill.cpp/FillRectilinear e `GCode.cpp` comparando os commits pinados. Manter curso de validação de macros M73, velocidade, extrusão e ventiladores.
4. Só após o código OEM **real** estar ligado ao Filuna, executar no **binário final** os G-code oficiais equivalentes sob testes negativos e estresse, modelos AD5X e AD5M, perfis dinâmicos, depois protocolo nativo simulado/segurança/ACK. A validação física permanece exclusivamente para o beta vigiado.

**Estado operacional:** `OEM_OFFICIAL_AB=PASS`; `FLASHFORGE_NATIVE_ENGINE_PARITY=FAIL`; `FLASHFORGE_SOURCE_CAUSAL_FIX=NOT_PROVEN`; `DEVICE_NETWORK_NATIVE=NOT_RUN`; `HOMOLOGACAO_INTERNA=BLOCKED`. Não publicar nem marcar correção funcional completa por SUCCESS do analisador.


## Correção de compatibilidade nativa candidata — wall_direction=auto (07/11/2026)

Auditoria adicional feita sobre os **blocos CONFIG preservados no G-code real**: o original Flashforge AD5X traz `wall_direction = auto` e o Filuna Windows pré-OEM traz `wall_direction = ccw`. A fonte FlashForge 1.7.9 implementa `WallDirection::Auto`, ao contrário do Orca base que só possuía CCW/CW. O modo é uma **diferença de semântica do core**, não uma linha de data a ser ignorada. Outros campos efetivos distintos observados nos blocos de configuração foram `filament_change_length`, `flush_volumes_matrix` e `wipe_tower_type`; `enable_arc_fitting` tem serialização distinta (`1` vs `enabled`) e exige verificação semântica antes de ser tratado como mismatch funcional.

**Mudanças nesta branch de QA:**
- `src/libslic3r/PrintConfig.hpp`: acrescentou `WallDirection::Auto` **após** os dois valores originais para preservar a numeração existente.
- `src/libslic3r/PrintConfig.cpp`: reconhece e apresenta o valor explícito `"auto"`; **default global permanece CounterClockwise**, para não alterar projetos Orca e outras marcas.
- `src/libslic3r/PerimeterGenerator.cpp`: o modo Auto usa laços CCW inicialmente e reversão condicional de sobreposição, somente quando escolhido explicitamente; os modos CCW/CW mantêm exatamente o ramo original. Inclui caminho de perímetro clássico e Arachne.
- `lab/test_oem_native_source_contract.py`: três testes adicionais para reconhecimento, isolamento do modo opt-in e bloqueio de homologação, sem mascarar diferenças de G-code.
- [CI #42339189237](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42339189237): **12/12 testes de contrato de fonte PASS**. Isso **NÃO compila** nem executa o novo motor; não prova que a opção Auto conserte as 534 diferenças geométricas, E/F, M73, ventiladores ou os perfis sob demanda.

**Dependência indispensável ainda ausente:** os dados JSON OEM efetivos podem omitir `wall_direction` porque o próprio executável OEM fornece o valor default Auto. O app Filuna precisará carregar esse **default original da fonte/verificação OEM no módulo sob demanda**, sem inventar compatibilidade por nome de impressora e sem alterar arquivos oficiais. O executor Filuna pré-OEM ainda escolherá CCW. É **proibido** marcar paridade só por registrar Auto no schema.

**Correção de hipótese:** a diferença estática de bounding box em `FillRectilinear::fill_surface_by_lines` mencionada acima **não está causalmente comprovada** para AD5X; `FillGrid` não habilita automaticamente o caminho `has_consistent_pattern()`. Não reverter genericamente código Orca por essa hipótese.

**Gates após patch:** `FLASHFORGE_AUTO_WALL_SCHEMA=SOURCE_PATCH_PASS`, `FLASHFORGE_DEFAULT_OEM_SELECTION=NOT_INTEGRATED`, `FLASHFORGE_FILUNA_GCODE_GENERATED_WITH_PATCH=NOT_RUN`, `FLASHFORGE_GCODE_PARITY=FAIL_NOT_READY`, `OEM_NATIVE_DEVICE=NOT_RUN`, `INTERNAL_HOMOLOGATION=BLOCKED`. Não gerar build intermediário, instalador, release ou impressão física.


## Verificação de configuração embutida nos dois G-codes reais (CI 07/11/2026)

[Workflow #42339362945](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42339362945) **SUCCESS**, com 10/10 testes do comparador passando. Artefato JSON `filuna-flashforge-ad5x-motion-delta-root-cause-evidence` (ID 13560627477) inspecionado. O teste lê apenas os intervalos `; CONFIG_BLOCK_START` até `; CONFIG_BLOCK_END`, sem reescrever G-code.

Foram **580 campos de configuração em comum**, com **cinco diferenças serializadas detectadas**:

| Chave | Oficial Flashforge | Filuna pré-OEM |
| --- | --- | --- |
| `wall_direction` | `auto` | `ccw` |
| `wipe_tower_type` | `type1` | `type2` |
| `filament_change_length` | `0` | `10` |
| `flush_volumes_matrix` | matriz 4×4, diagonais 0 e restantes 280 | `0` |
| `enable_arc_fitting` | `1` | `enabled` |

**Interpretação:** a primeira diferença é uma incompatibilidade real de escolha de default e esquema do motor. O patch C++ de `wall_direction=auto` nesta branch permite o modo **apenas quando explicitamente escolhido**, mas ainda **não implementa** a origem/default assinado `auto` do OEM no mecanismo de perfis sob demanda; portanto não é legítimo afirmar que o Filuna agora produz a mesma trajetória. A diferença `enable_arc_fitting` pode ser somente formato de serialização e requer verificação semântica. Não assumir que opções de purga/torre são irrelevantes sem testar o contexto. A quantidade de 534 blocos geométricos distintos não foi reduzida pelo patch, pois o teste continua sobre **binário anterior imutável**.

**Estado:** `FLASHFORGE_GCODE_PARITY=FAIL`, `FLASHFORGE_AUTO_SOURCE_CANDIDATE=PASS_STATIC`, `FLASHFORGE_NATIVE_INTEGRATION=NOT_RUN`, `HOMOLOGACAO_INTERNA=BLOCKED`.
