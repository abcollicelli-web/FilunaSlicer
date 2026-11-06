# FILUNA — Evidência controlada do laboratório VPS/Wine e continuidade de QA

**ID:** FLN-QA-VPS-WINE-2026-11-06.1  
**Execução:** 2026-11-05T23:31:14 UTC, com registro em 06/11 (Europe/Rome).  
**Origem:** ordem do proprietário de continuar o novo Filuna Slicer por dependências, usando instância de testes no Ubuntu e o laboratório virtual fornecido, **sem equipamento Windows do proprietário disponível**.  
**Status:** testes de protocolo SIMULADO aprovados (14/14 dos aplicáveis), motor fatiou via Wine; **nenhum gate G3/G4 ou hardware real aprovado**.

## 1. Infraestrutura realmente utilizada

| Item | Estado/evidência |
|---|---|
| Servidor | Ubuntu conectado SentinelX. Site de produção em \`/srv/site\` **não foi implantado nem alterado** |
| Wine | Wine 10 64-bit instalado na VPS; prefixo dedicado \`/var/tmp/filuna-qa-wine/prefix\` (nenhum perfil do cliente tocado) |
| Binário | \`orca-slicer.exe\` e \`OrcaSlicer.dll\` do artifact portátil GitHub Actions Windows run [42102469888](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42102469888). **Binário de teste, não fatiador de produção** |
| Lab original | \`filuna-laboratorio-impressoras-virtuais.zip\`, SHA256 \`474cfec12e40c0e47321c9c15ee19064c14371c5c8fa68fc21c0b896431d1197\`; código original versionado em [qa/virtual-lab](qa/virtual-lab/) |
| Rede | Bridge \`br-filuna-lab\`, \`10.77.0.0/24\`, quatro namespaces isolados (\`.11\` CC2, \`.12\` Bambu, \`.13\` Moonraker, \`.14\` SDCP), **sem NAT, rota default da VPS não alterada, nenhuma porta pública** |
| Regressão CI | [GitHub Actions run 42130113571](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42130113571) = SUCCESS; workflow \`filuna-virtual-lan-regression.yml\` cria laboratório e limpa no final |
| Matriz LAN VPS | [results-2026-11-05-vps.json](qa/virtual-lab/results-2026-11-05-vps.json); \`network_regression.py\` executado no servidor na cópia do repo, **14 PASS, 0 FAIL, 6 NOT_RUN** |
| G-code Wine | \`plate_1.gcode\`, **275.984 bytes**, cubo \`20mm_cube.obj\`, perfil Bambu P1S 0.4 via arquivos JSON reais. Executável retornou exit code **0** |
| Compatibilidade Windows | **NOT RUN** — Wine não é Windows nativo, não testa instalação, shell Filuna, DeviceBridge nem firmware |
| Hardware | **NOT RUN** — não foi conectada impressora física nem emitidos comandos físicos |

**A execução no Wine tem escopo CLI**: houve saída G-code real, mas **não** comparação contra G-code upstream na mesma JVM/Wine com fixtures equivalentes e não verificou GUI. O comparador Windows de referência tem prova independente na PR #3, mas **não** atribuir seu PASS ao teste Wine.

## 2. Matriz exata de resultados por dispositivo

| Dispositivo virtual | Detectado/identificado | Conectado/autenticado | Monitorado | Comandos | Transferência | Estado |
|---|---|---|---|---|---|---|
| Elegoo CC2 \`10.77.0.11\` | **PASS mock** UDP 52700 | **PASS mock** MQTT 1883, senha válida | **PASS mock** status MQTT | **PASS mock** start/pause/resume/cancel | **PASS mock** HTTP upload | Sem firmware real |
| Elegoo Carbon original \`10.77.0.14\` | **PASS mock** SDCP UDP 3000 | **NOT RUN** WS3030 | NOT RUN | NOT RUN | NOT RUN | Apenas descoberta |
| Bambu A1 \`10.77.0.12\` | **PASS mock** SSDP UDP 1990/2021 | **NOT RUN** MQTT/TLS 8883 | NOT RUN | NOT RUN | **NOT RUN** FTPS 990 | Apenas descoberta |
| Moonraker \`10.77.0.13\` | **PASS mock** identificação em IP conhecido | **PASS mock** HTTP 7125 | **PASS mock** consulta status | **PASS mock** start/pause/resume/cancel | **PASS mock** upload | WS7125 e autodescoberta não testados |

Testes negativos reais *contra os próprios emuladores*: MQTT CC2 rejeitou acesso incorreto (CONNACK 4); endpoints HTTP desconhecidos retornaram 404. Comandos aplicados **somente nos emuladores** sob IPs privados fixados; jamais foram enviados à rede externa. Estes resultados **não testam o adaptador real do Filuna**.

**Seis NOT RUN na matriz completa do CI:** Bambu MQTT/TLS 8883; Bambu FTPS 990; SDCP WebSocket 3030; Moonraker WebSocket 7125; integração real DeviceBridge com Filuna em Wine; firmware e impressora física.

## 3. Correções de qualidade feitas na branch separada

| Controle | Antes | Agora | Limitação honesta |
|---|---|---|---|
| \`evidence_schema.py\` / JSON schema | hardware \`firmware=TBD\` e URL fictícia podiam satisfazer validação sintática | G4 exige modelo/fabricante, firmware não-placeholder, fingerprint SHA-256, horário UTC e URL canônica Actions | Ainda exige checagem externa e humano para prova física |
| \`gate_governance.py\` | verificador imprimia BLOCKED mas sempre \`exit 0\` | modos \`--mode report\`, \`--mode assert-blocked\`, \`--mode enforce\`; enforce retorna !=0 quando sem evidência e sem escopo completo | Enforce é fail-closed, precisa de processo formal de aprovação/dossiê para futura promoção legítima |
| \`required-cases.json\` | seis casos fundacionais podiam sugerir cobertura total | \`scope=FUND-ONDA01-SUBGATES\` + \`global_promotion_allowed=false\` | Requisitos globais CORE/Windows precisam ampliação |
| \`check_master_plan.py\` | proibia qualquer PASS no grafo, impedindo progresso real | permite PASS somente se predecessores PASS e dossiê de aprovação por nó (URL, signoff) | Revisão de autenticidade dos dossiês ainda é gate separado |
| CI fundação | testes de contrato, mas nenhum assert explícito do bloqueio geral | workflow exige \`gate_governance.py --mode assert-blocked\` | Este é teste NEGATIVO, **não** liberação |
| CI LAN | laboratório criado fora do repo e sem regressão | lab versionado; GitHub Actions simula 4 impressoras em namespaces, 14 PASS/6 NOT RUN, publica artefato | Simuladores não testam GUI/firmware |
| Site antigo | sem módulo QA deste fork | rota privada \`/admin/qa-filuna-slicer\` preparada em **branch de site separada**, snapshot com data, papéis Admin | **NÃO PUBLICADA**; precisa build, revisão e deploy canônico |

## 4. Estado dos gates depois desta etapa

| Gate | Status | Justificativa |
|---|---|---|
| G0 | EM ANDAMENTO/PASS documental com escopo restrito conforme registro Grok | Leitura integral e conflitos ainda pendentes |
| G1 | Parcial (contratos existentes) | Visual e assets reais não homologados |
| G2 | **BLOCKED** | provas de código fora das áreas testadas insuficientes |
| G3 | **BLOCKED** | 1 corpus pequeno + emulador isolado não prova regressão completa, rede/DeviceBridge não testados |
| G4 | **NOT RUN/BLOCKED** | Windows nativo e impressoras reais ausentes |
| G5/G6 | **BLOCKED** | assinatura, pacote, rollback, licença, segurança e owner signoff |
| Impressão remota | **DESLIGADA** | depende de LAN física e autorização do cliente |

## 5. Próximo passo executável (sem pular dependências)

1. [ ] Corrigir verificador de evidências e concluir CI do branch QA, auditar diff, anexar PR rascunho para revisão Grok; sem merge automático.
2. [ ] Endurecer laboratório: adicionar MQTT/TLS com certificado de laboratório e FTPS Bambu, SDCP WebSocket original e Moonraker WS; modelar autenticação inválida, perda de conexão e transferência parcial.
3. [ ] Fazer o **Filuna DeviceBridge realmente consumir** discovery e status dos emuladores sob Wine; registrar logs/ACKs do app, não apenas dos sockets Python.
4. [ ] Comparar G-code Wine com Orca pinado e acrescentar STL/3MF/STEP/perfis sem ignorar failures.
5. [ ] Usar hardware real de CC2/A1/Anycubic/10 marcas na matriz G4, com versão firmware e autorização; sem hardware, **manter BLOCKED**.
6. [ ] Compilar e homologar a página de QA no site privado, testar a rota protegida e só depois deploy **exclusivamente** por \`/srv/site/deploy/atualizar.sh\` com backup e aprovação.

## 6. Continuidade multi-IA

Ler em ordem: \`FILUNA/REGISTRO-CONTINUIDADE-ETAPAS.md\` **no HEAD da PR #3**, \`REGRA-MESTRE-QUALIDADE.md\`, este documento, \`CRONOGRAMA-DEPENDENCIAS.json\`, o resultado JSON + run 42130113571 e a PR de QA correspondente. **Não duplicar registro mestre.** Cada IA deve verificar HEAD SHA atualizado antes de escrever e anexar novo checkpoint com testes PASS/FAIL/NOT RUN, links e dependências.

**Conclusão:** há prova real de fatiamento CLI no Wine, emuladores funcionais e regressão CI dos protocolos mock. Não existe evidência que autorize declarar o Filuna conectado às quatro impressoras, controle remoto, câmera, multimaterial ou impressão real homologados.
