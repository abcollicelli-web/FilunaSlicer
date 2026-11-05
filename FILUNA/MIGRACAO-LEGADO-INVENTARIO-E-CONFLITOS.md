# FILUNA — GENEALOGIA, MATRIZ DE MIGRAÇÃO, CONFLITOS E FONTES

**FLN-HIST-2026-11-05.1** • Registro: 05/11/2026 • **Estado: inventário documental auditável; G0 ainda não homologado.**  
Objetivo: conservar decisões, experiências e recursos do Filuna anterior, diferenciando **fonte histórica, implementação existente, teste técnico, homologação física e release do novo fork**.

## 1. Repositórios com papéis distintos

| Sigla | Repositório / branch | Papel | O que é proibido concluir |
|---|---|---|---|
| S | \`abcollicelli-web/Filuna\` \`main\`, \`Filuna-Slicer/\` | regras do proprietário, histórico do site e contratos Grok | documentação visual ≠ aplicativo operante |
| W | \`abcollicelli-web/filuna-slicer\` \`main\` | cliente Windows anterior C#/.NET 10 + WebView2, Orca separado, Hub beta, serviços, adaptadores e módulos | build/QA Ubuntu W ≠ teste ou código do fork atual |
| F | \`abcollicelli-web/FilunaSlicer\`, \`filuna/rebrand-1\` | novo fork Orca 2.5.0-dev, C++17/wxWidgets, core nativo | presença de perfil/arquivo OEM ≠ integração/firmware homologado |
| Q | \`abcollicelli-web/FilunaSlicer\`, \`filuna/quality-master-20261105\` | documentação, arquitetura, governança, QA e cronograma | PR em draft ≠ branch final ou atualização instalável |
| O | \`OrcaSlicer/OrcaSlicer\`, upstream pin \`a620d804efbe74162c96ad29d755cdab2b823530\` | referência técnica e teste comparativo de core | liberar divergências de geometria e G-code sem especificação |

**Inventário por Git tree em 05/11:** S contém **61** arquivos MD/JSON em \`Filuna-Slicer/\`, W contém **66** documentos MD/JSON em \`docs/\`, 11 contratos em \`contracts/\` e três propostas em \`proposals/\`. Esses números são registros de árvore, **não prova de auditoria semântica integral do conteúdo ou do código de cada arquivo**. F possui milhares de fontes C++/resources e o inventário por hash desse código ainda deve ser consolidado.

**Infraestrutura:** \`/srv/site\` Ubuntu = site legado em produção; \`/srv/filuna-slicer-windows-new\` = antigo Windows/Hub; \`C:\Program Files\FilunaSlicer\` = instalação Windows informada pelo proprietário, **não inspecionada por SentinelX**. Nunca mover arquivos, DLLs, segredos ou banco do primeiro diretamente para o terceiro.

## 2. Documento mestre e precedências

| Regra / referência | Caminho literal no repositório S | Como migrar |
|---|---|---|
| Continuidade | \`14-CONTINUIDADE-INTEGRAL.md\`; \`15-LINHA-DO-TEMPO.md\` | cronologia e decisões, contrastar revisões |
| Padrão visual inicial | \`12-REFERENCIA-VISUAL-OFICIAL.md\` | usar somente o que não foi atualizado em 03/11 e 05/11 |
| Home homologada | \`18-HOME-GROK-2026-11-03.md\` e \`GROK-GITHUB/ATUALIZACOES/2026-11-03-ESTRUTURA-HOME-PREPARAR-CALIBRACAO.md\` | estrutura Home final, dados reais, último trabalho, faixas e menus |
| Preparar | \`GROK-GITHUB/ATUALIZACOES/2026-11-03-MESTRE-PREPARAR.md\` | contrato visual PROPOSTO: reimplementar com viewport e estado Orca reais |
| Dispositivos | \`GROK-GITHUB/ATUALIZACOES/2026-11-03-MESTRE-DISPOSITIVOS.md\` | PROPOSTO: interface vinculada ao DeviceBridge e suas capabilities |
| Paramétricos | \`GROK-GITHUB/ATUALIZACOES/2026-11-03-MESTRE-PARAMETRICOS.md\` | PROPOSTO: galeria em cards e item com ferramentas próprias |
| Auto-update | \`25-REGRA-MESTRE-ATUALIZACOES-2026-11-03.md\` | A/B/C/D, classe por impacto, pacote cumulativo, hash/assinatura/rollback |
| Operação após plano | \`11-ACESSO-POS-ASSINATURA.md\` | manter Orca livre, estoque histórico/export e permissões premium separadas |
| Catálogo protegido | \`20-MODELOS-3D-IMPRESSAO-PROTEGIDA.md\` | print-only sob licença, sem export mestre pela UI; arquivos próprios livres |
| Multi-placa | \`23-QA-LUA-ORCA-MULTIMESA-2026-11-03.md\` | cor/material compatível por placa, Z=0, ordem e preservação 3MF |
| Segurança | \`09-SEGURANCA-LICENCAS.md\`, \`10-APIS-E-QUALIDADE.md\` | governança, AGPL, controle de segurança e QA |

**Contratos herdados W mais importantes:** \`docs/AGENTS.md\`, \`PROJECT_STATE.json\`, \`CONTINUAR-PROJETO-FILUNA-SLICER.md\`, \`GROK_CONTRACT.md\`, \`ROADMAP.md\`, \`SYSTEMS_MAP.md\`, \`ENGINE_CONTRACT.md\`, \`HOMOLOGACAO-SITE.md\`, \`PENDENCIAS.md\`, \`PLATE-LAYOUT-CONTRACT.md\`, \`PROTOCOLOS-IMPRESSORAS.md\`, \`INTEGRACOES-IMPRESSORAS.md\`, \`FUP-003-DIAGNOSTICO-DISPOSITIVOS.md\`, \`PARAMETRICOS-ANALISE.md\`, \`MODELOS-3D.md\`, \`SECURE-CATALOG-PRINT-CONTRACT.md\`, \`FILUNA-ID.md\`, \`IA-SUPORTE-PESQUISA.md\`, \`UPDATE-RELEASE-POLICY.md\`, \`QA-LUA-LITOFANIA-2026-11-03.md\`, \`QA-0.1.9-IMPRESSORAS-NATIVAS-2026-11-03.md\`, \`ORCA-2.5-INTEGRACAO-NATIVA.md\`, \`ORCA-2.5-PELO-FATIADOR.md\`, \`VALIDACAO-0.2.37-WINDOWS.md\`, \`AUDITORIA-SEGURANCA-2026-11-05.md\`, \`D5-D6-CORRECAO-INDIVIDUAL.md\`.

Esses documentos possuem STATUS históricos e alguns se contradizem porque refletem fases de 03–05/11. Não copiar qualquer símbolo verde sem conferir **data, build, ambiente e evidência física**.

## 3. Funcionalidades em ondas e sua classificação de migração

| ID | Subsistema legado / base real | Prioridade & estratégia | Dependências no fork | Qualidade mínima |
|---|---|---|---|---|
| MIG-01 | Orca 2.5 real, geometria, G-code, manipulação e perfis | **P0 PRESERVAR** \`src/libslic3r\`; não reimplementar | upstream pin, CI, baseline | G-code regressivo, STL/3MF/STEP, Windows |
| MIG-02 | Login/planos Filuna ID | **P1 REPROJETAR** sessão/entitlements, separado do Orca | API/conta, segurança, storage | assinatura nunca bloqueia core |
| MIG-03 | Home e identidade Grok | **P1 REIMPLEMENTAR** layout/tema/caminhos | inventário, assets originais, dados reais | prints e aceite visual |
| MIG-04 | Preparar e Visualização | **P0 INTEGRAR** Orca real com painel Filuna, um só estado | Orca core, eventos/presets, GUI shell | mudança de UI tem efeito no G-code |
| MIG-05 | Calibração do Orca | **P1 PRESERVAR + TEMA**; recursos iguais aos originais | Preparar/Perfis | testes de calibração reais |
| MIG-06 | Dispositivos | **P0 CRIAR** DeviceBridge/AdapterRegistry e telemetria | auth local, modelo/firmware, protocolo | sem falso ACK, print autorizado |
| MIG-07 | Impressoras nativas (10 fabricantes) | **P1 AMPLIAR** modelo por modelo | MIG-06, código OEM licenciado, firmware físico | camera/pause/filamento por capability |
| MIG-08 | Perfis impressora/filamento/processo, presets manuais | **P0 PRESERVAR / CORRIGIR** herança e editor real | Orca core, storage | presets compatíveis, sem “Novo PLA” fake |
| MIG-09 | Organização multi-placa/cores | **P0 PADRONIZAR** peças/material e colisões | Orca/Perfis/Migração | 3MF com placas, Z0, G-code correto |
| MIG-10 | Modelos 3D do site, catálogo print-only | **P2 REIMPLEMENTAR** autorização e política de export | Filuna ID, Orca, segurança | sem export mestre pela UI, threat model |
| MIG-11 | Galeria paramétricos (22) | **P2 REIMPLEMENTAR** por slug; manter os não aprovados separados | shell, motor local, contrato de placa | cada gerador gera arquivo válido |
| MIG-12 | Lua Litofania, Árvore de Natal, Chaveiro Botton e imagem→3MF | **P2 MIGRAR CONTRATO E BENCHMARK**, não copiar módulos C# sem teste | geometria local, image pipeline, Orca | malha, encaixe, qualidade, peça física |
| MIG-13 | Ferramentas SVG, 3MF colorido, reparo, preço, foto-linear IA | **P2 SEPARAR EM MÓDULOS** | engine local, storage e proxy IA opcional | saída real e teste específico |
| MIG-14 | Estoque de bobinas / QR / NFC | **P2 ADAPTAR DADOS** | conta, filamento real, devices, ledger | baixa por job, dados consistentes |
| MIG-15 | Ateliê: clientes, orçamentos, pedidos, financeiro, relatórios | **P3 CONSTRUIR SOBRE ESTOQUE E PRODUÇÃO** | conta, banco, ledger, custos, produção | integração end-to-end e auditoria |
| MIG-16 | Produção: fila, avisos, histórico, agendamento | **P2 REAPROVEITAR CONTRATO** | DeviceBridge, G-code, Estoque | nunca iniciar impressão automaticamente |
| MIG-17 | E-commerce Mercado Livre e etiquetas | **P3 ADAPTAR API** | conta, estoque, pedidos/produção, autorização | OAuth oficial, etiqueta real |
| MIG-18 | Assistente IA local/servidor, GPT/Claude/Gemini/Grok | **P3 ADAPTAR CAMADAS**, não chaves embutidas | API gateway, conta, quota, permissões | respostas/arquivos reais, segredos protegidos |
| MIG-19 | Atualização A/B/C/D assinada e restart | **P0 NOVO UPDATER C++/Windows**, contrato W como referência | packaging, trust, storage | rollback injetado e Windows real |
| MIG-20 | Diagnóstico FUP-001/002/003, Telegram do proprietário | **P2 TRANSPORTAR CONTRATOS**, não copiar sender C# diretamente | Devices, logging, API, privacidade | sem alertas duplicados por offline |
| MIG-21 | Site/mobile vitrine/conta/estoque/impressão online | **P3 INTEGRAR POR API**, não acoplar core Orca | conta, api auth, adapters, sync | privacidade, revogação, dispositivos reais |
| MIG-22 | Segurança, licença AGPL, privacidade, CRA | **P0 TRANSVERSAL** | nenhum (começa na origem) | obrigação normativa e docs auditáveis |
| MIG-23 | Acessibilidade, performance, localization | **P0 TRANSVERSAL** | design system, shell, CI | DPI, teclado, telas e perfis de máquina |
| MIG-24 | Plugin próprio Filuna para GPT, Claude, Gemini, Grok | **P3 REPROJETAR** APIs MCP e integrações | auth, Hub, quotas, segurança | contratos e testes reais de acesso |
| MIG-25 | Ofertas, catálogo de parceiros, busca de modelos | **P3 REAPROVEITAR REGRAS**, não função crítica do slicer | Hub, catálogo, API/mercado | legalidade e dados autênticos |
| MIG-26 | Atualização de versões alpha/beta/estável dos OEM/Orca | **P1 DESENVOLVER MONITOR**, somente leitura e proposta | upstream pin, licença, test harness | PR/relatório; nunca auto-merge |

**Não afirmar que módulos W 0.2.x já estão homologados no fork F**. A existência do arquivo \`clients/windows/Filuna.Desktop/ui/atelie.js\` prova implementação antiga de UI, não integração real no fork.

## 4. Decisões mais recentes que superam propostas antigas

| Conflito | Fontes / evidência | Precedência / decisão |
|---|---|---|
| Vários motores Orca+Prusa+Cura+Kiri | S \`04-MOTORES.md\`, W \`docs/ENGINE_CONTRACT.md\` | **SUPERADO por ordem atual 05/11**: Orca único principal e integral, adaptadores/protocolos OEM (Bambu/Elegoo/Anycubic/Creality/Flashforge/QIDI/Snapmaker/Prusa/UltiMaker/Raise3D) |
| Interface Orca externa/embutida | W \`docs/PENDENCIAS.md\`: 0.2.40 revertida por incompreensão; W \`ORCA-2.5-INTEGRACAO-NATIVA.md\` | **REJEITAR** janela Orca dentro de shell falso. Novo fork é produto C++ com mesa/GUI nativa, camada Filuna com um estado real |
| Hero Home e busca antiga | S \`12-REFERENCIA-VISUAL-OFICIAL.md\` contra revisão 03/11 | nova disposição aprovada de 03/11 prevalece |
| Cores exatas do tema | S 03/11 vs imagens 05/11 e W \`PROJECT_STATE.json\` | **CONFLITANTE PARCIAL**. Identidade visual 05/11 prevalece como referência, mas amostra JPEG não substitui tokens finais sem homologação |
| Planos e valores | S \`11-ACESSO-POS-ASSINATURA.md\` vs W \`FILUNA-ID.md\`, W \`HOMOLOGACAO-SITE.md\` | regra Orca livre é fixa. Preço, tolerância, cobrança, moeda, ciclo e política jurídica requerem decisão mais recente e testes; não codificar número histórico às cegas |
| Versão C# 0.2.42 “publicada” | W \`PENDENCIAS.md\` indica publicada após fases revertidas | **NÃO** usar publicação beta de W como homologação do F; confirmar resultado pelo binário real e testes físicos |
| Modo LAN/conta do fabricante | Nova ordem atual + SimplyPrint + W \`PROTOCOLOS-IMPRESSORAS.md\` | buscar exaustivamente modos oficiais e públicos autorizados. Porta exposta ≠ permissão. Sem bypass de SDK privado |
| Marca e assets | ícones provisórios F do fork vs cinco originais recentes | versão 05/11 vinculante para aparência; binários/hashes originais precisam entrar antes de G1 PASS |
| Portátil/instalador | W updater C# e F NSIS rebrand | implementar updater próprio F; somente a regra de segurança A/B/C/D migra, **não o binário C#** |
| Catálogo protegido versus arquivos do cliente | S \`20-MODELOS...\`, W \`SECURE-CATALOG-PRINT-CONTRACT.md\` | origem e licença determinam export; proteção absoluta no PC é impossível |

## 5. Achados e defeitos que não podem renascer

1. **Perfis W 2.4.2:** \`docs/ORCA-2.5-INTEGRACAO-NATIVA.md\` relata erro de herança em 128/619 configurações do CLI e substituição incorreta de PETG por PLA. 0.2.42 registra redução de diferenças restantes para 25 após resolver presets. **No fork F testar o estado nativo atual e não portar a solução C#**.
2. **Dispositivos F:** \`src/slic3r/Utils/OrcaPrinterAgent.cpp\` tem funções sem execução que retornam \`BAMBU_NETWORK_SUCCESS\`. **P0 eliminar falso sucesso**, antes de habilitar DeviceBridge.
3. **Multi-placa/litofania:** layout deve preservar cores, limites, tolerâncias, orientação, Z=0 e metadados 3MF; não reorganizar placas automaticamente depois de gerar. W \`QA-LUA-LITOFANIA-2026-11-03.md\` só tem QA de CLI/Ubuntu e pede impressão real.
4. **Windows W QA4–QA9:** exemplos de builds com warnings, partes decorativas, Orca externo, reinício e atualizador parcialmente testados. Não declarar homologação com base em 14/17/28 testes JS/Ubuntu.
5. **Instalação F:** ícone, publisher SoftFever, exe interno Orca, coexistência de AppData e \`orcaslicer://\`; apenas compatibilidade deliberada pode preservar identificador técnico.
6. **FUP W:** telemetria pode ser ruidosa ao desconectar impressoras; separar offline esperado de exceção inesperada, deduplicar por instalação/firmware e redigir dados.
7. **Câmeras OEM:** stream, porta, autorização e formato variam; câmera não encontrada é indisponível, não falha fatal do core.
8. **Anycubic:** perfil existente e upload na geração W não comprovam comando de iniciar. Exigir ACK semântico.
9. **Legado em rede:** site atual tem dados e tokens; não transferir banco/cofre, log/serial e segredos sem aprovação e DPA/DPIA quando aplicável.
10. **Proteção catálogo:** não desabilitar \`Save As\`/export para arquivos do cliente; testes de caminho de origem e entitlement obrigatório.

## 6. Matriz de status, auditoria e regras de decisão

Cada linha de MIG-01 a MIG-26 deve receber, na execução de G0/G1:
\`[source_repository, source_path, source_blob_sha, source_revision_date, user_decision, status(proposed/approved/implemented/tested/revoked/conflicted), target_module_id, dependency_ids, test_case_ids, owner_approval, acceptance_evidence]\`.

**Critério de encerramento G0:** manifestos de fonte completos com repositório, caminho, hash, status; todos conflitos com ADR; todas as 26 linhas mapeadas para nodes de dependência e suites; aprovação explícita dos tokens e do escopo do primeiro release; nenhum documento normativo órfão.

## 7. O que foi verificado e o que continua pendente

**Verificado diretamente pelo GitHub/SentinelX:**
- Árvores atuais dos dois repositórios históricos, incluindo **61 arquivos S** e **66 documentos W**; os títulos, caminhos, SHAs, subpastas e papéis de cada repositório.
- Leitura de contratos centrais citados, código crítico do novo fork (\`OrcaPrinterAgent.cpp\`, \`ElegooLink.cpp\`, \`Flashforge.cpp\`, \`PrintHost.cpp\`, \`NetworkAgentFactory.hpp\`, \`IPrinterAgent.hpp\`) e relatos históricos sobre QA.
- Documentos OEM AGPL e fontes oficiais referenciados na arquitetura de fabricantes.
- Normas internacionais e UE por fontes primárias, com distinção entre requisito legal/certificação opcional.

**Pendente para declarar “leitura de tudo” / G0 PASS:**
- Inventário semântico completo **dos 61 arquivos S, 66 W, contratos/propostas e código efetivamente vinculado**, com extração linha por linha de exceções e conferência do que ainda prevalece, inclusive mudanças após 05/11.
- Leitura integral do conjunto de projetos e dados do site \`/srv/site\` que dependam de migração; **somente com escopo e segurança apropriados**, sem copiar dados pessoais.
- Execução do novo binário Windows real, leitura de versão, inspeção dos cinco assets originais já no Git, screenshots funcionais.
- Firmware e impressoras piloto de todos os dez fabricantes para homologar recursos e comparação física.
- Conflitos de precificação, termos legais, licença/SDK OEM e definição de tokens da paleta.

**Transparência:** este inventário é uma **base versionada para auditoria completa**, não a alegação de que cada documento histórico foi aprovado ou que todos os módulos estão implementados.  
**Bloqueio:** até cumprir G0, nenhuma promoção de UI/DeviceBridge como concluído. Pesquisa/documentação e correção da própria fundação podem continuar.


## 8. Adendo G0 — leitura prioritária 05/11 (Europe/Rome)

Inventário detalhado de 13 fontes lidas com hashes e REQ: [G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md](G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md). Confirma CF da seção 4 (quatro motores SUPERADO; Home 03/11 a reimplementar; print-only; updates A/B/C/D). Não esgota as 141 fontes.


### Conflitos adicionais após leitura 141 (05/11)

Ver [G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md](G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md) CF-06..08 (updates multi-motor, pendências W não herdáveis, Filuna ID/cofre).

## Adendo G0 — rastreabilidade conflitos + SBOM (2026-11-05 23:07:34 CET)

Detalhe operacional: [G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md](G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md).  
SBOM inicial parcial: [sbom/README.md](sbom/README.md).  
Correção: cobertura do manifesto 141 ≠ 141 leituras semânticas integrais.

