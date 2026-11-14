# Flashforge AD5X 0,4 — incompatibilidades reais do motor Filuna versus Flash Studio 1.7.9

**Data:** 07/11/2026. **Contexto:** comparação entre fatiador oficial Linux Flash Studio 1.7.9 em AppImage, SHA pinado no laboratório, e **portátil Filuna pré-OEM de 05/11**. Todos os resultados são experimentos sem hardware; não publicar nem homologar marca/modelo.

## Evidências de referência

- [Oficial Flashforge original #42321095370](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321095370) — G-code `ad5x_04/run-A/plate_1.gcode`, 209.677 bytes; 8.905 linhas.
- [Diferença do Filuna pré-OEM #42338506954](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42338506954) — 214.319 bytes; 9.048 linhas, 5.753 movimentos versus 5.759 movimentos oficiais; diferença de velocidade, extrusão, posicionamento e controle. `mismatching_layers=100`.
- [Auditor adicional #42338797102](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42338797102) foi inicialmente bloqueado pelo registro repetido `wipe_tower_x=15.000` e `wipe_tower_x=15`, presente **nos dois** G-codes. O parser foi corrigido para armazenar e comparar todas as ocorrências (`292a661`, teste `86e796c`); consultar a execução subsequente para o resultado definitivo.
- O manifesto mínimo de modelo do laboratório `lab/oem_model_scoped_manifest_probe.py` carrega o preset oficial preservado como `system` e gera G-code no Filuna pré-OEM, mas não resolve a paridade funcional.

## Diferenças observadas no bloco de configuração efetivamente exportado

O **bloco completo do G-code**, mais abrangente que os três presets efetivos parciais, apresenta:

| Chave | Flash Studio 1.7.9 | Filuna base 2.5.0-dev |
| --- | --- | --- |
| `wall_direction` | `auto` | `ccw` |
| `filament_change_length` | `0` | `10` |
| `wipe_tower_type` | `type1` | `type2` |
| `flush_volumes_matrix` | matriz 4×4 com valor cruzado 280 | `0` |
| `enable_arc_fitting` | `1` | `enabled` |

O último par possui **representação diferente**, sem prova nesta etapa de diferença física. Há também **4 chaves do oficial sem equivalente exportado no Filuna**: `wall_filament`, `sparse_infill_filament`, `solid_infill_filament`, `silent_mode`. O Filuna exporta opções novas ausentes no oficial. Esses resultados são um mapa de discrepâncias, não causalidade provada para cada movimento divergente.

## Causa estrutural de importação — modo Auto perdido

Na fonte pública pinada [FlashForge/Orca-Flashforge @70f2359](https://github.com/FlashForge/Orca-Flashforge/tree/70f2359ba8b4bc8ff9e54818a4bb1d05e56e7856):

- `src/libslic3r/PrintConfig.hpp`: `WallDirection` inclui `Auto`; `PrintConfig.cpp` define o default `WallDirection::Auto`.
- `src/libslic3r/PerimeterGenerator.cpp` usa `WallDirection::Auto` para decidir a orientação e a detecção de contornos inclinados.

No **Filuna atual derivado do Orca 2.5-dev**:

- `PrintConfig.hpp` removeu `Auto`, `PrintConfig.cpp` default virou `CounterClockwise`, e a migração `wall_direction=auto` é mapeada para `ccw`.
- `PerimeterGenerator.cpp` foi reestruturado, incluindo direção de contornos/furos, lógica de `overhang_reverse` e ordem de extrusão. Logo, **não basta** reintroduzir uma opção de interface ou forçar `wall_direction=auto` em JSON sem portar a semântica e testar regressão.

**Impacto de engenharia:** o Filuna atualmente **não representa fielmente** uma opção oficial necessária para revisão da paridade. O porte deve ocorrer sobre o único core Orca, isolando os deltas compatíveis por fabricante/perfil e sem trocar a direção dos usuários atuais. Não tornar `Auto` um default global nem deslocar IDs da enumeração de forma incompatível. Requerem-se testes C++ direcionados de orientação de contornos/furos, reversão em camadas, velocidade/extrusão, portas/câmera/IFS em outro gate, e saída G-code real repetida do Filuna atualizado.

**Critérios não flexibilizados:** nunca normalizar `G1`, `E`, velocidade, ventoinha, aceleração, torre, `M73` ou diferenças de posição para obter PASS. `GCODE_PARITY=FAIL`, `NATIVE_MODULE=NOT_INTEGRATED`, `DEVICE_NETWORK=NOT_RUN`, `BETA_FISICA=NOT_RUN`. Sem builds intermediários/instalador/merge/release. **Bambuddy não faz parte desta validação.**

## Verificação final do auditor — runner Windows #42338941059

- [Workflow #42338941059](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42338941059) **SUCCESS** como QA; artefato `filuna-existing-windows-base-versus-3-official-oem-gcodes` ID `13561025608`.
- Corrigida a leitura de opções exportadas em duplicata, preservando a sequência exata `wipe_tower_x=[\"15.000\",\"15\"]` sem descartar nada.
- Auditor de blocos completos: `BLOCKED_EFFECTIVE_DEFAULTS_CONFLICT`, **584** chaves no OEM e **728** no Filuna; **5** valores compartilhados diferentes, **4** chaves apenas do OEM e **148** apenas do Filuna.
- As cinco diferenças são `wall_direction` (auto ≠ ccw), `filament_change_length` (0 ≠ 10), `flush_volumes_matrix` (matriz 4×4 ≠ 0), `wipe_tower_type` (type1 ≠ type2), `enable_arc_fitting` (1 versus enabled, sem afirmar semântica diferente). Nenhum ajuste artificial aplicado.
- Valor `auto` consta como opção e default na fonte Flashforge pinada, enquanto o core novo suprime o modo Auto e o migra para ccw. Isso prova **incompatibilidade de esquema/semântica**, não prova, isoladamente, qual mudança explica cada movimento divergente.
- `NATIVE_FILUNA_GCODE_PARITY=FAIL/NOT_PROVEN`; versão Filuna com deltas nativos ainda não foi produzida nem executada. Requer implementação de compatibilidade real com revisão de ABI, regressoes e múltiplos modelos antes de permitir avanço.
