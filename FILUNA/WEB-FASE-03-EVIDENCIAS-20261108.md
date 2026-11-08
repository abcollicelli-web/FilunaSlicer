# Fase03 — evidência real de objetos/versionamento/restore/bytes

08/11/2026, Europe/Rome. Base documental `5fc3438d5204fbf9e7f4a4d0b7129b9acb4aa82a`; somente documentação pública, código privado permanece na área nova.

## Execução comprovada
- 111 testes PASS / 0 FAIL: 76 baseline + 35 novos; `.venv/bin/python -W error::ResourceWarning -m unittest discover -s tests -v`. Sem warnings; pip check PASS, dependências fase02 intactas.
- HTTP e Ed25519 reais com chaves efêmeras SOMENTE em memória e DB/objetos sintéticos temporários. Upload/download retornam bytes reais conferidos, Content-Digest/SHA256/tamanho; não só existência de metadata.
- Negativos: consentimento/hash/tamanho/tipo/framing/aborto, limite técnico, traversal/encoded path/symlink/hardlink, cross-account mesmo tenant/cross-tenant, papel/conta desabilitados, revogação DURANTE body antes do commit, CAS concorrente entre PCs e idempotência com payload alterado.
- Crash antes do commit reporta órfão sem falso ACK; perda de ACK após commit recuperada por retry sem duplicar versão. Falha de fsync injetada não confirma metadata; objeto comprometido/ausente bloqueia entrega/restore/retry. Não é teste real de falta de energia.
- Restore explícito cria revisão nova sem apagar versões; suspensão não destrói dados próprios. Snapshot privado DB+objetos foi copiado/restaurado em diretório isolado de teste; DB/objeto adulterados e snapshot interrompido rejeitados. Migração propositalmente ruim revertida, preservando versões existentes.
- Download de catálogo: publicação/JWS/engine/compatibilidade, licença e grant premium independentes; assinatura/chave revogada, licença ausente, suspensão, bytes/binding adulterados ou publicação draft bloqueados. Recurso livre ainda exige licença correspondente, não assinatura paga. Nenhum modelo/release real publicado.

## Arquivos reais e estado
Novos `service/object_store.py`, `service/storage_api.py`, `service/snapshot.py`, `migrations/004_private_objects.sql`, `contracts/object-storage.v1.json`, `tests/test_object_storage.py`; integração em app/private_api, OpenAPI/índice/bindings dos módulos. Mestre local/NEW-SITE/ledger/continuidade atualizados. `runtime/objects` privado e vazio; contas/identidades/versões/bindings operacionais zero. Migração 004 já aplicada após backup exclusivo do DB novo, 001–003 intactas; integridade/foreign keys ok. Não reaplicar/instalar cegamente. Processo próprio www-data loopback18765 atualizado; smoke normal privado 503, arquivo interno 404, home/central 200 e hashes visuais iguais.

## Limitações materiais
Filesystem+DB não são transação distribuída: órfãos são reportados sem GC automático; falta/corrupção referenciada bloqueia serviço. ACK após fsync+commit não é offsite/DR ou RPO/RTO. Snapshot é operador local privado, nunca endpoint para cliente baixar DB multiusuário; não restaura sobre DB ativo. Sem streaming/resume/range/ETag/CDN; teto técnico inicial, não quota/plano. Firmware só comparação literal `==...`; ranges complexos negados. Proveniência é a do manifesto assinado/aprovado, não auditoria externa de licença/autor. Declaração de compatibilidade não é hardware atestado.

Login email/Google/browser, trust legítimo e signer continuam NÃO configurados/implementados nesta fatia, sem usuários reais ou concessões emitidas; decisão UX/provider tratada pelo coordenador. Sem Google/SMTP/pagamentos/serviços externos criados. Primitivas reais testadas não significam serviço comercial pronto. Clientes desktop, instalação, geração/fatiamento/hardware não retomados; básico Orca livre/offline preservado. Sem novas quotas/retenção/janelas comerciais.

## Estado visual t34
Pedido literal nos dois mestres e registro de revisão. Nenhuma alteração de home/site.css/account.css/client_portal.py; frente DESIGN distinta prepara candidato, sem segundo backend. Home/central NÃO homologadas após crítica posterior. CU anterior navegou export offline e cinco áreas/acesso, desktop sem recortes inesperados vistos; sem teste de dados privados/mobile/HTTP vivo/100%. Screenshots pertencem ao coordenador; não são prova de conformidade do backend.

## Índice SHA256
| Arquivo na raiz nova | SHA256 |
|---|---|
| `service/object_store.py` | `3425f235374c5ec8b5102971bc9ff56f4b90b652822a0f9851c08c99647c8e86` |
| `service/storage_api.py` | `7adaaaa36385551d84b50815f22c811676faeb79c8636371d59bbc68a204b1be` |
| `service/snapshot.py` | `9cf7db9631d158cdab27ae0c92c8fe4f072ed10d11346e282d286f38cc13ee97` |
| `service/app.py` | `118228c6e32243624726275eb00c7071fd239b4e0c91f3086c06ff6444f566fa` |
| `service/private_api.py` | `d0836d1efa23c6732ffccee0c876619cb7c2e9d57fb364a7a95060dde9fc3a6e` |
| `migrations/004_private_objects.sql` | `af632d3cc48fc744efd3c1da56e7ae27b568143a3312da7d5020613e6c591499` |
| `contracts/object-storage.v1.json` | `3152f78ea4f43af4c77b5cf1cc1bf71c863272b98714d29a388256950b505020` |
| `tests/test_object_storage.py` | `6f11ad3228ccda44ffafef815ef4273074b4097676b2c0a760dc6ae7ace0ee2b` |
| `docs/evidencias/fase03-tests.txt` | `fc9ff3b9fd17f8d8b62fc891a91c1344effc73948776e22e99803394832d1a79` |
| `docs/evidencias/RESULTADOS-FASE-03.json` | `d5ec4f7c4574dc8225f761b02205c8ce948aeaa542f95a40106a00326e786e2b` |
| `docs/evidencias/MANIFESTO-FASE-03.json` | `32e3d1eb683b0f081651963074d24899538d2627fead635681fa4984e102339f` |

Manifesto local fase03: 67 arquivos, exclui runtime/venv/cache/self; snapshots de fases anteriores preservados historicamente. Nenhuma leitura/varredura/hash global do cofre nesta fase. Próxima subfatia: integração de identidade legítima após decisão/configuração segura; UI após revisão DESIGN; uploads grandes e recuperação externa somente com testes/políticas explícitas.
