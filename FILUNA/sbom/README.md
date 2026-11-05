# FILUNA — SBOM parcial Windows (verificável, NÃO completo)

**ID** FLN-SBOM-F-WIN-2026-11-05.1 · **Atualizado** 2026-11-05 23:24:00 CET  
**Status** PARTIAL / NOT COMPLETE — adequado como *SBOM inicial* de evidência G0, **não** como declaração de fechamento de supply-chain.

## Escopo

| Campo | Valor |
|---|---|
| Produto | FilunaSlicer Windows x64 |
| Upstream pin | `a620d804efbe74162c96ad29d755cdab2b823530` |
| Base rebrand | `2dace0db86d305bec5b9627179f265012e4f5069` |
| Build evidência | [42102469888](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42102469888) success |
| Versão app | `SoftFever_VERSION` 2.5.0-dev (`version.inc`) |
| Licença raiz | AGPL-3.0 (`LICENSE.txt`) |
| Artefato machine-readable | [filuna-windows-partial.cdx.json](filuna-windows-partial.cdx.json) (CycloneDX 1.5 **parcial**) |

## Método e limites

- Fontes: receitas `deps/**/*.cmake`, árvore `deps_src/`, `version.inc`, `UPSTREAM_PIN.md`.
- **Não** descompactamos o instalador/portable para inventariar cada DLL.
- **Não** afirmamos licença SPDX de pacote só pelo nome; `license_asserted=false` no CDX.
- **Não** fechamos dependências transitivas (ex.: submódulos de libdatachannel, deps do OpenCV).
- Runtime MSVC/Windows SDK/imagem do runner GitHub Actions: **não capturados**.
- Plugins de rede proprietários Bambu/Flashforge: **fora do pacote** por política Filuna (não prova ausência total de código fechado em todos os blobs sem auditoria binária).

## Componentes com origem+hash/versão em receita CMake (amostra verificada)

| Componente | Versão (da receita) | SHA-256 / git | Evidência |
|---|---|---|---|
| libpng | 1.6.59 | `6fd97a1e5e2ac55cec0a538a9800a351eb3844086736291844ebd12db72ba433` | `deps/PNG/PNG.cmake` |
| zlib | 1.2.13 | `c2856951bbf30e30861ace3765595d86ba13f2cf01279d901f6c62258c57f4ff` | `deps/ZLIB/ZLIB.cmake` |
| FFmpeg | n7.0.3 (Windows prebuild orca-shared) | `85da19daf198f5548259d8aabb349db84997a3f6e6886d8d7764114add9c6dae` | `deps/FFMPEG/FFMPEG.cmake PREBUILD_HASH_x64` |
| OpenCV | 4.14.0 | `ee8fb9b30eb60850431b4656447080e3737b56e45719c92b67f245950609f86e` | `deps/OpenCV/OpenCV.cmake` |
| GMP | 6.3.0 | `a3c2b80201b89e68616f4ad30bc66aee4927c3ce50e33929ca819d5c43538898` | `deps/GMP/GMP.cmake` |
| MPFR | 4.2.2 | `9ad62c7dc910303cd384ff8f1f4767a655124980bb6d8650fe62c815a231bb7b` | `deps/MPFR/MPFR.cmake` |
| GLFW | 3.4 | `a133ddc3d3c66143eba9035621db8e0bcf34dba1ee9514a9e23e96afd39fd57a` | `deps/GLFW/GLFW.cmake` |
| Draco | 1.5.7 | `27b72ba2d5ff3d0a9814ad40d4cb88f8dc89a35491c0866d952473f8f9416b77` | `deps/Draco/Draco.cmake` |
| libdatachannel | v0.24.5 | `N/A (git tag)` | `deps/DataChannel/DataChannel.cmake GIT_TAG` |
| WebView2 headers | product 138.0.3351.48 (from header define) | `N/A (headers tree)` | `deps/WebView2/include/WebView2EnvironmentOptions.h` |
| Eigen | 5.0.1 | `0dbb1f9e3aaad66f352c03227d8c983f6f0b49e0b07e71a7300f4abcc01aee12` | `deps/Eigen/Eigen.cmake` |
| CGAL | 6.2.1 | `eebd737d9b7f0199647ba5c1f9f6c7a3d0651aacef9c2fd4dedc52da6c25edbe` | `deps/CGAL/CGAL.cmake` |
| OCCT | V8_0_1 | `7c033d917ee8f040c0512d289dcc5f02c148889d5bac17c3e25639accb44f0da` | `deps/OCCT/OCCT.cmake` |
| Boost | 1.84.0 | `4d27e9efed0f6f152dc28db6430b9d3dfb40c0345da7342eaa5a987dde57bd95` | `deps/Boost/Boost.cmake` |
| oneTBB | 2021.5.0 | `83ea786c964a384dd72534f9854b419716f412f9d43c0be88d41874763e7bb47` | `deps/TBB/TBB.cmake` |
| OpenSSL | 1.1.1w | `2130E8C2FB3B79D1086186F78E59E8BC8D1A6AEDF17AB3907F4CB9AE20918C41` | `deps/OpenSSL/OpenSSL.cmake` |
| Assimp | 5.3.1 (CMake <3.22) / 5.4.3 (≥3.22) | `a07666be…` / `66dfbaee…` | `deps/Assimp/Assimp.cmake` |
| wxWidgets (Orca fork) | git b44c39c100975a0ac0e46f937aa3f3eae5e18631 | `N/A (git)` | `deps/wxWidgets/wxWidgets.cmake` |
| OrcaSlicer / Filuna core | 2.5.0-dev | `git a620d804efbe74162c96ad29d755cdab2b823530` | `FILUNA/UPSTREAM_PIN.md + version.inc SoftFever_VERSION` |

## Vendored `deps_src/` (27 dirs)

Presentes no tree do pin; licenças apenas apontadas por arquivos LICENSE/COPYING/NOTICE quando existem — **SPDX não inferido automaticamente como verdade jurídica**.

| Dir | version_guess | license_files |
|---|---|---|
| `Shiny` | 3.13 | — |
| `admesh` | 3.13 | — |
| `agg` | 3.13 | — |
| `ankerl` | 3.13 | — |
| `clipper2` | 3.10 | — |
| `earcut` | 3.13 | `deps_src/earcut/LICENSE` |
| `expat` | 3.13 | `deps_src/expat/COPYING` |
| `fast_float` | 3.13 | — |
| `glu-libtess` | 3.13 | — |
| `hidapi` | — | — |
| `hints` | 3.13 | — |
| `imgui` | 3.13 | `deps_src/imgui/LICENSE.txt` |
| `imguizmo` | 3.13 | `deps_src/imguizmo/LICENSE` |
| `libigl` | 3.13 | — |
| `libnest2d` | — | `deps_src/libnest2d/LICENSE.txt` |
| `mcut` | 3.13 | `deps_src/mcut/LICENSE.GPL.txt`, `deps_src/mcut/LICENSE.txt` |
| `md4c` | 3.5 | `deps_src/md4c/LICENSE.md` |
| `mdns` | 3.13 | `deps_src/mdns/NOTICE.md` |
| `minilzo` | 3.13 | `deps_src/minilzo/COPYING` |
| `miniz` | 3.13 | `deps_src/miniz/LICENSE` |
| `nanosvg` | 3.13 | — |
| `nlohmann` | 3.13 | — |
| `pybind11` | — | `deps_src/pybind11/LICENSE` |
| `qhull` | 3.13 | `deps_src/qhull/COPYING.txt` |
| `qoi` | 3.13 | — |
| `semver` | 3.13 | — |
| `stb_dxt` | 3.13 | — |

## Lacunas explícitas (impedem “SBOM completo”)
- Exact set of packages compiled on Windows depends on options in deps-windows.cmake + CMake cache of the CI runner — not fully reconstructed from recipes alone
- wxWidgets git tag known but license must be read from that checkout
- CURL recipe version partially commented — need resolved URL in CI log for exact curl version
- System/MSVC runtime, Windows SDK, WebView2 redistributable versions from runner image not captured here
- No CycloneDX emitted as complete SBOM — JSON inventory is evidence-backed partial
- EXPAT/CURL: receita CURL sem URL ativa óbvia no trecho principal (comentários de tags); EXPAT.cmake sem URL parseada neste lote.
- Inventário binário do artefato Filuna_Windows_* não executado.

## Relação com G0

O plano (Onda 00) exige **SBOM inicial**. Este pacote (`PARTIAL_NOT_COMPLETE`, receitas CMake + `deps_src` + CycloneDX parcial com lacunas explícitas) **satisfaz SBOM inicial**.  
**Não** exige SBOM completo nem inventário DLL do artefact para fechar G0. Completo = fase QA/REL posterior.  
G0 PASS documental também exige os demais critérios REGRA/PLANO (fontes, conflitos, baseline, riscos, mapa) — ver `G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md`.
