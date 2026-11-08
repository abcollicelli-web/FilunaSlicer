# Filuna — atualizador nativo da primeira versão

**Estado: PRIMEIRA FATIA C++17 + CI. Sem conexão HTTP ativa, helper, reinício ou instalador assinado. Não é um beta liberado.**

O servidor novo em /srv/filuna-new-site já tem contratos de releases.v1 e release-lifecycle.v1, testados com 196 PASS somente em instalações temporárias. Ainda não há raiz de confiança operacional nem migrações 006/007 em produção, logo nenhum download beta deve ser anunciado como disponível.

NativeUpdatePolicy.hpp protege a decisão de uso após evidência independente de identidade, canal beta, manifesto ECDSA, SHA-256, procedência GitHub, Authenticode, consentimento/QA e compatibilidade. Os bools de TrustEvidence NUNCA são preenchidos com campos do JSON recebido: dependem de validadores confiáveis vinculados aos bytes reais. Com confiança ausente a decisão é BLOCK.

Classes: A dados sem instalador; B módulo compatível versionado; C pacote cumulativo de código/UI Filuna, com assinatura Windows, salvar e reiniciar com consentimento; D somente quando o motor/runtime exigir instalador tecnicamente justificado. Checagem ao abrir e a cada 30 minutos, aviso acima da Central de ajuda, novidades legíveis, proteção de tarefas e projetos abertos, rollback e reabertura. Orca offline e dados locais preservados.

No servidor: máximo CINCO conjuntos físicos de versões globais, incluindo staging/trash, protegendo versão ativa, rollback, pins e downloads; histórico, notas, hashes e QA conservados para sempre. A implementação do servidor é apenas isolada até trustroot, migração e provas de release.

Próxima integração obrigatória: transporte HTTPS/BFF autenticado + verificadores reais, UI nativa Grok/UPD-W, helper Windows assinado (download, staging, atomic swap, shutdown gracioso, restart/rollback), teste de falha com projeto não salvo, perda de energia, UAC, base cumulativa e offline; então aprovação explícita de primeira beta/admin no site. Nada foi publicado ou alterado no servidor nesta fatia.

## Camada de leitura HTTPS nativa (em teste)
`NativeWinHttpReleaseClient.hpp` consulta o catálogo privado `/api/v1/tenants/{tenant}/releases` por WinHTTP/Schannel (TLS obrigatório) com Bearer fornecido pela sessão, sem seguir redirecionamentos, sem IP fixo, sem execução de comandos, sem chamada quando token/tenant for inválido, timeout e limite de 128 KiB. Resposta HTTP 200 continua **não verificada** até ECDSA/hash/procedência. O servidor ainda não ativou o loader de confiança e o cliente não está integrado à GUI; testes não fazem tráfego real nem usam credenciais.
