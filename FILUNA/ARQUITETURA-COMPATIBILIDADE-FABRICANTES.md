# Filuna Slicer — arquitetura de compatibilidade por fabricante

**Decisão do proprietário, 06/11/2026.** Estado: REGRA DE PROJETO APROVADA; implementação por fabricante ainda depende de provas. Este documento não autoriza distribuir código proprietário nem declarar dispositivos homologados.

## Princípio

**Orca = motor-base livre de fatiamento; Filuna = camada própria de interoperabilidade e integração com implementações oficiais de fabricantes**, quando tecnicamente compatíveis e juridicamente reutilizáveis. O objetivo não é mudar apenas nomes e perfis: reconhecer modelos/firmware reais, conectar, autenticar, transmitir, observar, comandar e gerar arquivos válidos para aquele equipamento.

O motor livre precisa continuar funcional sem assinatura. Integrações comerciais Filuna são separadas dos direitos do motor e sujeitas às licenças aplicáveis.

## Adaptadores por fabricante / família / modelo

Cada adaptador terá contrato explícito:
- **identidade:** fabricante, modelo, variante, firmware, serial pseudonimizado, capacidades e protocolo correspondente;
- **descoberta:** SSDP/mDNS/UDP ou IP manual, endereçamento e tratamento de duplicatas;
- **transporte e credenciais:** MQTT, MQTT/TLS, SDCP WebSocket, Moonraker HTTP/WS, OctoPrint HTTP, FTPS ou outro meio *efetivamente suportado*; armazenamento de credenciais protegido, falha de autenticação segura;
- **estado:** online/offline, temperatura, camada, tempo restante, erros e sensores, sem inventar telemetria;
- **controles:** comandos tipados e permitidos por modelo e estado, confirmação quando necessária, falha segura;
- **arquivos:** G-code/3MF e outros formatos aceitos, limites, upload, checksum/verificação, retomada e falha de transferência;
- **vídeo:** endpoint/stream negociado por modelo e firmware, autenticação, limites de rede, nunca assumir que uma câmera está disponível;
- **perfil e fatiamento:** extração autorizada de perfis oficiais; mesa, bico, altura, hotend, filamentos, temperatura, aceleração, velocidade, purga, multimaterial, start/end G-code, comandos e recursos específicos, com herança e proveniência;
- **capacidade comprovada:** cada função marcada NOT_RUN / PARTIAL_SIMULATED / FILUNA_INTEGRATION_PASS / HARDWARE_PASS, com SHA, evidência, versão e observações.

Nunca confundir família de impressora com compatibilidade universal. Ex.: Elegoo Carbon original usa SDCP WS enquanto CC2 apresenta MQTT; não intercambiar adaptadores.

## Fontes upstream e licenciamento

Pesquisar e fixar versão/commit do software oficial, dados de perfis, protocolos documentados e licenças. **Código só entra no Filuna depois de auditoria de direitos, dependências, compatibilidade GPL/AGPL e disponibilidade de fonte quando exigida.** Evitar distribuição de SDKs, certificados, chaves e binários proprietários sem permissão. Quando não houver licença compatível, implementar um adaptador independente e interoperável com base em APIs e comportamentos documentados, com testes, sem copiar código proibido.

Famílias priorizadas: Bambu Lab, Elegoo, Creality, Flashforge, QIDI, Klipper/Moonraker e OctoPrint. Adaptador por família/modelo; extensões plugáveis, identificação por capacidades, sem rotular fornecedor inteiro como homologado.

## Estratégia de validação

1. **Fontes:** identificar release oficial com hash, modelo/firmware, perfis e dependências.
2. **Perfis:** comparar **valores efetivos após herança**, não apenas JSON isolado; preservar comandos específicos; registrar divergências.
3. **Fatiamento comparativo:** executar binários oficial e Filuna com **o mesmo fixture, máquina, bico, material e parâmetros equivalentes**. Testar permutação de perfis para distinguir diferenças do motor e do preset.
4. **Semântica:** comparar por camada trajetória, extrusão, estado térmico, ventilação, aceleração, retrações, purga, macros e start/end; diferenças relevantes precisam ser compreendidas, não normalizadas para PASS.
5. **Integração em LAN virtual:** descoberta, autenticação correta/incorreta, identificação, monitoramento, comandos, vídeo simulado, arquivos, desconexão/reconexão e corrupção; registrar o que passou no *emulador* separadamente do que passou no **código real Filuna sob Wine**.
6. **Fila física final:** firmware real, câmera, transferência, comportamento térmico/sensores, movimentos e impressão em equipamento real. Simulação bem-sucedida = PARTIAL_SIMULATED, nunca aprovação física automática.

## Caso real CC2 — 06/11/2026

Referência oficial: `elegooofficial/ElegooSlicer` v1.5.3.5, portátil Windows oficial; referência Filuna: executáveis compilados da branch do projeto. Ambos rodaram via Wine. Quatro combinações de binário × perfis foram fatiadas com OBJ cubo 20mm, bico 0,4, PLA, 0,20 mm.

Diferenças relevantes que **persistiram ao trocar perfis**:
- oficial: 209.800 bytes, 5.424 movimentos G1; Filuna: 255.165 bytes, 7.532 movimentos G1;
- `SET_VELOCITY_LIMIT`: oficial 428 / Filuna 201; `G92`: oficial 3 / Filuna 103;
- início `M204 S1000` oficial vs `M204 S5000` Filuna; limite explícito máximo de feed 20.000 vs 30.000 mm/min (não equivale à velocidade real);
- 100 camadas em ambos, mas trajetórias e cálculos diferentes.
- `M6211`, `G180`, `BED_MESH_CALIBRATE` e macros Klipper/Elegoo devem ser reconhecidos semanticamente.

**Veredito: PARTIAL_COMPARISON_MISMATCH.** Não declarar G-code equivalente, nem liberar impressão remota/segura por esses testes. Investigar diferenças de motor/valores herdados; reproduzir G-code comparável e executar integração Filuna de verdade antes do teste físico final.

## Governança

Documentar no **mesmo** `FILUNA/REGISTRO-CONTINUIDADE-ETAPAS.md`; testes e adaptadores em branches/PRs; sem sobrepor a branch ativa do Grok; sem merge/deploy sem passagem pelos portões. A tela do site deve mostrar estados separados por dispositivo, não um PASS geral.
