# Fase 1 — Comparação visual preliminar (NÃO APROVADA)

**Estado: QA VISUAL FAIL / AGUARDA VALIDACAO DO PROPRIETARIO.**

## Fonte canônica
- `FILUNA/design/visual-c/telas-aprovadas-20261110/01-preparacao.png` (aprovado)
- `FILUNA/design/visual-c/telas-aprovadas-20261110/02-dispositivo.png` (aprovado)
- O HTML/CSS em `FILUNA/design/visual-c/ui/` **não substitui** a imagem aprovada.

## Evidências (1600 px / Chromium)
- [Preparação lado a lado](preparacao-comparacao-lado-a-lado.png)
- [Dispositivo lado a lado](dispositivo-comparacao-lado-a-lado.png)
- [Captura somente HTML — Preparação](preparacao-referencia-chromium.png)
- [Captura somente HTML — Dispositivo](dispositivo-referencia-chromium.png)

**Importante:** a coluna direita é a renderização Chromium dos protótipos HTML do próprio repositório, **não** é uma captura do binário Windows Filuna. O HTML representa um protótipo e contém dados ilustrativos; não comprova integração C++/wxWidgets nem dispositivos reais. A Fase 1 ainda **não tem captura do aplicativo** e não possui aceite.

## Diferenças observadas (FAIL)
1. Preparação: viewport no PNG aprovado é escuro (#17161A); a versão HTML Chromium apresenta fundo claro.
2. Preparação: geometria da peça e plano de impressão apresentam detalhamento e composição diferentes; não são o mesmo render.
3. Preparação: logotipo do PNG aprovado é volumétrico; HTML apresenta texto plano.
4. Dispositivo: câmera/visualização 3D não reproduz a geometria e o enquadramento da referência aprovada.
5. Dispositivo: distribuição e proporções dos controles/legendas não correspondem pixel a pixel.

## Limite da arquitetura (guia 01-TELAS.md, seção 3B)
Preparação e Dispositivo são telas nativas wxWidgets + OpenGL. Não reescrever em HTML. A meta de aceite do guia para telas nativas é **blocos, ordem, rótulos, cores, mesmas ferramentas e funcionalidades**, com diferenças arquiteturais registradas. Uma captura Chromium do HTML é referência auxiliar, não captura real. Somente comparar com uma captura do executável Windows no mesmo tamanho permite concluir a aceitação visual e funcional.

**Nenhum merge, release, deploy nem distribuição de instalador foi realizado.**
