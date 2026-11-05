# FILUNA SLICER — MATRIZ MESTRE DE TESTES, EVIDÊNCIAS E LIBERAÇÃO 100%

**Contrato** FLN-QA-2026-11-05.1 · **Status** especificação inicial; **não são testes executados**.  
Subordinado a [REGRA-MESTRE-QUALIDADE.md](REGRA-MESTRE-QUALIDADE.md), [PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md](PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md), [ARQUITETURA-DEVICEBRIDGE-10-FABRICANTES.md](ARQUITETURA-DEVICEBRIDGE-10-FABRICANTES.md) e [TEST-ACCEPTANCE.md](TEST-ACCEPTANCE.md).

## 1. A fórmula do portão: 100% significa 100% de casos OBRIGATÓRIOS COM PASS

\`\`\`text
REQ_TOTAL = total de testes obrigatórios APLICÁVEIS ao escopo da versão/etapa/modelo
REQ_PASS  = total desses testes com execução real e evidência validada
GATE_SCORE = (REQ_TOTAL>0) ? 100 * REQ_PASS / REQ_TOTAL : 0
PASS = GATE_SCORE == 100% AND zero FAIL, BLOCKED, NOT RUN e SKIP obrigatório
       AND revisão de risco + aprovação do proprietário quando exigida
\`\`\`

- Contar testes de **comportamento e requisitos**, não somente cobertura de linhas. Uma linha sem cobertura não é necessariamente falha, mas **nenhum comportamento obrigatório pode ficar sem teste**.
- Os testes devem ter casos **positivos, negativos, fronteira, regressão e adversariais** conforme risco.
- Marcação \`N/A\` é válida somente com explicação e prova de que o equipamento/firmware não oferece a capacidade; o teste de **não disparar comando não suportado** continua obrigatório.
- Jamais retirar teste aplicável ou reduzir matriz depois de implementação para fabricar 100%. Mudança de escopo exigirá ADR, aprovação do dono e release notes.
- Um PASS de Ubuntu, simulador, software legado, SimplyPrint, outro fabricante ou outra versão de firmware **não conta como** PASS de Windows/hardware Filuna.
- Um bug obrigatório novo reabre gate afetado; commit novo invalida no mínimo testes impactados e seus descendentes; atualizar hash do binário/fixtures.
- Sem resultado/sem evidência = NOT RUN. Teste SKIP obrigatório = BLOCKED. Função que não existe = NOT IMPLEMENTED, nunca PASS.
- Status de projeto são distinguíveis: \`PROPOSTO\` / \`IMPLEMENTADO\` / \`TESTADO TECNICAMENTE\` / \`HOMOLOGADO NO EQUIPAMENTO\` / \`LIBERADO PUBLICAMENTE\`.

## 2. Formato imutável de evidência

Todo caso deve gerar registro com: \`case_id\`, \`req_id\`, \`gate\`, \`commit_sha\`, \`upstream_sha\`, \`build_run\`, \`binary_sha256\`, \`fixture_sha256\`, \`os_version\`, \`printer_manufacturer\`, \`printer_model\`, \`firmware\`, \`adapter_revision\`, \`connection_mode\`, \`test_steps\`, \`expected\`, \`actual\`, \`result\`, \`evidence_url\`, \`tester\`, \`reviewer\`, \`timestamp_utc\`. Na documentação cada campo inaplicável será \`N/A: razão\`, campos obrigatórios vazios bloqueiam.

- Logs sem segredos, IP externo, credenciais, nomes de clientes nem arquivos privados; hashes vinculam anexos de evidência.
- Screenshots de Home/Preparação/Dispositivos devem ser do **programa real** identificado por versão/hash, não de Figma/Grok ou HTML isolado.
- Peças físicas: registrar perfil, material, bico, firmware, foto, medições, peso e comentário de qualidade; fotos só com consentimento.
- Para firmware que muda, marcar como **reteste necessário** as capacidades afetadas.
- Gate de release acumula sumário reproduzível; nenhuma mudança em \`FILUNA/QA/release-evidence.json\` deve ser aprovada por uma única marcação manual sem validação das URLs/hashes e revisão humana.

## 3. Escada de testes

| Nível | Onde é feito | Como passa |
|---|---|---|
| T0 Documental | GitHub + leitura do legado | fontes e divergências inventariadas, requisitos ligados a decisões |
| T1 Estático | CI: clang-format, warnings, SPDX, segredo, schema, SBOM, análise de risco | todos checks requeridos PASS |
| T2 Unitário | CTest/C++/Python/JS, conforme recurso | toda ramificação relevante e casos negativos PASS |
| T3 Contrato | UI⇄core; core⇄adapter; adapter⇄protocolo; Hub⇄app | I/O, versões, tempo e erros PASS |
| T4 Integração | Linux/Windows VM + simuladores determinísticos de LAN | trabalho/sessão/ACK/timeout sem false positive |
| T5 Windows real | Win10/Win11 x64, GPU, DPI, driver de câmera, instalação | app e UI verdadeiros testados |
| T6 Hardware real | impressora, firmware, bobina e câmera identificados | todos recursos declarados funcionam |
| T7 Robustez/segurança | falhas injetadas, isolamento, ataque e soak | nenhum risco inaceitável e testes PASS |
| T8 Release e operação | pacote, assinatura, rollback, beta, aprovação e monitoramento | owner aprova versão/SHA e dossiê completo |

T2–T4 não dispensam T5/T6. Testes físicos são executados **com os modelos e firmware contemplados no release**. Não produzir etiquetas “compatível com toda marca” quando só um modelo foi provado.

## 4. Gates G0–G6

**G0 Inventário:** todos os documentos e inventários críticos lidos ou explicitamente pendentes; SHA de referência; licença de cada fonte OEM; baseline de upstream e G-code; referências visuais oficiais versionadas; hardware disponível; cronograma lógico e critérios de sucesso aprovados. Se leitura integral de fonte relevante não acabou, G0 continua **EM ANDAMENTO**, não PASS.

**G1 Contrato:** tela e estados (erro, vazio, offline, carregando, vencido, confirmação); dados reais, assets e tokens homologados; interfaces DeviceBridge, capacidades, autorização e rejeição; especificações de cada módulo e ameaças; casos de teste escritos antes de código; dono homologa design/procedimentos.

**G2 Código isolado:** branch pequena, revisão, licença, testes unitários/negativos, compilação do alvo, recurso escondido se ainda não pronto; falso ACK proibido; builds de QA não são versão final.

**G3 Regressão/integração:** comparar Orca upstream fixado vs Filuna em fixture e preset idênticos, 3MF multi-placa e material real, device mock/timeout/credencial, sem tocar perfis, conta/offline e segurança; 100% casos PASS.

**G4 Windows e hardware:** Win10 e Win11 x64, coexistência Orca/Filuna, 10 marcas no escopo “dez nativas” com equipamento real por marca; câmera, status, fila, início, pausa, retomada, cancelar, filamento, energia e desligamento conforme capacidades reais. Peças físicas e QA pelo dono. Falta de equipamento => BLOCKED.

**G5 Atualização/pacote:** assinatura, hash, licença, manifestos de A/B/C/D, pacote cumulativo, rollback, recuperação de dados e execução, UAC e ícone Filuna, nenhuma reinstalação por simples tema; testes falha parcial e atualizações em progresso.

**G6 Liberação:** nenhum caso obrigatório de G0–G5 pendente ou falho; aprovação expressa do dono por versão/SHA/hash; changelog, documentação, fontes e limitações, plano de incidente/rollback e monitoramento. Se não liberar, as funcionalidades podem continuar em **candidata privada**, honestamente.

## 5. Catálogo de casos obrigatórios por subsistema

Os IDs são contratos iniciais. Cada um deve desdobrar-se em **variações** (fixtures, firmware, falhas, materiais, DPI). Ainda não há PASS indicado neste catálogo.

| ID | Área | Procedimento e resultado esperado |
|---|---|---|
| CORE-001 | Orca | Abrir STL, manipular, fatiar e salvar G-code igual ao upstream pinado conforme tolerância explícita |
| CORE-002 | Orca | Abrir/salvar 3MF complexo; manter objetos, cores, negativos, suportes, perfis e metadados |
| CORE-003 | Orca | STEP/CAD por conversor presente e licenciado; preservar escala e malha |
| CORE-004 | Orca | Múltiplos objetos/partes: mover/rotacionar/escalar/dividir/booleanas/modificadores |
| CORE-005 | Orca | Pintura suporte/costura/multicor aplicada de verdade e codificada no G-code |
| CORE-006 | Orca | Purga, cores, \`T1\` onde cabível e multi-mesa no mesmo projeto, uma placa por cor/material conforme contrato |
| CORE-007 | Orca | Centralização XY, Z=0, sem sobreposição/fora da mesa e nada autoescalado indevidamente |
| CORE-008 | Orca | Perfil máquina/processo/filamento resolve herança e salva sem corromper perfil oficial |
| CORE-009 | Orca | Fatiamento local SEM Internet/conta/assinatura; export permitido |
| CORE-010 | Orca | Falha de mesh/G-code/CLI gera FAILED, nunca projeto pronto falso |
| CORE-011 | Orca | Presets/arquivos sobrevivem update, downgrade e reinstalação |
| CORE-012 | Orca | G-code do dispositivo não muda silenciosamente quando só a Home Filuna muda |
| UI-001 | Marca | Logotipo Filuna 3D, ícone F, paleta, mascote usados com fidelidade e alfas corretos |
| UI-002 | Home | Barra, lateral, faixas e último projeto REAL exatamente segundo revisão homologada |
| UI-003 | Preparar | Controles modificam o estado nativo Orca, não apenas HTML ilustrativo |
| UI-004 | UI | Navegação/dados reais, botões funcionais, sem scroll horizontal e sem corte em DPI 100/125/150/200 |
| UI-005 | UI | Resoluções 1280×720, 1920×1080 e 2560×1440 + testes acessibilidade, teclado e foco |
| UI-006 | UI | Carregamento, erro, offline, sem conta e sem permissões com mensagens corretas |
| UI-007 | UI | Tema original/azul/escuro mantém contraste e ícones legíveis |
| DEV-001 | Descoberta | Somente LAN permitida; mDNS/SSDP/anúncio e IP manual, sem confundir marcas |
| DEV-002 | Identidade | IP alterado, impressoras duplicadas e firmwares conflitantes não atribuem falsa identidade |
| DEV-003 | Autorização | Código LAN/conta oficial incorretos ou revogados geram recusas; credenciais protegidas |
| DEV-004 | Protocolo | Serviços desconhecidos/portas abertas não são interpretados como recursos disponíveis |
| DEV-005 | ACK | Stub Orca/adapter sem operação não pode retornar sucesso |
| DEV-006 | Upload | Arquivo parcial e checksum divergente geram erro |
| DEV-007 | Início | Sem confirmação humana, job real e ACK: **nenhuma impressão começa** |
| DEV-008 | Início | Timeout produz UNKNOWN e reconciliação antes de qualquer retry |
| DEV-009 | Comando | Pausa/retomar/cancelar do firmware suportado, com ACK e estado |
| DEV-010 | Comando | Capability UNSUPPORTED significa controle inativo e zero comandos enviados |
| DEV-011 | Comando | Duas impressoras diferentes não recebem comando cruzado |
| DEV-012 | Comando | Fechar/abrir Filuna e reconectar não duplica job em andamento |
| DEV-013 | Descoberta | Sem autorização, nenhuma varredura LAN ativa, sem Internet scanning |
| CAM-001 | Câmera | Autorização abre imagem real, origem/idade visível, codec disponível |
| CAM-002 | Câmera | Câmera offline/senha errada gera sem imagem/erro claro |
| CAM-003 | Câmera | 30 min de stream e 100 aberturas/fechamentos sem vazamento significativo |
| CAM-004 | Privacidade | Nenhum stream/imagem sobe ao servidor sem consentimento |
| FIL-001 | Filamento | Leitura de slots reais, identificação de material/cor/idade |
| FIL-002 | Filamento | AMS/ACE/CFS/Canvas/QIDI Box ausentes não aparecem ativos |
| FIL-003 | Filamento | ASA≠ABS, PETG-CF≠PETG, PLA Silk≠PLA comum |
| FIL-004 | Filamento | Falta perfil específico permite Generic da mesma família só com aviso |
| FIL-005 | Filamento | Mudança de slot só se protocolo e estado permitirem, com ACK |
| FIL-006 | Filamento | Dados da impressora não inventam peso remanescente nem custo de bobina |
| DATA-001 | Conta | ACTIVE, PAST_DUE, EXPIRED, CANCEL_AT_PERIOD_END, REACTIVATED, NETWORK_UNAVAILABLE: Orca sempre livre |
| DATA-002 | Offline | Outbox/sync e revisões não duplicam, sobrescrevem ou perdem dados |
| DATA-003 | Privacidade | Tenant A nunca consulta nem altera dados/segredos/impressora de B |
| DATA-004 | Backup | Restaura 100% do que teve ACK e hash, alerta para não sincronizado |
| DATA-005 | Estoque | Evento de consumo vinculado ao job, bobina e tempo real, sem valor fictício |
| PARAM-001 | Parametria | Chaveiro Botton gera geometria verdadeira no PC, com validação de mínimos/máximos |
| PARAM-002 | Parametria | Conversor imagem→3MF só geometria PLANA colorida, sem prometer escultura volumétrica |
| PARAM-003 | Parametria | Lua/árvore/luminária obedecem tamanho, encaixe, cor, mesa, Z e testes físicos |
| PARAM-004 | Parametria | Malha sem serrilhado desnecessário, manifold, texto e imagens definidos, importação real |
| MODEL-001 | Modelos | Catálogo print-only não expõe mestre na UI, externos/paramétricos exportam |
| MODEL-002 | Modelos | Um modelo protegido não perde autorização do servidor por trocar nome local |
| AT-001 | Ateliê | Pedido/cliente/orçamento/financeiro/relação custo e venda consistentes |
| AT-002 | Ateliê | Estoque QR/NFC, cadastro de bobinas e baixa por produção reproduzíveis |
| AT-003 | Produção | Fila e agendamento jamais iniciam impressão sozinhos |
| AT-004 | Produção | Pular objeto exige nomes reais do equipamento e confirmação, nunca último |
| AT-005 | E-commerce | Mercado Livre OAuth, estoque, anúncios e etiquetas com teste real autorizado |
| IA-001 | Assistente | Só KB aprovada atende; IA não declara ferramenta/arquivo inexistente |
| IA-002 | Assistente | Chaves no servidor, quotas/custos e consentimento de imagem/diagnóstico |
| UPD-001 | Atualização | Manifesto/hash/assinatura inválidos bloqueiam alteração |
| UPD-002 | Atualização | Falha 5/50/95% da cópia recupera app, trabalho e perfis |
| UPD-003 | Atualização | Atualizar código/GUI exige consentimento; sem trabalho salvo não fecha |
| UPD-004 | Atualização | Reinício reabre e restaura projetos/sessão |
| UPD-005 | Atualização | Cliente antigo para último pacote cumulativo, sem sequência de dezenas de downloads |
| UPD-006 | Atualização | Mudança A/B de dados não impõe reinstalar pacote gigante |
| SEC-001 | Segurança | Nenhum token, IP externo, serial, arquivo de cliente ou câmera vazando em logs |
| SEC-002 | Segurança | Start/move/heat não aceitam protocolo/firmware errado |
| SEC-003 | Licença | AGPL/sources/créditos/SBOM e licenças por arquivo OEM presentes |
| REL-001 | Release | Windows 10/11 reais, GPU/presets/Orca lado a lado aprovados |
| REL-002 | Release | Owner aprova versão/SHA/hash/canal com evidência explícita |
| REL-003 | Release | Suporte/rollback/monitoramento de incidentes revisados |

## 6. Matriz física para DEZ fabricantes

**Atenção:** as dez marcas são meta de produto; **a versão só pode se anunciar compatível com dez marcas** depois de homologar pelo menos um modelo de cada com firmware identificado e todas as capacidades anunciadas. Nos modelos sem determinada capacidade, testar obrigatoriamente que o Filuna a deixa indisponível e **não transmite comandos inválidos**.

| Fabricante / piloto | Registro de variante | Status inicial |
|---|---|---|
| Elegoo Centauri Carbon 2 | \`VND-ELEGOO-*\` | NOT RUN |
| Anycubic Kobra 3 V2 | \`VND-ANYCUBIC-*\` | NOT RUN |
| Bambu A1 | \`VND-BAMBU-*\` | NOT RUN |
| Creality K1/K2 — modelo e firmware a fixar | \`VND-CREALITY-*\` | NOT RUN |
| Flashforge Adventurer 5M/AD5X — modelo a fixar | \`VND-FLASHFORGE-*\` | NOT RUN |
| QIDI Plus4 | \`VND-QIDI-*\` | NOT RUN |
| Snapmaker U1 | \`VND-SNAPMAKER-*\` | NOT RUN |
| Prusa MK4S/CORE One — modelo a fixar | \`VND-PRUSA-*\` | NOT RUN |
| UltiMaker S-series — modelo a fixar | \`VND-ULTIMAKER-*\` | NOT RUN |
| Raise3D Pro3/E2 — modelo a fixar | \`VND-RAISE3D-*\` | NOT RUN |

Para cada um, desdobrar **12 casos básicos**:
- **D1** Descoberta automática/autorização ou fallback documentado;
- **D2** Pareamento LAN/conta/chave/PIN, erro e revogação;
- **D3** Identidade, estado, temperaturas, progresso e freshness;
- **D4** Envio e integridade de arquivo G-code/3MF aceito;
- **D5** Início **com confirmação** e recibo; sem confirmação zero início;
- **D6** Pausar, retomar, cancelar ou não suportado sem comando;
- **D7** Câmera live/snapshot e exceções de codec/ausência;
- **D8** Material/cor/slot/filamento multimaterial ou modo indisponível;
- **D9** Peça física e comparação de dimensão/qualidade;
- **D10** Queda de rede, token, firmware, timeout, sleep e reconciliação;
- **D11** Isolamento entre duas impressoras distintas;
- **D12** App fecha/reabre sem duplicar job nem perder estado.

**120 casos-base** para dez marcas, além dos testes de fronteira e variações. Não representam 120 PASS. Para cada recurso citado no marketing exigir evidência do Filuna, não uma tabela de terceiros.

## 7. Testes de segurança e atualização de alta criticidade

- UI confirma → pedido com \`request_id\` → validação de capability/estado → envio → ACK semântico → consulta estado; perder ACK em qualquer ponto não repete impressão cegamente.
- Um serviço exposto em porta local não é validação de identidade nem autorização. Spoofing de mDNS/SSDP, URLs maliciosas, IP trocado, rede convidada/VLAN, firmware incompatível e payload grande devem ser rejeitados.
- Upload de firmware e modificações físicas não acontecem automaticamente ou por reconhecimento.
- Filuna não usa API privada de SimplyPrint; nenhuma credencial de fabricante é reaproveitada sem consentimento e documentação.
- Atualizador deve rejeitar downgrade malicioso, assinatura inválida, pacote incompleto, revogação, disco cheio, app com sessão não salva; testar backup, rollback e coexistência.
- Câmera não pode ser gravada em telemetria sem autorização; stream bloqueado ou expirado não é “ao vivo”.
- Instalação somente com editor/assinatura verificável; publisher “SoftFever” no pacote anterior é achado a corrigir antes de release.

## 8. Aprovação por lote de trabalho

Cada item de planejamento exige:
\`\`\`text
FICHA:
  ID, título, origem(s), revisão, dono, prioridade, predecessores, escopo
  código/arquivo, produto/firmware, risco, licença, estado documental
  critério de PASS por requisito, fixtures e hashes, testes positivos/negativos
  execução CI, evidência Windows, evidência física (quando cabível)
  resultado PASS/FAIL/BLOCKED/NOT RUN, incidentes e severidade
  revisão, aprovação por SHA/canal, rollback, data
REGRA:
  if nenhum teste obrigatório definido ou se algum !== PASS:
      bloqueia promoção
  if hardware necessário e não testado:
      bloqueia anúncio de compatibilidade
  if assinatura/revisão legal pendente:
      bloqueia distribuição
\`\`\`

## 9. Qualidade após lançamento

- Receber incidentes com versão, modelo, firmware, etapa, última etapa boa e erro sanitizado (padrão FUP-003 do legado).
- Alertar regressão de firmware/OEM/Orca em testes canário; novo firmware revoga aprovação das capacidades afetadas até reteste.
- Retenção e privacidade conforme consentimento, sem dump de projeto ou credenciais.
- Corrigir por canal interno com rollback, reenviar suites impactadas, atualizar docs, fechar defeito com evidências.
- A inexistência de bugs observados não é prova matemática de “zero defeitos”; o objetivo é **não liberar problemas conhecidos nem itens sem teste**.

**NENHUM CASO DESTE CATÁLOGO FOI EXECUTADO POR SUA CRIAÇÃO.** O check estático de fundação do PR #2 é evidência limitada de documentos presentes e coerentes, não testes de impressoras.


## 10. Testes obrigatórios de servidor e impressão remota

Matriz específica [ARQUITETURA-SERVIDOR-DADOS-REMOTO.md](ARQUITETURA-SERVIDOR-DADOS-REMOTO.md) define **SRV-QA-001 a SRV-QA-020**, incluindo HTTPS/DB privado, isolamento por tenant, backup real/restore, sincronização idempotente, monitoramento **sem permissão de comando ao administrador Filuna**, consentimento por dispositivo, revogação, modo remoto default OFF, comandos assinados/anti-replay/timeout e confirmação da impressão. O plano encadeado [CRONOGRAMA-DEPENDENCIAS.json](CRONOGRAMA-DEPENDENCIAS.json) liga implementação de SRV-08 à homologação local de Elegoo CC2 e Anycubic Kobra 3 V2, e SRV-10 ao release local e à validação remota independente.

É obrigatório diferenciar o proprietário do software (**admin Filuna: apenas incidentes e observações permitidas**) do proprietário da **impressora** (**cliente: permissões exclusivas, com compartilhamento explícito e revogável**). O servidor pode administrar a fila de mensagens autenticadas para a instalação do cliente, mas **jamais pode assumir sessão de comando do administrador Filuna**.

O status de todas as fases remotas permanece NOT RUN/BLOCKED até haver teste físico autorizado; a simples criação de schemas/rotas não constitui conexão nem QA.


### Registro G0 documental (2026-11-05 23:24:00 CET)

Casos executados: ver [G0-TESTES-CASOS-2026-11-05.md](G0-TESTES-CASOS-2026-11-05.md). Resultado gate G0 = **PASS DOCUMENTAL**. Casos Windows/hardware = gate G4 (**NOT RUN**).
