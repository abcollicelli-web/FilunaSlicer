# FILUNA — HOMOLOGAÇÃO DE LABORATÓRIO: EXECUÇÃO REAL — 06/11/2026

**Rodada**: `/srv/site/incoming/filuna-local-oem-qa-20261106/run-20261106T153215Z/`.

**Escopo**: Windows/Wine em Ubuntu, fatiamento CLI oficial versus adaptador CLI Filuna, verificação dos simuladores. Nenhum merge, deploy, publicação, impressão física ou certificação Windows nativo. A execução do script único com manifesto inicialmente vazio gerou 136 registros `NÃO EXECUTADO`; os comandos reais da rodada foram executados separadamente e guardados nas subpastas por fabricante. O executor foi corrigido para produzir resumo apenas uma vez.

## Matriz 8 x 11

| Fabricante | Modelo | T1 | T2 | T3 | T4 | T5 | T6 | T7 | T8 | T9 | T10 | T11 | Veredito |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Bambu | A1 · AMS lite | PASS | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Elegoo | CC2 Combo · CANVAS | FALHA | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Flashforge | AD5X · IFS | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| QIDI | Q2 Combo · QIDI Box | FALHA | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Creality | K2 Plus Combo · CFS | FALHA | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Snapmaker | U1 · SnapSwap | FALHA | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Prusa | CORE One · MMU3 | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Anycubic | Kobra S1 Combo · ACE Pro | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |

**T1 PASS da Bambu é CLI oficial versus adaptador Filuna, não a integração nativa do Preparar.** T3–T9 permanecem NÃO EXECUTADO para todos os modelos porque o executável filuna_contract_test não está presente nos locais consultados. Os PASS dos serviços dos simuladores abaixo não preenchem essas células. T2/T10/T11 pendentes de campo conforme ordem.

## T1 — resultados reais e SHA-256

### Bambu A1 — PASS

CLI Bambu Studio 02.09.01.70, perfis oficiais completos A1 e PLA. 3MF cubo 20 mm criado pela própria CLI, exit 0, SHA 212202ece5982aaceccdbe57359c070ecec3c4ddb00d4baf211c80556f50baa4. Fatiamento oficial e adaptador Filuna, exit 0/0, SHA G-code bruto igual f907c93683ae5208fe47a9d22f3b9b125e51bbbaf87d8947b7a0e94c026950a4 (374541 bytes).

Quatro peças e filamentos atribuídos pela CLI da marca com --load-filament-ids 1,2,3,4. 3MF exportado exit 0, SHA 6dd70edded2f705b99ad8467c907121d4a15906d20d2632593cfd3c814e6cfa4. Os quatro extrusores 1–4 estão documentados em Metadata/model_settings.config. G-code oficial/adaptador exit 0/0, bruto idêntico SHA bafae92490dd04a924f7ffa9312ba7a1ddb4ea79aebbae21a64f628e009569e1 (1234609 bytes); M620 S0/S1/S2/S3 presentes. Nota: cores/filamentos da fixture derivam de preset oficial PLA e não são operação física.

Comando de exportação 4 cores executado, com ROOT apontando para a pasta desta rodada, EXE para unpacked/bambu-v02.09.01.70/bambu-studio.exe e CFG para /var/tmp/filuna-qa-wine/qa-a1-bambu-fully-resolved:

    env WINEPREFIX=/var/tmp/filuna-qa-wine/prefix timeout 50 xvfb-run -a wine64 "$EXE" --debug 5 --load-settings "$CFG/machine.json;$CFG/process.json" --load-filaments "$F1;$F2;$F3;$F4" --load-filament-ids '1,2,3,4' --export-3mf 4colors-a1.3mf --outputdir "$ROOT/bambu/four-color-assigned" "$FIX/filament-1-FF0000.stl" "$FIX/filament-2-00FF00.stl" "$FIX/filament-3-0000FF.stl" "$FIX/filament-4-FFFF00.stl"

Para fatiar, foram executadas separadamente a CLI oficial com --slice 0 --outputdir .../official e o adaptador/filuna_oem_cli.py --brand bambu --runtime wine -- --slice 0 --outputdir .../filuna, ambos recebendo o MESMO 4colors-a1.3mf. Evidências: bambu/cli-real e bambu/four-color-assigned.

### Elegoo CC2 — FALHA no requisito multicor

3MF cubo criado com CLI ElegooSlicer 1.5.3.5 e perfis CC2, exit 0, SHA c5a6bb377548c129cb1c14d2b72ea1cc49197ecccc339da99c3c37b75ea68450. G-code oficial/adaptador exit 0/0, bruto diferente somente na linha 2 com data/hora; após normalização exclusiva dessa linha, SHA igual ee6ba68f2d4699edb68a580cb4aa9ea07295d4fb1584fd78dc80033d9c67375f (208003 bytes).

3MF inicial de quatro peças saiu com exit 0, SHA a94f407d45b5e940044de4b1d456abcfb254dbfe55938ee4247fc1d75148d9b8. Fatiamento oficial/adaptador exit 0/0, normalizado igual SHA 8cc9b5b1457e0c13232deed2e34bc5af4b01c474bfd989eeb729a6e4e36ea344, MAS só T0 apareceu no G-code: **não houve quatro cores efetivas**. Nova tentativa com --load-filament-ids 1,2,3,4 terminou exit 3 sem 3MF nem saída da aplicação; Xvfb registrou libEGL warning: failed to open /dev/dri/card1: Permission denied. Não é erro de protocolo Elegoo.

### Flashforge AD5X — NÃO EXECUTADO

Instalador 1.7.15 presente, mas sem executável portátil/resources validado para CLI nesta rodada. Nenhum comando oficial executado ou 3MF/G-code produzido. Não gerar erro E apenas pela ausência.

### QIDI Q2 Combo — FALHA no requisito multicor

3MF cubo via CLI oficial QIDI Studio 02.07.02.73, exit 0, SHA 2e5a8d293bac8846d547e16a0d4d04ee07e67b63ed1eea6f3b1844b38266ac04. Fatiamentos oficial/adaptador exit 0/0, SHA bruto idêntico 375e1e3334391267d5079e95877d5eeb2fcc5845f5897a93c34c19a1f1fdb5c0 (258131 bytes).

Exportação original de quatro peças/filamentos exit 251: filament_is_support's count 1 not equal to filament_colour's size 4. Em cópia de diagnóstico do preset, foi acrescentado filament_is_support=0,0,0,0, permitindo exportar 3MF com quatro extrusores, exit 0, SHA a72db1300e5d1c563d19cbef63a644b537abf686ead22c83eca5302de8fc080f. Fatiamentos oficial e adaptador desse mesmo 3MF falharam, exit 156/156, mensagem interna: Failed slicing the model. Please verify the slicing of all plates on QIDI Studio before uploading. Código interno -100. **Perfil alterado somente na cópia de diagnóstico; não homologar.**

### Creality K2 Plus Combo — FALHA

Perfis machine/process/filament K2 Plus carregados dos resources oficiais, sem heranças faltantes. Tentativa --cli --export-3mf terminou exit 254 com stderr: Invalid option --export-3mf. Sem --cli terminou exit 1: Invalid option --export-3mf; GUI parameter parsing failed. Alternativa --cli --slice 0 sobre STL terminou exit 0, porém nenhum G-code foi preservado em --outputdir; log registra gcode temporário removido. O erro histórico E-01 código 205 (Filament nozzle mapping is incomplete) ocorreu em outro projeto e não foi reproduzido aqui. Caminhos: creality/cli-real, creality/export-no-cli, creality/slice-cli.

### Snapmaker U1 — FALHA

CLI oficial Snapmaker Orca 2.4.1 com perfis U1 dos resources do portátil e --debug 5: --export-3mf terminou exit 0 sem 3MF; --slice 0 sobre STL terminou exit 0 sem G-code. Logs stdout/stderr completos em snapmaker/cli-real e snapmaker/slice-direct. Este comportamento reproduz o histórico E-02, agora com novas evidências.

### Prusa CORE One MMU3 — NÃO EXECUTADO

Versão/portátil oficial PrusaSlicer não fixado neste ambiente. Sem comando CLI executado, exit code ou SHA da fixture.

### Anycubic Kobra S1 Combo — NÃO EXECUTADO

Anycubic Slicer Next oficial não localizado como portátil executável. A evidência anterior de MQTT LAN da Kobra 3 V2 (modelId 20027) não substitui teste Kobra S1 (modelId 20025). ACE Pro e vídeo FLV permanecem sem teste pelo Filuna.

## Serviços de laboratório — 13/13 PASS auxiliares

Script de teste executado pelo SentinelX, resultados e timestamps em network-probes.json: TLS Bambuddy 8883 e FTPS 990 (A1 e A1 Mini), UDP descoberta Elegoo CC2, UDP descoberta SDCP, Moonraker GET info, upload sintético, start, pause, resume, cancel e estado final ready. Resultados adicionais anteriores: MQTT da CC2 com CONNACK 20020000, WebSocket SDCP 3030 com ping/pong, upload multipart e MD5, vídeo MJPEG 3031. Nenhum desses testes é evidência da invocação de módulo Filuna.

## Erros reais e ações (somente falhas)

| Código | Fabricante | Mensagem / saída | Evidência | Próxima ação |
|---|---|---|---|---|
| E-01 (histórico) | Creality | Filament nozzle mapping is incomplete, retorno 205 | QA-STATUS-PORTATEIS-E-MODULOS-LOCAL-2026-11-06.md | Corrigir mapeamento bico/filamento e suportar 3MF oficial K2 Plus |
| E-02 (histórico e reproduzido) | Snapmaker | Exit 0 sem 3MF/G-code | snapmaker/cli-real e snapmaker/slice-direct | Conferir parser CLI/paths de saída e recursos U1 |
| E-03 | Creality | Invalid option --export-3mf (exit 254 / 1) | creality/cli-real/stderr.log; export-no-cli/stderr.log | Descobrir interface de exportação suportada pela 7.3.1 |
| E-04 | Creality | Exit 0 sem G-code persistido | creality/slice-cli/stdout.log | Corrigir exportação do temporário para outputdir |
| E-05 | QIDI | filament_is_support's count 1 not equal to filament_colour's size 4, exit 251 | qidi/four-color/export.stdout.log | Ajustar preset oficial/fixture para quatro filamentos |
| E-06 | QIDI | Failed slicing the model. Please verify the slicing of all plates on QIDI Studio before uploading., exit 156, interno -100 | qidi/four-color-diagnostic/official/result.json | Revisar layout e compatibilidade Q2 Box, repetir no oficial |
| E-07 | Elegoo | Quatro peças mas apenas T0 no G-code | elegoo/four-color/official/plate_1.gcode | Corrigir o mapeamento de quatro slots CANVAS no 3MF |
| E-08 | Elegoo | Exportação explícita exit 3 e Xvfb libEGL Permission denied | elegoo/four-color-assigned/xvfb.log | Isolar display e repetir sem falha gráfica |

## Veredito e próximos gates

**NENHUM fabricante HOMOLOGADO EM LABORATÓRIO (CLI + CONTRATO).** Bambu A1 tem T1 PASS real, mas T3–T9 não foram executados pelo contrato nativo. O executável filuna_contract_test não foi encontrado no laboratório/checkout consultados; não criar PASS apenas porque as impressoras simuladas respondem. T2/T10/T11 permanecem PENDENTE DE CAMPO. Nenhum fabricante foi liberado para o build condicionado à homologação.