# FILUNA — REQ Onda 01 Fundação

**ID** FLN-FUND-REQ-2026-11-05.1 · **Atualizado** 2026-11-05 23:30:00 CET  
**SPEC** [FUND-SPEC-ONDA01-2026-11-05.md](FUND-SPEC-ONDA01-2026-11-05.md)  
**Estado** PROPOSTO (não implementado)

| REQ ID | Classe | Texto | Fonte | Exceções | Nó DAG | Gate |
|---|---|---|---|---|---|---|
| REQ-FUND-001 | obrigatório | Manter pin Orca `a620d804efbe74162c96ad29d755cdab2b823530` e `upstream-mirror` idêntico; mudanças de motor só via UPDATE-PLAYBOOK | UPSTREAM_PIN; REGRA | cherry-pick documentado | CORE-04 | G3 |
| REQ-FUND-002 | obrigatório | Não modificar `src/libslic3r/**` neste ciclo de fundação salvo bugblocker com ADR | REGRA fronteira 1 | ADR + baseline | CORE-04 | G2 |
| REQ-FUND-003 | obrigatório | Pasta de dados padrão `%APPDATA%\Filuna` distinta de Orca | rebrand; SPEC §4 | migração opcional futura = REQ separado | DATA-05 | G3 |
| REQ-FUND-004 | obrigatório | Baseline G-code: mesmo fixture+preset+seed; normalizar só metadados não funcionais; divergência fora da tolerância = FAIL | PLANO Onda01; SPEC §5 | fixtures marcadas “non-deterministic” com motivo | CORE-04 | G3 |
| REQ-FUND-005 | obrigatório | `check_quality_gate --mode release` falha sem dossiê | foundation-check | — | QA-03 | G2 |
| REQ-FUND-006 | obrigatório | SBOM inicial permanece versionado; evolução não apaga lacunas sem documentar | Onda00/SBOM | completo ≠ bloqueio fundação | QA-03 | G2 |
| REQ-FUND-007 | obrigatório | Nenhum envio/start de impressão sem confirmação do usuário | REGRA; CF-04 | — | SAFETY/QA | G2 |
| REQ-FUND-008 | obrigatório | Logs de fundação sem caminho de modelo, G-code cru, token, senha, e-mail | AUDITORIA W ref; REGRA | — | QA-03 | G2 |
| REQ-FUND-009 | obrigatório | Instalador/portable Filuna tratado como candidato de teste até G4/G5 | REGRA builds | run 42102469888 evidência atual | QA-03/DATA | G2 |
| REQ-FUND-010 | obrigatório | Créditos AGPL Slic3r→Prusa→Bambu→Orca→Filuna preservados | LICENSE; LEGAL-02 | — | LEGAL-02 | G0/G2 |
| REQ-FUND-011 | obrigatório | Sem empacotar plugin rede proprietário Bambu/Flashforge | REGRA §6; G0-OEM | SDK autorizado futuro | LEGAL-02 | G0/G2 |
| REQ-FUND-012 | opcional | Coexistência Orca instalado + Filuna (`orcaslicer://`) | decisão dono | só G4 prova | DATA-05 | G4 |
| REQ-FUND-013 | obrigatório | Versionamento legível no app/candidato (versão SoftFever/Filuna documentada no artefato) | PLANO Onda01 | — | QA-03 | G2 |
| REQ-FUND-014 | obrigatório | Escopo Onda01 **não** inclui Home/tokens HEX/DeviceBridge/SRV provisionado | PLANO ondas 01 vs 03/05/12 | — | GOV | — |

**Órfãos:** nenhum REQ sem nó/gate. REQ-FUND-012 explicitamente G4.
