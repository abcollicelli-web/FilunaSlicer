# Filuna QA — Snapmaker U1 (0.4 mm), fonte oficial v2.4.1

**Modelo seguinte escolhido após Bambu A1, Elegoo CC2, QIDI Q2 e Creality K2 Plus.**
**Estado:** laboratório iniciado; até haver G-code A/B comprovado, **NÃO HOMOLOGADO**.

## Fontes originais verificadas

- Release portátil oficial: `Snapmaker/OrcaSlicer`, tag `v2.4.1`, `Snapmaker_Orca_Windows_V2.4.1_portable.zip`, SHA-256 `1f002e2168c576f800bcfd0509b5e164b5ad65a0963abc2bef8317d929d48c9d`.
- `machine.json` da versão oficial: `resources/profiles/Snapmaker/machine/Snapmaker U1 (0.4 nozzle).json` — blob Git `d9a6ab12cb872af25275fab6045b05cfc4531e2f`.
- `process.json` da versão oficial: `resources/profiles/Snapmaker/process/0.20mm Standard @Snapmaker U1 (0.4 nozzle).json` — blob Git `932a52d485a20ea2042716b366733f48f40facfc`.
- `filament.json` da versão oficial: `resources/profiles/Snapmaker/filament/Snapmaker PLA Basic @U1.json` — blob Git `8e4ba5984b072578b82c842b7b465947bf82cfb2`.
- Extrusoras declaradas no perfil de máquina: quatro bicos de 0,4 mm. O ensaio de cubo usa as quatro posições com o **mesmo filamento PLA oficial**, sem afirmar validação de troca de ferramentas ou multicor.

## Atenção à divergência dos perfis antigos

A árvore `resources/profiles/Snapmaker` do Filuna herdado contém valores e nomes de perfis que divergem da release `v2.4.1` (inclusive nome do processo). **Não misturar essas árvores** no teste atual. A versão original fixada em `fixtures/oem/snapmaker/official-profiles` é a fonte do ensaio, com blob Git byte-exato.

## Gates de QA interno

1. Origem, ZIP, SHA-256 e três blobs oficiais do GitHub.
2. Confirmação do modelo U1 e bico 0,4 e compatibilidade de processo/filamento.
3. CLI do Snapmaker portátil **real**, duas execuções independentes (A/B), cada uma com diretório, log, arquivo G-code e SHA próprios; nenhuma normalização autorizada sem prova A/B.
4. Exportação opcional do 3MF pelo executável oficial (independente do G-code).
5. Próximas fases internas: integração do módulo de fatiamento Filuna × OEM, simulação dos protocolos de dispositivos, UI Preparar→Fatiar→Enviar e regressões Windows.

**Não confundir a conclusão de uma CLI oficial com `PARIDADE_FILUNA_PASS` nem com `INTERNAL_QA_PASS`.**

## Regra de lançamento aprovada em 07/11/2026

`FILUNA/REGRA-FASES-HOMOLOGACAO-INTERNA-BETA-FISICA-20261107.md`: todos os testes internos primeiro; **depois** build final; **depois** beta vigiado, quando será feita validação física dos dispositivos. Somente Bambu com Bambuddy foi intencionalmente adiado. Nenhuma prova física é exigida na fase portátil inicial, mas todo resultado físico permanece não testado até o beta.

## Execução

- Script: `lab/snapmaker_u1_official_qa.py`
- Workflow: `.github/workflows/filuna-snapmaker-u1-qa.yml`
- Primeira execução: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42311663149

Nenhum merge, release ou instalação de teste autorizado por esta evidência.
