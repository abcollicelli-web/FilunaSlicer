# Filuna — mapa de migração da personalização aprovada

**Registro inicial:** 2026-11-05 • **Status:** inventário G0, NÃO homologação do novo aplicativo.  
**Base alvo:** `abcollicelli-web/FilunaSlicer`, `filuna/rebrand-1`, SHA inicial `2dace0db86d305bec5b9627179f265012e4f5069`.  
**Legado de documentação:** `abcollicelli-web/Filuna`, `main`.  
**Legado de código:** `abcollicelli-web/filuna-slicer`, `main` (C#/.NET; não copiar diretamente).

## Fontes com precedência

| Fonte GitHub (prefixo `Filuna-Slicer/` no antigo `Filuna`) | Uso | Status documental |
|---|---|---|
| `14-CONTINUIDADE-INTEGRAL.md` | Contexto e decisões, não presumir status atual | Fonte mestre histórica |
| `12-REFERENCIA-VISUAL-OFICIAL.md` | Hierarquia visual, tokens, referência Grok e capturas | Referência de design aprovada |
| `18-HOME-GROK-2026-11-03.md` e `19-PUBLICACAO-PRIVADA-HOME-0.1.1.md` | Home visual aprovada; QA beta C# não equivale a QA no fork | Visual aprovado, funcional do novo pendente |
| `GROK-GITHUB/ATUALIZACOES/2026-11-03-ESTRUTURA-HOME-PREPARAR-CALIBRACAO.md` | Mapa detalhado Home, Preparação, Calibração | Contrato visual PROPOSTO; Home aprovada posteriormente |
| `GROK-GITHUB/ATUALIZACOES/2026-11-03-MESTRE-PREPARAR.md` | Preparação, menus, controles e parâmetros | PROPOSTO |
| `GROK-GITHUB/ATUALIZACOES/2026-11-03-MESTRE-DISPOSITIVOS.md` | Dispositivos, painel e protocolos | PROPOSTO |
| `GROK-GITHUB/ATUALIZACOES/2026-11-03-MESTRE-PARAMETRICOS.md` | Galeria/item e catálogo por slug | PROPOSTO |
| `25-REGRA-MESTRE-ATUALIZACOES-2026-11-03.md` | Classes A/B/C/D, assinatura/rollback, bloqueio | Regra APROVADA |
| `11-ACESSO-POS-ASSINATURA.md` | Core livre, offline, estoque leitura | Regra de negócio a preservar |
| `20-MODELOS-3D-IMPRESSAO-PROTEGIDA.md` | Diferenciar catálogo print-only de arquivo externo | Regra a preservar, implementação pendente |
| `08-ETAPAS.md` e `10-APIS-E-QUALIDADE.md` | Portões, matriz QA e contratos de interfaces | Planejamento; integrar com qualidade atual |
| `GROK-GITHUB/LEIA-PRIMEIRO-GROK.md` e `GROK-GITHUB/ESTADO-DO-PROJETO.md` | Quem registra design e diferença entre build/homologação | Governança e estado histórico |

**Precedência visual concreta:** `12-REFERENCIA-VISUAL-OFICIAL.md` descreve uma Home anterior com busca e ações Abrir/Novo; a revisão específica de 03/11 em `2026-11-03-ESTRUTURA-HOME-PREPARAR-CALIBRACAO.md` proíbe esses componentes nessa Home. A Home de 03/11 foi posteriormente aprovada pelo proprietário em `18-HOME-GROK-2026-11-03.md`. Para recriar a Home usar a revisão de **03/11**, não combinar as duas.

## Primeiro contrato visual para o fork (não implementar botões simulados)

- **Shell:** app-bar vinho `56px`; navegação lateral vinho `208px`; área central creme e cards brancos. Home adotou seis itens laterais: Recentes, Modelos 3D, Paramétricos, Ferramentas, Estoque, Ateliê. Barra superior: Início, Preparação, Visualização, Dispositivo, Projeto, Calibração; conta na barra superior. Temas: original, azul, dark. Valores exatos de cor por região no contrato 03/11: `berry #64184a`, `berry-bar #7a215c`, `berry-active #c43b78`, `magenta #c2185b`, `orange #ff6b1a`, `cream #f7f2f4`, `charcoal #3a2a32`. Assets devem seguir identidade oficial e atualizações aprovadas.
- **Home:** destaque central com último trabalho **real** fatiado e painel de impressão; abaixo, faixas **Recentes → Modelos → Ferramentas** em vez de três grades indistintas. O gato rosa, impressoras `Filuna Core/Prime`, tempo `3h42` e custos dos esboços eram **DEMONSTRAÇÃO**; nunca exibir como estado real de máquina ou impressão.
- **Não adicionar:** campo de pesquisa, Abrir/Novo projeto no destaque e botão Fatiar na Home quando não existirem no contrato final de 03/11. Ações de impressão não podem operar sem artefato real e confirmação.
- **Preparação:** preservar a mesa 3D **real do Orca** em vez de trocar por SVG/mockup; controles e níveis **Simples/Avançado/Especialista**, inicial Avançado; Global/Objetos; perfil real; configurações e presets do Orca sem perder campos. Contrato técnico completo no MESTRE-PREPARAR; status PROPOSTO.
- **Dispositivos:** aba/tela própria com Impressoras/Filamentos/Fila e histórico, busca LAN conforme protocolo real; nunca confundir modelos ilustrativos/endereço de exemplo com dispositivos conectados. Moonraker, OctoPrint, Bambu LAN, PrusaLink/Connect, RRF, Elegoo SDCP, Snapmaker e Marlin requerem adaptadores/capacidades próprias. Status PROPOSTO, implementação nova incompleta.
- **Paramétricos:** galeria apenas em cards, sem lista esquerda; item com `Voltar para galeria`, ferramentas só do slug, render contido e download apenas após gerar arquivo local real. Contrato 03/11 é PROPOSTO; catálogo publicado e dados reais precisam ser conferidos novamente antes de card/ação.
- **Ateliê, Estoque e Ferramentas:** não confundir componentes ilustrativos do antigo cliente com módulos plenamente implementados; requisitos completos em documentos próprios do legado. Não iniciar antes da etapa anterior passar.
- **Licença e acesso:** Orca livre permanece em qualquer estado de assinatura e sem Internet; ferramentas extras elegíveis podem depender de plano; a cessação não apaga dados e o estoque fica leitura/exportação. Sem plano nunca desabilitar o motor open source.
- **Catálogo protegido:** modelos próprios print-only não exportam malha; modelos externos e paramétricos criados pelo cliente mantêm exportação. Exige threat model e QA; impossível prometer proteção absoluta da geometria no PC.
- **Atualização:** classes A/B/C/D com manifesto assinado/hash, rollback e consentimento conforme contrato 25; o código do atualizador C# antigo não é reutilizável diretamente em C++.

## Primeira onda do novo fork

| ID | Requisito verificável | Estado no fork no início deste registro | Gate |
|---|---|---|---|
| F-001 | Orca completo preservado, 3MF/G-code/perfis sem regressão | BASE PRESENTE; teste comparativo não demonstrado | G3 |
| F-002 | Pasta de dados Filuna, coexistência Orca | MUDANÇA IMPLEMENTADA; teste físico pendente | G3/G4 |
| F-003 | Home visual 03/11 fiel e funcional | NÃO IMPLEMENTADO; rebrand somente | G1/G2/G4 |
| F-004 | Preparação com viewport nativo, sem duplicar motor | BASE ORCA EXISTE; contrato Filuna não implementado | G1/G2/G3 |
| F-005 | Dispositivos com adaptação por fabricante | Parte do Orca presente; camada Filuna ausente | G1/G3/G4 |
| F-006 | Paramétricos e geradores locais | AUSENTES no fork | G1/G2/G3 |
| F-007 | Ateliê/estoque/planos separados do core | NÃO DEMONSTRADOS no fork | G1/G2/G3 |
| F-008 | Atualizador seguro para esta base C++ | NÃO DEMONSTRADO; NSIS atual não assinado | G3/G4/G5 |
| F-009 | Agentes sem falso sucesso | RISCO EXISTENTE: `src/slic3r/Utils/OrcaPrinterAgent.cpp` contém stubs que retornam sucesso | G2/G3 |
| F-010 | Elegoo Centauri Carbon 2 e Anycubic Kobra 3 V2 | Perfis existem; aceites físicos ainda vazios | G4 |

**Próxima execução permitida:** concluir G0 (inventário de todos os docs relevantes, conflitos e evidências) e validar G1 antes de modificar Home/GUI. Nesta entrega as tabelas são um começo rastreável do inventário, não alegação de leitura exaustiva dos ~17 mil recursos ou de conclusão G0.

**Risco de migração:** o legado C# possuía Orca externo e atualização C# específica; o novo fork é Orca 2.5.0-dev C++/wxWidgets. Não copiar DLL, agente, binário ou comando antigo do servidor; portar contratos e implementar sobre APIs reais do novo fork.

## Identidade visual atualizada pelo proprietário em 05/11/2026

O proprietário anexou novas referências para logo “Filuna 3D”, amostras cromáticas, ícone F e duas imagens da mascote 3D. A identidade agora está catalogada em [IDENTIDADE-VISUAL-E-MASCOTE.md](IDENTIDADE-VISUAL-E-MASCOTE.md) com hashes e manifesto de origem. Esta referência mais recente orienta os novos assets de marca sem revogar automaticamente a disposição de telas da Home homologada de 03/11. Os tons fotografados devem ser tratados como aproximações até confirmação dos tokens finais; **G1 fica bloqueado enquanto os binários oficiais e os critérios de fidelidade não estiverem integrados e aprovados**.
