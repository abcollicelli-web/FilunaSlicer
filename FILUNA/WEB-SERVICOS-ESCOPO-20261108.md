# FILUNA — contrato de escopo da home, admin e serviços novos

**ID:** FLN-WEB-20261108.1. **Autorização:** solicitação expressa + «esta liberado», 08/11/2026, 14:57 Europe/Rome (horário registrado na conversa do proprietário). [Texto literal e proveniência](approvals/WEB-SERVICOS-AUTORIZACAO-20261108.json). **Estado deste lote: documentação registrada neste commit; implementação, migração e deploy NOT RUN.** Este documento não afirma que a nova área já foi acessada.

## 1. Fontes mestras e precedência

[Plano mestre Slicer](PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md#web-servicos-20261108) e [mestre existente do servidor](ARQUITETURA-SERVIDOR-DADOS-REMOTO.md#web-servicos-20261108) permanecem os dois pontos normativos; este é contrato subordinado, não terceiro mestre concorrente. Reusar no servidor a estrutura efetiva definida por `/srv/filuna-new-site/docs/NEW-SITE.md`, após lê-lo. Não inventar segunda árvore, framework ou banco com base no legado. O guia NEW-SITE e os dois guias permitidos do cofre ainda não foram lidos neste lote, porque seus caminhos não existem no computador de trabalho disponível. Stack, serviços, isolamento do deploy, rotas e ownership técnico real **não verificados**.

A nova autorização supera o bloqueio de iniciar **somente o novo workstream web**; não reabre implementação geral, CI, gates ou hardware do Slicer. Regras históricas de pausa continuam válidas no restante. Testes da fatia web implementada são necessários, mas não equivalem a G2–G6 do desktop. Sem promessa de software literalmente incapaz de falhar: prevenir, detectar, testar, fazer rollout controlado e recuperar.

## 2. Área canônica e ownership

| Área | Responsabilidade / limite |
|---|---|
| `/srv/filuna-new-site` | Única raiz autorizada para código e desenvolvimento web; ler guia e inventário antes de escolher caminhos. Reutilizar stack, lockfiles, estrutura e execução reais. |
| `/srv/filuna-vault/VAULT.md` e `RESTORE.md` | Apenas leitura desses dois guias, se acessíveis; não copiar eventual segredo para documentos, código ou logs. |
| `/srv/filuna-vault` e legado | Não restaurar, manipular, migrar, explorar recursivamente nem usar chaves, envs, DB ou modelos como origem. |
| Repositório público Slicer | Só contratos/documentação públicos seguros; nenhum código web privado, segredo, topologia restrita ou dump. |
| Proprietário | Aceita visual candidato, resolve políticas comerciais/AGPL conflitantes e aprova liberação quando exigido. |
| Implementação web / segurança / dados | Responsáveis técnicos a identificar no NEW-SITE; conflito com trabalho ativo de outra pessoa impede sobrescrita. |
| Desktop / DeviceBridge | Integração contratual coordenada em fase futura, sem mudança implícita por este trabalho web. |

Não trocar domínio vivo, DNS, proxy, serviço, banco ou runtime legado por suposição. Publicação da home só após comprovar isolamento e o fluxo autorizado de deploy no NEW-SITE. Não processar cobrança, criar conta externa, contratar serviço ou enviar e-mail/chat por esta autorização genérica.

## 3. Escopo funcional e mapa de dependências

Os vínculos abaixo apontam aos IDs reais do [DAG existente](CRONOGRAMA-DEPENDENCIAS.json). Não modificam seus gates nem alegam predecessores PASS. Pesquisa, documentação e primeira fatia web isolada estão autorizadas; ativação integrada requer dependências e aceite.

| ID deste escopo | Entrega / ownership | Dependências existentes | Estado neste lote |
|---|---|---|---|
| WEB-HOME-01 | Home pública descritiva do Filuna em C, candidata para revisão | BRAND-08, PORTAL-36 (integração), SRV-00 | NOT RUN |
| WEB-ADMIN-01 | Shell admin autenticado, RBAC e auditoria; sem acesso público ao painel | SRV-01, SRV-02, LEGAL-02 | NOT RUN |
| WEB-CONTENT-01 | Conteúdo, ferramentas, paramétricos, receitas e modelos originais versionados | PARAM-29, CATALOG-30, TRUST-11 | NOT RUN |
| WEB-OEM-01 | Catálogo compatível de módulos/perfis/configurações por identidade/firmware/ABI | PROFILES-06, BRIDGE-14, TRUST-11 | NOT RUN |
| WEB-RIGHTS-01 | Contas, planos, acesso e direitos assinados; consulta assíncrona | AUTH-12, SRV-02, LEGAL-02 | NOT RUN |
| WEB-SYNC-01 | Estado, recentes, configurações, metadados, modelos e retomada em outro PC | DATA-05, SYNC-28, SRV-03 | NOT RUN |
| WEB-STOCK-01 | Registro isolado de estoque, impressoras, perfis e configurações | STOCK-31, SRV-03 | NOT RUN |
| WEB-RELEASE-01 | Versão, downloads, updates e rollback verificáveis | TRUST-11, SRV-01, QA-03 | NOT RUN |
| WEB-SUPPORT-01 | Central do assinante, ajuda e chat com acesso/privacidade | SRV-02, LEGAL-02, SRV-04 | NOT RUN |
| WEB-BACKUP-01 | Backup/restore com catálogo, isolamento e ensaio de restauração | SRV-03, DATA-05 | NOT RUN |

O inventário deve manter por recurso: ID, dono, fonte, caminho canônico, rota/API, versão, consumidor, dependências, migração, teste, evidência, rollout e rollback. Caminho sem consumidor ou rota sem handler/documentação/teste é bloqueio, não “pronto”. Não criar diretórios vazios para parecer sistema completo. Acrescentar novos nós ao DAG quando o desenho real da implementação exigir, sem remover dependências ou mudar gates para fazê-los passar.

## 4. Contratos de fronteira, sem servidor fatiador

- Um Orca compartilhado pinado a `a620d804efbe74162c96ad29d755cdab2b823530`; deltas OEM legalmente reutilizáveis isolados, completos por capacidade e verificáveis. Não baixar fatiadores completos duplicados. Rede e fatiamento são módulos distintos no contrato OEM.
- Descoberta, identificação, pareamento e online são fatos separados. Cliente identifica modelo/firmware/capacidades localmente; resolver conjunto mínimo compatível por manifesto versionado, dependências/heranças, ABI, origem, licença, hashes e assinatura. Não anunciar compatibilidade só porque existe JSON.
- Servidor armazena catálogo/base/receita/modelo original e distribui conteúdo autorizado; **geração geométrica, preparação, download no cliente e fatiamento executam no PC**. Não transformar web em cálculo de geração ou motor obrigatório.
- Admin/servidor gerem registros, direitos, conteúdo e observabilidade sanitizada consentida. **Nunca** recebem sessão LAN/câmera, códigos OEM ou ações de aquecer/imprimir/pausar/cancelar das impressoras dos clientes. Estoque e registro não concedem controle físico.
- Envio ≠ ACK ≠ início. Fila/agenda não iniciam impressão. Controle local requer titular, sessão, capacidade e confirmação. Remoto futuro permanece OFF e sujeito a gates próprios.
- G-code imutável e U1 preservados: risco OEM pode requerer aceite explícito por SHA/operador; `ENGINE_DIVERGENCE` não admite override nem licença paga como bypass.
- Catálogo Filuna print-only na experiência Filuna não apaga direitos sobre arquivos próprios/exportáveis nem promete DRM impossível de extrair.

## 5. Direitos e consulta rápida sem congelar UI

Entitlement/manifesto deve ser assinado pelo serviço apropriado e verificável localmente: versão do esquema/política, emissor e audiência, sujeito/instalação autorizada, escopos/conteúdo, validade, versão/época de revogação e proteção a replay, identificador de chave pública e algoritmo **a selecionar no projeto real**. Nenhum segredo permanente em desktop, JS, manifesto ou repositório. Tokens de sessão não são chaves de assinatura; a origem da assinatura fica em serviço restrito, não no cliente.

Renovar assincronamente, com cache local verificável e estado explícito: válido, indisponível, expirado, revogado, incompatível. Validação incorreta não concede serviço pago. Não bloquear a thread de UI com rede; preservar trabalho não salvo e sessão segura em andamento. Timeout, política de cache/offline, revogação, relógio/replay, reconexão e orçamento de latência são contratos mensuráveis **pendentes de decisão/ensaio**, não números inventados.

**Conflito a resolver antes de implementar autorização comercial:** plano mestre §1, §13 e regra de qualidade preservam Orca livre/offline e fatiamento sem login/assinatura; o novo pedido menciona chave para fatiamento/criação/preparação ao mesmo tempo que restringe “serviços pagos”. Não interpretar como revogação silenciosa do core AGPL. **Questão precisa:** a chave limita somente conteúdo/módulos/serviços premium legalmente separáveis, mantendo fatiamento/preparação básicos com arquivos próprios e perfil válido livres/offline, conforme contrato vigente? Até resolução, bloquear apenas a política conflitante; home candidata, inventário, contratos e API independente podem avançar. Não implementar bloqueio comercial global do motor básico.

Pendências comerciais adicionais: planos/preços, provedor de pagamento, número de dispositivos, janela offline, tolerância de inadimplência, quotas/retenção e regras de mudanças de plano. Não preencher defaults comerciais por conta da IA. Suspensão restringe serviços pagos conforme política aprovada, não apaga dados próprios.

## 6. Dados, conta e restauração real

Reutilizar modelo lógico do mestre servidor: contas/identidades/sessões, tenants/memberships, instalações, registro de impressoras/capacidades, estoque/ledger, projetos/objetos, sync/outbox/cursors, consentimentos, auditoria e catálogo de backup. Não anunciar tabelas ou migrações executadas sem código e evidência.

Estado de última tela/recentes/configurações é parte da retomada, **não substituto do backup de trabalhos/metadados/modelos autorizados**. Objetos 3D grandes ficam em storage privado com hash/versão, metadados e referências no DB; não inflar banco com binários por conveniência. Upload depende de consentimento/minimização e política de licença; não extrair modelos do cofre.

Tenant vem da identidade autorizada no servidor, nunca de parâmetro confiado cegamente. Verificar RBAC/ABAC por entidade/objeto, separação de usuários A/B, logs e acessos admin. Outbox persistente, IDs idempotentes, sequência/cursor e CAS/revision detectam conflitos entre PCs; nenhum last-write-wins silencioso para estoque ou modelos. “Salvo na nuvem” só após ACK durável. Restore deve reconstituir dados reais e verificar hashes num ambiente isolado, incluindo reinstalação, máquina nova, upload parcial, disco cheio, corrupção e suspensão. RPO/RTO e retenção exigem medição/decisão, não promessa absoluta. Dados próprios e export permanecem independentes do pagamento, respeitando exclusão legal consentida.

## 7. API, schemas, atualização e migrações

O contrato existente exige HTTPS/API versionada (`/v1` como padrão arquitetural, não declaração de endpoint ativo), OIDC/PKCE, DB privado e erros estruturados. A rota exata, stack e schema físico devem ser obtidos do NEW-SITE; **nenhuma nova rota de produção foi inventada ou publicada neste lote**.

Antes de implementar a fatia: registrar operações com autenticação, escopo, payload mínimo, validação, limite de tamanho, erro real, idempotência, compatibilidade e teste. Domínios necessários: sessão/direitos, catálogo e resolução de perfis/módulos, metadados de downloads/releases, sync de estado/objetos e admin. Não criar endpoint de impressão nem de acesso LAN/câmera para admin. Autenticação indisponível deve negar acesso explicitamente; não usar dashboard mock ou token de demonstração em produção.

Schemas/API/manifests com versão e compatibilidade retroativa; extensões opcionais sem quebrar cliente antigo. Pacote desconhecido/incompatível não ativa código. Manifesto associa artefatos a hash, assinatura, plataforma, ABI, dependências, origem/licença, canal e versão anterior íntegra. Instalação staging/atômica, last-known-good, retomada após queda, rollback e consentimento/reinício seguro quando aplicável. Não atualizar motor OEM, runtime ou firmware por interpretação de JSON.

Migrações versionadas e não destrutivas: expandir primeiro, validar compatibilidade com consumidores antigo/novo, backfill idempotente mensurado e auditável, só contrair depois de aprovação, backup e restore testados. Nenhuma migração do legado/cofre autorizada aqui. Rollout gradual/canário e observação sem dados sensíveis; regressão bloqueia promoção e conserva versão anterior. Nova função requer mapa de dependências/rotas e teste de links, consumidor e rollback; eliminar caminhos mortos por revisão controlada, não apagar trabalho alheio automaticamente.

## 8. Direção C e revisão da home nova

[Regra vinculante visual](design/visual-c/documentacao/REGRA-FIDELIDADE-VISUAL-C.md), [montagem/recursos](design/visual-c/documentacao/CONTRATO-MONTAGEM-VISUAL-C.md) e baseline `720291b8263e0898f7d6723bc904112ba2c6b3eb` continuam intactos. Porcelana `#F8F7F3`, grafite `#1B2024`, rosa `#A82F57`, seleção `#F2E1E7`, texto `#252A2E`, secundário `#646B72`, laranja `#E88759`, estado `#26735D`; Inter 400/500/600 com OFL e logo original, sem redesenho. Composição elegante orientada a conteúdo/3D, não dashboard genérico de caixas idênticas.

A autorização atual permite **preparar home web candidata no padrão C para revisão**, não declara aprovadas suas medidas/responsividade/interações nem as 206 linhas derivadas. Não exigir que a home pública replique abas técnicas do desktop. Quando mostrar a ferramenta, apresentar as imagens como estudos identificados, nunca como screenshot de aplicativo homologado ou UI funcional. Preparação continua sem painel direito nº4, viewport amplo e toolbar de ícone único, se representada. Só as duas âncoras têm aceite anterior; nova aprovação visual explícita fixa a referência web antes de alegar reprodução 100%/conformidade. Nenhuma arte existente é substituída por este contrato.

## 9. Aceitação da primeira fatia — todos NOT RUN neste lote

| Caso | Evidência obrigatória antes de afirmar pronto |
|---|---|
| WEB-T01 | Leitura NEW-SITE, estrutura/stack/serviços reais e prova de isolamento do ambiente novo; sem intervenção legado/cofre. |
| WEB-T02 | Home candidata renderizada, logo/fonte/tokens corretos, desktop/mobile/teclado/foco/reduced-motion; links sem rota morta, sem métricas/integrações fictícias; revisão do proprietário. |
| WEB-T03 | Admin anônimo negado pelo servidor, sessão inválida/expirada negada, role insuficiente negada, CSRF quando aplicável; shell não exposto só por URL escondida. |
| WEB-T04 | API/schema versionados válidos; payload inválido, erro indisponível e rota desconhecida sem falso sucesso; links/rotas coerentes. |
| WEB-T05 | Tenant A/B e RBAC por entidade/objeto/auditoria; nenhuma permissão admin de LAN/câmera/comando. |
| WEB-T06 | Manifesto/hash/assinatura/replay/ABI inválidos bloqueados; renovação assíncrona sem travar UI, política offline decidida e testes comerciais/core livre separados. |
| WEB-T07 | Migração limpa e repetida/backfill idempotente, compatibilidade antigo/novo, queda/rollback e restauração com hashes; nenhum dado legado usado. |
| WEB-T08 | Conflito entre PCs, retries/ACK, retomada de projeto/estado real e backup em instalação nova sem vazamento entre contas. |
| WEB-T09 | Install atômico interrompido, artefato corrompido, last-known-good/rollback; sem reinício em impressão nem download de slicer duplicado. |
| WEB-T10 | Smoke isolado e testes efetivamente implementados com versão, comandos, resultados brutos, logs sanitizados, limitações e checklist de deploy revisado. |

Não considerar contrato/teste de texto como execução dessas verificações. Uma fatia parcial deve informar exatamente o que funciona e o que está bloqueado, sem painel fake, billing improvisado ou declaração de sistema inteiro pronto. Homologação desktop/física, beta e release continuam sujeitos aos gates anteriores e não foram realizados.


<a id="base-orca-livre-20261108"></a>
## Decisão posterior — base Orca livre e catálogo Filuna condicionado (08/11/2026, 15:18 Europe/Rome)

Texto literal do proprietário:
> o fatiamento basico por regras do orca e modelo base devem serem liberados unica regra perfis e modelo homologado no filuna slicer nao carregam para o cliente e passa a usar a versao limpa do orca

[Registro da decisão e limites](approvals/WEB-ACESSO-BASE-ORCA-20261108.json). Esta decisão **resolve a dúvida comercial core básico versus premium** levantada anteriormente: fatiamento básico e modelo-base Orca livres/offline; sem assinatura válida, não carregar perfis/modelos homologados do catálogo Filuna; usar modo base limpo **na mesma engine Orca única**, sem instalar segundo aplicativo/fatiador. Não apagar/confiscar arquivos próprios, criações ou dados do usuário, nem revogar direitos de cópias sob licenças livres; respeitar licença por recurso. Prazos offline, cache e revogação de autorização de serviço continuam pendentes, sem defaults comerciais inventados.

A política foi incorporada ao domínio/API da prévia na área nova: `contracts/access-policy.v1.json`, `service/domain.py::access_policy` e `/api/v1/policy` **somente loopback**. Básico sempre permitido; carregamento premium só admite estado ativo com entrada interna de assinatura verificada pelo futuro adaptador confiável. Não confiar em booleano/query do cliente. Autenticação e assinatura reais não configuradas: API individual de direitos continua negada (503), sem concessões emitidas, sem alteração do desktop.

[Marco real da primeira fatia](WEB-FASE-01-EVIDENCIAS-20261108.md): home C candidata, API de prévia e schema/migração inicial com 33 testes executados PASS no subconjunto descrito; não equivalem a homologação visual, produto completo ou G2–G6. Cofre/legado não restaurados, tráfego público não trocado.


<a id="central-cliente-identidade-20261108"></a>
## Continuidade — Central do cliente e identidade verificável (08/11/2026, Europe/Rome)

**Novo pedido e aceite visual condicional do proprietário, t33u, 15:36 Europe/Rome — ipsis litteris:**
> esta bom a unica coisa que precisa ter precisa ser um acesso ao cliente para ele contratar, assinar, acessar seus backup, seus recursos, seus download, e isso sera expandido conforme novas ferrasmentas para o cliente serao criadas

[Registro novo da decisão e limites](approvals/WEB-CENTRAL-CLIENTE-T33-20261108.json). O visual apresentado foi aceito com a adição de acesso claro à **Central do cliente**. Manter hero, paleta C, Inter e marca; não redesenhar. Isso NÃO é homologação funcional, pixelperfect 100%, aprovação de derivadas, cobrança ou deploy geral. A entrada superior foi adicionada à home candidata; nova central tem navegação canônica `/conta/` e `/conta/acesso/`, com Contratar, Assinatura, Backups, Meus recursos e Downloads. Registro versionado de módulos governa ID/rota/owner/capability/handler; evoluir sem menus/pastas paralelos. Client e operador admin são separados.

**Estado real da fase02:** implementação somente na área nova; verificação Ed25519 real com PyJWT/cryptography, issuer/audience/datas/alg/tipo/kid controlados, identidade resolvida server-side, RBAC/membership por tenant e dados por conta; role admin exige MFA e não libera dados de clientes. Rotação/revogação de chaves/tokens e revisões anti-rollback testadas; bearer não é one-time/DPoP. Licença do recurso é verificação separada da assinatura. Manifesto JWS é validado criptograficamente, mas a API continua somente metadados, sem alegar instalação/bytes verificados no cliente.

**Operação ainda fechada:** provedor de login, configuração pública legítima de confiança e signer NÃO instalados/configurados; nenhum usuário ou direito real emitido. APIs privadas seguem 503 AUTH_NOT_CONFIGURED na prévia normal. Login browser/BFF/sessão interativa pendentes. Página pública da central explica isso em HTML navegável, sem senha/token fake, checkout inventado ou 503 bruto. Com chaves efêmeras somente em memória e DBs sintéticos temporários, testes HTTP positivos e negativos exercitaram leitura privada somente da própria conta. Não apresentar fixture como integração de produção.

[Resultados, arquivos e limites da fase02](WEB-FASE-02-EVIDENCIAS-20261108.md): **76 testes PASS** (33 baseline + 28 de segurança + 15 da central), dependências fixadas com SHA256 em venv da área nova; migrações 002/003 aditivas após backup apenas do DB novo, 001 intacta, integridade/foreign keys conferidas e tabelas operacionais vazias. Nginx/OS/tráfego/legado/cofre não alterados; nova central NÃO publicada no domínio. Código privado web e configurações não integram este repositório público.

Decisão t32 preservada: básico Orca e modelo-base livres/offline na MESMA engine; assinatura inválida impede carregar perfis/modelos homologados premium Filuna, sem apagar arquivos próprios nem direitos livres. Prazo offline/cache/revogação comercial, preços/planos/provedor e limites continuam sem defaults inventados. Storage/sync/restauração, entrega de downloads, contratação/cobrança e integração desktop não operacionais. A captura anterior em Chrome GUI offline prova apenas os trechos vistos; inspeção da nova entrada/central e mobile continuam pendentes. FLN-VISUAL-C-100, evidência humana e G2–G6/veto à retomada do Slicer permanecem.

### Contrato adicional da Central do cliente
A autorização t33 acrescenta uma central expansível, não uma segunda administração. `contracts/client-modules.v1.json` no projeto novo registra cinco módulos com versão, namespace `/conta/`, owner, capability e handler. Extensão requer schema/API/dados e ownership claros, teste de rota/permissão/compatibilidade e rollback; não duplicar componentes/menus. Novas funções não herdam automaticamente acesso a dados existentes.

APIs de prévia `/api/v1/me/{modules,backups,recursos,downloads,assinatura}` resolvem account_id exclusivamente de issuer/sub verificados; parâmetros/claims de tenant, papel e status não são autoridade. Metadados próprios não autorizam restore/download/carga premium. Rotas `/api/v1/tenants/{tenant}/...` exigem membership server-side; concessão paga exige assinatura válida, sujeito/issuer/tenant/revisão/escopo/recurso e licença correspondente. Admin não ganha bypass. Entradas inválidas, revogadas, expiradas, adulteradas e cruzadas devem negar acesso; nenhuma ausência de configuração gera PASS.

Configuração legítima futura usa apenas chaves PÚBLICAS no arquivo canônico privado da raiz nova, sem private key/credencial no frontend, código, URL ou log. Não criar dados/grants operacionais com fixtures de teste; login OIDC/PKCE/BFF e gestão de sessão devem ser conectados por provedor autorizado, sem pedir senha em chat. EdDSA/Ed25519 foi a allowlist desta fatia testada, não declaração de compatibilidade com provedor ainda não selecionado. Atualização de algoritmo/provedor exige novo contrato e teste; não aceitar alg escolhido pelo token.

A decisão t33 não cria preço, checkout nem autorização de cobrar. Contratação/assinatura podem explicar indisponibilidade e navegar às áreas; ações efetivas somente após provedor/preço/configuração autorizados. Nenhum botão pode fingir sincronização, restauração ou download. Preservar dados próprios e licenças livres em suspensão. Nenhuma promessa literal de jamais quebrar substitui prevenção/observação/rollout/rollback testáveis.


<a id="storage-restore-bytes-20261108"></a>
## Continuidade — storage, restore e entrega autorizada de bytes (08/11/2026, Europe/Rome)

Subfatia efetivamente implementada SOMENTE em `/srv/filuna-new-site`: objetos privados opacos, versões imutáveis próprias, upload explícito por consentimento, SHA256/tamanho, CAS entre PCs, idempotência e ACK após fsync/commit; restore cria NOVA versão, preserva história. Identidade/membership server-side revalidadas depois da leitura e dentro da transação. Nenhuma assinatura suspensa apaga/bloqueia arquivos próprios; básico Orca livre/offline na mesma engine preservado.

Rotas de prévia: `/api/v1/tenants/{tenant}/files`, `.../files/{file}/versions`, `.../versions/{revision}/bytes`, `.../files/{file}/restore`; download de catálogo em `.../content/{content}/bytes`, exigindo publicação aprovada/JWS real, licença independente, grant premium quando aplicável, engine/compatibilidade e bytes/hash/tamanho verificados. Não expor URL de filesystem. Nenhum artefato real/instalador foi publicado; testes usam exclusivamente conteúdo sintético temporário e chaves efêmeras em memória.

**111 PASS / 0 FAIL** (76 baseline + 35 storage), pip check PASS, sem ResourceWarning. Migração 004 aditiva JÁ aplicada uma vez após backup somente do DB novo, versões 001–003 intactas, integridade/foreign keys ok. Estado operacional: zero contas/versões/bindings/objetos, provedor/signer/configuração legítima AUSENTES, privado continua 503 AUTH_NOT_CONFIGURED. Login email/Google/browser não implementado/configurado por esta fatia; nenhum Google/SMTP/serviço externo criado. Não confundir teste positivo com concessão real.

Filesystem+SQLite **não são transação distribuída atômica**: falha antes do commit pode deixar objeto órfão privado, reportado sem deletar; referência ausente/corrompida bloqueia bytes/restore/retry. Snapshot de operador copia DB+objetos referenciados com writers excluídos, verifica integridade e só conclui manifesto após fsync. Restore efetivamente testado em diretório isolado, nunca sobre DB ativo. É backup no MESMO host, não offsite/PITR/DR comercial nem RPO/RTO comprovados; não houve corte físico de energia. Falhas IO/crash foram injetadas em teste.

Transporte inicial: default 8 MiB, configurável com teto técnico 128 MiB, timeout de body 10s; não é quota/plano/janela offline. Sem streaming/resume grande ainda. Compatibilidade firmware aceita literal `==...`; range complexo bloqueia 409, declaração do cliente não é atestação. Sem Range/ETag/CDN; Range/If-None-Match rejeitados 400. Content-Digest/hash calculados dos bytes verificados. Downloads não instalam/controlam desktop/impressora nem executam/geram/fatiam objetos.

[Arquivos, hashes, testes e lacunas](WEB-FASE-03-EVIDENCIAS-20261108.md). Contrato privado canônico `contracts/object-storage.v1.json`, OpenAPI e bindings dos módulos atualizados; mestre/NEW-SITE/ledger/continuidade locais atualizados. Cofre/legado/Nginx/OS/tráfego/domínio/Slicer/hardware não alterados; nenhum gate promovido. Sem política comercial/retenção/GC/preço inventados.

### Revisão visual posterior t34u — 15:57 Europe/Rome
> minha colaboracao as paginas estao muito com cara de wordpress travado layout pronto precisa ser moderno e perfeito como o visual do filuna pagina premiada

[Registro literal e limites](approvals/WEB-VISUAL-REVISAO-T34-20261108.json). Palavra WordPress é comparação, não autorização para instalar CMS/plugin. O pedido posterior torna o aceite visual anterior condicionado à NOVA revisão DESIGN: **home/central NÃO homologadas**. Backend não altera hero/CSS/markup e não copia template para login. Frente DESIGN candidata separada, sem segundo backend; futura integração só após revisão coordenada. CU anterior navegou Central, cinco módulos e Acesso seguro no export offline, desktop sem recortes inesperados nos trechos vistos; não expôs dados privados nem comprovou mobile/HTTP vivo/100%. Isso não anula a crítica posterior nem promove FLN-VISUAL-C-100.


## Fase04 web — dual login implementado, não ativado (08/11/2026 Europe/Rome)

**Registro literal obrigatório** — decisão do dono no widget **t33s249**, **08/11/2026 15:52 Europe/Rome**: «Quero login com e-mail e senha e também com conta Google.» Esta escolha autoriza login da aplicação por ambos os métodos, não criar Google Project/OAuth app/SMTP externo, cobrar ou enviar email real nesta execução. Corrige lacuna de registro anterior.

Backend exclusivo `/srv/filuna-new-site`: Argon2id/email verificado/reset, sessões opacas BFF/cookie Secure HttpOnly SameSite, CSRF/origem, rotação/revogação, Google authorization-code+PKCE/state/nonce e **RS256** oficial (não confundir Ed25519 de entitlements). Identity estável issuer/sub, sem auto-link por email; vínculo exige provas independentes/recentes, sem mesclar outra conta/duplicar backups. Conta/tenant/roles/subscription nunca vêm como autoridade do cliente. Google/password não emitem grant premium; básico Orca/base continua livre/offline, premium só entitlement/licença legítimos separados. Nenhum hardware/Slicer retomado.

**148 testes PASS / 0 FAIL**, incluindo111 baseline e37 novos. HTTP real loopback com RSA/Argon2 reais efêmeros, email/JWKS em memória de TESTE; não é login real Google, TLS/browser, email operacional, pentest, MFA ou produção. Config auth/trust AUSENTE, DB operacional com zero contas/credenciais/sessões/fluxos; APIs privadas503. GET `/api/v1/auth/options` público200 só explica disponibilidade; nenhum token real/usuário admin/permissão inventados. Migração005 aditiva já aplicada UMA vez após backup somenteDBnovo, integridade/FK ok. Venv/dependências13 fixadas+hashes, pip check PASS. Sem operações no cofre/legado/nginx/OS/tráfego neste workstream.

Contratos canônicos: `contracts/auth-bff.v1.json`, `auth-config.v1.schema.json`, OpenAPI/índice. Front integra JSON/viewmodels; esta fase não fez UI/CSS/home/client_portal. Requisitos/rotas/config/limites e evidência reproduzível: [FASE04](WEB-FASE-04-EVIDENCIAS-20261108.md). Escolha literal estruturada: [registro](approvals/WEB-DUAL-LOGIN-ESCOLHA-20261108.json).

**Cronologia visual separada:** crítica t34 às15:57 exigiu revisão; t35 às16:19 Europe/Rome, «aprove e publique», aprovou candidata `/workspace/filuna-web-redesign-C-20261108/` e publicação pelo executor DESIGN/deploy separado. ZIP SHA256 `874b50d79ba3b1c301f77e19455e452cb03fe808870999fd1985e47307a78839`. Este aceite visual não ativa auth/pagamento nem prova deploy; executor backend não altera frontend/processos após esta coordenação. Publicação terá evidência própria, não é afirmada neste checkpoint.

Ativação futura só com origin HTTPS/BFF/TLS exatos; cliente Google WEB legítimo client_id/client_secret/callback registrada+consent; SMTP legítimo host465 TLS/sender verificado/username/credential; config privada canônica via Vault/Form/Handoff seguro pelo pai, nunca segredos em chat. Sem esses insumos a implementação permanece fechada, não paralisada nem simulada. SMTP fila memória limitada não é outbox durável. DB SQLite/devHTTP/snapshot mesmo host não são prontidão de produção/offsiteDR. Parâmetros de sessão/anti-abuso são segurança, não política comercial offline. Nenhum gate desktop/hardware/comercial promovido.


## Fase05 privada — releases/integridade/retenção: contrato proposto, implementação bloqueada
08/11/2026 Europe/Rome. t36/t37 resumidos pelo pai exigem integração nativa desde primeira versão, resumo/reabertura seguros e histórico completo; download inicial só beta/admin, máximo CINCO VERSÕES de instaladores, novo build só necessário. t38 literal: «a assinatira da priimeira versao sera feita pelo github conforme regras». Não equivale a dispensar ECDSA manifesto, SHA256, assinatura real de código/identidadeFiluna, consentimento/salvamento ou QAWindows; attestation GitHub é proveniência separada. [Especificação pré-código e testes NOT RUN](WEB-FASE-05-RELEASE-CONTRATO-20261108.md).

**Checkpoint honesto:** somente especificação/plano local e patch documental; código/rotas/migração/serviço privado novo NÃO implementados nesta fatia. Canal SSH existente deixou de responder a comandos, embora check local do master tenha sucesso; nenhuma mutação remota, acesso alternativo, build/CI/assinatura/publicação/pruning/remoção. Fase04 148 PASS é histórica, não novo baseline executado. Estado atual DB/zero dados NÃO reverificado. Config/trust não devem ser inventados, frontend aprovado/t35/APIs públicas/nginx/cofre/legado/desktop preservados por não atuação. Retomar leitura de NEW-SITE/mestres atuais pelo mesmo acesso existente antes de código.
