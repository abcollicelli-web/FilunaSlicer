# FILUNA SLICER — FERRAMENTA DE OBSERVABILIDADE E HOMOLOGAÇÃO FÍSICA ASSISTIDA DO BETA

**ID:** FLN-BETA-PHYSICAL-OBSERVABILITY-2026-11-07.1  
**Decisão expressa do proprietário:** 07/11/2026.  
**Abrangência:** **todos os fabricantes, modelos, variantes, bicos, firmwares e módulos OEM** que estiverem com `HOMOLOGACAO_FISICA=PENDENTE`.  
**Estado atual:** **REQUISITO APROVADO PARA PLANEJAMENTO; IMPLEMENTAÇÃO E ATIVAÇÃO NÃO INICIADAS**. Não criar coleta silenciosa nem executar testes físicos nesta fase.

## Regra superior atualizada — assistente de conexão e três provas físicas

Aplicar em conjunto a **seção 13 do [PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md §13](PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md)**: instalar somente motor-base + deltas OEM, sem catálogos completos de perfis; fazer download assinado/validado **sob demanda** para modelo/firmware/bico/material conectado; **abrir automaticamente checklist guiado na primeira conexão** de cada impressora física e avaliar rede/portas/protocolo pelo sistema, câmera real com prévia **local** e confirmação visual do proprietário, multimaterial com detecção de slots pelo sistema e validação humana dos filamentos. Separar resultado automático de confirmação humana e `N/A` comprovado.

**NOVA POLÍTICA DE PROMOÇÃO NO BETA:** após checklist local PASS → `LIBERADA_PARA_TESTE_DE_IMPRESSAO`; **1 impressão física válida completa** → `PARCIALMENTE_HOMOLOGADA` para a próxima unidade da mesma combinação modelo/variante/firmware/módulo; **3 impressões físicas distintas válidas com matriz inteira PASS** → `HOMOLOGADA_BETA_LIBERADA_IMPRESSAO` para aquele modelo e capacidades. Contador idempotente, sem contar testes repetidos/incompletos/simulados; erro crítico reabre gate. Teste local de cada nova unidade continua. Essa classificação automática **no beta** não é autorização para release estável sem aprovação do proprietário.

A norma anterior que atribuía à revisão humana a promoção final `HOMOLOGADO_FISICO` deve ser lida com esta atualização mais recente: **promoção de status dentro do beta é automática por três provas completas**; a revisão/aceite formal do proprietário continua exigida para distribuição estável, inclusão irrestrita no catálogo público ou mudança de canal. Proteções de privacidade, consentimento e servidor somente observador permanecem obrigatórias.

## 1. Objetivo e momento de ativação

Quando o **Filuna Slicer beta vigiado for aberto**, criar uma ferramenta própria de monitoramento técnico online, com **painel no servidor Filuna** e coletor integrado ao aplicativo/DeviceBridge, para acompanhar automaticamente as impressoras pendentes de homologação física durante o uso autorizado pelos participantes do beta.

Não se trata de um serviço que imprime sozinho. A ferramenta observa os trabalhos que o usuário efetivamente abre, fatia e manda imprimir, correlaciona as evidências do Filuna com os estados reais comunicados pela impressora e prepara um dossiê para homologação por **fabricante + modelo + variante + firmware + módulo OEM + versão do Filuna**.

**Ordem de fases vigente:** A) homologação técnica interna com portáteis oficiais e simuladores → B) compilação e testes do Filuna final → **C) beta vigiado com impressoras físicas e observabilidade** → D) estável após revisão e aprovação expressa do proprietário. O laboratório atual não deve iniciar impressão remota, observação de clientes ou build final por causa desta especificação.

## 2. O que acompanhar automaticamente — por instalação, impressora, trabalho e placa

| Domínio | Captura necessária no beta | Evidência e fonte |
|---|---|---|
| **Aplicativo aberto** | Abertura, versão/hash do executável e do módulo OEM, inicialização, erros, travamentos, atualização, integridade, reconexão; ciclo abrir/fechar/retomar | Eventos do app assinados/identificados; nunca inferir que a impressora está online só porque o Filuna abriu |
| **Arquivo/projeto aberto** | Tipo de arquivo STL/3MF/G-code, leitura/reabertura, número de placas, peças/cores, preservação 3MF, identificadores e **hashes dos arquivos** | Metadados mínimos no cliente; projeto/nomes/STL/3MF completos **não sobem automaticamente** |
| **Impressora e identidade** | Descoberta, pareamento autorizado, modelo, variante, firmware, bico(s), capacidades, serial pseudonimizado, módulo/ABI, rede conectada/desconectada, TTL do último estado | DeviceBridge OEM, protocolo/ACK genuínos; `ONLINE`, `OFFLINE`, `STALE`, `UNKNOWN` diferentes |
| **Perfis e fatiamento** | Preset efetivo após todas as heranças, bico, material, temperatura, velocidades, purga, suportes, tempo/gramas estimados, início/fim/erro do slicing, worker, versão/commit e SHA do preset | Orca interno do Filuna + módulo OEM de fato selecionado; nunca um Orca externo instalado |
| **AMS/ACE/CFS/multimaterial** | Presença e capacidade real, slots/bobinas/cores/material, mapeamento de extrusores, troca de cor, sequência, purga, troca de material, falta de filamento, enrosco/jam, sensores/erros suportados | Modelo de eventos e telemetria do fabricante; coletar apenas informações que o equipamento realmente fornece |
| **G-code gerado** | Hash bruto, tamanho, hash por plate, macros pertinentes ao modelo, material/tempo previsto, comandos críticos, diferenças com G-code OEM de referência da **mesma versão** e mesmas configurações | Comparador interno versionado; normalizar **somente** exceções provadas (p.ex. linha de data no HEADER_BLOCK), nunca movimentos/temperaturas/trocas |
| **G-code enviado e aceito** | Hash local do arquivo realmente enviado, nome/ID técnico pseudonimizado da tarefa, etapa de upload, checksums e ACK confirmado, identidade da impressora e da placa | Driver OEM de envio/transferência; falha/timeout de upload NÃO é aceite |
| **G-code realmente recebido/executado** | Quando o protocolo/firmware disponibilizar, digest do arquivo retido pela máquina, job ID/arquivo ativo e/ou trilha de comandos/telemetria para comparação com o enviado | **Três níveis distintos:** `GENERATED`, `SENT_ACK`, `PRINTER_CONFIRMED` e, se possível, `EXECUTION_OBSERVED`. Se a impressora não devolver arquivo/comandos, **não afirmar comparação byte-a-byte do G-code executado** |
| **Impressão física** | Início autorizado pelo usuário, estado/etapa, aquecimento, tempo, progresso, camadas, percentuais, interrupção, pausa, retomada, cancelamento, conclusão, cooldown e desconexão | Notificações OEM atuais com `observed_at`, `received_at`, firmware/versão, origem e sequenciamento |
| **Falha e resultado** | Erro de conexão, autenticação, upload, G-code, firmware, motor, AMS, sensor, filamento, impressora offline, timeout, impressora parada, abortada, concluída, resultado desconhecido | Código OEM bruto saneado + classificação Filuna, impacto/severidade, ação e repetição, último ACK, hashes e evidência técnica |

**Ponto essencial:** `PRINT_FINISHED` informado pela impressora **não é prova de peça perfeita**. Para homologar qualidade física, exigir quando aplicável comprovação humana autorizada (foto/medição, observação de defeitos e declaração do testador), incluindo peça realmente impressa, com consentimento específico para anexos.

## 3. Arquitetura obrigatória da futura ferramenta

```text
FILUNA SLICER BETA (Windows)
  Orca interno / Preparar / 3MF / G-code / Job manager
          + DeviceBridge / módulos OEM de rede / AMS / status
          -> coletor local assíncrono, somente eventos permitidos e saneados
          -> fila offline com sequência, expiração, reconciliação e reenvio
                          |
                       HTTPS/TLS
                          |
       API de diagnósticos beta do SERVIDOR FILUNA
          autenticação + tenant + política/opt-in por instalação
          deduplicação + ingestão idempotente + integridade
                          |
       Banco de evidências técnico / objetos cifrados opcionais
           + comparador de hashes e G-code minimizado
           + correlacionador de jobs e status OEM
           + detecção de erros/anomalias
                          |
        PAINEL PRIVADO DO PROPRIETÁRIO (somente leitura)
       Marca -> Modelo -> Firmware -> Módulo -> Impressora beta
          -> Trabalhos -> Fatiamento -> G-code -> AMS
          -> Upload/ACK -> Impressão -> Falha/Finalização
          -> Provas, inconsistências, gates e fila de revisão
```

- Reutilizar, **quando tecnicamente compatível**, o histórico de diagnóstico FUP-003 e os contratos `app_incidents`, `printer_observations`, `jobs/job_events` e `consents` já especificados em `FILUNA/ARQUITETURA-SERVIDOR-DADOS-REMOTO.md`. Não presumir que a infraestrutura antiga esteja homologada no novo fork.
- O **servidor Filuna observa**, compara e registra evidências; **não** fica com acesso LAN às impressoras nem com botão para iniciar/pausar/cancelar/imprimir. Administrador **jamais** controla a impressora de cliente. O cliente continua com controle local.
- Integrar a instrumentação de forma transversal ao pipeline comum e aos **contratos OEM versionados**, sem codificar dois modelos/firmwares fixos nem depender de abrir manualmente o dashboard para iniciar coleta consentida.
- Instalação/app permanece usável offline; sincronização automática de eventos permitidos quando houver conexão, com deduplicação de seq, retry e TTL. Sem afirmar status atual baseado em telemetria antiga.
- O painel deve filtrar por fabricante/modelo/firmware/capacidade/versão, detectar versões afetadas por erro e sinalizar regressões após atualização de SDK/firmware/origem OEM.

## 4. Contrato lógico mínimo dos eventos (proposta a implementar e versionar)

Chaves comuns: `schema_version`, `event_id`, `occurred_at_utc`, `received_at_utc`, `monotonic_sequence`, `installation_pseudonym`, `tenant_ref`, `printer_pseudonym`, `manufacturer_id`, `model_id`, `hardware_variant`, `firmware_version`, `filuna_version`, `filuna_build_sha256`, `oem_module_id`, `oem_module_version`, `oem_abi`, `job_correlation_id`, `plate_id`, `capability_id`, `event_type`, `event_source`, `state`, `error_code`, `evidence_digest`, `consent_scope`.

Tipos mínimos: `APP_OPENED`, `PROJECT_OPENED`, `FILE_PARSE_FAILED`, `PRINTER_DISCOVERED`, `PAIRING_AUTHORIZED`, `DEVICE_STATE`, `AMS_SLOT_STATE`, `MATERIAL_CHANGE`, `SLICE_STARTED`, `SLICE_FAILED`, `SLICE_COMPLETED`, `GCODE_GENERATED`, `OEM_GCODE_COMPARED`, `UPLOAD_STARTED`, `UPLOAD_FAILED`, `UPLOAD_ACK`, `PRINTER_JOB_CONFIRMED`, `PRINT_STARTED`, `PRINT_PROGRESS`, `PRINT_PAUSED`, `PRINT_RESUMED`, `PRINT_CANCELLED`, `PRINT_FAILED`, `PRINT_FINISHED`, `PRINT_STATUS_UNKNOWN`, `PRINTER_DISCONNECTED`, `FILUNA_CRASH`, `USER_PHYSICAL_RESULT`.

Para cada etapa manter `EVENT_OBSERVED` versus `ASSERTION_VERIFIED`; evento ausente, origem indisponível e N/A não viram PASS automaticamente.

## 5. Comparador de G-code real: quatro camadas

1. **Reprodutibilidade local:** arquivos A/B gerados pelo Filuna com geometria, posições, perfis e versão iguais; hash bruto e normalização **estritamente documentada** de diferenças não semânticas comprovadas.
2. **Paridade com oficial:** G-code do motor Filuna versus G-code do fatiador oficial da versão OEM fixada, mesmo modelo/perfil/material/placa; verificar comandos, movimento, velocidades, tempos, purga, troca de material e instruções específicas. Arquivos de referência pinados com hashes.
3. **Integridade do envio:** o hash do G-code que saiu do Filuna é vinculado ao ID do upload e ao ACK genuíno do protocolo; não confundir API que respondeu OK com arquivo realmente gravado na impressora.
4. **Execução em impressora física:** comparar identificação, checksum do job residente ou conteúdo/trace efetivamente disponibilizado pelo firmware **quando disponível e autorizado**; correlacionar telemetria de progresso, erro, finalização e consumo estimado versus observado. Na ausência do arquivo executado, marcar `EXECUTION_GCODE_UNAVAILABLE`, não inventar igualdade entre enviado e executado.

Anexar divergências mínimas e os hashes ao dossiê. Arquivos completos STL/3MF/G-code privados, fotos e gravações **somente por opt-in separado, expresso e limitado ao caso**, cifrados, com exclusão/expiração. Não fazer upload silencioso de projetos, comandos privados ou dados da LAN.

## 6. Painel de homologação e critérios de promoção

**Visão por modelo/firmware** com total de instalações participantes, impressoras observadas, sessões, trabalhos abertos, fatias válidas, uploads com ACK, impressões iniciadas, concluídas, falhas por causa e taxas por período. Separar telemetria SIMULADA, beta físico GENUÍNO, dados ausentes e testes reproduzidos. Não misturar telemetria de modelos ou firmwares diferentes.

Linha do tempo por job: `app aberto → arquivo aberto → perfil confirmado → slice → G-code → upload ACK → impressão física → status final → revisão`, com indicadores de falta de evidência e arquivos/hash de cada estágio.

Estados sugeridos por combinação modelo+firmware+capacidade:
`AGUARDANDO_BETA` → `MONITORANDO` → `EVIDENCIAS_INCOMPLETAS` / `FALHA_REPRODUZIDA` / `CANDIDATO_PASS_FISICO` → `HOMOLOGADO_FISICO` **somente após revisão formal e aprovação do proprietário**.

Um fim de impressão recebido automaticamente pode formar `CANDIDATO_PASS_FISICO`, mas **nunca libera marca/modelo/capacidade na versão estável sozinho**. Exigir matriz de casos positivos/negativos, eventos e ACKs válidos, falhas/recuperação, atualização de firmware/perfis, multimaterial quando suportado, evidência de peça quando aplicável e decisão humana auditada. Falhas críticas reabrem o gate daquele modelo/capacidade e alertam o dono.

## 7. Segurança, consentimento e proteção de dados — bloqueadores do beta

- Monitoração de **impressoras reais e atividades de usuários** exige participação beta autorizada, aviso transparente e **opt-in granular e revogável** para observações técnicas/estado de impressora; escopos separados para anexos, G-code completo, projeto/foto/câmera. Coleta de crash técnico mínimo segue o regime de privacidade aprovado, sem dados pessoais desnecessários.
- A coleta mínima deve evitar nome de projeto, G-code completo, caminhos, IP, MAC, serial em claro, conta OEM, tokens, código LAN, credencial, URL privada, vídeo/câmera e dados de terceiros. Permitir ver/baixar/apagar dados próprios conforme política aplicável; definir retenção e região antes de ativar.
- Tráfego cifrado e autenticado, dispositivo instalado vinculado ao tenant, pseudonimização, RBAC, trilhas de auditoria, limitação de frequência, esquema validado, proteção contra reenvio, revogação de token e segregação de ambiente. Painel de proprietário **somente leitura** e acesso mínimo.
- Não transmitir comandos para impressora pela API do servidor, não comandar remotamente, não ligar câmera nem disparar impressão como consequência de evento. Eventos de segurança devem bloquear promoção.
- Disponibilização e uso devem atender às regras de privacidade e proteção de dados aplicáveis na Europa (inclusive RGPD/GDPR); detalhes jurídicos e retenção exigem revisão antes de distribuir o beta.

## 8. Plano de entrega obrigatório — sem ativar agora

| Etapa | Entregável futuro | Gate mínimo |
|---|---|---|
| P0 — especificação | Documento/issue deste requisito, contrato e arquitetura aprovados | Este registro, **não implementação** |
| P1 — instrumentação e modelo de dados | Coletor local do app, schema dos eventos, fila offline, minimização de dados, consentimentos e revogação | Unitários + negativos, sem tráfego cliente antes do aceite |
| P2 — serviço Filuna | API segura de ingestão, storage técnico, correlacionador de job, hash/G-code, status, incidentes | Integridade/idempotência/carga/reconciliação/privacidade |
| P3 — painel privado beta | Dashboards por fabricante/modelo/firmware/módulo, timeline de jobs, filtros e alertas, revisão e evidência | Provar ausência de poderes de comando para admin |
| P4 — integração de laboratório | Simuladores OEM de falha de rede, AMS, upload/ACK, job concluído/falha, dados ausentes e auditoria de acesso | T3/T4 e Windows real sem disparo físico |
| P5 — abertura do beta físico | Ativar somente para participantes consentidos e módulos já aptos ao beta; observar impressões iniciadas pelo dono | Build final e gates internos prévios + autorização do proprietário |
| P6 — homologação modelo a modelo | Provas reais de hardware e 3MF/G-code, material/AMS, status, erro e peça | Revisão humana + aprovação expressa; depois liberar estável |

## 9. Condições de aceitação negativas obrigatórias

- App aberto com impressora desligada não gera falso `PRINTER_ONLINE`.
- `UPLOAD_STARTED` e `UPLOAD_ACK` não viram `PRINT_FINISHED`; `PRINT_FINISHED` não vira peça aprovada sem avaliação.
- Impressora que não fornece G-code residente não recebe `EXECUTED_GCODE_MATCH=PASS`.
- Evento duplicado/atrasado não gera dois jobs nem muda histórico; telemetria desatualizada vira `STALE/UNKNOWN`.
- Falta de permissão ou revogação corta observação/remessas conforme política, sem prejudicar fatiamento local.
- Modelo/firmware desconhecido não recebe homologação de outro modelo; atualização do OEM reabre gates afetados.
- Falhas de AMS/troca de ferramenta, desconexão, sensor, upload incompleto e impressão cancelada ficam explicitamente visíveis, com correlação ao job.
- Administrador/servidor **não conseguem** iniciar, pausar, cancelar, aquecer ou acessar câmera da impressora do participante.
- Nenhum evento cria automaticamente release, merge, hotfix ou promoção de modelo.

**Decisão final:** ferramenta universal e automática de observabilidade da fase beta, **monitorada no servidor Filuna**, sem escopo restrito a um modelo. O desenvolvimento e a implantação permanecem futuros e condicionados ao produto final, política de privacidade e gates já definidos.
