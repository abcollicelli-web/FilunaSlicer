# ADR — Baseline G-code e isolamento na Onda 01

**ID** FLN-ADR-FUND-2026-11-05.1 · **Atualizado** 2026-11-05 23:30:00 CET  
**Status** PROPOSTO (com SPEC; não implementado)

## Contexto
Onda 01 exige baseline G-code e isolamento de dados sem reabrir multi-motor e sem adulterar o core.

## Decisão
1. **Comparação pin↔Filuna** via CLI/unit no mesmo SHA de profiles/fixtures; Filuna = fork rebrand do mesmo pin, logo divergência inesperada é regressão Filuna (camada/dados), não “segundo motor”.  
2. **Normalização** restrita a metadados não funcionais (SPEC §5).  
3. **Sem execução de CI Windows caro** só para documentação; job de baseline sob demanda após aceite.  
4. **Isolamento:** manter `FILUNA_DATA_DIR_NAME` / `%APPDATA%\Filuna`; não copiar AppData Orca automaticamente neste ciclo.  
5. **libslic3r intocado** na fundação salvo ADR emergencial.

## Alternativas rejeitadas
- Rebaseline contínuo apagando diffs (esconde regressão).  
- Exigir G4 hardware para fechar Onda 01 (viola ordem do plano).  
- Começar tokens/Home (Onda 03).

## Consequências
+ Rastreabilidade; − baseline real só quando houver artifact+script; convivência `orcaslicer://` fica G4.
