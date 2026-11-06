# FILUNA — Homologação de laboratório 8 fabricantes — Execução R2, 06/11/2026

**Escopo:** evidências da rodada `run-20261106T153215Z` mais novas tentativas de correção nesta mesma pasta. Ubuntu/Wine/Xvfb e simuladores da rede isolada, sem impressora física. **Nenhum merge, deploy, release ou build distribuído.** A matriz abaixo corrige a anterior.

## Matriz obrigatória T1–T11

| Fabricante | Modelo e cores | T1 | T2 | T3 | T4 | T5 | T6 | T7 | T8 | T9 | T10 | T11 | Veredito |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Bambu | A1 · AMS lite | PASS | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Elegoo | Centauri Carbon 2 Combo · CANVAS | FALHA | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Flashforge | AD5X · IFS | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| QIDI | Q2 Combo · QIDI Box | FALHA | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Creality | K2 Plus Combo · CFS | FALHA | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Snapmaker | U1 · SnapSwap | FALHA | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Prusa | CORE One · MMU3 | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Anycubic | Kobra S1 Combo · ACE Pro | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |

**T1 PASS** significa CLI oficial versus adaptador CLI com os mesmos projetos, e não a GUI Filuna. **T3–T9 são NÃO EXECUTADO pelo código nativo Filuna**, apesar dos PASS dos emuladores e dos testes do novo protótipo de contrato abaixo. T2/T10/T11 são PENDENTE DE CAMPO. Nenhuma marca é HOMOLOGADO EM LABORATÓRIO (CLI + CONTRATO).

## 1. T1 — resultados e comandos por marca

### Bambu A1: PASS, evidência original reaproveitada
- Oficial Bambu Studio 02.09.01.70 e adaptador `filuna_oem_cli.py` fatiaram os dois mesmos 3MF criados pela Bambu CLI; ambos exit `0`.
- Cubo: 3MF `bambu/cli-real/cubo20-a1.3mf` SHA256 `212202ece5982aaceccdbe57359c070ecec3c4ddb00d4baf211c80556f50baa4`; G-code oficial e adaptador `f907c93683ae5208fe47a9d22f3b9b125e51bbbaf87d8947b7a0e94c026950a4`.
- Quatro cores: 3MF `bambu/four-color-assigned/4colors-a1.3mf` SHA256 `6dd70edded2f705b99ad8467c907121d4a15906d20d2632593cfd3c814e6cfa4`; G-code SHA256 nos dois lados `bafae92490dd04a924f7ffa9312ba7a1ddb4ea79aebbae21a64f628e009569e1`, 1.234.609 bytes. G-code contém `M620 S0/S1/S2/S3`.
- Comando crítico: `xvfb-run -a wine64 bambu-studio.exe --debug 5 --load-settings <machine;process> --load-filaments <4 JSONs> --load-filament-ids 1,2,3,4 --export-3mf 4colors-a1.3mf --outputdir <diretório> <4 STL>`. Comandos completos e caminhos reais constam do relatório original da rodada e dos arquivos `bambu/four-color-assigned`.
- Trata-se de PASS do **adaptador CLI**, não do contrato de rede do aplicativo.

### Elegoo Centauri Carbon 2 Combo: FALHA no T1 multicor
- Cubo: 3MF `elegoo/cli-real/cubo20-elegoo.3mf` SHA `c5a6bb377548c129cb1c14d2b72ea1cc49197ecccc339da99c3c37b75ea68450`; oficial/adaptador exit `0`; G-code diferente apenas no timestamp, normalizado igual.
- Quatro peças: `elegoo/four-color/4c-elegoo.3mf` SHA `a94f407d45b5e940044de4b1d456abcfb254dbfe55938ee4247fc1d75148d9b8`; G-code de ambas as CLIs igual após remover timestamp, mas gera **somente T0**, não é quatro cores efetivas.
- Tentativa `--load-filament-ids 1,2,3,4` + `LIBGL_ALWAYS_SOFTWARE=1` + `xvfb-run -a -s '-screen 0 1920x1080x24 +extension GLX'`: exit `3`, stdout/stderr vazios, **nenhum 3MF**. Log Xvfb: `libEGL warning: failed to open /dev/dri/card1: Permission denied`. Comando completo pode ser reconstruído do log de execução desta sessão; não houve arquivo de comando no diretório. Evidência em `elegoo/glx-retry/xvfb.log`.
- Status: FALHA multicor; não declarar definitivamente que o erro de GLX seja causado pelo fatiador.

### Flashforge AD5X: NÃO EXECUTADO
Não foi localizado executável portátil oficial com `resources/` extraídos para a bateria; instalador 1.7.15 arquivado, sem CLI utilizável confirmada. **Sem comando T1 executado, exit code ou SHA de G-code.**

### QIDI Q2 Combo: FALHA no T1 multicor
- Cubo oficial/adapter exit `0`, 3MF `qidi/cli-real/cubo20-qidi.3mf` SHA `2e5a8d293bac8846d547e16a0d4d04ee07e67b63ed1eea6f3b1844b38266ac04`; G-code SHA bruto idêntico `375e1e3334391267d5079e95877d5eeb2fcc5845f5897a93c34c19a1f1fdb5c0`.
- Quatro cores: primeira tentativa exit `251`, log `filament_is_support's count 1 not equal to filament_colour's size 4`. Ajuste **diagnóstico** somente em cópia do preset com quatro PLA, `filament_is_support=[0,0,0,0]`: gera 3MF exit `0` SHA `a72db1300e5d1c563d19cbef63a644b537abf686ead22c83eca5302de8fc080f`.
- A matriz de purga **4×4 (16 posições)** já está presente no 3MF; não é correto dizer que falta. Mesmo assim, fatiamento oficial e pelo adaptador retornam exit `156`, `return_code=-100`, `Failed slicing the model. Please verify the slicing of all plates on QIDI Studio before uploading.`. Trechos completos: `qidi/four-color-diagnostic/official/stdout.log` e `result.json`. Sem G-code de 4 cores.

### Creality K2 Plus Combo: FALHA no T1 completo, correção real da persistência
- No código da tag em `src/slic3r/CLI/SliceCommand.cpp`, `need_gcode_file` é a opção que preserva o G-code; sem ela a CLI apaga deliberadamente o arquivo temporário após exportação.
- **Comando que funcionou:** `env WINEPREFIX=/var/tmp/filuna-qa-wine/prefix timeout 50 xvfb-run -a wine64 CrealityPrint.exe --cli --debug 5 --slice 0 --need-gcode-file --load-settings <machine.json;process.json> --load-filaments <filament.json> --outputdir <run>/creality/need-gcode-file /opt/filuna-tools/proto/models/cubo20.stl`. Perfis K2 Plus 0.4 oficiais resolvidos a partir do portátil. Exit `0`, arquivo `plate_1.gcode` (274 KB) com SHA `f6f361cf151e68761cf9eb21facc37efa248b7cbf3bcceafe9f2fc15bfefbe58`. Evidência `creality/need-gcode-file/command.txt`, `stdout.log`, `stderr.log` e `exit-code.txt`.
- Mesmo comando pelo adaptador Filuna, com `--brand creality --runtime wine --`: exit `0`, arquivo SHA `933da9b3b3113b7b68352001aeeeac0f479627cee32356399221617d33c7a467`. A comparação linha a linha mostra **duas diferenças**, timestamp do gerador na linha 2 e `creality_uuid` na linha 7. A regra desta bateria só permite normalizar timestamp: **não há PASS de equivalência estrita**.
- `--export-3mf` continua opção inválida (exit `254` com --cli, `1` sem --cli). Sem 3MF gerado pela CLI própria, nem peça 4 cores com CFS. A falha histórica `Filament nozzle mapping is incomplete`, 205, permanece como antecedente, não repetida na nova execução sobre STL.
- Saída retida corrigida; gerar projeto 3MF com o mecanismo documentado da fabricante e resolver mapeamento filamento→bico ainda necessário.

### Snapmaker U1: FALHA no T1
- Presets machine/process/filament U1 presentes no **resources do portátil** e resolvidos sem ausência de herança.
- `--debug 5 --export-3mf` exit `0` **sem arquivo 3MF**; `--debug 5 --slice 0` exit `0` **sem G-code**, logs `snapmaker/cli-real`, `snapmaker/slice-direct`.
- Diagnóstico `--need-gcode-file`: exit `254`, `Invalid option --need-gcode-file` em `snapmaker/need-gcode-file/stderr.log`. `--help` exit `0` sem listar opções.
- Não substituir por arquivo Orca genérico e não marcar PASS.

### Prusa CORE One · MMU3: NÃO EXECUTADO
PrusaSlicer oficial win64 ainda não fixado/executável não disponibilizado nesta bateria; sem exit code, 3MF ou G-code.

### Anycubic Kobra S1 Combo · ACE Pro: NÃO EXECUTADO
Anycubic Slicer Next oficial não localizado como portátil nesta bateria; MQTT local/câmera FLV comunitários são investigação separada. Kobra 3 V2 (`modelId 20027`) não é Kobra S1 (`modelId 20025`).

## 2. Contrato de rede experimental — executado; NÃO é o módulo nativo Filuna

Criado e validado `lab/filuna_contract_test.py` e cópia executável `lab/filuna_contract_test`, SHA256 `ab3f40d98cda13d4e4c09e9d58d8ecd32e2ab5db84cbac7a8b305386f7f52891`. Commit na branch QA: `13defed802f80cc4d40a38c4fb0acb1fd37bc2ae`.

Comando efetivamente executado para cada marca:
```sh
RUN=/srv/site/incoming/filuna-local-oem-qa-20261106/run-20261106T153215Z/contract-20261106
python3 /srv/site/incoming/filuna-local-oem-qa-20261106/lab/filuna_contract_test.py --brand bambu --run-dir "$RUN"
python3 /srv/site/incoming/filuna-local-oem-qa-20261106/lab/filuna_contract_test.py --brand moonraker --run-dir "$RUN"
python3 /srv/site/incoming/filuna-local-oem-qa-20261106/lab/filuna_contract_test.py --brand elegoo --run-dir "$RUN"
```

As três execuções terminaram com **exit code 0 do processo** (isso significa que a ferramenta registrou todos os resultados, não que cada etapa passou).

| Módulo de teste | PASS experimental | NÃO EXECUTADO | Observações |
|---|---:|---:|---|
| Bambu | 0 | 10 | Não houve descoberta SSDP sem IP nem Bind/MQTT autenticado; porta 2021 ocupada por processo do laboratório |
| Moonraker | 7 | 3 | Subnet discovery encontrou 10.77.0.13; par salvo; snapshot de status, start/pause/resume/stop responderam; upload **sem SHA recebido** não passa; câmera e materiais não implementados |
| Elegoo CC2 | 2 | 8 | Descoberta broadcast 52700 e cadastro local; não demonstra autenticação MQTT, persistência após reinício, camera ou CANVAS |

Logs completos por etapa com horário: `contract-20261106/contract-events.jsonl`, e JSONs individuais `moonraker-summary.json`, `elegoo-summary.json`, `bambu-summary.json`.

**Limitação decisiva:** o protótipo Python não faz chamadas à classe C++ `Slic3r::Moonraker` nem ao módulo Bambu do aplicativo distribuído. Também não entrega `filuna_contract_test` nativo compilado em C++. Esses PASS são **auxiliares de implementação experimental**, não T3–T9 oficiais da matriz. O envio Moonraker ocorreu, mas não obteve confirmação SHA-256 calculada no simulador. T3–T9 ficam NÃO EXECUTADO pelo contrato nativo Filuna.

Os 13/13 PASS anteriores de Bambuddy TLS/FTPS, Elegoo UDP e Moonraker HTTP permanecem **auxiliares**; evidência `network-probes.json`.

## 3. Falhas reais e ações

| ID | Marca | Teste | Mensagem/exit | Onde | Próxima ação |
|---|---|---|---|---|---|
| E-01 histórico | Creality | T1 | `Filament nozzle mapping is incomplete`, 205 | QA-STATUS original | Reproduzir em projeto K2 Plus 3MF com mapa filamento→bico |
| E-02 | Snapmaker | T1 | exit 0 sem projeto/G-code | `snapmaker/cli-real` | Verificar CLI oficial e mapeamento U1 |
| E-03 | Creality | T1 | `Invalid option --export-3mf`, 254/1 | `creality/cli-real`, `export-no-cli` | Usar o comando real da fabricante para criar 3MF; sem inventar --export |
| E-04 | Creality | T1 | SHA diferente por `creality_uuid` além de horário | `creality/need-gcode-file` | Reutilizar o mesmo projeto oficial 3MF para fixar UUID |
| E-05 | QIDI | T1 | `filament_is_support's count 1 not equal to filament_colour's size 4`, 251 | `qidi/four-color` | Quatro PLA comuns, vetores 4 valores; não modificar pacote oficial |
| E-06 | QIDI | T1 | `Failed slicing the model...`, 156 / -100 | `qidi/four-color-diagnostic` | Investigar geração da placa, toolchanges e campos adicionais; matriz de purga já 4×4 |
| E-07 | Elegoo | T1 | Sem trocas: G-code só T0 | `elegoo/four-color` | Perfil CANVAS e atribuição explícita funcional |
| E-08 | Elegoo | T1 | Exit 3, sem projeto, `libEGL warning: failed to open /dev/dri/card1: Permission denied` | `elegoo/glx-retry/xvfb.log` | Resolver Xvfb/Mesa e reexecutar exportação |
| E-09 | Snapmaker | T1 | `Invalid option --need-gcode-file`, 254 | `snapmaker/need-gcode-file/stderr.log` | Identificar opção real do Snapmaker Orca |

**Falha E-04 antiga (arquivo G-code temporário apagado pela Creality) foi corrigida pela opção real `--need-gcode-file`; não persistir como erro ativo de arquivo ausente.**

## 4. Veredito

- Bambu A1: **T1 CLI PASS**; T3–T9 nativos ainda NÃO EXECUTADO, **EM VALIDAÇÃO**.
- Elegoo, QIDI, Creality, Snapmaker: **EM VALIDAÇÃO** por falhas reais descritas.
- Flashforge, Prusa, Anycubic: **EM VALIDAÇÃO** por falta da CLI oficial de referência nesta bateria, sem falha inventada.
- Nenhuma marca está homologada para build; nenhuma execução de Windows real/GUI, T2/T10/T11 ou câmera física.
- Sem merge, deploy, publicação ou alteração do aplicativo Windows distribuído.

**Pasta de evidências:** `/srv/site/incoming/filuna-local-oem-qa-20261106/run-20261106T153215Z/`.
