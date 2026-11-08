# Fase02 web — evidência real e limites (08/11/2026, Europe/Rome)

Base documental: `6b597152c384db37767df70e45681dcd953cae8a`, branch documental `filuna/visual-c-20261108`; publicação deste adendo depende do coordenador. Implementação privada exclusivamente `/srv/filuna-new-site`. Este arquivo não contém código privado, credenciais ou dados de contas.

## Executado
- Baseline 33 testes reexecutados e revisados: primitivas/estrutura, não prova de autenticação.
- **76 PASS / 0 FAIL**: 33 fundação + 28 segurança + 15 central; `.venv/bin/python -W error::ResourceWarning -m unittest discover -s tests -v`, sem warnings remanescentes; `pip check` PASS.
- Chaves Ed25519 efêmeras geradas SOMENTE em memória; dados sintéticos em SQLite temporário. Assinaturas reais positivas/negativas, adulteração, none/HMAC, issuer/audience/expiry/nbf/iat/alg/kid/tipo, JSON duplicado, revogação/rotação, versão antiga, licença versus entitlement, MFA/admin e isolamento entre contas inclusive no mesmo tenant.
- Composer cliente e APIs privadas testados via HTTP real com fixture confiável efêmera; estado normal SEM configuração continua negando privado. Bearer permanece reutilizável até expiry/revogação; sem alegar DPoP/one-time ou renovação comercial implementada.
- PyJWT 2.15.1 + cryptography 50.0.2 + cffi 2.1.1 + pycparser 3.12 fixados com SHA256 em `requirements.lock`, venv só na raiz nova. Não é auditoria completa ou stack de produção aprovada.
- Migrações 002/003 aditivas no DB novo vazio, depois de backup exclusivo; 001 intacta; integrity_check ok e foreign_key_check vazio. Teste preserva dados sintéticos v1. Nenhum usuário, grant ou conteúdo operacional inserido.
- Smoke HTTP da prévia atualizada loopback: `/`, `/conta/`, `/conta/acesso/` e cinco seções + CSS 200; APIs privadas/admin 503 sem configuração, configuração privada 404. Processo próprio www-data atualizado sem alterar SSH mestre, Nginx/OS/tráfego/serviços terceiros. `public/index.html` e `admin/index.html` originais preservados.

## Arquivos efetivos no projeto novo
`service/security.py`, `service/private_api.py`, `service/client_portal.py`; integração em `service/app.py` e migração com checksum em `service/domain.py`; `requirements.lock`; `migrations/002_identity_authorization.sql`, `003_client_centre_metadata.sql`; `contracts/security-config.v1.schema.json`, `client-modules.v1.json`, OpenAPI e índice atualizados; `tests/test_security.py`, `test_client_portal.py`, fundação atualizada; `public/home.html` com entrada discreta e `public/assets/account.css` separado. Hero, site.css, logo, Inter e imagens preservados. Mestres locais, NEW-SITE, `docs/MIGRACOES.md`, `docs/CONTINUIDADE.md`, logs sanitizados e evidências atualizados. Não criar árvore pública paralela deste código.

## Funciona versus pendente
A central tem apresentação e fluxo de orientação reais, não link morto. Leitura de metadados próprios funciona com identidade verificada no teste; sem provedor/configuração, mostra estado honesto e não libera dados. SEM login interativo/sessão browser, signer operacional, emissão/renovação reais, checkout, cobrança, storage/sync/restore ou entrega de bytes. Manifesto é JWS verificado, `artifact_bytes_verified=false`/`install_allowed=false` na API; hash/tamanho de bytes exercitados apenas em testes. Distribuição/instalação atômica/rollback no cliente futuros. SQLite/devHTTP não promovidos a produção.

Core Orca livre/offline (t32), mesma engine; restrição só ao carregamento homologado premium, sem apagar dados ou direitos livres. Sem política offline/cache/preços/devices inventada. Cofre nesta fase não lido nem varrido, nenhum hash global alegado; somente os dois guias permitidos haviam sido lidos no marco anterior. Nenhuma retomada desktop/hardware/CI/G2–G6.

## Revisão visual e exportação
Revisão anterior coordenador/CU em Chrome GUI offline: topo/maquete/rodapé e âncoras, assets/Inter/logo vistos renderizados, sem recorte inesperado nos trechos vistos. Não mobile, não HTTP vivo, não 100%. Export anterior SHA `51e9c2efeaf65b08ad80ec969b7495240a4f14c7eddbf54b0af1a8bfb23fa27c` preservado. T33 aceita o visual com a adição solicitada, não a funcionalidade.

Novo export anônimo solicitado: oito HTMLs autossuficientes para home+central, CSS/assets/fontes embutidos e links canônicos mapeados entre arquivos locais. Sem conta real ou token. SHA home export `0f8b8e43baa608e66c08a87818ca95c6a133759f2a56ce604a620667379db6b2`; SHA central export `39aa7a8e95c24d8fa99849931280e6f44a5976dca4a70c14bee6b4a575b2776e`. Nova inspeção CU/mobile e aceite específico de estados ainda pendentes; não confundir export com login/serviço vivo.

## Índice de integridade desta evidência
SHA256 dos arquivos reais do servidor no checkpoint (não inclui seus conteúdos privados):

| Caminho na raiz nova | SHA256 |
|---|---|
| `requirements.lock` | `47353027c5e5d392ecd1352ad6affe4cd72e1092704b7ee9cf7e53988a30589a` |
| `tests/test_foundation.py` | `507972116149218d691e85140ae8238f883277bfc5facf7c1446540c34f431ab` |
| `tests/test_security.py` | `31ea7bb665d5764ac42b2a6808d74bbff0bf3d48974a069bb8ebee6aeaa99e72` |
| `tests/test_client_portal.py` | `4f00c2d629438a0f074ca706339cf5d9ab5e95d19d8b1aabb4e768e41ab1cef8` |
| `docs/evidencias/fase02-tests.txt` | `5936b3f5f710f0814e582b7c2287b36b6b3f965f6292cd2c752ff4da05e3f613` |
| `docs/evidencias/fase02-pip-check.txt` | `9261363b733079a641c2e4cc9bc46ffa1d8336945a87f807b6cf68847dbc9b09` |
| `public/home.html` | `ad6ead866d34539a2b9435362004fbb6fac4b3a5d16e1bd5f0c3e735609d3874` |
| `public/assets/site.css` | `622ca7dcda074eaa353be228899a205e7dd051fe42c47c4f986945061fe03d43` |
| `public/assets/account.css` | `ab981a95b805529d9e2155306af69f7f3e748d09f22f459e42e445c5735b272f` |
| `public/assets/filuna-original.png` | `4d6262bc1ee368e0c2f53b1b06a9cd0f1ad94cb530e893de93e0756b04293ef5` |
| `public/assets/Inter-VariableFont.ttf` | `0be2399ea925f1f83ff974764761da9860ec50742ed29a5d4c1ffd0c5c7ac3a8` |
| `public/assets/preparacao-C.png` | `9e0faadedf2e4dd89cc7e62ed26becc807d2e26e86288cae8b1ec59ed19d0fd6` |
| `migrations/001_foundation.sql` | `e7dae1c67f20746853a70ae31344411dde3a3b602e2036f81267e9a79eb6ad3e` |
| `migrations/002_identity_authorization.sql` | `8f37882b6554dc9e9b3307ed05a139a783b53e4e7d889b77d0c6b428bb55deef` |
| `migrations/003_client_centre_metadata.sql` | `16314a9e09a74c0240f78f9959912a79d1c627dc51a440988619882df2f13d8e` |
| `contracts/client-modules.v1.json` | `6d51154339131a486d32e14b79edbb711c2dee6688e2d7f6ffaf9db6c4f74f17` |

Ledger e manifesto fase02 locais excluem runtime/venv/cache e a própria lista para evitar hash autocircular; manifestos fase01 são snapshots históricos, não assertivas de igualdade após alterações posteriores autorizadas. Próximas etapas: captura nova home/central; integração segura de provedor autorizado + signer + sessão/BFF/provisionamento; storage e delivery com teste real por conta; política comercial pendente antes de renovação/offline. Nenhuma falta disso autoriza simulação de sucesso.

Checks finais da área nova: 17 JSONs parseados, links documentais internos PASS, 54 arquivos no manifesto; SHA manifesto fase02 `08c5f80e652261251b1b55f22a0282e5e17ade55bba5362facd4101ed779ed6f`. Resultado sanitizado SHA `f0bddedf42d23b52cb8d05af34046a15908ca786cff71272abea88d040c80d9a`. Teste restaura a home removendo exatamente as duas inserções autorizadas (link CSS complementar e link Central) e recupera SHA anterior; assets/CSS original intactos. Smoke do processo final confirma os mesmos hashes HTTP utilizados na exportação.
