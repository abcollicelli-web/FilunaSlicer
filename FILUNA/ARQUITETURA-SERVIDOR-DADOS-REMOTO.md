# FILUNA — ARQUITETURA MESTRE DO SERVIDOR, DADOS E CONTROLE REMOTO FUTURO

## Estado mais recente — Fase05.1 privada implementada/testada, NÃO ativada

08/11/2026 Europe/Rome, após recuperação autorizada t43 às17:04: código real de catálogo/ledger candidato, bytesstreaming privados e planner retenção existem em `/srv/filuna-new-site`, **173 testesPASS/0FAIL =148baseline+25novos**. As afirmações abaixo de bloqueioSSH/ausência total de implementação são checkpoints HISTÓRICOS, superados somente nesta subfatia. Não foi publicada primeira versão integrada, nem criada confiança operacional;006 só em DBtemporário, processo operacional NÃO reiniciado e domínio estático intacto. [Evidência, rotas efetivas xcarregadas e limitações](WEB-FASE-05-IMPLEMENTACAO-EVIDENCIAS-20261108.md). GatesG2/G3BLOCKED,G4NOTRUN,G5/G6BLOCKED permanecem; nenhuma homologação/assinatura/certificação/updaterWindows inferida.


## Prioridade vigente — ordens t36–t38 de 08/11/2026

O proprietário autoriza iniciar AGORA a integração nativa de atualizações Slicer/servidor e o fluxo de assinatura pelo GitHub conforme regras. Primeira distribuição só beta/admin; até CINCO versões de instaladores, preservando o histórico documental; compilar somente quando necessário. A retomada geral/homologação fica condicionada à integração comprovada. Esta ordem atualiza a pausa histórica somente nesse escopo; não promove gates nem aprova telas derivadas.

**Estado neste checkpoint: especificação proposta, código e testes novos NOT RUN, nenhum instalador beta integrado publicado.** A sessão SSH antiga deixou de responder; uma conexão normal nova alcança o mesmo servidor, mas requer autenticação. Nenhuma chave/configuração foi alterada. Ler [pedido literal e limites](approvals/ATUALIZACAO-NATIVA-BETA-T36-T38-20261108.json) e [contrato pré-código](WEB-FASE-05-RELEASE-CONTRATO-20261108.md) antes de continuar.


**Contrato:** FLN-SERVER-2026-11-05.1 · **Origem:** decisão expressa do proprietário em 05/11/2026 · **Estado:** ESPECIFICAÇÃO; ainda não homologada nem implantada no novo fork.
**Regra de segurança inegociável:** o Filuna poderá observar, com controles de privacidade, incidentes e saúde técnica dos clientes; **o administrador Filuna NÃO terá capacidade de acessar, comandar, imprimir, pausar, cancelar, aquecer ou abrir câmeras das impressoras dos clientes**. O controle remoto futuro pertence ao **próprio titular da impressora** e a pessoas que ele autorizar expressamente, segundo política granular auditável.

## Determinação futura adicional — homologação física monitorada no beta (07/11/2026)

A ferramenta de **monitoramento online no servidor Filuna**, prevista em [REGRA-BETA-MONITORAMENTO-HOMOLOGACAO-FISICA-20261107.md](REGRA-BETA-MONITORAMENTO-HOMOLOGACAO-FISICA-20261107.md), deverá acompanhar automaticamente os trabalhos autorizados das impressoras pendentes de homologação física durante o beta vigiado: abertura/arquivo, fatiamento, AMS/multimaterial, perfis, G-code gerado × oficial × enviado × confirmado/executado quando observável, andamento, erros e término.

**Harmonização obrigatória com esta arquitetura:** a observabilidade é somente leitura; não cria poder de comando/impressões nem acesso à LAN/câmera ao administrador. Eventos de clientes exigem consentimento granular/revogável, dados mínimos e segregação; STL/3MF/G-code completos não são enviados automaticamente. A camada de ingestão não é autorização para imprimir; distinguir ACK, estado de job e confirmação real do firmware. O painel não deve marcar homologação física por `FINISHED` sozinho; preservar revisão humana. **Estado: requisito futuro, não serviço implantado.**

## 1. Fronteira: programas, servidores e dados

\`\`\`text
                     [Aplicativo Windows Filuna Slicer]
                   Orca core + GUI Filuna + DeviceBridge
               * slicing local, perfis e projetos no cliente
               * dispositivos e LAN pareados pelo próprio cliente
               * store offline cifrado + fila de eventos transacional
                                  |
                          HTTPS/TLS saída (443)
                     tokens e escopos por dispositivo
                                  |
             [API Gateway Filuna / autenticação / políticas]
                   1) Filuna ID / planos / tenants
                   2) Sync API / ledger / histórico / backups
                   3) Diagnósticos SANITIZADOS de software
                   4) Estado técnico agregado de dispositivos (opt-in)
                   5) Fila/broker de comandos remotos FUTURO - OFF
                                  |
              [Banco privado + objeto cifrado + logs separados]
              serviços só por rede privada, sem SQL no cliente
                                  |
             [Portal/mobile autenticado do próprio cliente]
               * histórico e status - fase futura de leitura
               * futura impressão remota - habilitação individual
               * nenhuma ação permitida ao administrador Filuna

             [Admin Filuna - observação e suporte]
               somente métricas/erros permitidos e minimizados
               sem canal de comando nem conexão LAN à impressora
\`\`\`

O servidor é **serviço crítico de sincronização, diagnóstico e crescimento**, mas **não é necessário para executar o motor Orca offline**. O Hub não fatia projetos do cliente e não atua como gateway irrestrito para sua LAN.

### Estado real verificado no legado

- O antigo \`abcollicelli-web/filuna-slicer\` documenta um Hub em \`/srv/filuna-slicer-windows-new\`, SQLite de **desenvolvimento** e listener privado **127.0.0.1** no bootstrap, com proxy HTTPS isolado em etapas posteriores. Arquivos: \`docs/ARCHITECTURE.md\`, \`docs/SYSTEMS_MAP.md\`, \`docs/FUP-003-DIAGNOSTICO-DISPOSITIVOS.md\`. **Não presume que a atual topologia de produção, IP e DB comercial estejam homologados.**
- Diagnósticos FUP-003 e telemetria v1/v2 do aplicativo anterior possuem eventos, pseudonimização e rotas de leitura; foram historicamente publicados em beta, mas **não estão migrados/homologados no fork C++**.
- Servidor Ubuntu acessado por SentinelX é diferente do Windows do proprietário e de seus clientes. **Nenhum servidor, banco, nginx ou segredo foi modificado nesta fase**.

## 2. Endereços, IPs, DNS, TLS e banco — NÃO hardcode

1. Criar \`FILUNA-SERVER-ENVIRONMENT-MANIFEST\` versionado **sem segredos**, por canal \`owner-beta\`, \`staging\`, \`production\`: \`api_base_url\`, \`allowed_origins\`, \`api_version\`, \`certificate_pin_policy\`, \`region\`, \`data_retention_policy_id\`, \`min_client_version\`, \`release_channel\`, \`status\`. O endereço **preferido é domínio HTTPS**, porque o IP pode mudar; não fixar IP público em DLL, JS, builds ou QR.
2. IP/hostname real do servidor, IP privado do banco, roteamento interno e portas ficam em inventário de infraestrutura de acesso restrito e no deployment, não embutidos como “constantes” do instalador. Publicar ao cliente somente hostname DNS e API pública autorizada; nenhum IP do cliente/impressora é transmitido por padrão.
3. Entrada \`443/TCP\` no gateway HTTPS, autenticação com OAuth2/OIDC + PKCE para pessoa, credenciais vinculadas ao dispositivo e tokens rotativos curtos; mTLS **quando viável** entre agentes/servidor; TLS moderno e validação de identidade/certificado. Proibir credenciais estáticas em pacote ou no Git.
4. DB de produção ainda **a selecionar e homologar** (candidato PostgreSQL gerenciado ou self-hosted com replicação/PITR; não assumir que SQLite DEV é DB comercial). Proibir acesso SQL direto pelo Windows/mobile e proibir expor porta do banco à Internet. \`api → service\` usa conta DB de privilégio mínimo, rede interna, conexões TLS quando configuradas.
5. Segredos no secret manager do host e vault (usuários/serviço em contas separadas) com rota de rotação, auditoria de leitura e teste de revogação. Nunca expor DSN, senha, token OEM ou chave LAN em ticket, repo, IA ou telemetria.
6. Separar serviços e ambientes: banco por ambiente, chaves por ambiente, storage por ambiente, tenants com controle lógico e, se necessário, isolamento adicional. Não reutilizar \`nucleo.db\` ou a pasta \`/srv/site\` sem plano de migração aprovado, backup testado e consentimento explícito.
7. O contrato de API deve suportar versionamento (\`/v1\`/futuras \`/v2\`), esquemas rejeitando campos desconhecidos onde necessário, compatibilidade retroativa, limites de payload, rate limit, idempotência e autenticação independente do cliente.

## 3. Modelo de dados de referência (lógico, NÃO migração SQL pronta)

| Entidade | Chave e relações | Responsabilidade e segregação |
|---|---|---|
| \`accounts\`, \`identities\`, \`sessions\` | \`account_id\`, OIDC sub, devices vinculados | identidade real, revogação e MFA para funções sensíveis |
| \`tenants\`, \`memberships\` | \`tenant_id\`, \`account_id\`, role | separação por cliente e permissões no servidor |
| \`device_installations\` | instalação Filuna, public_key/fingerprint | identifica o aplicativo, não concede controle da impressora |
| \`printer_registry\` | \`printer_id\`, \`tenant_id\`, identificação pseudônima/modelo | registro privado de máquinas visíveis **ao cliente** |
| \`printer_capabilities\` | \`printer_id\`, firmware, protocol, allowed operations, validated_at | flags do protocolo por impressora/firmware |
| \`printer_observations\` | \`printer_id\`, estado agregado, updated_at, TTL | **só com opt-in**; separar ONLINE de stale/UNKNOWN |
| \`jobs\`, \`job_events\` | \`job_id\`, \`tenant_id\`, \`printer_id\`, seq e idempotency | ledger de estados, jamais auto-imprimir por receber evento |
| \`filament_spools\`, \`stock_ledger\` | \`spool_id\`, \`tenant_id\`, \`job_id\`, revisions | estoque/histórico verdadeiro, sem estimativa fabricada |
| \`user_projects\`, \`project_objects\` | \`project_id\`, \`tenant_id\`, encrypted object ref/hash | **upload opt-in** e políticas de exclusão; preferir processamento local |
| \`sync_events\`, \`sync_cursors\` | \`event_id\`, \`device_id\`, \`sequence\`, \`entity_revision\` | event sourcing/idempotência/CAS/conflitos auditados |
| \`app_incidents\`, \`app_diagnostics\` | \`incident_id\`, \`tenant_id\`, \`install_ref\`, \`error_code\` | técnico sanitizado, retenção limitada e acesso admin segregado |
| \`consents\`, \`sharing_grants\` | \`tenant_id\`, subject, scope, expiry, revoked_at | base de permissão granular para status e remoto |
| \`remote_intents\`, \`remote_receipts\` | \`command_id\`, \`tenant_id\`, \`actor_id\`, \`printer_id\`, expiry | **FUTURO / tabelas sem operadores habilitados** |
| \`security_audit\`, \`backup_catalog\` | origem, hash, timestamp, retention, restore_case | auditoria de segurança, backup e restauração |

**Regras transversais:** cada row de usuário com \`tenant_id\`, relação de permissão; autorização **sempre no servidor**, não confiar em \`tenant_id\` fornecido pelo cliente. Invariantes do ledger e migrações transacionais com \`schema_version\`, rollback/forward fix. Apagamento por solicitação legítima/GDPR precisa ser possível — "não apagar nunca" não substitui direitos legais e políticas de retenção. Uma operação de delete deve ser autorizada, auditada e coerente com backup e restauração.

## 4. Proteção real contra perda de dados — compromissos testáveis

**Princípio:** cliente deve conhecer exatamente o que foi salvo localmente, o que teve ACK persistente no servidor e o que ainda espera sincronização. Sem falsas promessas de “100% dados imortais”.

- **Local:** SQLite transacional (ou storage equivalente) com integridade, migração reversível, cópia de segurança cifrada, gravações atômicas, WAL quando apropriado, restauração de projetos, configuração e fila offline persistente. A escrita de G-code/3MF não depende de Internet.
- **Sync:** eventos imutavelmente identificados \`event_id\`, \`device_id\`, sequência monotônica, revision/CAS, idempotency, hash de conteúdo; retries com backoff sem duplicação, DLQ e reconciliação. **Não confirmar “salvo na nuvem” antes do ACK de durabilidade servidor**.
- **Servidor:** DB com backups cifrados, **PITR**, retenção e rotação, verificações de integridade, réplicas conforme RPO, pelo menos uma cópia isolada fora do ambiente principal, controle anti-ransomware (imutabilidade/object lock quando possível), verificação 3-2-1 e testes frequentes de RESTORE em ambiente isolado.
- **RPO/RTO:** não prometer números antes da escolha do DB e ensaios. Estabelecer metas por classe (dados de conta/estoque/jobs críticos vs caches) e publicar RPO/RTO reais após medições e aprovação; perda de dados **já confirmados** pelo servidor é incidente crítico P0.
- **Reconciliação:** 2 dispositivos editando mesma bobina/arquivo, concorrência, clock drift, usuário troca PC, queda 1/50/95% de upload, disco cheio e upgrade interrupted; conflito pede escolha ou resolução por regra explícita, nunca sobrescrever silenciosamente.
- **Restores:** mensal automatizado de restore de subconjunto sanitizado e trimestral de desastre completo em staging; simular corrupção, expiração, revogação, backup incompleto e recuperação do último ACK. Cada ensaio produz duração observada, bytes recuperados, hashes, RPO/RTO e aprovação. Calendário é meta de processo, não resultados já executados.
- **Direitos do cliente:** export de dados, cópia de projetos e perfil, exclusão legal, acesso ao histórico e aviso de status de sincronização \`Local\` / \`Na fila\` / \`Salvo na nuvem\` / \`Falhou\`. Não dizer que nada será apagado se houver exclusão pedida, política de retenção ou ação do próprio usuário; garantir integridade e restauração verificáveis.

## 5. Monitoramento Filuna do software — sem controle de impressoras

**Dois canais distintos:**
- **A. Diagnóstico de aplicação:** erro de software, falha de atualização, crash classificado, versão/build, stack sanitizada, OS, etapa, módulo, causa observada, severidade, id de incidente; sem STL/3MF/G-code, modelos privados, caminhos, tokens, e-mails, IP/MAC/serial ou imagem de câmera. Usar regras do FUP-003 anterior como ponto de partida e adicionar scrubber C++ e teste de vazamento antes de enviar.
- **B. Saúde opcional de dispositivo:** cliente habilita “Compartilhar estado técnico com o suporte Filuna”; coleta minimizada: fabricante/modelo/firmware **apenas se necessário**, adaptador e versão, estado de conexão, categoria da falha e momento de última leitura. Nunca presumir consentimento pelo simples uso do app. Sem câmera, sem fila nominativa, sem nome de trabalho, sem leitura de sensor não necessária ao diagnóstico.

**Perfil de autorização:**
| Ator | Ver incidentes sanitizados | Estado técnico compartilhado | Ver job/projeto privados | Comando de impressão |
|---|---|---|---|---|
| Admin Filuna suporte | conforme função, necessidade e auditoria | **somente opt-in/abstraído** | **NÃO** | **NUNCA** |
| Dono de tenant/usuário da impressora | próprios incidentes e controles de privacidade | impressoras próprias | próprios dados | somente **após gate remoto futuro** |
| Membro convidado pelo dono | conforme escopo e duração | conforme escopo | conforme escopo | opcional, dependente de política explícita futura |
| Serviços de alerta | contagem/código de incidente | categoria agregada | NÃO | NUNCA |
| IA suporte técnico | diagnóstico sanitizado e necessário, acesso aprovado | dados mínimos | NÃO por padrão | NUNCA |

Administradores não recebem escopo \`printer.command\`, \`printer.camera\`, \`printer.lan\` nem podem solicitá-los por painel administrativo; **não existe “modo proprietário Filuna” para ver/manipular impressoras alheias**. Conta de suporte com acesso a dados identificáveis deve ser separada, MFA, aprovação por necessidade, trilha de auditoria e tempo limitado; cliente pode revogar observação.

Eventos **INFO** (impressora deliberadamente desligada) não acionam incidente P0. \`FIL-DEV-FAILED\` e \`FIL-APP-UNHANDLED\` podem ser agrupados por instalação/versão/erro e ter severidade por impacto, sem 233 alertas repetidos. FUP-003 W é fonte de testes, não implementação a copiar.

## 6. Programa de etapas do servidor — ligações exatas com o produto

| Fase | Depende de | Entrega concreta | Gate |
|---|---|---|---|
| **SRV-00 Topologia e inventário** | Governança/G0, LEGAL | inventário privado de servidores/ambientes, domínio, IP e DNS real, riscos, configuração sem segredo, diagrama e proprietário técnico | G0 |
| **SRV-01 Gateway/DB base** | SRV-00 + CI e segurança | HTTPS, API versionada, controle de acesso, DB privado escolhido, isolamento staging/prod, migrations, audit logs | G2/G3 |
| **SRV-02 Filuna ID e autorização** | SRV-01 + contrato de dados | OIDC/PKCE, sessões, MFA sensível, tenancy, RBAC/ABAC, revogação, API de perfil | G3 |
| **SRV-03 Sync/backup confiável** | SRV-02 + cliente storage local | fila/outbox, ledger, backup offsite cifrado, PITR, teste restore, status real no app | G3/G4 |
| **SRV-04 Diagnósticos seguros** | SRV-02 + logging local | v1/v2 compatível, scrubber duplo, alertas agrupados, painel admin **read-only**, privacy controls | G3/G4 |
| **SRV-05 Observações de impressoras** | DeviceBridge local homologado + SRV-04 | opt-in, status agregado/TTL/UNKNOWN, cliente revoga, admin não comanda | G4 |
| **SRV-06 Portal cliente somente leitura** | SRV-03 + SRV-05 | próprias impressoras, histórico, estoque, job e câmeras **apenas se houver autorização específica e privacidade** | G4/G5 |
| **SRV-07 Base remota desativada** | SRV-06 + segurança/ameaças | schema de intent/receipt, isolamento actor/tenant, envelopes assinados, fila com TTL, flags OFF | G1/G3; nenhuma execução real |
| **SRV-08 Pilot remote print (cliente)** | **DeviceBridge + impressão local homologados em hardware**, SRV-07, MFA, consentimento e autorização do dono | comando remoto para a impressora PRÓPRIA, sessão ativa, assinatura, revogação e ACK real | **G4 remoto obrigatório** |
| **SRV-09 Mobile/web com comando** | SRV-08 PASS, política de emergência e fraude | visualizar, enviar/iniciar/pausar/cancelar **conforme firmware**, controles do cliente, histórico e rollback | G4/G5 |
| **SRV-10 Lançamento remoto** | SRV-09 + segurança, DR e owner sign-off | rollout canário, contrato de suporte, verificação jurídica/seguro, documentação e kill switch | G6 específico remoto |
| **SRV-11 Operação contínua** | SRV-10 | alertas, patches, firmware changes, auditoria de permissão e ensaios de DR | permanente |

**Regra de prioridade:** \`SRV-01..05\` é fundação útil desde o início, mesmo se nunca ocorrer remoto. \`SRV-07\` pode reservar nomes de API/schema e UI atrás de feature flag, mas não oferecer “comando” funcional até \`SRV-08\` PASS. O cronograma principal pode ter fases paralelas de pesquisa, mas **nenhuma liberação do remoto antes da impressão LAN local ser física e integralmente homologada**.

## 7. Segurança específica de impressão remota — futura

**Arquitetura autorizada:** app móvel/web do cliente autenticado → servidor valida \`tenant/actor/scope\` e política da máquina → registra \`RemoteIntent\` com expiração e nonce → Filuna Agent do cliente, que estabeleceu canal **de saída** autenticado, busca mensagem → verifica assinatura, usuário/permission/firmware, job, estado, confirmação e nonce → opera pelo DeviceBridge **local** → recebe ACK físico e retorna \`RemoteReceipt\`. **O servidor não abre conexão direta à LAN privada do cliente.**

- **Pré-condição:** cliente que é dono da máquina fez pareamento local e **habilitou individualmente** acesso remoto. Controles podem ser desligados sem login no painel técnico local. \`deny_by_default\`; admins Filuna não têm token/role de controle.
- **Autenticação:** OIDC/PKCE, sessões de curta duração, MFA/step-up antes de imprimir/cancelar/alterar calor, confiança no dispositivo e autorização de membro por impressora com prazo/escopo. Credenciais LAN nunca saem do agente local.
- **Comando:** assinado, autenticado, expirado (TTL curto), vinculado a \`tenant_id, actor_id, printer_id, job_id, expected_firmware, operation, policy_version, request_id\`, proteção a replay e numeração monotônica; **idempotência forte**, ACK semântico, reconciliação. Sucesso de API enqueue ≠ sucesso de impressão.
- **Consentimento:** confirmação explícita do cliente no app ou web para cada início; mensagens de autorização só são permitidas do titular/delegado autorizado, nunca do suporte. Sem material/mesa prontos ou proteção física desconhecida, bloquear e orientar inspeção local.
- **Estados offline:** com Windows/agent desligado **não há canal de impressão**. Futuro gateway local dedicado do cliente (p.ex. Raspberry Pi) só é nova etapa com produto/licença/testes; jamais prometer “imprime com PC desligado” sem isso.
- **Ações:** começar com **consulta remota**, depois **pausar/cancelar** e somente em seguida **enviar/iniciar trabalho** após teste de segurança. Aquecer, mover e firmware ficam **fora do MVP remoto**, por risco físico.
- **Câmera:** consentimento separado, sem gravação/nuvem por padrão, codec real, sinalização de streaming, acesso restrito e revogação instantânea. Permitir ao cliente ver sua câmera não dá direito ao suporte Filuna.
- **Incidentes:** kill switch por release/marca/firmware, revogação de sessão/grant, detectar anomalia, limites de frequência, auditoria sem dados privados. Nunca “replay” remoto após timeout.
- **Teste remoto real:** usuário A vs B, cross-tenant, role support, token roubado/revogado, TTL expirado, sessão offline, comando duplicado, estado stale, duas redes, alteração firmware, câmera sem permissão, conflito de jobs, print iniciado sem consentimento; **100% PASS** e peça real quando anunciar impressão.

## 8. Testes e níveis de evidência obrigatórios

| Case ID | Caso | Gate de aprovação |
|---|---|---|
| SRV-QA-001 | DB sem porta pública e API com TLS/host correto | G2 |
| SRV-QA-002 | clientes não podem executar SQL/ler segredos do servidor | G2/G3 |
| SRV-QA-003 | isolamento tenant A/B nas entidades, logs e arquivos | G3 |
| SRV-QA-004 | sync offline/online com retries idempotentes e conflitos CAS | G3 |
| SRV-QA-005 | backup cifrado offsite e RESTORE completo comprovado por hash | G3/G4 |
| SRV-QA-006 | cliente vê Local/Na fila/Salvo na nuvem/Falhou sem falso ACK | G4 |
| SRV-QA-007 | diagnóstico sem nomes de modelos, e-mail, IP, serial, token ou câmera | G3/G4 |
| SRV-QA-008 | suporte Filuna tem apenas incidentes sanitizados; **nenhuma permissão de comando** | G3/G4 |
| SRV-QA-009 | cliente opt-in/out das observações, revogação imediata e TTL | G3/G4 |
| SRV-QA-010 | leitura do painel cliente não permite controle remoto | G4 |
| SRV-QA-011 | com feature OFF, **ZERO remote intents** são executados | G2/G3 |
| SRV-QA-012 | usuário não autorizado/tenant B/support não consegue criar comando | G3/G4 |
| SRV-QA-013 | comando vencido/replay/duplicado não chega à impressora | G3/G4 |
| SRV-QA-014 | app local/desligado ⇒ intent não provoca impressão inesperada ao voltar | G4 |
| SRV-QA-015 | sem confirmação expressa e capability real ⇒ ZERO \`StartPrint\` | G4 |
| SRV-QA-016 | ACK de broker não se passa por ACK da impressora | G3/G4 |
| SRV-QA-017 | cliente revoga remoto/câmera; servidor/agent respeitam imediatamente | G4 |
| SRV-QA-018 | atualização/reinício/queda de LAN durante impressão remota não duplica job | G4 |
| SRV-QA-019 | prova de restauração parcial com dispositivo sem sync e dados não confirmados | G4 |
| SRV-QA-020 | rollback remoto com flags OFF e core Orca livre preservado | G5/G6 |

**Não declarar nenhum PASS sem execução real.** Testes de \`SRV-08..10\` dependem de PC Windows, LAN/impressora física, duas contas isoladas e cliente remoto autenticado. A avaliação legal inclui GDPR, CRA, AI Act para diagnósticos com IA quando houver e direito do consumidor aplicáveis.

## 9. Decisões pendentes e riscos abertos

- **Produção:** definir região UE, fornecedor/DB, domínio e gateway real, retenção e orçamento; IP não se torna fonte de verdade se DNS/rotas podem mudar. Verificar servidor em modo leitura antes da configuração.
- **Dados:** classificar projetos/modelos e uploads opcionais, privacidade e termos, RPO/RTO mensurados, restore e backup offsite, direito de exclusão.
- **Dispositivos:** prioridade de fabricantes e hardware real; capacidade de status agregada sem expor detalhes do dono; opt-in e logs.
- **Remoto:** definir modelo de autorização doméstica/atelier e possibilidade de colaboradores, seguro/responsabilidade por controle físico, resposta a incidentes e opção de gateway quando PC desligado.
- **Implementação:** fork C++ não contém o Hub W automaticamente. Reutilizar contratos, não copiar serviços/dados de produção cegamente. Atualizações de protocolo/firmware requerem reteste.
- **Conformidade:** avaliar CRA comercial, GDPR, retenção de vídeo e, quando aplicável, avaliações externas de segurança. **Esta documentação não é homologação legal**.

**Regra final:** Filuna pode prestar suporte técnico lendo **erros e estados autorizados minimizados**; não terá controle remoto das impressoras dos clientes. A **impressão à distância do cliente** só existirá depois do **controle local aprovado + servidor, privacidade, segurança e hardware remoto 100% testados**, com opt-in, revogação, dossiê e aprovação por versão.


<a id="web-servicos-20261108"></a>
## 10. Mestre servidor/web — nova autorização de 08/11/2026, 14:57 Europe/Rome

Este arquivo já é o mestre de servidor/dados; **é ampliado em continuidade, não substituído por árvore ou mestre paralelo**. O proprietário solicitou home pública C, admin e serviços novos e depois declarou **«esta liberado»**. Pedido completo e autorização literal: [registro de origem](approvals/WEB-SERVICOS-AUTORIZACAO-20261108.json). [Plano mestre Slicer](PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md#web-servicos-20261108) e [contrato detalhado obrigatório](WEB-SERVICOS-ESCOPO-20261108.md) ficam cruzados.

Desenvolver apenas em `/srv/filuna-new-site`, usando seu `docs/NEW-SITE.md` como mapa de stack, árvore, ownership e deploy reais. Cofre preservado; apenas seus dois guias permitidos podem ser lidos sem revelar segredos, sem restaurar, migrar ou buscar credenciais. Não presumir que DB/stack/topologia do legado são os do novo site. Não publicar código web privado nem dados/segredos no repositório público Slicer.

A extensão compreende home descritiva candidata C, shell admin protegido, conta/usuário/planos/direitos, conteúdo original/versionado e receitas locais, perfis e deltas OEM sob demanda, estoque/configurações, downloads/update, backup/retomada, central/ajuda/chat. A §3 continua modelo lógico, não DB implantado. Servidor distribui direitos/manifests e dados; geração e fatiamento ficam no PC, não servidor. Admin não adquire nenhum controle de impressoras. API/schema/migrações versionados, RBAC/tenant isolation, idempotência, assinatura/hash/compatibilidade/replay, cache/renovação assíncrona, consentimento, rollback e restore verificável são obrigatórios. Nenhum novo preço, janela offline, dispositivo permitido ou provedor é escolhido por inferência.

Contratos comerciais com core AGPL/offline devem ser reconciliados antes de ativar bloqueio: pedido de chave não autoriza esta IA a revogar silenciosamente fatiamento básico livre. Dados próprios não são apagados por suspensão. A nova autorização permite pesquisa, implementação e testes da fatia web isolada; não habilita `SRV-08..10`, não retoma implementação geral Slicer e não promove G2–G6. Ordem de aceitação, dependências reais e casos WEB-T01..10 constam no contrato. Atualização segura significa prevenção, teste/rollout e recuperação, não garantia metafísica de nunca falhar.

**Checkpoint deste lote:** documentação registrada neste commit; acesso ao novo ambiente/NEW-SITE ainda não estabelecido neste contexto, stack/rotas/deploy não verificados. Implementação, schemas executáveis, migrações, testes funcionais e deploy NOT RUN. Atualizar a continuidade a cada marco com evidências reais, sem passar contrato por serviço ativo.


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


<a id="web-publicacao-c-t35-20261108"></a>
## Publicação visual web C — t35 (08/11/2026 Europe/Rome)

**Ordem literal do proprietário, t35u,16:19 Europe/Rome:** «aprove e publique». [Aceite estruturado](approvals/WEB-VISUAL-PUBLICACAO-T35-20261108.json). A candidata refeita após t34, ZIP SHA256 `874b50d79ba3b1c301f77e19455e452cb03fe808870999fd1985e47307a78839`, foi aprovada e **publicada realmente16:23:45 Europe/Rome** em https://filuna3d.com/, `/conta/`, `/conta/acesso/` e cinco áreas canônicas. [Evidências de publicação, hashes, limites e rollback](WEB-PUBLICACAO-C-T35-EVIDENCIAS-20261108.md). Registros anteriores de não publicação/visual pendente são históricos; a fase04 de autenticação permanece íntegra.

Publicação estática allowlisted via Nginx/TLS existente na mesma raiz nova, releaseversionada/symlink atômico. CSS/JS/Inter/logo/geometria mantêm bytes aprovados; somente links/paths canônicos, CSP de publicação e nota factual de aprovação adaptados. Home e Central agora usam a composição revista, sem reinterpretar o modelo aprovado nem alterar âncorasSlicer. Backend/app/client_portal/auth/storage/admin/migrações não alterados/reiniciados; contratos/índice recebem apenas registro aditivo da publicação. Nenhum segundo backend/CMS ou cofre/legado.

**Evidência:** 45 verificações públicasPASS com HTTPS/certificado normal,16 arquivos publicados por hash e145 referênciasallowlisted;2 consultasHTTP do box500Pingora, enquanto origem301HTTPS, limitação registrada sem bypass/DNS/proxy alterados. Admin anônimo401; arquivos internos/traversal/APIsprivadas404 no domínio. Backend148testesPASS reexecutados após deploy; design115estáticos+11doublesDOMPASS. Nginx-t/reloadPASS; snapshotnovo/config anterior e rollbackpreparados. No checkpoint inicial, Browser/CU público e execução real de rollback estavam NOT RUN neste executor; desktop público foi conferido posteriormente pelo coordenador, no adendo abaixo. Rollback executado e mobile público continuam NOT RUN. Não chamar47/47PASS ou auditoriaAA.

**Publicar visual NÃO ativa serviços:** e-mail/senha disabled, Google explicativo, nenhum cadastro/checkout/backup/download/premium simulado. Auth/trust operacionais continuam ausentes; privado loopback503, API pública404 por não publicação. Básico/baseOrca livres/offline na MESMAengine, sem carregar catálogoFiluna homologado sem direito válido; dados próprios preservados. G2/G3BLOCKED,G4NOTRUN,G5/G6BLOCKED e FLN-VISUAL-C-100 desktop/hardware não promovidos. Próximo passo do coordenador: inspeçãoCU no domínio e integração segura de serviços somente com configuração legítima/escopo específico.


### QA público desktop posterior — 08/11/2026 16:30 Europe/Rome

O coordenador executou Chrome público real, desktop1280×800/zoom100%, sem emulação e HTTPS normal sem aviso de certificado. Abriu a home, clicou **Central do cliente** para `/conta/` e **Acessar conta** para `/conta/acesso/`. Logo e estudo3D renderizaram; não houve fallback de fonte evidente nem texto/botões sobrepostos nas áreas vistas. Acesso mostrou conexão não configurada e campos desativados. Google não foi clicado, nenhuma credencial submetida. Executor inspecionou as três capturas brutas, sem segredos aparentes.

**Estado atual:** desktop público conferido no escopo acima; mobile público, comparação pixel/diff, auditoriaAA e homologação funcional/global **NOT RUN**. Captura1280×800 não fixa a equivalência CSS/device/DPI. Os45 checksHTTP/HTTPS anteriores continuam45; os2HTTPsimples500Pingora permanecem registrados, sem contarCU como mediçãoHTTP adicional. Não ativaGoogle/SMTP/login/pagamento nem altera gates/aplicativo/hardware.

[Capturas brutas e manifesto SHA256](evidencias/web-t35-public-20261108/MANIFEST-SHA256.json): home, Central e acesso, sem resize/crop/máscara. SHA256 do manifesto `f9680c1019474669fd6ae57c2c4b6f97e5c44f249d0d6fdc119cfdcf064ba790`. NOTRUN anterior do browser é histórico do checkpoint do executor, superado SOMENTE por esta inspeção desktop delimitada.


## Fase05 privada — releases/integridade/retenção: contrato proposto, implementação bloqueada
08/11/2026 Europe/Rome. t36/t37 resumidos pelo pai exigem integração nativa desde primeira versão, resumo/reabertura seguros e histórico completo; download inicial só beta/admin, máximo CINCO VERSÕES de instaladores, novo build só necessário. t38 literal: «a assinatira da priimeira versao sera feita pelo github conforme regras». Não equivale a dispensar ECDSA manifesto, SHA256, assinatura real de código/identidadeFiluna, consentimento/salvamento ou QAWindows; attestation GitHub é proveniência separada. [Especificação pré-código e testes NOT RUN](WEB-FASE-05-RELEASE-CONTRATO-20261108.md).

**Checkpoint honesto:** somente especificação/plano local e patch documental; código/rotas/migração/serviço privado novo NÃO implementados nesta fatia. Canal SSH existente deixou de responder a comandos, embora check local do master tenha sucesso; nenhuma mutação remota, acesso alternativo, build/CI/assinatura/publicação/pruning/remoção. Fase04 148 PASS é histórica, não novo baseline executado. Estado atual DB/zero dados NÃO reverificado. Config/trust não devem ser inventados, frontend aprovado/t35/APIs públicas/nginx/cofre/legado/desktop preservados por não atuação. Retomar leitura de NEW-SITE/mestres atuais pelo mesmo acesso existente antes de código.


## Continuidade executável — Fase05.1 privada (08/11/2026 Europe/Rome)
T36–t38 literais continuam no [registro autorizado](approvals/ATUALIZACAO-NATIVA-BETA-T36-T38-20261108.json); backend agora implementou primeira subfatia REAL, não outra proposta: ledger imutável de candidatos/história, ECDSAcapacidade de verificação, streaming privado por64KiB, autorização tenant/conta/beta/adminMFA/licença e plano retenção5VERSÕES global/pins/leases/snapshotconflito. Nenhum executor de pruning/publicação, nenhum artefato real removido/importado/assinado. ECDSAcurva/envelope/raiz NÃO escolhidos operacionalmente; testes P256/SHA256/DER são só capacidade. Adaptadores GitHubprovenance/Windowscode/ownerQA apenas fixtures em memória, não provas reais.

173PASS de unidade/integradosHTTP/cripto não equivalem aos22 requisitos globais aceitos: matriz19PASS estritamente escopados/3NOTRUN com subcasos incompletos explícitos. NativeWindows/confirm/save/helper/reopen/rollback, cadeiaGitHub/Windows e recovery/promotion/pruning reais continuamNOTRUN. Auth/trust config AUSENTES; DBoperacional intacto1–5/integridadeFK/zero contas e catálogo, tabela006 não existe nele. Código novo NÃO carregado no PID2089150, nenhum restart/deploy; domínio/Nginx/publict35 preservados. Antes de aplicar006 futuramente exigir backupDBnovo/revisão/coordenação. Não executar --migrate-only cegamente.

[Arquivos/hashes, testes, contratos, matriz e próximas lacunas](WEB-FASE-05-IMPLEMENTACAO-EVIDENCIAS-20261108.md). Nada de cofre/legado/OS/SSHconfig/CI/build/pagamentos/GoogleSMTP/hardware. Operação atual permanece fechada e catálogo real vazio, básicoOrca/offline e arquivos próprios preservados.
