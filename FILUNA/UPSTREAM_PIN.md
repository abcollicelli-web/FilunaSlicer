# Upstream fixado (pin)

| Campo | Valor |
|---|---|
| Upstream | https://github.com/OrcaSlicer/OrcaSlicer (branch `main`, 2.5.0-dev) |
| SHA | `a620d804efbe74162c96ad29d755cdab2b823530` |
| Data do commit | 2026-11-05 13:40 UTC (14:40 Europe/Rome) |
| Mensagem | By layer: keep As object list copies in their list order (#17232) |
| CI upstream | "Build all" verde: https://github.com/OrcaSlicer/OrcaSlicer/actions/runs/42037889437 (Windows x64/arm64 build + unit tests = success) |

**Motivo:** decisão do dono de usar a main 2.5-dev, porém fixada num commit cujo CI Windows completo passou no próprio upstream.
A branch `upstream-mirror` aponta exatamente para este SHA. Atualizar o pin só segue o UPDATE-PLAYBOOK.md.
