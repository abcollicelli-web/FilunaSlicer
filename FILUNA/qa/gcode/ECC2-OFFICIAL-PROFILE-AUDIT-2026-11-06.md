# Centauri Carbon 2 — auditoria de perfis oficiais vs Filuna

Data: 2026-11-06. **Resultado: PARTIAL_PROFILE_AUDIT — não é homologação de G-code nem hardware.**

## Fontes e proveniência

- Fabricante: [elegooofficial/ElegooSlicer](https://github.com/elegooofficial/ElegooSlicer), commit fixado `2d507e39a96ab9562b35d06b23ef5df8fe613297` (23/09/2026).
- Filuna: branch `filuna/qa-gates-hardening-20261105`, arquivos `resources/profiles/Elegoo/`.
- Equipamento: Elegoo Centauri Carbon 2, bico 0,4 mm, perfil 0,20 mm Standard, filamento Elegoo PLA.
- Arquivos verificados diretamente via GitHub:
  1. `machine/ECC2/Elegoo Centauri Carbon 2 0.4 nozzle.json`
  2. `process/ECC2/0.20mm Standard @Elegoo CC2 0.4 nozzle.json`
  3. `filament/ECC2/Elegoo PLA @ECC2.json`

## Divergências observadas

| Perfil | Chave | Oficial | Filuna | Classificação |
|---|---|---|---|---|
| Máquina | `setting_id` | `ECC204` | `hjSig28o75Oxgd7J` | Identidade/compatibilidade a revisar |
| Máquina | `machine_load_filament_time` | `51` | `29` | **Pendente de análise de purga/troca** |
| Máquina | `machine_unload_filament_time` | `0` | `28` | **Pendente de análise de purga/troca** |
| Máquina | `default_bed_type` | `4` | ausente | Pendente: herança e escolha da mesa |
| Máquina | `support_mms` | `1` | ausente | Pendente: suporte CANVAS |
| Máquina | `support_multi_filament` | ausente | `1` | Pendente: semântica e equivalência |
| Processo | `setting_id` | `PECC204020` | `pjHwBoHNTMAhLSbk` | Identidade |
| Filamento | `setting_id` | `EPLAECC2` | `49fzd1JgB0epXI3n` | Identidade |

Todos os demais campos JSON desses três arquivos comparados coincidiram literalmente; isso **não prova** equivalência de configuração efetiva após herança, geração de G-code ou firmware.

## G-code e protocolo oficial a preservar

O perfil oficial da CC2 usa `gcode_flavor=klipper`, `host_type=elegoolink`, volume `256 × 256 × 256`, exclusão de mesa `246x0–256x20`, `M6211` para troca de filamento e comandos `G180`, `BED_MESH_CALIBRATE`, `SET_PRINT_STATS_INFO` em rotinas específicas. Estes comandos devem ser interpretados como **comandos específicos da Elegoo**, não eliminados por um comparador genérico.

## Critério de avanço

1. Verificar herança `fdm_elegoo_3dp_001_common` e mapeamento de `support_mms`/`support_multi_filament` antes de alterar parâmetros.
2. Obter ou executar binário **oficial ElegooSlicer** com a mesma versão de perfis, em ambiente isolado, e gerar um G-code independente para o mesmo modelo.
3. Executar o **Filuna** com a configuração efetiva correspondente, comparar trajetórias, temperaturas, velocidades, purga, CANVAS, start/end, comandos proprietários e limites; comparar simulação visual e eventos por camada.
4. Classificar como `PARTIAL_SIMULATED` quando passar e guardar diferenças explicadas. Ensaios reais de firmware, CANVAS, sensores, aquecimento e transferência permanecem para homologação final.
5. **Não reutilizar** o G-code Bambu P1S já gerado como prova da Elegoo. **Não afirmar** que os G-codes oficiais e Filuna já foram comparados; por enquanto somente os perfis JSON foram comparados.

## Fontes externas

- [Perfil oficial da CC2 0,4 mm](https://github.com/elegooofficial/ElegooSlicer/blob/2d507e39a96ab9562b35d06b23ef5df8fe613297/resources/profiles/Elegoo/machine/ECC2/Elegoo%20Centauri%20Carbon%202%200.4%20nozzle.json)
- [Manual oficial ElegooSlicer / Centauri Carbon 2](https://www.elegoo.com/pages/download?c1=3d-printers-slicing-softwares)
