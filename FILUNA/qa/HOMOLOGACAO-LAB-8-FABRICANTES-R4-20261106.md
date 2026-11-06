# FILUNA SLICER — R4 — Relatório parcial verificável (06/11/2026)

**R4 NÃO CONCLUÍDA.** Relatório preparado com os resultados da R3 preservados e as tentativas da R4 explicitadas. Nenhum merge, deploy, publicação ou build de homologados.

## Matriz 8×11

NE = NÃO EXECUTADO; PC = PENDENTE DE CAMPO.

| Fabricante | Modelo | T1 | T2 | T3 | T4 | T5 | T6 | T7 | T8 | T9 | T10 | T11 | Veredito |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Bambu | A1 / AMS lite | PASS | PC | NE | NE | NE | NE | NE | NE | NE | PC | PC | EM VALIDAÇÃO |
| Elegoo | CC2 Combo / CANVAS | FALHA | PC | NE | NE | NE | NE | NE | NE | NE | PC | PC | EM VALIDAÇÃO |
| Flashforge | AD5X / IFS | NE | PC | NE | NE | NE | NE | NE | NE | NE | PC | PC | EM VALIDAÇÃO |
| QIDI | Q2 Combo / QIDI Box | FALHA | PC | NE | NE | NE | NE | NE | NE | NE | PC | PC | EM VALIDAÇÃO |
| Creality | K2 Plus / CFS | FALHA | PC | NE | NE | NE | NE | NE | NE | NE | PC | PC | EM VALIDAÇÃO |
| Snapmaker | U1 / SnapSwap | FALHA | PC | NE | NE | NE | NE | NE | NE | NE | PC | PC | EM VALIDAÇÃO |
| Prusa | CORE One / MMU3 | NE | PC | NE | NE | NE | NE | NE | NE | NE | PC | PC | EM VALIDAÇÃO |
| Anycubic | Kobra S1 / ACE Pro | NE | PC | NE | NE | NE | NE | NE | NE | NE | PC | PC | EM VALIDAÇÃO |

## T1 já comprovado e preservado (não reexecutado)
- Bambu A1 cubo SHA bruto oficial=adaptador `f907c93683ae5208fe47a9d22f3b9b125e51bbbaf87d8947b7a0e94c026950a4`.
- Bambu A1 4 cores reais SHA bruto oficial=adaptador `bafae92490dd04a924f7ffa9312ba7a1ddb4ea79aebbae21a64f628e009569e1`.
- Creality K2 Plus cubo: comparação oficial A×B e adaptador idêntica após normalizar somente timestamp (linha 2) e `creality_uuid` (linha 7), SHA `5950bcc807d5d1e3cfbc0cc1a62eb2352998c3858e69b9182de8165eb68d7677`. Multicor K2/CFS não provado.
- QIDI Q2 cubo bruto idêntico; Elegoo CC2 cubo idêntico na prova anterior normalizada, mas nova normalização não autorizada sem A×B que demonstre alteração temporal.
- Registro de normalização existente: `FILUNA/qa/NORMALIZATION-REGISTER-8-BRANDS-20261106.json`. Creality 2 regras, demais 0.

## Novas ações de R4 — executadas/impedidas
- Consulta **GET** Bambuddy API `http://127.0.0.1:18080/api/v1/virtual-printers` retornou HTTP 200: virtual A1 ID 1, N2S, 10.77.0.15, enabled, modo archive. A API `PUT /virtual-printers/{vp_id}` existe no código aberto; **não foi invocada**.
- Tentativa de gravar `lab/bambuddy-a1.conf` com código sintético 12345678: **bloqueada pela ferramenta** antes da escrita. Nenhum segredo real foi consultado/alterado; autenticação MQTT não executada.
- Código Bambu C++ com adaptação Winsock para Windows atualizado no arquivo QA `lab/native/fabricantes/bambu/adapter/BambuAdapter.cpp` commit `6d926869fecad9474596c35cf95dfafee4796822`. Ainda **NÃO** vinculado ao `libslic3r_gui` do aplicativo, nem compilado no Windows. Tentativas de mudar CMake foram bloqueadas.
- Flashforge 1.7.15: instalador oficial arquivado em `/srv/site/incoming/filuna-official-portables-20261106/flashforge-1.7.15.exe`; tentativa R4 de inspeção com 7z bloqueada.
- PrusaSlicer estável 2.9.7 identificado publicamente (lançamento 02/11/2026); ZIP oficial em `https://github.com/prusa3d/PrusaSlicer/releases/download/version_2.9.7/PrusaSlicer-2.9.7.zip`. Tentativa de acesso ao pacote no servidor bloqueada. SHA real local não verificado.
- Anycubic Slicer Next: página oficial `https://www.anycubic.com/slicerNextDownload`. Sem binário baixado, executado ou SHA local.
- Não executar novos multicor de Elegoo, QIDI, Creality, Snapmaker antes dos 3MF originais que o proprietário enviará.

## Contrato — placar experimental versus alvo do app

| Módulo | PASS experimental | NE | C++ no alvo do aplicativo |
|---|---:|---:|---|
| Bambu C++ (R3) | 1 descoberta SSDP | 9 | **NÃO** |
| Moonraker Python (R3) | 7 | 3 | **NÃO** |
| Elegoo CC2 Python (R3) | 2 | 8 | **NÃO** |
| Demais marcas | 0 | 10 | **NÃO** |

Os PASS experimentais não equivalem aos testes nativos T3–T9. Câmera, AMS com mapeamento e envio com SHA confirmado no receptor não passaram. Nenhuma marca homologada em CLI + contrato.

## Erros reais preservados
E-01 Creality nozzle map 205; E-02 Snapmaker exit 0 sem G-code; E-03 Creality --export-3mf inválido; E-04 resolvido **somente no cubo** por normalização comprovada; E-05 QIDI 1 vetor de suporte versus 4; E-06 QIDI -100; E-07 Elegoo só T0; E-08 Elegoo exit 3 com stdout/stderr da app vazios (aviso libEGL não comprova causa); E-09 Snapmaker --need-gcode-file inválido.

**Pendências claras:** liberar a escrita de configuração somente da impressora virtual; vincular a biblioteca C++ ao CMake do Filuna e validar Windows; receber quatro 3MF oficiais multicor; extrair binários oficiais e calcular SHA. Relatório R3 e toda a evidência original permanecem intactos.
