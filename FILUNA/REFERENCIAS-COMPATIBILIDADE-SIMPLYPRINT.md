# FILUNA — Pesquisa de compatibilidade externa (SimplyPrint e protocolos)

**Registro:** 2026-11-05 · **Tipo:** fonte terciária de pesquisa de integração, não código a copiar nem compatibilidade Filuna demonstrada.  
**Decisão do proprietário:** usar plataformas existentes, especialmente **SimplyPrint**, como fonte de pesquisa de modelos e caminhos de conexão para expandir o Filuna, além dos fatiadores OEM, APIs documentadas, OctoPrint e Moonraker.

## 1. Critério de confiança em três níveis

1. **Evidência de mercado (E1):** a SimplyPrint anuncia uma impressora como compatível; é indício de viabilidade e norteia a investigação. A lista distingue Verified, Compatible, Expected e Not confirmed. **Nunca mapear essa evidência diretamente para “Compatível com Filuna”**.
2. **Evidência de protocolo (E2):** documentação pública de portas, transporte e autenticação + implementação legalmente admissível/SDK; teste seguro, na LAN de propriedade ou autorização do usuário, confirma handshake/status ou envio.
3. **Evidência de produto (E3):** implementação **no Filuna** com binário SHA identificado, dispositivo/firmware real e 100% dos testes obrigatórios PASS para cada capacidade anunciada. Só E3 permite anunciar compatibilidade nativa Filuna.

**Fonte pública:** https://simplyprint.io/compatibility/ lista modelos/estados de suporte; https://simplyprint.io/setup-guide/ e https://simplyprint.io/setup-guide/methods/ discriminam rede direta, SimplyPrint Client, OctoPrint, Klipper e firmware. Pesquisar o **modelo exato**, não inferir suporte da marca toda.

## 2. Arquitetura da SimplyPrint — lição que podemos aprender

A documentação informa que o **SimplyPrint Client** é um pequeno aplicativo rodando em Windows/Linux/macOS/Raspberry Pi na **mesma LAN** das impressoras, identificando marcas diferentes e usando APIs locais. O Client pode ligar diversas impressoras ao serviço SimplyPrint. A operação local da impressão continua no equipamento se o client/cloud ficar offline; o monitoramento remoto, não. Documentação: https://simplyprint.io/setup-guide/methods/simplyprint-client.

**Implicação para Filuna:** o processo Windows Filuna já estará rodando no PC, então deve exercer a função de bridge **local e primeira**, sem obrigar um segundo aplicativo residente nem conta na SimplyPrint. Para operação remota com o PC desligado seria necessário agente dedicado seguro ou integração oficial independente: não prometer conectividade que o equipamento não fornece.

**Restrição:** não copiar nem tentar chamar o software, a nuvem ou as credenciais SimplyPrint. Reutilizar apenas **informações públicas, protocolos autorizados e padrões arquiteturais**, reproduzindo pesquisa e testes próprios. O selo “compatible” pertence à SimplyPrint, não ao Filuna.

## 3. Pesquisa documentada de portas locais — SOMENTE referência, não varredura autorizada

A página https://simplyprint.io/hardware-network-requirements publica portas que **seu próprio Client usa** para se comunicar com impressoras na LAN, por fabricante. Portas abaixo são **pistas de protocolo**, não provas de que qualquer firmware/modelo responda nem de que o Filuna deva usar todas.

| Fabricante/protocolo | Portas de referência divulgadas | Transporte/uso declarado | Cuidado |
|---|---|---|---|
| Bambu Lab | TCP 8883, 990, 322, 6000 | MQTT/TLS, FTPS, RTSP; controle/arquivo/câmera | autorização e modo LAN/firmware podem restringir |
| Prusa, Duet, UltiMaker | TCP 80 | HTTP API local | firmware e auth por marca, nunca interface idêntica |
| Klipper/Moonraker, Snapmaker | TCP 7125 | HTTP + WebSocket | alguns fabricantes fecham ou personalizam Moonraker |
| Creality | TCP 9999, 80, 7125, 8000, 1984, 8080 | WebSocket/HTTP e câmera | CFS pode exigir outro driver |
| Anycubic | TCP 9883, 18910, 18088 | MQTT/HTTP API local | Kobra V2/ACE exigem prova por firmware |
| Elegoo | TCP 3030, 1883, 8080; **UDP 52700** | WebSocket/MQTT/HTTP/câmera | não confundir com todos os protocolos SDCP por geração |
| Flashforge | TCP 8898, 8899, 8080; **UDP 19000, 48899** | TCP/HTTP/câmera/descoberta | o Orca já implementa parte do LAN |

**Descoberta:** documentação SimplyPrint cita mDNS multicast **224.0.0.251:5353** e SSDP **239.255.255.250**, com portas **1900/2021**. VLAN ou rede convidada pode impedir descoberta; endereço IP manual é fallback. A tabela não é convite para explorar serviços públicos. **Executar sondagem apenas em impressora do usuário, LAN privada e mediante consentimento, com timeout, limites e sem tentativa de senha.**

Também consultar:
- https://simplyprint.io/integrations: separa OctoPrint/Moonraker/Mainsail/Fluidd e gestão de impressão.
- https://simplyprint.io/compatibility/check: assistente de método por fabricante/modelo.
- https://simplyprint.io/hardware-network-requirements: separa LAN da impressora de portas HTTPS da nuvem SimplyPrint. NÃO direcionar mensagens do Filuna para servidores SimplyPrint sem integração contratada.

## 4. Provas de mercado especialmente úteis aos dois primeiros testes Filuna

### Elegoo Centauri Carbon 2

**Fonte por modelo:** https://simplyprint.io/compatibility/elegoo-centauri-carbon-2  
**Estado na SimplyPrint:** \`Compatible\`, via SimplyPrint Client, **não** Moonraker/OctoPrint padrão segundo a página. A plataforma relata leitura de câmera, progresso, temperatura, envio/início, pausa/retomar/cancelar e, com acessório, mapeamento do Elegoo Canvas. Recursos que ela declara indisponíveis a software de terceiros incluem envio arbitrário de linhas de G-code, ajuste de Z-offset e nivelamento remoto.

**Decisão de QA:** examinar \`ElegooLink.cpp\` já presente no Orca e o \`src/slic3r/Utils/Elegoo/\` do ElegooSlicer, de origem autorizável. Testar cada capacidade em **CC2 real com firmware identificado**; suporte SimplyPrint (E1) não garante mesma capacidade no Filuna (E3). Para câmera e Canvas, protocolos, autenticação e formatos reais são subtarefas separadas.

### Anycubic Kobra 3 V2

**Fonte por modelo:** https://simplyprint.io/compatibility/anycubic-kobra-3-v2  
**Estado na SimplyPrint:** \`Compatible\`, via SimplyPrint Client. Há também modo Moonraker com **firmware comunitário Rinkhals**, que **não deve ser requisito obrigatório** no Filuna nem modificado automaticamente. A tabela distingue os caminhos: pelo SimplyPrint Client a plataforma anuncia câmera, envio/início, pausa/retomar/cancelar e, com ACE, mapeamento e sincronização de filamento; via Moonraker comunitário alguns recursos multimaterial não estão disponíveis.

**Decisão de QA:** priorizar o firmware original com API LAN autorizada/conta oficial se existirem, pesquisar documentação das portas e do AnycubicSlicerNext; tratar \`host_type=octoprint\` em perfil de slicing como **não equivalente** a integração nativa. Nenhum comando de início será disparado sem ACK verificável.

### QIDI Plus4 e Snapmaker U1

- https://simplyprint.io/compatibility/qidi-plus4 classifica QIDI Plus4 como compatível por Moonraker/Klipper sem agente extra; status, câmera e controles de impressão são anunciados, mas não inferir QIDI Box automático.
- https://simplyprint.io/compatibility/snapmaker-u1 distingue Snapmaker U1 por SimplyPrint Client ou Moonraker; pelo Client a plataforma anuncia mapeamento de materiais, enquanto a conexão Moonraker não disponibiliza a mesma sincronização do sistema multicor. Filuna deverá testar essa distinção.

## 5. Matriz de pesquisa obrigatória — fabricantes e fontes

| Marca | Fonte principal | Referência independente prioritária | Pesquisa adicional |
|---|---|---|---|
| Elegoo | ElegooSlicer + ElegooLink | SimplyPrint Centauri Carbon 2 | protocolo LAN e SDCP por firmware |
| Anycubic | AnycubicSlicerNext + API pública | SimplyPrint Kobra 3 V2 | API/conta oficial, ACE e LAN Key |
| Bambu | Bambu Studio + SDK autorizado | SimplyPrint Client Bambu LAN vs cloud | Bambu Connect / firmware autorizado |
| Creality | Creality Print | SimplyPrint Creality K1/K2/CFS | Moonraker/Creality LAN + CFS específico |
| Flashforge | Orca Flashforge + Orca \`Flashforge.cpp\` | SimplyPrint Flashforge | LAN upload/ACK, plugin fechado não copiar |
| QIDI | QIDI Studio/Slicer | SimplyPrint QIDI Plus4 | Klipper e QIDI Box por firmware |
| Snapmaker | Snapmaker Orca | SimplyPrint Snapmaker U1 | Moonraker U1, câmera e ferramenta |
| Prusa | PrusaSlicer + PrusaLink | SimplyPrint Prusa | APIs/documentos oficiais |
| UltiMaker | firmware/API oficial | SimplyPrint UltiMaker | API local/comercial e permissões |
| Raise3D | documentação oficial/SDK | pesquisa em catálogos independentes | reuso e autorização antes de claim nativo |

**Método de encerramento:** para cada modelo/firmware, tentar as opções **que têm suporte documental e autorização**: descoberta LAN → API local → pareamento/código/chave LAN → conta/SDK oficial → adaptador genérico autorizado → export manual. Ordem de testes otimizada por fabricante. Documentar tentativas falhas; **não iniciar exploração de portas públicas nem furar autenticação**. Se E3 faltar, a capacidade segue \`NOT VERIFIED\`.

## 6. Matriz de funcionalidades públicas ≠ promessas Filuna

A SimplyPrint distingue recursos por **método de conexão**, não só fabricante. Para um mesmo modelo:
- visualizar webcam depende de hardware, codec, endpoint e permissão;
- pause/resume/cancel podem funcionar quando envio arbitrário de G-code não funciona;
- iniciar impressão pode estar disponível com ACK sem permitir movimento manual;
- Mapear AMS/ACE/CFS/Canvas requer modo específico e sistema físico presente;
- recurso que exige firmware comunitário ou modificação do equipamento **não será apresentado como capacidade padrão no Filuna**.

Cada registro no Filuna deve guardar: \`model_id\`, \`firmware\`, \`connection_mode\`, \`reference_url\`, \`reference_status\`, \`Filuna implementation status\`, \`test_case IDs\`, \`tested_at\`, \`evidence URLs\`. O cliente vê apenas capacidades **Filuna implementadas e validadas**, nunca o marketing de terceiro.

## 7. Critérios de passagem G0/G1/G3/G4 para esta pesquisa

- [ ] Para cada uma das dez marcas e seus modelos pilotos: link oficial + fonte de compatibilidade independente + fonte de protocolo, todas acessíveis e versionadas por data/commit.
- [ ] Para cada modelo/firmware: método(s) LAN/conta/chave identificados, requisitos e restrições descritos, sem prometer código que não seja aberto.
- [ ] Para as CC2 e Kobra 3 V2: criar matriz câmera, start, pause/resume/cancel, temperaturas, filamento e estado por **método de conexão**, sem extrapolar.
- [ ] Teste de descoberta seguro em LAN do usuário, sem usar API/cloud de terceiros não autorizada; respostas com evidência sanitizada.
- [ ] Drivers reais só aprovados após protocolo/capability, validação positiva/negativa e ensaio físico na impressora.
- [ ] Cada claim público da Filuna atribuído a E3, não a E1/E2.
- [ ] Registrar recusas, SDKs indisponíveis, firmware restritivo e alternativas aprovadas.

**Conclusão:** SimplyPrint prova que existem **abordagens viáveis** para muitas impressoras que parecem fechadas, inclusive bridge local e diferentes métodos de autenticação. **Não prova** que Filuna possa copiar seu código ou usar o mesmo protocolo em qualquer firmware. A meta é reproduzir **comportamento** por implementação própria, segura e devidamente licenciada, com 100% dos testes definidos PASS antes de chamar uma integração de nativa.
