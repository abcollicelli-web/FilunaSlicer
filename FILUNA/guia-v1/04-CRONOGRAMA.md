# 04 — Cronograma de implementação (fase por fase)

Cada fase tem: objetivo, onde mexer, passos, e **Aceite** (o teste que prova que terminou). Faça na ordem. Não comece uma fase sem o Aceite da anterior. Regras gerais em `00-LEIA-PRIMEIRO.md`.

Legenda de repositório: **[F]** = FilunaSlicer (fatiador) · **[S]** = Filuna/site (servidor).

---

## Fase 0 — Preparar o terreno [F]
**Objetivo:** ambiente que compila e uma base para o código Filuna.
1. Criar a pasta `src/slic3r/Filuna/` e registrá-la em `src/slic3r/CMakeLists.txt` (veja como `GUI/DeviceCore/Filuna*.hpp` já foi registrado).
2. Adicionar SQLite (amalgamation) em `src/sqlite3/` como biblioteca estática (02-DADOS §1).
3. Rodar o workflow `filuna-windows-qa-exe.yml` e confirmar executável gerado.
**Aceite:** build Windows verde com a pasta nova e o SQLite ligados; `ctest` das suítes existentes sem regressão.

## Fase 1 — Marca, nome e instalador [F]
**Objetivo:** o programa se chama Filuna e tem a cor e os ícones Filuna.
1. `version.inc` linhas 4–5: `SLIC3R_APP_NAME "Filuna Slicer"`; **manter `SLIC3R_APP_KEY`** até decidir migração da pasta de dados (mudar a chave troca a pasta `%APPDATA%` e o cliente "perde" perfis) — pergunte ao dono.
2. Cores: 01-TELAS §3-B item 1.
3. Ícones e splash: substituir `resources/images/OrcaSlicer*.{ico,png,svg,icns}` pelos da marca (`FILUNA/brand/`); splash em `GUI_App.cpp` classe `SplashScreen` (linha ~448). Pedir ao dono o logotipo com fundo transparente — os arquivos atuais têm fundo creme.
4. Instalador: `CMakeLists.txt` linhas 1369–1388 — `CPACK_PACKAGE_FILE_NAME "FilunaSlicer_Windows_Installer_V…"`, `CPACK_PACKAGE_VENDOR "Filuna"`, ícone Filuna. Manter os avisos de licença AGPL e o crédito ao OrcaSlicer na tela Sobre (`AboutDialog.cpp`) — é obrigação da licença.
**Aceite:** instalador gera atalho "Filuna Slicer"; janela, splash e Sobre mostram Filuna; nenhuma ocorrência visível de verde `#009688`; Sobre cita OrcaSlicer e AGPL.

## Fase 2 — Português do Brasil e Autoajuda [F]
1. Idioma padrão pt-BR e revisão do `.po` (01-TELAS §4).
2. Autoajuda (01-TELAS §6): chave `filuna_autoajuda`, interruptor, arquivo `resources/filuna/autoajuda/pt_BR.json`.
3. Conteúdo inicial da autoajuda: todos os parâmetros das abas Qualidade, Resistência, Velocidade, Suporte, Multimaterial, Outros e todas as ferramentas da barra 3D. Para cada um: título, o que faz, efeito, imagem. Imagens em `resources/filuna/autoajuda/img/` (PNG 480×270).
**Aceite:** primeira execução abre em português; varredura das seis abas sem inglês; interruptor liga/desliga e persiste após reiniciar; com a Autoajuda desligada o fatiamento de um cubo gera G-code **byte a byte igual** ao da Autoajuda ligada (prova de que não alterou função).

## Fase 3 — Banco local e leitor de G-code [F]
1. `FilunaDb` com o esquema de 02-DADOS §2, migrações numeradas (`PRAGMA user_version`).
2. `FilunaGcodeResumo` (02-DADOS §6) + a verificação do item 6.4 registrada em `VERIFICACOES.md`.
3. Assinatura de produto (02-DADOS §3).
4. Testes Catch2 em `tests/` (ler `tests/AGENTS.md` para saber onde colocar): leitor de G-code com os exemplos de 02-DADOS; soma de eventos (§5); fração impressa (§7); manutenção (§8); previsão (§10) — **use exatamente os números dos exemplos do guia como casos de teste**.
**Aceite:** todos os exemplos numéricos do 02-DADOS passam como teste automático.

## Fase 4 — Estoque de bobinas [F]
1. Página `resources/web/filuna/estoque.html` a partir de `ui/estoque.html`, ligada pela ponte (01-TELAS §5).
2. Cadastro individual e em lote; etiqueta com código + QR; pesagem; arquivar.
3. Diálogo "Confirmar bobinas" ao enviar para imprimir (02-DADOS §4), nativo wx.
**Aceite:** roteiro manual — cadastrar 10 bobinas de PLA branco em lote; enviar impressão; o diálogo sugere uma; confirmar; marcar concluído; só aquela bobina baixou o peso; pesar e ver o evento `pesagem`; captura da tela comparada com a referência.

## Fase 5 — Fim de impressão, histórico e painel [F]
1. Ligar os eventos de término/cancelamento/falha dos módulos de dispositivo ao fluxo de 02-DADOS §7. Onde a impressora não informa término: botão manual em Histórico.
2. Aba **Meu espaço** (`ui/meu-espaco.html`) como nova página WebView na barra (`MainFrame.cpp` ~1644), com os blocos do desenho e o bloco Painel (02-DADOS §11).
3. Manutenção: lista por impressora, situação e botão "Registrar manutenção feita".
4. Vínculo de arquivos e "Localizar arquivo" (03-SERVIDOR §3).
**Aceite:** imprimir (ou simular) o mesmo modelo 3 vezes → 1 produto com 3 impressões no ranking; cancelar uma impressão na metade → evento `falha` com metade do peso e desperdício no painel; previsão mostra "dados insuficientes" com menos de 3 trabalhos e o alerta correto com os dados do exemplo.

## Fase 6 — Contas e sincronização [S] depois [F]
1. [S] `slicer_accounts.py`, `slicer_sync.py`, tabelas `slc_*`, testes (03-SERVIDOR §2, §4, §6). Rodar a suíte do site antes e depois; nada do que existe pode quebrar.
2. [F] `FilunaCloudServiceAgent` (login), `FilunaSync` (fila, enviar, receber, restauração).
3. Indicador discreto de estado no rodapé: "Salvo no servidor" / "Sem conexão — será enviado depois".
**Aceite:** criar bobina no PC A → aparece no PC B em até 10 s; desligar a internet, registrar consumo, religar → sobe sozinho; apagar o banco local, fazer login → tudo volta; cliente A não acessa dado do cliente B (teste automático no servidor); nenhum arquivo 3D trafega (conferir o tráfego: só JSON).

## Fase 7 — Atualização automática [S] e [F]
03-SERVIDOR §5.
**Aceite:** publicar um pacote de recurso `autoajuda` novo → o programa aplica sem reinstalar; publicar `hostVersion` maior → oferece o instalador; corromper o pacote de propósito → o programa recusa e continua na versão anterior.

## Fase 8 — Demais telas [F]
Modelos 3D, Modelos paramétricos, Detalhe do modelo, Paramétrico, Reparação — WebView a partir de `ui/`. Os dados de catálogo vêm do servidor (`/api/modelos/…` já existe); respeitar a regra do catálogo protegido (`site/REGRAS.md` §95: imprimir sem entregar geometria).
Reparação: a tela comanda o reparo de malha que o Orca já tem (`docs/HLSD/mesh-repair.md`); não escrever outro algoritmo. Campos sem função correspondente no Orca: listar e perguntar ao dono, não fingir.
**Aceite:** cada tela com captura comparada (01-TELAS §7) e cada botão fazendo a função descrita.

## Fase 9 — Plugins Filuna [F]
Usar o sistema de plugins Python do Orca (`src/slic3r/plugin/`, `docs/HLSD/preset-plugin-api.md`, `python-plugin-async-bridge.md`). Entregar um plugin de exemplo Filuna e o empacotamento via canal de recursos da Fase 7.
**Aceite:** plugin de exemplo instala, aparece na interface e é removido sem deixar resíduo.

## Fase 10 — Módulos de fabricante e homologação [F]
Segue `REGRA-CANONICA-MODULOS-FABRICANTE-SOB-DEMANDA-20261110.md` e `REGRA-CANONICA-CONECTIVIDADE-OPEN-20261110.md`. Primeiro Snapmaker U1 (prova de G-code em `lab/snapmaker_source_build_parity.py`, workflow `filuna-snapmaker-source-build-parity.yml`).
**Aceite por módulo:** G-code idêntico ao do fatiador oficial na mesma versão + descoberta/conexão/envio pelo código aberto.

## Fase 11 — Primeiro beta
Instalador Filuna, publicação no canal `beta` do site, roteiro de instalação limpa.
**Aceite:** instalar em Windows limpo, fazer login, cadastrar bobina, fatiar, registrar impressão, ver o painel, receber uma atualização de recurso. **Publicar só com ordem do dono.**

---

## O que testar sempre, em qualquer fase
- G-code de um cubo de referência idêntico ao de antes da fase (a camada Filuna não pode mudar o fatiamento).
- Programa funciona **sem login e sem internet** (base grátis).
- Nenhum texto em inglês novo na tela.
- Nenhum segredo, token ou caminho pessoal em log ou repositório.
