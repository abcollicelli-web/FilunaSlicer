# Checkpoint Filuna Slicer — 2026-11-06

**Estado: pausa para análise do proprietário.** Não executar novos testes, merge, deploy ou build até novas instruções.

**Repositório:** abcollicelli-web/FilunaSlicer, branch filuna/oem-gcode-routing-qa-20261106.

**Relatório integral R2:** FILUNA/qa/HOMOLOGACAO-LAB-8-FABRICANTES-R2-20261106.md (commit f20f5b83524be485eece51b55e2c67884293d6ca).
**Executor de contrato experimental:** lab/filuna_contract_test.py (commit 13defed802f80cc4d40a38c4fb0acb1fd37bc2ae).
**Executor da bateria:** lab/filuna_lab_run.sh (commit eae88c68f01c36035422b660b0b21a482b347bdd).
**Evidências originais:** /srv/site/incoming/filuna-local-oem-qa-20261106/run-20261106T153215Z/.

## Resultado mantido
Bambu A1 T1 CLI completo PASS — cubo SHA f907c93683ae5208fe47a9d22f3b9b125e51bbbaf87d8947b7a0e94c026950a4; quatro cores SHA bafae92490dd04a924f7ffa9312ba7a1ddb4ea79aebbae21a64f628e009569e1. Oficial e adaptador com exit 0/0, quatro slots AMS lite. Bambu A1 mini SHA legado 42864cc2b2dde3677bc16763599266a94e5ed8fbeaca623309a2099c0d04446f.

Elegoo CC2 e QIDI Q2: cubo igual (normalizado na Elegoo, bruto QIDI); quatro cores ainda falham. Creality K2: --need-gcode-file preserva G-code oficial e adaptador (exit 0), mas diferem em timestamp e creality_uuid; T1 completo não passou. Snapmaker U1: saída 0 sem G-code; --need-gcode-file inválido. Flashforge AD5X, Prusa CORE One e Anycubic Kobra S1: T1 NÃO EXECUTADO.

Rede: 13/13 PASS auxiliares dos simuladores. Protótipo de contrato Python: Bambu 0/10 PASS, Moonraker 7/10 PASS experimentais, Elegoo CC2 2/10 PASS experimentais. **Não é integração com as classes nativas C++ Filuna** e não homologa T3–T9. T2/T10/T11 PENDENTE DE CAMPO. Todos os oito fabricantes **EM VALIDAÇÃO**, nenhum liberado ao build.

## Falhas e correções pendentes
- Elegoo: 4 peças só T0; exportação com GLX/llvmpipe exit 3.
- QIDI: suporte de filamento 1 vs 4 corrigido em fixture diagnóstica; slice multicor ainda exit 156, erro interno -100, matriz purga 4×4 presente.
- Creality: projeto 3MF nativo K2/CFS, identidade UUID e multicor.
- Snapmaker: descobrir opção CLI válida que de fato exporte.
- Contrato nativo OEM: ainda não construído/conectado para homologação.

Regra de retomada: PASS apenas com comando, exit code, SHA e evidência; BLOQUEADO somente com tentativa e pré-requisito concreto; não executado sem número E; sem merge, deploy ou publicação. Aguardar avaliação do proprietário.
