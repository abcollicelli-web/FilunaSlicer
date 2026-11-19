# 01 — Telas aprovadas: o que são e como aplicar sem destruir

## 1. Fonte de verdade

| Tela | Arquivo de referência (abra no navegador) | Origem |
|---|---|---|
| Preparação | `FILUNA/design/visual-c/ui/preparacao.html` | aprovada pelo dono (`telas-aprovadas-20261110/01-preparacao.png`) |
| Dispositivo | `ui/dispositivo.html` | aprovada pelo dono (`telas-aprovadas-20261110/02-dispositivo.png`) |
| Reparação | `ui/reparacao.html` | ferramentas e disposição enviadas pelo dono, estilo Filuna |
| Paramétrico | `ui/parametrico.html` | idem |
| Meu espaço | `ui/meu-espaco.html` | idem |
| Estoque | `ui/estoque.html` | idem |
| Modelos 3D | `ui/modelos-3d.html` | idem |
| Modelos paramétricos | `ui/modelos-parametricos.html` | idem |
| Detalhe do modelo | `ui/modelo-detalhe.html` | idem |
| Início, Visualização, Projeto, Calibração | pranchas em `filuna/visual-c-20261108:FILUNA/design/visual-c/{inicio,visualizacao,calibracao}/` | **propostas, não aprovadas** — peça aprovação antes |

`ui/base.css` contém os estilos compartilhados. **A regra do dono:** as ferramentas, campos e disposição de cada tela são exatamente os do desenho; só o estilo é o da Filuna.

## 2. Sistema visual (não invente valores)

Tokens oficiais, de `Filuna/site/marca/tokens.css`:

| Uso | Valor |
|---|---|
| Destaque (botão principal, aba ativa, links, barra de progresso) | `#C2185B` (magenta escuro) e `#FF2E8A` (magenta) |
| Fundo da aba/chip ativo | `#FDE3EC` |
| Acento "3D" do logo, cor secundária | `#FF6B1A` |
| Fundo da janela | `#EDEBE7` · painéis `#FFFFFF` · painel lateral `#FCFAF7` · creme `#FFF8F0` |
| Linhas/bordas | `#ECE8E2` · bordas de campo `#E7E3DD` |
| Texto | `#2C2A28` · secundário `#6B6660` · apagado `#9A948C` |
| Sucesso | `#1F8F5F` sobre `#E3F5EC` · alerta `#D9480F` |
| Viewport 3D | escuro `#17161A` (Reparação, Paramétrico, Detalhe, câmera) ou claro `#E7E4DF` (Preparação) |

Tipografia: **Inter**. Título de página 30 px/800; título de cartão 12 px maiúsculas espaçadas 0,12em/700; corpo 14–15 px; números grandes 28–52 px/800.
Formas: cartão raio 16 px; campo e botão raio 10–12 px; botão 40–48 px de altura; interruptor 46×26 px.
Marca: logotipo "Filuna 3D" (não trocar por "F" avulso; não trocar pela mascote).

## 3. Decisão de arquitetura das telas (siga exatamente)

O fatiador tem dois tipos de tela. Trate cada um do jeito certo:

**A. Telas novas da Filuna → WebView, usando o HTML/CSS de `ui/` como base.**
Meu espaço, Estoque, Modelos 3D, Modelos paramétricos, Detalhe do modelo, Paramétrico, e os painéis de resultado da Reparação. O Orca já mostra a aba Início em WebView (`MainFrame.cpp` linha ~1577, `WebViewPanel`, páginas em `resources/web/`). Faça igual:
1. Copie `ui/*.html` e `base.css` para `resources/web/filuna/`.
2. Troque os dados fixos do desenho por dados reais vindos do C++ (ver §5).
3. Não reescreva o CSS. Se precisar mudar estilo, mude `base.css` e confira todas as telas de novo.

**B. Telas nativas do Orca → restilizar, não reconstruir.**
Preparação, Visualização, Dispositivo (monitor), Projeto, Calibração são wxWidgets + OpenGL com milhares de comportamentos. **Não as reescreva em HTML.** O que fazer:
1. Cor: trocar a cor do Orca pela Filuna nos pontos centrais — `src/slic3r/GUI/Widgets/StateColor.cpp` linhas 14 e 22 (`#009688`→`#C2185B`, `#00675b`→`#8E1243`, `#26A69A`→`#FF2E8A`), `GUI_App.cpp` linha 486, `Widgets/Label.cpp` linha 347; depois os demais arquivos que contêm `009688` (54 arquivos de código, 502 SVG em `resources/images/`). Nos SVG, substituição textual de `#009688` por `#C2185B`.
2. Abas do topo: `MainFrame.cpp` linhas 1584–1644 (`m_tabpanel->AddPage`). Ordem e nomes: Início · Preparação · Visualização · Dispositivo · Projeto · Calibração · Meu espaço.
3. Rótulos: só pelos catálogos de tradução (§4).
4. Barra de ferramentas 3D e gizmos: manter ordem e função; trocar apenas ícones/cores.

**Limite honesto:** nas telas do tipo B o resultado não será idêntico pixel a pixel ao desenho, porque o desenho é HTML e a tela é wxWidgets. O alvo é: mesma disposição, mesmas cores, mesmos rótulos, mesmas ferramentas. Onde não der, registre a diferença no PR; não force quebrando função.

## 4. Tradução pt-BR

- Arquivo: `localization/i18n/pt_BR/OrcaSlicer_pt_BR.po` (9.688 entradas, nenhuma vazia hoje). Regras completas em `AGENTS.md` → "Localization & translations".
- Edite só `msgstr`. Nunca `msgid`. Preserve `%s`, `%d`, `%1%`, `\n`.
- Idioma padrão: pt-BR na primeira execução — `GUI_App::load_language` (`GUI_App.cpp` linha ~8566): se `app_config->get("language")` estiver vazio, gravar `pt_BR`.
- Strings novas da Filuna: escrever em português direto dentro de `_L("…")`? **Não.** O `msgid` fica em inglês e a tradução no `.po`, para o sistema de idiomas continuar funcionando. Adicione o arquivo novo em `localization/i18n/list.txt`.
- Telas WebView: textos em um único `resources/web/filuna/textos.pt-BR.json`; nada de texto solto no HTML.
- Vocabulário fixo (use sempre igual): Preparação, Visualização, Dispositivo, Projeto, Calibração, Meu espaço, Fatiar mesa, Exportar, Bobina, Estoque, Purga, Troca de filamento, Mesa, Bico, Parede, Preenchimento, Suporte, Camada, Autoajuda.
- Verificação: `msgfmt --check-format --check-header` sem erro, e varredura manual de cada tela procurando inglês.

## 5. Ponte WebView ↔ C++ (dados reais nas telas A)

O Orca já tem a ponte: a página chama `window.wx.postMessage(JSON)` (ver `resources/web/homepage/js/globalapi.js` linha ~276) e o C++ recebe em `WebViewPanel::OnScriptMessage` (`src/slic3r/GUI/WebViewDialog.cpp` linha ~1050); o C++ responde executando script na página, que o recebe em `HandleStudio` (`resources/web/homepage/js/home.js` linha ~679). Leia esses três pontos antes de escrever a sua. Use o mesmo mecanismo com um prefixo próprio:

```
pedido  (JS → C++): {"filuna":1,"id":"<uuid>","acao":"estoque.listar","dados":{...}}
resposta(C++ → JS): {"filuna":1,"id":"<mesmo uuid>","ok":true,"dados":{...}}  ou  {"ok":false,"erro":"<codigo>","msg":"<texto pt-BR>"}
```

Ações mínimas: `estoque.listar|criar|editar|pesar|arquivar`, `bobina.etiqueta`, `historico.listar`, `produto.listar|renomear|unir`, `painel.resumo`, `previsao.filamentos`, `manutencao.listar|registrar`, `arquivo.abrir|relocalizar`, `conta.estado`, `sync.estado`. Cada ação lê/grava o banco local (02-DADOS) — nunca a rede diretamente.

## 6. Autoajuda (liga/desliga)

- Interruptor "Autoajuda" no rodapé da Preparação (como no desenho) e em Configurações. Chave em `app_config`: `filuna_autoajuda` = `"1"`/`"0"`, padrão `"1"`.
- Parâmetros: o texto vem de `Field::get_tooltip_text` (`Field.cpp` linha 409). O Orca 2.5 já tem ilustração em tooltip — **leia `docs/HLSD/tooltip-illustrations.md` e reaproveite**; não crie outro mecanismo.
- Conteúdo: `resources/filuna/autoajuda/pt_BR.json`, uma entrada por chave de parâmetro (`opt_key`) ou ferramenta:
  `{"wall_loops":{"titulo":"Voltas da parede","oque":"Quantas voltas de contorno a peça tem.","efeito":"Mais voltas = peça mais forte e mais tempo.","exemplo":"wall_loops.png"}}`
- Ferramentas 3D: texto vem de `GLToolbar::get_tooltip` (`GLToolbar.cpp` linha 488) e `GLGizmosManager::get_tooltip` (linha 722).
- Ligada: cartão com título, o que faz, efeito e imagem. Desligada: tooltip curto original. **Nenhuma** mudança no valor ou na função do parâmetro.

## 7. Como provar que a tela está certa

1. Abrir a referência `ui/<tela>.html` no Chrome em 1600 px de largura e salvar captura.
2. Abrir a tela real no executável compilado, mesma largura, e salvar captura.
3. Colocar as duas lado a lado no PR e listar, item por item: blocos presentes, ordem, rótulos, cores. Qualquer bloco faltando ou trocado de lugar = tela reprovada.
4. Conferir que cada botão/controle faz o que fazia antes (telas B) ou o que a 02-DADOS define (telas A).
