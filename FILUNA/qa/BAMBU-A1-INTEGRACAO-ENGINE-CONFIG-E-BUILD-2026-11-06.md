# FILUNA — A1: integração executada de configuração no motor e testes (06/11/2026)

**PR #5, branch:** `filuna/oem-gcode-routing-qa-20261106`. **Estado:** alterações de fonte publicadas em branch de QA, sem merge e sem release.

## Decisão operacional
O proprietário determinou: Orca como motor base ÚNICO; Dispositivos carrega 100% da rede do fabricante, vincula impressora; Fatiar usa a impressora cadastrada e executa 100% das funções OEM de slicing necessárias. As diferenças OEM devem ser materialmente executadas pelo motor, não apenas apontadas em JSON nem registradas em log. A regra ainda não está completamente implementada para Bambu, nem para a linha toda; reportar status com fidelidade.

## Código incorporado nesta rodada
1. `FILUNA/fabricantes/bambu/engine/BambuA1ConfigOverlay.hpp`: função `apply_a1_slicing_defaults(DynamicPrintConfig&)`, faz tradução de DEFAULTS de Orca para Bambu A1 no config efetivo, **não altera presets persistentes**. Traduções condicionais somente se eram os defaults Orca: `precise_outer_wall` verdadeiro→falso; `accel_to_decel_enable` verdadeiro→falso; `internal_solid_infill_pattern` ipMonotonic→ipZigZag; `travel_jerk` 12→9. Preserva valores personalizados que não coincidam com esses defaults. Não contém planner completo Bambu.
2. `src/libslic3r/PrintConfig.cpp`: opção **internal_solid_infill_pattern** agora permite valor `zigzag` (enum `ipZigZag`, existente no Orca mas antes excluído da lista válida para esta opção). Antes, CLI com `internal_solid_infill_pattern=zigzag` falhou `invalid value zigzag`.
3. `src/slic3r/GUI/Plater.cpp::priv::update_background_process`: aplica tradução OEM nos dois caminhos `background_process.apply(...)`, inclusive multifilamento, **somente** quando a impressora selecionada em Dispositivos é Bambu A1 (`N2S/BBL-A1/Bambu Lab A1`) e o perfil de impressão atual é Bambu Lab A1. Esta é uma integração no ponto REAL de entrega de `DynamicPrintConfig` ao motor Orca, não apenas um logger em `reslice()`.
4. `src/slic3r/GUI/Plater.cpp::reslice()`: antes de fatiar, se a impressora cadastrada selecionada é A1, procura preset correspondente ao modelo e nozzle via `get_printer_preset(device)`, seleciona com `update_objects_position_when_select_preset`/tab de impressora, e bloqueia caso ausente ou recusado. Não substitui interface nem instala slicer Bambu à parte.
5. `.github/workflows/filuna-oem-gcode-regression.yml`: expandido para testar mudanças em `src/libslic3r/PrintConfig.cpp` e `src/slic3r/GUI/Plater.cpp` e invocar compilação Windows completa reutilizando `build_check_cache.yml`, com `force-build=true`. O resultado de jobs offline **NÃO** é resultado do Windows nativo.

**Revisão de diff:** `git diff --check` branch vs base nos arquivos citados: PASS no servidor Ubuntu SentinelX.

## Testes numéricos reais de fatiamento executados
Fixture: `tests/data/20mm_cube.obj` no Linux/Wine, bico A1 0.4, Bambu PLA Basic e processo 0.20 Standard; JSON máquina/processo/filamento achatados com herança e scripts A1. **Apenas o Bambu Studio oficial e o Orca portátil anterior à compilação da branch puderam ser executados em CLI nesta rodada.**

| Caso | G-code bytes | G1 | M204 | Tempo total |
|---|---:|---:|---:|---|
| Bambu Studio 02.09.01.70 oficial + presets herdados | 343563 | 6018 | 803 | 19m19s |
| Orca portátil 2.5.0-dev + mesmos presets base | 373539 | 6170 | 762 | 19m55s |
| Orca portátil anterior + controles traduzidos de forma aproximada | 372034 | 6202 | 772 | 19m53s |

**Importante:** no ensaio Orca anterior com tradução foi usado `rectilinear` como substituto diagnóstico, porque o binário antigo rejeitava `zigzag` antes da correção do código-fonte. **Não é validação do novo módulo/algoritmo**; o novo `ipZigZag` aguarda Windows build e execução.

Fontes no servidor:
- Oficial: `/var/tmp/filuna-qa-wine/qa-a1-bambu-fully-resolved/out/plate_1.gcode`.
- Orca base: `/var/tmp/filuna-qa-wine/bambu-17-models-qa/filuna-with-official-flat/plate_1.gcode`.
- Orca traduzido (controle diagnóstico antigo): `/var/tmp/filuna-qa-wine/bambu-config-engine-qa/old-orca-control/plate_1.gcode`.
- Resultado modal controle: `/var/tmp/filuna-qa-wine/bambu-config-engine-qa/legacy-translated-vs-official.json`.

Os dados após tradução aproximada demonstraram: topo 70/70 extrusões e 72/140 deslocamentos (Bambu/Orca), M204 3/138; infill sólido interno E 139.48/141.38 mm e 30/105 deslocamentos. O problema de planner continua, além de interpretações/defaults. **Não remover M204 só para igualar contagem**: aceleração ativa é relevante à segurança.

## Evidência do build / pendências
Workflows PR em GitHub Actions, incluindo:
- `42153684518`: testes offline Linux sucesso; build Windows full iniciou a etapa `Build on Windows`.
- `42154053309`: controles offline sucesso; pipeline Windows `Build Deps / Build Deps` iniciou `Build on Windows`.
**No instante da última consulta, não havia resultado final de compilação Windows nem G-code novo gerado com o módulo integrado**. Atualizar este item quando o job concluir; se falhar, diagnosticar logs/reparar o erro de compilação. Não publicar instalador sem resultado.

Conector SentinelX: host `host_585dce92e0030ae5`; git worktree de apoio `/var/tmp/filuna-qa-wine/qa-repo` checkout antigo — trabalhar com `git fetch` e `git show FETCH_HEAD:path` ou branch separada para não destruir estado.

## Bloqueios inequívocos restantes
- **Algoritmo completo de planner/movimentos Bambu ainda não portado.** Código atual não garante equivalência em topo, infill sólido, jerk/tempo, extrusão nem metadados.
- **Rede OEM Bambu ainda não completa**: SSDP catálogo 17 IDs emulada PASS, mas MQTT/TLS 8883 autenticado, FTPS 990, pareamento, telemetria, upload/ACK nativos e hardware físico ainda NOT RUN.
- **Toda linha Bambu não homologada**: catálogo e snapshot de 17 IDs não significa suporte funcional; a primeira tradução ativa é apenas A1.
- **Tamanho final do pacote Filuna não medido** com novo engine; os 220,47 MB do portátil e 168,86 MB do instalador são dos artefatos Windows ANTERIORES.
- **Não há equivalência G-code demonstrada** do novo build; métricas antigas permanecem MISMATCH.

## Continuação obrigatória
1. Acompanhar o resultado do Windows full e corrigir falha de compilação identificada, sem simplesmente marcar PASS em teste isolado.
2. Gerar novo G-code com executável Windows dessa branch usando mesma fixture, comparar numérico por feature e metadados GCode.3MF, com tolerâncias acordadas.
3. Se a tradução por default não resolver, portar a parte divergente real de Bambu `GCode` (inclusive tratamento de M204, topo, infill e smooth coefficient) com proveniência/licença, em vez de alterar G-code textual para mascarar diferença.
4. Exercitar rede OEM autenticada em emulador e equipamento físico quando disponível, incluindo credenciais inválidas, desconexão, ACK e rollback.
5. Atualizar manifesto Bambu e tamanho da build; só o proprietário determina publicação/homologação sobre evidência.

**Esta rodada incorporou código de engine e conexão GUI de configuração real, mas NÃO o módulo Bambu inteiro. Não contrariar esse fato em respostas futuras.**
