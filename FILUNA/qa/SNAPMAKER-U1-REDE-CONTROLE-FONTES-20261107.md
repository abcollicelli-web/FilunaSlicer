# Snapmaker U1 — mapa do código de rede/controle oficial v2.4.1 (07/11/2026)

**Estado:** fontes localizadas e inventariadas, **não integradas**, **não compiladas**, **sem prova de conexão ou impressão**. Este mapa prepara a homologação de Dispositivos/Preparar/Enviar da U1 usando o adaptador OEM completo, não reescrevendo um mock como recurso funcional.

## Fonte imutável
- Repositório oficial: https://github.com/Snapmaker/OrcaSlicer
- Tag: `v2.4.1`, commit Git `bac6fd5c1e1c23ddf057cf6471ba4ed7ed621c4c`.
- Auditoria automática da árvore **inteira** da versão oficial: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42316033247
- Inventário: 2.115 entradas sob auditoria, 1.978 mudanças/deltas candidatos preparados isoladamente, 101 itens para revisão de dependências/classificação/licença. **Não confundir inventário com integração ou revisão jurídica concluída.**

## Implementações reais do OEM que precisam ser preservadas e rastreadas no módulo nativo

| Subsistema | Origem na tag oficial | Estado |
|---|---|---|
| Gateway: transporte HTTP/WebSocket, conexão, ciclo de vida e fallback | `src/slic3r/Utils/GatewayService.cpp`, `.hpp` | SOURCE_DISCOVERED_NOT_PORTED |
| Protocolo JSON-RPC, saúde, respostas e reconexão | `src/slic3r/Utils/GatewayProtocol.cpp`, `.hpp` | SOURCE_DISCOVERED_NOT_PORTED |
| Dispositivo: leitura de identidade, bicos e perfis efetivos | `src/slic3r/Utils/GatewayDevice.cpp`, `.hpp` | SOURCE_DISCOVERED_NOT_PORTED |
| Autenticação/conta de gateway | `src/slic3r/Utils/GatewayAccount.cpp`, `.hpp` | SOURCE_DISCOVERED_NOT_PORTED |
| Processo/serviço de conexão e reinício | `src/slic3r/Utils/ConnectionProcessManager.cpp`, `.hpp` | DEPENDENCY_REVIEW_REQUIRED |
| Nozzles, extrusoras, valores válidos e flows `standard`/`high_flow` | `src/libslic3r/SSWCPProtocol.cpp`, `.hpp` | SOURCE_DISCOVERED_NOT_PORTED |
| Fatiador/dispositivo Moonraker e envio/controle MQTT | `src/slic3r/Utils/MoonRaker.cpp`, `.hpp`; `src/mqtt/` | TRANSITIVE_DEPENDENCIES_REVIEW |
| Interface/cadastro físico e seleção de perfil | `src/slic3r/GUI/SMPhysicalPrinterDialog.cpp`, `.hpp` | ADAPTER_AND_UI_MAPPING_PENDING |
| Envio para impressora e transferência | `src/slic3r/GUI/SendToPrinter.cpp`, `src/slic3r/GUI/Jobs/PrintJob.cpp` | SOURCE_DISCOVERED_NOT_PORTED |

**Dependências devem ser coletadas recursivamente** por includes, CMake, tipo/ABI, protocolo, runtime, licença e hash; o nome da marca no arquivo não determina sozinho sua pertinência. Alguns arquivos genéricos, inclusive nomes BBL no fork OEM, não são automaticamente exclusivos da U1. Arquivo desconhecido fica em `REVIEW`, nunca descartado silenciosamente. Fonte original não deve ser editada para mascarar falhas.

## Contrato real detectado na fonte, para o futuro simulador

No `GatewayService.hpp`, a configuração usa o host local `127.0.0.1` para o **serviço gateway local da aplicação** e define `/api/health`, `/ws`, `/api/cache/all`, `/api/account`. **Isso não é uma afirmação de que a impressora U1 exponha essas rotas diretamente na LAN.** Separar serviço local gateway, transporte da impressora e autorização.

- `GET /api/health`: o código valida `status=ok` ou `ok=true`, ambos os componentes `ipc_server=ok` e `web_server=ok`, `server_url.base_url`, e tipos válidos em `device_connected` e `sn` quando presentes. Retorno inválido bloqueia conexão.
- `GET /api/cache/all`: `GatewayDevice::query_machine_info()` lê `data.info.product.machine_type` (fallback `data.identity.productName`), `data.info.product.nozzle_diameter`, `data.identity.name` e `data.objects` para inferência de extrusoras. Identidade sem modelo deve ser rejeitada; contagem de bicos e volume deve ser comprovada antes de selecionar os perfis.
- `/ws`: frames JSON-RPC versionados têm `id`, `method`, `params`; parsing de notificações e respostas em `GatewayProtocol.cpp`, com reconexão e erros. Precisará de teste de fechamento, timeout, autenticador, associação e canais de status.
- Qualquer credencial/certificado deve vir do usuário/aplicativo de forma segura, jamais ser copiado dos artefatos, G-code ou logs. O gateway local não autoriza por si só imprimir numa U1 remota.

## Sequência de aceitação, sem substituir o motor por scripts
1. Disponibilizar o conjunto OEM **completo e juridicamente revisado** de rede/acesso/controle (e dependências transitivas) ao adaptador nativo Filuna, evitando DLL fechada não autorizada.
2. Resolver a partir do gateway/dispositivo a identidade do modelo, número de bicos, firmware, material e versão; conectar automaticamente ao catálogo inteiro gerado em `FILUNA/tools/oem_universal_inventory.py`. Sem tabelas fixas de U1.
3. Exercitar `GET health` e `cache/all` e JSON-RPC WebSocket com falso estado online, falha de autorização, desconexão, atraso, duplicidade de identidade; evidências de protocolo/ACK exigidas por capacidade.
4. Demonstrar na aplicação Filuna efetiva **Dispositivos → Preparar → Fatiar → Enviar**, associando o módulo OEM completo, preservando 3MF/macros, e G-code comparável ao oficial. `code staged` / `mock PASS` não são integração nem paridade.
5. Controle remoto de impressão, câmera, pausa e retomada somente após contrato/ACK interno comprovado e autorização segura; impressão física real apenas no beta supervisionado após a compilação final e gates internos.

**Acesso/rede/câmera, envio, estado e autenticação de U1 neste documento:** todos **NOT_RUN/BLOCKED**; nenhum PASS atribuído. O Bambuddy Bambu permanece o único adiamento voluntário de agenda, sem interferir neste modelo.
