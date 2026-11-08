# Fase05.2 — lifecycle de releases privado, ensaiado somente em TEMP

Base documental: `843a1277b6d2f2e83b122abdf4c6d7be7a37be7f`. Registro aditivo de08/11/2026, Europe/Rome. NÃO publicação/ativação de instalador.

## Autoridade e precondições

Ordens originais [t36/t37/t38](approvals/ATUALIZACAO-NATIVA-BETA-T36-T38-20261108.json), com acesso autenticado recuperado t43, continuam sendo a autoridade. t37 inclui: «e ja colocar a 1 versao para download apenas para beta e administrativo no site e cada versao deve ir para o site com limite de 5 versoes removendo as anteriores, update de novos instaladores e compilaçao serao criadas somente se houver necessidade de compilar uma versao nova». t38: «a assinatira da priimeira versao sera feita pelo github conforme regras». O pedido nativo t36 exige confirmação e salvamento seguros antes do ciclo automático; essa integração Windows não foi implementada/provada por testes backend. Não há nova aprovação inferida da publicação documental05.1.

A regra legal exige assinatura E attestation: GitHub provenance não substitui assinatura Windows/identidadeFiluna nem ECDSA de manifesto. Curva/envelope/raiz operacionais permanecem pendentes; nenhum trustroot/certificado/chave operacional criado, nenhum CI/build disparado. Core Orca/offline/básico, arquivos e backups próprios não são objeto desta retenção.

## O que foi implementado

Biblioteca interna `service/release_lifecycle.py`: `plan(rid,headers)`, `promote(rid,sources,expected_snapshot,headers)`, `recover(op,headers,mode=abort|resume)`. Revalida identidade legítima, cadastro/roleadmin server-side e MFA; valida manifesto e os quatro domínios de prova separados. Não há mutador HTTP novo, loader operacional, conta/grant real ou parâmetro cliente que autorize poda. Uma chamada biblioteca não equivale a serviço já carregado.

Migração aditiva007 somente em DBsTEMP: rootbinding/generation/highwater, estados/heads, journal e audit. Intenção server-generated com campos imutáveis e SHA256 de integridade, não assinatura e não proteção contra DBadmin comprometido. Triggers negam novo pin/lease/presença para versão managed não publicada/retiring. Históricos imutáveis de006 não são removidos.

Namespace dedicado `runtime/.../release-lifecycle`, UUID+device/inode+markerprivate vinculados ao DB, lockinode verificado, mutex por instância e flock entre instâncias/processos. FDs/O_NOFOLLOW e arquivos hash esperados; rejeita traversal, symlinks, hardlinks, namespace de clientes, entradas nãojournaled e alterações de raiz/lock. Nunca interpreta comandos presentes em notas/artefatos.

### Cinco versões físicas e o compromisso explícito

O teto é global entre canais/arquiteturas e conta VERSÕES, não arquivos. Inclui objects/staging/trash, para não esconder sexta versão em staging. Primeiro source pass em64KiB verifica bytes/hash/tamanho sem persistir o candidato. Sob plano snapshot atual e locks, grava intenção durável, marca vítimas retirando/presença0, move e remove somente blobs esperados de versões nãoactive/rollback/pin/lease. Só depois escreve segunda passagem64KiB em staging, reverifica hash/tamanho, fsync+rename e commitSQL de publicação/presença/head/highwater/pins; ACK apenas após commit. Não muda limite128MiB do armazenamento de clientes nem lê instalador inteiro em memória; bound de release é explícito e server-side.

Se cinco versões estão protegidas, a sexta é negada antes da remoção. Se a segunda passagem/fase posterior falhar após poda, podem restar quatro versões: os bytes já podados não são prometidos como recuperáveis. Active/rollback e toda história, correções, QA, notas, fonte e hashes permanecem. FS+SQL não são um único recurso atômico.

### Recuperação testada

Journal server-side atual, rootbinding e adminMFA fresco são obrigatórios. Abort primeiro libera bytes do candidato e restaura vítima apenas se todos os bytes ainda existirem íntegros; vítima parcial já removida vira pruning escopado consistente, sem apagar história. Resume exige candidato completo já staged/renamed e verificado, nunca inventa fonte ou buscaURL. Póscommit apenas finaliza, sem repetir poda. Segunda recuperação é idempotente. Lease de reader morto fica conservadoramente como proteção até revisão, sem TTL comercial inventado. Unknown journal/entrada/integridade nega e exige revisão, não executa rmtree genérico.

`service/releases.py` ganhou mapper do layout porversão e rechecagem de presença dentro da transação que cria lease, fechando corrida com retire. `tests/test_releases.py` mantém o mesmo teste de migração inválida usando próxima versão dinâmica para evitar colisão com007; nenhum guard foi desabilitado.

## Provas executadas e falhas

**196 testes PASS,0FAIL**,18.892s no último log de código;173baseline+23novos. Baseline173 também reexecutado antes da implementação,14.323s. [Matriz nominal23](evidencias/fase05-2-private-lifecycle-20261108/MATRIZ-FASE-05-2.json).

- Seis versõesTEMP reais com dois artefatos/versão: cinco diretórios/10blobs retidos, seis históricos íntegros. Múltiplos canais, cinco pins bloqueando promoção, active/rollback/leases preservados.
- Plano stale, readers/pins/publishers concorrentes e threads na mesma instância; busy409/replan, replay/downgrade de servidor negados. HTTPloopback real com identidade criptográfica TEST e bytes do layout lifecycle; lease protege, após poda bytes antigos negados.
- Dez subprocessos encerrados realmente por `os._exit(91)`: oito fronteiras intent/rename/unlink/prune/stage/renamecandidate/commit, mais dois resumes de candidatos completos. Não são testes de corte físico de energia. [Oito resultados abort/finalize brutos](evidencias/fase05-2-private-lifecycle-20261108/fase05-2-real-crashes.json); os dois resumes constam no teste16 e matriz.
- Falha real em unlink, fsync após rename e triggerSQLabort emCOMMITTED, comprovando ausência de ACK falso, rollbackSQL de heads/presença e retomada; sources alterados entre passes; troca symlink de staging depois de prune; root/marker/lockinode/hardlink e helperdestrutivo sem intenção atual negados. Alvos de clientes de TEST ficaram intactos.
- Falha inicial de preparação: SyntaxError no script de upload, antes de escrever arquivos novos/backup; checagem confirmou ausência desses arquivos e173PASS. Script corrigido e compilado, instalação guardada por hashes; primeiro conjunto191PASS, hardening/faulttests ampliou até196PASS. Logs intermediários preservados no espelho privado. Não foi necessário restaurar DB/fonte operacional.

ECDSA em memória é criptografia real efêmera deTEST. Adaptadores de GitHub/Windows/QA são **fixtures**, não validação de cadeia/proveniência/certificado real. Nenhum fixture virou release/config/usuário operacional.

## Estado operacional rechecado

08/11/2026 17:39 Europe/Rome: DB `preview.sqlite3` integrityok, FKsem violações, migrations1–5 e zero dados em todas tabelas operacionais;006/007 ausentes. Rootlifecycle e auth/securityconfig ausentes. PID2089150 UID33 permanece, nenhum restart/reload. Quarenta e seis arquivos protegidos comparados porSHA256 antes/depois: app/domain/auth/security/BFF/storage/snapshot/clientportal, frontendpublic/admin e migrações001–006 intactos. Nginx/OS/SSH/proxy/legado/cofre não inspecionados nem alterados nesta fase. Backups criados somente dos dois arquivos-fonte alterados na área nova; nenhum ciclo real de importação/publicação/remoção/restore de instalador.

## Artefatos e hashes privados

Espelho final: `/workspace/filuna-web-fase05-2-20261108/final-mirror/`. Manifesto completo23arquivos SHA256: `b75ef4522e4cffb996ab4ff649828069bd6e2171dd4d2911345b51df8e46e38f`. Fonte/testes/contratos/docs/evidências são privados; patch público contém só documentação/evidência sanitizada, não implementação. Sem novo pacote/dependência; pipcheckPASS. RuntimeSQLite/devHTTP continua preview, não serviço comercial/DR.

| Arquivo privado | SHA256 |
|---|---|
| `service/release_lifecycle.py` | `5cf7b25bf2e183c6c3580dd8d723fb0f8a5ea8091bd29c61e4483956c6c9d4d2` |
| `service/releases.py` | `ce5ece14a881457b912b50cbd3ea79717733eaa33427f27d43ff0cae1b57e94d` |
| `tests/test_release_lifecycle.py` | `0e0709794217f607ebfd9295faf7a2286e06fb47b76ca8828f23c3dc50ab0eeb` |
| `tests/test_releases.py` | `8de874d47d594bf2519acc82009e7e3a0e4fdd0b60b6fef526cd42f02187ad91` |
| `migrations/007_release_lifecycle.sql` | `699545d7fb384333d775cb718dd9636b8f1eebb0314d4dfc221a4aa8edec211b` |

## NOTRUN / bloqueios e próximo passo

Nenhuma migração006/007 ou loader operacional, nenhum mutadorHTTP/config real/restart/publicação/import/deleção de instalador. GitHub/Authenticode/identidadeFiluna/QA reais, perfilECDSA operacional, corteenergia, DBprodução/escala/DR e Windowsupdater/helper/save/consent/close/reopen/rollback NOTRUN. G2/G3BLOCKED,G4NOTRUN,G5/G6BLOCKED permanecem. Somente após confiança/contrato/config legítimos e rollout coordenado poderá haver ativação; não inferir autorização daqui. A prova Windows e autorização real de primeira distribuição são frentes independentes, não satisfeitas pelo backendTEMP.
