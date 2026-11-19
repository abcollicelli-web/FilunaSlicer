# 03 — Servidor, conta do cliente e backup automático

**Regra do dono:** tudo que o cliente faz no Filuna Slicer Windows tem cópia no servidor, de forma leve e rápida. **Arquivo 3D não sobe**: o servidor guarda só o vínculo com a pasta/arquivo no computador do cliente.

## 1. Como o servidor é hoje (fatos verificados)

- Código: `abcollicelli-web/Filuna`, pasta `site/app/` — FastAPI, SQLite (`db.py`, `sqlite_connection.py`), ~14.600 linhas.
- Autenticação: **só administrador** (`admin_users`, cookie `filuna_session`, `main.py` linhas 151–180). **Não existe conta de cliente ainda.**
- Estoque existente: `atelie_stock.py` (`atelie_stock_spools`, `atelie_stock_events`, `atelie_stock_locations`), amarrado a `owner_admin_id`. Serve de referência de esquema; **não reutilizar as tabelas** (são do ateliê do dono).
- Cofre: `credential_vault.py`. OAuth de integrações: `oauth_providers.py`.
- Releases do fatiador: `site/deploy/slicer-releases/<versão>.windows.json`; download autenticado em `slicer_access.py`.
- Leitura de G-code: `price_calculator.py::parse_orca_gcode`.
- Regras do site: `site/REGRAS.md` e `site/AGENTS.md` — **leia antes de tocar em qualquer coisa**.

## 2. O que precisa ser criado no servidor

Arquivo novo `site/app/slicer_sync.py` (rotas) e `site/app/slicer_accounts.py` (contas), registrados em `main.py` como os outros módulos. Prefixo de todas as rotas: `/api/slicer/v1`.

### 2.1 Contas de cliente
```sql
clientes(id TEXT PRIMARY KEY, email TEXT UNIQUE, nome TEXT, senha_hash TEXT,
         plano_id TEXT DEFAULT 'gratis', criado_em TEXT, ativo INTEGER DEFAULT 1)
cliente_dispositivos(id TEXT PRIMARY KEY, cliente_id TEXT, nome_pc TEXT,
         token_hash TEXT UNIQUE, criado_em TEXT, ultimo_acesso_em TEXT, revogado INTEGER DEFAULT 0)
```
- Senha: mesmo algoritmo de hash usado em `admin_users` (não criar outro).
- Token do aplicativo: 32 bytes aleatórios em base64url; o servidor guarda só `SHA-256(token)`. Um token por computador; revogável.
- Plano: `plano_id` aponta para `public_plans.json`. O que cada plano libera (módulos de fabricante, limites) **depende de aprovação do dono** — implemente a verificação lendo de um único lugar (`slicer_accounts.direitos(cliente)`), para mudar depois sem tocar no resto.

### 2.2 Login no aplicativo (sem senha digitada dentro do fatiador)
1. Fatiador abre um servidor local em `http://127.0.0.1:<porta aleatória>/retorno` e gera `estado` (aleatório) e `desafio = BASE64URL(SHA-256(verificador))` (PKCE).
2. Abre o navegador em `https://filuna3d.com/slicer/entrar?estado=…&desafio=…&porta=…&pc=<nome do computador>`.
3. O cliente entra no site (e-mail e senha). O site redireciona para `http://127.0.0.1:<porta>/retorno?codigo=…&estado=…`.
4. Fatiador confere `estado` e troca: `POST /api/slicer/v1/token {codigo, verificador}` → `{token, cliente:{id,nome,email,plano}}`.
5. Guardar o token no cofre do Windows (Credential Manager), nunca em arquivo texto.
No fatiador, implementar como **novo provedor** `filuna` ao lado de `orca` e `bbl`: classe `FilunaCloudServiceAgent : ICloudServiceAgent` (modelo: `src/slic3r/Utils/OrcaCloudServiceAgent.*`; registro em `NetworkAgentFactory.hpp` linha ~147; constante em `CloudProvider.hpp`). O login Orca continua existindo e independente.

Toda rota abaixo exige `Authorization: Bearer <token>`; resposta `401` → fatiador pede login de novo.

### 2.3 Tabelas espelho
Uma tabela por entidade de `02-DADOS §2`, com prefixo `slc_` e **uma coluna a mais**: `cliente_id`. Mesmos nomes de coluna. Chave primária `(cliente_id, id)`. Mais:
```sql
slc_rev(cliente_id TEXT PRIMARY KEY, rev INTEGER)       -- contador monotônico por cliente
```
`sync_fila` não existe no servidor.

## 3. O que sobe e o que não sobe

| Dado | Sobe? |
|---|---|
| Bobinas, tipos de filamento, eventos de estoque | Sim, completo |
| Impressoras, regras e registros de manutenção | Sim |
| Trabalhos, consumo por filamento, produtos | Sim |
| Perfis/predefinições do usuário (impressora, filamento, processo) e configurações Filuna | Sim (JSON do preset, poucos KB) |
| **Arquivo 3D (STL/3MF/OBJ/STEP) e G-code** | **Não.** Só a linha de `arquivo_local`: caminho, nome, tamanho, SHA-256, data, id do PC |
| Miniatura da peça (PNG ≤ 100 KB) | Só se o dono aprovar (decisão pendente nº 1) |
| Senhas de impressora, códigos de acesso LAN | **Nunca.** Ficam no cofre do Windows |

**Vínculo de arquivo.** Ao abrir/importar um modelo ou gerar um G-code: calcular SHA-256, gravar `arquivo_local` com o caminho completo e o `pc_id` (um UUID gerado na instalação, salvo em `app_config` como `filuna_pc_id`). No histórico, ao clicar:
- mesmo PC e arquivo existe no caminho → abrir;
- arquivo não está no caminho → marcar `encontrado=0`, mostrar "Arquivo não encontrado neste computador" com o botão **Localizar arquivo** (escolhe o novo caminho; conferir o SHA-256; se diferente, avisar "este arquivo é diferente do original" e deixar o cliente decidir);
- outro PC (`pc_id` diferente) → mostrar o item com o nome do computador de origem e o mesmo botão **Localizar arquivo**.

## 4. Sincronização (backup automático e rápido)

**No fatiador.** Toda gravação no banco passa por `FilunaDb`, que na mesma transação insere uma linha em `sync_fila` (`tabela`, `registro_id`, `op` = `upsert`, `payload_json` = a linha inteira). Um serviço `FilunaSync` (thread própria; nunca bloquear a interface) envia e recebe.

**Quando sincroniza:** 5 segundos após a última alteração (agrupa rajadas); a cada 5 minutos; ao abrir o programa; ao fechar (espera no máximo 3 s). Sem internet: a fila acumula e sobe depois. Sem login: nada sobe; o programa funciona local.

**Enviar** — `POST /api/slicer/v1/sync/enviar`
```json
{"pc_id":"…","itens":[{"tabela":"bobina","id":"…","atualizado_em":"2026-11-10T20:00:00Z","dados":{…}}, …]}   // até 200 por chamada
```
Resposta: `{"aceitos":[{"tabela":"bobina","id":"…","rev":812}], "recusados":[{"id":"…","motivo":"…"}], "rev":812}`.
Servidor, para cada item: validar a tabela contra a lista permitida e os campos contra o esquema (rejeitar campo desconhecido); incrementar `slc_rev`; gravar com esse `rev`. Fatiador: marcar `enviado=1` e gravar o `rev` no registro local.

**Receber** — `GET /api/slicer/v1/sync/receber?desde=<rev>&limite=500`
Resposta: `{"itens":[…mesmo formato, com rev…],"rev":830,"tem_mais":false}`. O fatiador guarda o último `rev` recebido em `app_config` (`filuna_sync_rev`) e aplica os itens.

**Conflito (mesmo registro alterado em dois PCs):**
- Tabelas comuns: vence o `atualizado_em` mais recente (comparação no servidor). O perdedor recebe a versão vencedora no próximo "receber".
- **`estoque_evento`, `manutencao_registro`, `trabalho_filamento`: só inserção, nunca conflito.** Por isso o peso da bobina é sempre a soma dos eventos (02-DADOS §5) — dois PCs que baixaram consumo da mesma bobina resultam na soma correta dos dois.
- `bobina.restante_g` e `impressora.horas_impressao_s` **não são sincronizados como valor**; cada lado recalcula a partir dos eventos/trabalhos.

**Restauração (PC novo ou reinstalação):** login → `receber?desde=0` em páginas até `tem_mais=false` → banco local reconstruído. Arquivos 3D aparecem como "em outro computador" até o cliente localizar.

**Tamanho e velocidade (meta verificável):** um evento de estoque ≈ 300 bytes; uma impressão completa (1 trabalho + 2 consumos + 4 eventos) < 4 KB; envio em uma única requisição < 1 s em conexão comum. Compactar o corpo com gzip.

## 5. Atualização automática (auto-update)

- Ponto de entrada existente: `GUI_App::check_new_version_sf` (`GUI_App.cpp` linha ~6623); URL em `AppConfig.cpp` linha 62 (`VERSION_CHECK_URL`). Trocar a URL para `https://filuna3d.com/api/slicer/v1/atualizacao?canal=beta&versao=<atual>&plataforma=windows`.
- Resposta do servidor (montada a partir de `site/deploy/slicer-releases/*.windows.json`):
```json
{"versao":"1.2.0","hostVersion":"1.2.0","binario":{"url":"…","sha256":"…","tamanho":123},
 "recursos":[{"id":"perfis","versao":"2026.11.10","url":"…","sha256":"…"},
             {"id":"autoajuda","versao":"…","url":"…","sha256":"…"},
             {"id":"web","versao":"…","url":"…","sha256":"…"},
             {"id":"modulo:snapmaker","versao":"…","url":"…","sha256":"…"}],
 "novidades":["…"]}
```
- **Duas camadas** (regra do dono: só recompilar quando necessário):
  1. **Recursos** (perfis, textos de autoajuda, páginas `resources/web/filuna`, módulos de fabricante): baixar o pacote, conferir SHA-256, extrair em `<data_dir>/filuna/recursos/<id>/<versão>/`, trocar o ponteiro `atual` e manter a versão anterior para desfazer. Não exige reiniciar, exceto módulos.
  2. **Binário** (só quando `hostVersion` do servidor > versão do executável): baixar o instalador, conferir SHA-256, pedir confirmação, executar e reiniciar.
- Verificar ao abrir e a cada 6 horas. Falha de download ou hash errado: descartar e manter o que está; nunca aplicar pela metade.
- Fase beta: instalador sem assinatura de código (o Windows mostra aviso SmartScreen). Assinar antes de distribuir para clientes.

## 6. Segurança mínima obrigatória

- HTTPS sempre; recusar certificado inválido.
- Cada rota confere que o registro pertence ao `cliente_id` do token.
- Limitar: 200 itens por envio, corpo ≤ 2 MB, 60 requisições/minuto por token.
- Nunca registrar token, senha ou caminho completo de arquivo do cliente em log do servidor.
- Testes do servidor em `site/tests/`: conta, token revogado, cliente A não lê dados do cliente B, conflito, restauração completa.
