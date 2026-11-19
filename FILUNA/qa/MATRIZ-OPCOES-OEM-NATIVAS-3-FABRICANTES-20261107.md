# Filuna — opções de configuração OEM ausentes no core e incompatibilidades reais (07/11/2026)

**Objetivo:** matriz de integração OEM nativa para Snapmaker, Anycubic e Flashforge, sem compilar motores de fabricantes nem anexar todos os presets ao instalador. Diagnóstico estático de fonte: nomes de opções declaradas explicitamente por chamadas `this->add("key", ...)` em `src/libslic3r/PrintConfig.cpp`, e não auditoria completa de todas as opções geradas por macros, CMake, código de rede, fórmulas ou ABI. **Não** promover hardware ou paridade com base nesta contagem.

## Proveniência imutável

- Core Filuna/Orca no branch do PR #6: blob SHA `507a2c9bee5b251e4bc30811cffad3142a0a8646`, 999 opções únicas declaradas explicitamente, sujeito a atualizações posteriores.
- Snapmaker `Snapmaker/OrcaSlicer` commit `bac6fd5c1e1c23ddf057cf6471ba4ed7ed621c4c`, blob `d41f4c0d8288f66b5cfff4af27863772401323da`, 805 opções; **53 opções OEM extras não encontradas na lista de definições core**.
- Anycubic `ANYCUBIC-3D/AnycubicSlicerNext` commit `6103ed8b511609658d00d0538cc7f0609cdb57da`, blob `498678ad29d38a5af5a7bb046ec4590996e049cc`, 746 opções; **5 extras**.
- Flashforge `FlashForge/Orca-Flashforge` tag 1.7.9 commit `70f2359ba8b4bc8ff9e54818a4bb1d05e56e7856`, blob `bd6bdb687d9e9670fec456d0503e011073224c5a`, 815 opções; **4 extras**.

### Diferenças específicas detectadas

**Snapmaker — 53 opções próprias** (comparação simples de nome): `graphic_effect_plate_temp`, `graphic_effect_plate_temp_initial_layer`, `filament_multi_colors`, `filament_colour_mode`, `thumb0`, `thumb1`, `tool_change_temprature_wait`, `filament_is_high_temperature`, `first_layer_travel_acceleration`, `first_layer_travel_jerk`, `sparse_infill_filament`, `silent_mode`, `filament_volume_type`, `filament_flow_support`, `process_flow_support`, `printer_flow_support`, `filament_grouping_mode`, `filament_flow_step_size`, `wall_filament`, `mixed_color_layer_height_a`, `mixed_color_layer_height_b`, `mixed_filament_gradient_mode`, `mixed_filament_height_lower_bound`, `mixed_filament_height_upper_bound`, `mixed_filament_advanced_dithering`, `mixed_filament_pointillism_pixel_size`, `mixed_filament_pointillism_line_gap`, `mixed_filament_component_bias_enabled`, `mixed_filament_surface_indentation`, `mixed_filament_region_collapse`, `mixed_filament_definitions`, `dithering_z_step_size`, `dithering_local_z_mode`, `dithering_local_z_whole_objects`, `dithering_local_z_infill`, `dithering_local_z_direct_multicolor`, `dithering_step_painted_zones_only`, `z_hop_when_prime`, `solid_infill_filament`, `delta_temperature`, `ramming_line_width_ratio`, `enable_change_pressure_when_wiping`, `ramming_pressure_advance_value`, `support_interface_min_area`, `tree_support_transition_layers`, `support_transition_perimeter`, `support_transition_speed`, `support_transition_flow_ratio`, `tree_support_adaptive_layer_height`, `prime_tower_brim_chamfer`, `prime_tower_brim_chamfer_max_width`, `wipe_tower_wall_gap`, `local_z_wipe_tower_purge_lines`.

**Anycubic — 5 extras**: `sparse_infill_filament`, `silent_mode`, `wall_filament`, `solid_infill_filament`, `tree_support_adaptive_layer_height`.

**Flashforge — 4 extras**: `sparse_infill_filament`, `silent_mode`, `wall_filament`, `solid_infill_filament`.

**Aviso Anycubic:** a fonte Git pinada acima **não corresponde comprovadamente** ao Debian 2.0.0.6 testado com 40 máquinas/191 processos/382 filamentos; os cinco nomes são uma lista da fonte Git **para revisão**, e não a lista certificada do binário publicado. Flashforge Linux 1.7.9 também não certifica Flash Studio 1.7.20 Windows. O core e as fontes têm históricos/ABI diferentes; existência de definição não implica comportamento idêntico.

## Incompatibilidades executadas no Filuna, além dessa contagem

- Anycubic Kobra 3 V2: `retraction_distances_when_cut=0` da configuração efetiva é rejeitado porque a definição atual Orca estabelece mínimo 10/máximo 18. **Ambos os códigos-fonte inspecionados definem esse intervalo**; resolver por seleção correta de perfil/semântica de máquina, checagem OEM e gate real, nunca trocar 0 arbitrariamente por 10.
- Flashforge AD5X: o motor-base gera 214.319 bytes com preset efetivo temporário, contra 209.677 bytes oficial; G-code funcional diferente mesmo com as mesmas 0,4 mm/PLA/0,20 mm. Requer portar diferenças de geração de G-code/macros/planner e provar equivalência real.
- Snapmaker U1: o placeholder `chamber_cooling_mode` do `machine_start_gcode` oficial **não aparece como uma opção declarada por `this->add` nem no PrintConfig.cpp do core nem no da própria Snapmaker**; portanto não inventar que basta adicionar uma definição. Investigar variáveis de macro, geração de contexto do placeholder e regras OEM em outros arquivos. Preservar falha de intermitência de trajetórias OEM v2.4.0/v2.4.1 como bloqueio independente; patch de guarda de acesso nulo não corrige aleatoriedade de percurso.

## Aceite para desenvolvimento

1. Mapear dependências C++ reais de cada opção OEM e macros, incluindo bibliotecas, scripts e ABI, classificando arquivos por licença; portar **apenas diferenças OEM necessárias** sobre motor Orca único sem duplicar executáveis ou embutir catálogos inteiros.
2. Resolver os perfis por fabricante/modelo/firmware/bico sob demanda com verificação de fonte/assinatura e compatibilidade, cache local, rollback e testes negativos; tipagem/desconhecimento não podem ser silenciosamente convertidos.
3. Rodar Filuna Windows real com saída G-code por modelo, paridade sem normalizar movimentos/temperaturas/purga e stress repetido; depois validar descoberta/rede/auth/ACK/câmera/AMS em simulador e app real.
4. Somente após todos os gates internos PASS, compilar o Filuna final leve e entrar no beta físico guiado pelo cliente. 3 impressões reais verificadas para classificação final do modelo no beta; sem impressão física antes.

**STATUS:** `AUDITORIA_ESTATICAS_OPCOES=PASS_LEITURA`; `PORT_NATIVE=NOT_RUN`; `FILUNA_GCODE_PARITY=FAIL_OR_BLOCKED`; `NETWORK_NATIVE=NOT_RUN`; `HOMOLOGACAO_INTERNA=0/3`. Fonte: logs [#42325032419](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42325032419), 100 stress OEM [Anycubic #42325230868](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42325230868) e [Flashforge #42325288710](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42325288710), falhas Snapmaker [#42324349445](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42324349445) e [#42321783683](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321783683).
