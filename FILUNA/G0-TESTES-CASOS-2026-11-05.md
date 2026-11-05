# FILUNA G0 — Casos de teste documentais (execução real)

**ID** FLN-G0-TEST-2026-11-05.1  
**Atualizado** 2026-11-05 23:22:00 CET (Europe/Rome)  
**Plataforma** Linux (box agente) — verificações documentais/CI metadata.  
**Hardware Windows/impressoras** N/A neste gate (pertence a **G4** — REGRA-MESTRE tabela Portões).  
**Commit base dos checks** `cc4952f0e0c334c6bb7a164c43b358eeeee0bce0` (antes do commit deste lote; reexecutar após push).

## Matriz de casos

| case_id | gate | requirement_id | script/entrada | expected | actual | result | evidence | platform | printer/firmware |
|---|---|---|---|---|---|---|---|---|---|
| G0-T01 | G0 | REGRA G0 mapa fontes | `FILUNA/MANIFESTO-FONTES-LEGADO-2026-11-05.md` + `G0-MAPA-141-STATUS-2026-11-05.md` | 141 path+SHA+status; 0 UNKNOWN | manifesto 141; mapa 141 linhas status; UNKNOWN=0 | **PASS** | arquivos versionados; sha256 mapa=`c40acaada9d8440c…` | Linux | N/A |
| G0-T02 | G0 | REGRA G0 fontes mestras / PLANO fontes obrigatórias | `G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md` | conjunto normativo definido e lido | doc presente; N1 justificado ≠141 cláusula | **PASS** | blob no branch continuidade | Linux | N/A |
| G0-T03 | G0 | REGRA G0 conflitos sem órfãos | `G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md` CF-01..11 + CF-07 | conflitos rastreados; legado ≠ falha G0 | CF rastreados; CF-07 RESOLVIDO por hierarquia REGRA §hierarquia | **PASS** | este dossiê | Linux | N/A |
| G0-T04 | G0 | Baseline upstream/Filuna | `UPSTREAM_PIN.md` + `gh api` upstream-mirror + rebrand-1 | pin=mirror; rebrand tip conhecido | upstream-mirror=`a620d804efbe74162c96ad29d755cdab2b823530` pin contém a620d804; rebrand=`2dace0db86d305bec5b9627179f265012e4f5069` | **PASS** | gh api 2026-11-05 | Linux | N/A |
| G0-T05 | G0 | Riscos (PLANO §8 / REGRA) | `G0-RISCOS-OEM-PROVENIENCIA-2026-11-05.md` + PLANO §8 | riscos OEM/licença com lacunas e tratamento | 10 OEM + política + lacunas nomeadas | **PASS** | sha256=`0057a90ef40c5b32…` | Linux | N/A |
| G0-T06 | G0 | PLANO Onda00 fonte por feature / matriz migração | `MIGRACAO` MIG-01..26 + `G0-MIG-DAG-ALINHAMENTO` | 26 MIG ↔ nós DAG; 52 nós / 149 deps | MIG 26/26; DAG 52 nós / 149 deps | **PASS** | CRONOGRAMA-DEPENDENCIAS.json + alinhamento | Linux | N/A |
| G0-T07 | G0 | PLANO Onda00 SBOM **inicial** | `sbom/README.md` + CDX + inventory | SBOM inicial verificável (completo NÃO exigido) | status `PARTIAL_NOT_COMPLETE`; receitas+deps_src; lacunas explícitas | **PASS** | FILUNA/sbom/* | Linux | N/A |
| G0-T08 | G0 | Testes/documentação Onda00 | `check_master_plan.py` | 113/113 PASS estático | exit=0; 113/113=YES | **PASS** | stdout abaixo | Linux | N/A |
| G0-T09 | G0 | Fundação checks | `check_quality_gate.py --mode foundation` | 0 falhas foundation | exit=0; 0 falhas=YES | **PASS** | stdout abaixo | Linux | N/A |
| G0-T10 | G0 | Build candidato conhecido (≠ G4) | `gh run view 42102469888` | success + artifacts Filuna (estado, não QA Windows) | conclusion=success; headSha=2dace0db86d3 | **PASS** (estado documental) | https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42102469888 | Linux/API | N/A |
| G0-T11 | G4 | Windows 10/11 real + impressoras | — | — | não executado (dono: testar depois) | **NOT RUN** | REGRA G4 | N/A | N/A |
| G0-T12 | G1 | Aceite visual/tokens HEX | — | — | assets no repo; tokens/QA visual pendentes | **NOT RUN** | REGRA G1 | N/A | N/A |

## Saída capturada (resumo)

### check_master_plan.py
```
PASS [109] release remoto só ocorre depois de release local e QA remoto
...
PASS [112] release local vinculado ao diagnóstico seguro do servidor
PASS [113] operação ocorre depois do release
GRAFO: 52 entregas, 149 vínculos, ordem sem ciclo: GOV-00 → SRC-01 → LEGAL-02 → QA-03 → BRAND-08 → SRV-00 ...
RESULTADO DOCUMENTAL: 113/113 PASS, 0 FAIL
ATENÇÃO: validação estática aprovada; G0/G1 e QA Windows/Hardware NÃO foram homologados.
```

### check_quality_gate.py --mode foundation
```
PASS: perfil Anycubic existe
PASS: modelo real do perfil Anycubic confere
PASS: evidência de aceite físico permanece explicitamente pendente
PASS: homologação física não é omitida
PASS: regressão de fatiamento e rollback permanecem no aceite
Resumo: 0 falha(s); modo=foundation
```

## Interpretação

- Casos **G0-T01..T10** = evidência do gate **G0 Inventário** (REGRA) + entregáveis Onda 00 (PLANO).  
- **G0-T11** é G4 → **NOT RUN** não bloqueia G0.  
- **G0-T12** é G1 → **NOT RUN** não bloqueia G0.  
- `113/113` sozinho ≠ G0 PASS; neste dossiê entra como **G0-T08** entre os critérios documentais.
