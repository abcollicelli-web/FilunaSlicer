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


## 4. Resultados efetivos após liberação do runtime Linux (07/11/2026)

**CI oficial:** [execução #42321095370](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321095370) **SUCCESS**. O runner Ubuntu 24.04 precisou somente dos runtime packages verificados de GStreamer/WebKit após atualizar o índice APT; **não** compilou nem instalou fatiador OEM. O pacote original Flash Studio **1.7.9 AppImage** SHA-256 `96578656466416adc1ccd2e7ec656a0b217560edadeeb29642259e67922adf45` permaneceu intacto.

### Flashforge AD5X 0,4 mm
- Presets OEM completos: máquina "Flashforge AD5X 0.4 nozzle", processo "0.20mm Standard @FF AD5X", material "Flashforge HS PLA @FF AD5X". Todos resolvidos pela árvore oficial de heranças, sem dados de outras marcas.
- Duas invocações **separadas** do executável oficial; G-code gerado **209.677 bytes em cada**. **SHA-256 bruto idêntico**: `68ed51e6e830ed7ad1d2159a26640b4d54d046f857851f3ad9054b3d045063d6`.
- **`PASS_OEM_CLI_AB_SAMPLE`**, não versão Filuna, não IFS físico.

### Flashforge Adventurer 5M 0,4 mm
- Presets OEM completos: máquina "Flashforge Adventurer 5M 0.4 Nozzle", processo "0.20mm Standard @Flashforge AD5M 0.4 Nozzle", material "Flashforge Generic PLA".
- Duas invocações **separadas** do executável oficial; G-code gerado **185.038 bytes e 8.714 linhas em cada**.
- A/B com hashes brutos distintos, mas a comparação **byte a byte** mostrou **uma única diferença**: linha 2, no HEADER_BLOCK, exatamente `; generated by Flash Studio 1.7.9 on ... at ...`. Todos os outros bytes são iguais. SHA-256 normalizado, removendo **somente essa linha comprovada**, `d1df7a1024d4e21722054e07d9f05644aeb3a2019c4d9f22e812eb176ce8ea66`.
- O comparador `lab/flashforge_original_ad5x_ad5m_cli_qa.py` foi reforçado com seis testes negativos/positivos em `lab/test_flashforge_original_ab_qa.py`; **movimento, velocidade, temperatura, outra versão do cabeçalho, ausência da seção e mutação de CRLF/LF não podem ser normalizados**.
- **`PASS_OEM_CLI_AB_HEADER_ONLY`**, não G-code Filuna nativo, câmera ou rede.

### Inventário bruto versus catálogo interpretado
- O AppImage 1.7.9 extraído contém **59 JSON em machine, 130 em process e 636 em filament**, abrangendo arquivos base/condicionais/específicos no diretório Flashforge.
- A leitura por perfis nomeados da mesma árvore e origem Git do fabricante reconhece **26 perfis máquina, 60 processos e 123 filamentos**; não igualar contagem de arquivos à quantidade de modelos homologados.
- **64 pontos de revisão de fonte/dependência ainda abertos**, módulos de impressão, câmera, LAN, IFS e integração com motor único Filuna continuam NOT_RUN.

**Novo status seguro:** `OFFICIAL_PORTABLE_RUNTIME=PASS`, `OEM_AB_AD5X=PASS`, `OEM_AB_AD5M=PASS_TIMESTAMP_ONLY`, `FILUNA_NATIVE_GCODE=NOT_RUN`, `FILUNA_DEVICE_NETWORK=NOT_RUN`, `INTERNAL_HOMOLOGATION=BLOCKED`, `PHYSICAL_BETA=NOT_RUN`. Nenhuma autorização de imprimir automaticamente, mesclar PR ou publicar instalador decorre destes resultados.
