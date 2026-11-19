# FILUNA — POLÍTICA CANÔNICA DE CONECTIVIDADE (rede, controle, câmera, cores)

**Decisão do proprietário:** 10/11/2026. Complementa `REGRA-CANONICA-MODULOS-FABRICANTE-SOB-DEMANDA-20261110.md`.

## Princípio

Rede, descoberta, envio, controle, câmera e sistema de cores de cada impressora são implementados **a partir de código aberto (AGPL) e de SDKs abertos já disponíveis**, mais os protocolos locais abertos que faltarem. O Filuna **não empacota nem redistribui componente proprietário fechado** (DLL, serviço de nuvem, binário auxiliar), e **não contorna autenticação/emparelhamento** de dispositivo. Cada fabricante é homologado por modelo.

## Base jurídica (resumo, não é parecer)

- OrcaSlicer, Bambu Studio, ElegooSlicer e AnycubicSlicerNext são AGPL-3.0; o Filuna também. A obrigação de abrir o código correspondente vale igual para todos.
- A AGPL **não** obriga a abrir todo protocolo, firmware ou serviço de nuvem desenvolvido de forma independente.
- O caso do `bambu_networking` (plugin fechado da Bambu; issue pública #10952, argumento da FSF sobre vínculo dinâmico formar obra única) é uma **controvérsia documentada, não uma infração comprovada**.
- Ponto decisivo para o Filuna: **um eventual descumprimento de AGPL por um fabricante não nos transfere licença** para redistribuir o código fechado dele. A reparação pertence aos detentores do direito do upstream. Portanto, fechado não entra no Filuna — independentemente do mérito da controvérsia.
- Não sou advogado; a auditoria de licença por componente distribuído é condição antes de qualquer release.

## Caminho por fabricante (fonte aberta; homologar por modelo)

| Fabricante | Caminho aberto adotado | Situação |
|---|---|---|
| Snapmaker (U1) | **Moonraker/MQTT** (Klipper nativo; `u1-klipper`/Fluidd). Código aberto na fonte: `MoonRaker.*`, `Bonjour.*`, `MQTT.*` | **DECIDIDO**. Exclui o binário fechado `snapmaker_control` |
| Elegoo | **Elegoo Link SDK** oficial aberto (`elegooofficial/Elegoo-Link`, confirmado existir): SDCP/MQTT/WebSocket, descoberta, envio, status, comandos; CANVAS a validar | CANDIDATO — conferir licença e cobertura antes de integrar |
| Anycubic (ACE Pro) | Implementação comunitária aberta de controle LAN e 4 slots ACE Pro citada na análise do dono | CANDIDATO — **localizar o repositório e verificar licença**; validar por modelo |
| Bambu | **Protocolo LAN aberto** + implementações comunitárias abertas de AMS; **sem** a DLL `bambu_networking` | CANDIDATO — localizar/verificar; usar o mesmo emparelhamento que o oficial pede |
| Creality, Flashforge, QIDI, Prusa | Código aberto da fonte + protocolo local aberto do modelo | A levantar por marca |

"CANDIDATO" = fonte aberta identificada mas ainda não auditada (licença, cobertura, teste real). Não é homologação.

## Homologação de conectividade (por modelo)

Descoberta na rede, conexão, envio, status e controle executados pelo código aberto do módulo, com o mesmo emparelhamento do fatiador oficial. Câmera visualizada localmente. Sem binário fechado e sem desvio de autenticação. Impressão física não bloqueia; é registrada à parte.
