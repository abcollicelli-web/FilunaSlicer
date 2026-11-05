# Playbook de atualização

## Upstream (OrcaSlicer)
1. Escolher um SHA de `OrcaSlicer/OrcaSlicer` main cujo workflow "Build all" esteja verde:
   `gh api "repos/OrcaSlicer/OrcaSlicer/actions/workflows/build_all.yml/runs?branch=main&status=success&per_page=5"`
2. `git fetch upstream <sha>`, depois `git checkout upstream-mirror` e `git merge --ff-only <sha>`, depois `git push origin upstream-mirror`. Nunca usar force-push.
3. `git checkout -b filuna/update-<sha> filuna/main` e `git merge upstream-mirror`.
4. Conflitos: aceitam-se apenas os que caem nos arquivos listados em ARCHITECTURE.md. Qualquer outro conflito leva à revisão manual e é registrado no PR.
5. Rodar `filuna-windows.yml` na branch de update.
6. Rodar o TEST-ACCEPTANCE.md completo nas duas impressoras.
7. Abrir um PR para `filuna/main` e atualizar UPSTREAM_PIN.md (sha, data, motivo, link do CI).

## Módulos de fabricante
1. Verificar a nova tag/SHA do slicer do fabricante e conferir a licença.
2. Atualizar `FILUNA/modules/<id>/src` e o MODULE.md.
3. Rodar CI e aceite da impressora correspondente, depois abrir o PR.

## Reversão
Cada update é um merge commit. Para desfazer, usar `git revert -m 1 <merge>`.
