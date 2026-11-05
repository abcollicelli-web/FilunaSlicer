# FILUNA G0 — Alinhamento MIG-01..26 ↔ DAG 52 nós / 149 deps / SRV-00..11
**ID** FLN-G0-MIG-DAG-2026-11-05.1  
**Atualizado** 2026-11-05 23:20:00 CET  
**Escopo** documental G0 (fonte por feature + matriz migração). **Não** implementa módulos.

Fonte DAG: `FILUNA/CRONOGRAMA-DEPENDENCIAS.json` revision `FLN-DAG-2026-11-05.2-server` — **52** nodes, **149** arestas `depends_on` (verificado por script neste lote).

| MIG | Subsistema (resumo) | Nós DAG | Gate típico nó | Estado nó (DAG) | Nota G0 |
|---|---|---|---|---|---|
| MIG-01 | Orca core | CORE-04, PROFILES-06 | G3 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-02 | Filuna ID | AUTH-12, SRV-02 | G3 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-03 | Home/identidade | BRAND-08, SHELL-09 | G1,G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-04 | Preparar | PREP-10, CORE-04 | G3,G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-05 | Calibração | PREP-10 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-06 | DeviceBridge | BRIDGE-14, SAFETY-13 | G2,G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-07 | 10 OEMs | OEM-ELEGOO-16, OEM-ANYCUBIC-17, OEM-BAMBU-18, OEM-CREALITY-19, OEM-FLASHFORGE-20, OEM-QIDI-21, OEM-SNAPMAKER-22, OEM-PRUSA-23, OEM-ULTIMAKER-24, OEM-RAISE3D-25 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-08 | Perfis | PROFILES-06 | G3 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-09 | Multi-placa | PLATES-07 | G3 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-10 | Catálogo print-only | CATALOG-30 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-11 | Paramétricos galeria | PARAM-29 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-12 | Lua/Árvore/Botton | PARAM-29 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-13 | Ferramentas | PARAM-29 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-14 | Estoque | STOCK-31 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-15 | Ateliê | ATELIE-33 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-16 | Produção | PROD-32 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-17 | E-commerce | ECOM-34 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-18 | IA | AI-35 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-19 | Updater | TRUST-11 | G5 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-20 | FUP/diagnóstico | SRV-04, SRV-05 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-21 | Site/mobile | PORTAL-36 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-22 | Segurança/AGPL | LEGAL-02, GOV-00 | G0 | IN_PROGRESS,NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-23 | A11y/perf/i18n | SHELL-09, QA-03 | G2,G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-24 | Plugin MCP | AI-35 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-25 | Ofertas/parceiros | CATALOG-30, PORTAL-36 | G4 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |
| MIG-26 | Upstream monitor | UPSTREAM-37 | G3 | NOT_STARTED | Fonte↔nó mapeado; implementação FORA_G0 |

## SRV-00..11 (inventário G0; implementação NOT_STARTED)

| ID | Título | depends_on | exit_gate | state |
|---|---|---|---|---|
| SRV-00 | Inventário privado de servidor, domínio, IP e riscos | GOV-00, SRC-01, LEGAL-02 | G0 | NOT_STARTED |
| SRV-01 | Hub API HTTPS, gateway/DB privado e schemas | SRV-00, QA-03 | G3 | NOT_STARTED |
| SRV-02 | Filuna ID servidor, sessões, consentimentos e RBAC | SRV-01, AUTH-12 | G3 | NOT_STARTED |
| SRV-03 | Sincronização segura, ledger, backup e restauração | SRV-02, DATA-05 | G4 | NOT_STARTED |
| SRV-04 | Diagnóstico sanitizado de aplicativo e painel read-only | SRV-02, QA-03, LEGAL-02 | G4 | NOT_STARTED |
| SRV-05 | Telemetria opt-in do estado de impressoras sem controle | SRV-04, BRIDGE-14, DEVICE-UI-15 | G4 | NOT_STARTED |
| SRV-06 | Portal e mobile leitura própria de impressoras | SRV-03, SRV-05 | G4 | NOT_STARTED |
| SRV-07 | Base remota desligada, fila assinada, policy engine e flags OFF | SRV-06, SAFETY-13 | G3 | NOT_STARTED |
| SRV-08 | Piloto de impressão remota do próprio cliente APÓS local homologado | SRV-07, OEM-ELEGOO-16, OEM-ANYCUBIC-17 | G4 | NOT_STARTED |
| SRV-09 | Comandos remotos web/mobile do cliente e câmera por escopo | SRV-08, PORTAL-36 | G5 | NOT_STARTED |
| SRV-10 | Release canário e homologação remota separada | SRV-09, RELEASE-38, TRUST-11 | G6 | NOT_STARTED |
| SRV-11 | Operação, revogação e teste desastre/remoto contínuo | SRV-10, OPERATE-39 | G6 | NOT_STARTED |

**Admin sem controle:** SRV-04/05 read-only — alinhado CF-05 / REGRA §servidor.

## Cobertura

- MIG-01..26: **26/26** com ≥1 nó DAG.
- Nós DAG sem MIG dedicado (ex.: GOV-00, QA-03, RELEASE-38): cobertos por GOV/LEGAL/QA e release — não são órfãos de migração; GOV-00/SRC-01/LEGAL-02 são o próprio G0.
- Nenhum MIG sem destino DAG.
