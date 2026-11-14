# FILUNA — REGRA DE HOMOLOGAÇÃO EM FASES: TESTE INTERNO → BUILD FINAL → BETA VIGIADO → VALIDAÇÃO FÍSICA

**Decisão expressa do proprietário:** 07/11/2026. Válida para todos os fabricantes, módulos OEM e modelos do Filuna Slicer. Esta decisão complementa, sem substituir, as regras de segurança, código-fonte, contrato OEM, evidências e aprovação do proprietário.

## Atualização vinculante do proprietário — empacotamento leve e beta guiado (07/11/2026)

O plano mestre [PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md §13](PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md) §13 estabelece que o **build final** inclua apenas o motor-base compartilhado e os módulos/deltas OEM necessários, **sem empacotar as árvores completas de perfis de máquina, filamento e processo**. Esses perfis são descarregados individualmente sob demanda na primeira conexão de uma impressora em **Dispositivos**, com validação de origem, compatibilidade, cache local e funcionamento offline quando já configurada.

No **beta físico**, cada impressora recém-vinculada passa pelo assistente guiado: rede/portas/protocolo e fatiamento avaliados pelo sistema; câmera visualizada localmente e confirmada pelo cliente; módulos multicolor/slots detectados pelo sistema e filamentos confirmados pelo cliente. Todos os testes obrigatórios PASS → **"Impressora liberada para teste de impressão"**. Após uma execução física completa e validada → **"Impressora parcialmente homologada"** para a próxima unidade da mesma combinação; após **três execuções físicas independentes totalmente PASS**, sem incidentes críticos e com evidência real → **"Impressora homologada — liberada para impressão"** no beta. Cada nova unidade ainda faz seu teste local. Três retries, reconexões ou eventos duplicados não contam. Releases estáveis seguem aprovação expressa do proprietário.

A regra não antecipa fase física, build final, coleta real ou autorização de envio remoto antes da fase beta.

## 1. Fases obrigatórias e ordem

**Fase A — desenvolvimento e homologação técnica interna (AGORA).**

- Validar versões **portáteis oficiais** do fatiador de cada fabricante, com versão e SHA-256 fixados; **não compilar versões OEM para testes iniciais**.
- Investigar e corrigir CLI, presets/heranças, algoritmo, comparação real de G-code, cores/multimaterial, UI Preparar/Fatiar/Dispositivos, contrato de rede, atualizações e persistência.
- Rodar Windows real no GitHub Actions e, quando aplicável, Linux/Wine, simuladores e testes de contrato. Salvar logs, hashes, casos positivos/negativos, capturas e artefatos.
- Homologação **interna** por modelo/capacidade exige prova efetiva do binário Filuna, comparada ao oficial e ao simulador, não apenas testes do programa OEM nem compilação que passou.
- Marcar explicitamente `PASS_INTERNO`, `FALHA` e `PENDENTE_TECNICO`. Não declarar capacidades de conexão, câmera ou impressão como confirmadas sem evidência correspondente.
- A falta de impressora física **não impede a conclusão de gates internos que já possam ser comprovados sem hardware**; não apresentar simulação como teste físico.

**Fase B — compilação final do Filuna (DEPOIS de todos os gates internos exigidos).**

- Somente após concluir e comprovar os testes internos, produzir a compilação final do Filuna e seu pacote/atualização, com versão, origem, integridade, rollback e testes de instalação.
- Os testes iniciais continuam usando executáveis OEM portáteis; não publicar builds intermediários como versões para teste do proprietário.
- Reexecutar no binário final os testes internos e de segurança que dependem do produto empacotado. Regressões bloqueiam a promoção para beta.

**Fase C — lançamento do beta vigiado (DEPOIS da validação interna e da compilação final).**

- Lançar **somente a versão beta identificada como beta**, com telemetria de falhas consentida, observabilidade, rollback e limites operacionais; não representar beta como homologação física concluída.
- A **validação física das impressoras ocorrerá aqui**, com hardware real, modelo/firmware corretos e logs de comunicação, envio e ACK, estado real, cancelamento e segurança.
- Testes físicos podem requerer acesso do proprietário/participantes autorizados; nenhum teste automatizado deve disparar impressão física sem comando explícito autorizado.
- Eventuais erros encontrados no beta são corrigidos, revalidados internamente e submetidos a nova versão beta. Homologação física por modelo/capacidade só é registrada após prova em hardware.

**Fase D — promoção posterior para a versão estável.**

- Somente após o beta vigiado, suas provas físicas e aprovação expressa do proprietário; não promover automaticamente a versão beta.

## 2. Única exceção intencional de agenda

A validação autenticada da **linha Bambu com Bambuddy** é a **única atividade propositalmente adiada**, conforme ordem anterior. Permanece `BAMBUDDY_AUTH=PENDENTE` até ordem posterior. Os demais testes Bambu independentes do Bambuddy e as outras marcas prosseguem agora. Impedimentos técnicos reais são tratados e registrados, não classificados como adiamentos voluntários.

## 2.1. Obrigação adicional da fase beta — ferramenta de monitoramento no servidor

**Nova determinação expressa do proprietário em 07/11/2026:** antes de abrir o **beta vigiado**, implementar e validar o monitoramento automático, no **servidor Filuna**, das impressoras cuja homologação física esteja pendente, conforme [REGRA-BETA-MONITORAMENTO-HOMOLOGACAO-FISICA-20261107.md](REGRA-BETA-MONITORAMENTO-HOMOLOGACAO-FISICA-20261107.md).

A instrumentação transversal ao app, Orca interno e módulos OEM deve acompanhar, sob observabilidade autorizada: **abertura do programa/arquivo, seleção de impressora, heranças de perfis, fatiamento, AMS e multimaterial, G-code gerado, G-code enviado e ACK, status real da impressão, conclusão, pausa/cancelamento e falhas**. O painel administrativo privado deve correlacionar por modelo, firmware, versão, módulo e trabalho, detectar inconsistências e preservar evidências para homologação modelo a modelo.

A comparação de G-code deve separar **gerado, enviado, confirmado pela impressora e executado (somente quando comprovável)**; não declarar o G-code executado como conhecido quando o firmware não fornece essa evidência. O sistema é **observador, não controlador**: jamais autoriza o administrador a imprimir, parar, aquecer ou acessar câmeras; usuários participam com escopos de privacidade consentidos e revogáveis.

**Fase atual:** requisito **documentado, ainda não implementado nem ativado**. Não alterar a sequência A→B→C→D, não fazer coleta real nesta fase, não antecipar testes físicos nem promover automaticamente resultados a homologação. Antes da abertura do beta, validar ingestão, privacidade, integridade, autorização, armazenamento, dashboard e casos negativos, além dos gates atuais.

## 3. Critério de comunicação

Separar sempre: `BINARIO_OEM_PORTATIL_VERIFICADO`, `A_B_OFICIAL_PASS`, `PARIDADE_FILUNA_PASS`, `REDE_SIMULADA_PASS`, `HOMOLOGACAO_INTERNA_PASS`, `BUILD_FINAL_PASS`, `BETA_VIGIADO`, `HOMOLOGACAO_FISICA_PASS` e `ESTAVEL_APROVADA`. Esses estados não são equivalentes e não podem ser inferidos um do outro.

**Decisor final de liberação:** proprietário. Nenhum merge, deploy, publicação, ativação de impressão real ou estabilidade assumida por conclusão do runner isolado.
