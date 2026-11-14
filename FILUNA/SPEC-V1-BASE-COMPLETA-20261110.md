# Filuna Slicer — Especificação integral da v1 (base completa e sólida)

**Decisão do proprietário (10/11/2026):** a v1 nasce completa. Depois, só correções; a base cresce sólida, não remendada. Este documento é a fonte de verdade contra a qual a v1 inteira é compilada. Fonte visual: **Direção Visual C aprovada** (`FILUNA/design/visual-c/`, telas em `telas-aprovadas-20261110/`). Nada fora desta especificação sem decisão do dono.

Repositórios: **fatiador** `abcollicelli-web/FilunaSlicer` (fork AGPL do Orca); **servidor/site** `abcollicelli-web/Filuna` → `site/` (FastAPI, cofre, canal de releases, OAuth, admin-ui).

---

## 1. Identidade e marca (exatamente como as telas aprovadas)

- Nome do produto na UI, splash, instalador, atalhos e atualizador: **Filuna** (lockup "Filuna" magenta + "3D" laranja, volumétrico). Não substituir por "F" avulso.
- Tokens oficiais (de `site/marca/tokens.css`, não inventar cores):
  `--filuna-magenta-pink #FF2E8A` (destaque/ações), `--filuna-orange #FF6B1A` (acento "3D"), `--filuna-deep-purple #7B4BFF`, `--filuna-soft-purple #A78BFA`, `--filuna-cream #FFF8F0` (fundos/cards), `--filuna-charcoal #3A3A3A` (texto), fundo branco/creme.
- Layout aprovado (Visual C):
  - **Topo:** logo Filuna 3D · abas **Início · Preparação · Visualização · Dispositivo · Projeto · Calibração** · sino · **?** (ajuda) · ações à direita (Exportar / Fatiar mesa na Preparação; Revisar envio / Configurar unidade no Dispositivo) · engrenagem.
  - **Preparação:** painel esquerdo Perfis (Impressora/perfil, Bico, Tipo, Mesa), Filamento (slots com cor), Processo (Global/Objetos; Simples/Avançado/Especialista; Qualidade/Resistência/Velocidade/Suporte/Multimaterial/Outros; Paredes…), **IA Filuna**; canvas 3D central com a barra de ferramentas; rodapé com Sugestão de venda/Peso/Custo/Tempo e **toggle Autoajuda**.
  - **Dispositivo:** cabeçalho da unidade (nome, estado, impressora/perfil), LAN local, Sessão do titular, Capacidades e saúde; card de ilustração 3D com rótulo "Ilustração 3D · não é transmissão real" e Abrir câmera local; Trabalho atual (%, tempo, etapas Preparado/Aquecido/Imprimindo/Concluído, bico/mesa), Controle local (Revisar pausa/cancelamento, com confirmação do titular); abas Impressoras/Filamentos/Fila e histórico.
- Splash e instalador personalizados Filuna. Tela de abertura = Filuna.

**Pronto quando:** as telas Preparação e Dispositivo do app compilado batem com as telas aprovadas (marca, cores, disposição, rótulos), conferidas pelo dono.

## 2. Tradução pt-BR (função intacta, texto direto ao público Brasil)

- **Regra de ouro:** traduzir **somente strings de UI** (catálogos `localization/i18n/pt_BR/`), nunca comportamento. As ferramentas da barra e suas funções não mudam; só o texto/ajuda. Nunca escolher comportamento comparando texto traduzido (regra do `CLAUDE.md`).
- Sem termo em inglês e sem tradução ruim na superfície do usuário. Termos que devem permanecer em inglês por норма técnica (G-code, tokens de G-code, nomes de marca/arquivo) ficam como são, conforme o glossário.
- Cobertura 100% das strings visíveis; cada `msgid` com `msgstr` pt-BR natural e direto. QA: nenhuma string vazia ou igual ao `msgid` nas telas de uso.

**Pronto quando:** varredura das telas não acha inglês nem tradução truncada; `msgfmt --check-format` passa; comportamento idêntico ao original (mesmos ids/flags).

## 3. Autoajuda (liga/desliga, card + imagem por função e por parâmetro)

- Toggle **Autoajuda** persistente (como no rodapé da Preparação aprovada; já previsto no changelog 1.1.x "Autoajuda ON/OFF ao lado de Cinema").
- Com Autoajuda ligada: ao passar o mouse sobre uma ferramenta **ou um parâmetro**, abre um **card** com: nome em pt-BR, o que faz, detalhe do efeito, e **um exemplo em imagem**. Desligada: tooltips mínimos nativos.
- Conteúdo de ajuda em catálogo de dados próprio (texto + caminho de imagem) por `setting_id`/ferramenta, versionado e traduzível, **sem** alterar a lógica do parâmetro.

**Pronto quando:** toggle persiste; hover de ferramenta e de parâmetro mostra card com texto pt-BR e imagem; desligar volta ao tooltip simples; nenhum efeito sobre o fatiamento.

## 4. Login Orca e Login Filuna

- **Login Orca:** entrada de conta Orca nativa (fluxo aberto do upstream), sem quebrar nada do Orca.
- **Login Filuna:** autenticação contra o servidor Filuna (`site/app`, OAuth/cofre em `oauth_providers.py` + vault). Liga o cliente ao **plano contratado**: define quais marcas/módulos e recursos o cliente pode baixar (Dispositivos → módulo OEM sob demanda, conforme `REGRA-CANONICA-MODULOS-FABRICANTE-SOB-DEMANDA-20261110.md`).
- Segredos (client_id/secret, chaves de assinatura) ficam **no cofre do servidor**; o cliente nunca os embute. Tokens de acesso criptografados no cofre local, nunca retornados por HTTP (padrão já usado no `oauth_providers.py`).

**Pronto quando:** cliente faz login Filuna, o servidor devolve o plano, e Dispositivos mostra só o que o plano libera; login Orca funciona em paralelo; nenhum segredo no binário.

## 5. Auto-update integrado (beta, risco assumido pelo dono)

- O cliente verifica atualizações no **canal já existente** `site/deploy/slicer-releases/<versão>.windows.json` (campos `version`, `hostVersion`, `changes`, `tools`). Inicia com verificação automática ligada.
- **Update diferencial:** pequenos updates atualizam apenas recursos (perfis, ajuda, textos, módulos, web assets) sem recompilar; só gera **nova versão compilada** quando muda o binário (`hostVersion`). Integridade por hash; rollback.
- Beta na máquina do dono: o updater não depende de assinatura/verificação do Windows para o teste. **Nota honesta:** instalador sem assinatura dispara aviso SmartScreen para terceiros; assinatura de código entra antes de distribuição ampla (não bloqueia o teste do dono).

**Pronto quando:** app detecta uma versão nova no canal, aplica update de recurso sem recompilar, e só baixa binário quando `hostVersion` muda; rollback testado.

## 6. Sistema de plugins do Orca novo

- Adotar o mecanismo de plugins do Orca upstream atual (pin em `UPSTREAM_PIN.md`), isolado na camada Filuna. Plugins Filuna (ex.: IA Filuna, ajuda, módulos OEM) carregam por esse sistema, sem editar o motor.
- Permissões de plugin e carregamento sob demanda; nada proprietário fechado empacotado.

**Pronto quando:** um plugin Filuna de exemplo carrega e aparece na UI pelo sistema de plugins, sem alterar `src/libslic3r`.

## 7. Integração com o Filuna web server e cofre

- Cliente ↔ `site/app`: login/plano (§4), canal de update (§5), e sincronização de conta/recursos conforme `ARQUITETURA-SERVIDOR-DADOS-REMOTO.md` (opt-in; sem controle de impressora pelo admin).
- Cofre: chaves e registros ficam no servidor; o que o build precisar (endpoints, chaves públicas de verificação) é injetado na compilação a partir do cofre, não comitado.

**Pronto quando:** app conversa com o servidor para login, plano e updates; nenhum segredo no repositório nem no binário.

## 8. Ordem de construção (cada camada verificada antes da próxima)

1. **Marca + tradução pt-BR + autoajuda** no fork (sem servidor): splash/nome/instalador Filuna, catálogos pt-BR, toggle e cards de ajuda. Build Windows de verificação.
2. **Login Orca + Login Filuna** contra o servidor (plano → módulos liberados).
3. **Auto-update** diferencial contra o canal de releases; rollback.
4. **Sistema de plugins** + plugin Filuna de exemplo.
5. **Integração servidor/cofre** completa; endpoints e chaves pelo cofre.
6. **Primeiro beta** empacotado (instalador Filuna) e publicado no canal do site.

Cada camada só avança com seu "Pronto quando" cumprido. Nada de merge/release/deploy em produção sem ordem do dono. Homologação de impressão segue a regra canônica (G-code idêntico + nativo; física não bloqueia).

## 9. Bloqueios reais a resolver com o dono

- **Acesso ao cofre/segredos e ao deploy** (via SentinelX ou entrega do beta para o dono subir): necessário para login Filuna, auto-update assinado e publicação no site.
- **Verificação visual:** o build do GUI não é visível daqui; a conformidade "idêntico às telas" é confirmada pelo dono a cada camada.
- Cada build Windows completo leva horas; a convergência é por camada, não um único build cego.
