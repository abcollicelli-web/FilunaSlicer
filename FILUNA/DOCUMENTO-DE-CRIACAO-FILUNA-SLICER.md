# Documento de criação — Filuna Slicer

**Dono do produto:** Anderson (proprietário do Filuna).
**Última revisão:** 13/11/2026. Este documento fecha a fase de criação: consolida o que o Filuna Slicer é, o que já está construído, as decisões tomadas e o que falta para o lançamento. O andamento fino continua no `REGISTRO-CONTINUIDADE-ETAPAS.md`; as regras de engenharia continuam nos documentos REGRA-* e no `guia-v1/`. Quando este documento e o registro divergirem, vale o registro (ele é mais novo por definição).

---

## 1. O que é o produto

O Filuna Slicer é um fatiador 3D para Windows (depois macOS/Linux), derivado do OrcaSlicer (AGPL), vendido como parte do sistema Filuna: **o sistema de quem vende impressão 3D no Brasil**. O posicionamento decidido em 13/11: o Filuna não compete como "fatiador universal" (o Orca é grátis e enorme); compete juntando o fatiador a custo real em reais, estoque de filamento que baixa sozinho, venda no Mercado Livre, ofertas de filamento e app no celular — tudo em português.

**As três peças e o papel de cada uma (decisão do proprietário, 13/11):**
- **Filuna Slicer** — autônomo; faz o trabalho pesado no PC (fatiar, banco local, cálculos). Funciona sem internet e sem conta.
- **Servidor (filuna3d.com)** — ponte de dados leve entre Slicer, site e app; nada pesado roda nele para o cliente. Também é a conta, o plano, o estoque espelhado e o catálogo.
- **App Android** — controle remoto e ferramenta de trabalho: enviar impressão, câmera, pausar tudo, avisos de fim/travamento, scanner de bobina (QR/código de barras/NFC).

## 2. Identidade e versões (motor de identificação)

Decisão de 13/11: o produto é um só, mas cada motor interno tem id e versão próprios, para atualização independente pelo canal de recursos (guia-v1/03 §5) e para o suporte saber o que o cliente roda. Fonte no código: `src/slic3r/Filuna/FilunaVersoes.hpp`; exposto na ponte pela ação `versoes.listar` e mostrado no rodapé do Meu espaço.

| Id | Nome | Versão atual | Cobre |
|---|---|---|---|
| — | Filuna Slicer (camada Filuna) | 1.2.0-beta | o produto; a base Orca aparece à parte (`base_orca`) |
| `cad` | Filuna CAD | 1.1.0 | aba Design: motor paramétrico (base Orca) + interface Filuna |
| `dados` | Filuna Dados | 1.2.0 | banco local, fórmulas de estoque, leitor de G-code |
| `sincronizacao` | Filuna Sincronização | 1.0.0 | fila, enviar/receber, restauração |
| `telas` | Filuna Telas | 1.2.0 | páginas internas (Meu espaço, Estoque, Histórico, Painel) |

Regras: a versão de um motor só muda com mudança real nele, registrada no registro; os ids nunca mudam (são a chave dos pacotes de atualização); o esquema do banco é o `PRAGMA user_version` do FilunaDb, não uma versão daqui.

## 3. Arquitetura construída (estado em 13/11/2026)

- **Base:** Orca 2.5.0-dev (pin `a620d804`, 05/11) **mais** as correções do 2.5.0 final aplicadas por cherry-pick (ADR de 13/11 no registro). O pin já traz plugins Python, aba Design (CAD), textura em relevo e mistura de cores.
- **Camada Filuna** (fora do `libslic3r`, exceto o que veio do próprio Orca):
  - `src/slic3r/Filuna/` — FilunaDb (SQLite local em `%APPDATA%\Filuna\filuna\filuna.db`, migrações numeradas, fila de sincronização), FilunaCalculos (fórmulas do guia 02), FilunaGcodeResumo (leitor de G-code), FilunaProduto (assinatura), FilunaVersoes.
  - `src/slic3r/GUI/Filuna*` — ponte das telas (FilunaDados), painel WebView (FilunaPanel), sincronização em thread própria (FilunaSync), diálogo Confirmar bobinas.
  - `resources/web/filuna/` — telas em pt-BR (Meu espaço, Estoque com desenho da bobina, etiqueta com QR `FLN-<id>`, Histórico, Painel), `ponte.js`, `qr.js` (QR em SVG, sem dependências), `bobina.js` compartilhado com o site.
- **Servidor:** repositório privado `abcollicelli-web/filuna-site`, branch `site-v2/producao` no ar. Rotas `/api/slicer/v1` (estoque, impressoras, comandos, fila, sincronização `sync/enviar|receber|estado`, app). Ponte Slicer↔site↔app: a bobina mantém o mesmo id nas três pontas.
- **App:** repositório privado `abcollicelli-web/filuna-app`, 0.2.0-beta (código 3) no canal beta, com NFC (ELEGOO Canvas e OpenSpool) e o desenho da bobina.
- **Etiquetas NFC** (13/11): ELEGOO Canvas (NTAG213, layout da comunidade) e OpenSpool (NDEF JSON). Bambu: impossível (assinada). Creality CFS: fora até decisão do dono (chaves extraídas pela comunidade). Anycubic/QIDI: pesquisar.

## 4. Filuna CAD — plano aprovado (maquete de 13/11)

Maquete aprovada no canvas "Filuna Design — CAD no estilo Shapr3D": a aba Design do Orca ganha interação no estilo Shapr3D, com a identidade visual Filuna (Direção Visual C). Oito telas:

1. **Face selecionada** — barra contextual (empurrar/puxar, desenhar na face, furo, casca, texto); arrastar mostra a medida ao vivo e aceita valor digitado.
2. **Aresta selecionada** — arredondar/chanfrar com raio, prévia e "aplicar nas arestas iguais".
3. **Desenhar na face** — face de frente, grade, alinhamentos, cota editável no lugar.
4. **Paramétricos Filuna** — catálogo do filuna3d.com dentro da aba, com controles; o backup guarda a **configuração** que gera a peça, nunca o arquivo gerado (REGRAS do site §95.3; guia-v1/03 §3).
5. **Recortar = extrudar negativo** — desenhar na face e puxar para dentro vira furo; alternância Adicionar/Recortar; "até atravessar".
6. **Texto e SVG na face** — relevo (+) ou gravação (−), fonte e tamanho; logotipo por SVG.
7. **Roscas e encaixes** — rosca ISO interna/externa com **folga calibrada para impressão 3D** e par gerado junto; biblioteca de encaixes (pressão, rabo de andorinha, tampa rosqueada, ímã, inserto M3). Diferencial sobre o Shapr3D.
8. **Esboço por plano e eixo** — face, planos XY/XZ/YZ, plano deslocado; revolução em torno do eixo.

**Ordem de implementação:** telas 1–3 e 5 → 8 → 6 → 4 → 7. Tudo vira passo da "Receita" (o histórico paramétrico do Orca), com peso, custo em reais e bobina do estoque na barra inferior. Cada entrega sobe a versão do motor `cad`.

## 5. Decisões de criação (consolidado)

1. **Servidor é ponte; Slicer é autônomo; app é controle remoto** (13/11).
2. **Estoque único:** a bobina criada no Slicer vai ao servidor com o **mesmo id**; etiqueta `FLN-<id>` impressa antes de sincronizar continua valendo (13/11).
3. **Backup de paramétricos guarda a configuração, não o arquivo** (REGRAS §95.3).
4. **Motores com identidade própria** (seção 2; 13/11).
5. **Aba Design ligada por padrão** no Filuna; quem desligar continua desligado (13/11).
6. **Correções do Orca por cherry-pick, não troca de pin** — o pin tem mais que a tag 2.5.0 (ADR 13/11).
7. **Privacidade dos repositórios:** site e app privados; FilunaSlicer continua público (fork AGPL, sem segredos — varrido em 13/11); abrir o código do Slicer é obrigação só quando o programa for entregue a clientes.
8. **PC do proprietário valida builds** pela sessão Claude Code (cópia portátil, nunca o instalador; resultados em `filuna-qa-prints`).
9. **Posicionamento e preço** (auditoria 13/11): vender o sistema, não o fatiador; rever teto do plano Negócio; gancho de monitoramento no grátis.
10. **Nada de segredo em repositório público**; detalhes de segurança do servidor ficam no servidor e no repositório privado do site.

## 6. O que falta para fechar a v1 (ordem de ataque)

| # | Item | Fase/origem | Estado |
|---|---|---|---|
| 1 | QA v3 no PC (cubo, fatiar, Confirmar bobinas, Histórico, etiqueta QR, menu do logo, seta do Exportar) | validação | roteiro enviado, aguardando execução |
| 2 | QA v4: sincronização real Slicer↔app (bobina criada no PC aparece no celular; ajuste volta) | Fase 6 aceite | depende do build `35de5d81` |
| 3 | Atualização automática: recursos por motor + binário pelo servidor; desligar o updater do Orca/Bambu | Fase 7 | não começou |
| 4 | Filuna CAD telas 1–3 e 5 (primeira entrega do motor `cad` 1.2.0) | seção 4 | maquete aprovada |
| 5 | Autoajuda completa (todas as abas + barra 3D, com imagens) | Fase 2 aceite | 1 parâmetro hoje |
| 6 | Varredura pt-BR completa (Início ainda tem inglês) | Fase 2 | parcial |
| 7 | Nome/instalador "Filuna Slicer" com aceite formal (os 4 CTest que seguram o APP_NAME) | Fase 1 | pendente |
| 8 | Telas restantes: Modelos 3D, Detalhe, Reparação | Fase 8 | não começou |
| 9 | Plugins Filuna pela conta/catálogo Filuna (preço com taxas ML/Shopee, previsão de falta × ofertas, lucro por produto) | Fase 9 | não começou |
| 10 | Módulos de fabricante carregando dentro do Slicer + decisão motor único × 8 motores | Fase 10 | decisão pendente do dono |
| 11 | Push em tempo real no app (FCM) e agente 24/7 sem o PC | auditoria | não começou |
| 12 | Site vendável: pagamento (Pix/cartão), página de download, termos/privacidade (advogado), promessas × entregas | auditoria | não começou |
| 13 | Beta fechado com 20 vendedores + impressão física homologada na CC2 | Fase 11 | não começou |
| 14 | Infra: cópia do backup fora do VPS; reinício do servidor (libc); 2FA do GitHub conferida | auditoria | pendente do dono |

**Critério de "fechado":** itens 1–8 feitos e aceitos = v1 beta fechada; 9–13 são o lançamento comercial; 14 é contínuo.

## 7. Como este documento se mantém

Este documento não recebe status do dia a dia. Ele muda só quando uma decisão de criação muda (seções 1, 2, 4, 5) ou quando a lista da seção 6 zera uma fase inteira. O resto vive no registro.
