# FILUNA — REGRA MESTRE DE QUALIDADE E CRESCIMENTO CONTROLADO

**Contrato:** FLN-QG-2026-11-05.1  
**Decisão:** ordem expressa do proprietário, 05/11/2026.  
**Âmbito:** novo fork `abcollicelli-web/FilunaSlicer`; todas as telas, configurações, motores, perfis, fabricantes, plugins, releases e atualizações.  
**Estado de incorporação:** candidata nesta branch até integração/CI; esta regra não transforma QA pendente em PASS.

## Princípio inegociável

**NENHUMA ETAPA AVANÇA SE QUALQUER REQUISITO OBRIGATÓRIO, TESTE OU EVIDÊNCIA DA ETAPA ANTERIOR ESTIVER FALHANDO, PENDENTE, NÃO EXECUTADO OU SEM PROVA.** Um único FAIL/BLOCKED/UNKNOWN interrompe a promoção. `build success` ≠ `test pass` ≠ `homologação do proprietário` ≠ `release público`. Não existem dispensas tácitas, aprovação por silêncio, botão decorativo contado como funcional ou sucesso simulado.

- Matriz única por mudança: `ID`, `versão/commit`, `arquivo/linha`, `origem/revisão`, `requisito`, `escopo`, `estado documental`, `classe A/B/C/D`, `risco`, `teste`, `resultado`, `evidência verificável`, `aprovação do proprietário`.
- Estados documentais: **HOMOLOGADO**, **PROPOSTO**, **IMPLEMENTADO NÃO TESTADO**, **EM TESTE**, **REJEITADO**, **REVOGADO**, **CONFLITANTE** e **SEM EVIDÊNCIA**. Testes usam **PASS/FAIL/BLOCKED/NOT RUN**; ausência de evidência = **NOT RUN**, nunca PASS.
- Uma regra proposta do Grok não altera código, não libera distribuição e não substitui homologação. O contrato visual específico mais recente prevalece sobre esboços anteriores quando declarar expressamente a prioridade; conflitos sem regra de precedência exigem decisão do proprietário.
- Todo commit relevante atualiza no mesmo PR o inventário de requisitos, changelog/registro de decisão, critérios, testes, resultados e limitações. Preservar commits e documentos antigos como histórico, sem reescrevê-los.
- Nunca declarar `sem erros` sem o conjunto de evidências definido; comunicar limitações, ambientes não testados e bugs abertos.

## Portões sequenciais — cada portão é bloqueador

| Gate | O que é exigido | Evidência para PASS |
|---|---|---|
| **G0 Inventário** | Ler fontes mestras, identificar conflitos, baseline upstream/Filuna, riscos, arquivo por arquivo no escopo | Mapa de fontes versionado com caminhos, hashes/SHAs e status sem itens obrigatórios órfãos |
| **G1 Contrato e visual** | Critérios mensuráveis, matriz de componentes, protótipo autorizado, identidade, fluxos, estados reais vs demonstração | SPEC revisada, evidência visual de referência, aceitação explícita do proprietário para mudanças de aparência |
| **G2 Implementação local** | Um recurso isolado por vez, feature flag quando cabível, sem alterar motor sem justificativa, entrada/saída reais | Lint, testes unitários negativos/positivos, smoke, compilação e cobertura dos casos afetados todos PASS |
| **G3 Integração/regressão** | 3MF/STL/STEP, mesa/Z, escala, cores/purga, perfis, fatiamento, UI, dados, acessos, licença e segurança | Comparação com Orca fixado, testes automatizados e testes reais de integração, nenhum item obrigatório FAIL/NOT RUN |
| **G4-I Windows e testes internos** | Windows x64, portáteis oficiais com versão/SHA pinados; fluxo Preparar/Dispositivos/Fatiar/Enviar no laboratório, simulação OEM representativa, testes negativos e segurança | Logs/screenshot, comparações Filuna×OEM realmente distintas, G-code de mesmo perfil, simulador/ACK quando aplicável; **sem exigir impressora física antes do build final** |
| **G5 Pacote/canal** | Classe A/B/C/D, manifesto e arquivos verificados, assinatura, SHA256, rollback, notas e consentimento | Artefatos identificados por SHA, assinatura verificável, teste de atualização/retorno e aceite do proprietário |
| **G6 Liberação** | Não publicar versões públicas antes de todos os gates de release, sem quebrar acesso livre | Registro completo, aprovação nominal do dono para versão/SHA/canal; monitoramento e rollback pronto |

**Progressão atualizada por decisão posterior do proprietário (07/11/2026):** G0 PASS → G1 PASS → G2 PASS → G3 PASS → **G4-I (interno) PASS** → **compilação final Filuna e regressão do binário final** → G5 PASS → **G6-B (beta vigiado, mediante aprovação expressa)** → **G4-F (validação física real durante o beta)** → **G6-S (estável, mediante nova aprovação expressa)**. Os requisitos internos continuam bloqueadores; não declarar G4-I PASS com qualquer capacidade interna obrigatória FALHA/NOT RUN. A ausência de hardware físico é registrada em G4-F como **PENDENTE_BETA**, não bloqueia a preparação do build final nem o beta supervisionado. Builds técnicos e provas oficiais A/B isoladas não são homologação Filuna. A existência de artefato no GitHub não o aprova.

## Decisão vigente — motor-base sem perfis OEM no instalador e assistente de homologação beta (07/11/2026)

**Autoridade:** ordem expressa do proprietário consolidada no [PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md §13](PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md). O Filuna final será compilado como **um único motor-base Orca com deltas de código OEM legalmente reutilizáveis**, **sem empacotar no instalador o catálogo de perfis de impressora, filamento e impressão de todos os fabricantes**. Na conexão autorizada de cada impressora, o cliente Filuna obtém perfis compatíveis assinados/validados **sob demanda**, preserva cache offline e executa **teste guiado** por capacidade: portas/rede/identidade/ACK e G-code com prova do próprio sistema; câmera com render local e confirmação visual do usuário; AMS/ACE/CFS/IFS com detecção dos slots e confirmação dos filamentos pelo usuário. Não aceitar confirmação manual como substituto de testes técnicos.

No beta, a impressora inédita com checklist PASS recebe **"Impressora liberada para teste de impressão"**; depois do primeiro teste físico completo e válido, outra unidade compatível recebe **"Impressora parcialmente homologada"**; com **três testes físicos reais, distintos, completos e auditáveis** com todos os gates PASS, o modelo/variante/firmware/capacidades recebe **"Impressora homologada — liberada para impressão"**. Os testes locais da nova unidade permanecem necessários. Falha crítica retira a condição e exige regressão; nada promove binário/release automaticamente. Distinção de homologação do beta versus aprovação do release mantida.

**Estado:** registrado no plano mestre; **NOT RUN para implementação e QA funcional**, sem alterar o pacote atual ou a etapa de homologação física.

## Regra posterior prioritária — separar homologação interna de física

**Autoridade:** decisão do proprietário, 07/11/2026, consolidada em [REGRA-FASES-HOMOLOGACAO-INTERNA-BETA-FISICA-20261107.md](REGRA-FASES-HOMOLOGACAO-INTERNA-BETA-FISICA-20261107.md). Esta regra substitui **somente o antigo requisito temporal** de G4 que exigia hardware antes de G5/G6. Não dispensa nenhum gate interno nem permite lançar produto estável antes do teste físico beta e da aprovação do proprietário. A única atividade voluntariamente adiada é a linha Bambu com Bambuddy; demais QA prossegue agora. Nenhum comando de impressão física pode ser disparado por simulador/CI ou sem autorização do proprietário/operador.

## Fronteiras técnicas e de governança

1. `upstream-mirror` permanece cópia do Orca; motor/presets/algoritmos são preservados. Alterações de extensão são mínimas e listadas em `ARCHITECTURE.md`. Não substituir ou fundir motores silenciosamente. Orca livre e configuração manual permanecem disponíveis offline e após término de assinatura.
2. Nova implementação: C++17/wxWidgets/OrcaSlicer 2.5.0-dev; legado Windows C#/.NET/WebView2/Orca 2.4.2 é **referência funcional e visual**, NÃO base para copiar código/binários/serviços automaticamente. Site, Windows anterior e novo fork são projetos distintos.
3. Identidade pública Filuna, mas conservar nomes internos técnicos definidos no contrato para compatibilidade com 3MF, G-code, perfis e traduções. `orcaslicer://` pode conflitar com Orca instalado; não mudar associações sem teste de coexistência.
4. Tokens visuais/estrutura, menus e conteúdo vêm do contrato Grok homologado em GitHub; não fabricar números, projetos recentes, impressoras ou disponibilidade em produção. A referência visual viva não gera deploy ou homologação automática.
5. Adaptadores de fabricante separados, licença e proveniência registradas, matriz de capacidades por modelo/firmware. Função não implementada deve retornar erro explícito; nunca retornar falso sucesso de conexão/envio/impressão. Nenhuma impressão se inicia sem confirmação do usuário.
6. Pacotes de rede proprietários não são distribuídos como parte do Filuna sem direito específico. Revisar AGPL, atribuição, segurança, segredos e comunicação autorizada. Plugins Python não são sandbox forte.
7. A/B só dados não executáveis em diretório usuário; C código/UI e D motor/runtime requerem testes e controle apropriados. TLS, assinatura de manifesto e hash SHA-256, preservação de sessão, consentimento e rollback. Instalador completo apenas quando indispensável; nunca matar processo com trabalho não salvo ou atualizar firmware automaticamente.
8. Catálogo 3D `print-only` não exporta geometria original; externos e paramétricos gerados pelo cliente podem exportar. Sem proteção criptográfica comprovada, não anunciar impossibilidade absoluta de extração.
9. Nada de deploy no site legado, produção ou clientes a partir de branch candidata. Lançamento previsto para 15/12/2026 é meta, não liberação automática.

## Registro obrigatório de teste

Para cada caso manter: `case_id`, `gate`, `requirement_id`, `commit_sha`, `build_run`, `platform`, `printer_model`, `firmware`, `input_fixture_sha256`, `steps`, `expected`, `actual`, `result`, `evidence_url`, `tester`, `timestamp_utc`. Os campos inaplicáveis são explicitamente `N/A`; campos necessários vazios bloqueiam PASS.

Mínimos transversais: aplicação abre no Windows real; STL/3MF e presets antigos preservados; G-code comparado com upstream; instalação lado a lado com Orca; sem plano/offline o fatiamento livre funciona; sem impressão iniciada sem confirmação; atualização/rollback protegem arquivos; janela não ultrapassa tela; sem scroll horizontal; sem botões fictícios; nenhuma funcionalidade declarada disponível antes de funcionar; detecção/manipulação de erros de dispositivo; segurança e fonte AGPL verificadas.

## Hierarquia de fontes e migração

1. Ordem explícita atual do proprietário e regra comprovadamente homologada para seu escopo.
2. Contratos mestras e revisões datadas `abcollicelli-web/Filuna/Filuna-Slicer/`; prevalece a especificação mais recente específica quando substituir explicitamente uma anterior.
3. Estado e código reais do antigo `abcollicelli-web/filuna-slicer`: **evidência de implementação**, não homologação por si só.
4. Estado e código reais do novo `abcollicelli-web/FilunaSlicer` em SHA determinado.
5. `AGENTS.md`, `CONTRIBUTING.md` e licença do Orca para disciplina de código, compatibilidade e tests. Regras legais e de segurança não se anulam por um desenho.

Ver `FILUNA/MAPA-MIGRACAO-PERSONALIZACAO.md` e `FILUNA/TEST-ACCEPTANCE.md`. O primeiro gate habilitado nesta entrega é exclusivamente **G0**: documentar e testar a fundação; nenhum módulo funcional foi declarado pronto.


## Regra vinculante adicional — servidor Filuna e impressão remota segura

**Ordem expressa do proprietário (05/11/2026):** o desenvolvimento do Windows estará conectado à arquitetura do servidor desde o início; porém **servidor não poderá comandar impressoras de clientes por conta administrativa Filuna**, mesmo que receba diagnósticos e métricas da conexão. O cliente poderá futuramente monitorar e controlar suas próprias impressoras a distância, mas **somente após o dispositivo local, a conexão LAN e o serviço remoto passarem por 100% de testes obrigatórios com evidências reais**.

1. **Servidor e dados:** o aplicativo usa API HTTPS versionada por DNS, banco privado interno, identidade Filuna ID, segregação de tenant, backups cifrados fora do host, restore efetivamente ensaiado, filas transacionais/idempotentes, status de sincronização confirmado e export dos dados do cliente. IP/credenciais/DSN nunca no executável ou no Git. O legado \`SQLite DEV\` não autoriza declarar banco de produção existente.
2. **Observabilidade admin:** Filuna pode conhecer **somente** erros de software sanitizados e saúde técnica mínima de impressoras quando o titular autorizar; suporte não pode abrir câmera, ler projeto, executar comandos ou iniciar/pausar/cancelar impressões. É uma fronteira de API e autorização, não apenas um botão escondido.
3. **Acesso remoto do cliente:** futuro e **desligado por padrão**. Usar conta própria, concessão por dispositivo, consentimento explícito, MFA quando necessário, comando firmado com validade/nonce/ID, verificação local pelo agente, capabilities e ACK verdadeiro. O servidor recebe fila e devolve recibo; não conecta diretamente à LAN do cliente. Com app/agent desligado, não prometer impressão.
4. **Gate remoto:** o nó \`SRV-08\` só se habilita após local Elegoo CC2 e Anycubic Kobra 3 V2 aprovados, \`SRV-07\` em modo OFF testado e auth/storage/observabilidade concluídos. A liberação \`SRV-10\` sucede ainda o release local \`RELEASE-38\`. Um só requisito **FAIL / BLOCKED / NOT RUN** bloqueia o remoto; os testes \`SRV-QA-001..020\` estão em [ARQUITETURA-SERVIDOR-DADOS-REMOTO.md](ARQUITETURA-SERVIDOR-DADOS-REMOTO.md).
5. **Proteção de dados é verificável, não promessa absoluta:** nenhum dado com ACK servidor pode sumir sem incidente P0 e restauração; a fila offline só é descrita como salva localmente até confirmação. O cliente mantém o direito à exclusão de dados e à privacidade. Testar restore/backup com hashes e retenção; não declarar “dados impossíveis de apagar”.
6. **Nova funcionalidade SEM LIGAÇÃO é bloqueada:** qualquer feature nova declara ID, dependências no [CRONOGRAMA-DEPENDENCIAS.json](CRONOGRAMA-DEPENDENCIAS.json), contratos, versão, testes e plano de rollback antes de código. Módulos órfãos não entram em build/release, e uma conexão remota não adquire permissões por herança de outros modos.

**Esta atualização é DOCUMENTAL na branch de qualidade; nenhum endpoint remoto foi ativado nem servidor modificado.**


## ADENDO VINCULANTE — POLÍTICA UNIVERSAL DE DISPOSITIVOS E MÓDULOS OEM (06/11/2026)

**Decisão expressa do proprietário: aplica-se a TODAS as impressoras, marcas, famílias e futuros adaptadores Filuna.** Preservar a aplicação Windows Filuna Slicer já compilada, a GUI e o motor-base Orca nativo. Não iniciar outro aplicativo de fatiamento instalado nem popular a lista de impressão com todos os presets disponíveis.

**Ordem de execução obrigatória por identidade/firmware:**
1. Dispositivos descobre a impressora por protocolos e portas locais suportados/documentados (mDNS/SSDP/UDP, HTTP, MQTT/MQTT-TLS, WebSocket etc. conforme modelo). Porta aberta não implica acesso não autenticado. Detectar capacidades de maneira permitida; sem scan amplo indiscriminado e sem autenticação presumida.
2. Verificar fabricante/modelo/revisão/firmware e autorização/pareamento exigidos; persistir registro privado do cliente, credenciais no cofre apropriado e relação módulo+ABI+versão. Sem dispositivo físico não alegar descoberta real, conexão, firmware ou ACK.
3. **Primeiro aproveitar os protocolos e recursos de rede disponíveis** para descoberta/telemetria/transferência/comandos suportados, com a autenticação que cada serviço exigir. **Somente quando esses recursos forem insuficientes**, tentar o modo LAN com código fornecido pela impressora, quando o firmware permitir. LAN Code pode ser credencial do próprio MQTT/FTPS, não necessariamente um segundo transporte; não contornar autenticação, certificado, firmware ou controles privados.
4. Preparar mantém GUI Orca; ao selecionar um equipamento registrado, deve ativar implementação OEM compatível **efetivamente conectada ao pipeline**, com capacidades, macros, perfis e código livre reutilizável conforme licença por arquivo. Seleção de um ID ou preset JSON, isoladamente, não cumpre a regra. Se recurso incompatível/fechado, declarar indisponibilidade ou fallback identificado, sem equivalência presumida.
5. Imprimir oferece somente máquinas registradas; offline permanece rotulada offline. Enviar apenas com impressão autorizada, integridade de arquivo e ACK real. Nenhum comando de impressão automática na descoberta.
6. Atualizar módulos/firmware-matrix/perfis independentemente quando tecnicamente possível, via manifesto assinado, SHA, versão, ABI, rollback e reinício seguro quando necessário; motor-base Orca permanece acessível offline sem assinatura.

**Primeiro piloto:** Bambu Lab A1. **Próximos:** Flashforge AD5X, Anycubic Kobra 3 V2 (não confundir com nome informal Kobra V3 sem confirmação de identidade) e Elegoo Centauri Carbon 2. A política também vale às demais marcas e novos dispositivos.

**Estado de prova:** apenas seleção candidato por modelo/ABI em C++ e emuladores parciais; nenhum módulo OEM completo comprovado no pipeline Preparar. Wine CLI não valida GUI/firmware/hardware. Não declarar gates G2-G6 PASS nem release; preservar evidências e testes negativos.


## Decisão vinculante adicional — atualizações diferenciais OEM/Orca (06/11/2026)
Por ordem do proprietário, a política válida para **todos os fabricantes e modelos** é [REGRA-MESTRE-ATUALIZACAO-INCREMENTAL-OEM-ORCA-2026-11-06.md](REGRA-MESTRE-ATUALIZACAO-INCREMENTAL-OEM-ORCA-2026-11-06.md). Em atualizações Bambu Studio, Orca ou outros OEM, comparar SHA/versão/licença por arquivo, reutilizar apenas diferenças necessárias, compilar, testar a ligação real com Preparar, G-code oficial inalterado, rede simulada, Windows e hardware. Sem promover módulo não testado e sem update ou merge automático. Primeiro piloto A1: cinco macros já idênticos, port C++ do candidato matemático testado isoladamente, integração ao pipeline **NOT RUN**, G-code oficial×Filuna **MISMATCH**. A atualização de fontes é um alerta para revisão, nunca aprovação.


## Regra permanente — Snapmaker U1: G-code imutável e validação de segurança geométrica (07/11/2026)

**Origem:** ordem expressa posterior do proprietário, com prioridade sobre os guards e tentativas de alterar trajetórias de G-code anteriormente propostas no PR #6. Aplica-se aos módulos OEM e ao futuro DeviceBridge do motor único Filuna.

1. **Nunca inserir, remover ou reescrever G0/G1/G2/G3, retração, Z-hop, ferramenta ou macro para “corrigir” o G-code do fabricante.** Perfis e scripts oficiais permanecem imutáveis. A paridade Filuna × OEM é um teste independente: se só o Filuna criar um risco/um caminho divergente, interromper envio e corrigir o motor/perfil, não atribuir culpa ao fabricante.
2. **Validação em leitura antes de enviar, sem modificar bytes do arquivo:** construir mapa de ocupação XY/Z pelas extrusões com perfil de altura e margem; acompanhar G90/G91, M82/M83, G92, homing incerto e arcos G2/G3; verificar travessias não extrusoras sobre peça já depositada; tratar purga final e trocas de ferramenta como regiões críticas, separar torre de purga da peça, e conferir limites de 270×270×270 mm para os movimentos explícitos da U1. Z-hop espiral que ultrapasse limite => WARN e recomendar Normal, sem alterar G-code.
3. **Prova de macro, nunca suposição:** extrair `change_filament_gcode`, `machine_end_gcode` e comandos de troca do portátil versionado; cada macro liberada terá nome, fonte exata, hash, trecho, condições e escopo em `FILUNA/fabricantes/snapmaker/vendor.yaml`. `PRINT_END` levanta Z para estacionamento em condições documentadas, mas **não protege retroativamente a purga final**. Macro desconhecida não afasta risco. Para avaliar macro específica instalada no cliente, consultar `/printer/objects/query?configfile` do Moonraker autorizado ou exportação Fluidd antes de declarar prova em dispositivo.
4. **Decisões separadas:** ausência de risco no escopo verificado => PASS; espiral fora do volume => WARN; colisão no mesmo G-code oficial pinado => BLOCK, indicar linha/camada/XYZ e permitir exclusivamente aceitação consciente e explícita, vinculada ao hash do arquivo e usuário, com registro auditável. Quando G-code Filuna difere do original, `ENGINE_DIVERGENCE` continua bloqueado até correção/paridade; **não é override de risco OEM**.
5. **Evidência não equivale a produto pronto:** os scripts de laboratório `lab/snapmaker_spatial_gcode_validator.py` e testes no Actions **não estão conectados ao fluxo nativo Preparar/Dispositivos/Enviar**; não são aprovação de ABI, firmware/DeviceBridge, compilação do app nem impressão. O próprio G-code oficial Snapmaker Orca 2.4.1 bicolor apresenta uma travessia insegura no final da purga (linha 38911), então **não liberar como se houvesse Z-hop universal**. Outro fabricante requer geometria/volume e caminhos de estacionamento oficiais próprios; nunca aplicar whitelist de risco global.
6. **Gates:** G4-I continua BLOCKED enquanto validador do aplicativo, perfil/versão original, paridade do binário e contratos de dispositivos não forem testados com evidência. G4-F fica somente para beta físico após autorização expressa. Sem merge, build final, deploy, envio ou release a partir de QA.

**Prova e código:** [SNAPMAKER-U1-GCODE-IMUTAVEL-VALIDADOR-GEOMETRICO-20261107.md](qa/SNAPMAKER-U1-GCODE-IMUTAVEL-VALIDADOR-GEOMETRICO-20261107.md), [CI #42344893252](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42344893252), PR #6 DRAFT.
