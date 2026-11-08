# Filuna Slicer — continuidade (fork)

Este repositório é o **FilunaSlicer** (fork AGPL do OrcaSlicer). Antes de qualquer trabalho Filuna:

1. Leia `FILUNA/REGISTRO-CONTINUIDADE-ETAPAS.md` (estado: **G0 PASS DOCUMENTAL**; próximo = **Onda 01 Fundação** / `FILUNA/FUND-SPEC-ONDA01-2026-11-05.md` — **não** Identity/Home/tokens)
2. Leia `FILUNA/REGRA-MESTRE-QUALIDADE.md` e `FILUNA/PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md`
3. Respeite `upstream-mirror` intacto e a pasta `FILUNA/`; não alterar `src/libslic3r/**` na fundação sem ADR

As seções abaixo são as regras de engenharia herdadas do OrcaSlicer (também em `CLAUDE.md` → este arquivo). Elas continuam válidas para o core C++/wxWidgets.

---

# CLAUDE.md

OrcaSlicer — open-source C++17 3D slicer. wxWidgets GUI, CMake build system.

## Build Commands

Build the Release configuration unless asked otherwise.

```bash
# macOS
cmake --build build/arm64 --config Release --target all --

# Linux
cmake --build build --config Release --target all --

# Windows
cmake --build . --config Release --target ALL_BUILD -- -m
```

## Testing

Catch2 framework. Tests in `tests/`; see [tests/AGENTS.md](tests/AGENTS.md) for where a new test belongs and the conventions to follow.

```bash
cd build && ctest -C Release --output-on-failure    # all tests
ctest --test-dir ./tests/libslic3r -C Release       # individual suite
ctest --test-dir ./tests/fff_print -C Release
```

## Documentation

- Docs live in `docs/`; the high-level design of a subsystem goes in `docs/HLSD/<subsystem>.md`.
- Describe the design as it stands — what the subsystem does, why it exists, and the constraints that shape it. Not the route that got there: no phases, task lists, status markers, or "before/after this PR" framing.
- Planning and investigation output (brainstorms, superpowers design and plan docs) stays in `docs/superpowers/`, which is gitignored. Never commit it.
- Write a doc only when the design is not evident from the code, and when a change invalidates an existing one, update it in the same PR.

## Code Style

- C++17, selective C++20. PascalCase classes, snake_case functions/variables
- `#pragma once` for headers. Smart pointers and RAII preferred
- Include what you use: include the header for every symbol a file uses, and keep headers compilable on their own. Never rely on the precompiled header or a transitive include. The `clang-tidy` CI job enforces this on changed lines; run the same check locally with `scripts/run_clang_tidy.sh` (`scripts\run_clang_tidy.ps1` on Windows), which sets up everything it needs
- Parallelization via TBB — be mindful of shared state
- Always use `SetSizerAndFit(sizer)` instead of `SetSizer(sizer)` on top level window. Unless `SetSizer` must be called before the full layout is built, call `sizer->SetSizeHints(window)` afterwards in this case.

## Key Entry Points

- App startup: `src/OrcaSlicer.cpp`
- Slicing pipeline: `src/libslic3r/Print.cpp`
- All print/printer/material settings: `src/libslic3r/PrintConfig.cpp`
- GUI: `src/slic3r/GUI/`
- Core algorithms: `src/libslic3r/` (GCode/, Fill/, Support/, Geometry/, Format/, Arachne/)
- Printer profiles: `resources/profiles/[manufacturer].json`

## Critical Constraints

- **Backward compatibility required** for .3mf project files and printer profiles
- **Cross-platform** — all changes must work on Windows, macOS, and Linux
- Profile/format changes need version migration handling
- Dependencies built separately in `deps/build/`, then linked to main app

## Pull requests

Before opening a pull request or posting on GitHub, read [CONTRIBUTING.md](CONTRIBUTING.md), including its "If you are an AI agent" section.

## Code review focus areas

- Changes must not cause regressions in existing functionality, defaults, profiles, or project compatibility.
- Features gated by options must not affect existing behavior when those options are disabled.
- Changes should follow the existing code style and architecture. Architectural changes should be justified in code comments and the PR description.
- Add helper functions or utilities only when existing code cannot reasonably be reused. Avoid duplication.
- Keep code concise and clear. Manually simplify AI generated bloated codes before review.
- Include targeted tests or documented verification for behavior changes, especially in slicing logic, profiles, formats, and GUI defaults.
- For profile changes (`resources/profiles/<Vendor>/**`), check that `version` in the sibling `resources/profiles/<Vendor>.json` was bumped.
- For new or changed UI strings, check they follow [Writing translatable strings in the source](#writing-translatable-strings-in-the-source): no concatenated fragments, plural forms for counts, contexts for ambiguous words.
- For translation changes (`localization/i18n/**/*.po`), check that recurring terms match the [Localization glossary](https://github.com/OrcaSlicer/OrcaSlicer_WIKI/blob/main/developer_reference/localization_glossary.md) for that language.

## Localization & translations

Catalogs live in `localization/i18n/<lang>/OrcaSlicer_<lang>.po`; the template is `OrcaSlicer.pot`.
See the [Localization guide](https://github.com/OrcaSlicer/OrcaSlicer_WIKI/blob/main/developer_reference/localization_guide.md) for the human-facing version of these principles.

### Writing translatable strings in the source

Translators see one msgid at a time and cannot change the code around it, so each message must be complete in a single msgid. Use the wrapper that fits instead of assembling text from pieces:

- **One msgid per sentence, with placeholders.** Never build a message by concatenating translated fragments (`_L("A") + name + _L(" is B")`) or by splicing a translated word such as `_L("left")` into another sentence: the translator can't reorder or inflect the parts. Write `format(_L("%1% is too close to others"), name)`.
- **A count needs a plural form.** Don't write `file(s)` or leave a verb unchanged after a number. Use `_L_PLURAL("%1% object", "%1% objects", n)`. libslic3r has no plural lookup, so word the message so it doesn't depend on a count, or build it in the GUI.
- **An ambiguous word needs a context.** When a short string means different things in different places (verb or noun, a mode name, a CAD term), give each sense its own context with `_L_CONTEXT`/`_u8L_CONTEXT`/`L_CONTEXT`. Keep the most common sense without a context so its existing translations stay valid.
- **Mark every user-visible string** (`_L`/`_u8L`, or `L` where it is translated later) and list new source files in `localization/i18n/list.txt`: files that aren't listed are never extracted.
- Add a `// TRN` comment above a new string whose meaning or placeholders aren't obvious.
- Never choose behavior by comparing translated text; compare ids or flags.

See `.claude/skills/orca-translations/references/source-audit.md` for the full checklist.

### Terminology

- Use the [Localization glossary](https://github.com/OrcaSlicer/OrcaSlicer_WIKI/blob/main/developer_reference/localization_glossary.md) as the source of truth for recurring terms, so the same English term is always rendered the same way within a language, and terms that must stay in English (brand/product names, acronyms, materials, file formats, G-code tokens, macros/variables/identifiers) are not translated.
- If a term's established translation changes, update both the affected `.po` files and the glossary (`developer_reference/glossary/<lang>.tsv` in the wiki, then regenerate) so they stay in sync.
- Translate the *meaning*, not the words. Check what the string actually controls before translating it — English reuses one word for different things. `Flow ratio` (multiplier), `Flow Rate` (throughput) and `Flow Dynamics` (pressure compensation) are three different terms; `extruder` may mean the toolhead, the feeder motor, or the nozzle depending on the string.
- Reuse one template per recurring message shape (`Failed to connect to …`, `Are you sure you want to …?`), even where the English wording varies.

### Editing rules

- Only edit `msgstr` — **never** change `msgid`, and never "fix" wrong English in the translation alone. Report the source string instead.
- Preserve exactly: placeholders (`%s`, `%d`, `%1%`, `%zu`, `%%`), every `\n` (count *and* position, including leading/trailing), leading/trailing spaces, HTML tags, `℃`, and the file's encoding and line endings.
- **Never reorder positional arguments** in a `c-format` string. If the msgid is `%d` then `%s`, that order must hold — swapping them breaks at runtime.
- `msgctxt` separates homonyms — always read it. `Back`/`Camera View` is the rear view of the 3D navigator, while `Back`/`Navigation` is the go-back button; `Top` exists in the *Alignment*, *Layers* and *Camera View* senses.
- When a string needs disambiguating, add context in the source (`_L_CONTEXT`/`_u8L_CONTEXT`), don't work around it in the translation.
- A literal `%` inside a string xgettext flagged `possible-c-format` will fail `msgfmt`. Fix it with a `// xgettext:no-c-format, no-boost-format` comment above the string in the source — do not mangle the translation or use `%%` in text that is never passed through printf.
- Plural entries: read `nplurals` from the catalog's `Plural-Forms` header (it is **not** always 2 — ja/ko/zh/th/vi use 1, ru/cs/pl/lt use 3, uk uses 4). Each form must be genuinely inflected for its quantity; repeating one sentence across all forms is a bug in Slavic/Baltic languages, though it is correct for Turkish and Hungarian.
- An entry whose `msgstr` equals its `msgid` is untranslated even though it is not empty; a plural entry with any empty form is likewise incomplete.
- Mark machine-produced translations with an `# AI Translated` translator comment. Don't add it to a human translation you didn't actually rewrite.
- When you can't be sure of a machine translation's meaning or UI wording, also add `# Needs human review: <what to check>`. Never use `fuzzy` for this — fuzzy entries are hidden from users.
- Don't reflow or re-wrap unrelated entries — keep the diff limited to the strings you changed.

### Verifying

- `scripts/run_gettext.bat --full` (Windows) regenerates the template, merges every catalog and compiles the `.mo` files. It must exit 0.
- Or check a single catalog with `msgfmt --check-format --check-header -o <out>.mo localization/i18n/<lang>/OrcaSlicer_<lang>.po`. Without `--check-header`, msgfmt skips the placeholder check in plural forms.
- Fuzzy entries are not shown to users. If you correct one, clear its `fuzzy` flag, otherwise the fix never ships.


## Direção visual C — referência aprovada em 08/11/2026

Antes de criar ou alterar qualquer tela Filuna, consultar `FILUNA/design/visual-c/documentacao/DESIGN-SYSTEM-C.md`, a aprovação literal em `FILUNA/design/visual-c/documentacao/APROVACAO-VISUAL-C.json`, os dois modelos C aprovados e a galeria/matriz em `FILUNA/design/visual-c/README.md` e `FILUNA/design/visual-c/MATRIZ-COBERTURA.md`. O proprietário aprovou a direção visual; as demais telas e detalhamentos são propostas em revisão. O inventário consolidado contém 123 pranchas e 208 itens rastreados, sem equivaler à homologação de software.

Preparação permanece sem painel direito de informações de arquivo; Orca é o motor único. A regra vigente de G-code imutável e a distinção entre risco OEM e divergência de motor permanecem intactas. Esta aprovação não retoma implementação, CI, hardware, distribuição, servidor ou website. No escopo pausado pelo proprietário, G2/G3 permanecem BLOCKED, G4 NOT RUN e G5/G6 BLOCKED. Nenhum requisito funcional ou registro histórico foi revogado.

## Obrigação visual FLN-VISUAL-C-100 — 08/11/2026

Antes de qualquer trabalho de UI, ler obrigatoriamente [REGRA-FIDELIDADE-VISUAL-C.md](FILUNA/design/visual-c/documentacao/REGRA-FIDELIDADE-VISUAL-C.md), contrato de montagem e ordem expressa de 14:16 Europe/Rome. Toda IA/pessoa deve reproduzir 100% identicamente o modelo aprovado; nenhuma mudança estética/estrutural/de recurso sem nova aprovação explícita do proprietário. Derivadas não são aprovadas automaticamente; lacuna ou conflito bloqueia o item. Evidência visual real e aceite humano são obrigatórios; conformidade da aplicação NOT RUN. Pausa funcional e gates preservados.


## Continuidade web autorizada — 08/11/2026, 14:57 Europe/Rome

Nova ordem expressa + **«esta liberado»** autorizam iniciar home/admin/serviços somente na área nova `/srv/filuna-new-site`, respeitando `docs/NEW-SITE.md`. Ler [contrato web](FILUNA/WEB-SERVICOS-ESCOPO-20261108.md), [mestre servidor](FILUNA/ARQUITETURA-SERVIDOR-DADOS-REMOTO.md#web-servicos-20261108) e [pedido literal](FILUNA/approvals/WEB-SERVICOS-AUTORIZACAO-20261108.json). Não mexer no cofre/legado; não copiar código privado/segredo para este repositório. Home C é candidata para revisão, não aprovação individual automática. Esta autorização específica não retoma implementação geral Slicer, CI ou hardware nem promove gates. Contrato comercial conflitante com core AGPL/offline deve ser resolvido, não interpretado como bloqueio global do motor. Neste lote documental, implementação/testes/deploy web NOT RUN.


## Regra de acesso posterior — 08/11/2026, 15:18 Europe/Rome

[Decisão expressa básica/premium](FILUNA/approvals/WEB-ACESSO-BASE-ORCA-20261108.json): básico Orca/modelo-base livres/offline; sem assinatura válida, não carregar perfis/modelos homologados Filuna e usar modo base limpo da MESMA engine. Não apagar arquivos/criações nem revogar direitos de licenças livres. Prazos offline/cache/revogação ainda pendentes. [Fatia web real e limites](FILUNA/WEB-FASE-01-EVIDENCIAS-20261108.md): prévia isolada, 33 testes de subconjunto PASS; home candidata e APIs privadas fechadas, não sistema completo nem homologação. Não publicar código web privado ou alterar gates do desktop por esse resultado.


### Continuidade obrigatória — Central do cliente / identidade (08/11/2026)
Antes de evoluir web, ler [FILUNA/WEB-FASE-02-EVIDENCIAS-20261108.md](FILUNA/WEB-FASE-02-EVIDENCIAS-20261108.md), [decisão t33](FILUNA/approvals/WEB-CENTRAL-CLIENTE-T33-20261108.json) e [contrato atualizado](FILUNA/WEB-SERVICOS-ESCOPO-20261108.md#central-cliente-identidade-20261108). Aceite visual condicional exige entrada Central do cliente, sem redesenho. Composer `/conta/` e verificação real testados somente na área nova, 76 PASS; login/provedor/signer e checkout/storage/delivery ainda não operacionais, privado fail-closed. Não importar fixture de teste para conceder acesso. Código privado só na raiz autorizada; nenhum deploy, cofre/legado, desktop ou gate promovido.


### Marco storage e revisão visual t34 (08/11/2026)
Ler [evidências fase03](FILUNA/WEB-FASE-03-EVIDENCIAS-20261108.md), [contrato](FILUNA/WEB-SERVICOS-ESCOPO-20261108.md#storage-restore-bytes-20261108) e [ordem visual t34](FILUNA/approvals/WEB-VISUAL-REVISAO-T34-20261108.json). 111 testes PASS no backend privado novo; 004 já aplicada, não repetir cegamente. Provedor/signer ausentes, privado negado; não produção/DR. T34 exige revisão DESIGN: não homologar home/central nem mudar CSS/markup neste workstream backend. Nenhum segundo backend, cofre/legado/deploy/Slicer/hardware.


## Continuidade web Fase04 — dual login
Escolha literal t33s249,08/11/2026 15:52 Europe/Rome: «Quero login com e-mail e senha e também com conta Google.» Backend BFF+Argon2id+GoogleOIDC/RS256 implementado/testado148 PASS, auth/trust semconfig e privado fechado, zero dados operacionais. [Evidências/contratos/ativação e limites](FILUNA/WEB-FASE-04-EVIDENCIAS-20261108.md). Front/deploy t35 coordenado separadamente pelo pai; não sobrescrever backend/migrações/dependências nem confundir aceite visual com login/provider operacional. Nada de gates desktop/hardware.


## Continuidade obrigatória — visual web publicado t35
Dono08/11/2026 16:19 Europe/Rome: «aprove e publique». Modelo web aprovado e publicado16:23:45 Europe/Rome em https://filuna3d.com/ e Central canônica; [evidências e limites](FILUNA/WEB-PUBLICACAO-C-T35-EVIDENCIAS-20261108.md), [aceite/hash](FILUNA/approvals/WEB-VISUAL-PUBLICACAO-T35-20261108.json). Não redesenhar composição/paleta/Inter/logo aprovados. Publicação estática não ativa loginGoogle/SMTP/checkout/premium: camposdisabled, APIsdomínio404, admin401; backend148PASS intacto. Design115+11PASS, HTTPS/guards45PASS,2HTTPbox500Pingora com origem301; não alegar47/47PASS. Browser público desktop conferido às16:30Europe/Rome no escopo documentado; mobile público/rollbackexecutadoNOTRUN. Preservar fase04, migração005jáaplicada, dados/engine/gatesdesktop; nenhum segredo/backendprivado no repositório.


### QA público desktop t35 registrado
Chrome público real pelo coordenador,08/11/2026 16:30Europe/Rome: home→Central→acesso,desktop1280×800/zoom100%,HTTPS normal; logo/3D renderizados, sem sobreposições/fallback evidente nas áreas vistas; inputsdisabled. SemGoogle/credenciais/mobile público/teste global.45checksHTTP anteriores e2HTTP500Pingora preservados. [Capturas/hash](FILUNA/evidencias/web-t35-public-20261108/MANIFEST-SHA256.json). Gates/serviços não ativados.
