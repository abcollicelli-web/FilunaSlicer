# Filuna — atualizador nativo da primeira versão

**Estado: PRIMEIRA FATIA C++17 + CI. Sem conexão HTTP ativa, helper, reinício ou instalador assinado. Não é um beta liberado.**

O servidor novo em /srv/filuna-new-site já tem contratos de releases.v1 e release-lifecycle.v1, testados com 196 PASS somente em instalações temporárias. Ainda não há raiz de confiança operacional nem migrações 006/007 em produção, logo nenhum download beta deve ser anunciado como disponível.

NativeUpdatePolicy.hpp protege a decisão de uso após evidência independente de identidade, canal beta, manifesto ECDSA, SHA-256, procedência GitHub, Authenticode, consentimento/QA e compatibilidade. Os bools de TrustEvidence NUNCA são preenchidos com campos do JSON recebido: dependem de validadores confiáveis vinculados aos bytes reais. Com confiança ausente a decisão é BLOCK.

Classes: A dados sem instalador; B módulo compatível versionado; C pacote cumulativo de código/UI Filuna, com assinatura Windows, salvar e reiniciar com consentimento; D somente quando o motor/runtime exigir instalador tecnicamente justificado. Checagem ao abrir e a cada 30 minutos, aviso acima da Central de ajuda, novidades legíveis, proteção de tarefas e projetos abertos, rollback e reabertura. Orca offline e dados locais preservados.

No servidor: máximo CINCO conjuntos físicos de versões globais, incluindo staging/trash, protegendo versão ativa, rollback, pins e downloads; histórico, notas, hashes e QA conservados para sempre. A implementação do servidor é apenas isolada até trustroot, migração e provas de release.

Próxima integração obrigatória: transporte HTTPS/BFF autenticado + verificadores reais, UI nativa Grok/UPD-W, helper Windows assinado (download, staging, atomic swap, shutdown gracioso, restart/rollback), teste de falha com projeto não salvo, perda de energia, UAC, base cumulativa e offline; então aprovação explícita de primeira beta/admin no site. Nada foi publicado ou alterado no servidor nesta fatia.

## Camada de leitura HTTPS nativa (em teste)
`NativeWinHttpReleaseClient.hpp` consulta o catálogo privado `/api/v1/tenants/{tenant}/releases` por WinHTTP/Schannel (TLS obrigatório) com Bearer fornecido pela sessão, sem seguir redirecionamentos, sem IP fixo, sem execução de comandos, sem chamada quando token/tenant for inválido, timeout e limite de 128 KiB. Resposta HTTP 200 continua **não verificada** até ECDSA/hash/procedência. O servidor ainda não ativou o loader de confiança e o cliente não está integrado à GUI; testes não fazem tráfego real nem usam credenciais.

## Sequenciamento de atualização (C++17, QA)
`NativeUpdateFlow.hpp` adiciona os estados: oferta verificada, confirmação explícita em C/D, download permitido, pacote validado, aguardar salvamento/tarefas, helper confiável, recibo e reabertura confirmados, ou pedido de rollback. Esta classe **não** executa nenhuma ação de I/O nem substitui o mecanismo Windows: todos os sinais de assinatura/hash/save/ACK ainda dependem de verificadores e executor reais. Os testes cobrem ausência de manifesto, ofertas sobrepostas, recusa de usuário, trabalho não salvo, tarefa ativa, falta de snapshot, helper sem assinatura, recibo incorreto e progressão segura. Um novo processo não herda seu estado: journal/restore operacional continua obrigatório.

## Integração GUI, primeiro gate
`MainFrame::generate_help_menu` oferece a entrada **Atualizações Filuna**; até existir confiança operacional mostra aviso honesto, sem downloads. No startup, o aplicativo deixa de consultar automaticamente o feed **Orca upstream** como se fosse distribuição Filuna. O motor Orca e o código de fatiamento são preservados. A consulta Filuna autenticada na abertura e a cada 30 minutos só será ativada após trustroot, licenças, sessão segura, manifesto e helper; compilar o GUI é obrigatório, fonte estática não comprova funcionalidade.
