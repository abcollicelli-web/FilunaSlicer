# FILUNA SLICER — HOMOLOGAÇÃO LAB R3 — 06/11/2026

**Resultado factual:** T1 Bambu A1 completo PASS (mantido); Creality K2 Plus cubo agora igual após normalização baseada em duas execuções oficiais (E-04 resolvida no escopo do cubo), porém T1 integral ainda FALHA por ausência do 3MF próprio e da peça CFS quatro cores. Contrato Bambu em C++ compilado e SSDP testado, mas **NÃO** homologado nativamente. Nenhum fabricante atende todos os gates CLI+CONTRATO.

**Escopo:** Ubuntu/Wine/Xvfb em rede de simuladores 10.77.0.0/24. Não imprimir em hardware, não fazer merge, deploy, release ou publicar build.

## Matriz obrigatória 8 × 11

| Fabricante | Modelo / cores | T1 | T2 | T3 | T4 | T5 | T6 | T7 | T8 | T9 | T10 | T11 | Veredito |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Bambu | A1 · AMS lite | PASS | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Elegoo | Centauri Carbon 2 Combo · CANVAS | FALHA | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Flashforge | AD5X · IFS | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| QIDI | Q2 Combo · QIDI Box | FALHA | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Creality | K2 Plus Combo · CFS | FALHA | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Snapmaker | U1 · SnapSwap | FALHA | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Prusa | CORE One · MMU3 | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Anycubic | Kobra S1 Combo · ACE Pro | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | PENDENTE DE CAMPO | EM VALIDAÇÃO |

**T2, T10 e T11:** PENDENTE DE CAMPO. **T3–T9:** NÃO EXECUTADO pelo aplicativo Filuna, embora haja PASS em simuladores e em adaptadores experimentais. Cada resultado experimental está abaixo e não foi promovido para a matriz.

## 1. Normalização controlada (OFICIAL A × OFICIAL B)

Regra: só normalizar campos cuja variação apareça entre duas execuções do **mesmo fatiador oficial**, mesmo comando literal e mesmo arquivo/presets. O comando executado e os logs ficam na pasta correspondente.

| Marca | Oficial A | Oficial B | Campos que variaram | Lista de normalização aprovada |
|---|---|---|---|---|
| Bambu A1 4 cores | Exit 0, SHA `bafae92490dd04a924f7ffa9312ba7a1ddb4ea79aebbae21a64f628e009569e1` | Exit 0, mesmo SHA | Nenhum | Vazia |
| Elegoo CC2 cubo | Exit 0, SHA `c8e2d5eaac0c9e92341f717df7b63bfea364cd69a3f71b12653b92a4f227e9af` | Exit 0, mesmo SHA | Nenhum nesta dupla | Vazia; não autorizar timestamp sem prova A×B com mudança |
| QIDI Q2 cubo | Exit 0, SHA `375e1e3334391267d5079e95877d5eeb2fcc5845f5897a93c34c19a1f1fdb5c0` | Exit 0, mesmo SHA | Nenhum | Vazia |
| Creality K2 Plus cubo | Exit 0, SHA `c64780f0c06b008c31f2c5688e56b4775afa53eb092d5b284884f7f2a764c6db` | Exit 0, SHA `d2e5b547bc43812a992fbdbdd507aa84f284c753446f0025d7ac9ff016b964f3` | Linha 2 timestamp; linha 7 `creality_uuid` | **Somente linhas 2 e 7 com os formatos validados** |
| Snapmaker, Flashforge, Prusa, Anycubic | Sem G-code oficial A×B válido | — | Não determinado | Vazia |

### Comando oficial Creality A e B (mesmo argv literal)

```sh
ROOT=/srv/site/incoming/filuna-local-oem-qa-20261106/run-20261106T153215Z
EXE=/srv/site/incoming/filuna-official-portables-20261106/unpacked/creality-v7.3.1/CrealityPrint.exe
CFG=$ROOT/creality/resolved-profiles
OUT=$ROOT/R3/creality-official-repeat/same-output
env WINEPREFIX=/var/tmp/filuna-qa-wine/prefix /usr/bin/timeout 50 /usr/bin/xvfb-run -a /usr/bin/wine64 "$EXE" --cli --debug 5 --slice 0 --need-gcode-file --load-settings "$CFG/machine.json;$CFG/process.json" --load-filaments "$CFG/filament.json" --outputdir "$OUT" /opt/filuna-tools/proto/models/cubo20.stl
```

Executado com os mesmos argumentos duas vezes; após cada execução foi arquivada uma cópia independente em `R3/creality-official-repeat/A/plate_1.gcode` e `B/plate_1.gcode`. Ambas com exit `0` e 280.075 bytes. Evidências: `command.txt`, `A/stdout.log`, `B/stdout.log`, `official-A-vs-B.json`.

Linhas observadas:
- Linha 2 A: `; generated by Creality_Print V7.3.1.6378 on 2026-11-06 at 16:20:22`
- Linha 2 B: `; generated by Creality_Print V7.3.1.6378 on 2026-11-06 at 16:20:23`
- Linha 7 A: `; creality_uuid: 1b007ffd-269d-44be-9f02-58c36cec270b`
- Linha 7 B: `; creality_uuid: 0c4cbfe0-8afd-4154-a0d2-d5e458ed2709`

**Oficial A normalizado = oficial B normalizado = Filuna adaptador normalizado**, sem outras diferenças: SHA-256 `5950bcc807d5d1e3cfbc0cc1a62eb2352998c3858e69b9182de8165eb68d7677`. O G-code Filuna bruto tinha SHA `933da9b3b3113b7b68352001aeeeac0f479627cee32356399221617d33c7a467`. Prova formal: `R3/creality-official-repeat/CREALITY-NORMALIZATION-PROOF.json`. **E-04 resolvida para o cubo.**

A tentativa de obter variação entre oficial A e B com intervalo na Elegoo retornou exit `3` para ambas sem produzir G-code. Não há nova regra de normalização Elegoo; tentativa e saída em `R3/official-repeat/elegoo-delayed/`.

## 2. T1 — falhas e correções (na ordem dos fabricantes)

**Bambu A1 — PASS mantido:** cubo SHA bruto igual `f907c93683ae5208fe47a9d22f3b9b125e51bbbaf87d8947b7a0e94c026950a4`; 4 cores efetivas AMS lite SHA bruto igual `bafae92490dd04a924f7ffa9312ba7a1ddb4ea79aebbae21a64f628e009569e1`. Ambos exit 0/0. Evidência no relatório R2, pastas `bambu/cli-real` e `bambu/four-color-assigned`. Nenhum campo normalizado para Bambu.

**Elegoo CC2 — FALHA:** OpenGL no mesmo Xvfb passou `glxinfo -B`, `OpenGL renderer string: llvmpipe (LLVM 21.1.8, 256 bits)`, OpenGL 4.5 Mesa 26.0.8. Nova tentativa com `LIBGL_ALWAYS_SOFTWARE=1`, `GALLIUM_DRIVER=llvmpipe`, `__GLX_VENDOR_LIBRARY_NAME=mesa`, `EGL_PLATFORM=surfaceless`, `MESA_LOADER_DRIVER_OVERRIDE=llvmpipe` e Xvfb `-screen 0 1920x1080x24 +extension GLX +render -noreset`: exit 3, **stdout/stderr da aplicação vazios**, `libEGL warning: failed to open /dev/dri/card1: Permission denied` em `R3/elegoo-surfaceless/xvfb.log`. Isso **não prova** que o aviso libEGL seja a causa do exit 3. Também não foi encontrado perfil separado com nome CANVAS nos resources locais; projeto anterior tem 4 objetos mas G-code apenas `T0`. Sem autenticação CANVAS demonstrada.

**Flashforge AD5X — NÃO EXECUTADO:** recursos portáteis oficiais ainda não extraídos/validados, sem CLI executada. Instalador 1.7.15 arquivado no laboratório, mas esta rodada não recuperou o executável.

**QIDI Q2 — FALHA:** tentativa anterior gerou 3MF 4 cores com `--load-filament-ids 1,2,3,4` e quatro PLA com `filament_is_support=0,0,0,0` apenas em cópia diagnóstica. Matriz de purga 4×4 presente no 3MF. Fatiamento oficial e Filuna exit 156, `return_code=-100`; trecho antes do erro em `qidi/four-color-diagnostic/official/stdout.log`: `tree support construct finish`, `is_support_necessary takes 0 secs`, `found slicing or export error for partplate 1`. Não houve novo G-code multicor.

**Creality K2 Plus — FALHA no T1 completo:** o cubo normalizado é idêntico, conforme prova oficial A×B. A CLI precisa de `--need-gcode-file --outputdir`, já confirmado no código-fonte `src/slic3r/CLI/SliceCommand.cpp`. `--export-3mf` é inválido na versão oficial. Sem geração do projeto 3MF da K2 pelo método próprio e sem quatro cores/CFS não há T1 completo PASS.

**Snapmaker U1 — FALHA:** fontes de `src/Snapmaker_Orca.cpp` confirmam `--slice`, `--outputdir`, `--load-settings`, `--load-filaments` e `--export-3mf`; **não** admitem `--need-gcode-file` (exit 254, `Invalid option --need-gcode-file`). A CLI ainda retorna exit 0 sem produzir G-code/3MF nas tentativas anteriores. Evidência em `snapmaker/cli-real/`, `slice-direct/`, `need-gcode-file/`. Ainda não corrigido.

**Prusa CORE One / Anycubic Kobra S1 — NÃO EXECUTADO:** executáveis oficiais não estavam disponíveis/fixados nesta bateria. Não inventar comandos, hashes ou efeitos de rede.

## 3. Rede Bambu — implementação C++ de contrato reutilizável, estado e limite

Verificação no GitHub (`abcollicelli-web/FilunaSlicer`) e checkout C++: **não** existem `IPrinterAdapter.cs`, `BambuAdapter.cs`, `BambuCloud.cs`, `BambuFtps.cs` nesta árvore. O aplicativo atual usa C++ (`src/slic3r/Utils/Moonraker.hpp`, `src/slic3r/GUI/Printer/BambuTunnel.h`). Não tratar interfaces .NET do projeto legado como se já integrassem o novo Filuna.

Criados em QA, para reutilização com a aplicação após integração:
- `lab/native/fabricantes/IPrinterAdapter.hpp`
- `lab/native/fabricantes/bambu/adapter/BambuAdapter.cpp` (SSDP broadcast, MQTT/TLS, pushall, transporte FTPS, comandos e inspeção AMS; senha via variável de ambiente em teste, **nunca** escrita em log)
- `lab/native/contract_test.cpp` — chama a **mesma classe** compilada na biblioteca `filuna_bambu_adapter`
- `lab/native/CMakeLists.txt`

CMake configure exit `0` e build exit `0`, biblioteca C++ real e binário `lab/native/build/filuna_contract_test` gerados. SHA-256 binário: `20d6f58d3afed2188195eded3b35fb7aa965a12a5058f1eb19fb0de2fdae7ffd`. Somente avisos de funções SHA256 OpenSSL deprecadas; sem erros C++.

Comando experimental executado:
```sh
LAB=/srv/site/incoming/filuna-local-oem-qa-20261106
timeout 20 "$LAB/lab/native/build/filuna_contract_test" "$LAB/run-20261106T153215Z/R3/native-bambu" "$LAB/bambuddy-data/virtual_printer/certs/bbl_ca.crt" "$LAB/run-20261106T153215Z/bambu/four-color-assigned/4colors-a1.3mf"
```

**Exit 3**, resultado por etapa: descoberta SSDP **PASS** via broadcast sem digitar IP, resposta A1 `serial=03900A391800001`, `ip=10.77.0.15`; par MQTT **NÃO EXECUTADO** porque a credencial de laboratório não foi injetada. Tentativa de injeção protegida via conector foi rejeitada pela camada de segurança; **não contornar**. Com credencial ausente, status, FTPS, project_file, pause/resume/stop, AMS e câmera nativos **NÃO EXECUTADO**. Evidências em `R3/native-bambu/bambu-native-contract.jsonl` e `stdout.log`.

**Atenção:** o módulo C++ compartilhado pelo teste ainda **não está ligado à árvore de build do executável Filuna Windows**, logo o PASS de SSDP experimental não vira PASS de T3–T9 oficiais. Também falta confirmação de SHA do lado receptor e ACK para os comandos: a existência das funções não equivale a aprovação do contrato.

O executor Python anterior (Bambu 0/10, Moonraker 7/10 auxiliares, Elegoo 2/10 auxiliares) continua separado e não homologa os módulos C++ do aplicativo. Os serviços simulados 13/13 PASS também continuam auxiliares.

## 4. Erros reais e pendências (não inventar erros para NÃO EXECUTADO)

| ID | Fabricante | Teste | Evidência | Estado e próxima ação |
|---|---|---|---|---|
| E-01 histórico | Creality | T1 | `Filament nozzle mapping is incomplete`, exit 205 | Repetir com 3MF criado pela K2 Plus e CFS; não reapareceu no cubo novo |
| E-02 | Snapmaker | T1 | Exit 0 sem G-code/3MF | Investigar execução/flag de exportação suportada, logs U1 completos |
| E-03 | Creality | T1 | `Invalid option --export-3mf`, 254 | Usar método próprio documentado para projeto 3MF |
| E-04 | Creality | T1 cubo | UUID variável provado na linha 7 A×B | **RESOLVIDO para o cubo**: SHA normalizado oficial A=B=Filuna |
| E-05 | QIDI | T1 | `filament_is_support` 1 versus 4, exit 251 | Vetor ajustado no fixture, teste multicor ainda não aprovado |
| E-06 | QIDI | T1 | Exit 156 / interno -100 | Falha de slicing do projeto 4 cores apesar de matriz purga 4×4 |
| E-07 | Elegoo | T1 | G-code só T0 | Faltam perfil/capacidades CANVAS e atribuição válida |
| E-08 | Elegoo | T1 | Exit 3; stderr vazio; libEGL warning | Investigar saída precoce e ambiente antes de atribuir causa |
| E-09 | Snapmaker | T1 | `Invalid option --need-gcode-file`, exit 254 | Não usar essa opção da Creality no Snapmaker |

## 5. Veredito e referências

**Nenhum fabricante HOMOLOGADO EM LABORATÓRIO (CLI + CONTRATO).** Bambu A1 possui T1 PASS. Creality teve avanço real no cubo por normalização comprovada. T3–T9 nativos continuam NÃO EXECUTADO. **Nenhum build de homologados**, nenhum merge, deploy, publicação, nem impressão física.

Pasta R3: `/srv/site/incoming/filuna-local-oem-qa-20261106/run-20261106T153215Z/R3/`.

Relatório anterior R2 preservado: `FILUNA/qa/HOMOLOGACAO-LAB-8-FABRICANTES-R2-20261106.md`.
