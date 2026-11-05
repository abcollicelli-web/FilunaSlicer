# FILUNA G0 — Riscos, licença e proveniência OEM (documental)

**ID** FLN-G0-OEM-RISK-2026-11-05.1  
**Atualizado** 2026-11-05 23:20:00 CET  
**Gate** G0 (risco/proveniência). Implementação DeviceBridge = gates posteriores.

Fontes: PLANO §8; REGRA fronteiras 5–6; CONFORMIDADE-LEGAL; ARQUITETURA-DEVICEBRIDGE; POLITICA-CONEXAO-LAN; PROTOCOLOS W (legado referência).

## Política transversal (RESOLVIDO)

| Regra | Fonte | Efeito F |
|---|---|---|
| Sem bundling de plugin rede proprietário (Bambu/Flashforge) sem direito | REGRA §6; PLANO domínio segurança | Não empacotar `bambu_networking` / Flashforge closed |
| LAN / conta / chave oficiais antes de qualquer bypass | POLITICA-CONEXAO-LAN | Pesquisar meios autorizados; sem engenharia de contorno |
| Stub `BAMBU_NETWORK_SUCCESS` proibido como sucesso real | DEVICEBRIDGE P0; PLANO §8 | UNSUPPORTED/TIMEOUT até ACK real |
| Admin Filuna sem controle de impressoras | REGRA §servidor; CF-05 | SRV-04/05 read-only |
| AGPL créditos Slic3r→…→Orca→Filuna | LICENSE / REGRA | Mantidos |

## Matriz por fabricante (10)

| OEM | Código aberto reutilizável no tree F / refs | Componente fechado / risco | Tratamento G0 | Gate de implementação |
|---|---|---|---|---|
| Elegoo | `ElegooLink.cpp` no fork; ElegooSlicer AGPL (estudo) | ElegooLink componente fechado possível no OEM | Preferir LAN MQTT/HTTP CC2 documentado; sem DLL fechada | OEM-ELEGOO-16 / G4 |
| Anycubic | Perfis Orca; protocolo LAN a especificar | Início automático não homologado no legado W | Lacuna protocolo/ACE explícita; revalidar em Onda 06 | OEM-ANYCUBIC-17 / G4 |
| Bambu Lab | Studio AGPL (estudo); hosts/agent no Orca | Plugin rede proprietário; cloud/auth firmware | **LAN-only / SDK autorizado**; sem bundling plugin | OEM-BAMBU-18 / LEGAL-02 / G4 |
| Creality | Creality Print AGPL (estudo); `crealityprint` host_type | Detalhes CFS/câmera parcialmente não verificados | API LAN autorizada; sem falso ACK | OEM-CREALITY-19 / G4 |
| Flashforge | `Flashforge.cpp` LAN no Orca | Plugin rede não livre no OEM | Só LAN aberta/`Flashforge.cpp`; sem binário fechado | OEM-FLASHFORGE-20 / G4 |
| QIDI | Moonraker/perfis Orca; QIDI Studio fork (estudo) | — | Moonraker/protocolos oficiais | OEM-QIDI-21 / G4 |
| Snapmaker | Moonraker U1; Snapmaker Orca (estudo) | — | Moonraker + capabilities | OEM-SNAPMAKER-22 / G4 |
| Prusa | PrusaLink OpenAPI; PrusaSlicer AGPL (não motor F) | Prusa Connect cloud = conta oficial se usado | PrusaLink local primeiro | OEM-PRUSA-23 / G4 |
| UltiMaker | API docs públicas | SDK/API comercial possível | Sem autorização → suporte parcial declarado | OEM-ULTIMAKER-24 / LEGAL / G4 |
| Raise3D | Pesquisa obrigatória | API/SDK pode ser N | Sem autorização → parcial declarado | OEM-RAISE3D-25 / LEGAL / G4 |

## Lacunas explícitas (não bloqueiam G0 documental; bloqueiam OEM-*/G4)

1. Autorização jurídica/SDK Bambu Connect: **não obtida** — tracked LEGAL-02 / OEM-BAMBU-18.  
2. Termos Flashforge plugin fechado: **não embutir** — tracked OEM-FLASHFORGE-20.  
3. Protocolo Anycubic start + ACE: **não comprovado no F** — tracked OEM-ANYCUBIC-17.  
4. UltiMaker/Raise3D API comercial: **pesquisa/autorização pendente** — tracked OEM-*-24/25.  
5. Auditoria binária completa do instalador por DLL OEM: **não feita** (SBOM inicial não exige).  

## Proveniência código F relevante (amostra)

| Path F | Papel | Licença raiz |
|---|---|---|
| `src/slic3r/Utils/ElegooLink.cpp` | Host Elegoo | AGPL tree Orca |
| `src/slic3r/Utils/Flashforge.cpp` | Host Flashforge LAN | AGPL tree Orca |
| `src/slic3r/Utils/OrcaPrinterAgent.cpp` | Agent (stubs P0) | AGPL — stubs não são sucesso |
| `src/slic3r/Utils/PrintHost.cpp` | Print hosts | AGPL |
| `LICENSE.txt` | AGPL-3.0 | Obrigatório |

**Conclusão G0 riscos:** inventário de riscos OEM/licença/proveniência **PASS** com lacunas nomeadas e tratamento; nenhum órfão obrigatório sem destino de fase.
