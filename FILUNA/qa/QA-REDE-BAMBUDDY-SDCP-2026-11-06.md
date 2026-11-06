# FILUNA — QA rede Bambu Bambuddy e Elegoo SDCP (06/11/2026)

## Natureza da evidência
Laboratório exploratório no Ubuntu conectado via SentinelX, bridge `br-filuna-lab` em `10.77.0.0/24`. Não é homologação Windows nem física. Nenhum merge, release, instalação nos clientes ou deploy de produção foi realizado. A referência detalhada permanece no servidor em `/srv/site/incoming/filuna-local-oem-qa-20261106/QA-REDE-BAMBUDDY-SDCP-CONTINUIDADE-2026-11-06.md`.

## Bambu — Bambuddy Virtual Printer
- Código Bambuddy obtido do repositório `maziggy/bambuddy`, commit `75c9c2db740b74d17803c82119803f6ae7088783`; ambiente Python isolado, painel em `127.0.0.1:18080` (sem exposição pública).
- Impressoras virtuais de laboratório: A1 `10.77.0.15` (N2S) e A1 Mini `10.77.0.16` (N1), modo `archive`, ambas com `running=true`.
- Portas `3000` Bind, `3002` Bind/TLS, `8883` MQTT/TLS, `990` FTPS acessíveis em ambos IPs privados. Teste `openssl s_client` validou a CA e obteve `Verify return code: 0 (ok)` nas portas `8883` e `990` de ambos os modelos. SSDP UDP `2021` em escuta. **Não** confundir abertura e handshake TLS com envio de impressão pelo Bambu Studio ou Filuna.
- Permissão `CAP_NET_BIND_SERVICE` aplicada apenas ao executável Python exclusivo do laboratório para a porta 990; não foi modificada a configuração de portas privilegiadas do sistema.
- CA do Bambuddy com fingerprint SHA256 `21:9D:41:95:DA:D3:FF:7F:7B:7B:BE:86:9B:C5:F9:75:2D:C9:77:A4:E9:5F:0A:2E:E5:08:2A:27:C1:EF:A2:A3`, **adicionada sem substituir** os certificados originais às cópias `printer.cer` do Bambu Studio 02.09.01.70 e Orca portátil de teste em `/var/tmp/filuna-qa-wine`; backups originais `printer.cer.pre-bambuddy-20261106`, seis certificados após concatenação. Não houve modificação do aplicativo distribuído.

## Elegoo SDCP
- Mantida descoberta `M99999` por UDP 3000 no namespace `flab-sdcp`, IP `10.77.0.14`.
- Sidecar de teste `sdcp_v3_sidecar.py`: WebSocket `ws://10.77.0.14:3030/websocket`, resposta HTTP `101`, heartbeat `ping/pong`, tópicos de atributos/status, limite configurado de 5 sessões; comandos não implementados recusados. **Não** é emulação completa do firmware.
- Upload de laboratório em `/uploadFile/upload`: multipart, offsets, tamanho, UUID, MD5 de arquivo completo. Fixture sintética de 24 bytes: HTTP 200 com hash correto e HTTP 422 com MD5 errado. Nenhuma compatibilidade OEM integral inferida.
- Servidor MJPEG sintético em `10.77.0.14:3031/mjpeg`: HTTP 200 e frames JPEG. Não é prova de câmera física nem RTSP OEM.
- Logs JSONL locais `network-logs/sdcp-v3-lab.jsonl` e `network-logs/sdcp-camera.jsonl`.

## Moonraker e demais simuladores
- Autoteste local `smoke_test.py`: **5/5 PASS** — CC2 UDP, CC2 MQTT, Moonraker HTTP, SDCP UDP, Bambu SSDP.
- Moonraker `10.77.0.13:7125`: identidade e status; upload sintético, start, pause, resume e cancel responderam `ok`; final `ready`. Apenas API emulada.
- CC2 `10.77.0.11`: UDP 52700 retornou serial sintético e MQTT 1883 autenticação `CONNACK 20020000`.

## Comparação de logs
- Utilitário local `compare_network_sessions.py` normaliza timestamps/IDs e remove parte dos campos sensíveis. Autoteste de igualdade semântica aprovado e teste de diferença intencional detectado.
- **NOT RUN:** sessões completas de Bambu Studio oficial **versus executável Filuna Windows identificado**; evidência atual decorre de clientes sintéticos e dos próprios emuladores. Não há pares de logs equivalentes de aplicativos; nenhum resultado de conformance oficial×Filuna pode ser marcado PASS.
- **NOT RUN:** impressoras físicas, GUI Windows 10/11, câmera OEM, segurança multimaterial e validação de firmware.

## Gate para próxima etapa
1. Identificar precisamente o binário Filuna Windows e executá-lo isolado do Orca local; usar a mesma fixture e perfil do fatiador oficial.
2. Logar ambas as sessões por dispositivo e direção (descoberta, vínculo, status, arquivo/hash, iniciar, pausar, retomar, cancelar), com expurgo de credenciais e dados sensíveis.
3. Comparar logs com normalização explicitamente permitida; diferença, evento ausente ou teste não executado impedem PASS.
4. Separar validação exploratória Wine da homologação Windows real/hardware.
5. Manter gates G0–G6, sem publicação ou merge antes dos aceites exigidos.

**Situação:** infraestrutura virtual Bambu e SDCP parcialmente validada; homologação do novo Filuna Slicer permanece pendente.
