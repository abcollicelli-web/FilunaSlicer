# Filuna web — primeira fatia real na área nova (08/11/2026, Europe/Rome)

**Estado:** IMPLEMENTAÇÃO PARCIAL TESTADA EM PRÉVIA ISOLADA. Não é sistema completo, app Slicer conforme, homologação de visual, produção comercial ou promoção de gates. [Decisão básica/premium](approvals/WEB-ACESSO-BASE-ORCA-20261108.json) e [contrato de escopo](WEB-SERVICOS-ESCOPO-20261108.md).

## Guias e ambiente realmente verificados
Lidos integralmente `/srv/filuna-new-site/docs/NEW-SITE.md`, `/srv/filuna-vault/VAULT.md` e `/srv/filuna-vault/RESTORE.md`. Encontrado ambiente novo independente, HTML estático sob Nginx, apenas `public/index.html`, `admin/index.html` e guia; sem repositório Git/backend/framework próprios. A interface Nginx explicitamente citada no guia foi lida, não alterada; arquivo de autenticação/segredos não lido. Apenas os dois guias foram acessados no cofre: não houve varredura, hashing global, acesso aos conjuntos arquivados, segredo ou restauração.

Runtime já disponível: Python 3.14.4 e SQLite 3.46.1 da stdlib. Nenhuma dependência instalada. SQLite novo só para prévia, não escolha/homologação de banco comercial. Processo de prévia sem root (`www-data`), loopback. Nenhuma alteração de OS, SSH, chaves, TLS, Nginx ou serviços existentes.

## Arquivos reais no servidor — nenhum código privado publicado no Slicer
- `/srv/filuna-new-site/public/home.html` e `public/assets/site.css`: home descritiva C candidata, HTML sem JS/botões fictícios. Links internos reais; recursos avançados explicitamente planejados.
- `public/assets/`: Inter/OFL, logo PNG original extraído da imagem embutida da âncora sem alteração de pixels, geometria ilustrativa autoral e prancha Preparação C preservada. Hashes/origem em `proveniencia.json`.
- `admin/shell.html`: shell candidato sem dados/ações de clientes; não está conectado a auth de aplicação, portanto não anunciado como painel funcional. O admin de transição preexistente permanece protegido no Nginx.
- `service/app.py`: HTTP de prévia loopback com rotas explícitas e CSP/fontes/arquivos permitidos; nenhuma API de controle de impressoras, nenhum código de geração/fatiamento no servidor.
- `service/domain.py`: política básica/premium, validação estrutural não criptográfica de manifesto, primitivos de estado por tenant, CAS e recibos idempotentes.
- `contracts/access-policy.v1.json`, `module-manifest.v1.schema.json`, `openapi.v1.json`, `index.json`: registros versionados de política/estrutura/API/consumidores. Não são prova de assinatura implementada.
- `migrations/001_foundation.sql`: migração inicial aplicada ao DB vazio `runtime/preview.sqlite3`, com checksum e repetição idempotente. Nenhum dado real ou legado importado.
- `tests/test_foundation.py`: testes efetivamente executados com bancos temporários sintéticos.
- `docs/PLANO-MESTRE-SERVIDOR-WEB.md`: mestre operacional local; NEW-SITE ampliado por adendo sem remover o texto original. Cruza os mestres públicos, decisão literal, árvore, rotas, ownership, testes, rollout e rollback.
- `docs/evidencias/`: baseline dos arquivos ativos, log de testes, resultados/smoke e manifesto SHA-256 da fatia. Runtime mutável/manifesto próprio excluídos dos hashes, sem ciclo.

## Rotas e evidência de execução
Prévia remota somente `http://127.0.0.1:18765/`, acessível ao revisor mediante encaminhamento SSH local mantido; não é URL pública. Home/assets 200. `/api/v1/health`, `/api/v1/policy`, `/api/v1/catalog` 200; catálogo verdadeiramente vazio, sem dados de demonstração no DB da prévia. `/admin*`, `/api/v1/entitlements`, `/api/v1/state` retornam **503 AUTH_NOT_CONFIGURED**, inclusive token fictício, sem entregar conteúdo privado. Query não suportada 400; mutações 405; arquivos internos/legado/controle de impressoras 404. Nenhuma autenticação/signer fictícia nem token permanente no frontend.

Produção HTTPS conferida sem autenticação: `/` 200, `/admin/` 401, `/robots.txt` 200; `/api/health`, `/api/v1/health`, `/public/home.html`, `/assets/site.css` 404. Logo, candidata/API não foram publicadas no domínio. `public/index.html` e `admin/index.html` conservam hashes baseline; NEW-SITE conserva prefixo original e ganha adendo. Não afirmar integridade global do cofre por hashing que não foi autorizado/executado.

## Testes realmente executados e limites
Comando no diretório novo: `python3 -m unittest discover -s tests -v` → **33 testes PASS, 0 FAIL**. Log: `docs/evidencias/unit-tests.txt`; consolidação `RESULTADOS-FASE-01.json`; manifesto `MANIFESTO-FASE-01.json` na mesma pasta.

| Subconjunto | Evidência real | Não comprova |
|---|---|---|
| Política | básico livre em estados ativo/suspenso/expirado/desconhecido; premium só no ramo interno ativo+verificado; nenhuma janela comercial inventada | credencial/assinatura real, cliente desktop ou revogação/cache comercial |
| Manifesto | campos, pin do motor, hash/estrutura, proveniência/licença; desconhecidos inválidos bloqueados; resultado nunca autoriza instalação | verificador criptográfico, cadeia de confiança, anti-replay e entrega/instalação assinada |
| DB/migração | idempotência/checksum, rollback de falha, catálogo público sem vazamento de pago/draft, tenant A/B, suporte negado, CAS e idempotência | OIDC/RBAC real, isolamento completo por usuário/produto, sync entre PCs |
| Backup | backup/restauração de banco temporário sintético com hash/estado preservados | PITR, offsite, RPO/RTO ou DR comercial |
| HTTP | respostas reais no loopback, fail-closed, traversal/arquivos privados negados, assets/links/âncoras, HEAD e bloqueio de mutações | auditoria completa de segurança, sessão admin válida, navegador, teclado ou responsividade |

## Pendente, sem falsa conclusão
Capturas browser, revisão visual humana, responsividade/teclado/acessibilidade e fidelidade web 100% **NOT RUN**. Home não é screenshot do aplicativo nem substitui viewport funcional; estudos têm legenda. Autenticação de aplicação/OIDC/PKCE/RBAC, assinatura/trust store, assinatura comercial/pagamento, entrega de módulos/perfis, catálogo com conteúdo real/licenças, storage 3D, sync/restore real multi-PC, atualizações atômicas e rollback de cliente, central/chat permanecem por implementar/testar. Escopo autorizado não é alegação de conclusão.

A decisão de 15:18 resolve somente básico versus catálogo condicionado. Preços, provedor, dispositivos, quotas, prazo offline, cache/revogação comercial e licença por recurso não foram inventados. Nenhum arquivo próprio apagado, segundo motor instalado ou desktop/hardware retomado. G2/G3 BLOCKED, G4 NOT RUN, G5/G6 BLOCKED do Slicer não foram promovidos.
