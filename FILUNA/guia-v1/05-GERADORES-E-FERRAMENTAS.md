# 05 — Geradores paramétricos e ferramentas: executa no cliente, decide no servidor

**Decisão do dono (11/11/2026):** os geradores (litofania, vaso, porta-retrato…) e as ferramentas (selador 3MF, aplicador de textura…) rodam **dentro do Filuna Slicer no computador do cliente**, para não sobrecarregar o servidor. O **servidor controla** quem recebe, quem usa, quanto custa cada uso, o que o plano libera e o bloqueio no vencimento. Toda a lógica de negócio fica no servidor, para o cliente trocar de computador ou notebook sem perder nada.

**Este documento é desenho. Nada aqui está implementado.** Não alterar o que está rodando: os geradores atuais do site (`Filuna/site/app/*.py`) continuam funcionando até a migração de cada um ser aprovada.

## 1. Os dois tipos

| Tipo | O que faz | Entrada | Saída | Exemplos |
|---|---|---|---|---|
| **Gerador** | Cria uma peça nova a partir de parâmetros | parâmetros, às vezes imagem/texto/fonte | malha nova (ou 3MF com cores) colocada na mesa | litofania, vaso, porta-retrato, chaveiro, árvore de bottons, carimbo de biscoito |
| **Ferramenta** | Transforma o que já está na mesa | objeto selecionado + parâmetros | objeto alterado, modificador ou volume novo | selador 3MF, aplicador de textura, paleta de cores, imagem para 3MF |

Os dois usam o mesmo mecanismo; muda só onde aparecem na tela (§7).

## 2. O pacote do gerador

Cada gerador é um **pacote** baixado do servidor, nunca embutido no instalador.

```
litofania-1.3.0.filunapkg   (zip)
├── manifesto.json
├── gerador/            código Python
├── previa.png          imagem do catálogo
└── assinatura.sig      Ed25519 do servidor sobre o SHA-256 de todos os outros arquivos
```

`manifesto.json`:
```json
{
  "id": "litofania",
  "versao": "1.3.0",
  "tipo": "gerador",
  "nome": "Litofania",
  "kit": "base-2026.11",
  "entradas": ["imagem"],
  "parametros": [
    {"chave":"largura_mm","rotulo":"Largura","tipo":"numero","unidade":"mm","min":20,"max":250,"padrao":100,
     "ajuda":{"oque":"Largura da placa.","efeito":"Maior = mais detalhe e mais tempo.","imagem":"largura.png"}},
    {"chave":"espessura_max_mm","rotulo":"Espessura máxima","tipo":"numero","unidade":"mm","min":1,"max":6,"padrao":3}
  ],
  "saida": "malha",
  "custo": {"previa": 0, "final": 1},
  "limites": {"tempo_max_s": 120, "memoria_max_mb": 1024, "triangulos_max": 2000000}
}
```
- A tela de parâmetros é **montada a partir do manifesto** (rótulo, unidade, limites, ajuda da Autoajuda). Nenhum gerador desenha a própria tela; todas seguem `ui/parametrico.html`.
- `custo` em **usos** (unidade abstrata). Quanto um uso vale em dinheiro ou quantos usos o plano dá é decisão do servidor (§4), não do pacote.

## 3. Onde o código roda no cliente

**Reaproveitar o sistema de plugins Python que o Orca 2.5 já tem** (`src/slic3r/plugin/`). Fatos verificados no código:
- carrega pacotes Python com dependências em `.whl` (`PluginLoader.cpp`, `add_wheel_dependencies_to_sys_path`);
- tem acesso a modelo e malha pela ponte `PluginHostModel.cpp` / `PluginHostMesh.cpp`;
- tem tipos de capacidade (`Script`, `Pages`, `Importer`…) e revoga permissões a cada instalação (`PluginManager.cpp`, `revoke_plugin_permissions`).

O que criar (camada Filuna, sem tocar no motor):
1. **`FilunaGeradores`** em `src/slic3r/Filuna/`: baixa pacote, confere assinatura, instala como plugin, abre a tela de parâmetros, pede o vale (§5), executa, coloca o resultado na mesa, confirma o uso.
2. **Kit base** compartilhado: os geradores atuais do site usam `numpy`, `trimesh`, `shapely`, `manifold3d`, `scipy`, `opencv` e `svgpathtools` (verificado nos imports). São ~150 MB. Em vez de cada pacote trazer suas dependências, um **kit base versionado** (`base-2026.11`) é baixado **uma vez** pelo canal de recursos (03-SERVIDOR §5) e cada gerador declara qual kit usa. Pacote de gerador fica pequeno (KBs).
3. Execução em **processo separado** (o plugin chama um executor isolado), com `tempo_max_s` e `memoria_max_mb` do manifesto. Gerador travado ou que estourou memória é encerrado sem derrubar o fatiador.
4. Saída sempre conferida antes de entrar na mesa: malha fechada (`is_manifold`), triângulos abaixo do limite, dentro do volume da impressora.

## 4. O que o servidor guarda e decide

Tabelas novas em `site/app/` (módulo `slicer_geradores.py`), ligadas a `clientes` de 03-SERVIDOR §2.1:

```sql
geradores(id, tipo, nome, ativo)
gerador_versoes(gerador_id, versao, kit, sha256_pacote, url_pacote, publicado_em, minimo_app)
plano_direitos(plano_id, gerador_id,            -- '*' = todos
               modo TEXT CHECK(modo IN ('incluso','cota','creditos','bloqueado')),
               cota_mensal INTEGER, usos_offline INTEGER, horas_offline INTEGER)
carteira(cliente_id, creditos INTEGER, atualizado_em)
carteira_movimento(id, cliente_id, delta INTEGER, motivo, uso_id, criado_em)  -- só inserção
usos(id, cliente_id, dispositivo_id, gerador_id, versao, hash_parametros,
     estado TEXT CHECK(estado IN ('reservado','confirmado','cancelado','expirado')),
     custo INTEGER, criado_em, confirmado_em, resultado_json)
receitas(id, cliente_id, gerador_id, nome, parametros_json, atualizado_em)   -- parâmetros salvos
licencas_offline(id, cliente_id, dispositivo_id, valida_ate, usos_restantes, assinatura)
```

**Regra única de autorização** — uma função, `slicer_geradores.pode_usar(cliente, dispositivo, gerador, fase)`, devolve `sim` ou `não + motivo + como_liberar`. Ordem das verificações (a primeira que falhar decide):
1. Conta ativa e dispositivo não revogado.
2. Plano vigente: `hoje ≤ vencimento`. Vencido → `não`, motivo `plano_vencido`, como liberar: "Renovar plano".
3. Gerador ativo e versão compatível com o app (`minimo_app`).
4. Direito do plano (`plano_direitos`, linha específica do gerador vence a linha `*`):
   - `incluso` → sim;
   - `cota` → `usos confirmados no mês + reservados ≤ cota_mensal`;
   - `creditos` → `carteira.creditos ≥ custo`;
   - `bloqueado` → não, como liberar: "Disponível no plano X".
5. Limite de dispositivos do plano (computadores ativos nos últimos 30 dias).

**Prévia nunca cobra (decisão do dono, 11/11/2026).** `custo.previa` é sempre 0 e o servidor ignora qualquer outro valor. A prévia ainda passa pelas verificações 1–3 (gerador de plano vencido não roda nem prévia).

## 5. Vale de uso (como o servidor controla algo que roda no cliente)

1. Cliente clica **Gerar** → `POST /api/slicer/v1/geradores/{id}/autorizar` com `{versao, fase, hash_parametros, dispositivo_id}`.
   `hash_parametros = SHA-256` do JSON dos parâmetros com chaves em ordem alfabética, sem espaços (+ SHA-256 da imagem de entrada, se houver).
2. Servidor roda `pode_usar`. Se sim, cria `usos` em `reservado` (para créditos: grava movimento `−custo` como reserva) e devolve o **vale**:
   ```json
   {"uso_id":"…","cliente_id":"…","dispositivo_id":"…","gerador":"litofania","versao":"1.3.0",
    "fase":"final","hash_parametros":"…","expira_em":"<agora+10 min>","assinatura":"<Ed25519>"}
   ```
3. O fatiador confere a assinatura com a chave pública embutida no app, confere que o hash dos parâmetros é o mesmo que está na tela, e só então executa.
4. Deu certo → `POST /usos/{uso_id}/confirmar` com `{triangulos, sha256_resultado, tempo_s}` (nunca o arquivo) → `confirmado`.
   Falhou ou o cliente cancelou → `POST /usos/{uso_id}/cancelar` → `cancelado` e a reserva volta.
   Ninguém respondeu em 30 min → servidor marca `expirado` e devolve a reserva.
5. Mesmo cliente, mesmo gerador, mesmo `hash_parametros` já confirmado antes → **não cobra de novo**, sem limite de tempo: é a recriação de uma peça que o cliente já pagou (§5b).

**Exemplo.** Plano com `cota_mensal = 30` para litofania. No mês: 28 confirmados + 1 reservado = 29 ≤ 30 → libera. Próxima tentativa com o reservado ainda pendente: 28 + 2 = 30 ≤ 30 → libera. Depois: 31 > 30 → bloqueado, "Cota do mês esgotada — use créditos avulsos ou mude de plano".

## 5b. Histórico de gerações: guarda os parâmetros, não o arquivo

**Decisão do dono (11/11/2026):** o servidor guarda **os parâmetros** de cada geração, nunca o arquivo gerado, para não pesar. Para usar de novo uma peça gerada há uma semana, o cliente acha no histórico, o servidor reenvia os parâmetros e o fatiador **recria** a peça localmente.

```sql
geracoes(id, cliente_id, uso_id, gerador_id, versao, kit,
         parametros_json, hash_parametros,
         entradas_json,          -- referências às entradas (imagem, texto, fonte), ver abaixo
         sha256_resultado, triangulos, criado_em, nome)
```
Uma linha por uso **confirmado** (prévias não entram). Tamanho típico: menos de 2 KB.

**Recriar:**
1. Cliente abre Meu espaço → Histórico de gerações (ou o gerador → "Minhas gerações") e escolhe a peça.
2. O fatiador baixa a linha: gerador, **versão**, kit e parâmetros.
3. Recria com **a mesma versão do gerador e do kit** da época. Por isso o servidor mantém todas as versões publicadas de cada pacote (nunca apagar versão que tenha geração registrada): versão nova pode mudar o resultado.
4. Confere: o SHA-256 da malha recriada deve ser igual a `sha256_resultado`. Igual → "Peça recriada idêntica". Diferente → compara triângulos e volume (tolerância 0,1%) e avisa "recriada com pequena diferença"; nunca troca em silêncio.
5. Não cobra (§5, item 5) — **decisão do dono (11/11/2026): recriar é grátis com plano ativo.** Plano suspenso ou vencido: o histórico de gerações não abre e o botão Gerar fica bloqueado, com a mensagem exata: **"Regularize seu plano para ver suas criações e criar novas."** e o botão **Regularizar plano**. Nada é apagado; tudo volta ao regularizar.
6. Se o cliente alterar algum parâmetro antes de recriar, é geração nova (novo `hash_parametros`) e cobra normalmente.

**Imagens de entrada (litofania, foto, SVG) — decisão do dono (11/11/2026):** sem a imagem não dá para recriar, então ela é guardada, em dois lugares possíveis.

**A. Armazenamento Filuna (padrão): 50 MB por cliente.**
- A imagem sobe **como o cliente enviou** (a qualidade importa para litofania; não recomprimir).
- Tabela `imagens_entrada(id, cliente_id, sha256, tamanho_bytes, tipo_mime, local TEXT CHECK(local IN ('filuna','drive')), drive_file_id, criado_em, ultimo_uso_em)`. Mesma imagem usada em várias gerações = um arquivo só (deduplicada pelo SHA-256).
- Ocupado = `Σ tamanho_bytes` das imagens com `local='filuna'` do cliente. Limite = `50 × 1024 × 1024` bytes (valor por plano em `plano_direitos`, para o "contrate mais" funcionar).
- Ao subir imagem nova e passar do limite: apagar do servidor as imagens de `ultimo_uso_em` **mais antigo** até caber. Por isso ficam "as últimas 5, 6, 7" — quantas couberem conforme o tamanho.
  **Exemplo:** imagens de 9, 8, 7, 12 e 10 MB = 46 MB. Nova de 8 MB → 54 MB > 50 → apaga a mais antiga (9 MB) → 45 MB, cabe.
- **Antes de apagar, avisar** (nunca em silêncio): "Seu armazenamento de imagens está cheio. Para guardar esta, a imagem mais antiga (\"foto-praia.jpg\", usada em 2 criações) será removida. Contrate mais espaço com a Filuna ou use seu Google Drive para não perder seus projetos que usam imagens." Botões: **Contratar mais espaço** · **Usar meu Google Drive** · **Continuar e remover**.
- Aviso também ao passar de 80% do limite, no Meu espaço.
- Geração cuja imagem foi removida fica no histórico como "Imagem não disponível — envie a imagem de novo para recriar"; ao reenviar, o SHA-256 confere se é a mesma.

**B. Google Drive do próprio cliente (opcional, sem limite Filuna).**
- O cliente conecta a conta Google uma vez (OAuth 2.0 no site; o fatiador só usa o resultado). Escopo **`https://www.googleapis.com/auth/drive.file`**: o Filuna só enxerga os arquivos que ele mesmo criou, nunca o resto do Drive do cliente.
- As imagens vão para a pasta **"Filuna — imagens"** no Drive do cliente; o servidor guarda só `drive_file_id` e SHA-256 (`local='drive'`, não conta nos 50 MB).
- Token de atualização do Google criptografado no cofre do servidor (`credential_vault.py`); nunca no fatiador nem em log.
- Ao conectar, oferecer "Mover minhas imagens atuais para o Drive" (libera os 50 MB).
- Recriar: o servidor busca no Drive e entrega ao fatiador (ou entrega um link temporário). Se o cliente apagou o arquivo no Drive ou desconectou a conta: mesma mensagem de "Imagem não disponível".
- Estado real: o servidor tem só o cadastro de credenciais `google_login` em `integrations_hub.py`, **sem callback Google implementado**. Criar o fluxo (`/api/slicer/v1/drive/conectar`, `/callback`, `/desconectar`) e registrar o app no Google Cloud com o escopo acima. A tela de consentimento do Google precisa ser verificada pelo Google antes de abrir para todos os clientes.

## 6. Sem internet

- Ao conectar, o servidor emite uma **licença offline** assinada por dispositivo: `valida_ate = min(agora + horas_offline, vencimento do plano)` e `usos_restantes = usos_offline` do plano.
- Sem internet, o fatiador gasta da licença local e guarda os usos numa fila; ao reconectar envia todos (`POST /usos/offline`), o servidor confirma e desconta.
- Licença vencida ou sem usos → gerador bloqueado até reconectar. Plano vencido **nunca** gera licença nova.

## 7. Migrar de computador e o que acontece no vencimento

**Trocar de computador:** login no novo notebook → o servidor já tem plano, créditos, histórico de usos, **histórico de gerações** (§5b, recria qualquer peça já gerada), **receitas** (parâmetros salvos) e favoritos → o catálogo aparece igual → cada gerador é baixado só quando o cliente abre (mais o kit base uma vez). Nada fica preso ao PC antigo. O PC antigo pode ser revogado no site.

**Plano vencido:**
- geradores e ferramentas não executam (nem prévia); a tela mostra o motivo e o botão "Renovar";
- receitas, histórico e créditos ficam guardados e voltam ao renovar;
- **peças já geradas e salvas em projetos continuam do cliente**: o vencimento bloqueia gerar de novo, não abrir ou imprimir o que já foi feito;
- o fatiador base (Orca grátis + personalização Filuna) continua funcionando.

## 8. Onde aparece na tela

| Tipo | Entrada na interface | Tela |
|---|---|---|
| Gerador | aba Início → Modelos paramétricos | catálogo `ui/modelos-parametricos.html` → parâmetros `ui/parametrico.html` (o bloco "Resumo do pedido" mostra o custo: "1 uso · restam 12 este mês") |
| Ferramenta | Preparação → botão de ferramentas Filuna na barra 3D (fim da barra, sem mexer nas ferramentas do Orca) e menu do botão direito sobre o objeto | painel lateral montado pelo manifesto, mesmo estilo de `ui/reparacao.html` |

Estados obrigatórios do botão Gerar: **Gerar** (liberado), **Gerar (1 uso)** (vai cobrar), **Bloqueado — motivo** (com o botão para liberar), **Sem internet — 4 usos offline restantes**.

## 9. Proteção: o que dá e o que não dá

- **Dá:** impedir pacote alterado ou falso (assinatura); impedir uso sem direito pelo caminho normal (vale); cobrar só o que deu certo; auditar (cada uso tem dispositivo, versão, hash).
- **Não dá:** garantir que um código Python que roda no computador do cliente não seja copiado ou alterado por alguém determinado. Isso vale para qualquer programa que roda localmente.
- Consequência de projeto: gerador cujo **segredo é a própria lógica** (algo que não pode vazar) declara `"execucao": "servidor"` no manifesto e roda no servidor, como hoje. A regra §95 do site (catálogo protegido, imprimir sem entregar geometria) continua valendo e esses modelos **não** viram pacote local.
- Sinais de abuso que o servidor deve marcar para revisão humana: muitos usos confirmados sem nenhuma reserva prévia; confirmações vindas de dispositivo revogado; o mesmo vale confirmado duas vezes.

## 10. Migração dos geradores que já existem no site

Os 12 módulos atuais (`lua_litofania`, `photo_linear`, `letter_inlay`, `thermoform`, `gift_thermoform`, `arvore_bottons`, `button_magnet`, `image_to_3mf`, `color_palette_3mf`, `svg_tracer`, `cookie_stamp`, e o catálogo `parametric_catalog`) já são Python com as mesmas bibliotecas do kit base. Migrar um gerador = extrair a função de geração (sem FastAPI), escrever o manifesto, empacotar e assinar. **Um por vez, cada um com aprovação do dono.** A versão do site continua no ar até o pacote passar no teste abaixo.

**Teste de aceite por gerador migrado:** com os mesmos parâmetros e a mesma imagem, a malha gerada no cliente tem o mesmo número de triângulos e o mesmo volume (tolerância 0,1%) que a gerada hoje pelo site; uso aparece no servidor como `confirmado`; com plano vencido o botão mostra "Bloqueado — Renovar plano".

## 11. Encaixe no cronograma (04-CRONOGRAMA)

Entra **depois da Fase 6** (contas e sincronização), porque depende de conta de cliente, como uma fase nova **6b — Geradores e ferramentas**: servidor (§4, §5, §6) → `FilunaGeradores` + kit base no fatiador → telas (§8) → primeiro gerador migrado (sugestão: litofania, por ser o mais simples) → selador 3MF como primeira ferramenta.

## 12. Decisões do dono antes de implementar

1. Preço por uso, cota por plano e quais geradores cada plano inclui.
2. Horas e usos offline por plano (sugestão: 72 h e 10 usos).
3. Quantos computadores por conta em cada plano.
4. Quais geradores ficam **só no servidor** porque a lógica é segredo.
5. Preço do espaço extra de imagens (acima de 50 MB).
