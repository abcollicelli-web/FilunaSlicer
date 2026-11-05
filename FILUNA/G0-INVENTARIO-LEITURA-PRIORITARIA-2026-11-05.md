# FILUNA G0 — Inventário de leitura do manifesto (141 caminhos)

**ID** FLN-G0-READ-2026-11-05.6  
**Atualizado** 2026-11-05 23:24:00 CET (Europe/Rome)
**Gate** G0 **PASS DOCUMENTAL** (Onda 00) — ver conflitos rev .3; G1–G6 NOT RUN/BLOCKED.  
**Correção de honestidade (obrigatória):** afirmar “141 leituras integrais” seria indevido. Há três profundidades (ver tabela). Cobertura de *caminhos/hashes* = 141/141; cobertura *semântica cláusula-a-cláusula* ≠ 141.

## Cobertura

| Métrica | Valor |
|---|---|
| Caminhos no manifesto | 141 |
| Caminhos com bytes obtidos + hash conferido | **141** |
| Blobs únicos (13 prévios + 119) | **132** (9 dedup S≡W) |
| Formato | 100% texto/markdown/json neste manifesto |
| Método obtenção | `gh api` raw + `git hash-object` = blob SHA |

### Profundidade de leitura (corrigida — não misturar com “lido integral”)

| Classe | Qtd | O que foi feito de fato | Status honesto |
|---|---|---|---|
| A — semântica integral | **32+** | 13 prioritárias originais + ≥19 normativas promovidas do COVER neste lote (hashes conferidos) | `READ_SEMANTIC_FULL` |
| B — estruturada profunda | **33** | Texto completo + headings/linhas normativas (cartões §7); vários já cobrem itens §2 | `READ_STRUCTURED_DEEP` |
| C — cobertura estruturada | **≤67** | Restantes do manifesto **fora** do conjunto N1 obrigatório (MCP QA micro, etc.) ou ainda não promovidos | `READ_STRUCTURED_COVER` |
| Dedup | 9 pares | Conteúdo idêntico; profundidade = a do blob único | `DEDUP_SAME_BLOB` |

**Implicação G0 (N1):** o manifesto 141 **não** é o conjunto obrigatório. Após lote 23:12 CET, o conjunto normativo (MIGRACAO §2 + PLANO §3 + REGRA + dossiê F) está esgotado semanticamente — ver [G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md](G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md). COVER restante = histórico não-obrigatório.

### Deduplicação por hash (conteúdo processado uma vez, dois caminhos)

- `0ec123d37e9b…`: `Filuna-Slicer/CHATGPT-MCP-INSTALL-TROUBLESHOOT-2026-11-03.md` ≡ `docs/CHATGPT-MCP-INSTALL-TROUBLESHOOT-2026-11-03.md`
- `2a6dca379623…`: `Filuna-Slicer/CHATGPT-MCP-FIX-INSTALL-IDENTIDADE-2026-11-03.md` ≡ `docs/CHATGPT-MCP-FIX-INSTALL-IDENTIDADE-2026-11-03.md`
- `4ac076910f61…`: `Filuna-Slicer/CHATGPT-MCP-PLANO-MESTRE.md` ≡ `docs/CHATGPT-MCP-PLANO-MESTRE.md`
- `6e6da20a0116…`: `Filuna-Slicer/CHATGPT-MCP-TESTE-ESTOQUE-PDF-BETA-2026-11-03.md` ≡ `docs/CHATGPT-MCP-TESTE-ESTOQUE-PDF-BETA.md`
- `b889bda54b71…`: `Filuna-Slicer/CHATGPT-MCP-FUNCOES-PROPOSTAS-2026-11-03.md` ≡ `docs/CHATGPT-MCP-FUNCOES-PROPOSTAS-2026-11-03.md`
- `ba896868f265…`: `Filuna-Slicer/24-QA-CONTA-E-INSTALADOR-WINDOWS-0.1.4.md` ≡ `docs/LOGIN-ENTITLEMENT-INSTALADOR-0.1.4-2026-11-03.md`
- `dc51db402be7…`: `Filuna-Slicer/19-PUBLICACAO-PRIVADA-HOME-0.1.1.md` ≡ `docs/RELEASE-0.1.1-HOME-GROK.md`
- `e0d8124263d5…`: `Filuna-Slicer/CHATGPT-MCP-GATEWAY-PRIVADO-2026-11-03.md` ≡ `docs/CHATGPT-MCP-GATEWAY-PRIVADO-2026-11-03.md`
- `e80320eb4fcb…`: `Filuna-Slicer/20-MODELOS-3D-IMPRESSAO-PROTEGIDA.md` ≡ `docs/SECURE-CATALOG-PRINT-CONTRACT.md`

## REQ adicionais deste lote (além do inventário .2)

| REQ | Classe | Fonte | Texto condensado | Precedência F |
|---|---|---|---|---|
| REQ-G0-SEC-02 | A | S 09-SEGURANCA | Orca/AGPL nunca bloqueados por assinatura; SBOM; LGPD/GDPR; impressão remota com autorização explícita | Mantém; alinhar a SRV |
| REQ-G0-ACC-01 | A | S 11-ACESSO | Pós-vencimento: Orca livre; estoque Filuna read-only; sem apagar projetos; sync/remoto premium OFF | Mantém |
| REQ-G0-UPD-02 | A | W UPDATE-RELEASE | Update in-app assinado; A/B dados; C UI/DLL; D motor só com consentimento; pacote cumulativo | Mantém; C++ updater futuro |
| REQ-G0-PEI-02 | A | W PLATE-LAYOUT | Mesma cor/material por mesa; Z=0; multi-mesa; não cortar/escalar peça grande sozinho | Mantém |
| REQ-G0-ID-01 | B | W FILUNA-ID | Filuna ID único; OAuth Google; Stripe hospedado; segredos no cofre; sem migrar site antigo | SRV-02+ |
| REQ-G0-CAT-02 | A | S LEIA-PRIMEIRO / 20-MODELOS | Catálogo print-only; Grok só GitHub; status ≠ captura | Mantém |
| REQ-G0-BIZ-01 | A | S 02-NEGOCIO | Não vender licença mensal para rodar Orca | Mantém |
| REQ-G0-ARCH-01 | A | S 03-ARQUITETURA | Auth/pagamento nunca pré-condição do Orca offline | Mantém |
| REQ-G0-SYNC-01 | A | S 06-SYNC | Após vencimento: sem novos sync/backups Filuna; estoque histórico acessível | Mantém |
| REQ-G0-ENG-03 | A | S 05-MIGRACAO + W ENGINE | Histórico “quatro motores” — SUPERADO 05/11 para fatiamento; adapters OEM ≠ 2º motor | SUPERADO motores extras |

## Conflitos adicionais

| ID | Achado | Decisão |
|---|---|---|
| CF-06 | UPDATE-RELEASE e PENDENCIAS ainda falam em Orca/Prusa/Cura/Kiri classe D | SUPERADO para backends de fatiamento; classe D no fork F = runtime/Orca pin apenas |
| CF-07 | PENDENCIAS marca vários itens “BLOQUEANTE” do legado W 0.2.x | **Não herdar** como PASS/FAIL do fork F; reabrir como REQ novos com evidência F |
| CF-08 | FILUNA-ID pede credenciais Google/Stripe no cofre | Inventário apenas; **não** provisionar nem pedir segredos neste G0 |

## Lote prioritário §7 (33 fontes deep) — cartões

### `Filuna-Slicer/01-DIAGNOSTICO.md`
- blob `21c80a15d0fc860ea4fbdd1356fc1e7a170771a3` · 3322 B · 28 linhas · status **READ_INTEGRAL**
- título: # 01 — Diagnóstico do projeto existente
- extrato: | Área | Origem observada | Situação e ação |
- trechos normativos (amostra):
  - L15: | Suporte | `app/suporte_ia/` | Fases iniciais relatadas como publicadas, não homologadas comercialmente. |
  - L19: Processamento pesado de usuários em servidor mistura compute e dados, gera filas, demanda CPU/RAM e aumenta custo conforme crescimento. Vários geradores são publicados mas não homo
  - L22: **O PC calcula; a nuvem guarda, autentica, sincroniza, vende, notifica, compartilha e permite comando seguro.** Ferramentas geométricas são oferecidas no Filuna Slicer. O site perm
  - L25: O `AGENTS.md` e `docs/ARQUITETURA.md` atuais ainda descrevem vertente Web com slicing no servidor; regras de gate paramétrico e upload de modelos ainda exigem pipeline Web. Este pl

### `Filuna-Slicer/02-NEGOCIO.md`
- blob `fd6ed491d5baa4f34889eb5ed0e1c7bf2cd2085d` · 4182 B · 39 linhas · status **READ_INTEGRAL**
- título: # 02 — Plano de negócio e operações
- extrato: **Não vender licença mensal para continuar rodando Orca.** O motor livre continua disponível com configuração manual de impressoras, fatiamento e G-code. A assinatura inclui integrações adicionais **p
- trechos normativos (amostra):
  - L4: **Não vender licença mensal para continuar rodando Orca.** O motor livre continua disponível com configuração manual de impressoras, fatiamento e G-code. A assinatura inclui integr
  - L18: ## Planos comerciais — proposta, não preço aprovado
  - L21: | **Base gratuita** | Motores livres integrados conforme respectivas licenças, preparo/slicing e arquivos locais, sem conta obrigatória | Nunca bloquear Orca após cancelar plano. |
  - L24: | **Consumo opcional** | IA externa e storage acima da cota, quando efetivamente usado | Preço e autorização explícitos; nunca para o fatiador livre. |
  - L26: **Licença:** a presença de um programa/plug-in premium dentro de um conjunto com AGPL pode obrigar distribuição de fonte e outros direitos. Não alegar que basta separá-lo em pastas
  - L29: Receita de assinaturas, armazenamento/assentos extras, créditos de IA custosa e afiliados de ofertas (não tratar afiliado como ARR).

### `Filuna-Slicer/03-ARQUITETURA.md`
- blob `8455497782f2394573abbb77f474a3f962a90bd1` · 5196 B · 57 linhas · status **READ_INTEGRAL**
- título: # 03 — Arquitetura única do Filuna Slicer
- extrato: Nenhuma chamada de autenticação, pagamento, entitlement ou rede deve ser pré-condição para executar **Orca e as funções livres**, importar modelos e gerar G-code. Expiração desabilita apenas component
- trechos normativos (amostra):
  - L4: Nenhuma chamada de autenticação, pagamento, entitlement ou rede deve ser pré-condição para executar **Orca e as funções livres**, importar modelos e gerar G-code. Expiração desabil
  - L34: A interface unificada do Filuna Slicer seguirá como referência **viva** o protótipo definido pelo proprietário: https://flora-beam-zippy-monarch.grok.me/. Sua navegação e seus card
  - L41: Capacidades declaradas (FDM, multimaterial, suportes, pressão, perfis, velocidades, formatos) impedem seleção de motor incompatível. Uma opção avançada define **motor por trabalho*
  - L45: **Cloud:** identidade, plano, organizações, consentimentos, dados sincronizados confirmados, revisões e acervo de arquivos; o servidor não precisa inspecionar geometria para guarda
  - L51: Não embarcar chaves do servidor no instalador; serviços de IA externos ficam mediadas por API com limite.
  - L54: O Kiri:Moto indica possibilidade de execução browser-side; apenas quando a POC mostrar impressão equivalente, isolamento de dados e performance em navegadores reais. Para outros mo

### `Filuna-Slicer/05-MIGRACAO.md`
- blob `8dfb028215884eb022d25f2add1c708818119f41` · 4442 B · 38 linhas · status **READ_INTEGRAL**
- título: # 05 — Matriz de migração do site para o Filuna Slicer
- extrato: Depois da migração, o usuário continua usando os quatro motores nos termos de suas licenças, sobretudo **Orca com configurações manuais de impressora**. Recursos adicionais Filuna (integrações especia
- trechos normativos (amostra):
  - L4: Depois da migração, o usuário continua usando os quatro motores nos termos de suas licenças, sobretudo **Orca com configurações manuais de impressora**. Recursos adicionais Filuna 
  - L21: | Paramétricos rejeitados anteriormente | **não ativar** sem motor reconstruído e homologação explícita | nunca anunciar como operacionais | não | Bloqueado |
  - L27: | Farm, fila, sensores, impressão remota | LAN agent, comando e estado real | pedir/acompanhar, não fatiar | painel e comando | P2 |
  - L30: | Web Lite | pode abrir resultado | browser compute POC, **nunca VPS** | opcional | P3 |
  - L34: - Portar Python mediante runtime local isolado ou implementação equivalente, sem entregar chave secreta; sempre priorizar reuso dos **motores canônicos** de SVG e traço, não reescr
  - L35: - Ferramentas não homologadas no Web não se tornam homologadas só porque foram portadas.

### `Filuna-Slicer/06-SYNC-E-RECUPERACAO.md`
- blob `fce6695146213bf09e223ee19b8d9ba186aa6f3b` · 4464 B · 44 linhas · status **READ_INTEGRAL**
- título: # 06 — Sincronização, cópia cloud e restauração
- extrato: Após vencimento, a Filuna suspende novas sincronizações, backups e alterações de estoque por serviços próprios; **estoque e histórico já salvos continuam visíveis e exportáveis**, arquivos locais abre
- trechos normativos (amostra):
  - L4: Após vencimento, a Filuna suspende novas sincronizações, backups e alterações de estoque por serviços próprios; **estoque e histórico já salvos continuam visíveis e exportáveis**, 
  - L8: Ao reinstalar Windows ou trocar computador, recuperar **tudo que foi salvo e sincronizado com confirmação**: estoque, movimentações, orçamentos e versões, projetos 3MF/STL escolhid
  - L17: Eventos: `tenant_id`, `user_id`, `device_id`, `event_id` UUID, `entity_id`, `schema_version`, `base_revision`, `occurred_at`, `payload_hash`, `payload`. Chave de idempotência `tena
  - L21: - **Estoque:** movimentos de ledger, saldo derivado; não sincronizar saldo como texto sobrescrevendo alteração concorrente.
  - L23: - **Arquivos 3MF/STL/SVG:** blobs binários com SHA256, upload resumível, versão, status e quota; dois PCs editando mesmo projeto criam conflito preservado, não merge automático.
  - L25: - **Impressões:** eventos `preparado -> iniciado -> em andamento -> concluído/falhou/cancelado` com origem do dispositivo; não inferir sucesso de comando HTTP.

### `Filuna-Slicer/07-WEB-ANDROID-IMPRESSAO.md`
- blob `92df222c5d9b9e8905b170d2effc6b799279f22b` · 3823 B · 43 linhas · status **READ_INTEGRAL**
- título: # 07 — Filuna Web, Android e impressão online
- extrato: O site público segue catálogo Filuna. Conta Web permite renovação/cancelamento, consulta e portabilidade de registros já existentes; **não** permite novas alterações no estoque Filuna ou serviços prem
- trechos normativos (amostra):
  - L4: O site público segue catálogo Filuna. Conta Web permite renovação/cancelamento, consulta e portabilidade de registros já existentes; **não** permite novas alterações no estoque Fil
  - L9: **Preservar caminhos atuais** e metadados para não quebrar visitas do Google; nos antigos POSTs geométricos, migração programada/explicação e compatibilidade antes de desativar.
  - L26: **Orçamento:** PC obtém tempo/gramas/purga de job real do motor selecionado, calcula preço conforme configuração do usuário, registra versão e envia metadados via sync. Web pode ed
  - L37: O PC do usuário **precisa estar ligado, conectado e autorizado**. Não expor sua LAN/IP à internet, não abrir porta pública, não aceitar shell, scripts externos ou upload arbitrário
  - L40: MVP: login, estoque, orçamentos, histórico, projetos, notificação e monitor de máquina, com ação Abrir no PC. Próximo: pareamento seguro, operação limitada e configurações. Não pro

### `Filuna-Slicer/08-ETAPAS.md`
- blob `b3f16c1e36c2e19e61fac5721b34ac48269182de` · 4573 B · 38 linhas · status **READ_INTEGRAL**
- título: # 08 — Etapas de desenvolvimento, dependências e cronograma lógico
- extrato: Cada etapa exige testes de `ACTIVE`, `EXPIRED`, `CANCEL_AT_PERIOD_END`, `PAST_DUE` e `NETWORK_UNAVAILABLE`: Orca continua fatiando e com impressora manual; estoque antigo é legível/exportável, sem edi
- trechos normativos (amostra):
  - L4: Cada etapa exige testes de `ACTIVE`, `EXPIRED`, `CANCEL_AT_PERIOD_END`, `PAST_DUE` e `NETWORK_UNAVAILABLE`: Orca continua fatiando e com impressora manual; estoque antigo é legível
  - L7: **Esta pasta contém plano, não execução.** Aprovação do proprietário e alteração formal de governança são pré-requisitos antes de qualquer implementação. Nunca estimar prazo sem eq
  - L13: | **2. Conta Cloud** | multi-tenancy, autenticação, RBAC, planos e entitlements, cobrança/cancelamento homologados | teste A/B não vaza; webhooks idempotentes; Orca funciona sem pl
  - L16: | **5. Segunda família Prusa** | adapter Prusa opcional e perfis mapeados | resultado comparável e logs de diferenças; engine não duplica UI. |
  - L35: Frentes: Cloud/auth/billing, Desktop/UI/SDK, Engines, Ferramentas/Paramétricos, Catálogo/SEO, Android/Impressoras, Segurança/QA. Se for equipe pequena, executar sequencialmente con
  - L38: Para cada endpoint público: confirmar que há substituto local disponível; manter GET de catálogo e links; comunicar transição; medir chamadas; bloquear POST pesado com resposta cla

### `Filuna-Slicer/09-SEGURANCA-LICENCAS.md`
- blob `6b5f83d8de9fbe7a6c5af7219ae9d22f152f3f73` · 3904 B · 30 linhas · status **READ_INTEGRAL**
- título: # 09 — Segurança, licenças, privacidade e custos
- extrato: **É vedado bloquear a execução do Orca ou de recursos cobertos por AGPL/GPL em razão de pagamento vencido.** A cobrança se aplica a serviços e extras Filuna legalmente separados, não a simples adaptaç
- trechos normativos (amostra):
  - L4: **É vedado bloquear a execução do Orca ou de recursos cobertos por AGPL/GPL em razão de pagamento vencido.** A cobrança se aplica a serviços e extras Filuna legalmente separados, n
  - L8: - **OrcaSlicer:** AGPL-3.0, derivado de Bambu Studio/PrusaSlicer.
  - L10: - **CuraEngine:** AGPL-3.0-or-later conforme repositório oficial; confirmar todas as dependências e assets.
  - L12: - **Motores proprietários Filuna:** modelo de licença dependerá da integração concreta com código AGPL. Processo separado ou API **não garante** ausência de copyleft; avaliar se ex
  - L13: - O Filuna pode cobrar por serviço e suporte sob licença livre; "pago" não significa automaticamente "código fechado". Não prometer DRM inviolável.
  - L14: - Fazer SBOM e documentação de fontes, revisões de AGPL e licenças comerciais de qualquer SDK de fabricante. APIs Bambu/cloud podem ter termos separados do código aberto.

### `Filuna-Slicer/10-APIS-E-QUALIDADE.md`
- blob `360e5029977e4718d8db342dcd0b76caac9f40e7` · 5111 B · 66 linhas · status **READ_INTEGRAL**
- título: # 10 — Contratos de dados, APIs, QA e métricas
- extrato: Testar `ACTIVE`, `CANCEL_AT_PERIOD_END`, `PAST_DUE`, `EXPIRED`, `NETWORK_UNAVAILABLE` e `REACTIVATED`. Em EXPIRED, Orca fatia, exporta G-code, abre arquivos e aceita configuração manual das impressora
- trechos normativos (amostra):
  - L3: ## Estados de acesso e novos gates obrigatórios
  - L4: Testar `ACTIVE`, `CANCEL_AT_PERIOD_END`, `PAST_DUE`, `EXPIRED`, `NETWORK_UNAVAILABLE` e `REACTIVATED`. Em EXPIRED, Orca fatia, exporta G-code, abre arquivos e aceita configuração m
  - L32: Nulos significam métricas não demonstradas. Converter preferências Filuna para engine sem inventar parâmetros inexistentes. Relatório identifica versão de perfis e proveniência de 
  - L41: **Não implementar esses endpoints como rotas sem contratos completos, testes e revisão de segurança.**
  - L49: | Conta | A não pode consultar/provocar ação nos recursos de B; logs e testes de autorização. |
  - L51: | Restore | Disco novo restaura 100% dos itens com ACK+hash existentes, informando itens nunca enviados. |

### `Filuna-Slicer/11-ACESSO-POS-ASSINATURA.md`
- blob `d03ac2dec467d369bd4591c7aaa74c8a7cec2c95` · 7500 B · 70 linhas · status **READ_INTEGRAL**
- título: # 11 — Política oficial planejada: assinatura vencida
- extrato: **Decisão expressa do proprietário em 2026-11-02.** Fonte normativa de acesso do projeto **Filuna Slicer**. **Somente planejamento no GitHub**; nenhuma implementação, alteração do Windows, Ubuntu ou s
- trechos normativos (amostra):
  - L6: **A assinatura nunca é condição para abrir o Filuna Slicer ou continuar utilizando o motor Orca e as funcionalidades cobertas por licenças livres.** O fatiador continuará funcionan
  - L8: A assinatura paga **serviços conectados à Filuna e recursos adicionais que sejam juridicamente separáveis dos motores AGPL/GPL**. Orca, PrusaSlicer, CuraEngine e Kiri:Moto mantêm o
  - L20: | Criar/editar estoque e registrar consumos/movimentações no módulo Filuna | SIM | NÃO |
  - L22: | Ferramentas adicionais Filuna elegíveis, inclusive conversões e reparos próprios | SIM | NÃO; não restringir funções equivalentes já livres |
  - L23: | IA Filuna, créditos e automações premium | Conforme plano/créditos | NÃO |
  - L24: | Sync PC↔Web↔Android, novos backups e upload de projetos | SIM | NÃO |

### `Filuna-Slicer/12-REFERENCIA-VISUAL-OFICIAL.md`
- blob `c997027e7dcfe8df318535f4fc0347a92f3a09a7` · 6673 B · 72 linhas · status **READ_INTEGRAL**
- título: # 12 — Referência visual oficial do Filuna Slicer
- extrato: **Decisão explícita do proprietário:** 2026-11-02. **Estado:** referência de design aprovada como base, NÃO homologação funcional do sistema.
- trechos normativos (amostra):
  - L3: **Decisão explícita do proprietário:** 2026-11-02. **Estado:** referência de design aprovada como base, NÃO homologação funcional do sistema.
  - L11: **Hierarquia de referência (para design, não para licenças ou segurança):**
  - L15: 4. Identidade visual Filuna e assets oficiais já documentados no projeto existente — nunca redesenhar a marca de forma arbitrária.
  - L31: As capturas representam uma **fotografia da referência**, não substituem o site vivo. Elementos do ambiente de autoria Grok (ex.: faixa “Created with Grok”) **não fazem parte da in
  - L42: | Recentes | Chaveiro Botton; Lua Litofania LED; Foto Linear 3D; Placa vazia — **exemplos visuais**, não trabalho realmente salvo |
  - L46: **Identidade:** base clara/creme, painéis claros, texto carvão, destaques magenta, laranja e roxo de apoio. Respeitar os tokens oficiais da marca Filuna já existentes em `site/marc

### `Filuna-Slicer/16-TESTES-BETA-PRIVADA.md`
- blob `caf30e1063bf335f8bbcd61ed86de0f5c7fd650a` · 2456 B · 15 linhas · status **READ_INTEGRAL**
- título: # Teste privado — execução e critérios (2026-11-02)
- extrato: - **NÃO é lançamento ao público**. Beta do proprietário apenas. Data de lançamento pretendida: 15/12/2026.
- trechos normativos (amostra):
  - L3: - **NÃO é lançamento ao público**. Beta do proprietário apenas. Data de lançamento pretendida: 15/12/2026.
  - L6: - Auth: convite único, senha scrypt, sessão 24h/revogação. O código de convite é secreto e não existe em documentos.
  - L8: - Sync: cliente Windows e portal Web autenticam mesma conta de homologação, produzem eventos por tenant com revisão CAS/idempotência, enviam e leem registros `setting`. Teste do ba
  - L9: - Update: Windows verifica após restaurar login quando abre e a cada 30 minutos, feed assinado ECDSA P256, protegido por bearer, SHA256 e ZIP seguro, notas exibidas e ativação medi
  - L10: - QA Chromium: HTML real da home e conta em 1536/1024/940px sem scroll horizontal ou exceções JS. Capturas no Ubuntu `/srv/filuna-slicer-windows-new/runtime/qa/filuna-windows-home-
  - L12: - Publicação site: único ajuste de infraestrutura no legado foi nova localização reversa nginx `/filuna-slicer-dev/` para `127.0.0.1:17865`, via script canônico e com backup; dados

### `Filuna-Slicer/20-MODELOS-3D-IMPRESSAO-PROTEGIDA.md` (dedup≡`docs/SECURE-CATALOG-PRINT-CONTRACT.md`)
- blob `e80320eb4fcb53aa97b46ad0a36dd8a2abe6367f` · 7645 B · 59 linhas · status **READ_INTEGRAL**
- título: # Modelos 3D do catálogo Filuna — licença de impressão protegida
- extrato: Data da decisão: 2026-11-03.
- trechos normativos (amostra):
  - L3: Estado: REGRA DE PRODUTO APROVADA; IMPLEMENTAÇÃO PENDENTE. Não declarar como funcional.
  - L6: O objetivo é impedir que alguém assine por um único mês, copie os arquivos digitais dos modelos autorais do proprietário e depois cancele. O serviço concede acesso para imprimir, n
  - L8: - FILUNA_CATALOG_PROTECTED: modelo 3D publicado no catálogo do site Filuna, especialmente arquivo original do proprietário; somente visualizar, abrir em sessão protegida do Filuna 
  - L13: A classificação é atribuída no servidor a partir da origem verificada e licença de cada item; não pode mudar por renomear um arquivo nem copiar um modelo para outra categoria. Um a
  - L16: 1. No site, em Modelos 3D, o cliente escolhe o item e vê uma opção de Abrir no Filuna Slicer ou Imprimir. Não mostrar Baixar STL, Baixar 3MF, Baixar ZIP ou exportar geometria.
  - L17: 2. O site cria uma sessão de impressão autenticada, temporária e vinculada a conta, plano, modelo e dispositivo. “Túnel” significa conexão HTTPS autenticada site–serviço Filuna–sli

### `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2026-11-03-PARAMETRICOS-EDITOR.md`
- blob `7c61b6acafa65dec51e574bf81b167b4abb93008` · 1888 B · 37 linhas · status **READ_INTEGRAL**
- título: # Paramétricos — galeria e ferramentas do item
- extrato: - **ID único:** `filuna.parametricos-editor`
- trechos normativos (amostra):
  - L5: - **Status:** PROPOSTO. Visual. Não gera malha.
  - L11: Esta nota corrige `2026-11-03-PARAMETRICOS.md`. A lateral do editor não é a lista de nomes.
  - L14: Paramétricos abre o catálogo, em cards, dentro da área clara. A página não cresce além da janela. Se houver muitos cards, rola só essa área.
  - L20: - À direita: o render, preso na altura que sobra. Não rola a janela.
  - L31: Gerar não baixa arquivo neste esboço.
  - L34: Os nomes vêm de `site/app/parametric_catalog.py`, só os publicados. Orca e Prusa não montam o chaveiro. Eles fatiam o 3MF depois que o gerador Filuna existir.

### `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2026-11-03-PARAMETRICOS.md`
- blob `db2774fc2ac56c718bda8f5d27adeca24542105f` · 4628 B · 82 linhas · status **READ_INTEGRAL**
- título: # Tela Paramétricos — alteração proposta pelo Grok
- extrato: - **ID único:** `filuna.tela-parametricos`
- trechos normativos (amostra):
  - L5: - **Status:** PROPOSTO. Visual no esboço. Não gera malha.
  - L9: - **Menu/rota de destino:** na home, item lateral `parametric` «Paramétricos». Não é aba nova da barra.
  - L16: Fora da lista: `arvore-porta-bottons`, porque o catálogo marca «em validação» e manda não exibir.
  - L18: O miolo claro não tem o gato, nem Recentes, nem Modelos, nem Ferramentas. Tem o render, a barra de vista e o rodapé de formato e Gerar.
  - L23: Escolher um paramétrico do Filuna, ver a prévia e os ajustes que o site já usa. Gerar o arquivo continua no gerador real do site, não neste esboço.
  - L55: A vista tem os botões Ver em 3D, Mais, Menos, Girar, Frente, Explodir e Luz. No esboço eles só avisam. Não movem malha.

### `Filuna-Slicer/GROK-GITHUB/ESTADO-DO-PROJETO.md`
- blob `a5aed3b4bf811b95bb67369093eac9ab67dfe35e` · 7806 B · 44 linhas · status **READ_INTEGRAL**
- título: # ESTADO ATUAL — Filuna Slicer e Grok (03/11/2026)
- extrato: **Comece por:** `Filuna-Slicer/19-PUBLICACAO-PRIVADA-HOME-0.1.1.md`, `Filuna-Slicer/14-CONTINUIDADE-INTEGRAL.md`, `Filuna-Slicer/18-HOME-GROK-2026-11-03.md` e `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2
- trechos normativos (amostra):
  - L5: A Home do NOVO Filuna Slicer Windows foi **aprovada pelo proprietário e publicada na beta privada 0.1.1**, a partir do projeto isolado `/srv/filuna-slicer-windows-new`, Git local `
  - L9: **NÃO EXISTE / NÃO HOMOLOGADO:** instalação efetiva do novo instalador num Windows real, fatiamento em G-code e impressão real pela integração Filuna, conta real do proprietário va
  - L11: **Regras a observar:** este projeto foi iniciado do ZERO, sem misturar o aplicativo antigo/site. Orca livre e seu motor continuam disponíveis offline mesmo sem assinatura. O código
  - L13: **Atualizações:** ferramentas e paramétricos pequenos homologados usam pacote interno assinado, checagem após login na abertura e a cada 30 minutos, ativação com confirmação/reiníc
  - L19: Modelos do catálogo 3D Filuna têm licença exclusivamente para impressão no futuro Filuna Slicer. Nenhum STL/3MF/ZIP ou geometria original exportável deve aparecer para cliente; ape
  - L23: - SOMENTE Lua Litofania LED implantada no gerador local beta do NOVO Filuna Slicer, não outros 21 geradores.

### `Filuna-Slicer/GROK-GITHUB/LEIA-PRIMEIRO-GROK.md`
- blob `2aa8f76678a68deeb4f42dffab70c7a19f8ddc87` · 6363 B · 54 linhas · status **READ_INTEGRAL**
- título: # Filuna Slicer — DIRETÓRIO OFICIAL DO GROK (GitHub)
- extrato: **Leia primeiro este arquivo.** A partir de 2026-11-02, **o GitHub é o único canal de registro e monitoramento de mudanças feitas pelo Grok**. Não criar robôs para visitar o site do Grok, não criar we
- trechos normativos (amostra):
  - L3: **Leia primeiro este arquivo.** A partir de 2026-11-02, **o GitHub é o único canal de registro e monitoramento de mudanças feitas pelo Grok**. Não criar robôs para visitar o site d
  - L21: **Sempre anexar ao documento, ou linkar por caminho dentro desta pasta, os materiais visuais relevantes** (capturas, referência, nome dos controles, estados). Evitar snapshots enor
  - L25: > Você está colaborando com o novo projeto **Filuna Slicer**. A aparência do protótipo https://flora-beam-zippy-monarch.grok.me/ é aprovada pelo proprietário, mas não equivale a im
  - L29: - **Monitor GitHub** confere periodicamente se há documentos novos ou modificados; traz as novidades para análise. Não abre o site Grok.
  - L32: - **Licenças livres não são bloqueadas** por vencimento da assinatura. Serviços e módulos Filuna legalmente elegíveis podem exigir plano.
  - L33: - **Não usar produção /srv/site** nem alterar o app existente.

### `docs/AGENTS.md`
- blob `a07321665dd9023541bc29cfa5b9d3ea10c352cc` · 3397 B · 19 linhas · status **READ_INTEGRAL**
- título: # Normas para IA — NOVO Filuna Slicer
- extrato: 1. Antes de criar menus/ferramentas, ler `docs/PROJECT_STATE.json`, `docs/GROK_CONTRACT.md`, `docs/ARCHITECTURE.md` e `docs/ENGINE_CONTRACT.md`.
- trechos normativos (amostra):
  - L3: 2. Este projeto é **clean-room**: NUNCA copiar código de `/srv/site` ou do app Windows antigo; o código novo vive apenas neste diretório e depois em um repo privado novo.
  - L4: 3. O design visual vivo está em https://flora-beam-zippy-monarch.grok.me/; o canal obrigatório de atualizações é o GitHub em Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/. **Não monitora
  - L6: 5. Orca, Prusa, CuraEngine e Kiri:Moto são engines consolidadas; NÃO alterar seus algoritmos nem perfis nativos. O adaptador Filuna é responsável por preservar coordenadas, offsets
  - L7: 6. Não declarar integração operacional ou impressão homologada sem adapter construído e prova de comunicação real. Isso é teste de integração Filuna, não re-homologação dos engines
  - L8: 7. Nunca bloquear funções livres AGPL/GPL por inadimplência; restringir somente serviços e módulos Filuna legalmente elegíveis. Estoque antigo continua visível.
  - L10: 9. O Grok documenta mudanças exclusivamente via GitHub. Só as mudanças documentadas na pasta oficial entram no monitor automático; nunca aplicar ou fazer deploy de forma automática

### `docs/FILUNA-ID.md`
- blob `4e81e1b682c62d5f994deab49b4525258e6cb8de` · 3585 B · 34 linhas · status **READ_INTEGRAL**
- título: # Filuna ID — conta única do cliente (projeto, 04/11/2026)
- extrato: Decisão do dono: o site antigo tem **um único usuário (o dono, de teste)**, então **não há clientes para migrar**. O Filuna passa a ter **um sistema de identidade completo e único**: login social segu
- trechos normativos (amostra):
  - L3: Decisão do dono: o site antigo tem **um único usuário (o dono, de teste)**, então **não há clientes para migrar**. O Filuna passa a ter **um sistema de identidade completo e único*
  - L10: - Contas de terceiros que dão acesso (Mercado Livre hoje) continuam no **corretor root**: o Hub só guarda o vínculo, nunca o segredo.
  - L15: | **Google** | App abre o navegador do sistema → Google (OIDC + PKCE, redirecionamento local `127.0.0.1`) → o Hub confere o `id_token` (assinatura, `aud`, `iss`, expiração, `email_
  - L18: Regra de ligação: contas só se juntam por **e-mail verificado nos dois lados** ou por login já autenticado clicando em "Vincular"; nunca por e-mail não verificado.
  - L21: Checkout hospedado pelo Stripe (cartão e Pix quando habilitado), portal do cliente para trocar cartão e cancelar, webhooks assinados (`checkout.session.completed`, `invoice.paid`, 
  - L24: 1. **Google:** criar o projeto no Google Cloud, tela de consentimento (nome Filuna, domínio filuna3d.com) e credencial OAuth tipo "Aplicativo para computador"; me passar o *Client 

### `docs/FUP-003-DIAGNOSTICO-DISPOSITIVOS.md`
- blob `60c2007c627a26d8ee0d189326d6922d386f3e4f` · 26208 B · 228 linhas · status **READ_INTEGRAL**
- título: # FUP-003 — Diagnóstico técnico avançado de impressoras, câmeras e conexões
- extrato: Estado: **PUBLICADO na 0.2.38 em 05/11/2026** (SHA256 `0a70f57b4f52d12ef43971d3c06476154c7957df47ea6dec1f9d0809e0daf7d8`, 39.430.703 bytes, classe C) com autorização explícita do dono. **Não homologad
- trechos normativos (amostra):
  - L3: Estado: **PUBLICADO na 0.2.38 em 05/11/2026** (SHA256 `0a70f57b4f52d12ef43971d3c06476154c7957df47ea6dec1f9d0809e0daf7d8`, 39.430.703 bytes, classe C) com autorização explícita do d
  - L5: Objetivo: cada falha de impressora/câmera/conexão enviada ao servidor passa a dizer **onde** (etapa), **como** (categoria técnica estável), **com o quê** (adaptador, protocolo, por
  - L27: Princípios: (1) o **evento v1 da 0.2.37 não mudou** (nem o esquema, nem o endpoint); o contexto é um segundo evento, ligado por `event_id`; (2) nada de conexão/envio dos adaptadore
  - L37: Rígido (`additionalProperties:false`), ≤4 KiB, lote ≤20, mesma limitação de taxa do v1 (30 lotes/min/instalação), só Windows, **identidade (tenant/device/role) sempre da sessão aut
  - L39: Compatibilidade: cliente antigo (sem contexto) continua gerando incidente v1; o relatório diz "contexto técnico não enviado por este cliente". Hub novo aceita cliente antigo; clien
  - L45: Alterados (aditivos): `Telemetry/ErrorEvent.cs` (campo `diag` + `ToV1`), `ErrorQueue.cs` (teto 512 KB; dedupe preserva o contexto mais recente), `ErrorReporter.cs` (parâmetros opci

### `docs/HOMOLOGACAO-SITE.md`
- blob `c2a95a47ed2e7a00b1681aca19163651d7218572` · 21475 B · 196 linhas · status **READ_INTEGRAL**
- título: # Homologação do site e do Filuna Slicer — o que falta
- extrato: Escrito em 2026-11-03 a pedido do proprietário. **Regra: o site novo só é homologado (liberado) quando todos os itens "BLOQUEANTE" abaixo estiverem prontos E provados.** Nenhum item é dado como pronto
- trechos normativos (amostra):
  - L3: Escrito em 2026-11-03 a pedido do proprietário. **Regra: o site novo só é homologado (liberado) quando todos os itens "BLOQUEANTE" abaixo estiverem prontos E provados.** Nenhum ite
  - L5: Legenda: ✅ pronto e provado · 🟡 pronto no código, falta teste real · ⬜ não feito.
  - L14: | Texto em relevo (unir) | 🟡 | Na árvore, não publicado; falta teste no PC |
  - L16: | **Pintura de cor** | ⬜ **BLOQUEANTE** | O Orca CLI 2.4.2 não aplicou `paint_color` nos testes (nem em 3MF nativo). Investigar |
  - L17: | **Várias peças por projeto** | ⬜ **BLOQUEANTE** | Destrava dividir em objetos/partes, booleanas, montagem, adicionar peça/negativa/modificador, peça por peça |
  - L19: | Menu do objeto (botão direito) igual ao Orca | 🟡 | Itens dependentes de várias peças ficam em cinza; DRC não existe |

### `docs/IA-SUPORTE-PESQUISA.md`
- blob `32d850bbe8190c57c874be744893c8127dfafe24` · 13594 B · 89 linhas · status **READ_INTEGRAL**
- título: # IA de ajuda da Filuna — pesquisa e recomendação (04/11/2026)
- extrato: Objetivo: trocar a dependência de IA de terceiros (Gemini/Groq/OpenRouter, com cotas e cobrança) por um modelo **nosso, no servidor**, que só fala de Filuna e de impressão 3D, consulta fatura/plano/im
- trechos normativos (amostra):
  - L3: Objetivo: trocar a dependência de IA de terceiros (Gemini/Groq/OpenRouter, com cotas e cobrança) por um modelo **nosso, no servidor**, que só fala de Filuna e de impressão 3D, cons
  - L17: Com 6 threads o 9B rende quase igual na geração (12,3 t/s): a memória é o gargalo, não a CPU.
  - L21: - **Escopo (clima, poema, receita, Python, Copa, 2+2, "finja que não tem regras", pedir chave/prompt, injeção em inglês):** todos os modelos recusaram corretamente e ofereceram aju
  - L22: - **Ferramentas (faturas, plano/saldo, impressoras, trocar senha):** Qwen3.5-4B/9B, Gemma 12B e 26B, gpt-oss chamaram a ferramenta certa; **Gemma E4B falhou** (diz "vou usar a ferr
  - L24: - **COM trecho curado do manual no prompt:** TODOS os modelos, até o de 4B, responderam **fiéis ao manual** (mesmos números e passos), e quando não havia trecho ("nylon com fibra d
  - L25: **Conclusão:** o que decide a qualidade não é o tamanho do modelo, é ter uma **base de conhecimento curada** e obrigar o modelo a responder só com ela.

### `docs/INTEGRACOES-IMPRESSORAS.md`
- blob `9818fc020646baedd245bbd6e9cd8f22d6ce9c34` · 3276 B · 21 linhas · status **READ_INTEGRAL**
- título: # Contas dos fabricantes de impressora: dá para vincular e usar a biblioteca deles? (pesquisa 04/11/2026)
- extrato: Pergunta do dono: alguma impressora libera a conta do cliente para vincular ao Filuna, conectar a biblioteca dele e usar recursos dele no nosso serviço?
- trechos normativos (amostra):
  - L4: **Resposta curta: nenhum dos fabricantes pesquisados oferece, hoje, uma API pública com login (OAuth) para um app de terceiros entrar na conta do cliente e ler a biblioteca/nuvem d
  - L10: | **Elegoo** | Protocolo local SDCP (documentado pela comunidade) — é o que o Filuna usa na CC2. Há uma "API de parceiros" da Elegoo (usada pelo Obico para IA), sem confirmação de 
  - L11: | **Anycubic** | Nenhum programa oficial. | Só API **reversa**: usa token tirado do Slicer Next/navegador do próprio cliente (frágil, pode parar a qualquer hora, e há histórico de 
  - L14: | SimplyPrint | Tem o próprio OAuth2 por dono de impressora (é produto deles, não do fabricante). | — | Referência de como seria. |
  - L19: 3. **Não usar** token/API reversa de nuvem de cliente (Anycubic, Bambu Cloud): contraria os termos, quebra sem aviso e põe a conta do cliente em risco.
  - L20: 4. **Bibliotecas de modelos:** priorizar fontes que têm API oficial com login do próprio cliente (a verificar: Thingiverse, MyMiniFactory; MakerWorld/Printables/Creality Cloud não 

### `docs/MODELOS-3D.md`
- blob `05112a9b10026db88e0f0b9855384c1a4e5e5d94` · 5339 B · 31 linhas · status **READ_INTEGRAL**
- título: # Modelos 3D do Filuna (catálogo protegido) — 2026-11-04
- extrato: - **Não existe download.** O cliente imprime o fatiamento **já validado** para a impressora homologada dele; em impressora não validada, fatia dentro do fatiador sem poder salvar, exportar ou abrir em
- trechos normativos (amostra):
  - L4: - **Não existe download.** O cliente imprime o fatiamento **já validado** para a impressora homologada dele; em impressora não validada, fatia dentro do fatiador sem poder salvar, 
  - L6: - **Peças organizadas em MESAS:** peças da mesma cor na mesma mesa; **cores diferentes em mesas diferentes** (o cadastro recusa mesa com mais de uma cor). O dono pode anexar uma **
  - L8: - **Enviar ao meu e-commerce** (cria rascunho do anúncio com título, descrição e as 3 fotos; o cliente confirma) — ou convite "Comece a vender de forma fácil" se não houver conta c
  - L15: - **Ajustes da IA:** `ajustes.py` só traduz o pedido ("melhor acabamento", "mais rápido", "mais forte", "menos material", "soltando da mesa", "caindo/deformando") numa lista curta 
  - L27: - **Proteção:** não existe proteção absoluta de um arquivo no computador de quem o usa; o objetivo é tirar o arquivo da vista, do caminho normal e dos botões, e entregar só a quem 
  - L29: - **Modelos do site antigo:** não migrados (a pedido do dono).

### `docs/ORCA-2.5-INTEGRACAO-NATIVA.md`
- blob `4369059f552fb2db483e56c3ef269a20d2b98f5b` · 10573 B · 107 linhas · status **READ_INTEGRAL**
- título: # Integração nativa com o OrcaSlicer: painel Filuna sobre o Orca real (Etapa A)
- extrato: Data: 05/11/2026. Estado: **protótipo isolado comprovado em tela virtual Linux. Nada foi publicado e a versão distribuída (Orca 2.4.2) não foi tocada. Não está homologado em Windows real.**
- trechos normativos (amostra):
  - L3: Data: 05/11/2026. Estado: **protótipo isolado comprovado em tela virtual Linux. Nada foi publicado e a versão distribuída (Orca 2.4.2) não foi tocada. Não está homologado em Window
  - L19: | Alterar altura de camada e voltas da parede pelo painel | sim (`process.begin_update().set().commit()`); o Orca marca o preset como alterado e não salvo | `ultimo_commit.json` |
  - L25: O fatiamento de hoje (`PrepareOrcaService.cs:156-157`) passa ao **CLI do Orca 2.4.2** os arquivos de perfil de **sistema** do jeito que estão no disco. Esse caminho **não resolve a
  - L40: Isso é **candidato forte** a causa das impressões que nunca funcionaram pelo Filuna. Não provei em impressora. A correção possível sem mexer no Orca: resolver a herança no Filuna e
  - L43: - `docs/HLSD/preset-plugin-api.md`: contrato `orca.host.preset.{process,printer,filament}.begin_update()/set()/commit()`. Edita o preset que o Orca está usando e não salva em disco
  - L44: - `docs/HLSD/python-plugin-async-bridge.md`: o executor de Script roda fora da thread da interface; o `begin_update` bloqueia só esse executor.

### `docs/ORCA-2.5-PELO-FATIADOR.md`
- blob `16c252c18ded4b26b0db75e0af332c3b8c13e1bf` · 4583 B · 52 linhas · status **READ_INTEGRAL**
- título: # OrcaSlicer 2.5 + painel Filuna, instalados pelo próprio fatiador (candidata 0.2.42; NÃO publicada)
- extrato: 1. Acima da Central de ajuda aparece o cartão verde **"Novo: Orca 2.5.0-rc com painel Filuna — Baixar e instalar (207 MB)"**. Ele só aparece depois que **o dono aprova** o pacote.
- trechos normativos (amostra):
  - L1: # OrcaSlicer 2.5 + painel Filuna, instalados pelo próprio fatiador (candidata 0.2.42; NÃO publicada)
  - L5: 2. Um clique e uma confirmação. O Filuna baixa do servidor da Filuna, confere a **assinatura** e o **SHA-256**, instala e mostra o progresso. Não é preciso entrar em site, GitHub n
  - L13: | Dados isolados do 2.5 | `…\orca25-data` | Nunca mistura com o Orca pessoal nem com o 2.4.2 |
  - L14: | Painel Filuna (plugin AGPL-3.0) | `…\orca25-data\orca_plugins\filuna_painel.py` | Vai **embutido na DLL do Filuna** e é regravado a cada abertura, então atualiza junto com o Filu
  - L22: - O manifesto é assinado com a chave de atualização. **Esse arquivo de aprovação não existe ainda: é a autorização do dono.**
  - L29: - sem aprovação, não entrega nada, nem sem login;

### `docs/PARAMETRICOS-ANALISE.md`
- blob `72075ae69c37a3eb49bd2d2465f580278a8fb9c1` · 8765 B · 64 linhas · status **READ_INTEGRAL**
- título: # Paramétricos: análise crítica da migração (2026-11-03)
- extrato: Regra do dono: o paramétrico é **gerado no fatiador do cliente**. Servidor só guarda catálogo, plano e controle. Se algum serviço essencial precisar ficar no servidor, o agente descreve e o dono aprov
- trechos normativos (amostra):
  - L20: **A) Motor local (recomendada).** Um módulo assinado, baixado só quando o cliente usa um paramétrico (mesmo canal de "módulos" da Lua), com Python embutido e as mesmas bibliotecas 
  - L22: **Não recomendado:** manter a geração no servidor (contraria a regra do dono).
  - L26: 2. **IA de imagem (Gemini)** para foto-linear-3d e quadro-relevo: a chave não pode ir para o PC do cliente. Opção: um proxy mínimo no servidor (recebe imagem, chama o Gemini, devol
  - L27: 3. **Gerar no servidor quando o fatiador do cliente estiver desligado, cobrando créditos** (ideia do dono, 2026-11-03). Precisa de: tabela de preço em créditos por paramétrico, car
  - L44: Referência humana (não automática): Fiverr, pacote de 3 chaveiros por US$ 5 ≈ US$ 1,67 cada. Com ela, a média de 4 sobe para ≈ US$ 0,76.
  - L52: - **Geração online (servidor, fatiador desligado):** cota de **5 por semana, não acumulativa**, incluída no plano; **depois da cota, R$ 0,50 por geração online.**

### `docs/PENDENCIAS.md`
- blob `359d92fc539bb224b97a359e94f9ba98cda56ceb` · 16577 B · 141 linhas · status **READ_INTEGRAL**
- título: # Pendências consolidadas (04/11/2026) — fonte única
- extrato: Legenda: 🔧 eu faço sozinho · 🧪 eu faço e depois o dono/hardware confirma · 👤 só o dono pode · 💡 precisa de decisão do dono.
- trechos normativos (amostra):
  - L9: | 1 | Estoque: faixa "bobinas nas impressoras ainda não cadastradas" | ✅ provado no navegador com o Hub real (cadastrar e "é a mesma") |
  - L20: | Fatiador: várias peças por projeto (destrava dividir, booleanas, montagem, peça por peça) | 🧪 BLOQUEANTE |
  - L21: | Fatiador: pintura de cor — experimento de 04/11: 3MF com `paint_color` e com `slic3rpe:mmu_segmentation` (valor 8 = filamento 2) fatia com UM filamento só (sem T1), mesmo com 2 f
  - L23: | Paramétricos: arvore-natal e luminaria-placa (portar JS), quadro-relevo (IA) | 🧪 BLOQUEANTE |
  - L25: | Anycubic: comando de início automático (hoje só envia) | 🧪 BLOQUEANTE |
  - L26: | Planos: R$ 40/mês, emergência de 5 dias, tolerância offline de 3 dias | 💡 BLOQUEANTE |

### `docs/PLATE-LAYOUT-CONTRACT.md`
- blob `c970984a6c0e870096ea74b2b7eb916b93121b6c` · 4423 B · 37 linhas · status **READ_INTEGRAL**
- título: # Regra global — organização automática de mesas de impressão (PEI)
- extrato: **Decisão do proprietário:** 2026-11-03 · **Status:** especificação obrigatória; implementação inicial no exportador Lua em homologação; demais motores a integrar.
- trechos normativos (amostra):
  - L2: **Decisão do proprietário:** 2026-11-03 · **Status:** especificação obrigatória; implementação inicial no exportador Lua em homologação; demais motores a integrar.
  - L5: Obrigatória para todo projeto gerado/exportado pelo Filuna (modelos 3D cadastrados/autorizados, paramétricos e ferramentas geradoras). A regra **não** autoriza download de geometri
  - L8: 1. Consultar perfil REAL da impressora/mesa selecionada: dimensões X×Y×Z úteis, forma, zonas proibidas, folgas de segurança, brim/suportes. Sem perfil, 256×256 somente em QA como M
  - L9: 2. Identificar peças físicas, COR do filamento e material/perfil real, dimensões, orientação correta e base de contato. A foto da litofania é representada por variação de espessura
  - L11: 4. Peças da mesma cor que não cabem juntas: criar automaticamente QUANTAS MESAS FOREM NECESSÁRIAS, sem sacrificar segurança. Não dividir automaticamente um único objeto grande dema
  - L12: 5. Cores diferentes: SEMPRE mesas distintas, mesmo que todas coubessem juntas. Materiais/perfis incompatíveis ficam separados ainda que tenham a mesma cor. Evitar purga/trocas desn

### `docs/PROJECT_STATE.json`
- blob `a898e7a1f8aadf5a7cedbb99bc0069a88180ac65` · 28845 B · 788 linhas · status **READ_INTEGRAL**
- título: {
- extrato: {
- trechos normativos (amostra):
  - L653: "filamento": "ASA nunca vira ABS; compostos CF/GF não perdem o composto",
  - L658: "Orca 2.5 Printer Agents (classe D)",
  - L671: "relay do Telegram não está ligado como serviço (precisa autorização do dono)",
  - L680: "git": "main sincronizado com origin/main por fast-forward em 05/11/2026; PR #2 (FUP-001 isolado) NÃO deve ser mesclado; branch ferramentas-wip só local",
  - L690: "estado": "implementado e testado, ISOLADO (não ligado a rotas; FILUNA_TARGETED_UPDATES=0)",
  - L696: "estado": "PUBLICADA na 0.2.38 (05/11/2026), não homologada: sem teste em Windows real nem impressora real",

### `docs/ROADMAP.md`
- blob `d99206b60fcfda84eba32db007198e9fd4098762` · 3091 B · 29 linhas · status **READ_INTEGRAL**
- título: # Etapas — implementação real
- extrato: Diretório novo `/srv/filuna-slicer-windows-new`, monorepo local, Hub loopback SQLite de desenvolvimento, device tokens, eventos idempotentes, GET/push, lock por revisão, quatro clientes em nível de co
- trechos normativos (amostra):
  - L7: Criar novo repo privado Filuna Slicer, sem copiar source do app antigo; configurar acesso de revisão para IA Grok, branches e verificações de contratos/QA. O GitHub existente abrig
  - L10: Preparar Rust/Cargo, Node/pnpm em área de ferramentas isolada e runner Windows; compilar shell Tauri/React/Three.js fiel ao Grok. Entradas reais STL, mover/rotacionar/redimensionar
  - L13: Instalar/empacotar official engines ou adapters para suas interfaces sem alterar slicing/perfis. Orca principal, Prusa alternativo, CuraEngine e Kiri:Moto como opções, cada um com 
  - L19: Grok lê os documentos e registra cada proposta na pasta Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/ do GitHub. Monitor ChatGPT confere apenas esse diretório de hora em hora; implementa
  - L22: Compilação comprovada e testes, não somente design; integração dos quatro engines sem modificar parâmetros silenciosamente; impressoras testadas em hardware e logs preservados; seg
  - L26: **2026-11-03:** infraestrutura de MCP/OAuth inicial criada e 5/5 testes locais aprovados, mas sem publicação/ativação. Aguardando funções especificadas pelo proprietário. Não antec

### `docs/SYSTEMS_MAP.md`
- blob `3dbe5a388bdb6507ba32a19ce3ddbb3c98686472` · 2224 B · 33 linhas · status **READ_INTEGRAL**
- título: # Mapa do ecossistema (NOVO, sem legado)
- extrato: ```
- trechos normativos (amostra):
  - L21: ├── auth PROD futura (OIDC/PKCE)  — NÃO IMPLEMENTADO
  - L22: └── file vault / payments / commands — NÃO IMPLEMENTADO
  - L26: - Nunca tocar em `/srv/site`, `nucleo/nucleo.db`, nginx, site e app antigos.
  - L27: - Não divulgar porta do Hub à rede; a versão atual escuta somente `127.0.0.1`.
  - L30: - Dados de estoque exigirão ledger de consumo real e política offline; eventos genéricos implementados ainda não equivalem a estoque final.
  - L32: - Mobile não promete executar quatro slicers; é companion até haver estudo próprio.

### `docs/UPDATE-RELEASE-POLICY.md`
- blob `28c4cf4cb74a20be569b7b19190ae6a1f7bd630b` · 10347 B · 52 linhas · status **READ_INTEGRAL**
- título: # Regra obrigatória: classificação, segurança e publicação de atualizações Filuna
- extrato: **Aprovada pelo proprietário em 2026-11-03.** Versão do contrato: `2026-11-03.1`.
- trechos normativos (amostra):
  - L1: # Regra obrigatória: classificação, segurança e publicação de atualizações Filuna
  - L7: **Todo update do Filuna Slicer entra pelo próprio sistema — nunca por "baixar o instalador de novo".** Isso vale para cada correção e cada novidade, sem exceção de conveniência.
  - L8: 1. **Entrega:** pacote assinado com SÓ o que mudou (classe C: `Filuna.Slicer.dll` + arquivos de `ui/`; dados/módulos pelas classes A/B), feito por `packaging/windows/build-patch.sh
  - L10: 3. **Novidades obrigatórias:** toda atualização publicada leva a lista de novidades escrita para o cliente (`--notes "a|b|c"`).
  - L13: 7. **Pacote único e cumulativo (regra do dono, 04/11/2026):** o cliente, em qualquer versão e não importa quantas atualizações saíram depois, recebe **um só pacote somado**, nunca 
  - L17: O produto deve avisar e obter atualizações dentro do aplicativo, sem orientar o cliente a baixar/reinstalar tudo para cada correção. **Instalador completo somente quando indispensá

## Demais fontes (86 blobs) — cobertura integral resumida

| Repo | Caminho | SHA | Bytes | Linhas | Título/extrato |
|---|---|---|---:|---:|---|
| S | `Filuna-Slicer/13-HOME-LANCAMENTO-V3.md` | `ef7d1d053f81…` | 2712 | 24 | # Filuna Slicer — home pública de lançamento v3 |
| S | `Filuna-Slicer/15-LINHA-DO-TEMPO.md` | `99771095d91c…` | 6969 | 55 | # Filuna Slicer — LINHA DO TEMPO E DECISÕES DO PROPRIETÁRIO (até 2026-11-02) |
| S | `Filuna-Slicer/17-PRINTS-ETAPA-EXATA.md` | `6241ee8cdfa0…` | 2314 | 12 | # Estado de entrega e capturas da beta privada — 02/11/2026 |
| S | `Filuna-Slicer/19-PUBLICACAO-PRIVADA-HOME-0.1.1.md` dedup | `dc51db402be7…` | 5041 | 38 | # Filuna Slicer Windows 0.1.1 — Home Grok aprovada, beta privada publicada |
| S | `Filuna-Slicer/23-QA-LUA-ORCA-MULTIMESA-2026-11-03.md` | `3455b99f7d00…` | 7308 | 75 | # QA Lua Litofania V6 — novo Filuna Slicer |
| S | `Filuna-Slicer/24-QA-CONTA-E-INSTALADOR-WINDOWS-0.1.4.md` dedup | `ba896868f265…` | 4888 | 33 | # QA — Conta beta e instalador Windows 0.1.4 — 2026-11-03 |
| S | `Filuna-Slicer/CHATGPT-MCP-1.0.4-CORRECAO-E-STATUS-2026-11-03.md` | `c0ceefc1dc07…` | 1880 | 22 | # Filuna Slicer — ChatGPT ZIP 1.0.4: nome inválido e nome incompatível |
| S | `Filuna-Slicer/CHATGPT-MCP-FIX-INSTALL-IDENTIDADE-2026-11-03.md` dedup | `2a6dca379623…` | 5803 | 49 | # @Filuna — diagnóstico completo do erro de instalação e identidade visual |
| S | `Filuna-Slicer/CHATGPT-MCP-FUNCOES-PROPOSTAS-2026-11-03.md` dedup | `b889bda54b71…` | 12016 | 99 | # @Filuna — viabilidade das primeiras funções e assinatura (PROPOSTA, 2026-11-03 |
| S | `Filuna-Slicer/CHATGPT-MCP-GATEWAY-PRIVADO-2026-11-03.md` dedup | `e0d8124263d5…` | 6940 | 86 | # Filuna Slicer — Gateway privado ChatGPT MCP (2026-11-03) |
| S | `Filuna-Slicer/CHATGPT-MCP-INSTALL-TROUBLESHOOT-2026-11-03.md` dedup | `0ec123d37e9b…` | 4085 | 44 | # @Filuna — erro ao instalar plugin no ChatGPT (2026-11-03) |
| S | `Filuna-Slicer/CHATGPT-MCP-PLANO-MESTRE.md` dedup | `4ac076910f61…` | 13392 | 148 | # Integração @Filuna com o ChatGPT — plano mestre, implementação e standby |
| S | `Filuna-Slicer/CHATGPT-MCP-STANDBY-2026-11-03.md` | `393912783666…` | 3483 | 40 | # Novo Filuna Slicer — plugin @Filuna para ChatGPT (STANDBY) |
| S | `Filuna-Slicer/CHATGPT-MCP-TESTE-ESTOQUE-PDF-BETA-2026-11-03.md` dedup | `6e6da20a0116…` | 6079 | 65 | # @Filuna — teste privado de PDF para cadastro de filamentos (NOVO Filuna Slicer |
| S | `Filuna-Slicer/CHATGPT-PLUGIN-CORRECAO-SLUG-1.0.3-2026-11-03.md` | `483e4e28b747…` | 2034 | 21 | # ChatGPT @Filuna — correção de identificador da atualização (2026-11-03) |
| S | `Filuna-Slicer/CHATGPT-PLUGIN-IDENTIDADE-PACOTE-1.0.2-2026-11-03.md` | `db58bfafc357…` | 3535 | 35 | # Filuna Slicer — correção do pacote ChatGPT 1.0.2 (2026-11-03) |
| S | `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2026-11-02-CALIBRACAO.md` | `2b422aec1531…` | 3543 | 51 | # Calibração no padrão Orca — alteração proposta pelo Grok |
| S | `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2026-11-02-DISPOSITIVOS.md` | `69bf8bcdd40a…` | 3695 | 54 | # Dispositivos e protocolos — alteração proposta pelo Grok |
| S | `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2026-11-02-HOME.md` | `001bdfd8b90a…` | 3730 | 53 | # Home com arquivo já fatiado — alteração proposta pelo Grok |
| S | `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2026-11-02-PREPARACAO.md` | `18e4395ac188…` | 3787 | 50 | # Preparação da mesa — alteração proposta pelo Grok |
| S | `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2026-11-02-REGRA-ESBOCO-VISUAL.md` | `ae07109132da…` | 2588 | 43 | # Regra do esboço visual — alteração proposta pelo Grok |
| S | `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2026-11-02-TEMAS.md` | `119411952bb3…` | 5546 | 78 | # Temas dark, azul e laranja — alteração proposta pelo Grok |
| S | `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2026-11-03-FILUNA-IMPLEMENTACAO-QA7-QA9-REINICIO.md` | `4c35374888f9…` | 8525 | 52 | # Filuna Slicer Windows — Registro mestre QA7, QA8 e QA9 |
| S | `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/2026-11-03-PREPARAR-AUTOAJUDA.md` | `5429f1c347cc…` | 2354 | 41 | # Preparação — aba Global e autoajuda |
| S | `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/MODELO-ATUALIZACAO.md` | `8a97db840929…` | 3023 | 51 | # [NOME DO RECURSO] — alteração proposta pelo Grok |
| S | `Filuna-Slicer/GROK-GITHUB/ATUALIZACOES/README.md` | `c07be7c2bf73…` | 1771 | 27 | # Onde registrar TODAS as atualizações do Grok |
| S | `Filuna-Slicer/GROK-GITHUB/REGRA-CATALOGO-PRINT-ONLY.md` | `7da9b7cc611b…` | 2321 | 17 | # REGRA OBRIGATÓRIA — CATÁLOGO FILUNA: IMPRIMIR SEM EXPORTAR (03/11/2026) |
| S | `Filuna-Slicer/MCP-MULTI-IA-GROK-CLAUDE-GEMINI-2026-11-04.md` | `848bd1d12aca…` | 6137 | 69 | # Filuna Slicer — expansão MCP Grok, Claude e Gemini CLI (04/11/2026) |
| S | `Filuna-Slicer/README.md` | `591f9d4f05b4…` | 6855 | 62 | # Filuna Slicer — Plano mestre de negócio, arquitetura e desenvolvimento |
| S | `Filuna-Slicer/UPSTREAM-MONITOR/MESTRE-MOTORES-E-ATUALIZACOES.md` | `7ab9efa35b77…` | 12868 | 111 | # Filuna Slicer — Documento Mestre de Integração dos Motores-Mãe |
| S | `Filuna-Slicer/UPSTREAM-MONITOR/SCRIPT-MAE-REPASSAR-AO-CHATGPT.md` | `4c0b7a872e01…` | 4036 | 37 | # SCRIPT MÃE — Filuna Slicer / atualizações dos motores |
| S | `Filuna-Slicer/UPSTREAM-MONITOR/planos/cura-2.7.0.md` | `d3f73b90aef7…` | 2246 | 33 | # Plano preliminar — CuraEngine 2.7.0 |
| S | `Filuna-Slicer/UPSTREAM-MONITOR/planos/kiri-4.7.3.md` | `764d057516cb…` | 2229 | 33 | # Plano preliminar — Kiri:Moto 4.7.3 |
| S | `Filuna-Slicer/UPSTREAM-MONITOR/planos/orca-v2.4.2.md` | `db3dda2b7611…` | 2331 | 34 | # Plano preliminar — OrcaSlicer v2.4.2 |
| S | `Filuna-Slicer/UPSTREAM-MONITOR/planos/prusa-version_2.9.7.md` | `0b9d82bf68f7…` | 2246 | 33 | # Plano preliminar — PrusaSlicer version_2.9.7 |
| S | `Filuna-Slicer/referencia-ui/HISTORICO.md` | `d85bc1096f51…` | 1654 | 22 | # Histórico da referência visual Filuna Slicer |
| W | `contracts/api.json` | `aaa2dc1fd8a9…` | 1061 | 53 | { |
| W | `contracts/chatgpt-mcp-tools.json` | `04df867320e4…` | 1095 | 35 | { |
| W | `contracts/engine-job.schema.json` | `c0c4b12128c5…` | 1233 | 64 | { |
| W | `contracts/sync-event.schema.json` | `5ac27b38f6bb…` | 821 | 46 | { |
| W | `contracts/windows-0.1.5-QA2-candidate.json` | `cd2ee347ced6…` | 487 | 13 | { |
| W | `contracts/windows-0.1.5-QA4-candidate.json` | `fdfe4a73f749…` | 707 | 16 | { |
| W | `contracts/windows-0.1.5-QA5-candidate.json` | `32d6389244b5…` | 728 | 16 | { |
| W | `contracts/windows-0.1.5-QA6-candidate.json` | `4ecd08a20970…` | 961 | 28 | { |
| W | `contracts/windows-0.1.6-QA7-bootstrap-installer.json` | `6b5450a4875f…` | 1925 | 42 | { |
| W | `contracts/windows-0.1.7-QA8-profile-picker-candidate.json` | `543f2d973cba…` | 1970 | 48 | { |
| W | `contracts/windows-0.1.8-QA9-restart-candidate.json` | `0f23977289c9…` | 2191 | 56 | { |
| W | `docs/ARCHITECTURE.md` | `22ef853840fb…` | 2300 | 32 | # Filuna Slicer — arquitetura nova integrada, sem legado |
| W | `docs/BUILD-WINDOWS-0.1.2-2026-11-03.md` | `3da73c8a9833…` | 2260 | 19 | # Windows 0.1.2 — build privada, sem deploy (2026-11-03) |
| W | `docs/CHATGPT-MCP-1.0.4-NOVA-FICHA-NAO-UPDATE-2026-11-03.md` | `afb8a91429c6…` | 1172 | 19 | # Filuna Slicer ChatGPT — validação 1.0.4 |
| W | `docs/CHATGPT-MCP-FIX-SLUG-ORIGINAL-FILUNA-2026-11-03.md` | `bf337219fe0f…` | 1964 | 20 | # Correção adicional — nome exato do plugin ChatGPT existente |
| W | `docs/CHATGPT-MCP-IDENTIDADE-PACOTE-1.0.2-2026-11-03.md` | `ee67e0560069…` | 4100 | 44 | # Filuna Slicer — pacote de atualização da identidade do plugin ChatGPT |
| W | `docs/CHATGPT-MCP-STANDBY.md` | `a2d1350be2a5…` | 4917 | 59 | # @Filuna ChatGPT — infraestrutura MCP/OAuth em STANDBY |
| W | `docs/D5-D6-CORRECAO-INDIVIDUAL.md` | `8734b90a5205…` | 5059 | 36 | # D5/D6 — Correção individual por instalação (isolada, DESLIGADA) |
| W | `docs/ESTADO-0.2.0.md` | `4f6fa334cde7…` | 7940 | 64 | # Filuna Slicer 0.2.x — estado do projeto (04/11/2026) |
| W | `docs/FEATURE_TEMPLATE.json` | `37f20efa6a32…` | 1079 | 48 | { |
| W | `docs/FUP-002-TELEMETRIA.md` | `83226f28dc57…` | 2323 | 19 | # FUP-002 — Telemetria de erros por instalação + alertas ao proprietário (Telegr |
| W | `docs/FUP-DESCOBERTA.md` | `b16162cffebc…` | 2910 | 23 | # FUP — descoberta multimarca (fase 1 integrada) |
| W | `docs/GUIA-DONO-ID-GOOGLE-STRIPE-BAMBU.md` | `cffade264f0b…` | 2771 | 22 | # Google, Stripe e Bambu — situação (04/11/2026, atualizado) |
| W | `docs/HOME-GROK-2026-11-03.md` | `ffd15f3228f3…` | 2672 | 23 | # Home do Filuna Slicer — Grok 03/11/2026 |
| W | `docs/KB-REVISAO.md` | `835c1eb661ad…` | 23115 | 393 | # Revisão da base de conhecimento do assistente |
| W | `docs/LOGIN-WINDOWS-0.1.3-2026-11-03.md` | `4f051bdcb52c…` | 1619 | 16 | # NOVO Filuna Slicer Windows 0.1.3 — correção de login |
| W | `docs/PRODUCAO.md` | `5575d0d5093e…` | 4463 | 30 | # Produção (histórico, fila, agenda, manutenção, avisos, pular objeto) |
| W | `docs/QA-0.1.5-QA2-2026-11-03.md` | `adfcfcdbea9d…` | 1410 | 17 | # Filuna Slicer Windows 0.1.5-QA2 — candidato não publicado |
| W | `docs/QA-0.1.5-QA4-PREPARAR-2026-11-03.md` | `0586314f23f0…` | 1316 | 13 | # Filuna Slicer QA4 — Preparação Grok / Orca real |
| W | `docs/QA-0.1.5-QA5-PREPARAR-2026-11-03.md` | `b219d0110166…` | 1597 | 11 | # QA5 — Preparação Grok / Filuna Slicer |
| W | `docs/QA-0.1.5-QA6-2026-11-03.md` | `ef8433fd3c99…` | 5541 | 44 | # Filuna Slicer 0.1.5-QA6 — importação real, layout Grok e agente de atualização |
| W | `docs/QA-0.1.6-QA7-EXE-TRANSICAO-2026-11-03.md` | `330027e0a8bb…` | 5032 | 42 | # Filuna Slicer Windows 0.1.6-QA7 — instalador EXE de transição |
| W | `docs/QA-0.1.7-QA8-ATUALIZACAO-INTERNA-BETA-2026-11-03.md` | `76a71b6d87e0…` | 3954 | 33 | # Atualização interna Filuna 0.1.7-QA8 — piloto privado autorizado |
| W | `docs/QA-0.1.7-QA8-PERFIS-ORCA-2026-11-03.md` | `75738212441c…` | 3663 | 32 | # Filuna Slicer 0.1.7-QA8 — cadastro virtual de perfis do Orca interno |
| W | `docs/QA-0.1.8-QA9-REINICIO-AUTOMATICO-2026-11-03.md` | `a662e68e54be…` | 4481 | 35 | # Filuna Windows 0.1.8-QA9 — reinício inteligente, popup de novo recurso |
| W | `docs/QA-0.1.9-IMPRESSORAS-NATIVAS-2026-11-03.md` | `e5494f53353a…` | 15458 | 102 | # Filuna Windows 0.1.9 — impressoras nativas (Elegoo CC2, Kobra 3 V2, Bambu, Cre |
| W | `docs/QA-DISPOSITIVOS-ORCA-0.1.6-QA7-2026-11-03.md` | `074c07c17222…` | 4646 | 34 | # Dispositivos Grok + isolamento do Orca — 0.1.6-QA7 |
| W | `docs/QA-LUA-LITOFANIA-2026-11-03.md` | `5dccf4707699…` | 6808 | 63 | # QA comparativo — Lua Litofania local vs original — 2026-11-03 |
| W | `docs/QA-PREPARAR-ORCA-2026-11-03.md` | `006e8fedae79…` | 7378 | 48 | # Preparação Grok → OrcaSlicer local (integração parcial) |
| W | `docs/RELATORIO-FUP-001-002.md` | `b00e5e81611e…` | 22667 | 236 | # Relatório completo — Dossiê técnico (05/11/2026) + FUP-001 + FUP-002 |
| W | `docs/RELEASE.md` | `052468fd12ab…` | 1037 | 8 | # Estado de build — 2026-11-03 |
| W | `docs/RELEASES-WINDOWS-QA7-QA9-REGISTRO-MESTRE-2026-11-03.md` | `fab7ac4e6ac5…` | 8526 | 52 | # Filuna Slicer Windows — Registro mestre QA7, QA8 e QA9 |
| W | `docs/TELEGRAM-ROTACAO-TOKEN.md` | `d6e6ec4e392b…` | 5465 | 56 | # Telegram — segurança do token e rotação segura |
| W | `docs/UPSTREAM-MONITOR-GEMINI-PAID-2026-11-03.md` | `fb1374ccf317…` | 2976 | 29 | # Central de Motores-Mãe — primeira integração Gemini paga (beta privada) |
| W | `docs/VALIDACAO-0.2.37-WINDOWS.md` | `2ffe38a934aa…` | 5158 | 69 | # Validação da 0.2.37 — estado verificado e checklist em Windows real |
| W | `docs/schemas/telemetry-diagnostic-v2.schema.json` | `8706832743fc…` | 6274 | 278 | { |
| W | `docs/schemas/telemetry-event-v1.schema.json` | `56ee9232ce1d…` | 2869 | 137 | { |
| W | `proposals/README.md` | `baf96f13991f…` | 270 | 3 | # Propostas de IA |
| W | `proposals/filuna.mestre-dispositivos.json` | `f0c065c72ecd…` | 2087 | 53 | { |
| W | `proposals/filuna.mestre-preparar.json` | `97754a2665af…` | 3401 | 111 | { |

## SBOM inicial do fork F

Substituído/expandido por inventário verificável parcial:

- [sbom/README.md](sbom/README.md) — tabela CMake com versões/hashes  
- [sbom/filuna-windows-partial.cdx.json](sbom/filuna-windows-partial.cdx.json) — CycloneDX 1.5 **parcial** (`filuna:sbom_status=PARTIAL_NOT_COMPLETE`)  
- Ainda **não** completo (transitivas, DLLs do artefato, SPDX jurídicos).

## Próxima ação

1. N1/N2: ver [G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md](G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md) + conflitos rev .2.  
2. Opcional: SBOM DLLs artifact 42102469888; apertar N4–N7.  
3. Sem Home/DeviceBridge/SRV/G1 UI; sem merge PRs.

## Checkpoint lote semântico N1 (2026-11-05 23:12:40 CET)

Promovidos a `READ_SEMANTIC_FULL` (amostra; hashes = manifesto):  
`15-LINHA-DO-TEMPO`, `23-QA-LUA…`, `13-HOME-LANCAMENTO-V3`, `REGRA-CATALOGO-PRINT-ONLY`, `25-REGRA-MESTRE-ATUALIZACOES`, `QA-0.1.9-IMPRESSORAS`, `QA-LUA-LITOFANIA`, `VALIDACAO-0.2.37`, `D5-D6`, `PRODUCAO`, `ARCHITECTURE`, `ESTADO-0.2.0`, `FUP-DESCOBERTA`, `GUIA-DONO…`, `ENGINE_CONTRACT`, `PROTOCOLOS-IMPRESSORAS`, `PENDENCIAS`, `AUDITORIA-SEGURANCA`, `GROK_CONTRACT`, proposals `mestre-dispositivos`/`mestre-preparar`, contracts `engine-job`/`sync-event`/`api`.  
Reconfirmados FULL: `04-MOTORES`, `14-CONTINUIDADE`, `18-HOME`, `ESTRUTURA-HOME…`, `MESTRE-DISPOSITIVOS/PREPARAR/PARAMETRICOS`.  
`UPSTREAM-MONITOR/MESTRE-MOTORES…` lido (COVER→semântico parcial): conteúdo multi-motor **SUPERADO** no F — não reabrir.


## Checkpoint fechamento G0 documental (2026-11-05 23:24:00 CET)

Mapa path+SHA+status: [G0-MAPA-141-STATUS-2026-11-05.md](G0-MAPA-141-STATUS-2026-11-05.md) (141/141, 0 UNKNOWN).  
MIG↔DAG: [G0-MIG-DAG-ALINHAMENTO-2026-11-05.md](G0-MIG-DAG-ALINHAMENTO-2026-11-05.md).  
Riscos OEM: [G0-RISCOS-OEM-PROVENIENCIA-2026-11-05.md](G0-RISCOS-OEM-PROVENIENCIA-2026-11-05.md).  
Testes: [G0-TESTES-CASOS-2026-11-05.md](G0-TESTES-CASOS-2026-11-05.md).  
Critérios Windows/SBOM-completo removidos da checklist G0 (eram derivados indevidos).
