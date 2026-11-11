# FILUNA v1 — Fase 1, artes oficiais e cores

**Escopo:** somente identidade, splash, ícones, cor nativa da GUI e instalador (pré-aceite).
**Origem visual:** arquivos existentes de `abcollicelli-web/Filuna@main:site/slicer-source/branding/`. Não gerar marca nova e não alterar comportamento de fatiamento.

| Arte de origem | SHA-256 dos bytes | Aplicação |
|---|---|---|
| `filuna-logo-transparent.png` | `9b93f2012ca4ff702b3a5d994ce6dc732b02f11be969fa4a0392523e79c34c1f` | `Filuna_splash.png`, `Filuna_about.png` |
| `filuna-slicer-logo.png` | `6143b6f76587b81ad6ad0bb8a3c5894cc3eebcd811aad8cfc00b27d2bcd6a878` | `Filuna_horizontal_*.png` |
| `filuna_128.png` | `0b3a4503963a2b833f9a8b19b93dc67833defc251f9431fa7f99a91ad5e6d6b9` | ícones PNG legados em nomes preservados, marca F em miniatura |
| `Filuna.ico` | `49e4c480550857f6d8ee6bdf594859f9f29194b72479d540e743f13bfa96adcb` | ícone Filuna no instalador NSIS e ícones ICO com nomes de compatibilidade |

Paleta do guia `01-TELAS.md §3-B`: `#009688 → #C2185B`, `#00675b → #8E1243`, `#26A69A → #FF2E8A`. Algumas outras variantes do mesmo verde foram substituídas somente na GUI.

Aplicação deste commit: 47 arquivos C++/headers da GUI (106 ocorrências), 516 recursos SVG (2413 ocorrências), além das cores ImGui dos links. Arte original preservada no outro repositório.

**Limites e gates não testados:** janela/ícone/splash reais em Windows, atalhos do NSIS, abertura de 3MF, paridade G-code, UI idêntica à referência (nativas wx/OpenGL podem divergir conforme guia), imagens SVG de identidade herdadas que não receberam o PNG oficial, Mac ICNS não homologado. Também é necessária comparação de captura do executável **versus** projeto aprovado; Chromium de protótipo não substitui o app. O QA deste commit ainda será executado. Nenhum merge/release/deploy.
