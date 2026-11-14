# Snapmaker U1 — investigação causal e condições para correção definitiva

**Estado 2026-11-07:** correção definitiva **NÃO COMPROVADA**; manter `U1_FOUR_TOOL_SAFETY=BLOCKED` e `OEM_FOUR_TOOL_STRESS_REPEATABILITY=FAIL`. Este documento não altera código, G-code OEM ou perfil oficial.

## Evidências internas
- Run [#42396616586](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42396616586): mesmo 3MF/presets/versão OEM, 18/20 trajetórias A e 2/20 B com diferenças funcionais em extrusão, velocidade e arcos; origem exata da não determinismo **não isolada**.
- Ambas variantes geraram bloqueio geométrico no mesmo XYZ 132.044/141.691/20.05, em linhas distintas; run [#42396347068](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42396347068) comprovou bloqueio Python+C++ apenas da variante predominante. Exigir regressão nativa da variante B.
- 3MF BBS oficial via CLI: `0xC0000005`; Core 3MF sintético aceito. Hipótese de renderização/thumbnail ou metadados, **não causa demonstrada**.

## Nova evidência externa, independente — pista de causa do deslocamento final
- Relato com comparação física e G-code Snapmaker Orca 2.4.0 versus 2.4.1: https://forum.snapmaker.com/t/snapmaker-orca-2-4-1-nozzle-scratches-prints-during-final-toolchange-works-in-2-4-0/43865 . Em 2.4.1, movimento horizontal para prime tower no toolchange final sem Z-lift; em 2.4.0, Z-lift explícito. Relato é evidência externa, não validação física Filuna.
- Análise pública aponta mudança de condição em `GCode.cpp`, `append_tcr2`, commit upstream `b626b4be6`, PR #937: https://forum.snapmaker.com/t/snapmaker-orca-2-4-1-nozzle-scratches-prints-during-final-toolchange-works-in-2-4-0/43865/9 . **Hipótese forte a verificar diretamente na fonte pinada e por teste diferencial**, não considerar causalidade demonstrada para a fixture Filuna sem reproduzir.
- Relato de crash de CLI Orca multicolor 3MF Windows `0xC0000005` independente: https://github.com/OrcaSlicer/OrcaSlicer/issues/14181 . Versão/ambiente diferentes, não prova de mesma causa.

## Plano de correção em fonte (sem adulterar G-code OEM)
1. Fazer diff de `append_tcr2` entre pins OEM 2.4.0 e 2.4.1 e identificar condição de elevação final; escrever teste unitário de estado de toolchange final com percurso e clearance esperado.
2. Se confirmada causalidade, corrigir **somente a lógica do gerador Filuna** no delta OEM licenciado, não o G-code de referência; preservar hashes originais e exigir segurança de ambos os caminhos.
3. Testar a fixture U1 4 ferramentas em 20+ execuções com controle de threads/seeds/ordem e rastreamento de diferenças; identificar primeiro ponto divergente e testar causa isolada. Não mascarar divergências E/F/G2/G3.
4. Investigar crash BBS 3MF com stack/minidump no Windows runner, matriz de importação com/sem metadados e thumbnails; só corrigir após reproduzir a causa.
5. Reconstruir GUI Windows após commit CMake `d7762d13`, executar testes nativos de bloqueio para A e B, provar G-code gerado pelo Filuna e paridade por mesma entrada/presets, e manter rede/ACK e físico separados.

## Gates
`U1_FINAL_PURGE_ROOT_CAUSE=HYPOTHESIS_EXTERNAL_NOT_YET_REPRODUCED`; `U1_FINAL_PURGE_FILUNA_SOURCE_FIX=NOT_RUN`; `OEM_FOUR_TOOL_STRESS_REPEATABILITY=FAIL`; `U1_ORIGINAL_BBS_3MF=FAIL`; `U1_FOUR_TOOL_SAFETY=BLOCKED`; `FILUNA_NATIVE_GCODE_PARITY=NOT_PROVEN`; `WINDOWS_FULL_GUI_BUILD=NOT_PROVEN`; `DEVICE_NETWORK_ACK=NOT_RUN`; `PHYSICAL_BETA=NOT_RUN`; `G4-I=BLOCKED`.

**Restrições:** branch QA DRAFT somente; sem merge, deploy, release, instalador, servidor, impressora física, Bambuddy ou impressão. CI verde de detecção de risco não é homologação.
