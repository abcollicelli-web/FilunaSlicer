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
