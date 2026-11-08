# FILUNA SLICER — PLANO MESTRE DE ENGENHARIA, EXECUÇÃO E CRONOGRAMA

## Prioridade vigente — ordens t36–t38 de 08/11/2026

O proprietário autoriza iniciar AGORA a integração nativa de atualizações Slicer/servidor e o fluxo de assinatura pelo GitHub conforme regras. Primeira distribuição só beta/admin; até CINCO versões de instaladores, preservando o histórico documental; compilar somente quando necessário. A retomada geral/homologação fica condicionada à integração comprovada. Esta ordem atualiza a pausa histórica somente nesse escopo; não promove gates nem aprova telas derivadas.

**Estado neste checkpoint: especificação proposta, código e testes novos NOT RUN, nenhum instalador beta integrado publicado.** A sessão SSH antiga deixou de responder; uma conexão normal nova alcança o mesmo servidor, mas requer autenticação. Nenhuma chave/configuração foi alterada. Ler [pedido literal e limites](approvals/ATUALIZACAO-NATIVA-BETA-T36-T38-20261108.json) e [contrato pré-código](WEB-FASE-05-RELEASE-CONTRATO-20261108.md) antes de continuar.


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

## 1.1. NOVA REGRA MESTRE — pacote base leve, perfis sob demanda e homologação assistida por 3 testes físicos (07/11/2026)

**ID:** `D-FLN-BUILD-ONBOARD-3PHYSICAL-07NOV`. **Origem:** decisão expressa do proprietário. **Prioridade:** obrigatória para a construção, o instalador/portable, o DeviceBridge, a UX de Dispositivos, os módulos OEM, o catálogo de perfis, o servidor e os gates de beta. Esta seção **substitui interpretações anteriores que exigiam embarcar todos os presets, impressoras e filamentos dentro do instalador**; preserva a arquitetura de Orca único e os testes internos anteriores à compilação final. **Estado nesta data: regra documentada, ainda NÃO implementada ou aprovada em execução.**

### 1.1.1. O que compila e o que NÃO compila no instalador final

- **Compilar um único aplicativo Filuna e uma única base do motor Orca**, com infraestrutura de descoberta/dispositivos, contrato OEM versionado, compatibilidade de G-code, segurança, teste de onboarding e **deltas de código OEM estritamente necessários, auditados e legalmente reutilizáveis** (estáticos quando tecnicamente obrigatório; plugins assinados/versionados quando couber). **Não compilar nem distribuir programas oficiais completos duplicados por fabricante**.
- **NÃO incorporar à compilação/pacote final o catálogo completo de impressoras, variantes de bico, presets de processo ou filamentos de todos os fabricantes.** Todos esses **dados** são provisionados **sob demanda após a identificação/seleção de uma impressora** e sua versão, pelo catálogo versionado com origem oficial, heranças verificadas e integridade criptográfica. O instalador deve ser base e leve, sem um banco integral de perfis OEM empacotado.
- O **motor e os componentes de código indispensáveis** à interpretação correta dos presets/OEM continuam completos e funcionais; **não confundir exclusão dos perfis de dados com exclusão dos módulos de fatiamento, rede ou controle necessários**. As diferenças OEM podem ser incorporadas ao binário único ou recebidas como módulos assinados apenas quando exigidas, respeitando ABI/versão e política de atualização.
- Ao reconhecer uma máquina, carregar **automaticamente os dados da marca/modelo/firmware/variante/bico**, perfis de impressão e filamento compatíveis (incluindo heranças, macros, purga, limites e multimaterial), baixando **somente as dependências de perfil necessárias**. Configurações do usuário e ajustes locais são preservados separadamente. Não fixar uma lista de dois ou três modelos; toda a árvore oficial é indexada automaticamente pelo publicador.
- Fonte de dados confiável: assinatura ou manifesto com SHA, origem/licenças, compatibilidade de esquema, versão, rollback e cache local. **Não executar código arbitrário recebido junto com JSON**; dados são validados antes da ativação. Novas versões/firmwares solicitam atualização incremental apenas do que mudou.
- Sem internet, usar apenas dados já autenticados e armazenados em cache ou um **perfil manual explicitamente compatível e validado**; se a identidade/perfil/módulo indispensável não estiver disponível, informar a pendência e impedir funções específicas de impressão até corrigir, preservando abertura e edição offline de arquivos. A sessão de impressão do cliente não depende permanentemente do servidor Filuna nem de assinatura.
- O **download de presets e atualizações de dados não exige recompilar/reinstalar o programa inteiro**, exceto quando ABI, motor, biblioteca ou requisito de segurança tornar isso inevitável; instalar novos códigos apenas via canal assinado com rollback e preservação de projetos.

### 1.1.2. Assistente automático ao conectar uma impressora — testes por etapas, com participação do cliente

Quando o cliente abrir **Dispositivos**, detectar/selecionar e **autorizar o pareamento** de qualquer impressora, abrir um **assistente de diagnóstico de primeira conexão** no Filuna. O aplicativo executará automaticamente o que consegue provar, exibindo etapas, resultados e explicações legíveis; o cliente só terá de confirmar **observações físicas/visuais que o aplicativo não consegue provar com segurança**. O assistente deve ser retomável, guardar resultados por impressora/firmware/módulo e poder ser reexecutado após mudanças relevantes.

| Ordem | Etapa do assistente | Quem comprova | Gate |
|---|---|---|---|
| 1 | **Identidade:** fabricante, modelo, número de série pseudonimizado, firmware, bico(s), módulo OEM/ABI, recursos | Aplicativo, mediante resposta verificável da impressora e cadastro autorizado | Identidade verificada, sem aceitar texto de modelo digitado como prova de conexão |
| 2 | **Rede e portas:** detecção real, serviço/host/porta apropriados, protocolo, resposta/timeout, descoberta, handshake, pareamento e autenticação quando exigida | **Somente sistema** testa e confirma; usuário não precisa saber portas | Provas de socket/resposta/protocolo e credenciais autorizadas; porta aberta sozinha não é conexão |
| 3 | **Conexão e estado:** status online atual, reconexão, identificação do dispositivo, sensores/temperaturas, erros suportados | Sistema; jamais inferir online de cache obsoleto | Fonte OEM real, sem falso PASS por status de UI |
| 4 | **Multicolor (AMS, ACE, CFS, IFS etc.):** detectar unidade, slots, extrusores, ferramentas, carregamento, material/cor e capacidades reais | Sistema lê eletronicamente os dispositivos; **cliente confirma se as bobinas, cores, materiais e posições exibidos correspondem ao físico** | Se houver multimaterial, ambas as partes devem coincidir. Recursos não existentes recebem N/A fundamentado, não falso PASS |
| 5 | **Câmera:** abrir stream local permitido, decodificar e **mostrar render/visualização ao cliente** na própria interface, com estado de carregamento/erro/ausência de imagem | Sistema confirma stream e frames atuais; **cliente confirma que vê a câmera correta e a imagem está legível** | Sem vídeo real → não marcar câmera PASS. Sem câmera suportada → N/A informado. Não ligar câmera ou enviar vídeo sem permissão |
| 6 | **Perfis e fatiamento:** baixar/carregar o modelo/bico/perfil/filamento oficial correspondente, resolver heranças, abrir projeto de prova, testar motor e G-code sem disparar hardware | Sistema confirma hashes, compatibilidade, 3MF/cores, macros, fatiamento e comparação com referência OEM equivalente | Falha de perfil, fatiamento, G-code ou capacidade declarada bloqueia aquele gate |
| 7 | **Envio e resposta (quando seguro/suportado):** testar disponibilidade da operação e ACK documental do protocolo sem iniciar aquecimento, upload perigoso ou impressão | Sistema verifica canal/contrato real e seus erros negativos; usuário apenas autoriza ações de hardware quando necessárias | Não interpretar "porta aberta", HTTP 200 ou arquivo gerado como job executado; a prova de envio efetivo fica para teste físico autorizado |
| 8 | **Resumo e confirmação:** exibir lista de PASS/FAIL/N/A, opções de refazer, suporte e exatamente o que foi confirmado pelo cliente | Sistema consolida evidências; cliente confirma apenas observações e ações físicas | Só uma bateria de verificações obrigatórias completa e válida permite passar à fase de teste físico |

**Regras UX obrigatórias:** progresso por etapas (detectando → conectando → analisando → perfis → multimaterial → câmera → validando → resultado); permitir repetir etapas, explicar falhas, continuar com funcionalidades seguras quando um recurso opcional é N/A; nunca pedir ao usuário confirmação de teste de portas ou handshake; não aceitar clique humano como substituição de protocolo que a impressora deve provar; nunca afirmar vídeo funcional com placeholder/screenshot antigo.

**Saída quando os gates aplicáveis do diagnóstico pré-impressão passam:**

> **Impressora liberada para teste de impressão.**
> A conexão, os perfis e os recursos disponíveis passaram nas verificações iniciais. A impressão física ainda será validada durante o beta.

**Esta frase NÃO autoriza impressão automática.** O cliente inicia explicitamente o trabalho e decide quando permitir envio/início. Se qualquer gate obrigatório falhar, mostrar **“Impressora com pendências de configuração ou conexão”**, com causa e ação, sem conceder liberação de impressão beta baseada em teste incompleto.

### 1.1.3. Homologação física progressiva em TRÊS testes — estados vistos por novos clientes

Após o assistente e quando começar o **beta vigiado**, o servidor Filuna recebe **somente evidência técnica consentida e minimizada** dos trabalhos reais para formar uma **coorte** por **fabricante + modelo/variante + família de firmware compatível + bico/capacidade relevante + versão do módulo/Filuna**. **A regra é por modelo e capacidade, não por uma whitelist de modelos fixos**. Não emprestar uma certificação de outra geração/firmware/bico/protocolo.

| Evidência válida de impressões físicas completas da coorte | Situação técnica | Mensagem apresentada ao cliente no próximo pareamento |
|---|---|---|
| **0** impressões físicas válidas, com assistente integral PASS | `LIBERADA_PARA_TESTE_DE_IMPRESSAO` | **“Impressora liberada para teste de impressão.”** |
| **1 ou 2** testes físicos válidos **independentes**, inclusive ao conectar uma **segunda unidade física diferente do mesmo modelo** | `PARCIALMENTE_HOMOLOGADA` | **“Impressora parcialmente homologada.”** Informar progresso **1/3 ou 2/3**, oferecer novo teste físico autorizado e explicar que o modelo segue em beta |
| **3 ou mais** testes físicos válidos **independentes**, com **TODAS as regras e capacidades aplicáveis aprovadas** | `HOMOLOGADA_BETA_MODELO` | **“Impressora homologada, liberada para impressão.”** Explicitar quais firmware/bicos/capacidades foram abrangidos e manter rastreamento de novas versões |

**O que conta como 1 teste físico válido, sem falso positivo:**
1. Impressora física genuína **identificada, autorizada e pareada**; assistente PASS para todos os recursos aplicáveis; registrar hashes da versão Filuna/OEM, perfil, bico, firmware e identificação pseudonimizada da unidade.
2. Trabalho real de teste **independente** escolhido e iniciado **expressamente pelo cliente**: importação/abertura, fatiamento interno Filuna, G-code gerado e comparação com oficial quando exigida, mapeamento de cor/filamento, envio autêntico, ACK e **status real de impressão concluída** da impressora.
3. Validar também **erros negativos e recuperação** quando aplicável (desconexão, upload, pausa/retomada, multimaterial/troca de cor, status final); câmera/AMS só serão gates quando suportados pelo hardware e autorizados. Falhas críticas mantêm a coorte pendente e exigem correção/regressão.
4. **Confirmação humana de qualidade física** de impressão (peça realmente formada, defeitos, material/cor quando pertinente; foto/medições só com consentimento separado). Evento `PRINT_FINISHED` sozinho **não é teste completo nem comprova peça boa**.
5. Identificadores de `job`, impressão/unidade e eventos tornam os testes **deduplicados**; não contar reenvio, reabertura do mesmo G-code, mesma peça reapresentada ou vários eventos da mesma impressão como várias provas. Preferir unidades físicas distintas; se forem três trabalhos na mesma unidade, manter isso identificado, sem fingir diversidade de hardware. O avanço exige **três testes reais e independentes** com as regras do modelo, e diversidade de hardware conforme matriz de qualidade aplicável.
6. O servidor correlaciona dados para a coorte e publica um **estado verificável e assinado** para os novos clientes, sem expor projetos, rede, filmagem ou identidade do beta-testador. No aplicativo, executar novamente as verificações locais pertinentes da **nova impressora**; um modelo já homologado **não dispensa** teste de conexão, câmeras e mapeamento das bobinas daquela unidade.

**Lógica obrigatória:** assistente local PASS → habilitar somente teste físico iniciado pelo usuário; 1/3 ou 2/3 testes válidos → exibir **parcialmente homologada**; 3/3 testes completos e aderentes a todas as regras → **homologada e liberada para impressão** para a coorte exata. Não fazer essa promoção por evento isolado ou por conclusão do job sem prova. O histórico mantém transições de estado, hashes e quem confirmou etapas visuais/físicas. Falhas de segurança, nova versão incompatível, regressão ou problema de firmware **suspendem ou reabrem** a homologação afetada.

**Distinção de governança:** a progressão do **status técnico exibido no beta** pode ser automatizada após três provas rigorosas aprovadas, conforme esta nova decisão do proprietário. Isso **não constitui autorização para lançar uma nova versão estável, fazer merge/deploy ou ativar recursos fora do beta**: essas decisões de produto continuam sujeitas à aprovação expressa do proprietário e aos gates de release.

### 1.1.4. Testes, servidor, privacidade e critérios negativos

- A matriz de QA interno deve testar o **instalador sem todo o catálogo OEM**, descoberta de um fabricante/modelo desconhecido no pacote, busca incremental automática de perfis, instalação/cache offline, mudança de bico/firmware, herança, falta de internet, manifesto alterado, esquema inválido, atualização e rollback; falha exige mensagem clara, não fallback silencioso para Bambu A1.
- Simular **portas fechadas/abertas, handshake rejeitado, senha errada, estado desatualizado, câmera sem frames, feed incorreto, AMS presente mas slots inconsistentes, cor/material incorretos, sem multimaterial, sem câmera, falha de slice/G-code, ACK falso, impressão finalizada sem peça e três eventos duplicados de um único job**. Esses cenários **não podem produzir homologação**.
- O servidor Filuna mantém **contadores de testes independentes e provas de cada etapa** na ferramenta de observabilidade, conforme [REGRA-BETA-MONITORAMENTO-HOMOLOGACAO-FISICA-20261107.md](REGRA-BETA-MONITORAMENTO-HOMOLOGACAO-FISICA-20261107.md); o painel é de observação e revisão, **não de controle remoto**. Nunca abre câmera nem imprime pelo servidor.
- Participação física do beta, monitoramento diagnóstico, anexos de qualidade e câmera exigem escopos de **permissão apropriados, explícitos e revogáveis** e minimização de dados conforme a política aplicável. O teste de webcam/câmera é renderizado **localmente** para o próprio cliente; não transmitir imagens ao servidor por padrão.
- Disponibilizar uma API pública de **metadados não sensíveis de homologação por modelo** para que novos clientes consultem o estado de referência, assinada/versionada; nunca expor dados identificáveis dos proprietários, seriais em claro, tokens, caminhos STL/3MF, endereços LAN ou gravações.
- Evidências físicas de hardware só começam **após** os testes internos, a compilação final do **pacote base leve** e a abertura do beta supervisionado. Nenhum documento/CI de perfil ou portátil oficial equivale a teste físico. **Estado atual deste requisito: `DOCUMENTADO_NAO_IMPLEMENTADO`.**

**Critérios de aceite desta nova decisão:** `BASE_UNICA_SEM_CATALOGO_TOTAL=PASS`; `AUTO_DOWNLOAD_PROFILES_ON_CONNECT=PASS`; `ONBOARD_PORTS_AUTOMATED=PASS`; `CAMERA_LOCAL_RENDER_AND_CLIENT_CONFIRM=PASS_OR_SUPPORTED_NA`; `MULTICOLOR_OEM_READ_AND_CLIENT_CONFIRM=PASS_OR_SUPPORTED_NA`; `PROFILE_INHERITANCE_GCODE=PASS`; `PHYSICAL_TEST_VALIDATED_1_2_3=PASS`; `HOMOLOGATION_TRANSITION_NO_FALSE_POSITIVES=PASS`. Se qualquer requisito obrigatório estiver `NOT_RUN/FAIL/BLOCKED`, **não** marcar a funcionalidade pronta nem o fabricante completo.

---

### Nota técnica de implementação parcial do pacote leve (07/11/2026)

O protótipo **de dados por modelo sob demanda**, sem integrar ainda o aplicativo Windows, está em \`FILUNA/tools/oem_on_demand_profiles.py\`. Ele parte do catálogo OEM completo em **origem oficial SHA/commit fixado**, recebe o modelo real escolhido, entrega **somente máquina, processos e filamentos explicitamente compatíveis** em arquivos JSON individuais, grava manifesto SHA-256 e recusa integridade violada, modelos desconhecidos, nomes de caminho inseguros, catálogo com heranças quebradas e expressões OEM de compatibilidade não interpretadas. Nunca mistura perfis de outras máquinas ou utiliza uma whitelist de dois modelos. Presets condicionais desconhecidos são bloqueados para avaliação pelo interpretador OEM do motor.

**Evidência:** [workflow #42321919685](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321919685), **SUCCESS, 7/7 testes unitários e negativos**. Este PASS **não** significa que o instalador atual já está leve, que o backend publica catálogos assinados, que os dados já chegam ao app por dispositivo, ou que há protocolo de rede/câmera/multicolor ativo. Estado: \`ON_DEMAND_DATA_SNAPSHOT_PROTOTYPE=PASS_QA_ONLY\`; \`SIGNED_DISTRIBUTION=NOT_RUN\`; \`FILUNA_APP_DYNAMIC_LOADER=NOT_RUN\`; \`BETA_WIZARD=NOT_RUN\`. Assinatura/servidor/API, ABI, suporte offline e dashboard precisam ser implementados e homologados antes de distribuição.

## 2. Status de referência — não confundir gerações

**A. Site anterior:** \`abcollicelli-web/Filuna\`, documentação \`Filuna-Slicer/\` e módulo legado do site. Fonte de requisitos, UI, histórico e decisões, NÃO base de código a importar sem licença/testes.  
**B. Cliente Windows anterior:** \`abcollicelli-web/filuna-slicer\` \`main\`, C#/.NET 10/WinForms + WebView2, Orca separado e serviços Hub; há interfaces, adaptadores, contratos, protótipos, testes sintéticos e várias versões 0.2.x. Alguns relatórios indicam funções em produção beta **sem homologação física**. Não herdar certificados de teste para código novo.  
**C. Produto atual:** \`abcollicelli-web/FilunaSlicer\`, fork C++17/wxWidgets Orca 2.5.0-dev; rebrand e packaging parciais; DeviceBridge Filuna e Hub visual inexistentes; \`FILUNA/TEST-ACCEPTANCE.md\` pendente.  
**D. Instalação Windows relatada:** \`C:\Program Files\FilunaSlicer\`, ~581 MB, \`orca-slicer.exe\` launcher, \`OrcaSlicer.dll\` ~148 MB, \`%APPDATA%\Filuna\`, ainda visual Orca, pacote sem assinatura. Esses dados vêm do proprietário, não de inspeção direta deste Windows.  
**E. Build Windows da base rebrand:** GitHub run [`42102469888`](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42102469888) concluiu **success** em `filuna/rebrand-1` (`2dace0db`) e publicou `Filuna_Windows_x64_installer` + `Filuna_Windows_x64_portable`. Isso **não** é QA Windows nem homologação física. Estado operacional: [REGISTRO-CONTINUIDADE-ETAPAS.md](REGISTRO-CONTINUIDADE-ETAPAS.md).

## 3. Regras documentais de precedência e conflito

1. Segurança, obrigações de licenciamento e ordem expressa atual do proprietário são bloqueantes.
2. Identidade atual fornecida em 05/11: [IDENTIDADE-VISUAL-E-MASCOTE.md](IDENTIDADE-VISUAL-E-MASCOTE.md); os cinco originais estão em `FILUNA/brand/assets/original/` (SHA = manifesto; lote continuidade). **Histórico:** até antes desse lote os binários estavam ausentes do git. **Paleta amostrada de JPEG não é token HEX homologado** (Onda 03 / G1 visual).
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
0d. [G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md](G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md) — conjunto normativo N1 (≠ 141 cláusula-a-cláusula).
0e. [G0-MAPA-141-STATUS-2026-11-05.md](G0-MAPA-141-STATUS-2026-11-05.md) · [G0-MIG-DAG-ALINHAMENTO-2026-11-05.md](G0-MIG-DAG-ALINHAMENTO-2026-11-05.md) · [G0-RISCOS-OEM-PROVENIENCIA-2026-11-05.md](G0-RISCOS-OEM-PROVENIENCIA-2026-11-05.md) · [G0-TESTES-CASOS-2026-11-05.md](G0-TESTES-CASOS-2026-11-05.md).
0f. [sbom/README.md](sbom/README.md) — SBOM Windows **inicial** (`PARTIAL_NOT_COMPLETE`).
0g. [FUND-SPEC-ONDA01-2026-11-05.md](FUND-SPEC-ONDA01-2026-11-05.md) · [FUND-REQ-ONDA01-2026-11-05.md](FUND-REQ-ONDA01-2026-11-05.md) · [FUND-TEST-ONDA01-2026-11-05.md](FUND-TEST-ONDA01-2026-11-05.md) · [FUND-ADR-BASELINE-DADOS-2026-11-05.md](FUND-ADR-BASELINE-DADOS-2026-11-05.md) — Onda 01 (PROPOSTO).
1. Este plano: ordem de execução, escopo, cronograma e riscos.
2. [ARQUITETURA-DEVICEBRIDGE-10-FABRICANTES.md](ARQUITETURA-DEVICEBRIDGE-10-FABRICANTES.md): protocolos, fontes OEM, limitações, modelo de capacidades e matriz de hardware.
3. [MATRIZ-TESTES-ACEITE-100-POR-CENTO.md](MATRIZ-TESTES-ACEITE-100-POR-CENTO.md): casos, evidências, condições de bloqueio e ciclo de bugs.
4. [MIGRACAO-LEGADO-INVENTARIO-E-CONFLITOS.md](MIGRACAO-LEGADO-INVENTARIO-E-CONFLITOS.md): fontes dos três projetos, classes de migração, conflitos e pendências de inventário.
5. [REGRA-MESTRE-QUALIDADE.md](REGRA-MESTRE-QUALIDADE.md), [TEST-ACCEPTANCE.md](TEST-ACCEPTANCE.md), [IDENTIDADE-VISUAL-E-MASCOTE.md](IDENTIDADE-VISUAL-E-MASCOTE.md), [UPDATE-PLAYBOOK.md](UPDATE-PLAYBOOK.md).

## 11. Situação real deste registro

- **Continuidade operacional:** ver [REGISTRO-CONTINUIDADE-ETAPAS.md](REGISTRO-CONTINUIDADE-ETAPAS.md) — **G0 PASS DOCUMENTAL** (Onda 00); G1+ NOT RUN/BLOCKED.
- **Tip plano final (quality-master) confirmada:** `d1935aefa91a1eda86f57b67b342a3857247294e` — 52 entregas / 149 deps / SRV-00..11; auditoria documental 113/113. **G0 PASS DOCUMENTAL** no REGISTRO/PR #3 (`ee025544…`); 113/113 continua ≠ release/G4.
- **Planejamento iniciado e matriz de fontes consultada.** O inventário não afirma que TODOS os arquivos do legado foram lidos integralmente; o índice documenta a cobertura e a fila restante por área.
- **Onda 00:** G0 PASS DOCUMENTAL (continuidade). **Onda 01:** contrato FUND PROPOSTO (sem implementação neste registro do plano). Hardware/G4 ainda não.
- **Definição do produto corrigida** para motor Orca único; fabricantes nativos via adapters. Os 10 nomes são alvo, não certificado de compatibilidade.
- O PR permanece rascunho; qualquer alteração futura à GUI ou a drivers precisa de SPEC e gate anterior aprovado.


## 12. Adendo estrutural — servidor crítico e impressão remota FUTURA (decisão 05/11)

**Contrato específico e subordinado:** [ARQUITETURA-SERVIDOR-DADOS-REMOTO.md](ARQUITETURA-SERVIDOR-DADOS-REMOTO.md). O cronograma executável [CRONOGRAMA-DEPENDENCIAS.json](CRONOGRAMA-DEPENDENCIAS.json) inclui agora **SRV-00 a SRV-11**: inventário de servidor → HTTPS/DB privado → Filuna ID → sync/backup → diagnóstico read-only → estado de impressoras opt-in → portal leitura → fila remota DESLIGADA → piloto de impressão remota do próprio cliente **após homologação física LAN** → controle remoto web/mobile → aceite remoto específico → operação. O nó de release remoto **SRV-10 depende também de RELEASE-38**, e não concede liberação remota em releases anteriores.

**Regras não negociáveis:** suporte Filuna observa apenas erros sanitizados e estado técnico minimizado e consentido; **não acessa LAN, não lê câmeras, não controla, não imprime, não pausa e não cancela impressoras de clientes**. Cliente poderá futuramente controlar **suas** impressoras via opt-in, login seguro, RBAC, sessão e revogação, confirmação explícita por comando e ACK físico. Banco nunca exposto ao aplicativo: Windows usa API HTTPS, IP/DNS de servidor não são constantes no código, credenciais ficam no cofre. Projetos permanecem locais se não houver autorização de upload. Backup, restore comprovado, idempotência e estado de sincronização real são pré-requisitos. Motor Orca funciona offline. Não haverá "imprimir com PC desligado" sem agente/gateway futuro validado.

**Gates específicos de servidor:** \`SRV-QA-001..020\` em contrato; qualquer FAIL/BLOCKED/NOT RUN impede avanço. Esses casos **não foram executados** neste documento. A homologação de impressoras LAN precede toda impressão remota; a existência de botão ou tabela de comandos não autoriza execuções.

### Cobertura documental rastreável

O [MANIFESTO-FONTES-LEGADO-2026-11-05.md](MANIFESTO-FONTES-LEGADO-2026-11-05.md) indexa **141 arquivos de documentação/contrato** (61 S + 80 W) com blob SHA e tamanho. **Fato corrente:** G0 = **PASS DOCUMENTAL** (Onda 00) com conjunto normativo esgotado ([G0-FONTES-OBRIGATORIAS](G0-FONTES-OBRIGATORIAS-N1-2026-11-05.md)); mapa 141 path+SHA+status ([G0-MAPA-141-STATUS](G0-MAPA-141-STATUS-2026-11-05.md)); profundidade semântica cláusula-a-cláusula **≠** 141 (COVER restante = histórico não obrigatório).  
**Histórico (superado):** formulação anterior “G0 permanece EM ANDAMENTO até análise de cada fonte do manifesto” — válida antes do fechamento `ee025544…`; não usar como estado atual.

## 13. REGRA MESTRE VINCULANTE — MOTOR-BASE LEVE, PERFIS SOB DEMANDA E HOMOLOGAÇÃO GUIADA AUTOMÁTICA (07/11/2026)

**Decisão expressa do proprietário; vigência em toda a criação, implantação, empacotamento, onboarding Dispositivos e beta físico do Filuna Slicer.** Substitui qualquer texto anterior que implique compilar/distribuir um fatiador OEM inteiro, empacotar o catálogo completo de perfis de impressoras/filamentos no instalador, ou habilitar automaticamente uma impressora física ainda não testada. **Estado: REQUISITO APROVADO / IMPLEMENTAÇÃO NÃO INICIADA; não confundir especificação com teste executado.**

### 13.1 Arquitetura que o pacote final DEVE seguir

- **Instalador Filuna = interface Filuna + motor-base Orca compartilhado + diferenças necessárias do código dos fatiadores OEM que sejam legalmente reutilizáveis + DeviceBridge/contratos de rede, segurança, atualização e assistente de testes.** Não empacotar múltiplos fatiadores completos, binários OEM fechados ou aplicações independentes; preservar arquivos/licenças/SBOM obrigatórios. O mesmo motor-base atende todas as marcas, escolhendo automaticamente os deltas de comportamento compatíveis com marca/modelo/firmware/capabilities.
- **NÃO incluir no pacote final os catálogos completos de perfis de impressora, filamento, material, processo e variantes de cada fabricante; NÃO compilar esses perfis dentro do executável.** Perfis são **dados versionados externos**, não código-fonte nem componentes fixados no binário. O instalador inclui somente o mínimo necessário para abrir o Filuna, descobrir/associar uma impressora e pedir os perfis oficiais corretos. Os testes internos podem usar corpora de perfis oficiais em CI, sem incorporá-los à distribuição.
- Ao **conectar/adicionar uma impressora em Dispositivos**, detectar a identidade/capacidades reais da unidade, autenticar de forma segura quando exigido, identificar marca/modelo/variante/bico/firmware e **obter exclusivamente os perfis efetivos e suas dependências/heranças compatíveis**, de fonte oficial verificada ou catálogo Filuna validado que preserve proveniência OEM. Fazer download por API HTTPS, com manifesto versão/hash/assinatura, verificação de integridade, compatibilidade, instalação atômica, cache local, atualização diferencial e rollback. Não baixar silenciosamente todo o catálogo de todas as marcas.
- Sistema **universal e automático**: perfis e testes não ficam presos aos modelos-piloto usados no laboratório (U1, AD5X, Kobra S1 etc.). Resolver **todos** os modelos, bicos, materiais, tecnologias multimaterial e heranças disponíveis na origem aprovada; selecionar o subconjunto aplicável à máquina detectada e ao seu firmware, sem tabelas de exceções fixas de 2–3 modelos.
- **Sem conexão à Internet:** o motor-base Filuna continua abrindo, editando e fatiando quando houver perfil válido já armazenado ou perfil manual importado/validado. Uma impressora sem perfil local e sem download possível é sinalizada como **CONFIGURAÇÃO PENDENTE**, não recebe um preset inventado nem inicia impressão. Os perfis locais e os projetos persistem após atualizações.
- Firmware ou perfis OEM novos podem requerer deltas de código/módulo além de dados. O catálogo **não pode fingir** compatibilidade apenas por ter JSON; um delta de motor/rede ainda não integrado resulta em bloqueio de capacidade e atualização adequada. Nunca carregar código executável arbitrário do servidor como se fosse simples perfil.

### 13.2 Assistente obrigatório na PRIMEIRA conexão de cada impressora física

Ao reconhecer e vincular **uma impressora física real nova** em Dispositivos, o Filuna abre um **teste guiado em etapas**, com checklist visual e progresso, que deve funcionar tanto para modelos ainda em beta quanto para modelos homologados (nesses, pode executar verificação de integridade/compatibilidade abreviada, sem pular testes críticos). Cada etapa é classificada como **automática pelo sistema**, **confirmação humana pelo proprietário da impressora**, ou **NÃO SE APLICA** apenas quando a capacidade oficialmente não existe.

| Etapa | Verificação e evidência | Quem confirma |
|---|---|---|
| 1. Identidade e perfis | Descoberta, marca/modelo/variante, serial pseudonimizado, firmware, bico(s), módulo OEM/ABI, perfis efetivos e heranças, hashes de origem, limites compatíveis | **Sistema**, com identificação visual ao usuário |
| 2. Portas e rede | Interfaces/serviços/portas exigidos pelo protocolo da impressora, descoberta LAN quando suportada, reachability, pareamento/autenticação autorizados, TLS/certificados, reconexão e estado em tempo real | **Sistema**: usuário **não** atesta que portas funcionam |
| 3. Controle e transmissão | Estado do equipamento, comandos e permissões suportadas, sem efetuar ações perigosas; ensaios não destrutivos de protocolo e resposta/ACK quando disponíveis | **Sistema**; início de impressão somente mais tarde com confirmação do usuário |
| 4. Câmera | Se oficialmente suportada/autorizada, abrir **preview/render ao vivo na interface local Filuna** com imagem real, latência/erro; exibir claramente se abriu, falhou ou não existe; recurso indisponível ≠ PASS | **Sistema** confirma comunicação e imagem real; **cliente** confirma que a visualização corresponde à sua impressora e está correta |
| 5. AMS/ACE/CFS/IFS/multimaterial | Descobrir se há sistema de troca, quantidade de slots, unidades, extrusores, mapeamento real de cores, material e filamento por slot, sensores e capacidades de troca | **Sistema** lê dados OEM; **cliente** confirma visualmente se bobinas/cores/materiais/slots exibidos estão corretos |
| 6. Temperatura, estado e segurança | Temperaturas e sensores disponíveis, fim de curso/portas/tampas, alarmes, firmware, disponibilidade de upload e recursos por modelo sem disparar calor/motores | **Sistema**, com confirmação humana apenas para observações visuais não comprováveis digitalmente |
| 7. Fatiamento e G-code | Abertura do arquivo, perfil efetivo, motor-base + delta OEM correto, material e multicor, geração e validação de G-code/macros e comparação com referência OEM quando possível, sem ocultar diferenças de movimento | **Sistema**, com evidência do job e hashes |
| 8. Prontidão de impressão | Ações anteriores sem erros obrigatórios, material conferido, impressora apta, riscos e revisão final da tela de testes | **Sistema** libera o estado; **cliente** confirma se deseja fazer a impressão de teste |

O **assistente não é uma tela com botões PASS decorativos**: cada etapa produz \`test_id\`, versão, modelo, firmware, capacidade, origem da medição, responsável, status, timestamp, validade, hashes, eventos de erros e evidência. Testes de portas, rede, câmera conectada, protocolo, G-code ou ACK **não podem ser aprovados pelo cliente quando dependem de prova do software**. Confirmações humanas são específicas e registradas. Um \`UNKNOWN\`, \`TIMEOUT\`, \`NÃO TESTADO\` ou recusa do cliente **não** viram PASS. \`N/A\` só vale quando a ausência de recurso é provada para aquele modelo/firmware.

### 13.3 Estados e mensagens que o cliente verá — promoção automática no beta

**A homologação é por fabricante + modelo + variante + firmware + módulo OEM/versionamento + conjunto de capacidades aplicáveis**, não por marca global, nem apenas pelo nome de marketing. Distinguir **impressora individual conectada** e **modelo homologado**. Unidades de um mesmo modelo participam da mesma matriz, mas resultados de outro firmware/variante não são transferidos sem análise técnica.

1. **Primeira unidade física, ainda sem histórico validado no beta:** executar o assistente completo. Com todos os testes obrigatórios de conexão/recursos/fatiamento PASS e confirmações humanas necessárias registradas, mostrar exatamente **“Impressora liberada para teste de impressão”**. O cliente **decide e inicia** a impressão de teste; o servidor nunca a inicia. A liberação do teste **não** significa homologação.
2. **Depois de UMA impressão física de teste efetivamente concluída e validada ponta a ponta** (G-code gerado, enviado, ACK, início, estados reais, fim sem erro crítico, consistência multicor quando aplicável, confirmação física do usuário): a próxima **unidade do mesmo modelo/variante/firmware**, ao ser conectada, mostra **“Impressora parcialmente homologada”**. Ainda executa seu próprio checklist e só libera sua primeira impressão se seus testes locais passarem. Nunca propagar a aprovação de rede/câmera/AMS de uma impressora física para outra como se fossem testes locais.
3. **Depois de TRÊS testes físicos completos, distintos e verificáveis**, todos com **100% dos critérios e regras já aprovados PASS**, sem bloqueio crítico, inconsistência, identidade duplicada de job nem prova faltante: a classificação daquele **modelo/variante/firmware/capacidades** no beta passa automaticamente a **“Impressora homologada — liberada para impressão”** para o próximo cliente que conectar uma unidade compatível. O checklist local de segurança/pareamento/material ainda é obrigatório para cada unidade, e erros novos retiram a classificação até revalidação.
4. **Qualquer falha obrigatória ou ausência de evidência:** mostrar “Validação pendente” ou “Falha no teste: [etapa e causa]” com caminho de repetição e suporte; **não liberar a primeira impressão de um modelo ainda não autorizado**. Capacidade opcional inexistente provada recebe \`N/A\`, não reprovação injustificada. Critérios dependentes de hardware são avaliados **somente no beta físico**, não por simulador.

**Contagem antifalso positivo:** cada teste físico válido tem \`physical_test_id\`, \`job_id\`, \`installation_pseudonym\`, \`printer_pseudonym\`, \`model\`, \`firmware\`, \`module_hash\`, \`profile_hash\`, \`gcode_sha256\`, tempo de início/fim, estado OEM e confirmação física. Três execuções **reais e independentes**; reconexões, reenvio de telemetria, retries, demonstrações/simuladores, três confirmações na mesma tela ou cópias do mesmo job não aumentam o contador. É recomendável diversificar clientes/unidades e exigir diversidade quando definida pela matriz do modelo. Falha crítica reinicia/abre os gates afetados e sinaliza incidente.

**Precedência com a governança:** as mensagens e o indicador de homologação **dentro do beta** podem ser promovidos automaticamente após os três PASS válidos conforme esta nova decisão. Isso **não** faz merge, release estável, publicidade de compatibilidade irrestrita, autorização administrativa de controle ou publicação de módulo/binário: a saída do beta e releases continuam sujeitas aos gates de segurança, licença, testes e aprovação expressa do proprietário.

### 13.4 Monitoramento do servidor e fronteira de privacidade

- Assistente do cliente e instrumentação local publicam **somente eventos técnicos necessários, consentidos, minimizados e autenticados**, correlacionando \`APP_OPENED\`, \`PRINTER_DISCOVERED\`, \`PROFILE_LOADED\`, \`NETWORK_TEST\`, \`CAMERA_TEST\`, \`MULTICOLOR_SLOT_CONFIRMED\`, \`SLICE_COMPLETED\`, \`GCODE_GENERATED\`, \`UPLOAD_ACK\`, \`PRINT_PROGRESS\`, \`PRINT_FINISHED\`, \`PRINT_FAILED\` e \`USER_PHYSICAL_RESULT\`.
- O **servidor Filuna** recebe hashes, estados, evidências técnicas e contador das três provas para computar a classificação dos modelos; administra apenas observabilidade e métricas, **não recebe sessão de controle da impressora**, não imprime, não aquece, não acessa LAN, câmera ou bobinas do cliente diretamente.
- A prévia de câmera permanece **local no Filuna e visível ao próprio cliente**; o servidor armazena somente sucesso/falha e confirmação consentida, **não stream nem imagem** por padrão. Mesmo para câmera funcionando, o cliente continua responsável por autorizar sua visualização. Dados pessoais, IP, serial, tokens, códigos LAN, dados de trabalho e arquivos completos não entram no servidor sem base autorizada e escopo específico.
- Telemetria atrasada/offline, ACK ausente ou status \`FINISHED\` sozinho **não** prova peça boa nem G-code executado byte a byte. O dossiê vincula confirmação física a hash, firmware e job e diferencia \`GENERATED\`, \`SENT_ACK\`, \`PRINTER_CONFIRMED\`, \`EXECUTION_OBSERVED\` quando disponíveis.
- Regra operacional completa e retenção/privacidade em [REGRA-BETA-MONITORAMENTO-HOMOLOGACAO-FISICA-20261107.md](REGRA-BETA-MONITORAMENTO-HOMOLOGACAO-FISICA-20261107.md) e [ARQUITETURA-SERVIDOR-DADOS-REMOTO.md](ARQUITETURA-SERVIDOR-DADOS-REMOTO.md).

### 13.5 Contratos de implementação e aceitação indispensáveis

**Módulos de produto a implementar, não construídos por este adendo:** \`BaseEngine/OemDeltas\` (único motor), \`ProfileCatalogClient\` (manifests e delta por impressora), \`DeviceBridge\` (protocolo), \`FirstConnectWizard\` (etapas/sistema versus pessoa), \`BetaEvidenceCollector\` (fila local/consentimento), \`PhysicalHomologationRegistry\` (3 testes, identidade/firmware/capacidade, avaliação e regressão) e mensagens da interface em Dispositivos.

**Gates negativos e positivos novos:** pacote final sem catálogos OEM extensos nem fatiadores completos; app inicializável sem impressora e offline; identificação de modelo/firmware correta; catálogo baixado apenas para modelo/filamento selecionados, assinado e validado; sem perfil válido = nenhuma impressão; heranças completas; porta inacessível não confirmável manualmente; câmera sem imagem não vira PASS; cor/slot divergente exige correção e confirmação; multimaterial inexistente só \`N/A\` documentado; três trabalhos reais distintos aprovados, não três retransmissões; \`PRINT_FINISHED\` sem peça validada não soma; modelo/firmware novo reabre gates; falha crítica bloqueia a liberação; câmera não transmite imagens ao servidor; administrador sem controle remoto; atualização de perfil sem reinstalar o motor; funcionamento offline preservado com perfil local.

**Fase atual do projeto:** este texto é **regra registrada no plano mestre**, não implementação. A homologação interna dos motores/deltas OEM, dos protocolos/simuladores e do sistema de pacotes é pré-requisito para compilar o Filuna base e abrir o beta físico. **Não** iniciar impressão física, distribuir instalador ou afirmar homologação apenas por registrar o requisito.


## Regra vinculante OEM Snapmaker U1 — segurança sem alterar G-code oficial (07/11/2026)

**Decisão posterior do proprietário, prioritária para a integração OEM, DeviceBridge, Preparar e testes G4-I.** O Filuna não poderá executar correção automática de Z-hop, purga, torre, retração ou movimentos reescrevendo o G-code do fabricante. O G-code, em particular as coordenadas e comandos de movimento, é preservado como produzido pelo motor/perfil original. Diferença funcional Filuna × OEM exige correção de motor/perfil para obter paridade, não reescrita do arquivo nem classificação enganosa de PASS.

A análise de segurança deve acontecer **antes do envio** em módulo independente somente leitura: modelar ocupação XY/Z e altura impressa a partir de extrusões, varrer cada deslocamento crítico com margem física, distinguir torre de purga da peça, rastrear G90/G91, M82/M83, G92 e homing/posição desconhecida, verificar limites reais por marca e detectar espiral Auto fora do volume. Apenas macros OEM **com fonte pinada, corpo documentado e escopo limitado** entram no `safe_macros` do `vendor.yaml`. `PRINT_END` só pode certificar a própria sequência de estacionamento; não é prova de Z-hop na purga final anterior.

Classificar: `PASS` sem risco no escopo demonstrado; `WARN` se espiral Auto sai dos limites (orientar Z-hop Normal, não alterar G-code); `BLOCK` se colidir com geometria impressa **e houver referência oficial com o mesmo risco**; indicar camada, linha, X/Y/Z e oferecer aceite expresso, individual por operador/SHA/risco com registro auditável; `ENGINE_DIVERGENCE` se o Filuna divergir do OEM, bloqueando envio até corrigir a causa interna, sem opção de atribuir falsamente ao fabricante. O cliente pode assumir risco documentado do OEM, não validar implicitamente a implementação Filuna.

O laboratório GitHub [#42345082502](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42345082502) já comprova detecção no **G-code oficial Snapmaker Orca 2.4.1 bicolor, camada 14, linha 38911** e ausência de falso positivo nos cubos originais U1, Bambu A1 e Creality. O validador Python de QA **ainda não está integrado ao binário Windows/DeviceBridge**, ao cadastro de usuário e ao fluxo de enviar: logo **G4-I continua BLOQUEADO**; o fato de existir G-code original com colisão também impede uma aprovação irrestrita do modelo. A etapa física é posterior, no beta autorizado, e não dispensa essas provas. Ver [dossiê e código](qa/SNAPMAKER-U1-GCODE-IMUTAVEL-VALIDADOR-GEOMETRICO-20261107.md) e [Regra Mestre de Qualidade](REGRA-MESTRE-QUALIDADE.md).


## Direção visual C — referência aprovada em 08/11/2026

Antes de criar ou alterar qualquer tela Filuna, consultar `FILUNA/design/visual-c/documentacao/DESIGN-SYSTEM-C.md`, a aprovação literal em `FILUNA/design/visual-c/documentacao/APROVACAO-VISUAL-C.json`, os dois modelos C aprovados e a galeria/matriz em `FILUNA/design/visual-c/README.md` e `FILUNA/design/visual-c/MATRIZ-COBERTURA.md`. O proprietário aprovou a direção visual; as demais telas e detalhamentos são propostas em revisão. O inventário consolidado contém 123 pranchas e 208 itens rastreados, sem equivaler à homologação de software.

Preparação permanece sem painel direito de informações de arquivo; Orca é o motor único. A regra vigente de G-code imutável e a distinção entre risco OEM e divergência de motor permanecem intactas. Esta aprovação não retoma implementação, CI, hardware, distribuição, servidor ou website. No escopo pausado pelo proprietário, G2/G3 permanecem BLOCKED, G4 NOT RUN e G5/G6 BLOCKED. Nenhum requisito funcional ou registro histórico foi revogado.

<a id="fln-visual-c-100"></a>
## FLN-VISUAL-C-100 — reprodução visual obrigatória do modelo aprovado (08/11/2026)

**Ordem expressa do proprietário, 08/11/2026 14:16 Europe/Rome. Aplicável a toda IA e pessoa que construir ou alterar UI. O resultado DEVE ser 100% visualmente idêntico às imagens e regras de montagem/recursos do modelo aprovado.** Não basta inspiração, aproximação, reinterpretação, redesenho ou layout genérico/nativo/de framework. Nenhuma IA pode construir o design diferente do aprovado.

**É proibido mudar estética, estrutura, cores, tipografia, ícones, logo, posições, proporções, espaçamento, estados ou recursos sem NOVA aprovação EXPLÍCITA do proprietário registrada por versão/escopo/hash.** Todos os recursos e funções do projeto são preservados; não removê-los silenciosamente por estética e não maquiar aplicativo funcional com screenshot como fundo.

Regra operacional integral e obrigatória: [REGRA-FIDELIDADE-VISUAL-C.md](design/visual-c/documentacao/REGRA-FIDELIDADE-VISUAL-C.md). Recursos, coordenadas extraídas, SHA e mapeamento: [contrato de montagem](design/visual-c/documentacao/CONTRATO-MONTAGEM-VISUAL-C.md) / [JSON](design/visual-c/documentacao/CONTRATO-MONTAGEM-VISUAL-C.json). Declaração literal nova: [ordem de 08/11](design/visual-c/documentacao/ORDEM-FIDELIDADE-VISUAL-C-20261108.json); aprovação anterior preservada.

Baseline imutável `720291b8263e0898f7d6723bc904112ba2c6b3eb`, caminhos:
- `FILUNA/design/visual-c/preparacao/filuna-preparacao-C.png` — SHA-256 `9e0faadedf2e4dd89cc7e62ed26becc807d2e26e86288cae8b1ec59ed19d0fd6`;
- `FILUNA/design/visual-c/preparacao/filuna-preparacao-C.svg` — SHA-256 `98040a285ed17e4144397d8dc1e6f9b7823594111dc13cc7bda435bf4bc5e74f`;
- `FILUNA/design/visual-c/dispositivo/filuna-dispositivo-C.png` — SHA-256 `4113989f7162c02fad16f56dccf49bec78b22b70bf97de1f721ce5df37893b3f`;
- `FILUNA/design/visual-c/dispositivo/filuna-dispositivo-C.svg` — SHA-256 `52114cfb26faa9d4c04b9ab5a9fac47af6176c241624ce298bf7bfd2567e478d`;
- `FILUNA/design/visual-c/documentacao/APROVACAO-VISUAL-C.json`, `tokens.json` e `DESIGN-SYSTEM-C.md`;
- `FILUNA/design/visual-c/MATRIZ-COBERTURA.json` / `.md`: todo o universo rastreado (208 cenários/123 pranchas), mantendo o estado de aprovação de cada item.

Hashes dos quatro modelos constam integralmente no contrato de montagem e na aprovação preservada. Apenas as duas âncoras possuem aprovação visual: esta ordem **não aprova individualmente 206 linhas derivadas**. Tela/estado sem referência homologada, conflito, recurso ou contrato faltante, limitação técnica ou impossibilidade de demonstrar 100% => **BLOCKED no item**, apresentar divergência ao proprietário; nunca inventar solução estética ou reduzir a meta.

Aceitação futura exige screenshot real bruto, referência autorizada/hash, ambiente controlado (dimensões/DPI/escala/tema/locale/fontes/conteúdo), sobreposição/diff, evidências por toda tela/estado obrigatório e aceite humano expresso do dono. Sem tolerância visual autoconcedida; máscara/normalização/recorte não pode ocultar mudança. Conteúdo dinâmico exige fixture determinística ou exceção de região expressamente aprovada. Existência de arquivos, testes de texto ou “parece igual” não dão PASS. Sem 100% demonstrável, permanece BLOCKED até correção ou nova decisão explícita do proprietário.

**Estado: contrato visual documentado; conformidade da aplicação NOT RUN.** Nenhuma implementação ou teste funcional foi feito por este adendo. Não promove gates: G2/G3 BLOCKED, G4 NOT RUN, G5/G6 BLOCKED; pausa funcional, motor Orca único, G-code imutável, segurança/AGPL e histórico preservados. FLN-VISUAL-C-100 é identificador documental, não alteração dos gates do produto.


<a id="web-servicos-20261108"></a>
## Nova autorização — home, admin e serviços web (08/11/2026, 14:57 Europe/Rome)

O proprietário solicitou o novo sistema e declarou literalmente **«esta liberado»**. [Solicitação integral ipsis litteris e proveniência](approvals/WEB-SERVICOS-AUTORIZACAO-20261108.json). Fica autorizado iniciar exclusivamente o novo workstream em `/srv/filuna-new-site`, conforme seu `docs/NEW-SITE.md`, preservando o cofre/legado sem manipulação. Não é retomada funcional geral do desktop, CI, hardware, beta, release ou domínio vivo por suposição. Esta decisão posterior supera somente a pausa de iniciar o novo site/serviços; não altera resultados dos gates existentes.

**Contrato obrigatório:** [WEB-SERVICOS-ESCOPO-20261108.md](WEB-SERVICOS-ESCOPO-20261108.md), vinculado ao [mestre servidor existente](ARQUITETURA-SERVIDOR-DADOS-REMOTO.md#web-servicos-20261108): escopo, raiz canônica real, ownership, dependências, API/schemas versionados, entitlements assinados/assíncronos, backups/sync, compatibilidade, migrações seguras, testes e rollout/rollback. Home C candidata para revisão; admin protegido; contas, planos/acessos, conteúdo/estoque/impressoras/perfis/configurações, versões/updates/downloads, central/ajuda/chat e backups. Geração/preparação/fatiamento permanecem no PC. Admin nunca controla LAN/câmera/impressão de clientes; motor Orca único e G-code imutável preservados.

O texto literal está incorporado por referência ao registro de autorização, sem paráfrase substituir sua evidência. A cláusula visual FLN-VISUAL-C-100 continua obrigatória; preparar nova home candidata está autorizado, mas não aprova automaticamente derivadas nem conformidade. Integração desktop é etapa coordenada futura. Conflito de chave comercial versus core livre/offline permanece explicitado no contrato: não bloquear o motor básico por decisão da IA. Políticas comerciais não definidas ficam pendentes. Neste lote: documentação registrada neste commit; implementação nova, migração, testes web e deploy NOT RUN; nenhum gate Slicer promovido.


<a id="base-orca-livre-20261108"></a>
## Decisão posterior — base Orca livre e catálogo Filuna condicionado (08/11/2026, 15:18 Europe/Rome)

Texto literal do proprietário:
> o fatiamento basico por regras do orca e modelo base devem serem liberados unica regra perfis e modelo homologado no filuna slicer nao carregam para o cliente e passa a usar a versao limpa do orca

[Registro da decisão e limites](approvals/WEB-ACESSO-BASE-ORCA-20261108.json). Esta decisão **resolve a dúvida comercial core básico versus premium** levantada anteriormente: fatiamento básico e modelo-base Orca livres/offline; sem assinatura válida, não carregar perfis/modelos homologados do catálogo Filuna; usar modo base limpo **na mesma engine Orca única**, sem instalar segundo aplicativo/fatiador. Não apagar/confiscar arquivos próprios, criações ou dados do usuário, nem revogar direitos de cópias sob licenças livres; respeitar licença por recurso. Prazos offline, cache e revogação de autorização de serviço continuam pendentes, sem defaults comerciais inventados.

A política foi incorporada ao domínio/API da prévia na área nova: `contracts/access-policy.v1.json`, `service/domain.py::access_policy` e `/api/v1/policy` **somente loopback**. Básico sempre permitido; carregamento premium só admite estado ativo com entrada interna de assinatura verificada pelo futuro adaptador confiável. Não confiar em booleano/query do cliente. Autenticação e assinatura reais não configuradas: API individual de direitos continua negada (503), sem concessões emitidas, sem alteração do desktop.

[Marco real da primeira fatia](WEB-FASE-01-EVIDENCIAS-20261108.md): home C candidata, API de prévia e schema/migração inicial com 33 testes executados PASS no subconjunto descrito; não equivalem a homologação visual, produto completo ou G2–G6. Cofre/legado não restaurados, tráfego público não trocado.


<a id="central-cliente-identidade-20261108"></a>
## Continuidade — Central do cliente e identidade verificável (08/11/2026, Europe/Rome)

**Novo pedido e aceite visual condicional do proprietário, t33u, 15:36 Europe/Rome — ipsis litteris:**
> esta bom a unica coisa que precisa ter precisa ser um acesso ao cliente para ele contratar, assinar, acessar seus backup, seus recursos, seus download, e isso sera expandido conforme novas ferrasmentas para o cliente serao criadas

[Registro novo da decisão e limites](approvals/WEB-CENTRAL-CLIENTE-T33-20261108.json). O visual apresentado foi aceito com a adição de acesso claro à **Central do cliente**. Manter hero, paleta C, Inter e marca; não redesenhar. Isso NÃO é homologação funcional, pixelperfect 100%, aprovação de derivadas, cobrança ou deploy geral. A entrada superior foi adicionada à home candidata; nova central tem navegação canônica `/conta/` e `/conta/acesso/`, com Contratar, Assinatura, Backups, Meus recursos e Downloads. Registro versionado de módulos governa ID/rota/owner/capability/handler; evoluir sem menus/pastas paralelos. Client e operador admin são separados.

**Estado real da fase02:** implementação somente na área nova; verificação Ed25519 real com PyJWT/cryptography, issuer/audience/datas/alg/tipo/kid controlados, identidade resolvida server-side, RBAC/membership por tenant e dados por conta; role admin exige MFA e não libera dados de clientes. Rotação/revogação de chaves/tokens e revisões anti-rollback testadas; bearer não é one-time/DPoP. Licença do recurso é verificação separada da assinatura. Manifesto JWS é validado criptograficamente, mas a API continua somente metadados, sem alegar instalação/bytes verificados no cliente.

**Operação ainda fechada:** provedor de login, configuração pública legítima de confiança e signer NÃO instalados/configurados; nenhum usuário ou direito real emitido. APIs privadas seguem 503 AUTH_NOT_CONFIGURED na prévia normal. Login browser/BFF/sessão interativa pendentes. Página pública da central explica isso em HTML navegável, sem senha/token fake, checkout inventado ou 503 bruto. Com chaves efêmeras somente em memória e DBs sintéticos temporários, testes HTTP positivos e negativos exercitaram leitura privada somente da própria conta. Não apresentar fixture como integração de produção.

[Resultados, arquivos e limites da fase02](WEB-FASE-02-EVIDENCIAS-20261108.md): **76 testes PASS** (33 baseline + 28 de segurança + 15 da central), dependências fixadas com SHA256 em venv da área nova; migrações 002/003 aditivas após backup apenas do DB novo, 001 intacta, integridade/foreign keys conferidas e tabelas operacionais vazias. Nginx/OS/tráfego/legado/cofre não alterados; nova central NÃO publicada no domínio. Código privado web e configurações não integram este repositório público.

Decisão t32 preservada: básico Orca e modelo-base livres/offline na MESMA engine; assinatura inválida impede carregar perfis/modelos homologados premium Filuna, sem apagar arquivos próprios nem direitos livres. Prazo offline/cache/revogação comercial, preços/planos/provedor e limites continuam sem defaults inventados. Storage/sync/restauração, entrega de downloads, contratação/cobrança e integração desktop não operacionais. A captura anterior em Chrome GUI offline prova apenas os trechos vistos; inspeção da nova entrada/central e mobile continuam pendentes. FLN-VISUAL-C-100, evidência humana e G2–G6/veto à retomada do Slicer permanecem.


<a id="storage-restore-bytes-20261108"></a>
## Continuidade — storage, restore e entrega autorizada de bytes (08/11/2026, Europe/Rome)

Subfatia efetivamente implementada SOMENTE em `/srv/filuna-new-site`: objetos privados opacos, versões imutáveis próprias, upload explícito por consentimento, SHA256/tamanho, CAS entre PCs, idempotência e ACK após fsync/commit; restore cria NOVA versão, preserva história. Identidade/membership server-side revalidadas depois da leitura e dentro da transação. Nenhuma assinatura suspensa apaga/bloqueia arquivos próprios; básico Orca livre/offline na mesma engine preservado.

Rotas de prévia: `/api/v1/tenants/{tenant}/files`, `.../files/{file}/versions`, `.../versions/{revision}/bytes`, `.../files/{file}/restore`; download de catálogo em `.../content/{content}/bytes`, exigindo publicação aprovada/JWS real, licença independente, grant premium quando aplicável, engine/compatibilidade e bytes/hash/tamanho verificados. Não expor URL de filesystem. Nenhum artefato real/instalador foi publicado; testes usam exclusivamente conteúdo sintético temporário e chaves efêmeras em memória.

**111 PASS / 0 FAIL** (76 baseline + 35 storage), pip check PASS, sem ResourceWarning. Migração 004 aditiva JÁ aplicada uma vez após backup somente do DB novo, versões 001–003 intactas, integridade/foreign keys ok. Estado operacional: zero contas/versões/bindings/objetos, provedor/signer/configuração legítima AUSENTES, privado continua 503 AUTH_NOT_CONFIGURED. Login email/Google/browser não implementado/configurado por esta fatia; nenhum Google/SMTP/serviço externo criado. Não confundir teste positivo com concessão real.

Filesystem+SQLite **não são transação distribuída atômica**: falha antes do commit pode deixar objeto órfão privado, reportado sem deletar; referência ausente/corrompida bloqueia bytes/restore/retry. Snapshot de operador copia DB+objetos referenciados com writers excluídos, verifica integridade e só conclui manifesto após fsync. Restore efetivamente testado em diretório isolado, nunca sobre DB ativo. É backup no MESMO host, não offsite/PITR/DR comercial nem RPO/RTO comprovados; não houve corte físico de energia. Falhas IO/crash foram injetadas em teste.

Transporte inicial: default 8 MiB, configurável com teto técnico 128 MiB, timeout de body 10s; não é quota/plano/janela offline. Sem streaming/resume grande ainda. Compatibilidade firmware aceita literal `==...`; range complexo bloqueia 409, declaração do cliente não é atestação. Sem Range/ETag/CDN; Range/If-None-Match rejeitados 400. Content-Digest/hash calculados dos bytes verificados. Downloads não instalam/controlam desktop/impressora nem executam/geram/fatiam objetos.

[Arquivos, hashes, testes e lacunas](WEB-FASE-03-EVIDENCIAS-20261108.md). Contrato privado canônico `contracts/object-storage.v1.json`, OpenAPI e bindings dos módulos atualizados; mestre/NEW-SITE/ledger/continuidade locais atualizados. Cofre/legado/Nginx/OS/tráfego/domínio/Slicer/hardware não alterados; nenhum gate promovido. Sem política comercial/retenção/GC/preço inventados.

### Revisão visual posterior t34u — 15:57 Europe/Rome
> minha colaboracao as paginas estao muito com cara de wordpress travado layout pronto precisa ser moderno e perfeito como o visual do filuna pagina premiada

[Registro literal e limites](approvals/WEB-VISUAL-REVISAO-T34-20261108.json). Palavra WordPress é comparação, não autorização para instalar CMS/plugin. O pedido posterior torna o aceite visual anterior condicionado à NOVA revisão DESIGN: **home/central NÃO homologadas**. Backend não altera hero/CSS/markup e não copia template para login. Frente DESIGN candidata separada, sem segundo backend; futura integração só após revisão coordenada. CU anterior navegou Central, cinco módulos e Acesso seguro no export offline, desktop sem recortes inesperados nos trechos vistos; não expôs dados privados nem comprovou mobile/HTTP vivo/100%. Isso não anula a crítica posterior nem promove FLN-VISUAL-C-100.


## Fase04 web — dual login implementado, não ativado (08/11/2026 Europe/Rome)

**Registro literal obrigatório** — decisão do dono no widget **t33s249**, **08/11/2026 15:52 Europe/Rome**: «Quero login com e-mail e senha e também com conta Google.» Esta escolha autoriza login da aplicação por ambos os métodos, não criar Google Project/OAuth app/SMTP externo, cobrar ou enviar email real nesta execução. Corrige lacuna de registro anterior.

Backend exclusivo `/srv/filuna-new-site`: Argon2id/email verificado/reset, sessões opacas BFF/cookie Secure HttpOnly SameSite, CSRF/origem, rotação/revogação, Google authorization-code+PKCE/state/nonce e **RS256** oficial (não confundir Ed25519 de entitlements). Identity estável issuer/sub, sem auto-link por email; vínculo exige provas independentes/recentes, sem mesclar outra conta/duplicar backups. Conta/tenant/roles/subscription nunca vêm como autoridade do cliente. Google/password não emitem grant premium; básico Orca/base continua livre/offline, premium só entitlement/licença legítimos separados. Nenhum hardware/Slicer retomado.

**148 testes PASS / 0 FAIL**, incluindo111 baseline e37 novos. HTTP real loopback com RSA/Argon2 reais efêmeros, email/JWKS em memória de TESTE; não é login real Google, TLS/browser, email operacional, pentest, MFA ou produção. Config auth/trust AUSENTE, DB operacional com zero contas/credenciais/sessões/fluxos; APIs privadas503. GET `/api/v1/auth/options` público200 só explica disponibilidade; nenhum token real/usuário admin/permissão inventados. Migração005 aditiva já aplicada UMA vez após backup somenteDBnovo, integridade/FK ok. Venv/dependências13 fixadas+hashes, pip check PASS. Sem operações no cofre/legado/nginx/OS/tráfego neste workstream.

Contratos canônicos: `contracts/auth-bff.v1.json`, `auth-config.v1.schema.json`, OpenAPI/índice. Front integra JSON/viewmodels; esta fase não fez UI/CSS/home/client_portal. Requisitos/rotas/config/limites e evidência reproduzível: [FASE04](WEB-FASE-04-EVIDENCIAS-20261108.md). Escolha literal estruturada: [registro](approvals/WEB-DUAL-LOGIN-ESCOLHA-20261108.json).

**Cronologia visual separada:** crítica t34 às15:57 exigiu revisão; t35 às16:19 Europe/Rome, «aprove e publique», aprovou candidata `/workspace/filuna-web-redesign-C-20261108/` e publicação pelo executor DESIGN/deploy separado. ZIP SHA256 `874b50d79ba3b1c301f77e19455e452cb03fe808870999fd1985e47307a78839`. Este aceite visual não ativa auth/pagamento nem prova deploy; executor backend não altera frontend/processos após esta coordenação. Publicação terá evidência própria, não é afirmada neste checkpoint.

Ativação futura só com origin HTTPS/BFF/TLS exatos; cliente Google WEB legítimo client_id/client_secret/callback registrada+consent; SMTP legítimo host465 TLS/sender verificado/username/credential; config privada canônica via Vault/Form/Handoff seguro pelo pai, nunca segredos em chat. Sem esses insumos a implementação permanece fechada, não paralisada nem simulada. SMTP fila memória limitada não é outbox durável. DB SQLite/devHTTP/snapshot mesmo host não são prontidão de produção/offsiteDR. Parâmetros de sessão/anti-abuso são segurança, não política comercial offline. Nenhum gate desktop/hardware/comercial promovido.


<a id="web-publicacao-c-t35-20261108"></a>
## Publicação visual web C — t35 (08/11/2026 Europe/Rome)

**Ordem literal do proprietário, t35u,16:19 Europe/Rome:** «aprove e publique». [Aceite estruturado](approvals/WEB-VISUAL-PUBLICACAO-T35-20261108.json). A candidata refeita após t34, ZIP SHA256 `874b50d79ba3b1c301f77e19455e452cb03fe808870999fd1985e47307a78839`, foi aprovada e **publicada realmente16:23:45 Europe/Rome** em https://filuna3d.com/, `/conta/`, `/conta/acesso/` e cinco áreas canônicas. [Evidências de publicação, hashes, limites e rollback](WEB-PUBLICACAO-C-T35-EVIDENCIAS-20261108.md). Registros anteriores de não publicação/visual pendente são históricos; a fase04 de autenticação permanece íntegra.

Publicação estática allowlisted via Nginx/TLS existente na mesma raiz nova, releaseversionada/symlink atômico. CSS/JS/Inter/logo/geometria mantêm bytes aprovados; somente links/paths canônicos, CSP de publicação e nota factual de aprovação adaptados. Home e Central agora usam a composição revista, sem reinterpretar o modelo aprovado nem alterar âncorasSlicer. Backend/app/client_portal/auth/storage/admin/migrações não alterados/reiniciados; contratos/índice recebem apenas registro aditivo da publicação. Nenhum segundo backend/CMS ou cofre/legado.

**Evidência:** 45 verificações públicasPASS com HTTPS/certificado normal,16 arquivos publicados por hash e145 referênciasallowlisted;2 consultasHTTP do box500Pingora, enquanto origem301HTTPS, limitação registrada sem bypass/DNS/proxy alterados. Admin anônimo401; arquivos internos/traversal/APIsprivadas404 no domínio. Backend148testesPASS reexecutados após deploy; design115estáticos+11doublesDOMPASS. Nginx-t/reloadPASS; snapshotnovo/config anterior e rollbackpreparados. No checkpoint inicial, Browser/CU público e execução real de rollback estavam NOT RUN neste executor; desktop público foi conferido posteriormente pelo coordenador, no adendo abaixo. Rollback executado e mobile público continuam NOT RUN. Não chamar47/47PASS ou auditoriaAA.

**Publicar visual NÃO ativa serviços:** e-mail/senha disabled, Google explicativo, nenhum cadastro/checkout/backup/download/premium simulado. Auth/trust operacionais continuam ausentes; privado loopback503, API pública404 por não publicação. Básico/baseOrca livres/offline na MESMAengine, sem carregar catálogoFiluna homologado sem direito válido; dados próprios preservados. G2/G3BLOCKED,G4NOTRUN,G5/G6BLOCKED e FLN-VISUAL-C-100 desktop/hardware não promovidos. Próximo passo do coordenador: inspeçãoCU no domínio e integração segura de serviços somente com configuração legítima/escopo específico.


### QA público desktop posterior — 08/11/2026 16:30 Europe/Rome

O coordenador executou Chrome público real, desktop1280×800/zoom100%, sem emulação e HTTPS normal sem aviso de certificado. Abriu a home, clicou **Central do cliente** para `/conta/` e **Acessar conta** para `/conta/acesso/`. Logo e estudo3D renderizaram; não houve fallback de fonte evidente nem texto/botões sobrepostos nas áreas vistas. Acesso mostrou conexão não configurada e campos desativados. Google não foi clicado, nenhuma credencial submetida. Executor inspecionou as três capturas brutas, sem segredos aparentes.

**Estado atual:** desktop público conferido no escopo acima; mobile público, comparação pixel/diff, auditoriaAA e homologação funcional/global **NOT RUN**. Captura1280×800 não fixa a equivalência CSS/device/DPI. Os45 checksHTTP/HTTPS anteriores continuam45; os2HTTPsimples500Pingora permanecem registrados, sem contarCU como mediçãoHTTP adicional. Não ativaGoogle/SMTP/login/pagamento nem altera gates/aplicativo/hardware.

[Capturas brutas e manifesto SHA256](evidencias/web-t35-public-20261108/MANIFEST-SHA256.json): home, Central e acesso, sem resize/crop/máscara. SHA256 do manifesto `f9680c1019474669fd6ae57c2c4b6f97e5c44f249d0d6fdc119cfdcf064ba790`. NOTRUN anterior do browser é histórico do checkpoint do executor, superado SOMENTE por esta inspeção desktop delimitada.


## Fase05 privada — releases/integridade/retenção: contrato proposto, implementação bloqueada
08/11/2026 Europe/Rome. t36/t37 resumidos pelo pai exigem integração nativa desde primeira versão, resumo/reabertura seguros e histórico completo; download inicial só beta/admin, máximo CINCO VERSÕES de instaladores, novo build só necessário. t38 literal: «a assinatira da priimeira versao sera feita pelo github conforme regras». Não equivale a dispensar ECDSA manifesto, SHA256, assinatura real de código/identidadeFiluna, consentimento/salvamento ou QAWindows; attestation GitHub é proveniência separada. [Especificação pré-código e testes NOT RUN](WEB-FASE-05-RELEASE-CONTRATO-20261108.md).

**Checkpoint honesto:** somente especificação/plano local e patch documental; código/rotas/migração/serviço privado novo NÃO implementados nesta fatia. Canal SSH existente deixou de responder a comandos, embora check local do master tenha sucesso; nenhuma mutação remota, acesso alternativo, build/CI/assinatura/publicação/pruning/remoção. Fase04 148 PASS é histórica, não novo baseline executado. Estado atual DB/zero dados NÃO reverificado. Config/trust não devem ser inventados, frontend aprovado/t35/APIs públicas/nginx/cofre/legado/desktop preservados por não atuação. Retomar leitura de NEW-SITE/mestres atuais pelo mesmo acesso existente antes de código.
