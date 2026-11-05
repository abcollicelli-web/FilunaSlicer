# Filuna Slicer — Diário de qualidade e decisões (05/11/2026)

## Escopo e isolamento

- **Solicitação do proprietário:** ler documentos mestre de personalização do antigo Filuna e estabelecer regra: todas as etapas documentadas; nenhuma segue para a seguinte se houver um requisito de teste faltante ou falho.
- **Destino:** fork novo `abcollicelli-web/FilunaSlicer`, branch de trabalho `filuna/quality-master-20261105`, originada de `filuna/rebrand-1` SHA `2dace0db86d305bec5b9627179f265012e4f5069`.
- **Sem edição** da branch que compila Windows, da `filuna/main`, da `upstream-mirror`, do servidor, do site, das impressoras ou de módulos executáveis. Esta etapa é fundamento documental + verificador de contratos.
- **Política atual:** proprietário aprovou o PROCESSO de gates; a migração visual ainda depende de implementação e QA próprios nesta base C++.

## Documentos antigos examinados nesta passagem

- `abcollicelli-web/Filuna`, `main`: `Filuna-Slicer/14-CONTINUIDADE-INTEGRAL.md`; `12-REFERENCIA-VISUAL-OFICIAL.md`; `18-HOME-GROK-2026-11-03.md`; `25-REGRA-MESTRE-ATUALIZACOES-2026-11-03.md`; `16-TESTES-BETA-PRIVADA.md`; `08-ETAPAS.md`; `10-APIS-E-QUALIDADE.md`; `GROK-GITHUB/LEIA-PRIMEIRO-GROK.md`; `GROK-GITHUB/ESTADO-DO-PROJETO.md`; `GROK-GITHUB/ATUALIZACOES/2026-11-03-ESTRUTURA-HOME-PREPARAR-CALIBRACAO.md`, `2026-11-03-MESTRE-PREPARAR.md`, `2026-11-03-MESTRE-DISPOSITIVOS.md`, `2026-11-03-MESTRE-PARAMETRICOS.md`. Alguns arquivos longos foram examinados por faixas e destaques nesta sessão; inventário exaustivo do legado NÃO concluído.
- `abcollicelli-web/filuna-slicer`, `main`: `docs/GROK_CONTRACT.md`, `docs/CONTINUAR-PROJETO-FILUNA-SLICER.md` e `docs/RELEASES-WINDOWS-QA7-QA9-REGISTRO-MESTRE-2026-11-03.md`; identificada a árvore principal, ainda sem auditoria integral de todos os fontes.
- `abcollicelli-web/FilunaSlicer`: `AGENTS.md`, `CONTRIBUTING.md`, `FILUNA/README.md`, `FILUNA/ARCHITECTURE.md`, `FILUNA/TEST-ACCEPTANCE.md` e `FILUNA/UPDATE-PLAYBOOK.md`. Novo código C++/wxWidgets não é o legado C#.

## Entregas documentais da primeira onda

1. `FILUNA/REGRA-MESTRE-QUALIDADE.md`: gates G0-G6, disciplina de rastreabilidade, PASS/FAIL/BLOCKED/NOT RUN, sem dispensas tácitas.
2. `FILUNA/MAPA-MIGRACAO-PERSONALIZACAO.md`: precedência da Home homologada, especificações das telas, diferenciação de demonstrações e operações reais, riscos e status de implementação.
3. `FILUNA/README.md` e `FILUNA/TEST-ACCEPTANCE.md` atualizados com referências e checklist.
4. `FILUNA/tests/check_quality_gate.py` + workflow `.github/workflows/filuna-foundation-check.yml`: revisão automatizada de requisitos básicos e modo de liberação que bloqueia quando não existe dossiê de aprovação.

## Evidências reais de execução do CI

- Inicial `42106062307`: FAIL por um teste que exigia capitalização incorreta; corrigido.
- `42106142963`: FAIL; ajuste adicional de sintaxe e validação.
- `42106212658`: FAIL, teste de largura `56 px`/ `208 px` não aceitava CSS `56px`/ `208px`; corrigido.
- `42106323896`: FAIL por escape duplo na tentativa de regex; corrigido removendo a ambiguidade.
- **`42106347670` → SUCCESS** no commit `fc2840e1205ebf9f571536f15635355cac619522`, 29 verificações estáticas PASS e 0 FAIL, URL: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42106347670 .

**Limitação:** estas 29 verificações confirmam integridade de arquivos, referências, status de gates e perfis-base; não são testes de interface no Windows, fatiamento, impressão física, integração de fabricantes, segurança dinâmica nem assinatura do instalador. O gate G0 **continua em andamento** até terminar o inventário obrigatório. O gate de release não foi e não pode ser considerado aprovado sem evidências reais e aprovação do dono.

## Pendências bloqueadoras da próxima etapa

- Terminar a leitura integral e inventário de regras/decisões dos documentos restantes, rastrear contradições e a versão mais recente de cada tela.
- Confirmar a referência visual aprovada mais atual por materiais documentados autorizados, confrontar screenshot e build real.
- Definir os testes G1 e aprovação da SPEC para UI antes de mudar `MainFrame.cpp`, Home HTML ou o visualizador 3D do Orca.
- Realizar QA Windows 10/11 com Orca instalado lado a lado, G-code com fixtures e firmware real da Elegoo Centauri Carbon 2 e Anycubic Kobra 3 V2.
- Manter pacote Windows em compilação de `filuna/rebrand-1` independente; não publicar atualização ao cliente como homologada.

**Decisão operacional:** até G0 completar, a única evolução permitida é leitura, investigação, especificação e teste da mesma etapa. Nenhuma promoção, merge, release ou modificação de UI comercial nesta entrega.

## Adendo — identidade visual oficial e mascote (05/11/2026)

Após a fundação de QA, o proprietário entregou referências visuais para **Filuna 3D**: (1) logotipo Filuna rosa e 3D laranja; (2) paleta creme/vinho/rosa/laranja/cinza; (3) ícone de aplicativo F; (4) mascote 3D em composição circular; (5) mascote recortada transparente. Foram criados `FILUNA/IDENTIDADE-VISUAL-E-MASCOTE.md` e `FILUNA/brand/REFERENCIAS-MANIFESTO.json` com identificação, dimensões e SHA-256 das cinco imagens.

**Limite do registro:** os bytes originais constam das imagens anexadas pelo proprietário e de um pacote ZIP íntegro de preparação, mas NÃO foram versionados como arquivos binários neste repositório; até isso ocorrer, o gate G1 não poderá ser marcado PASS. As cores medidas no JPG são aproximações e precisam de tabela de tokens aprovada antes de qualquer edição visual. A mascote não substitui o ícone F do aplicativo.

O CI de fundação passou em https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42109636393 no commit `c4d74b91a60a894d942063baa48326e272667e5b`. O verificador do gate de release agora exige arquivos originais no repositório e hashes conferidos; nenhuma versão foi promovida, nenhum código de GUI/Orca alterado. PR #2 permanece em DRAFT.


## Adendo — plano mestre do produto, servidor e futuro remoto

O proprietário definiu o **OrcaSlicer como único motor integral** e integração nativa por adaptadores, incluindo até dez fabricantes, câmeras, controles e multimaterial, sempre conforme protocolos oficiais ou LAN legitimamente autorizada. A pesquisa de SimplyPrint é fonte de mercado, não certificado Filuna; LAN/conta oficial/chave do dispositivo foram documentados.

Foi construído o [plano mestre](PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md), [DAG versionado](CRONOGRAMA-DEPENDENCIAS.json) com **52 entregas, 149 vínculos**, [matriz de testes](MATRIZ-TESTES-ACEITE-100-POR-CENTO.md), [normas legais](CONFORMIDADE-LEGAL-NORMAS-E-CERTIFICACOES.md), [inventário de 141 fontes](MANIFESTO-FONTES-LEGADO-2026-11-05.md), DeviceBridge com [10 fabricantes](ARQUITETURA-DEVICEBRIDGE-10-FABRICANTES.md), [servidor e remoto](ARQUITETURA-SERVIDOR-DADOS-REMOTO.md) e [relatório de auditoria](AUDITORIA-PLANO-MESTRE-2026-11-05.md).

**Nova regra de infraestrutura:** o admin Filuna só recebe erros sanitizados e saúde da conexão conforme consentimento; **não tem comando ou câmera** das impressoras dos clientes. Controle remoto dos próprios clientes é futuro e default OFF; depende de serviço seguro, backup testado e homologação física LAN. Endereço IP, credenciais, DB e domínios de produção não são hardcoded nem declarados validados.

**Evidência:** GitHub Actions [42115301358](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42115301358), SHA \`fdb84688a099a4ab9feaf822c69d9df907c6f97f\`: **113/113 PASS estáticos**, grafo sem ciclo, tentativa de liberação sem dossiê rejeitada conforme esperado. Estado **G0 EM ANDAMENTO**, G1–G6 ainda sem homologação, nenhuma integração Windows/servidor/hardware alterada. A branch continua isolada e não promoveu release.


## Adendo — registro de continuidade operacional (2026-11-05 22:48:59 CET)

Por ordem do proprietário (continuar o cronograma sobre a base e documentar etapas para qualquer IA), foi criado [REGISTRO-CONTINUIDADE-ETAPAS.md](REGISTRO-CONTINUIDADE-ETAPAS.md), vinculado no plano mestre. Neste lote: verificação de pin/upstream-mirror, artefatos do run 42102469888 (installer+portable Filuna), contagem 141 do manifesto, ausência de `FILUNA/modules/`, reexecução local dos checks estáticos (foundation + 113/113). **G0 permanece EM ANDAMENTO** (leitura integral das 141 fontes e assets de marca ainda pendentes). Sem merge de PR #1/#2 e sem QA Windows/hardware.


## Adendo — G0 leitura prioritária (2026-11-05 22:55:30 CET)

Confirmado tip plano final  (sem avanço posterior no remoto): DAG 52/149, SRV-00..11, auditoria documental 113/113. Lidas integralmente 13 fontes prioritárias S/W com blob SHA = manifesto; REQ/conflitos e superação dos quatro motores registrados em [G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md](G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md). Cinco originais de marca continuam ausentes no GitHub e nos attachments do agente. Sem provisionamento de servidor; admin sem controle de impressoras reafirmado. G0 não PASS.


## Adendo — originais de marca versionados (2026-11-05 23:00:49 CET)

ZIP do proprietário validado: 5/5 hashes SHA-256 idênticos ao [REFERENCIAS-MANIFESTO.json](brand/REFERENCIAS-MANIFESTO.json). Arquivos em `FILUNA/brand/assets/original/` com nomes estáveis + hardlinks dos nomes de conversa. **G1 visual permanece não homologado** (tokens/QA). Não altera motor/UI.


## Adendo — leitura manifesto 141 (2026-11-05 23:02:32 CET)

Concluída cobertura dos 141 caminhos do manifesto legado (119 blobs novos verificados por hash + 13 do lote anterior; 9 deduplicações S/W). Detalhe em [G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md](G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md). G0 permanece EM ANDAMENTO; SBOM do fork F só inicial.

## Adendo — conflitos + SBOM parcial + correção leitura (2026-11-05 23:07:34 CET)

- Inventário rev .4: profundidades A/B/C; **retiro** qualquer leitura como 141 integrais.
- Novo: `G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md` (CF-01..11; N1–N8).
- Novo: `FILUNA/sbom/` README + inventory JSON + CycloneDX parcial (licenças **não** afirmadas).
- G0 continua PENDING (N1, N2). Checks documentais a seguir neste commit.



## Adendo — N1/N2 semântico (2026-11-05 23:12:40 CET)

Lote autorizado: leitura semântica real do conjunto normativo (não reclassificação vazia). Novo [G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md](G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md); conflitos rev .2 com N1/N2 PASS e inventário CF-07. Inventário leitura rev .5. Checks documentais 113/113 e foundation 0 FAIL reexecutados. **G0 continua EM ANDAMENTO** (N4–N7 parciais; Windows/hardware NOT RUN). Sem Home/DeviceBridge/SRV; sem merge.


## Adendo — G0 PASS DOCUMENTAL (2026-11-05 23:24:00 CET)

Revisão do proprietário: critérios derivados que pediam QA Windows e SBOM completo para G0 **contradiziam** REGRA (G4) e PLANO (SBOM inicial). Checklist reescrita com rastreio trecho-a-trecho; CF-07 resolvido por hierarquia; mapa 141 + MIG↔DAG + riscos OEM + casos G0-T*. **G0 PASS documental (Onda 00).** G1–G6 NOT RUN/BLOCKED. Sem merge; sem implementação G1 neste lote.


## Adendo — Onda 01 FUND contrato (2026-11-05 23:30:00 CET)

Criados FUND-SPEC/REQ/ADR/TEST. Primeiro nó elegível LEGAL-02→QA-03. Corrigidas desatualizações: originais de marca no git; cobertura §12 sem G0 EM ANDAMENTO corrente; índice 0d duplicado; REGISTRO/AGENTS apontam Onda 01 não visual. Checks docs reexecutados. Sem implementação UI/CI Windows novo. G1/G2/G3 não marcados PASS.


## Adendo — FUND implementação pós-aprovação (2026-11-05 23:35:00 CET)

Dono aprovou FLN-FUND-SPEC-2026-11-05.1 (t18s14, 23:24 Europe/Rome). Registro JSON sem assinatura/token. Entregues CMP, evidence schema fail-closed, normalizador G-code sintético, DATA-05 temp isolation, CI hooks. 19 unit PASS. Baseline binária Windows **NOT_RUN**. Sem UI/Home/SRV/merge.


## Adendo — baseline binário + correção gates (2026-11-05 23:45:00 CET / UTC 2026-11-05T22:45:00Z)

Corrigido relatório: sem «G2/G3 PASS» por 19 unitários. Normalizador allowlist estrita; evidence UTC-only; runner Windows + extensão `filuna-windows.yml` `baseline-gcode-only`. Fontes pin/Filuna EXACT runs. G4 NOT_RUN.

## 19. VEREDITO DO DONO t19u — 2026-11-05 23:33 Europe/Rome (22:33 UTC) — IMUTÁVEL

Registro: `FILUNA/approvals/FUND-VERDICT-2026-11-05-t19u.json`. **Aprovação com ressalvas, avanço bloqueado.** Autoriza só correção/testes da fundação no gate atual; **não** homologa etapa/release.

| Item | Estado |
|---|---|
| LEGAL-02 | checks documentais 7/7 PASS · **auditoria do pacote PARTIAL** (`FILUNA/legal/PACKAGE-INVENTORY-run42102469888.json`: 4062 arquivos, 75 binários, 26 sem proveniência verificada, FFmpeg avcodec/avformat BLOCKED até auditoria; relação instalador↔portable NOT_RUN) |
| QA-03 | validador + `gate_governance.py` (contrato `FILUNA/qa/required-cases.json` v2026-11-05.3): hoje G2/G3/G4 = **BLOCKED** |
| CORE-04 | **BLOCKED** (baseline binário Windows em execução/sem PASS) |
| DATA-05 | **BLOCKED** (AppData Windows real NOT_RUN) |
| Onda 01 | **BLOCKED**; nenhuma onda avança |
| G4 | NOT_RUN |

check 113/113, CMP 7/7 e testes unitários **não** são autorização para progredir. Dados sintéticos provam só o validador.

## 20. FUND-T07 baseline binário REAL Windows — 2026-11-05 22:47 UTC (23:47 Europe/Rome)

- Run: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42124288641 (success) · artifact `Filuna_baseline_gcode_windows` id 13458549645 · commit testado `6c844fb8`.
- Orca pin: run upstream 42037889437 @ a620d804 (head_sha verificado no job), orca-slicer.exe b51efb83…, OrcaSlicer.dll 372aeb32…
- Filuna: run 42102469888 @ 2dace0db (verificado), orca-slicer.exe 4b822731…, OrcaSlicer.dll f0914911… (binários distintos).
- CLI (igual aos testes upstream `tests/cli`): `--datadir <TEMP> --load-settings "BBL P1S 0.4;0.20mm Standard @BBL X1C" --load-defaultfila --slice 0 --outputdir <TEMP> 20mm_cube.obj`; exit 0/0; sem GUI, sem impressora.
- G-code bruto: 272882 B cada; diff = **1 linha** (`; generated by … 22:47:09` vs `22:47:10`). Normalizado idêntico sha 1884525d…
- Negativo real: E50→E50.5 na linha 836 → compare FAIL (detectado).
- **Caso FUND-T07 (1 fixture × 1 preset) = PASS com evidência.** CORE-04 permanece **BLOCKED**: STL/3MF/STEP, outros presets e signoff do dono NOT_RUN.
- Tentativas anteriores (falhas reais, corrigidas): 42122763283 (import path), 42123026960 e 42123239839 (fixture CRLF → hash mismatch bloqueou corretamente), 42124068177 (`--export-gcode` inválido nesta versão). Rebuild do pin 42122933198 **cancelado** (desnecessário: download do upstream funcionou com GITHUB_TOKEN). Branch `filuna/pin-win-a620d804` ficou criada, sem uso.
- Limitação conhecida: `gate_governance` exige commit da evidência == HEAD; cada commit novo invalida a evidência até nova execução (fail-closed, intencional).
