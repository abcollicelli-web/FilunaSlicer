# FILUNA — Recebimento do inventário enviado e execução de QA modal por feature

**Data:** 2026-11-06. **Repositório:** `abcollicelli-web/FilunaSlicer`; **PR #5**, branch `filuna/oem-gcode-routing-qa-20261106`.

## 1. O arquivo fornecido pelo proprietário
Nome recebido: `Filuna_Inventario_Modulos_Bambu_2026-11-06(1).txt` (282 linhas). Trata-se do inventário da rodada anterior, referenciado ao HEAD `cf6269efa08b6a057d990e7eb2309cbbbbd7964b`, **não de novo código-fonte Bambu ou uma listagem complementar de componentes a integrar**. O inventário lista arquivos já existentes e declara como pendentes: módulo completo de rede MQTT/TLS/FTPS autenticado, fatiamento OEM integrado ao planner Orca, compilação Windows atual e homologação física.

A leitura desse TXT não autoriza afirmar que o módulo completo tenha sido recebido/incorporado. NÃO duplicar perfis comuns, NÃO promover manifesto candidato para módulo completo e NÃO alterar produção.

## 2. Regras expressas do proprietário, mantidas
**Orca = único motor-base**, Filuna = única interface visual. Em **Dispositivos** a impressora escolhida/detectada carrega **100% do módulo funcional de rede do fabricante** e é vinculada. Em **Fatiar** a impressora cadastrada carrega automaticamente **100% das diferenças necessárias de fatiamento OEM** sobre a base Orca, com perfis efetivos, algoritmos, comandos de firmware e modelo/versão corretos. Não significa duplicar motores. Testes reais e números determinam a equivalência; o proprietário decide lançamento.

## 3. Código e evidência criados nesta rodada
Criado `FILUNA/qa/gcode/compare_bambu_modal_features.py`, parser modal G-code para `G90/G91`, `M82/M83`, `G92`, `G0/G1/G2/G3`, `M204` e rótulos `; FEATURE:`/`;TYPE:`. Mede por feature:
- Movimentos com extrusão vs deslocamentos (inclui comandos sem extrusão);
- Comprimento de filamento E positivo acumulado e distância XYZ linear;
- Comandos M204 e suas mudanças modais, alterações de avanço F;
- Retrações, E retraído, subidas puramente Z como indicador **não conclusivo** de z-hop;
- SHA dos G-codes e os campos presentes no HEADER_BLOCK.

**Execução no SentinelX Ubuntu `host_585dce92e0030ae5`: exit 0**, resultados gerados em `/var/tmp/filuna-qa-wine/bambu-17-models-qa/modal-features-20261106.json`.
Entradas imutáveis usadas no ensaio:
- Bambu Studio oficial 02.09.01.70, configurações herdadas e macros A1: `/var/tmp/filuna-qa-wine/qa-a1-bambu-fully-resolved/out/plate_1.gcode` (343563 bytes).
- Orca portátil 2.5.0-dev usando as mesmas configurações Bambu achatadas: `/var/tmp/filuna-qa-wine/bambu-17-models-qa/filuna-with-official-flat/plate_1.gcode` (373539 bytes).
- **Não são G-codes de novo build Filuna com engine OEM integral**.

### Resultado quantitativo relevante, por feature
| Feature | Extrusão Bambu/Orca | Deslocamento Bambu/Orca | Filamento E+ (mm) Bambu/Orca | M204 Bambu/Orca |
|---|---|---|---|---|
| Inner wall | 396 / 396 | 198 / 198 | 246.96 / 245.90 | 198 / 100 |
| Outer wall | 400 / 400 | 1190 / 1196 | 240.95 / 241.01 | 397 / 298 |
| Bottom surface | 108 / 108 | 6 / 6 | 26.71 / 26.72 | 2 / 3 |
| Internal solid infill | 665 / 677 | 30 / 86 | 139.48 / 140.15 | 10 / 32 |
| Sparse infill | 1933 / 1932 | 554 / 552 | 481.76 / 476.82 | 186 / 184 |
| Top surface | **70 / 70** | **72 / 140** | **29.91 / 30.50** | **3 / 138** |
| Bridge (Bambu) / Internal Bridge (Orca) | 112 / 112 | 6 / 6 | 39.06 / 38.56 | 2 / 2 |
| Floating vertical shell (só Bambu) | 12 / 0 | 2 / 0 | 2.09 / 0 | 2 / 0 |

Os G1 totais não são sinônimos de movimentos de extrusão: contam também viagens, retrações e comandos de velocidade. O aumento de comandos Orca no topo pode vir de viagens e alternâncias M204, não de maior número de linhas extrudadas. Em outras features os M204 do Orca são menores — por isso **NÃO** afirmar que Orca troca mais aceleração globalmente.

**Limitações e melhorias pendentes:** os rótulos Bridge/Internal Bridge precisam de normalização formal e `Floating vertical shell` pode corresponder a caminhos classificados diferentemente; comandos G2/G3 são identificados, mas distâncias de arcos não foram integradas; estimativa real de tempo por feature não está nos G-codes e requer dados do motor/simulador; Z-only positivo não prova z-hop; não recomputar cinemática de firmware apenas com M204. O parser é análise numérica independente e ainda necessita controles unitários com fixture fabricada.

## 4. Próxima implementação
1. Validar parser modal com fixtures unitárias: G90/G91, M82/M83, G92, M204, arcos, mudanças de feature, E negativo, F modal.
2. Normalizar taxonomia de features e calcular distância de arco (ou marcar explicitamente não modelado), distinguir viagens verdadeiras e comprimento E.
3. Usar os resultados para implementar camada de tradução de presets Bambu e testar na CLI Orca; identificar `precise_outer_wall`, `internal_solid_infill_pattern`, `accel_to_decel_enable`, `travel_jerk`, `smooth_coefficient` e planner, sem remover comandos essenciais.
4. Ligar módulo OEM completo à execução real `Plater/Print/GCode`, compilar Windows e fatiar novo projeto idêntico ao Bambu oficial. Exigir evidência de arquivo/hash/tempo.
5. Em Dispositivos, módulo Bambu de rede autenticada ainda incompleto; testar MQTT/TLS/FTPS no simulador e em hardware com credencial fornecida pelo cliente, mantendo segurança.
6. Atualizar manifesto e medições de pacote/instalador só após build e validação; manter PR em rascunho.

**Status desta rodada:** inventário recebido e reconciliado; script modal incorporado e executado no Ubuntu com resultados; nenhum componente OEM integral de rede ou planner incorporado com esta mensagem, nenhuma homologação concedida.
