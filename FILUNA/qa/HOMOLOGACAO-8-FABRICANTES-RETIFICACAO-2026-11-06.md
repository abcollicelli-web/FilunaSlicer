**CLASSIFICAÇÃO FINAL:** todas as células T3 são NÃO EXECUTADO: conexão TCP ao Moonraker genérico não é teste da GUI Filuna. Histórico de erros não gera erros novos.

# FILUNA — RETIFICAÇÃO DE HOMOLOGAÇÃO 8 FABRICANTES — 06/11/2026

**Este documento substitui a classificação preliminar de 88 erros e BLOQUEADOS.** A fonte detalhada da nova rodada está em `/srv/site/incoming/filuna-local-oem-qa-20261106/HOMOLOGACAO-LAB-FILUNA-8-FABRICANTES-REAL-2026-11-06.md`. Regras: PASS e FALHA apenas com evidências reais; BLOQUEADO apenas após tentativa com pré-requisito verificado; o que não foi tentado é **NÃO EXECUTADO** e não gera erro; T2/T11 não automatizados ficam **PENDENTE DE CAMPO**. Nenhum merge, release, deploy, QA físico ou build homologado.

## Matriz corrigida — modelo de referência, T1 a T11

| Fabricante | T1 | T2 | T3 | T4 | T5 | T6 | T7 | T8 | T9 | T10 | T11 | Veredito |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Bambu A1, AMS lite | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Elegoo CC2 Combo, CANVAS | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Flashforge AD5X, IFS | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| QIDI Q2 Combo, QIDI Box | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Creality K2 Plus Combo, CFS | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Snapmaker U1, SnapSwap | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Prusa CORE One + MMU3 | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | EM VALIDAÇÃO |
| Anycubic Kobra S1 Combo, ACE Pro | NÃO EXECUTADO | PENDENTE DE CAMPO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | NÃO EXECUTADO | PENDENTE DE CAMPO | EM VALIDAÇÃO |

## Evidências positivas reaproveitadas com seu escopo

- **Bambu A1 Mini (auxiliar T1):** corpus de 5 pares em duas pastas com SHA-256 bruto idêntico; cubo20 = `42864cc2b2dde3677bc16763599266a94e5ed8fbeaca623309a2099c0d04446f`. Não prova A1/AMS lite com 4 cores.
- **Moonraker (auxiliar T5–T7):** upload sintético, iniciar/status/pausar/retomar/cancelar executados com respostas HTTP `ok`, estado final `ready`. Não prova execução feita por Filuna nem um modelo QIDI/Creality.
- **Bambuddy:** Bambu A1 em `10.77.0.15`, Mini em `.16`; portas SSDP, Bind, MQTT/TLS 8883, FTPS 990; verificação TLS por `openssl s_client` deu `Verify return code: 0 (ok)`. 
- **Elegoo CC2:** descoberta UDP 52700, autenticação MQTT 1883 `CONNACK 20020000`; clientes sintéticos.
- **Elegoo CC original SDCP auxiliar:** WebSocket 3030 handshake/pong; upload sintético com MD5 correto HTTP 200/incorreto HTTP 422; MJPEG sintético 3031.

## Tentativas novas e impedimentos observados

Sondagem executada via Python `socket.connect_ex((IP, port))` com timeout 1s:
```text
Bambu A1 10.77.0.15 8883 CONNECTED 0
Elegoo CC2 10.77.0.11 1883 CONNECTED 0
Creality K2 10.77.0.13 9999 ERROR 111
QIDI Q2 10.77.0.13 7125 CONNECTED 0
Snapmaker U1 10.77.0.13 1884 ERROR 111
Prusa CORE One 10.77.0.13 80 ERROR 111
Flashforge AD5X 10.77.0.13 8898 ERROR 111
Anycubic Kobra S1 10.77.0.13 9883 ERROR 111
```
`10.77.0.13` é Moonraker genérico, NÃO instância OEM dessas marcas. As recusas TCP identificam falta de endpoint nesse host genérico, não falha do protocolo/printer. Essas sondagens não executaram o T3 da GUI Filuna: todos os T3 ficam NÃO EXECUTADO.

A tentativa de executar `timeout 18 xvfb-run -a wine64 .../CrealityPrint.exe --help` foi barrada pela ferramenta com texto `Esta ferramenta foi bloqueada pelas configurações de segurança da OpenAI. Verifique novamente o que está enviando.`; logo a CLI NÃO iniciou e não há saída/exit code da Creality a atribuir a essa tentativa. A execução `wine64 --version` foi barrada da mesma forma.

## Falhas históricas comprovadas, não repetidas nesta rodada

- **E-01:** Creality Print 7.3.1 com projeto genérico/Ender-3 V3, `Filament nozzle mapping is incomplete`, código 205. Preparar 3MF específico da K2 Plus com presets oficiais e mapeamento filamento→bico.
- **E-02:** Snapmaker Orca 2.4.1 terminou com código 0 sem produzir G-code. Reproduzir com debug máximo e presets oficiais U1 carregados do portátil, com linha de comando completa.

Nenhuma outra entrada E-XX. Para Anycubic, protocolo MQTT LAN e câmera FLV são caminhos de implementação comunitária: T3 NÃO EXECUTADO; não confundir Kobra 3 V2 modelId 20027 com Kobra S1 modelId 20025. Pesquisa/teste no modelo S1 ainda NÃO EXECUTADO; o erro de conexão ao host Moonraker é apenas evidência auxiliar e não configura T3 BLOQUEADO.

**T1 regra nova:** cada marca deve criar seu próprio 3MF via CLI do portátil oficial usando os seus presets oficiais machine/process/filament; usar o mesmo artefato no oficial e no Filuna, nunca reutilizar 3MF entre marcas. T1/T3–T10 seguem pendentes de execução integrada por falta de binário Filuna GUI identificado, fixture 4 cores e simuladores OEM completos. **Nenhum fabricante homologado e nenhum build de homologados criado.**
