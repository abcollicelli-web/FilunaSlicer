# Publicação visual C — t35 — 08/11/2026

## Autoridade e evidência própria deste deploy

Dono, t35u, **08/11/2026 16:19 Europe/Rome**, literal:
> aprove e publique

Aprova a candidata revista após t34 e autoriza publicar home/Central/acesso/cinco áreas. Fonte aprovada: ZIP SHA256 `874b50d79ba3b1c301f77e19455e452cb03fe808870999fd1985e47307a78839`. Os 37 arquivos fonte conferem com o ZIP. A revisão anterior do coordenador viu oito páginas sem colisão, três desktops Chrome 1280×800/zoom nativo100% e mobile390 home/Central (crop3D intencional; rail horizontal); isso é relato do coordenador sobre o offline, não execução deste deploy nem pixel-diff.

**Publicação real em 08/11/2026 16:23:45 Europe/Rome**, release `web-c-20261108-t35`. Nginx usa TLS existente e rotas EXATAS para a release estática na mesma raiz nova. Sem proxy de backend, novo servidor/framework/CMS ou exposição da porta de prévia. CSS/JS/Inter/logo/render/prancha mantêm bytes aprovados. Delta HTML: href/src canônicos, CSP sem file/data e correção factual da nota sobre aprovação/publicação. Nenhum redesenho.

## Endereços verificados em HTTPS normal

200 e hash de bytes correspondente em todos:
- https://filuna3d.com/
- https://filuna3d.com/conta/
- https://filuna3d.com/conta/acesso/
- https://filuna3d.com/conta/contratar/
- https://filuna3d.com/conta/assinatura/
- https://filuna3d.com/conta/backups/
- https://filuna3d.com/conta/recursos/
- https://filuna3d.com/conta/downloads/
- https://filuna3d.com/sobre-a-previa/

Sete assets públicos em `/assets/web-c-20261108-t35/`, incluindo Inter/OFL, logo original, estudo orbital, referência Preparação C, CSS e JS, verificados por SHA256 via HTTPS. WWW HTTPS também200/certificado validado. 145 referências de navegação/assets resolvem para a allowlist. Sem docs/pesquisa/config/DB/código privado publicados.

## Resultado de teste sem esconder falha de rede

- 45 verificações públicas PASS: páginas/assets, hashes, bloqueios, admin401 e redirects canônicos.
- **2 verificações HTTP simples do box receberam500 `Server: Pingora`, sem corpo**, em vez do301 esperado. O mesmo domínio, consultado normalmente a partir do host de origem, devolveu301 para HTTPS tanto raiz como WWW/conta. Não foi alterado proxy/DNS nem contornado o intermediário. Resultado externo HTTP permanece limitado; não chamar47/47PASS.
- Arquivos internos, runtime/configs, docs, research, service, contracts, venv, Git, deploy, API privada/auth e traversal testados:404. Admin anônimo `/admin/` e `/admin/shell.html`:401, proteção preexistente preservada.
- 115 verificações estáticas e11 testes Node com doubles DOM reexecutados PASS. Não são navegador/auditoriaAA.
- 148 testes atuais do backend PASS em12,295s após a publicação, fixtures temporárias; tentativa anterior foi interrompida pelo steering e não contada. Nenhuma migração operacional repetida. Não equivale a Google/SMTP/TLS/browser reais.
- `nginx -t` antes/depois PASS, reload PASS. Cache-Control no-store, CSP same-origin sem conexões/submissões e headers de proteção nas páginas/assets. Não havia CDN gerido a invalidar.
- Checkpoint inicial: Browser/CU público e comparação pixel/diff NOT RUN neste executor. Atualização posterior: desktop público conferido pelo coordenador às16:30Europe/Rome no adendo abaixo; mobile público/pixel-diff continuam NOT RUN. Rollback real: NOT RUN; script/snapshot preparados e checados, não executados para interromper o novo site.

## Árvore e arquivos afetados

- `public/releases/web-c-20261108-t35/`:9 HTMLs canônicos +7 assets allowlisted.
- `public/current`:symlink atômico para a release. Arquivos antigos `public/index.html`, `public/home.html`, `public/assets/*` permanecem intactos como baseline novo; não são as views públicas atuais. A prévia loopback antiga continua separada apenas como laboratório, sem publicação.
- Configuração Nginx do domínio: rotas exatas e assets allowlisted; admin/TLS/redirect HTTP preservados. Include de headers em `deploy/web-c-security.conf`.
- `deploy/incoming/`:pacote/manifesto/config usados. `deploy/snapshots/web-c-20261108-t35/`:snapshot somente do public NOVO e configNginx anterior; protected hashes e rollback. Não contém snapshot do cofre nem DB restaurado.
- `contracts/web-publication.v1.json` e entrada aditiva no índice: vínculos ao registro existente de cinco módulos, sem mudar IDs/capabilities/handlers/API.
- Este documento, evidências e apêndices em mestre/NEW-SITE/CONTINUIDADE. Apêndices fase04 preservados.

**Nenhum arquivo em service/, migrations/, admin/ ou estado privado foi modificado pelo deploy.** `app.py` continua SHA256 `85b2d40d297f80a1894a2934431558f19352630107743d581a1aaff36733d090`, preservando dispatch/auth headers/cookies/load_auth/kwargs. Nenhum restart do backend.

## Rollback operacional, sem tocar dados

Snapshot: `/srv/filuna-new-site/deploy/snapshots/web-c-20261108-t35/`.

Comando de operador, se necessário:
```
sh /srv/filuna-new-site/deploy/snapshots/web-c-20261108-t35/rollback.sh
```

Restaura APENAS configuraçãoNginx anterior, executa nginx-t/reload e remove o symlink current se não havia um antes. O baseline público novo não foi sobrescrito, portanto volta à página de transição nova. Não toca DB/migrações/auth/cofre/legado. Symlink/release podem ser promovidos novamente em rollout futuro autorizado. Snapshot é mesmo host, não DR/offsite.

Hashes: Nginx publicado `ddb929f0eb5e029c2beb6abd352adafdf0318a3d896b0070b3e7cd61807766ea`; manifesto `cfbf6b1e7850fe9985364e392bef49d720eaea3c4767b167cb34c6d9c78245f8`; home pública `37d8126668b364d9e74c139fc19778bb1e41d15d9c3e4c87536edffc99ae7ea7`.

## Limites preservados

Esta é a publicação do VISUAL aprovado, não ativação comercial ou de login. E-mail/senha continuam disabled, Google apenas explica integração pendente; não há coleta de senha, conta/plano/preço/status/arquivo fake, contratação, restore ou download simulados. Auth options loopback indica configured=false; auth privado503; no domínio APIs continuam404 porque não foram publicadas. Backend fase04 implementado/testado permanece inativo até configuração/insumos legítimos e integração segura das views — não inventar credenciais.

Básico/base Orca livres/offline, MESMA engine; catálogo homologadoFiluna somente com autorização válida, arquivos próprios preservados. Geração/preparação/fatiamento no PC. Nenhum admin controla impressora. FLN-VISUAL-C-100/âncorasSlicer/G2–G6/hardware não foram modificados nem homologados. Não há alegação de prêmioFiluna, prontidãoWindows, DR ou produto completo.

## Evidências publicáveis estruturadas
[Rotas, request IDs, hashes e resultados](WEB-PUBLICACAO-C-T35-EVIDENCIAS-20261108.json). Código/config/DB/segredos do serviço não integram este registro público. Snapshot e logs completos ficam somente na área nova autorizada. Este patch é exclusivamente documental, base `c1685c7d61fbb46d18cdba3165d90565738be5e1`, sem commit/push por este executor.


### QA público desktop posterior — 08/11/2026 16:30 Europe/Rome

O coordenador executou Chrome público real, desktop1280×800/zoom100%, sem emulação e HTTPS normal sem aviso de certificado. Abriu a home, clicou **Central do cliente** para `/conta/` e **Acessar conta** para `/conta/acesso/`. Logo e estudo3D renderizaram; não houve fallback de fonte evidente nem texto/botões sobrepostos nas áreas vistas. Acesso mostrou conexão não configurada e campos desativados. Google não foi clicado, nenhuma credencial submetida. Executor inspecionou as três capturas brutas, sem segredos aparentes.

**Estado atual:** desktop público conferido no escopo acima; mobile público, comparação pixel/diff, auditoriaAA e homologação funcional/global **NOT RUN**. Captura1280×800 não fixa a equivalência CSS/device/DPI. Os45 checksHTTP/HTTPS anteriores continuam45; os2HTTPsimples500Pingora permanecem registrados, sem contarCU como mediçãoHTTP adicional. Não ativaGoogle/SMTP/login/pagamento nem altera gates/aplicativo/hardware.

[Capturas brutas e manifesto SHA256](evidencias/web-t35-public-20261108/MANIFEST-SHA256.json): home, Central e acesso, sem resize/crop/máscara. SHA256 do manifesto `f9680c1019474669fd6ae57c2c4b6f97e5c44f249d0d6fdc119cfdcf064ba790`. NOTRUN anterior do browser é histórico do checkpoint do executor, superado SOMENTE por esta inspeção desktop delimitada.
