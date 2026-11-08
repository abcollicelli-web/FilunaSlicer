# FASE05 — contrato privado de releases, proposta pré-código

## Estado mais recente — Fase05.1 privada implementada/testada, NÃO ativada

08/11/2026 Europe/Rome, após recuperação autorizada t43 às17:04: código real de catálogo/ledger candidato, bytesstreaming privados e planner retenção existem em `/srv/filuna-new-site`, **173 testesPASS/0FAIL =148baseline+25novos**. As afirmações abaixo de bloqueioSSH/ausência total de implementação são checkpoints HISTÓRICOS, superados somente nesta subfatia. Não foi publicada primeira versão integrada, nem criada confiança operacional;006 só em DBtemporário, processo operacional NÃO reiniciado e domínio estático intacto. [Evidência, rotas efetivas xcarregadas e limitações](WEB-FASE-05-IMPLEMENTACAO-EVIDENCIAS-20261108.md). GatesG2/G3BLOCKED,G4NOTRUN,G5/G6BLOCKED permanecem; nenhuma homologação/assinatura/certificação/updaterWindows inferida.


**Estado: PROPOSTO / IMPLEMENTAÇÃO E TESTES NOT RUN.** Não é endpoint disponível, migração aplicada, release publicada ou assinatura verificada. Código privado existente fases01–04 não foi alterado nesta fatia: leitura remota bloqueada por canal SSH existente sem resposta; nenhum caminho alternativo de acesso tentado. Não criar segunda árvore/backend. Esta especificação fica na box para integração posterior aos documentos canônicos `/srv/filuna-new-site/docs/` quando acesso existente funcionar.

## Autoridade e fontes
- t36,08/11/2026 16:44–16:46 Europe/Rome: **resumo delegado, literal não fornecido neste checkpoint** — integração nativa Slicer/servidor desde primeira versão, manifesto/versão assinados, resumo de mudanças dentro do app, fechamento/reabertura seguros e histórico de TODAS versões/correções; retomada geral só depois da prova de integração.
- t37,mesmo intervalo: **resumo delegado, não transcrição literal** — primeiro download beta/admin, no máximo CINCO VERSÕES de instaladores, remover anteriores sem perder histórico; novo instalador/build só com necessidade real.
- t38,literal recebido: «a assinatira da priimeira versao sera feita pelo github conforme regras».
- Leitura direta read-only no commit `a8d8d4b43b921b835fac01f347afc0e09133ecd5`: `FILUNA/REGRA-MESTRE-QUALIDADE.md`46–60; `FILUNA/design/visual-c/fontes/documentos/UPD-W.captura.txt`14–32; `FILUNA/CONFORMIDADE-LEGAL-NORMAS-E-CERTIFICACOES.md`63–64/98CMP-015. Fontes espelhadas na box, não checkout modificado.
- Fonte adicional reportada pelo pai, não relida diretamente pelo executor: https://github.com/abcollicelli-web/filuna-slicer/blob/61339ae22ace4abe7899ce9d5b03b54d96ffa762/docs/UPDATE-RELEASE-POLICY.md — adendo04/11, cumulativas/notas acimaCentralajuda/CONFIRMAÇÃO antes instalar/reabrir6–14; ECDSA+SHA25630; code+manifest signatures classeC24; ficha/hash/QAWindows/approve40.
- GitHub attestation é proveniência/build; NÃO substitui assinatura Windows/identidade Filuna CMP-015 nem manifesto ECDSA. Curva/hash/envelope ECDSA e trustroot operacional não foram escolhidos por inferência. Ed25519 dos grants não é release trust. T38 não revoga essas exigências.

## Dependências e fronteiras
Um módulo privado canônico integrado à identidade/RBAC/BFF/storage existentes; nada estático público, nenhuma cópia de beta em public/assets. Primeiro verificar NEW-SITE/continuidade/mestre/stack atuais,148 baseline e handlers antes de código. Nenhuma migração001–005 repetida. Se006 realmente necessária: backup DBnovo, integridade/FK/ledger, adição/idempotência/rollback testados, sem apagar dados. Não criar/importar dados/roles/grants/chaves/certs reais nem fake release. Não Google/SMTP/pagamentos/CI/build/OS/nginx/SSH/DNS/proxy/cofre/legado/desktop/hardware.

## Registro imutável proposto
`release_id`, versão canônica única, sequence monotônica, schema/contract version, canal owner-beta/homologação/estável (estável bloqueado nesta fatia), SO/arquitetura, classeA/B/C/D, compat cliente min/max e dependências, notas cumulativas/versionadas, tamanho e SHA256 EXATOS de cada artefato, licença/origem, impacto/restart/consent/rollback, installer_necessary e justificativa verificável C/D, referências GitHub repo/commit/workflow/run/artifact/attestation, manifesto ECDSA e keyid/versão do trust, evidências de code-signingWindows, owner approval ligada ao digest e QAWindows. Metadata aceita torna-se imutável; correção gera nova revisão/evento, não reescreve anterior. Estado de artefatos e ponteiros é separado do histórico; tombstone/pruning não apaga notas/hash/auditoria.

Referência de assinatura não equivale a verificação. Resultados de verificação têm que ser produzidos no servidor por verificadores legítimos e vinculados a bytes/digest/identidade/origem/contrato, nunca flags `verified=true` enviadas pelo cliente. Sem cadeia confiável/configuração ou prova válida: publicação/catálogo/bytes negados. Curva/hash/envelope não resolvidos ficam explícitos como BLOCKED e não são substituídos silenciosamente por ES256/EdDSA/qualquer algoritmo. Nenhuma fixture vira trustroot/release operacional.

## API privada proposta, ainda NÃO registrada
Namespace versionado `/api/v1/releases/`: catálogo/histórico autorizado, manifesto porrelease, bytes porartefato; administração de ingestão/validação/planejamento de retenção. Os nomes finais dependem de revisão do roteador existente. Defaultdeny visitante/comum/`beta=true` de cliente/claimsroles não verificados. Beta requer concessão legítima server-side escopada; administração requer RBAC durável+MFA conforme gate já existente. Não inventar provider/conta/permissões para abrir UI. Separar conta/tenant, identidade BFF/Bearer, CSRF das mutações cookie; negar credenciais ambíguas. Contrato de release não concede entitlement premium.

## Bytes e streaming
Raiz privada DEDICADA a releases, nome opaco seguro, não storage do cliente. Não alterar limite atual128MiB do endpoint storage. Candidato ZIP reportado168864208bytes supera esse teto e EXE não medido; não baixar inteiro em memória, não publicar candidato SHA2dace0d não auditado. Primeiro projetar ingestion/response em blocos bounded, limite transporte release explícito/configurado, Content-Length real versus declarado, hash incremental, arquivo regular O_NOFOLLOW/dirfd, traversal/symlink negados, fsync+commit e recuperação de staged/orphan. Sem zip-extract/execução/modelagem no servidor. Byte delivery exige revalidar identidade+gate+compatibilidade+licença+manifest/code/provenance trust e conferir hash bytes antes do primeiro byte autorizado; usar FD do mesmo inode verificado, não path reaberto vulnerável a troca. Range/ETag só se implementados e testados; não prometer CDN. Não produzir ACK durável antes do commit. Dependências maduras pinadas/hashes só na venv desta área, sem instalar OS.

## Cinco VERSÕES, não cinco arquivos
Contar versão lógica com todos seus artefatos SO/arquitetura como UMA; não criar cinco por canal disfarçando o máximo global. Todo histórico/notas/SHA/correção permanece mesmo sem binários antigos. Plano de pruning transacional só de objetos releases, nunca backups/storage usuário. Pointers ativos/rollback e downloads em andamento impedem descarte inseguro. Se limite e pins conflitam: bloquear promoção/ingestão que excederia limite e reportar, não remover pinned à força nem chamar seis de cinco. Não prometer transação atômica filesystem+SQL: estados staged/verified/published/retiring/purged+intent/recovery, falhas entre fsync/commit/unlink e concorrência devem ser ensaiados; leases/FDs impedem invalidar download em curso. Política precisa distinguir staging/ocupação física e publicação antes de ativação. Neste checkpoint **NENHUM artefato real deletado**, nenhuma alteração de ponteiro, nenhum pruning operacional: só contrato/plano.

## Aplicativo e segurança de instalação
Resumo/cumulativas/classes/impacto/restart/notas dentro do app; confirmação ANTES do início, salvamento/fechamento seguros; automatizar ciclo somente após condições satisfeitas. Não forçar kill/fechar projeto não salvo, não instalar C/D silenciosamente. Compat min/max, anti-replay/downgrade e rollback explicitamente autorizado; cliente revalida assinatura/hash e registra estado, não confiar versão reportada como autoridade de licença. PresetUpdater/UpdateDialogs bundles/perfis/plugin não comprovam atualizadorEXE/helper/save/reopen; MsgUpdateSlic3rUI TODO não é funcionalidade. Integrar/ensaiar nativo ainda NOT RUN, sem alegar compatibilidade do instalador atual. Básico Orca/offline/arquivos próprios preservados.

## Estado observável e retomada
FASE04 histórica148 PASS, zero dados/config ausente naquela captura. Nesta fatia canal remoto não respondeu à leitura inicial nem printf limitado, apesar de `-O check` responder Master running pid218374 localmente. Canal de leitura encerrado, mestre preservado; sem alteração de chaves/known_hosts/acesso. Estado atual servidor/DB/processo NÃO REVERIFICADO. Especificação/testes só na box, não servidor. Restabelecer o mesmo canal/sessão existente é a única necessidade imediata; não pedir senha/chave em chat. Depois ler mestres atuais antes de qualquer mutation e entregar primeira subfatia verificável sem ativar produção.

## Plano de testes proposto — todos NOT RUN
- REL01: unconfigured trust/auth denies catalogue/bytes/admin — **NOT RUN**.
- REL02: visitor/common/client beta flag denied — **NOT RUN**.
- REL03: valid trusted beta can read only authorized channel — **NOT RUN**.
- REL04: admin requires server RBAC and MFA; BFF without MFA denied — **NOT RUN**.
- REL05: cross-account/tenant and forged roles denied — **NOT RUN**.
- REL06: cookie mutation CSRF/sameorigin; Bearer ambiguity denied — **NOT RUN**.
- REL07: immutable metadata, duplicate ID/version/idempotence/CAS conflicts — **NOT RUN**.
- REL08: ECDSA invalid/wrongroot/algorithm/keyid/revoked/expired proof denied — **NOT RUN**.
- REL09: GitHub provenance alone cannot satisfy manifest or Windows code signing — **NOT RUN**.
- REL10: GitHub workflow/repo/commit/digest identity mismatch denied — **NOT RUN**.
- REL11: hash bytes differs signed descriptor: no bytes/ACK — **NOT RUN**.
- REL12: declared size differs actual; bounded large stream and abort — **NOT RUN**.
- REL13: traversal/symlink/FD replacement/hostile names denied — **NOT RUN**.
- REL14: compat min/max/dependencies/class/notes/installer rationale missing denied — **NOT RUN**.
- REL15: replay/downgrade rejected except separate signed authorized rollback — **NOT RUN**.
- REL16: five versions across multiartifact/channel, all documentary history preserved — **NOT RUN**.
- REL17: active/rollback pin blocks pruning; concurrent download completes safely — **NOT RUN**.
- REL18: pruning cannot address user backup/storage roots; no real deletion — **NOT RUN**.
- REL19: crash after fsync/beforecommit/aftercommit/unlink recovery consistency — **NOT RUN**.
- REL20: aditive next migration backup/integrity/idempotence/rollback without reapplying005 — **NOT RUN**.
- REL21: baseline148 reexecuted; home/static/frontend bytes unchanged by backend — **NOT RUN**.
- REL22: native Windows confirmation/save/helper/reopen/rollback actual integration proof separate — **NOT RUN**.

## Adendo do coordenador — pedidos literais e autenticação

As transcrições literais t36/t37/t38, conferidas com as mensagens do proprietário, estão no [registro de autorização](approvals/ATUALIZACAO-NATIVA-BETA-T36-T38-20261108.json). Os resumos anteriores permanecem identificados como resumos, não substituem esse registro. A nova tentativa SSH normal ao mesmo host, com known-host estrito, alcançou o servidor e retornou necessidade de autenticação; a sessão antiga foi preservada. Recuperar autenticação legítima antes de ler os mestres atuais e implementar; nenhuma mudança de chaves/configuração nem credencial em chat.


## Continuidade executável — Fase05.1 privada (08/11/2026 Europe/Rome)
T36–t38 literais continuam no [registro autorizado](approvals/ATUALIZACAO-NATIVA-BETA-T36-T38-20261108.json); backend agora implementou primeira subfatia REAL, não outra proposta: ledger imutável de candidatos/história, ECDSAcapacidade de verificação, streaming privado por64KiB, autorização tenant/conta/beta/adminMFA/licença e plano retenção5VERSÕES global/pins/leases/snapshotconflito. Nenhum executor de pruning/publicação, nenhum artefato real removido/importado/assinado. ECDSAcurva/envelope/raiz NÃO escolhidos operacionalmente; testes P256/SHA256/DER são só capacidade. Adaptadores GitHubprovenance/Windowscode/ownerQA apenas fixtures em memória, não provas reais.

173PASS de unidade/integradosHTTP/cripto não equivalem aos22 requisitos globais aceitos: matriz19PASS estritamente escopados/3NOTRUN com subcasos incompletos explícitos. NativeWindows/confirm/save/helper/reopen/rollback, cadeiaGitHub/Windows e recovery/promotion/pruning reais continuamNOTRUN. Auth/trust config AUSENTES; DBoperacional intacto1–5/integridadeFK/zero contas e catálogo, tabela006 não existe nele. Código novo NÃO carregado no PID2089150, nenhum restart/deploy; domínio/Nginx/publict35 preservados. Antes de aplicar006 futuramente exigir backupDBnovo/revisão/coordenação. Não executar --migrate-only cegamente.

[Arquivos/hashes, testes, contratos, matriz e próximas lacunas](WEB-FASE-05-IMPLEMENTACAO-EVIDENCIAS-20261108.md). Nada de cofre/legado/OS/SSHconfig/CI/build/pagamentos/GoogleSMTP/hardware. Operação atual permanece fechada e catálogo real vazio, básicoOrca/offline e arquivos próprios preservados.
