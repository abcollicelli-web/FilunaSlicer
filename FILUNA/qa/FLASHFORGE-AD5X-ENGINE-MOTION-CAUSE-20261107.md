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
