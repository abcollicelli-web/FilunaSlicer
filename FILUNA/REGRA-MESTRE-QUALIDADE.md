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
| **G4 Windows e hardware** | Windows 10/11 x64 real; abrir/salvar projeto; recuperação; dispositivos por modelo e firmware; impressão e segurança | Logs/screenshot do executável instalado e testes físicos dos modelos abrangidos; dono aprova quando exigido |
| **G5 Pacote/canal** | Classe A/B/C/D, manifesto e arquivos verificados, assinatura, SHA256, rollback, notas e consentimento | Artefatos identificados por SHA, assinatura verificável, teste de atualização/retorno e aceite do proprietário |
| **G6 Liberação** | Não publicar versões públicas antes de todos os gates de release, sem quebrar acesso livre | Registro completo, aprovação nominal do dono para versão/SHA/canal; monitoramento e rollback pronto |

**Progressão:** G0 PASS → G1 PASS → G2 PASS → G3 PASS → G4 PASS → G5 PASS → G6 PASS. Um gate bloqueado permite apenas investigação, correção e testes do MESMO gate; não autoriza iniciar a etapa seguinte como pronta ou implantá-la. Builds técnicos privados são **candidatos de teste**, não releases aprovados. Para qualquer hardware obrigatório ainda não testado, o gate G4 permanece **BLOCKED**. A existência de artefato no GitHub não o aprova.

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
