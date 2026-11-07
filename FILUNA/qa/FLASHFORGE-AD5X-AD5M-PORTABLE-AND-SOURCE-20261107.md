# Flashforge — laboratório oficial 1.7.9 Linux / origem da rede e todos os modelos (07/11/2026)

**Status:** **FATIAMENTO OEM e PARIDADE FILUNA NÃO HOMOLOGADOS**. Documento de execução e continuidade, não garantia de funcionamento.

## 1. Proveniência e escolha técnica

A release oficial mais nova publicada no site Flashforge é **Flash Studio Desktop 1.7.20** Windows/macOS, mas o botão denominado `...win64.zip` encaminha para `Flash_Studio_Desktop_1.7.20_x64.exe`, um **instalador EXE**; não afirmar que há um portátil oficial 1.7.20.

O pacote Windows oficial 1.7.15 da release histórica GitHub tem SHA-256 verificado `b3cc8804e0426e3f4bb2c0953667e27512c0047af51099c0e1af5c5d1b3ef8f3`; extração direta via 7z não forneceu um executável+resources portátil funcional, portanto não há CLI PASS 1.7.15.

**Alternativa oficial para laboratório sem instalação nem compilação:** GitHub `FlashForge/Orca-Flashforge`, release **1.7.9** com Linux **`Flash.Studio_ubuntu24.04_V1.7.9.AppImage`** (153.561.592 bytes) e hash de release publicado pelo próprio GitHub **`96578656466416adc1ccd2e7ec656a0b217560edadeeb29642259e67922adf45`**. Código-fonte correspondente em tag/commit `70f2359ba8b4bc8ff9e54818a4bb1d05e56e7856`.

Links: https://github.com/FlashForge/Orca-Flashforge/releases/tag/1.7.9 e https://www.flashforge.com/pages/flash-studio-desktop

Este portátil **não** substitui a futura avaliação do firmware/código da versão Windows 1.7.20, nem dispensa comparar G-code Filuna. O manifesto de QA deve sempre identificar a diferença entre gerações.

## 2. Auditoria universal da árvore completa — PASS de inventário

CI https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42319915138, execução da job Flashforge:

- Git oficial fixado `70f2359ba8b4bc8ff9e54818a4bb1d05e56e7856` e auditoria automatizada de `src`, `deps`, `cmake`, `resources/profiles` e `resources/printers`: **PASS** de origem e inventário;
- **2.453 arquivos-fonte e recursos** inventariados; **2.267 candidatos de diferenças OEM** preparados em área isolada, sem integração no motor;
- **64 itens de fonte/dependência exigem revisão** de licenças, vínculo de código, ABI e recursos antes de promoção;
- O catálogo interpretou **26 perfis de máquina**, **60 processos**, **123 filamentos** da árvore Git e não detectou ciclos/ancestrais ausentes. Não confundir esses números com o número bruto de arquivos JSON nem com versões mais recentes da marca. Catálogo disponível ao mecanismo universal `FILUNA/tools/oem_universal_inventory.py`;
- Candidatos piloto a testar **pela mesma infraestrutura**: AD5X 0,4 e Adventurer 5M 0,4. A solução completa deve descobrir os demais automaticamente, inclusive AD5M Pro/Creator/Guider quando presentes.

A documentação pública do fabricante identifica Flash Studio como aplicativo com funções de conexão LAN/WAN, envio, câmera e multimaterial, mas essas **capabilities OEM não foram integradas nem testadas no Filuna**. A fonte do plugin de rede é historicamente marcada como baseada em bibliotecas não livres; não redistribuir componentes binários fechados sem autorização, nem simular autenticação verdadeira.

## 3. Etapas programadas e limites

Ferramenta atual: `lab/flashforge_appimage_original_qa.py`; workflow `.github/workflows/filuna-flashforge-original-appimage.yml`. Verificará hash do AppImage original, extração sem instalação, AppRun, perfis de todos os modelos e CLI `--help`, com logs por etapa. **Só declarar PASS após inspecionar CI/artefato, não pela existência da ferramenta.**

Gates futuros separados:

| Gate | Atual |
|---|---|
| Original 1.7.9 AppImage SHA, extração e CLI | Execução em curso; consultar resultados |
| Todos os perfis/fonte Git 1.7.9 | PASS inventário; 64 revisões |
| AD5X OEM CLI G-code A/B | NOT_RUN |
| AD5M OEM CLI G-code A/B | NOT_RUN |
| 3MF / multicor IFS | NOT_RUN |
| Filuna G-code real versus OEM | NOT_RUN |
| Rede física/virtual com ACK e capacidades OEM em processo Filuna | NOT_RUN |
| Homologação interna | BLOCKED |
| Homologação física | Somente beta vigiado após build final |

**Regra atual:** não compilar o motor OEM, não usar um Orca instalado no computador do cliente, não criar um segundo backend completo, não enviar prints nem fazer merge/deploy. Só liberar quando o Filuna nativo comprovar compatibilidade e tiver contratos OEM completos, incluindo rede/controle, conforme as regras do repositório.
