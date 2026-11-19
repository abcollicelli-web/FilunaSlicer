# Snapmaker U1 — G-code original imutável, prova de macros e validador geométrico

**Data:** 07/11/2026. **Escopo:** homologação laboratorial Snapmaker U1, preservando perfis e G-code do OEM. **PR:** #6 (`filuna/oem-portable-first-qa-20261106`, DRAFT). **Regra mestre permanente:** não modificar G-code para “corrigir” fabricante; segregar verificação geométrica e decisão de enviar.

## Entregas de código desta etapa

- `src/libslic3r/GCode.cpp`: removido o bloqueio provisório específico de `chamber_cooling_mode` para purga final e revertidas as tentativas de inserir Z-hop no gerador. `PrintConfig.cpp/.hpp`: eliminada a opção experimental `filuna_oem_final_purge_lift`. **Nenhuma transformação posterior de G-code** faz parte da solução.
- `lab/snapmaker_spatial_gcode_validator.py`: validador **somente leitura**, Python stdlib, acompanha G0–G3, G90/G91, M82/M83, G92, home sem origem conhecida, percurso de arco, ocupação XY/Z da peça ao longo do tempo e ferramenta; exclui da ocupação da peça somente as extrusões sinalizadas como torre `TYPE:Prime tower`; distingue `EXCLUDE_OBJECT_END` normal por camada de final real e limpeza curta de retração; testa volume U1 `270 × 270 × 270 mm`. Resultados `PASS`, `WARN`, `BLOCK`, `ENGINE_DIVERGENCE`, `REVIEW_REQUIRED`, `USER_ACCEPTED_RISK`. Fornece linha/camada/XYZ e SHA-256 do arquivo original, sem modificar bytes. `--official` exige G-code A/B para declarar comportamento OEM; risco só no Filuna é divergência interna e segue sem envio.
- Risco comprovado presente também no OEM: o usuário pode **depois de ser informado** assumir expressamente o risco para **um único arquivo SHA** usando frase exata `ACEITO O RISCO DE COLISAO DA SNAPMAKER`, identidade e `--audit-log`; a API de laboratório faz append JSONL com sincronização. **Isto ainda NÃO está integrado ao envio/dispositivos do aplicativo Windows**, com autenticação de conta/retenção de auditoria. Enquanto não integrado, o fluxo real de envio NÃO está liberado.
- `FILUNA/fabricantes/snapmaker/vendor.yaml`: política JSON-subset YAML 1.2 (sem loader que execute código), volume, fonte do firmware pinada, hashes/versões do portátil, `safe_macros` com prova por macro e **limites de escopo**. Nenhum nome de macro é isenção genérica.
- `lab/test_snapmaker_spatial_gcode_validator.py`: **16 testes geométricos e sintéticos**, com casos positivos/negativos e auditoria. `lab/test_oem_native_source_contract.py`: **18 verificações de contrato de fonte**, incluindo ausência de alteração do G-code.
- `.github/workflows/filuna-snapmaker-spatial-gcode-qa.yml`: quatro jobs de CI, incluindo G-code **real** dos fabricantes e negativos sintéticos; sem iniciar impressora ou modificar binário.

## Fonte exata do Z-hop da U1

### A. Perfil original 2.4.1 do portátil

Portátil `Snapmaker_Orca_Windows_V2.4.1_portable/resources/profiles/Snapmaker/machine/Snapmaker U1 (0.4 nozzle).json`, do ZIP SHA-256 `1f002e2168c576f800bcfd0509b5e164b5ad65a0963abc2bef8317d929d48c9d`. Preset efetivo pinado pelo GitHub Actions `42321783683`; `change_filament_gcode` SHA-256 `c5f09d22c099a7df2395ed76ed27c99d355eaf7caec7405b4eed2a6ebf2a5839`.

Trecho **da troca normal de ferramenta**, no modo por camada: `G91` / `G1 Z1.5 F600` / `G90`, **antes de** `T{next_extruder}` e `SM_PRINT_PREEXTRUDE_FILAMENT INDEX=...`; a opção por objeto tem movimento Z absoluto próprio. Não se pode aplicar essa prova à purga final sem troca real de ferramenta.

O `machine_end_gcode` chama `PRINT_END` e `TIMELAPSE_STOP`, mas o final da torre ocorre **antes** de `PRINT_END`.

### B. Firmware publicado pelo próprio fabricante

**Fonte usada:** `Snapmaker/u1-klipper`, commit `7ac453b50fe6779b2ec0b759038a17da5e6d671d`, arquivo [`lava/fluidd.cfg`](https://github.com/Snapmaker/u1-klipper/blob/7ac453b50fe6779b2ec0b759038a17da5e6d671d/lava/fluidd.cfg), blob SHA `dd8bf672d53cc58e3f57bd9d8a57f5aba31a37cf`, macro `[gcode_macro PRINT_END]` (~linha 216).

O script `PRINT_END` retrai e eleva Z: com Z<195, move para Z200; com 195≤Z<265, sobe 5 mm; perto de 270,5, limita o acréscimo. **Só depois** executa `INNER_PRINT_END`, `PARK_EXTRUDER` e `G0 X5 Y5`. O firmware tem proteção para **estacionamento ao final**, mas essa macro **não é executada antes do deslocamento final da purga**. Para a macro `MOVE_TO_DISCARD_FILAMENT_POSITION`, o trecho publicado encaminha XY por offsets, **sem elevação Z comprovada no próprio corpo**. `SM_PRINT_PREEXTRUDE_FILAMENT` é comando Python do Klipper; não foi promovido a seguro sem análise dinâmica de seu corpo e dependências.

Não houve acesso a U1 real para `/printer/objects/query?configfile` nem exportação autorizada Fluidd: **`CONFIGFILE_LIVE=NOT_RUN`**, e a versão instalada pelo cliente pode divergir da fonte pública. A política mantém esta limitação.

### C. G-code bicolor oficial — demonstração de risco real no próprio fabricante

**Fonte original:** Snapmaker Orca **2.4.1 Linux AppImage**, projeto bicolor da análise independente [DesignMaster-Solutions/u1-dryrun @38ab8c7](https://github.com/DesignMaster-Solutions/u1-dryrun/blob/38ab8c7/docs/orca-2.4.1-final-purge.md), `examples/sign-orca-2.4.1-linux.gcode.gz`, Git blob `ee6943bcf5ed255736218fc4942b05217d8f607c`. G-code extraído SHA-256 `05f227dd12598bd99626dd60a29e3ecdc2a97ae65db803d6e4f9520587254f48`.

**Resultado geométrico ao analisar o original sem modificá-lo: `BLOCK` com 1 único risco após correção dos falsos positivos.**

| Campo | Evidência |
|---|---|
| Linha do comando original | **38.911** |
| Comando G-code original | `G1 X19.842 Y224.850 F30000` sem Z-hop precedente |
| Camada | **14** (índice 0) |
| Ponto de cruzamento amostrado | **X63.569 Y147.185 Z3.05 mm** |
| Topo já impresso | **Z3.05 mm**, extrusão de referência na linha 35.807 |
| Tolerância de inspeção | 0,20 mm |
| Decisão | **BLOQUEAR ENVIO**, exibir posição e permitir aceite explícito auditado somente para risco do fabricante confirmado |
| Origem | **O próprio Snapmaker Orca 2.4.1**; não é alteração do Filuna neste arquivo |

A diferença está detalhada no [issue Snapmaker #1149](https://github.com/Snapmaker/OrcaSlicer/issues/1149) e no [PR upstream #1150](https://github.com/Snapmaker/OrcaSlicer/pull/1150), ainda candidato. Não foi aplicado esse patch ao motor Filuna porque a **ordem posterior do proprietário exige não alterar G-code para corrigir um comportamento do fabricante**.

## Casos positivos/negativos comprovados

**Execução:** [GitHub Actions #42345082502](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42345082502). Todos os quatro jobs concluídos com **SUCCESS**; 16/16 testes geométricos e 18/18 contratos de fonte PASS.

| Caso | Original/fixture | Esperado | Observado |
|---|---|---|---|
| U1 cubo monocor | Snapmaker Orca 2.4.1 oficial, stress-00, SHA `18486931...` | Não inventar falha de purga que não existe | **PASS**, sem bloqueio |
| U1 bicolor com purga final | Snapmaker Orca 2.4.1 original com risco real | Detectar colisão fabricante e **não** reescrever | **BLOCK**, camada 14 / linha 38911 |
| Sintético travessia sem Z-hop | Geometria com parede depositada | BLOCK | **PASS do teste, decisão BLOCK** |
| Sintético travessia com altura segura | Mesma geometria com Z elevado | PASS | **PASS** |
| Sintético espiral ultrapassa 270 mm | Arco G3 com Z crescente na borda | WARN / sugerir Normal | **PASS do teste, decisão WARN** |
| Sintético risco OEM confirmado + autorização | Mesmo G-code em candidato e oficial | Consentimento vinculado a SHA e registro | **PASS** |
| Sintético apenas Filuna divergente | Candidato tem travessia, oficial seguro | **ENGINE_DIVERGENCE**; não permitir override OEM | **PASS** |
| Bambu A1 cubo oficial | Original do runner #42312894880, SHA `ac7731e7653b...` | Sem falso positivo de geometria | **PASS** |
| Creality cubo oficial | Original do runner #42310133259, SHA `3ed1de4e20a...` | Sem falso positivo de geometria | **PASS** |

**Atenção:** a regressão cruzada Bambu/Creality compara **ocupação da peça**, mas **não homologa limites mecânicos de estacionamento dessas marcas**: o original Bambu usa X negativos de limpeza fora da mesa imprimível. Esse gate de limites está desativado nos casos cruzados por ausência de envelope mecânico autorizado. O limite de 270 mm da Snapmaker permanece ligado.

**Caso positivo de 4 cores da U1:** **NOT_RUN / BLOCKED**. Não há evidência de Z-hop via macro que proteja a purga final sem ferramenta: o original bicolor já apresentou risco real. Não se deve inventar PASS de 4 cores.

## O que continua faltando antes de “só impressão física”

- **Integração nativa do validador no aplicativo Filuna Windows**, antes de envio/execução, com UI da linha/camada/XYZ, autenticação de aceite, persistência segura e opção de anexar relatório à Snapmaker. QA Python externo não é o mesmo que aplicativo pronto.
- **Paridade G-code Filuna recompilado × OEM** com mesmo projeto e presets, comparação de G0/G1/G2/G3, E/F, scripts, purga e repetibilidade; original 2.4.1 tem divergência de hashes sob stress. Não há novo binário Filuna testado nesta rodada. Diferenciar erro do gerador Filuna e risco de origem OEM.
- **G-code original 4 cores** com fonte, firmware e scripts efetivos versionados; teste de macros em **configuração real autenticada** da U1 ou simulador comprovadamente equivalente; integração DeviceBridge e testes de descoberta, status, upload, confirmação, câmera/slots quando aplicáveis.
- **G4-I=BLOCKED, PHYSICAL_BETA=NOT_RUN, SNAPMAKER_INTERNAL_HOMOLOGATION=BLOCKED.** Este relatório comprova a correção da abordagem do validador, não a prontidão para imprimir. O build final único, assinatura, atualização e homologação de 3 impressões físicas continuam sujeitos às etapas do Plano Mestre.

## Política operacional e atualização

A regra foi inserida em `FILUNA/REGRA-MESTRE-QUALIDADE.md` e precisa continuar no registro de continuidade e no PR #6. A solução preserva o motor Orca único e o código do fabricante como referência. **Nenhum deploy/merge/release, impressora física, atualização de firmware, alteração de G-code ou alteração do servidor foi executado nesta rodada.** Bambuddy autenticado continua adiado.
