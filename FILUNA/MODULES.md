# FILUNA — Módulos de fabricante (matriz atualizada 05/11/2026)

**Decisão do proprietário:** um motor de fatiamento OrcaSlicer integral. Integrações **de comunicação e controle** de dez fabricantes via DeviceBridge, sem empacotar outros slicers inteiros, DLLs privadas ou fazer bypass de autenticação. Este documento **substitui a lista preliminar de 4 fabricantes** deste arquivo, mantendo histórico no Git.

**Contrato de referência:** [ARQUITETURA-DEVICEBRIDGE-10-FABRICANTES.md](ARQUITETURA-DEVICEBRIDGE-10-FABRICANTES.md), [REFERENCIAS-COMPATIBILIDADE-SIMPLYPRINT.md](REFERENCIAS-COMPATIBILIDADE-SIMPLYPRINT.md), [POLITICA-CONEXAO-LAN-CONTA-CHAVE.md](POLITICA-CONEXAO-LAN-CONTA-CHAVE.md), [MATRIZ-TESTES-ACEITE-100-POR-CENTO.md](MATRIZ-TESTES-ACEITE-100-POR-CENTO.md).

## Estrutura normativa de cada módulo (diretórios de código não criados ainda)

\`\`\`text
FILUNA/modules/<id>/
  MODULE.md           # fonte, SHA, arquivos, licença, capacidades, riscos, owner
  src/                # apenas código auditado, licenciado e necessário
  patches/            # pontos de integração mínimos com o Orca
  fixtures/           # mensagens sanitizadas positivas e negativas
  tests/              # unitários, integração LAN, CI e ensaios físicos
\`\`\`

Antes de criar código: \`MODULE.md\` descreve \`manufacturer_id, models, firmware_ranges, protocol, connection_methods, ports, discovery, auth, license_per_file, source_sha, capabilities, sdk_requirements, privacy, test_cases, rollback\`.

## Dez fabricantes prioritários (NÃO é ranking auditado de vendas)

| ID | Fonte principal de pesquisa | Modelo piloto | Estado do módulo Filuna |
|---|---|---|---|
| \`elegoo\` | [elegooofficial/ElegooSlicer](https://github.com/elegooofficial/ElegooSlicer), Orca \`ElegooLink.cpp\` | Centauri Carbon 2 | **PLANEJADO** — código parcial no core Orca, QA Filuna físico pendente |
| \`anycubic\` | [ANYCUBIC-3D/AnycubicSlicerNext](https://github.com/ANYCUBIC-3D/AnycubicSlicerNext) | Kobra 3 V2 (+ACE se houver) | **PLANEJADO** — preset Orca ≠ integração nativa |
| \`bambu\` | [bambulab/BambuStudio](https://github.com/bambulab/BambuStudio), LAN/SDK oficial quando permitido | Bambu A1 | **PLANEJADO** — plugin proprietário não redistribuir |
| \`creality\` | [CrealityOfficial/CrealityPrint](https://github.com/CrealityOfficial/CrealityPrint) | K1/K2 a fixar | **PLANEJADO** — firmware/CFS variam |
| \`flashforge\` | [FlashForge/Orca-Flashforge](https://github.com/FlashForge/Orca-Flashforge), Orca \`Flashforge.cpp\` | Adventurer 5M/AD5X a fixar | **PLANEJADO** — código LAN parcial presente no Orca |
| \`qidi\` | [QIDITECH/QIDIStudio](https://github.com/QIDITECH/QIDIStudio) | Plus4 | **PLANEJADO** — Moonraker por modelo/firmware |
| \`snapmaker\` | [Snapmaker/OrcaSlicer](https://github.com/Snapmaker/OrcaSlicer) | U1 | **PLANEJADO** — Moonraker U1 e multicor separados |
| \`prusa\` | [prusa3d/PrusaSlicer](https://github.com/prusa3d/PrusaSlicer), PrusaLink | MK4S/CORE One a fixar | **PLANEJADO** |
| \`ultimaker\` | [Ultimaker/Cura](https://github.com/Ultimaker/Cura) + API oficial | S-series a fixar | **PESQUISA** — licença/SDK/protocolo específico |
| \`raise3d\` | [Raise3D/ideaMaker](https://github.com/Raise3D/ideaMaker) + documentação OEM | Pro3/E2 a fixar | **PESQUISA** — direitos/SDK e hardware pendentes |

**Rótulo PLANEJADO:** nenhuma dessas dez marcas é homologada como *módulo nativo Filuna* simplesmente por ter um projeto público ou suporte de terceiro.

## Regras de implementação

1. **O motor Orca e seus perfis continuam intactos** salvo alteração mínima, registrada com upstream SHA e testes de regressão aprovados. Um módulo DeviceBridge **não altera algoritmos de G-code**.
2. Cada função efetiva usa \`Capability\` por modelo/firmware. Não habilitar câmera, pausa, material ou impressão se não comprovadas. \`OrcaPrinterAgent\` com retorno falso de sucesso é **P0 a corrigir**.
3. Para acesso: pesquisar LAN e portas documentadas, pareamento/chave na máquina e login/SDK OEM autorizado. SimplyPrint é **fonte de referência**, não software/API do Filuna. Sem interface permitida: export manual honesto, nunca bypass.
4. Nenhum pacote proprietário Bambu ou Flashforge é distribuído sem autorização e licença específicas. Repositório AGPL não implica que plugins não livres também sejam AGPL.
5. Cada módulo registra licença/NOTICE/SPDX por arquivo, SHA OEM fixado, histórico de merges, matriz de segurança e compatibilidade de firmware.
6. Todo módulo passa por testes de unidade/negativos, mocks, Windows real e dispositivo físico no escopo — **100% PASS, sem SKIP obrigatório**.
7. Para alterações de driver nativo, atualizar classe C/D, validar assinatura e rollback; dados de perfil autorizados são classe A/B conforme manifesto e natureza executável.
8. O modo remoto do **cliente** é bloqueado até homologação local e [ARQUITETURA-SERVIDOR-DADOS-REMOTO.md](ARQUITETURA-SERVIDOR-DADOS-REMOTO.md). Administrador Filuna só recebe incidentes/estado sanitizado e permitido, jamais controle da máquina.
