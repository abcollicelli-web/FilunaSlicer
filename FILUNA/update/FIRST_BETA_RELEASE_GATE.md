# Primeiro beta Filuna — gate de assinatura e distribuição

**Status atual:** BLOQUEADO; pipeline de assinatura pronto como candidato de QA, não executado. Não publicar um pacote sem assinatura verificada, nem afirmar que a primeira versão está no site.

## Ordem de assinatura indicada pelo proprietário em 08/11/2026
Primeiro usar o GitHub; a assinatura pelo computador do proprietário fica para depois. `filuna-github-attestation.yml` implementa a etapa independente do GitHub, sem Azure, certificado local ou reconstrução: exige build GUI Windows concluído com sucesso no mesmo repositório/branch e SHA exato, seleciona um único artefato portable x64, confere o SHA-256 do arquivo e gera uma atestação de importação/integridade com referência ao build de origem. A própria atestação é verificada contra o repositório e o workflow signatário antes de guardar a evidência.

O predicado `filuna.github-artifact-import.v1` registra expressamente `authenticode=NOT_PERFORMED` e `release_approval=NOT_GRANTED`. Não é uma nova compilação, uma atestação SLSA emitida pelo builder original, uma assinatura Authenticode ou aprovação de publicação. O binário não é republicado por este workflow; somente a evidência é guardada. O workflow anterior de assinatura Windows permanece separado para quando a identidade legítima estiver disponível. Os gates de integração nativa, ECDSA, recuperação e distribuição continuam exigidos; adiar a assinatura local não fornece resultados desses ensaios.

## Verificações existentes
- GitHub Actions público suporta *artifact attestation* (prova de procedência) no plano Pro; isso **não** é Authenticode nem garantia de ausência de alertas SmartScreen.
- Assinatura Windows confiável é independente. O workflow usa Azure Artifact Signing somente após perfil, identidade de editor, OIDC/certificado e consentimentos configurados fora do repositório. Sem esses itens recusa a execução, sem usar certificado de teste para beta.
- O servidor novo possui implementação privada releases e lifecycle em TEMP com 196 PASS/0 FAIL; trustroot de produção, migrações operacionais 006/007, processo carregado e cliente Windows permanecem NOT RUN.
- O frontend existente /conta/downloads/ é prévia, **não uma publicação de instalador**.

## Regras que bloqueiam qualquer download beta
1. Evidência de primeira versão assinada e aprovada pelo proprietário para um SHA/versão exatos, registrada sob FILUNA/QA/first-beta-release-evidence.json; arquivo não existe neste PR (fail closed).
2. Teste Windows de GUI, motor Orca, abertura 3MF, projetos não salvos, atualização incremental, rollback e reabertura da mesma instalação, todos PASS.
3. Validadores reais de ECDSA do manifesto, atestação GitHub, Authenticode e identidade Filuna; evidências verificáveis de hashes e auditoria jurídica.
4. Serviço privado ativo e com autenticação e 006/007 aplicadas sob backup, plano de recuperação e reversão.
5. Beta restrito por autorização real do servidor, site com até CINCO versões físicas globais incluindo staging/trash, histórico completo de TODAS as versões/patches preservado. Sem binários em public/assets.
6. Assinatura e empacotamento C/D apenas quando classe exigir. Atualizações A/B não provocam rebuild nem instalador; C/D oferecem resumo acima da Central de ajuda, confirmação do usuário, salvamento, fechamento e reinício seguros.

**Nenhuma promoção G4/G5/G6 nem homologação está implícita.** O novo commit não faz merge, publicação, deploy, instalação, criação de chaves ou alterações no servidor.
