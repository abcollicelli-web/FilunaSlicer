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
| `src/libslic3r/libslic3r.h` | `SLIC3R_APP_FULL_NAME` = "Filuna Slicer" (só exibição) e nova `FILUNA_DATA_DIR_NAME` = "Filuna" | rebrand-1 |
| `src/slic3r/GUI/GUI_App.cpp` | `SetAppName(FILUNA_DATA_DIR_NAME)` → pasta `%APPDATA%\Filuna`; ProgID `Filuna.Slicer.1`; URI `filunaslicer://` (não sobrescreve o `orcaslicer://` do Orca) | rebrand-1 |
| `src/libslic3r/utils.cpp` | Linux: pasta XDG `Filuna` (consistência) | rebrand-1 |
| `src/slic3r/GUI/Plater.cpp` | Título da janela "<projeto> - Filuna Slicer" (2 linhas) | rebrand-1 |
| `src/slic3r/GUI/AboutDialog.cpp` | Parágrafo de fork/AGPL no Sobre; créditos Slic3r → PrusaSlicer → Bambu Studio → OrcaSlicer mantidos | rebrand-1 |
| `src/dev-utils/platform/msw/OrcaSlicer.rc.in` | FileDescription/ProductName/InternalName do .exe = "Filuna Slicer" | rebrand-1 |
| `CMakeLists.txt` (bloco CPack) | Instalador: nome/pasta `FilunaSlicer`, chave de registro própria, novo GUID, atalho "Filuna Slicer" | rebrand-1 |
| `resources/images/OrcaSlicer*.png/.ico`, `OrcaSlicer.svg`, `OrcaSlicer_about*.svg` | Ícones provisórios Filuna nos mesmos nomes de arquivo (sem tocar código) | rebrand-1 |

### O que foi mantido de propósito como "OrcaSlicer"
- `SLIC3R_APP_NAME` / `SLIC3R_APP_KEY` (version.inc): usados no cabeçalho do G-code, na detecção do produtor, metadados 3MF, User-Agent HTTP e no catálogo de traduções (`OrcaSlicer.mo`). Mudar quebraria traduções e poderia afetar impressoras/firmwares que leem o cabeçalho.
- Nome do executável `orca-slicer.exe`, pasta de build `OrcaSlicer/` e nome de arquivo do instalador gerado pelo CPack: os workflows e scripts do upstream dependem deles. O CI Filuna republica os artefatos como `Filuna_Windows_*`.
- Identificação junto a serviços/impressoras (Moonraker, 3DPrinterOS, UltiMaker etc.) segue "OrcaSlicer".
- Perfis de sistema e recursos: vêm de `resources/` ao lado do executável, independentes da app key; a pasta de dados só guarda perfis do usuário/cache.

## Módulos de fabricante
Ver MODULES.md. Cada módulo traz o código de envio e controle vindo do slicer do fabricante, com versão e origem registradas. Eles não alteram o motor de fatiamento.

## Atualização de upstream
1. Avançar `upstream-mirror` (fast-forward) até um SHA com CI verde no upstream.
2. Criar `filuna/update-<sha>` e fazer merge (sem rebase) em cima de `filuna/main`.
3. Checar conflitos: se aparecer conflito fora dos pontos de extensão listados, a atualização para e é revisada.
4. CI de teste: `filuna-windows.yml` mais os unit tests.
5. Rodar o aceite (TEST-ACCEPTANCE.md) e só então fazer merge em `filuna/main` e atualizar UPSTREAM_PIN.md.
