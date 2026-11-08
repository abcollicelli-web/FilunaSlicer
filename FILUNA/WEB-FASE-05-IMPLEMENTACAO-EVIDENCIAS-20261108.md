# Fase05.1 — releases privados: código testado, operação desativada

Checkpoint08/11/2026 Europe/Rome. Base documental `6c5aaea2732f5e387443abf27589bbdf37665e8f`; retomada t43 autorizada17:04Europe/Rome pelo canal autenticado existente, sem configurar SSH/OS/chaves. Não promover gates nem afirmar instalador nativamente integrado disponível.

## Autoridade literal, preservando regras
### t36u literal
> vamos assinar uma versao do filuna conforme regras documentadas, criar uma ligacao do filuna com o servidor  para que meu ciente receba atualizacao automaticas diretamente com fechamento e abertura automatica a atualiçao tem que nascer nativa dessa maneira desde a primeira versao e nunca ser abandonada, cada versao e correcao fica no servidor listada e documentada, e apresentada no filuna um resumo do que esta sendo aplicado, o modelo padrao e como sera feito o filuna esta em documento tecnico ate mesmo com imagens
> precisa ser interligado desde o começo filuna slicer e servidor apos isso começaremos a retomar a homologacao e criaçao de todas as etapas do filuuna slicer ate obter certificado e homologacao em todos os itens

### t37u literal
> e ja colocar a 1 versao para download apenas para beta e administrativo no site e cada versao deve ir para o site com limite de 5 versoes removendo as anteriores, update de novos instaladores e compilaçao serao criadas somente se houver necessidade de compilar uma versao nova

### t38u literal
> a assinatira da priimeira versao sera feita pelo github conforme regras

Fontes read-only: dois mestres/continuidade atuais6c5aaea, qualidade46–60, UPD-W14–32, conformidade63–64/CMP-015. Legado UPDATE-RELEASE-POLICY@61339ae adendo04/11 foi evidência relatada pelo coordenador, não extraída do cofre. T36 automatiza ciclo APÓS confirmação/salvamento seguros, não os revoga. T38 GitHub NÃO equipara keylessprovenance a assinaturaWindows/identidadeFiluna/ECDSAmanifesto. Sem trustroot/chave/certificados de produção criados por inferência.

## Código REAL, privado, canônico
`service/releases.py`: valida registro candidato C/D, notas/classificação/impacto/restart/necessidade realinstalador/compat/deps/licença/origem/versão/hash/tamanho, sequência monotônica e imutabilidade/idempotência de evidência; eventos correção/review/withdrawal append-only, encadeamento hash (não log inviolável contra DBadmin). Registro candidato não é publicação. Triggers UPDATE/DELETE negados nos testes. Withdrawal append-only bloqueia manifesto/bytes; catálogo marca withdrawn_ids e mantém /history para o autorizado, testado por HTTP sem apagar versão.

`006_private_release_candidates.sql`: schema candidato/eventos/escoposbeta/licenças/presençabinários/pins/leases. **Só aplicado em DBs temporários de TESTE**, idempotência/checksum/foreignkeys,007 inválida rollback comprovados. Operacional NÃO migrou:1–5, sem tabela release_records. Não repetir001–005 nem executar006 sem backup/revisão futuros. `service/domain.py` reader usa sqlite3.complete_statement para triggers/quoted semicolon, mantém BEGIN/rollback sem executescript/commit implícito. Backups sourcebefore só app/domain em runtime, não restore; nenhum backupDB necessário/executado pois não houve migração operacional.

Integração APENAS backend em `service/app.py`: dispatch privado e payloadstream com fechamentoFD/lease inclusiveHEAD, sem mudar front/client_portal/auth/storage. Contrato `contracts/releases.v1.json`, índice/OpenAPI registrados; mestre/NEW-SITE/continuidade/migrações/spec locais atualizados aditivamente com ordens literais antes do código. Dependências existentes fixadas em requirements.lock preservadas, nenhuma instalação nova.

## Rotas implementadas em fonte e servidores efêmeros dos testes
- GET `/api/v1/tenants/{tenant}/releases`: catálogo somente beta autorizado por DB+membership ou adminMFA legítimos.
- GET `/api/v1/tenants/{tenant}/releases/{release}/manifest` e `/history`: provas separadas antes de retornar metadados/história, install_allowedfalse.
- GET `/api/v1/tenants/{tenant}/releases/{release}/artifacts/{artifact}/bytes`: exige também licença porconta/tenant/release, compat declaradaWindows/arquitetura/client major.minor.patch dentro min/max, descriptor e bytes íntegros. Declaração client não é atestação nem autoridade de direitos.
- GET `/api/v1/admin/releases/retention-plan`: platformrole server-side e MFA. BFF atual sem MFA não pode assumir admin.

BFFcookie verificado OU accessBearer verificado, ambos juntos negados; clientbeta/role não concedem nada. BFF real de TESTE após cadastro/verificação/login pôde ler catálogo quando grant TEMP concedido, logout negou novamente. Nenhuma mutaçãoHTTP release/ingest/pruning neste recorte; POST405, CSRF/handlers legados preservados e reexecutados.

**NÃO ativadas/publicadas/carregadas no processo operacional.** Nenhum restart/reload/deploy/Nginx. PID2089150 UID33 permaneceu vivo e igual. Smoke no processo ANTIGO: options200; tenantreleases503 pelo guard antigo; adminretention404 pois dispatch novo não foi carregado. Nova função route em processo isolado nega503 sem config mesmo no DBoperacional1–5, antes de acessar tabelas006. Isso não é evidência de serviço novo já em execução nem APIHTTPS pública.

## Trust e assinatura: limites materiais
ECDSA real por cryptography existente, chave efêmera de TESTE P256/SHA256/DER em memória. Função de biblioteca aceitaEC>=256bit e hash explícitoSHA256/SHA384; **não seleciona curva/hash/envelope/root operacional** e não afirma compatibilidade com cliente. ReleaseTrust requer QUATRO verificadores server-side: ECDSA, GitHubprovenance, Windowscode/identidadeFiluna e ownerQA. Nesta rodada apenas ECDSA é criptografia real; três últimos são fixtures sintéticas em memória ligadas ao digest/repo/commit/workflow/run. Ausência/verificador inválido/boolFalse nega. **Não existe loader/config de trustrelease operacional nem cadeiaGitHub/Authenticode implementada/homologada.** Ed25519 entitlement e GoogleRS256 continuam separados; nenhuma senha/login gera premium/releaseapproval.

## Streaming, integridade e retenção realmente exercitados
Raiz releases privada dedicada, nome/hash seguro, regularfile/no symlink/hardlink, ingest interno limitado porcaller confiável, read64KiB/size/hash/fsync/link, sem extrair/executar. Nenhum HTTPupload de instalador implementado; endpoint storage anterior default8MiB/teto128MiB INALTERADO. Bytesout: hash/tamanho integrais porFD antes de headers, mesmaFD, fingerprint e direitos/trust/licença revalidados entrechunks; Content-Digest/X-Content-SHA256 reais. Partial transfer não é ACKsucesso; cliente deve rejeitar comprimento/hash incompletos. Range/resume/CDN não implementados (Range400).

Teste gerou **168864208bytes sintéticos** em TEMP, NÃO baixou ZIP/EXE/candidato antigo. Maior read **65536bytes**, picoPython tracemalloc observado **262835bytes** (não mediçãoRSSglobal), sem carregar pacote inteiro. TEMP removido automaticamente; não vira release.

PlanoSQL usa read snapshot coerente; conta CINCO VERSÕES global, não5arquivos/5porcanal. Teste6versões/12artefatos propõe1versão, mantém TODAS notas/metadados/hashes. Pins e leases duráveis preservam download/rollback; pin concorrente invalida digest do plano; todos pinned =>BLOCKED. **Sem executor de pruning/eliminação/promoção/rollback**: limite só planejado nesta subfatia, nenhuma retenção operacional ativada. Crashdelease fica conservadoramente pin até revisão, semTTLinventado. Audit reporta staged/órfão/faltante sem deletar. Filesystem+SQL NÃO atômicos; errofsync póslink pode deixar órfão privado reportado e semACK. Recoverytransacional completo continuaNOTRUN.

## Testes, falhas corrigidas e honestidade
**173PASS/0FAIL**,15.281s:148baseline+25novos;148também reexecutados antes do código12.108s. pipcheckPASS, ResourceWarning tratado como erro no final. HTTP real emloopback, Ed25519acesso e ECDSA reais efêmeros; provasGitHub/Windows/QA não reais. Primeiros ensaios detectaram parserSQL sem suporte a trigger, slicing inválido de regexMatch e observação prematura de cleanupHTTP; corrigidos e logs históricos mantidos. No último caso teste espera evento real apósfinally do handler, sem enfraquecer lease. NÃO esconder primeira falha sob claim de zero erros históricos.

Matriz22 original ≠ contagem deunittest:19PASS em escopo explícito,3NOTRUN para requisitos completos, incluindo subcasos que passaram abaixo:

| Caso | Resultado escopado | Evidência/limitação |
|---|---|---|
| REL01 | PASS | isolated synthetic fixtures and real library/HTTP tests, not operational approval |
| REL02 | PASS | isolated synthetic fixtures and real library/HTTP tests, not operational approval |
| REL03 | PASS | isolated synthetic fixtures and real library/HTTP tests, not operational approval |
| REL04 | PASS | isolated synthetic fixtures and real library/HTTP tests, not operational approval |
| REL05 | PASS | isolated synthetic fixtures and real library/HTTP tests, not operational approval |
| REL06 | PASS | ambiguous credentials/unsupported mutation denied; real BFF signup+logout tested; CSRF baseline reexecuted, no release mutation route |
| REL07 | PASS | isolated synthetic fixtures and real library/HTTP tests, not operational approval |
| REL08 | PASS | real ECDSA valid/invalid/wrong key/revoked adapter; no operational curve/envelope/root approved |
| REL09 | PASS | negative separation: missing Windows verifier/false verification result closes; real Windows/GitHub chains NOT_RUN |
| REL10 | PASS | synthetic memory-proof digest/repository/commit/workflow binding only; real GitHub attestation verifier NOT_IMPLEMENTED/NOT_RUN |
| REL11 | PASS | isolated synthetic fixtures and real library/HTTP tests, not operational approval |
| REL12 | PASS | size/hash/abort/bound negatives,168864208byte generated fixture streamed64KiB; no real artifact fetched |
| REL13 | PASS | isolated synthetic fixtures and real library/HTTP tests, not operational approval |
| REL14 | PASS | isolated synthetic fixtures and real library/HTTP tests, not operational approval |
| REL15 | NOT_RUN | sequence replay/unknown channel denial PASS; signed native downgrade/authorized rollback/client anti-replay NOT_RUN |
| REL16 | PASS | planner counts6versions/12artifact references and proposes1version removal; no actual pruning/promotion executor |
| REL17 | PASS | durable read lease/pins, stream and concurrent SQL pin invalidates snapshot; crash-stale leases conservatively pinned |
| REL18 | PASS | planner has no executor, customer bytes/histories unchanged; no real deletion |
| REL19 | NOT_RUN | fsync-after-link fault/staged/orphan/missing audit/failclosed read PASS; full publish/unlink/crash recovery executor NOT_IMPLEMENTED/NOT_RUN |
| REL20 | PASS | 006 isolated/idempotent and invalid007rollback/quotedsemicolon PASS; operational006/backup/rollout NOT_RUN |
| REL21 | PASS | baseline148 reexecuted before code/included173final;38protected source hashes unchanged; schema1..5/process unchanged |
| REL22 | NOT_RUN | native Windows/helper/confirm/save/close/reopen/update/rollback NOT_RUN, not implemented/proven by backend |


## Estado efetivamente conferido e preservação
Às17:17:35 Europe/Rome: DBintegrityok/FKsemviolações, ledger1–5, TODAS tabelas operacionais0linhas exceto schema_migrations5. Nenhuma conta/grant/release/arquivo real semeado. Configauth/securityausentes; raiz operacionalrelease-objects nem criada. PIDvivo/UID33/semrestart. **38 arquivos protegidos** public/admin/migrações001–005/authBFF/security/client_portal/storage/object_store preservados porSHA; lockdeps preservado versusmanifestofase04. Publicação t35/CDN/API não alterados, não explorado cofre/legado nem gerado hash indevido de seu conteúdo. Preservação significa fronteira dos comandos/novo, não auditoria absoluta do cofre.

## Hashes de arquivos privados alterados (não código publicado)
- `service/releases.py`: `686befc69886bf051bbcb9d70a52261e6525108c8a999c32b357f9d1be210f0d`
- `service/app.py`: `fce999e957f261fcff0a347a90358a883858a12477dd04ac9e7a044a17852be0`
- `service/domain.py`: `8ca8578a1a6bba6c36e65d4a0350fcaca522c259e332ee97e9f9fb0c3f04d5d3`
- `tests/test_releases.py`: `65f97ac3ef635cfba45ea4f264f07462abb96eeacee0d51dad3b5b1b1cc0b176`
- `migrations/006_private_release_candidates.sql`: `6e54d6dff660db805bd30fec8374da8d84b0c789197694f3a77041e5ea00b6bc`
- `contracts/releases.v1.json`: `3a22f9627e0b11f2f32d049b7cc99f29c771a23645717a511f0088c3ebaccc60`
- `contracts/index.json`: `a6f76482c49eae4fe130be847c73d734b16fbd23aa18800c50296bfc35a9d3b9`
- `contracts/openapi.v1.json`: `9c9bce539828262474e8d5d76ef3c5c861a42b0f3ac23b1e6153f5f9ba226f92`
- `requirements.lock`: `77010e8e663cd7641e6772b7f66716b30cded8d22b73e719f23157bcf7193c6a`

Manifesto fonte/docs privado SHA256 `4638ea5954856c5c6dd99e6b5aa0f1f7eccfd556918014f9ed3fa25fabe35fe5`. Evidências servidor `/srv/filuna-new-site/docs/evidencias/fase05-*`, `RESULTADOS-FASE-05.json`, `MANIFESTO-FASE-05.json`; espelho privado box `/workspace/filuna-web-fase05-implementation-20261108/final-mirror/`.

## Próxima subfatia e faltantes
Resolver contrato releaseECDSA/envelope/root e verificadores reaisGitHub+Windows/Filuna+ownerQA por configuração legítima, sem pedir segredos emchat; projetar ingestHTTP/promoção/CAS/recuperação/pruning executável ensaiados antes de ativar. Operacional006 só após backupDBnovo/revisão; carregar serviço só por coordenação explícita, não confundirfontecomruntime. Integração clienteWindows confirmar/salvar/helper/fechar/reabrir/rollback, QAWindows/buildassinado e aceiteexato versão/SHA/canal continuamNOTRUN. Nenhuma certificação/updatercliente/installercompatível declarada. BásicoOrca/offline/arquivospróprios e licenças livres preservados, semcontrole de impressoras ouretomada geral.

## Evidências espelhadas no GitHub

[Resultados e estado operacional](evidencias/fase05-private-releases-20261108/RESULTADOS-FASE-05.json), [log dos 173 testes](evidencias/fase05-private-releases-20261108/fase05-tests.txt) e [manifesto original de hashes](evidencias/fase05-private-releases-20261108/MANIFESTO-FASE-05.json). Esses arquivos são cópias integrais do espelho privado; nenhum código do servidor foi incluído nesta publicação documental.
