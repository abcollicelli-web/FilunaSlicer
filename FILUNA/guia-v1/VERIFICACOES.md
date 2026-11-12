# Verificações do guia Filuna v1

## Fase 0 — origem do SQLite (10/11/2026)

- Fonte oficial: `https://www.sqlite.org/2025/sqlite-amalgamation-3500400.zip`
- Versão: SQLite **3.50.4**, domínio público.
- SHA-256 do arquivo ZIP original: `1d3049dd0f830a025a53105fc79fd2ab9431aea99e137809d064d8ee8356b032`.
- Extraídos diretamente da distribuição oficial: `src/sqlite3/sqlite3.c` e `src/sqlite3/sqlite3.h`, sem modificar conteúdo.
- SHA-256 `sqlite3.c`: `e3f5d6901e7492af4a1fc8c4d745cae84c264942524c3fbfc02b82a5ca8818c8`.
- SHA-256 `sqlite3.h`: `abd1514e0351f79393d1be882830afdb40a8099e8257f311f0bfdf8486f11bea`.
- Configuração: alvo estático `filuna_sqlite3`, vinculado apenas a `libslic3r_gui`; `src/libslic3r/**` permanece intocado.
- Verificação da purga do G-code (§6.4 de 02-DADOS): **NÃO EXECUTADA** — pertence à Fase 3.

- Teste local CMake/C++17 isolado (Linux): `filuna_sqlite3` compilou; `FilunaFoundation` linkou; `ctest` **1/1 PASS**; versão retornada `3050004`.
- CTest de regressão do Orca e build Windows: **PENDENTES**, sem afirmar aceite da fase.

## Fase 3 — purga da torre no G-code (§6.4 de 02-DADOS), 12/11/2026

- **Verificação pelo código-fonte** (`src/libslic3r/GCode.cpp`, `update_print_stats_and_format_filament_stats`): o peso de cada filamento na linha `; filament used [g]` é calculado sobre `extruder.used_filament() + wipe_tower_data.used_filament[id]`. Portanto o peso **já inclui** a torre de purga, e `; total filament used [g]` (soma desses pesos) também inclui.
- **Fórmula adotada:** `peça_g = total − purga`. Os eventos de fim de impressão gravam `consumo = −(real − purga)` e `purga = −purga`, então a soma na bobina é o total do G-code, sem contar a torre duas vezes.
- **A linha `; total filament used for wipe tower [g]` não é escrita por esta versão do Orca:** a constante existe em `Print.cpp`, mas não há chamada que a grave. Sem essa linha, `trabalho.purga_total_g` fica `NULL` (desconhecida), não há rateio e o desperdício de purga não é estimado. O leitor continua aceitando a linha se um módulo de fabricante a escrever.
- **Ainda não feito:** fatiar de verdade um cubo de 2 cores no executável e conferir os números. Esta verificação foi só de leitura do código.
