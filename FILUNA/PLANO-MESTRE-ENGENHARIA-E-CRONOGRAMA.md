# FILUNA SLICER — PLANO MESTRE DE ENGENHARIA, EXECUÇÃO E CRONOGRAMA

**ID** FLN-ENG-MASTER-2026-11-05-v1 · **Registro** 05/11/2026 · **Estado** PLANEJAMENTO CONTROLADO; implementação NÃO iniciada por este documento.  
**Produto** Filuna Slicer para Windows, fork nativo OrcaSlicer 2.5.0-dev.  
**Base técnica alvo** [OrcaSlicer SHA a620d804efbe74162c96ad29d755cdab2b823530](UPSTREAM_PIN.md); evolução Filuna isolada em PR/branch, nunca no \`upstream-mirror\`.  
**Decisão do proprietário** D-FLN-ENGINE-05NOV: **UM motor de fatiamento principal e integral — OrcaSlicer — com recursos e código de comunicação legalmente reutilizáveis de fatiadores dos fabricantes**, para reconhecimento automático, status, câmera, filamentos e comandos. O plano antigo de embutir quatro motores deixa de ser o requisito de produto desta versão. Não integrar PrusaSlicer, CuraEngine nem Kiri:Moto como backends de fatiamento por herança de planos antigos. O código de rede desses projetos pode ser analisado quando adequado e legal.


## ANTES DE QUALQUER IA OU COLABORADOR COMEÇAR A TRABALHAR

**Leia obrigatoriamente, nesta ordem:**

1. **[REGISTRO-CONTINUIDADE-ETAPAS.md](REGISTRO-CONTINUIDADE-ETAPAS.md)** — estado operacional, etapas já feitas, gates, builds, próxima ação exata (não depender de chat nem de disco local).
2. **[REGRA-MESTRE-QUALIDADE.md](REGRA-MESTRE-QUALIDADE.md)** — portões G0–G6 e bloqueios.
3. **Este plano mestre** e o restante do dossiê listado na seção 10.

Sem essa leitura, não alterar código, UI, CI, módulos de fabricante, releases nem marcar gates PASS. Build verde ≠ teste Windows ≠ impressão física ≠ homologação.


## 0. Critério de sucesso e controle

Objetivo: aplicativo de qualidade profissional, visual e funcionalmente Filuna, fiel aos contratos aprovados e com uso real e seguro das impressoras. "100% aprovado" significa **100% dos testes obrigatórios EXECUTADOS e PASS, associados ao SHA, binário, ambiente, modelo e firmware do escopo em teste**, não inexistência metafísica de falhas futuras.

- Fonte normativa principal: [REGRA-MESTRE-QUALIDADE.md](REGRA-MESTRE-QUALIDADE.md). **G0 → G1 → G2 → G3 → G4 → G5 → G6**: cada portão exige 100% dos critérios obrigatórios PASS; FAIL, BLOCKED, NOT RUN, evidência falsa/incompleta ou teste pulado obrigatório = 0% de promoção.
- Qualquer falha reabre o gate afetado e todas as aprovações de dependências impactadas; regressão integral proporcional ao risco. Sem "deixar para depois" requisito incluído no escopo daquela versão. Uma função não pronta deve estar fora do escopo da versão, claramente marcada **não disponível**, nunca simulada como operacional.
- Para release "compatível com dez marcas", exige **dez marcas homologadas em pelo menos um modelo real cada**, e função por função de cada modelo declarada e testada. Suporte a 10 marcas ≠ suporte a todos os modelos/funções dessas marcas. Mais modelos só são anunciados quando passarem.
- Cada etapa precisa de responsável, versão, fontes, dependências, critérios quantitativos, teste positivo/negativo, evidência, risco, saída e aprovação do proprietário. Definição de pronto sem campo essencial = BLOQUEADA.
- Documentação, implementação, CI, ensaio real, homologação e publicação são fatos diferentes. O sistema não poderá ser chamado "versão final perfeita" só por compilar; defeitos podem surgir depois e exigem monitoramento, atualização e rollback.
- Restringir implantação ao canal de testes enquanto faltarem segurança, assinatura, 10 matrizes físicas ou homologação; nenhum deploy no site antigo.

## 1. Escopo e decisões de arquitetura

| Domínio | Decisão obrigatória | Fronteira |
|---|---|---|
| Fatiamento e geometria | Core **OrcaSlicer** nativo, sem cópia paralela de outro slicer e sem Orca instalado no PC como dependência | \`src/libslic3r\`; perfis, G-code, 3MF, calibração, preview, suportes e multi-placa preservados |
| Visual | Camada Filuna com logo Filuna 3D, ícone F, paleta e mascote fornecidos; Home e telas conforme contratos Grok mais recentes | Pontos de extensão mínimos e registráveis; não trocar viewport Orca por maquete |
| Dispositivos | **Filuna DeviceBridge** com descoberta, identificação, autenticação e capacidades por modelo/firmware | \`FILUNA/modules/<vendor>/\`; priorizar interfaces existentes Orca e fontes OEM auditadas |
| Fabricantes fase de cobertura 10 | Elegoo, Anycubic, Bambu Lab, Creality, Flashforge, QIDI Tech, Snapmaker, Prusa, UltiMaker, Raise3D | É uma lista de **prioridade de integração**, não ranking comprovado de vendas mundiais. Outros fabricantes por protocolos genéricos depois |
| Recursos de dispositivos | Estado real, fila, conexão, envio, início confirmado, pausa, retomar, cancelar, câmera, temperatura, material/cor e sistemas multimaterial | Cada operação deve estar suportada **por modelo e firmware**; erros explícitos em vez de sucesso falso |
| Atualização | A/B dados e módulos; C Filuna UI/código; D binário/motor/runtime somente quando inevitável | App instala com consentimento, assinatura, hash, staging, salvamento, reinício controlado e rollback |
| Conta e cobrança | Orca e funções AGPL sempre livres/offline; serviços premium Filuna condicionais **somente se legalmente separáveis** | Nunca condicionar fatiamento a login/rede/assinatura |
| Negócio | Ateliê, Estoque, Produção, E-commerce, Ferramentas, Paramétricos, Modelos 3D, IA, Integrações e Meu Plano | Reimplementar em ondas a partir dos contratos legados; nenhum servidor fatiador obrigatório |
| Site/Mobile | Vitrine, conta, assinatura, catálogo, sync, gestão/monitoramento | Não acoplar o core Orca ao site legado; APIs versionadas e autorização explícita |
| Segurança/licença | SBOM, atribuições e fonte AGPL; sem copiar bibliotecas fechadas (Bambu/Flashforge) | O acesso autorizado de terceiros e a licença do código são avaliações distintas |

**Padrões de qualidade inegociáveis:** estado único entre GUI e motor; nenhuma configuração mentirosa; nenhuma impressão involuntária; preservação de projetos e perfis; erros em português com causa real; UX acessível, responsiva e testada em Windows; plugin de fabricante não pode executar comando incompatível ou escapar do isolamento.

## 2. Status de referência — não confundir gerações

**A. Site anterior:** \`abcollicelli-web/Filuna\`, documentação \`Filuna-Slicer/\` e módulo legado do site. Fonte de requisitos, UI, histórico e decisões, NÃO base de código a importar sem licença/testes.  
**B. Cliente Windows anterior:** \`abcollicelli-web/filuna-slicer\` \`main\`, C#/.NET 10/WinForms + WebView2, Orca separado e serviços Hub; há interfaces, adaptadores, contratos, protótipos, testes sintéticos e várias versões 0.2.x. Alguns relatórios indicam funções em produção beta **sem homologação física**. Não herdar certificados de teste para código novo.  
**C. Produto atual:** \`abcollicelli-web/FilunaSlicer\`, fork C++17/wxWidgets Orca 2.5.0-dev; rebrand e packaging parciais; DeviceBridge Filuna e Hub visual inexistentes; \`FILUNA/TEST-ACCEPTANCE.md\` pendente.  
**D. Instalação Windows relatada:** \`C:\Program Files\FilunaSlicer\`, ~581 MB, \`orca-slicer.exe\` launcher, \`OrcaSlicer.dll\` ~148 MB, \`%APPDATA%\Filuna\`, ainda visual Orca, pacote sem assinatura. Esses dados vêm do proprietário, não de inspeção direta deste Windows.  
**E. Build Windows da base rebrand:** GitHub run [`42102469888`](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42102469888) concluiu **success** em `filuna/rebrand-1` (`2dace0db`) e publicou `Filuna_Windows_x64_installer` + `Filuna_Windows_x64_portable`. Isso **não** é QA Windows nem homologação física. Estado operacional: [REGISTRO-CONTINUIDADE-ETAPAS.md](REGISTRO-CONTINUIDADE-ETAPAS.md).

## 3. Regras documentais de precedência e conflito

1. Segurança, obrigações de licenciamento e ordem expressa atual do proprietário são bloqueantes.
2. Identidade atual fornecida em 05/11: [IDENTIDADE-VISUAL-E-MASCOTE.md](IDENTIDADE-VISUAL-E-MASCOTE.md); os cinco originais têm hashes, mas binários ainda não constam na branch. **Paleta amostrada de JPEG não é token HEX homologado**.
3. Estrutura Home aprovada 03/11: \`Filuna-Slicer/18-HOME-GROK-2026-11-03.md\`, complementada pela \`GROK-GITHUB/ATUALIZACOES/2026-11-03-ESTRUTURA-HOME-PREPARAR-CALIBRACAO.md\`. Divergências de Home mais antigas devem ficar arquivadas, não misturadas.
4. Telas específicas do Grok \`2026-11-03-MESTRE-PREPARAR.md\`, \`...MESTRE-DISPOSITIVOS.md\`, \`...MESTRE-PARAMETRICOS.md\`: **propostas técnicas de UI**; só passam a HOMOLOGADAS no fork quando implementadas e aprovadas.
5. Documentos antigo site \`04-MOTORES.md\`, \`08-ETAPAS.md\`, \`docs/ROADMAP.md\`, \`PROJECT_STATE.json\` são úteis como histórico; a opção **quatro motores** foi expressamente SUPERADA por esta decisão de motor Orca único. Não apagar as fontes; criar registro de superação.
6. Regras de acesso, proteção do catálogo, placas PEI, atualizador, APIs e homologação antigas são invariantes, sujeitas a estudo de compatibilidade real no novo Orca.
7. Qualquer conflito não resolvido por especificidade ou revisão expressa vira item \`CONFLITANTE\` e impede passagem do gate afetado.

## 4. Ciclo padrão de desenvolvimento de uma funcionalidade (obrigatório)

\`DESCOBRIR → INVENTARIAR → DESENHAR CONTRATO → HOMOLOGAR ESCOPO → IMPLEMENTAR PEQUENO → TESTAR NEGATIVOS/POSITIVOS → INTEGRAR → REGREDIR → TESTAR WINDOWS → TESTAR HARDWARE QUANDO APLICÁVEL → HOMOLOGAR → EMPACOTAR → VALIDAR UPDATE/ROLLBACK → PUBLICAR COM APROVAÇÃO → MONITORAR\`.

Cada história cria no GitHub:
- **REQ**: ID imutável, fonte, texto preciso, classe obrigatório/opcional, contexto e exceções, dono e versão;
- **SPEC**: estado/fluxo de UX, API/eventos, entradas e saídas, limites, direitos/licença, comportamento offline, falhas, acessibilidade, privacy impact;
- **ADR**: alternativas técnicas, riscos e decisão; upstream SHA e fonte OEM com licença/commit;
- **TEST**: caso ID vinculado ao REQ, setup, fixture/hash, expected/actual, quem executou, Windows/firmware, timestamps e links de logs/screenshots;
- **PR**: diff mínimo, checklist QA e ameaça, revisão sem auto-merge;
- **REL**: versão/SHA, SBOM, assinatura, notas, canal, plano de rollback e aprovação explícita do proprietário.

**Antes de codificar:** não há \`SPEC + TEST\` = não há feature liberável. **Antes de merge:** não há 100% de testes obrigatórios PASS + revisão = merge bloqueado. **Antes de publicar:** não há Windows/hardware/evidência/consentimento/assinatura = publicação bloqueada.

## 5. Cronograma lógico — ondas sequenciais por dependências

**Medição:** as "semanas relativas" abaixo são faixas preliminares para planejamento, não datas prometidas. Cada duração será recalibrada depois de G0 com equipe disponível, infraestrutura de CI, impressoras físicas e licenças. **Portões são prioritários ao calendário.** A meta de lançamento de 15/12/2026 fica *condicionada*, não autoriza cortar aceites.

| Onda | Janela relativa indicativa | Entrega principal | Pré-requisito indispensável | Gate de saída |
|---|---|---|---|---|
| **00 — Genealogia** | Semanas 0–2 | Inventário do site e do Windows antigos, documentos, contradições, testes, fonte por feature, matriz de migração, SBOM inicial | GitHub e referência de Windows real | **G0** 100%; fontes obrigatórias esgotadas |
| **01 — Fundação** | Semanas 2–4 | CI, checks de verdade, baseline de G-code, instalador privado, isolamento de dados, rastreamento de versão, logs | 00 PASS | G1 + G2 + G3 da fundação |
| **02 — Qualidade nativa Orca** | Semanas 4–6 | Import/export STL/3MF/STEP, multimaterial, multi-placa, projetos, perfis e regressão upstream, conexão sem rede | 01 PASS | G3 e G4 Windows Orca, zero regressões críticas |
| **03 — Identidade e Home** | Semanas 6–9 | Logo/ícone/mascote verdadeiros, tokens, Home/menus, login independente do core, ajuda e temas | assets originais + 02 PASS | G1 visual, G2, G3, G4 com prints aprovados |
| **04 — Preparar / Visualização / Calibração** | Semanas 9–13 | UI Filuna em cima do Orca real, estado único dos controles, preview e G-code, menus por nível | 03 PASS + Orca real preservado | G3 regressão de parâmetros, G4 Windows |
| **05 — DeviceBridge fundacional** | Semanas 13–16 | Identidade de impressora, descoberta segura, auth, capability registry, telemetria, logs, comandos confirmados, painel e câmeras | 02–04 PASS | Testes negativos, rede falsa e hardware piloto PASS |
| **06 — Piloto Elegoo + Anycubic** | Semanas 16–20 | Centauri Carbon 2 e Kobra 3 V2: descoberta real, envio, status e operações legalmente disponíveis; Canvas/ACE com leitura validada | 05 PASS | G4 por capacidade e firmware dos dois modelos |
| **07 — Bambu + Creality + Flashforge** | Semanas 20–25 | Adaptadores com licenças oficiais verificadas e pilotos reais, câmera e materiais quando suportados | 05 e protocolos individualmente PASS | G4 por modelo/firmware e autorização |
| **08 — QIDI + Snapmaker + Prusa** | Semanas 25–29 | LAN/Moonraker/PrusaLink e extensões específicas em modelos homologados | 05 PASS; 07 não invalida | G4 de cada marca/recurso |
| **09 — UltiMaker + Raise3D** | Semanas 29–33 | Pilotos com interfaces oficiais/protocolos suportados; fallback manual documentado | licença/protocolo/SDK verificados | G4 de cada marca, sem inventar ações |
| **10 — Catálogo/paramétricos** | Semanas 33–39 | Galeria, gerador local, primeiro Chaveiro Botton, Lua, árvore, luminária, demais itens conforme status; modelos protegidos | 04/05 + segurança e licença | 100% de cada gerador/peça e testes físicos |
| **11 — Ateliê, Estoque e Produção** | Semanas 39–45 | Bobinas, rastreio, custos, pedidos, clientes, financeiro, fila, QR, impressões e auditoria | 05/06/10 e esquema de dados PASS | sync/ledger/isolamento/Windows/QA |
| **12 — Serviços e IA** | Semanas 45–50 | Filuna ID, plano, nuvem/backup, Mercado Livre, IA, Claude/Gemini/Grok/GPT via canais seguros | 10/11 e segurança jurídica | integração real, privacidade, offline, quotas |
| **13 — Produto e distribuição** | Semanas 50–54 | Atualizador modular C/D, assinatura, retorno, instalador, portabilidade, acessibilidade, desempenho, documentação e beta controlada | Gates de todos os módulos declarados | G5 + G6; dono aprova versão/SHA |
| **14 — Operação permanente** | Contínua após G6 | Incidentes, regressões, releases de correção, upstream watch, segurança, suporte e qualidade | release aprovado | nenhum incidente crítico conhecido aberto para promoção |

**Nota:** etapas que afetam áreas independentes podem ser pesquisadas em paralelo, mas **nenhuma é declarada concluída nem promovida se seus gates predecessores estiverem falhando**. As 10 marcas são objetivo da edição completa; uma edição limitada anterior só pode ser distribuída com lista honesta de marcas/modelos homologados e com aprovação expressa, nunca alegar "10 nativas" antes disso.

## 6. Pacotes de entrega por área (não perder o antigo Filuna)

| Frente | Mínimo que deve existir em estado real | Falso positivo a proibir |
|---|---|---|
| Home e navegação | último fatiamento real, faixas Recentes/Modelos/Ferramentas, conta, menus, temas e aviso correto | cartão com dados fictícios passando por reais |
| Preparar | mesa 3D Orca verdadeira, ferramentas reais, painel de máquina/filamento/processo, objetos, multi-mesa, purge, G-code, preview | segundo editor fictício que abre Orca externo |
| Visualização/Calibração | camadas, movimento G-code e testes de calibração conforme Orca | visual pré-renderizado sem relação com motor |
| Dispositivos | descoberta e identificação real; status, câmera e comando com ACK e autorização | \`connect\` ou \`start\` retornarem sucesso por stub |
| Perfis | impressoras, bicos, filamentos, processos, importação e edição no Orca; genéricos e herança corretos | cadastrar “Novo PLA” sem preset real |
| Paramétricos | geração no PC, imagem, geometria, 3MF colorido e mesas/cores com QA; IA via proxy mínimo | cards/geradores que não geram arquivos válidos |
| Ferramentas | SVG, imagem→3MF colorido plano (não escultura), reparo, precificação, foto linear quando aprovado | prometer IA de modelagem volumétrica para peça plana |
| Modelos protegidos | print-only na experiência Filuna, origem e autorização, sem download do mestre; livre export externo/paramétrico | alegar que DRM no PC é absolutamente inviolável |
| Estoque | bobinas, peso, QR/NFC, associação por impressora, ledger e baixa real | contagem ilustrativa tratada como física |
| Ateliê | visão geral, orçamentos, pedidos, clientes, financeiro, relatórios, empresa e biblioteca | cards vazios marcados prontos |
| Produção | fila, planejamento, avisos, estado da impressão, custos e retomada por conexão | fila iniciar impressão sem confirmar |
| E-commerce | Mercado Livre autorizado, anúncios, vendas, etiquetas; integrações futuras com critérios próprios | exigir credenciais do cliente fora do fluxo seguro |
| Conta/Plano/Sync | Filuna ID único, offline, entitlements, backup confirmado, recuperação e direitos após vencimento | perder acesso ao Orca por expiração |
| Assistente / IAs | FAQ aprovado, diagnósticos anonimizados, cotas/consentimento e erros honestos | IA dizer que enviou comando sem confirmação |
| Atualizações | A/B/C/D, pacote cumulativo compatível, origem e assinatura, preservação, restart/rollback | instalador inteiro para troca pequena de tema |
| Mobile/Site | conta, catálogo, gestão e impressão remota autorizada sem servidor fatiador obrigatório | prometer fatiamento nativo mobile não construído |

Mais detalhes e mapeamento de fontes: [MIGRACAO-LEGADO-INVENTARIO-E-CONFLITOS.md](MIGRACAO-LEGADO-INVENTARIO-E-CONFLITOS.md).

## 7. Indicadores quantitativos e política de testes

| Categoria | Requisito bloqueante |
|---|---|
| Correção de requisitos | **100% dos IDs obrigatórios do escopo** com testes PASS e evidência correspondente |
| Casos negativos críticos | 100% PASS: dispositivo offline, credencial inválida, firmware incompatível, timeout, upload parcial, resposta malformada, queda de rede, cancelamento, cache inválido, conflito de perfil |
| Segurança | 0 vulnerabilidades críticas/altas abertas para a versão; portas/segredos/tenancy revisados; nenhum comando executado sem autorização |
| Slicing | 100% das fixtures de regressão Orca de cada alteração PASS; resultado equivalente no motor fixado sob tolerâncias declaradas, sem ignorar alteração de geometria |
| Impressão | 100% dos cenários físicos obrigatórios aplicáveis por modelo PASS; qualidade visual/dimensional aceita por proprietário ou técnico designado |
| Operações por fabricante | Cada capacidade marcada IMPLEMENTED exige 100% de casos positivos/negativos PASS no firmware; UNSUPPORTED não deve disparar comandos |
| Atualização | Todos os cenários de assinatura inválida, hash, pacote incompleto, falta de espaço, processo ocupado e rollback PASS |
| Dados | 100% dos cenários de backup/restauração garantidos, sincronização confirmada e isolamento multiusuário PASS; zero perda ou corrupção induzida |
| UX | Rotas e controles do escopo testados: 100% PASS em resoluções/DPI definidos, teclado, foco, estados vazios, erros e tema |
| Confiabilidade | Soak/long run, reconexão e vários dispositivos; limiares de desempenho medidos por baseline, revisados e aprovados no G0/G1 |
| Testes pulados | Teste obrigatório SKIP/NOT RUN bloqueia. Teste não aplicável só é removido da matriz mediante justificativa vinculada a modelo/firmware e aprovação antes do ensaio |

**Atenção:** não existe percentual de linha de código que substitua testes de função e hardware. Cobertura unitária é indicador complementar; o gate é por **100% de requisitos/casos obrigatórios PASS**.

## 8. Matriz de risco, prevenção e bloqueio

| Risco | Controle | Onde bloqueia |
|---|---|---|
| Mudança em \`src/libslic3r\` altera fatiamento sem perceber | baseline G-code + fixtures complexas + cherry-pick pequeno e revisão upstream | G2/G3 |
| OEM usa DLL fechada ou serviço sem autorização | origem/linhas/licença/SDK termos e revisão jurídica | G0/G1 |
| Falso sucesso do \`OrcaPrinterAgent\` nativo (stubs \`BAMBU_NETWORK_SUCCESS\`) | declarar unsupported ou implementar de fato; teste de que comando não foi enviado sem ACK | G2/G3 |
| \`IPrinterAgent\` envia comandos JSON específicos Bambu a impressora não Bambu | dispatch por protocolo/capabilities; nunca herdar comando perigoso genericamente | G2/G3 |
| Qualquer descoberta vira acesso à impressora errada | evidência da identidade, pareamento, IP e firmware, consentimento por máquina | G3/G4 |
| Câmera sem suporte, codec, auth ou sessão expirada | adapters e codecs comprovados, botão desabilitado com motivo | G3/G4 |
| Multimaterial incompatível (AMS/ACE/CFS/Canvas) | slots vinculados a perfil, material exato, números e estados; nada de ASA→ABS | G3/G4 |
| Perfis Orca herdados incorretamente | resolver herança de \`filament/process/machine\`, preservar originais e defaults | G3 |
| Multi-placa com cores diferentes e peças flutuantes | padrão global de placa PEI, Z=0, separações e testes reais | G3/G4 |
| Atualizador C/D quebra DLL/GUI | assinatura, staging, cache, snapshot, rollback, teste VM + Windows real | G4/G5 |
| Conta vencida bloqueia fatiamento livre | testes ACTIVE, EXPIRED, PAST_DUE, CANCEL_AT_PERIOD_END, NETWORK_UNAVAILABLE, REACTIVATED | Todos |
| Upload de logs vazando modelos/tokens | esquema restrito + redaction antes de gravar/enviar + retenção | G2/G3/G5 |
| Datas substituem evidências | gates orientados a dependência, owner sign-off, release bloqueado no CI | Todos |

## 9. Responsabilidades e governança

- **Proprietário:** aprovação de visual, escopo, preços, modelos, imagens, testes de campo, liberação por versão+SHA, credenciais com provedor oficial, compra/fornecimento de hardware piloto; nenhuma ação implícita.
- **Arquitetura/integrações:** manter ADRs, licença de cada arquivo transplantado, API de DeviceBridge, fontes OEM e upstream pin.
- **Qualidade:** criar fixtures antes da feature, scripts reproduzíveis, evidências, revisão adversarial e matriz PASS/FAIL; recusar release sem prova.
- **Segurança:** revisão de autenticação, redes, cloud, consentimento de comandos, plugin isolation, SBOM e atualização.
- **Design:** elementos de marca oficiais, tokens, UX sem simulação, documentação de fluxos e handoff para testes visuais.
- **Operação:** observabilidade opt-in, incidente com código estável, retenção, comunicação, rollback e garantia de não interferir em impressões em andamento.

Um mesmo colaborador pode cobrir funções, mas nunca pode promover a própria mudança sem o controle documentado de QA e o aceite do proprietário quando obrigatório.

## 10. Documentos que compõem o dossiê técnico único

0. **[REGISTRO-CONTINUIDADE-ETAPAS.md](REGISTRO-CONTINUIDADE-ETAPAS.md)** — etapas realizadas, gates, builds e próxima ação (leitura obrigatória antes de trabalhar).
0b. [G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md](G0-INVENTARIO-LEITURA-PRIORITARIA-2026-11-05.md) — evidência parcial G0 (profundidades A/B/C; não 141 integrais).
0c. [G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md](G0-CONFLITOS-RASTREABILIDADE-2026-11-05.md) — CF/REQ e critérios N1–N8.
0d. [sbom/README.md](sbom/README.md) — SBOM Windows parcial (CycloneDX incompleto).
1. Este plano: ordem de execução, escopo, cronograma e riscos.
2. [ARQUITETURA-DEVICEBRIDGE-10-FABRICANTES.md](ARQUITETURA-DEVICEBRIDGE-10-FABRICANTES.md): protocolos, fontes OEM, limitações, modelo de capacidades e matriz de hardware.
3. [MATRIZ-TESTES-ACEITE-100-POR-CENTO.md](MATRIZ-TESTES-ACEITE-100-POR-CENTO.md): casos, evidências, condições de bloqueio e ciclo de bugs.
4. [MIGRACAO-LEGADO-INVENTARIO-E-CONFLITOS.md](MIGRACAO-LEGADO-INVENTARIO-E-CONFLITOS.md): fontes dos três projetos, classes de migração, conflitos e pendências de inventário.
5. [REGRA-MESTRE-QUALIDADE.md](REGRA-MESTRE-QUALIDADE.md), [TEST-ACCEPTANCE.md](TEST-ACCEPTANCE.md), [IDENTIDADE-VISUAL-E-MASCOTE.md](IDENTIDADE-VISUAL-E-MASCOTE.md), [UPDATE-PLAYBOOK.md](UPDATE-PLAYBOOK.md).

## 11. Situação real deste registro

- **Continuidade operacional:** ver [REGISTRO-CONTINUIDADE-ETAPAS.md](REGISTRO-CONTINUIDADE-ETAPAS.md) (atualizado em lotes; G0 ainda EM ANDAMENTO).
- **Tip plano final (quality-master) confirmada:** `d1935aefa91a1eda86f57b67b342a3857247294e` — 52 entregas / 149 deps / SRV-00..11; auditoria documental 113/113 ≠ G0 PASS.
- **Planejamento iniciado e matriz de fontes consultada.** O inventário não afirma que TODOS os arquivos do legado foram lidos integralmente; o índice documenta a cobertura e a fila restante por área.
- **Nenhuma dessas ondas foi executada por este commit.** Os checks estáticos da fundação anteriores passaram, mas testes e homologação de hardware do novo fork ainda não.
- **Definição do produto corrigida** para motor Orca único; fabricantes nativos via adapters. Os 10 nomes são alvo, não certificado de compatibilidade.
- O PR permanece rascunho; qualquer alteração futura à GUI ou a drivers precisa de SPEC e gate anterior aprovado.


## 12. Adendo estrutural — servidor crítico e impressão remota FUTURA (decisão 05/11)

**Contrato específico e subordinado:** [ARQUITETURA-SERVIDOR-DADOS-REMOTO.md](ARQUITETURA-SERVIDOR-DADOS-REMOTO.md). O cronograma executável [CRONOGRAMA-DEPENDENCIAS.json](CRONOGRAMA-DEPENDENCIAS.json) inclui agora **SRV-00 a SRV-11**: inventário de servidor → HTTPS/DB privado → Filuna ID → sync/backup → diagnóstico read-only → estado de impressoras opt-in → portal leitura → fila remota DESLIGADA → piloto de impressão remota do próprio cliente **após homologação física LAN** → controle remoto web/mobile → aceite remoto específico → operação. O nó de release remoto **SRV-10 depende também de RELEASE-38**, e não concede liberação remota em releases anteriores.

**Regras não negociáveis:** suporte Filuna observa apenas erros sanitizados e estado técnico minimizado e consentido; **não acessa LAN, não lê câmeras, não controla, não imprime, não pausa e não cancela impressoras de clientes**. Cliente poderá futuramente controlar **suas** impressoras via opt-in, login seguro, RBAC, sessão e revogação, confirmação explícita por comando e ACK físico. Banco nunca exposto ao aplicativo: Windows usa API HTTPS, IP/DNS de servidor não são constantes no código, credenciais ficam no cofre. Projetos permanecem locais se não houver autorização de upload. Backup, restore comprovado, idempotência e estado de sincronização real são pré-requisitos. Motor Orca funciona offline. Não haverá "imprimir com PC desligado" sem agente/gateway futuro validado.

**Gates específicos de servidor:** \`SRV-QA-001..020\` em contrato; qualquer FAIL/BLOCKED/NOT RUN impede avanço. Esses casos **não foram executados** neste documento. A homologação de impressoras LAN precede toda impressão remota; a existência de botão ou tabela de comandos não autoriza execuções.

### Cobertura documental rastreável

O [MANIFESTO-FONTES-LEGADO-2026-11-05.md](MANIFESTO-FONTES-LEGADO-2026-11-05.md) indexa **141 arquivos de documentação/contrato** (61 do repositório S e 80 do repositório W) com blob SHA e tamanho. O inventário está definido por caminhos, **mas nem todos os documentos foram integralmente lidos**. O portão **G0 permanece EM ANDAMENTO** até análise de conteúdo, exceções e precedência de cada fonte normativa.
