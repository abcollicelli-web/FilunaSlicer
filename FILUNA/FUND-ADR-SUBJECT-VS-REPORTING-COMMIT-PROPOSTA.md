# ADR (PROPOSTA, não vigente) — commit testado (subject) × commit de registro (reporting)

**Estado:** PROPOSTO · 2026-11-05 · aguarda decisão do dono. Regra vigente **inalterada**: `gate_governance.py` exige `commit_sha == HEAD` (fail-closed). Veredito t19u preservado.

## Problema
Registrar a evidência em docs cria commit novo → HEAD ≠ commit testado → evidência inválida (livelock).

## Dados já gravados (sem relaxar nada)
Cada evidência do baseline grava `subject_commit` (= `github.sha` do run) e `inputs_git_objects` (git object id de cada caminho de código/fixture/preset usado: `FILUNA/qa/baseline`, `FILUNA/tests/evidence_schema.py`, fixtures e presets), além de sha256 dos exe/dll.

## Proposta
Evidência vale para um `reporting_commit` R se e somente se:
1. `subject_commit` é ancestral de R; e
2. para todo caminho em `inputs_git_objects`, `git rev-parse R:<path>` é idêntico; e
3. binários (exe/dll sha256) e run/artifact continuam os mesmos.
Qualquer mudança em código/inputs → evidência inválida → nova execução obrigatória. Docs-only não invalida.

## Não decidido
Lista exata de caminhos “de código” (ex.: incluir `.github/workflows/filuna-windows.yml`?). Até decisão, manter BLOCKED.
