# Filuna QA — Prusa Original MK4S 0,4 mm (fase interna; não homologado)

**Iniciado em 07/11/2026.** Próximo modelo após o início do laboratório Snapmaker U1. A pendência U1 não é convertida em PASS, nem impede um ensaio **independente** de outro fabricante. Nenhum merge, build final, instalador, publicação, impressão física ou disparo de rede é permitido nesta fase.

## Fontes oficiais fixadas

- Slicer: `prusa3d/PrusaSlicer`, tag `version_2.9.7`.
- ZIP oficial: `PrusaSlicer-2.9.7.zip`, SHA-256 `5d4f395aeb6c344b300e0e1be37a0e1931622b642c3c3085eb19cdae211bc008`.
- Binário portátil de console: `prusa-slicer-console.exe`.
- Presets oficiais: `resources/profiles/PrusaResearch.ini` na mesma tag, **Git blob** `b7872a7c735329153b8508cfa53a3461a3a2bf7d`; runner comprova hash de objeto Git antes de interpretar.
- Modelo exato: `Original Prusa MK4S 0.4 nozzle`, sem MMU3 e sem bico High Flow.
- Processo: `0.20mm SPEED @MK4S 0.4`.
- Filamento: `Prusament PLA @MK4S`.
- Corpus diagnóstico: `fixtures/cube20.stl`. Não representa a peça multicor do proprietário.

## Implementação de laboratório

- `lab/prusa_mk4s_official_qa.py`: verifica ZIP e executável, baixa/valida arquivo de perfis da tag oficial e **resolve todas as seções herdadas**, recusando ancestrais ausentes, ciclos, duplicatas e modelo/bico incompatível. Constrói um arquivo INI **temporário** apenas no runner, preservando intocado o arquivo OEM. Registra árvore de herança, colisões de parâmetros e hashes.
- Executa o **mesmo console oficial** duas vezes, em `run-A` e `run-B` isolados. Salva comandos, logs, exit codes, SHA-256, tamanhos dos arquivos e diferenças linha a linha. Nenhum ajuste/normalização de cabeçalho é aprovado sem prova das diferenças reais.
- `lab/test_prusa_mk4s_official_qa.py`: provas positivas e negativas de herança, hash Git e rejeição de parâmetros de modelo incompatível.
- `.github/workflows/filuna-prusa-mk4s-qa.yml`: GitHub Actions Windows 2022; somente Python + ZIP OEM oficial com hash; artifacts preservados inclusive na falha. **Não compila** Orca/Prusa/Filuna nem instala sistema OEM.

## Critérios bloqueadores

| Evidência | Estado na criação |
|---|---|
| Fonte Prusa/tag/hash publicado | Confirmado como fonte; execução ZIP ainda a provar no novo runner |
| Prusa MK4S 0,4, processo e PLA herdados | Parser preparado; **PENDENTE da execução Windows** |
| G-code A/B oficial independente | **PENDENTE** |
| G-code Filuna nativo comparado ao oficial | **PENDENTE** |
| Dispositivos → Preparar → Fatiar → Enviar, rede simulada | **PENDENTE** |
| Homologação interna final do modelo | **PENDENTE** |
| Homologação física | **RESERVADA AO BETA VIGIADO** após todos os testes internos e build final |

O relatório do job e seus artefatos prevalecem sobre qualquer suposição. Em caso de exit code diferente de zero, G-code ausente ou hashes brutos divergentes, há falha técnica a investigar, jamais PASS presumido.

A regra atual é `FILUNA/REGRA-FASES-HOMOLOGACAO-INTERNA-BETA-FISICA-20261107.md`. Somente a autenticação Bambu × Bambuddy foi **intencionalmente** adiada; todas as outras falhas técnicas continuam em investigação. O PR #6 permanece DRAFT.
