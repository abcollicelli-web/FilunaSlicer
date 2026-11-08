# FASE04 — autenticação dual implementada, desativada operacionalmente

Base documental: `9d3988065342e471a7b5db03762caf3764fdb397`. Fase03 preservada, não reaplicar004 nem patches anteriores. Data:08/11/2026 Europe/Rome.

## Decisão e fronteiras
Widget t33s249,15:52 Europe/Rome: «Quero login com e-mail e senha e também com conta Google.» Email/senha primeira parte E Google. Sem GoogleProject/contaexterna/SMTP/messages/pagamentos criados. T34 crítica visual15:57 foi tratada por frente separada; t35 «aprove e publique»16:19 aprovou candidata visual e publicação coordenada, NÃO ativação auth. Home/CSS/markup/client_portal preservados neste lote. Não afirmar deploy/visual100% a partir dos testes backend.

## Arquivos e runtime reais
- Novos `service/auth_bff.py`, `google_oidc.py`, `email_delivery.py`; `tests/test_auth_bff.py`37 testes.
- Integração existente `security.py`, `private_api.py`, `storage_api.py`, `app.py`: cookie BFF verificado OU Bearer, ambos juntos negados; mesmo principal interno/tenant/account e RBAC. Auth configurável separada do trust Ed25519; BFF não tem MFA/admin automático.
- `migrations/005_first_party_auth.sql` SHA256 `68630652cb68d3d843e4944e6cf338cf647367c48169c910b8a6429f36d24212`; backupDBnovo prévio,001–004 intactas; não reaplicar. Novas tabelas credentials/emails/epochs/sessions/one-time/oauth/rate. DB/snapshot privados contêm PII/hashes/PKCE temporário quando ativados; não exportar/promover DR sem política.
- `requirements.lock` SHA256 `77010e8e663cd7641e6772b7f66716b30cded8d22b73e719f23157bcf7193c6a`:13 dependências+hashes, venv isolado, argon2-cffi25.1.0, Authlib1.9.5, requests2.34.2. pip check PASS.
- Contratos novos `contracts/auth-bff.v1.json` e `auth-config.v1.schema.json`; índice/OpenAPI atualizados; mestre/NEW-SITE/continuidade/migrações locais atualizados.

### Contrato UX JSON, nenhuma UI template
Namespace `/api/v1/auth/`. GET `options` público200 (flags apenas), `context` bootstrap sessão+CSRF, `session` sessão própria, `google/callback` code/state com redirect fixo `/conta/`. POST `register`/`login` {email,password}, `verify` {email,password,token}, `verify/resend` {email,password}, `reset/request` {email}, `reset/complete` {token,password}, `logout`/`logout-all` {}, `reauth` {password}, `google/start`/`link/google/start` {}, `link/password/start` {email,password}, `link/password/complete` {token}. POST cookie exige X-CSRF-Token/Origin/Host exatos. Front obtém context e guarda CSRF só memória; refaz context após Google303. Nenhum password/code/session/access/id token em URL ou JS persistente. Callback aceita code OIDC efêmero, não token de sessão. Config ausente503, options explica estado.

### Primitivas e distinções de confiança
Argon2id RFC_9106_LOW_MEMORY:64MiB/time3/parallelism4, salt16/hash32 bytes pela biblioteca; password15..128 caracteres/max512bytes, concorrência2 por processo. SQL rate20/60s por peer real+identificador normalizado, sem confiar forwarding. Generic responses/dummy Argon2, não prova formal timing/DDoS distribuído. Email ASCII/casefold nesta fatia. Registro PENDING até email+password; reset one-use revoga sessões, não suspensões do operador. Sem códigos de verificação no JSON/URL/log.

Cookie opaco256bit `__Host-filuna_session` Secure HttpOnly SameSite=Lax Path=/; DB só hashes cookie/CSRF, epoch/expiry/identidade revalidados; rotation/fixation/logout/all/reset, origem e CSRF. Timeouts sessão3600s/email900s/OAuth300s/prova recente300s são segurança, NÃO windows de entitlement/cache/offline. Cadastro/login não atribuem premium/admin. MFA BFF não implementado.

Google Authlib code+PKCE S256/state/nonce browser-bound one-use; PyJWT **RS256 apenas**, RSA>=2048bit, JWKS oficial bounded/rotação, issuer/audience/exp/iat/nbf quando presente/nonce/email_verified/azp/at_hash se presente; sem jku/redirects/netrc/proxies ambientais. Issuer/sub estável, nunca auto-link por email. Vínculo exige duas provas recentes independentes e não mescla outra conta. Revalidação após provider previne ressuscitar sessão revogada. Endpoints HTTPS Google fixos; scopes openid/email, sem refresh/userinfo. GoogleRS256 ≠ Ed25519 grant/content da fase02. Origem primeira parte+account_id estável entre métodos; sem emitir JWT/entitlement.

## Testes e estado
**148 PASS / 0 FAIL**,12.419s, baseline111 PASS também separado +37 novos. unittest com ResourceWarning como erro; pip check PASS. HTTP loopback REAL/Argon2 REAL/RS256 REAL com chaves/teste em memória; transporte email/JWKS sintéticos em memória. Testes positivos/negativos: PENDING/proof/reset/reuse/expiry; register/reset concorrentes; rate/concurrency; CSRF/origin/fixation/rotation/logout/all/revocation; callback revogado durante exchange; cross-account mesmo tenant/RBAC; provider linking forged/prova ambos/recent/conflicting; RS256 wrongkey/alg/claims/nonce/state replay/wrong-browser/openredirect/tokenURL; operador desabilitado não reativado por reset. Baseline111 inclui storage/version/CAS/traversal/tamper/cross-tenant/migration rollback; fixture de próxima migração agora dinâmica para não colidir005.

Runtime normal verificado16:19 Europe/Rome: auth/trust config AUSENTES; zero accounts/identities/credentials/sessions/one-time/flows/rate buckets. Home/conta200; options200; context/session/register/google-start/privatebackups503; caminho privado auth-config404. Integridade/FK ok. Preview loopback18765; NÃO trocar tráfego por este checkpoint. PID registrado antes coordenação deploy:2089150 (pode ser substituído pelo designer). `service/app.py` SHA256 `85b2d40d297f80a1894a2934431558f19352630107743d581a1aaff36733d090` integrado ANTES de t35; preservar auth dispatch/body cap/headers/Set-Cookie/Location/load_auth e kwargs auth nos handlers ao integrar front. Nenhum restart/app/frontend posterior à coordenação t35.

## Ativação legítima exata, nunca segredo em chat
1. Origin HTTPS autorizada, mesma origem BFF/TLS/routing revistos; não configurados aqui.
2. Google WEB client_id/client_secret legítimos e callback exata `https://<origin-autorizada>/api/v1/auth/google/callback` cadastrada, consent/brand/test-users legítimos; não criar GoogleProject implicitamente.
3. SMTP hostname/465 implicitTLS, sender verificado, username/password/app credential legítimos e autorização operacional de entrega. SMTP adapter inativo; fila memória2 workers/8 não outbox durável e pode perder envio em restart, precisa resend. Nenhum email real enviado.
4. Config canônica privada root-owned regular sem symlink/group-write/outros-acesso e legível pelo processo; schema_version1/origin/google{client_id,client_secret,redirect_uri} ou null/smtp{host,port465,username,password,sender} ou null. Obter pelo pai com Vault/Form/Handoff seguro, não chat/plaintext. `runtime/auth-config.json` NÃO criado.
5. Produção DB/storage/backup/privacidade/proxy/rate/pentest e MFA de operador pendentes; signer/trust Ed25519 separado para premium. Login não concede assinatura. Não mexer cofre/legado/OS/Nginx nem consultar credenciais globais.

**NOT RUN**: Google real, SMTP real, TLS/browser auth, MFA, durable delivery/outbox, escala/edge anti-abuso, pentest/revisão externa, UI moderna/visual desta fase, produção/comercial/offsiteDR. Cookies Secure foram reproduzidos manualmente em teste HTTP local, NÃO browser HTTPS. Novo DB/devHTTP/snapshot mesmo host não são produção. Nenhum gate desktop/hardware alterado, básico Orca/offline livre mantido. Sem quotas/preços/provider checkout/política comercial inventados.

## Evidência privada local não secreta
`/srv/filuna-new-site/docs/evidencias/`: `fase04-tests.txt`, `fase04-baseline-tests.txt`, `fase04-pip-check.txt`, `fase04-dependency-versions.json`, `fase04-migration-process.json`, `fase04-current-process.json`, `RESULTADOS-FASE-04.json`, `MANIFESTO-FASE-04.json`. Espelhos sanitizados para pai `/workspace/filuna-web-fase04-docpatch-20261108/evidencias/`.

Referências oficiais consultadas: https://developers.google.com/identity/openid-connect/openid-connect ; https://argon2-cffi.readthedocs.io/en/stable/api.html ; https://docs.authlib.org/en/stable/oauth2/client/http/index.html .
