# Filuna Slicer — atestação GitHub do candidato de QA

08/11/2026 — Europe/Rome

**Resultado: compilação Windows e atestação GitHub aprovadas. Candidato de QA; não é uma beta liberada.**

| Verificação | Resultado |
|---|---|
| Compilação completa Windows x64 | PASS |
| Testes nativos Linux e Windows | PASS |
| Atestação de procedência emitida pelo próprio build | PASS |
| Verificação independente da atestação pelo GitHub CLI | PASS |
| SHA-256 do pacote conferido pelo código nativo do Filuna | PASS |
| Instaladores NSIS/MSIX | Não gerados; etapas desativadas e confirmadas no job |
| Assinatura Windows local | Adiada pelo proprietário |
| Interface executada no Windows habitual | Não executada |
| Salvar, fechar, atualizar, reabrir e recuperar a versão anterior | Não ensaiado |
| Download beta no site | Não publicado |

Revisão exata: `c90f88c0d91e464cb668efcb8170279dc887c0bd`.

Pacote: `OrcaSlicer_Windows_V2.5.0-dev_x64_portable.zip`; **212238636 bytes**. O nome V2.5.0-dev vem do build upstream e não define uma versão comercial Filuna aprovada.

SHA-256 do ZIP original atestado:

`abc8b4823ff2ce459d9535262fb0e1bc2ead40b7fb8ab85a96c18473584922e7`

[Build e atestação no GitHub](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42511597052) · [Testes nativos Linux/Windows](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42507970589) · [PR #7](https://github.com/abcollicelli-web/FilunaSlicer/pull/7)

A verificação exigiu repositório, revisão de origem, revisão do workflow signatário e branch exatos, com executor hospedado pelo GitHub. O certificado identifica `build_orca.yml` como signatário e registra a execução 42511597052. A atestação SLSA de procedência não é a assinatura Authenticode do Windows; essa etapa local continua adiada, conforme solicitado.

As dependências da execução anterior foram concluídas e reaproveitadas; sua reconstrução foi pulada neste build. O build antigo foi cancelado depois de preservar esse trabalho. Nenhum instalador foi publicado e nenhum projeto do proprietário foi aberto.

## Limitação de execução observada

O código herdado chama `associate_url(L"orcaslicer")` no início do editor (`GUI_App.cpp:3486`). A implementação (`GUI_App.cpp:10408`) grava o protocolo no registro do Windows mesmo com `--datadir` separado. Por isso a interface deste candidato não foi aberta no Windows habitual do proprietário. Esse comportamento exige revisão e teste em Windows descartável antes da beta. Não houve teste de interface nem instalação operacional nesta rodada.

## Integração e documentação no servidor

A integração operacional do atualizador, a confiança ECDSA do manifesto e o auxiliar de atualização/recuperação ainda não foram comprovados. O serviço privado de versões não foi ativado nesta etapa. Não houve alteração de banco, reinício de serviço, publicação de release ou remoção de versões no servidor.

A conexão atual permite somente leitura em `/srv/filuna-new-site/docs`. O registro canônico no servidor permanece pendente de acesso de gravação a essa pasta; as evidências estão preservadas no repositório e neste comprovante. Não se usou o site antigo como destino alternativo.

Este documento não concede aprovação do proprietário para release e não substitui os gates de qualidade e distribuição da primeira beta.
