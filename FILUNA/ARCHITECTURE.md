# Arquitetura

```
┌──────────────────────────────────────────┐
│ Camada Filuna (marca, tema, telas, Hub)  │  FILUNA/ + pontos de extensão
├──────────────────────────────────────────┤
│ Módulos de fabricante (Elegoo, Anycubic…)│  FILUNA/modules/<fabricante>/
├──────────────────────────────────────────┤
│ OrcaSlicer upstream (motor, perfis, GUI) │  upstream-mirror, sem edição
└──────────────────────────────────────────┘
```

## Branches
- `upstream-mirror`: cópia exata de `OrcaSlicer/OrcaSlicer` no SHA fixado (UPSTREAM_PIN.md). Só avança por fast-forward a partir do upstream.
- `filuna/main`: branch de trabalho, igual a `upstream-mirror` mais os commits Filuna.
- `main` (do fork): espelha o upstream e não é usada pelo Filuna.
- `filuna/update-<sha>`: branch temporária para cada atualização de upstream.

## Camada Filuna
- Marca (nome, ícones, splash), tema e textos.
- Pasta de dados `Filuna`, separada da do Orca (como o Bambu Studio faz).
- Integração com o Hub Filuna.
- **Pontos de extensão no código upstream:** cada arquivo do Orca alterado entra nesta tabela, com o motivo. A meta é manter a tabela pequena.

| Arquivo upstream | Motivo | Desde |
|---|---|---|
| `src/slic3r/GUI/MainFrame.cpp` | Entrada de atualizações Filuna no menu de ajuda, bloqueada sem confiança operacional. | PR #7 |
| `src/slic3r/GUI/GUI_App.cpp` | Remove a consulta automática ao feed Orca na inicialização Filuna. | PR #7 |
| `.github/workflows/build_check_cache.yml`, `build_deps.yml`, `build_orca.yml` | Opção `windows-installer` propagada até o empacotamento. A validação GUI Filuna usa `false` para gerar somente portable, sem NSIS/MSIX; outros chamadores mantêm o padrão anterior. | PR #7 |

## Módulos de fabricante
Ver MODULES.md. Cada módulo traz o código de envio e controle vindo do slicer do fabricante, com versão e origem registradas. Eles não alteram o motor de fatiamento.

## Atualização de upstream
1. Avançar `upstream-mirror` (fast-forward) até um SHA com CI verde no upstream.
2. Criar `filuna/update-<sha>` e fazer merge (sem rebase) em cima de `filuna/main`.
3. Checar conflitos: se aparecer conflito fora dos pontos de extensão listados, a atualização para e é revisada.
4. CI de teste: `filuna-windows.yml` mais os unit tests.
5. Rodar o aceite (TEST-ACCEPTANCE.md) e só então fazer merge em `filuna/main` e atualizar UPSTREAM_PIN.md.
