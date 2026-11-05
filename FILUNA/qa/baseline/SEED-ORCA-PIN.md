# Seed / proveniência Orca pin Windows portable

## Fonte canônica (preferida)

| Campo | Valor |
|---|---|
| Upstream | `OrcaSlicer/OrcaSlicer` |
| headSha | `a620d804efbe74162c96ad29d755cdab2b823530` |
| Run Build-all success | https://github.com/OrcaSlicer/OrcaSlicer/actions/runs/42037889437 |
| Artifact | `OrcaSlicer_Windows_V2.5.0-dev_x64_portable` |

## Filuna comparado

| Campo | Valor |
|---|---|
| headSha | `2dace0db86d305bec5b9627179f265012e4f5069` |
| Run | https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42102469888 |
| Artifact | `Filuna_Windows_x64_portable` |

## Por que mirror no fork

`GITHUB_TOKEN` do Actions **não** baixa artifacts de outro repositório. Download cross-repo funciona no box com PAT do dono, mas **não** injetamos PAT em inputs/logs.

Caminhos suportados:

1. **Tentar** download upstream no job (pode 403) — se OK, usar direto.
2. **Mirror same-repo:** artifact nomeado `OrcaPin_Windows_x64_portable_a620d804` no fork.
3. **Rebuild pin no fork** (caro, ~Windows full): branch `filuna/pin-win-a620d804` = tree do pin + `filuna-windows.yml`; `gh workflow run filuna-windows.yml --ref filuna/pin-win-a620d804` (baseline-gcode-only=false). Depois o baseline usa o run ID do fork como `orca-pin-run-id` **somente se** head_sha do run == pin (job verifica).

Nightly/RC **proibidos** como substituto do pin.
