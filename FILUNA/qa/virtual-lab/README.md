# Filuna Slicer — Laboratório de impressoras virtuais (LAN)

## Finalidade
Reproduzir **pacotes de descoberta e sessões de comunicação de rede** que o Filuna Slicer espera receber, sem instalar firmware ou conectar motores, resistências e hardware físico. Isso testa compatibilidade básica de **descoberta, identidade, conexão, autenticação, leitura e alguns comandos**; não certifica compatibilidade com impressoras reais.

## Segurança
- Ambiente isolado: bridge `br-filuna-lab` com rede `10.77.0.0/24`, sem NAT ou configuração de encaminhamento.
- **Nunca** exponha portas MQTT/UDP/WebSocket de impressoras ao IP público da VPS.
- Somente rode em ambiente de testes; os equipamentos virtuais não devem aparecer para clientes reais.
- Scripts do shell exigem root para criar namespaces e usar porta 80, mas **não instalam dependências nem modificam o Filuna**.
- Faça análise prévia se a faixa `10.77.0.0/24` já for utilizada por uma VPN ou rede privada.

## Pré-requisitos
Linux com `iproute2`, `python3` 3.10+, kernel com `bridge` e `network namespaces` habilitados. Sem Docker e sem bibliotecas Python adicionais.

## Executar
Dentro da pasta extraída:

```bash
sudo bash setup_bridge.sh
sudo bash run_lab.sh
```

Abra outro terminal e inicie o Filuna Slicer Windows via Wine, **no mesmo host Linux e namespace de rede padrão**. Abra Dispositivos > Procurar de novo. Espere a busca terminar.

| IP virtual | Dispositivo | Esperado | Acesso |
| --- | --- | --- | --- |
| 10.77.0.11 | Elegoo Centauri Carbon 2 | descoberta + conexão MQTT + status + comandos | porta 1883, código 123456 |
| 10.77.0.12 | Bambu Lab A1 | descoberta SSDP apenas | código real não utilizado; 8883 ainda não simulado |
| 10.77.0.13 | Moonraker/Klipper | identificação por GET /printer/info e consulta HTTP de status | porta 7125, sem chave |
| 10.77.0.14 | Elegoo Centauri Carbon original | descoberta UDP/SDCP apenas | WS 3030 ainda não simulado |

**Atenção:** Bambu exige MQTT/TLS 8883 para status; SDCP exige WebSocket 3030 para controle. Nesta etapa esses dois modelos são apenas cenários de **descoberta**. Não confundir 'detectado' com 'conectado'.

## Verificações

```bash
curl -s http://10.77.0.13:7125/printer/info
curl -s 'http://10.77.0.13:7125/printer/objects/query?print_stats&extruder&heater_bed&display_status'
curl -s http://10.77.0.11/__lab/state
sudo tcpdump -ni br-filuna-lab 'udp port 52700 or udp port 3000 or udp port 2021 or tcp port 1883 or tcp port 7125'
```

Para simular conexão perdida, pressione `Ctrl+C` no `run_lab.sh` e execute a busca de novo. Para testar estado printing em Moonraker:

```bash
curl -s -X POST 'http://10.77.0.13:7125/printer/print/start?filename=lab.gcode'
curl -s 'http://10.77.0.13:7125/printer/objects/query?print_stats&display_status'
```

Para verificar a rede do Wine, confirme que ele enxerga o adaptador `10.77.0.1/24`. Se a rotina `DiscoveryService.Subnets()` não listar essa interface, a busca automática pode ignorar toda a rede virtual apesar de os servidores responderem a IP direto.

## Encerrar e remover
Pressione `Ctrl+C` no terminal do `run_lab.sh` e depois:

```bash
sudo bash cleanup_bridge.sh
```

## Pontos a integrar no projeto Filuna
1. Verificar o `DiscoveryService.Subnets()` sob Wine, não apenas sob .NET nativo.
2. A descoberta da CC2 usa UDP broadcast 52700 e exige `result.sn` no JSON.
3. A CC2 exige registro MQTT no tópico `elegoo/{serial}/api_register`, `register_response`, pedidos `api_request` e respostas correlacionadas `api_response`.
4. A identificação Moonraker testa `GET /printer/info` na porta 7125.
5. O Bambu usa SSDP nos UDP 1990/2021 e status MQTT/TLS na 8883; estes são testes separados.
6. O SDCP responde ao broadcast `M99999` UDP 3000, mas a build atual o marca como `Identificada`, sem registro automático.
7. Salvar evidências por impressora/protocolo/etapa com timestamp, IP, número de série de **teste**, tempo de resposta e falha; não registrar credenciais.

## Limitações
- Não simula eletrônica, temperatura física, firmware completo, câmera nem desempenho mecânico.
- CC2 MQTT é um broker **mínimo e dirigido ao protocolo usado pelo cliente atual**, não implementa a especificação MQTT completa. Para certificação, usar um Mosquitto verdadeiro e, depois, confrontar com capturas autorizadas de impressoras reais.
- Este pacote foi construído como laboratório independente, não foi instalado nem executado no servidor de produção.

## Autoteste dos emuladores (sem criar rede)

```bash
python3 smoke_test.py
```

Esse autoteste usa `127.0.0.1` **apenas internamente** e não tenta conectar o Filuna, porque o cliente bloqueia loopback por segurança. Para integrar ao Filuna/Orca via Wine, use a bridge com `10.77.0.x`.