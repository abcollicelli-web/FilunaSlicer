# FILUNA SLICER + CHATGPT
## Documento mestre de acessos, comandos, fatiamento e atendimento

**Versão:** proposta de homologação 0.1 — 19/11/2026  
**Situação:** implementação inicial isolada no GitHub; **não publicada nem conectada ao site comercial**  
**Proteção:** alteração no site, APIs, banco, autenticação, nginx ou interface SOMENTE após revisão e aprovação do proprietário.

---

## 1. Objetivo do Assistente Filuna

O plugin transforma o ChatGPT em um **assistente de atendimento Filuna**, externo ao site e ao programa Windows, que conversa naturalmente com cada cliente. Não é apenas uma lista de comandos: ele deve ensinar, explicar, buscar ajuda na documentação oficial Filuna, consultar dados da conta vinculada, investigar erros reais, propor soluções, preparar tarefas 3D e acompanhar chamados. O assistente é identificado como IA; não deve fingir ser humano.

**Exemplos:** “Quando vence minha fatura?”, “Quantos modelos tenho?”, “Minha K2 Plus está imprimindo?”, “Por que minha impressora desconectou?”, “Faça um chaveiro com 60 mm e guarde no Filuna”, “Anexei um 3MF; quero imprimir, use a configuração mais segura”.

**Fluxo de atendimento:** acolher pedido → identificar objetivo → usar documentação e consultas reais → diagnosticar → recomendar → pedir confirmação nas ações de risco → executar via Filuna → conferir recibo → acompanhar → encaminhar ao atendimento humano quando necessário.

## 2. Regras obrigatórias para todos os clientes

1. **Dados por titular:** autenticação OAuth da conta Filuna; nenhuma consulta usa conta de administrador; cada ferramenta revalida usuário, tenant, escopo, plano e papel no backend. A conexão ChatGPT sozinha não concede acesso a outros clientes ou à empresa de terceiros.
2. **Proibido entregar arquivos:** não devolver ao GPT arquivos STL, 3MF, G-code, OBJ, ZIP, SCAD, STEP, código de peça, geometria, anexo, bytes, base64 ou URL de download de arquivo. Modelos e resultados devem permanecer na biblioteca Filuna, com versão/revisão e apenas recibos/IDs no chat.
3. **Arquivos recebidos:** cliente PODE anexar STL/3MF ao chat para uso pelo Filuna, com autorização expressa; o trânsito é temporário e protegido, sem arquivo disponibilizado publicamente ou enviado ao próprio chat como resposta.
4. **Confirmação física:** criar projeto, fatiar e imprimir são ações separadas. A impressão só inicia após autorização do cliente e confirmação no aplicativo Windows, além de validação de máquina online, mesa limpa, material, risco e integridade do G-code.
5. **Prioridade à configuração oficial:** identificar máquina, filamentos, firmware, mesa PEI, AMS/CFS/ACE/IFS/MMU, perfil OEM e motor Orca. Recomendações devem respeitar as configurações efetivamente homologadas de cada fabricante; nunca alterar silenciosamente medidas, cores e presets de 3MF.
6. **Plano e licenças:** respeitar o plano comercial para ferramentas Filuna; jamais bloquear funções livres do OrcaSlicer por atraso de assinatura; preservar licenças de modelos de terceiros.
7. **Precisão:** só dizer que salvou, imprimiu, confirmou fatura ou resolveu erro depois de obter recibo real do Hub ou do dispositivo; se serviço offline, comunicar indisponibilidade.
8. **Privacidade:** nenhum segredo, IP interno, token, senhas, chave LAN, credenciais de nuvem ou dados bancários completos. Recusa para cross-tenant, tentativas de exportar geometria e comandos arbitrários.
9. **Site intocado:** esta branch contém contratos do plugin, testes, ícone, atalhos e documentação. Todo endpoint novo listado aqui é uma proposta — não está implantado. Antes de desenvolver qualquer alteração do site, apresentar diff e solicitar aprovação.

## 3. O que foi confirmado por leitura do projeto

- No servidor conectado SentinelX, o snapshot de release inspecionado foi `/srv/filuna-v2/releases/20261119T175145Z-165a18d`. Isso é uma fotografia em 19/11/2026, não um estado imutável.
- O site v2 no GitHub `abcollicelli-web/filuna-site` e o código atual possuem módulos de `assinaturas.py`, `db.py`, `farm.py`, `app_android.py`, `modelos3d.py`, `dispositivos.py`, `central.py`, `assistencia.py`, `ajuda_ia.py`, `comercio.py` e `ofertas.py`.
- O banco define usuários/planos/assinaturas, bobinas, impressoras, trabalhos, pedidos, orçamentos, cobranças, modelos salvos, suporte, backup e sincronização. `assinaturas.py` registra `vence_em` e integra provedores de cobrança.
- A API `/api/slicer/v1` já cobre conta, resumo, impressoras, estoque, fila, comandos, câmeras, ofertas e calibrações. **Isso não libera acesso direto pelo ChatGPT:** faltam endpoints de escopo limitado para o plugin.
- Existe histórico de MCP proprietário beta/estoque fictício. Ele não prova operação comercial e não substitui a nova autenticação multiusuário.
- A imagem base reutiliza o favicon Filuna existente e a paleta oficial. Nenhuma logo nova precisa ser inventada. Uma versão PNG final e validação do pacote ainda poderão ser necessárias.

## 4. Matriz completa do que o GPT lerá e escreverá

| Área | O assistente poderá LER/RESPONDER | O assistente poderá ESCREVER/ACIONAR |
|---|---|---|
| Conta e perfil | Plano, nome de exibição, titular, versão e recursos habilitados | Consultar perfil; criar chamado e ajustar preferências de baixo risco |
| Assinatura | Plano, estado, próxima cobrança (vence_em), valor, provedor, cancelamento programado | Sem pagamento, troca de cartão ou cancelamento pelo GPT; direcionar ao fluxo seguro |
| Faturas e cobrança | Fatura por número, valor, moeda, vencimento, comprovante disponível no site, situação | Consulta de metadados; sem retornar PDF nem dados completos de cartão |
| Biblioteca e modelos | Quantidade de arquivos, origens, projetos, revisões, permissões/licenças e últimos modelos | Guardar catálogo, gerar peças, corrigir modelo, renomear após confirmação |
| Armazenamento | Espaço usado, total, livre, limites de plano e sincronização | Sem listar conteúdo de arquivos ou baixar geometria |
| STL e 3MF enviados | Inspecionar anexos da própria conversa e geometria processada pelo Windows | Importar por fileParams, armazenar localmente na biblioteca, sem arquivo na saída |
| Fatiamento | Perfil oficial OEM, máquina, material, multicor, mesas, suportes, primeira camada, tempo, gramas e alertas | Recomendar e executar fatiamento no Windows, após confirmação, preservando presets válidos |
| Impressão | Máquina online, estado, progresso, fila, tempo, material e erros | Preparar; confirmar fisicamente no aplicativo; pausar/cancelar com confirmação e salvaguardas |
| Impressoras e manutenção | Fabricante, modelo, diagnósticos, códigos de falha, horas e calibrações | Diagnóstico guiado, orientações e abertura de suporte |
| Estoque de bobinas | Bobina física, identificação, tipo, cor, massa, consumo, saldo e movimentos | Ajustar estoque com motivo e confirmação, sem mistura de bobinas iguais |
| Nota fiscal | Documento autorizado, número, emissor, itens, unidades, SKUs e valores estritamente necessários | Analisar primeiro, conferência/antiduplicação e depois confirmar gravação |
| Farm e produção | Trabalhos em curso, concluídos, falhas, filas, manutenção, consumo e desperdício | Ações de impressão e produção apenas sob dispositivo válido |
| Pedidos e orçamentos | Pedidos, prazos, estados, orçamentos e valores da empresa autorizada | Criar orçamento ou alterar status após confirmação |
| Clientes e Ateliê | Clientes e indicadores do papel empresarial do usuário | Não exibir dados da base de outros clientes sem autorização de papel e empresa |
| Financeiro e loja | Resumo de receita, margem, despesas, vendas, cobranças e pedidos de e-commerce | Nenhum comando de transferência, pagamento ou venda sem fluxo homologado |
| Ofertas e catálogo | Material, preço/kg, fabricante e ofertas de produto físico; catálogos e paramétricos disponíveis | Somente link de produto físico; evitar links de arquivos 3D |
| Dispositivos e integrações | Conexões, canais habilitados, versão do aplicativo, status de sincronização | Não permitir ver senhas, token, endereço privado, cofre ou acesso admin |
| Backup e sincronização | Última sincronização, disponibilidade, falha e espaço ocupado | Não apagar backup nem restaurar automaticamente |
| Suporte e assistência | Artigos oficiais, tutoriais, sintomas, chamados, histórico e atendimento humano | Responder e abrir chamados autorizados, transferir ao humano quando necessário |
| Alertas e notificações | Avisos de falha, atividades, mudanças de status e atualizações | Marcar alertas lidos mediante autorização |
| Ajuda técnica geral | Instalação, configuração, materiais, perfil de impressora e boas práticas | Responder sem acessar dados privados; consultar documentação oficial |

**Leitura:** somente metadados e informações autorizadas do titular. **Escrita:** apenas operações explícitas e validadas com recibo real. A tabela é o destino funcional, não garantia de implementação final.

## 5. Fluxo completo de arquivo STL/3MF → fatiamento → impressão

**Etapa A — Anexo enviado no ChatGPT.** O cliente anexa um arquivo STL ou 3MF. O ChatGPT envia à ferramenta `filuna_arquivo_importar` pelo recurso oficial `_meta["openai/fileParams"]` (parâmetros: `download_url`, `file_id`, opcionalmente `file_name` e `mime_type`). Essa URL é temporária; o gateway valida procedência HTTPS/domínio, identificador e extensão, e o Hub precisa verificar limites, formato real, assinatura, malha e compactação maliciosa. Nenhuma URL de download sai como resultado no chat.

**Etapa B — Vincular fatiador e biblioteca.** O Hub identifica a conta e o dispositivo Windows associado, envia o trabalho ao aplicativo autenticado e atribui um ID privado. O arquivo é mantido no dispositivo e registrado na biblioteca Filuna. Se offline ou não autorizado, não finge salvamento.

**Etapa C — Inspecionar o projeto.** Ler formato, dimensões, número de objetos, placas, material, cores, mapeamento de extrusoras, presets existentes, suportes, altura Z, tolerâncias e alertas de geometria sem entregar malha/G-code ao chat.

**Etapa D — Recomendar configurações.** O Assistente Filuna consulta impressora e bobinas reais, verifica perfis OEM homologados e sugere camada, paredes, preenchimento, orientação, suportes, temperaturas, velocidade e tempo, considerando qualidade/velocidade/resistência. Sem confirmação, só recomenda; não sobrescreve configurações válidas do 3MF.

**Etapa E — Fatiar pelo programa Windows.** Após aprovação do cliente, `filuna_fatiamento_executar` dispara o motor local; recebe ID/status. `filuna_fatiamento_resultado` retorna estimativas de tempo, consumo, purga e avisos, além de falhas reais. O arquivo de impressão permanece apenas no Filuna.

**Etapa F — Botão “Imprimir” e confirmação.** O chat poderá exibir um controle de confirmação integrado à interface do plugin para preparar/imprimir. O comando nunca acessa diretamente a LAN nem começa impressão só porque o usuário escreveu “sim”: o programa Windows valida autorização local, disponibilidade, mesa, material e integridade do job; somente após isso o conector envia ao equipamento. O acompanhamento retorna estado/progresso real.

**Etapa G — Acompanhamento.** “Quanto falta?”, “Está imprimindo?”, “Por que parou?”, “Qual material gastou?” consultam o Hub; comandos de pausar/cancelar exigem confirmação e controles da impressora.

**Critérios de aceite específicos:** STL simples, 3MF multicor e multiplaca, perfil oficial vs Filuna, G-code comparado após normalização aprovada, troca de fabricante, arquivo corrompido/ZIP bomb, acima do limite, Windows offline, máquina desconectada, token revogado, tenant cruzado, sem estoque, reprodução após falha, duplicação de comando, retorno sem URLs, botão imprimir sem autorização física, cancelamento/pausa idempotentes.

## 6. Assistente de ajuda e atendimento humano

O plugin terá acesso ao **conteúdo público de ajuda e à documentação própria do Filuna**; sobre dúvidas particulares, consultará dados autorizados da conta. Deverá ensinar instalação, login, troca de impressora, configuração de filamentos, perfil por fabricante, resolução de falhas Orca e 3MF, importação, paramétricos, custos, atualização, assinatura e estoque.

Para erro de impressora, usar `filuna_impressoras_listar`, `filuna_impressora_erros`, `filuna_diagnostico_guiado` e `filuna_ajuda_pesquisar`. Deve diferenciar hipótese de diagnóstico confirmado, explicar o que verificar, não executar comandos perigosos e confirmar após cada tentativa.

Para problemas de conta/cobrança, usar `filuna_assinatura_consultar` e `filuna_faturas_listar`. Não concluir que houve falta de pagamento sem resposta atual do provedor.

Se não resolver, oferecer abrir chamado, acompanhar protocolo, responder e solicitar atendimento humano. Nunca afirmar que um humano já respondeu antes de receber mensagem real. O cliente mantém histórico/estado do chamado no Filuna, dentro das regras de privacidade.

**Tom:** português natural e acessível, técnico quando necessário, cordial e objetivo; sem inventar funções, sem pedir senha ou token, sem colocar a culpa no cliente; admitir limitações e oferecer encaminhamento quando necessário.

## 7. Inventário do MCP: 63 contratos de ferramenta

**Legenda:** `GET` = consulta; `POST` = criação, diagnóstico ou ação que pode alterar estado. Os caminhos `/plugin/v1/` abaixo são **propostos**, não implementados na instalação atual.

| Ferramenta | Método | Scope exigido | Contrato da API no Hub |
|---|---|---|---|
| `filuna_ajuda_artigo` | GET | `filuna.help.read` | `/plugin/v1/ajuda/artigo` |
| `filuna_ajuda_atendimento_historico` | GET | `filuna.account.read` | `/plugin/v1/ajuda/atendimento/historico` |
| `filuna_ajuda_capacidades` | GET | `filuna.help.read` | `/plugin/v1/ajuda/capacidades` |
| `filuna_ajuda_pesquisar` | GET | `filuna.help.read` | `/plugin/v1/ajuda/pesquisar` |
| `filuna_alerta_marcar_lido` | POST | `filuna.account.write` | `/plugin/v1/alertas/lido` |
| `filuna_alertas_listar` | GET | `filuna.account.read` | `/plugin/v1/alertas` |
| `filuna_armazenamento_consultar` | GET | `filuna.library.read` | `/plugin/v1/armazenamento` |
| `filuna_arquivo_importar` | POST | `filuna.library.write` | `/plugin/v1/arquivos/importar-chatgpt` |
| `filuna_arquivo_inspecionar` | GET | `filuna.create` | `/plugin/v1/arquivos/inspecionar` |
| `filuna_arquivos_estatisticas` | GET | `filuna.library.read` | `/plugin/v1/arquivos/estatisticas` |
| `filuna_assinatura_consultar` | GET | `filuna.billing.read` | `/plugin/v1/assinatura` |
| `filuna_backup_status` | GET | `filuna.account.read` | `/plugin/v1/backup/estado` |
| `filuna_biblioteca_guardar` | POST | `filuna.library.write` | `/plugin/v1/biblioteca/guardar` |
| `filuna_biblioteca_listar` | GET | `filuna.library.read` | `/plugin/v1/biblioteca` |
| `filuna_calibracoes_listar` | GET | `filuna.printer.read` | `/plugin/v1/calibracoes` |
| `filuna_catalogo_pesquisar` | GET | `filuna.catalog.read` | `/plugin/v1/catalogo` |
| `filuna_clientes_listar` | GET | `filuna.business.read` | `/plugin/v1/clientes` |
| `filuna_consumo_materiais` | GET | `filuna.stock.read` | `/plugin/v1/estoque/consumo` |
| `filuna_conta_resumo` | GET | `filuna.read` | `/plugin/v1/conta` |
| `filuna_diagnostico_guiado` | POST | `filuna.help.read` | `/plugin/v1/ajuda/diagnostico` |
| `filuna_dispositivos_listar` | GET | `filuna.account.read` | `/plugin/v1/dispositivos` |
| `filuna_estoque_bobina_ajustar` | POST | `filuna.stock.write` | `/plugin/v1/estoque/bobinas/ajustar` |
| `filuna_estoque_listar` | GET | `filuna.stock.read` | `/plugin/v1/estoque` |
| `filuna_estoque_movimentacoes` | GET | `filuna.stock.read` | `/plugin/v1/estoque/movimentacoes` |
| `filuna_fatiamento_executar` | POST | `filuna.print` | `/plugin/v1/fatiamento/executar` |
| `filuna_fatiamento_recomendar` | POST | `filuna.print` | `/plugin/v1/fatiamento/recomendar` |
| `filuna_fatiamento_resultado` | GET | `filuna.printer.read` | `/plugin/v1/fatiamento/resultado` |
| `filuna_fatura_consultar` | GET | `filuna.billing.read` | `/plugin/v1/faturas/detalhe` |
| `filuna_faturas_listar` | GET | `filuna.billing.read` | `/plugin/v1/faturas` |
| `filuna_ferramenta_executar` | POST | `filuna.create` | `/plugin/v1/ferramentas/executar` |
| `filuna_financeiro_resumo` | GET | `filuna.business.read` | `/plugin/v1/financeiro/resumo` |
| `filuna_historico_impressoes` | GET | `filuna.production.read` | `/plugin/v1/impressao/historico` |
| `filuna_impressao_cancelar` | POST | `filuna.print` | `/plugin/v1/impressao/cancelar` |
| `filuna_impressao_confirmar` | POST | `filuna.print` | `/plugin/v1/impressao/confirmar` |
| `filuna_impressao_pausar` | POST | `filuna.print` | `/plugin/v1/impressao/pausar` |
| `filuna_impressao_preparar` | POST | `filuna.print` | `/plugin/v1/impressao/preparar` |
| `filuna_impressao_status` | GET | `filuna.printer.read` | `/plugin/v1/impressao/status` |
| `filuna_impressora_erros` | GET | `filuna.printer.read` | `/plugin/v1/impressoras/erros` |
| `filuna_impressoras_listar` | GET | `filuna.printer.read` | `/plugin/v1/impressoras` |
| `filuna_integracoes_listar` | GET | `filuna.account.read` | `/plugin/v1/integracoes` |
| `filuna_licencas_consultar` | GET | `filuna.library.read` | `/plugin/v1/licencas` |
| `filuna_loja_resumo` | GET | `filuna.business.read` | `/plugin/v1/loja/resumo` |
| `filuna_manutencao_consultar` | GET | `filuna.printer.read` | `/plugin/v1/impressoras/manutencao` |
| `filuna_modelo_corrigir` | POST | `filuna.create` | `/plugin/v1/modelos/corrigir` |
| `filuna_nota_fiscal_analisar` | POST | `filuna.stock.write` | `/plugin/v1/notas/analisar` |
| `filuna_nota_fiscal_gravar` | POST | `filuna.stock.write` | `/plugin/v1/notas/gravar` |
| `filuna_ofertas_listar` | GET | `filuna.offers.read` | `/plugin/v1/ofertas` |
| `filuna_orcamento_criar` | POST | `filuna.business.write` | `/plugin/v1/orcamentos/criar` |
| `filuna_orcamentos_listar` | GET | `filuna.business.read` | `/plugin/v1/orcamentos` |
| `filuna_parametrico_gerar` | POST | `filuna.create` | `/plugin/v1/parametricos/gerar` |
| `filuna_pedido_consultar` | GET | `filuna.business.read` | `/plugin/v1/pedidos/detalhe` |
| `filuna_pedido_status_atualizar` | POST | `filuna.business.write` | `/plugin/v1/pedidos/estado` |
| `filuna_pedidos_listar` | GET | `filuna.business.read` | `/plugin/v1/pedidos` |
| `filuna_preferencias_consultar` | GET | `filuna.account.read` | `/plugin/v1/preferencias` |
| `filuna_producao_resumo` | GET | `filuna.production.read` | `/plugin/v1/producao/resumo` |
| `filuna_projeto_renomear` | POST | `filuna.library.write` | `/plugin/v1/projetos/renomear` |
| `filuna_projetos_listar` | GET | `filuna.library.read` | `/plugin/v1/projetos` |
| `filuna_relatorios_resumo` | GET | `filuna.business.read` | `/plugin/v1/relatorios` |
| `filuna_suporte_chamado_consultar` | GET | `filuna.account.read` | `/plugin/v1/suporte/chamado` |
| `filuna_suporte_chamado_criar` | POST | `filuna.account.write` | `/plugin/v1/suporte/chamados/criar` |
| `filuna_suporte_chamado_responder` | POST | `filuna.account.write` | `/plugin/v1/suporte/chamado/responder` |
| `filuna_suporte_chamados` | GET | `filuna.account.read` | `/plugin/v1/suporte/chamados` |
| `filuna_suporte_humano_solicitar` | POST | `filuna.account.write` | `/plugin/v1/suporte/transferir` |

## 8. Atalhos de perguntas e comandos

Arquivo versionado: `FILUNA/chatgpt_plugin/atalhos.json` com **26 sugestões**. Inclui “Minha conta”, “Faturas”, “Armazenamento”, “Biblioteca”, “Impressoras”, “Erros”, “Estoque”, “Nota fiscal”, “Criar paramétrico”, “Corrigir modelo”, “Atendimento”, “Anexar STL”, “Anexar 3MF”, “Melhor fatiamento”, “Fatiar modelo”, “Conferir antes de imprimir”, “Ofertas” e outros. Os atalhos são sugestões conversacionais, não botões que pulam autorização.

## 9. Permissões por área e confirmação

- `filuna.account.read`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.account.write`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.billing.read`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.business.read`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.business.write`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.catalog.read`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.create`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.help.read`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.library.read`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.library.write`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.offers.read`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.print`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.printer.read`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.production.read`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.read`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.stock.read`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.
- `filuna.stock.write`: habilitar somente após consentimento OAuth do cliente e verificação de papel/tenant pelo backend.

**Confirmação obrigatória** para: registro de nota fiscal, iniciar/pausar/cancelar impressão, fatiamento efetivo, mudanças de pedidos/estoque, criação de orçamentos, edição de biblioteca e envio de mensagens/chamados. Para iniciar impressão, confirmação GPT + autorização local são cumulativas.

**Bloqueios absolutos:** ler qualquer outra conta, acesso administrativo não concedido, executar comandos shell, baixar qualquer arquivo 3D para chat, expor endpoints internos ou identidades de dispositivos, revelar IP e credenciais, reiniciar serviço sem autorização, publicar anúncio/venda sem validação, ativar câmera sem permissão, iniciar impressão sem validação local.

## 10. Tecnologias e implementação criada na branch isolada

- `core.py`: operações fixas, campos permitidos na saída, validação de parâmetros, escopos, filtragem de resultados, bloqueio de arquivos e verificação de referência de anexo.
- `server.py`: servidor MCP para ChatGPT, verificador OAuth via introspecção do Hub, 63 ferramentas e declaração `openai/fileParams` para o anexo STL/3MF.
- `package/plugin.json`: identidade/descrição em português e casos de revisão; `package/mcp.json`: URL proposta de MCP. Rascunho, **não publicado**.
- `package/assets/filuna.svg`: ícone Filuna reutilizado.
- `atalhos.json`: comandos de abertura de atendimento e fatiamento.
- `tests/test_core.py`: regressões offline sem acesso ao site e controles de não vazamento.
- Workflow GitHub Actions isolado executa validações; não implanta nada.

**Status rigoroso:** código e contratos em branch de homologação ≠ ferramenta liberada para clientes. **O site em produção permanece intacto.**

## 11. O que está pendente e precisa de decisão/aprovação do proprietário

1. **Revisão desta matriz:** confirmar cada categoria que o GPT poderá ler, escrever, e quais devem ser bloqueadas por padrão.
2. **Backend:** aprovar em separado o diff com as rotas comerciais por tenant e permissões. Não aplicar alteração no site até aprovação.
3. **Identidade:** OAuth multiusuário/PKCE, consentimento por escopo, revogação, conta verificada, quotas, prevenção de abuso e auditoria. Sem login automático de proprietário.
4. **Biblioteca Windows:** finalizar armazenamento e referência persistente, versões, métricas de uso, isolamento de clientes e rotina de arquivos removidos.
5. **Relay de arquivos:** autorização temporária de anexo, proteção SSRF, limites, antivírus se aplicável, zip bomb, inspeção MIME/magic, exclusão de temporários e transferência para Windows sem servidor de arquivo público.
6. **Fatiamento nativo:** integração real com Orca/OEM, equivalência de perfis, testes de fabricantes e multicor, segurança de impressão.
7. **UI ChatGPT:** botão verdadeiro de revisão e confirmação, sem enviar G-code; ação física liberada somente com controle local.
8. **Notas fiscais:** parser comercial seguro para XML/PDF, anticadastramento duplicado, conferência de SKU/quantidade/unidade, trilha de auditoria e distinção de estoque fictício.
9. **Suporte:** conectar base oficial de conhecimento, diagnóstico, chamados, atendimento humano e histórico do cliente, com retenção e consentimentos definidos.
10. **Faturas:** verificar fonte da verdade do provedor, sincronização de pagamentos e campos mínimos seguros.
11. **Pacote:** exportar imagens PNG exigidas, conferir o slug interno da ficha antiga Filuna v1.0.0 antes de atualizar; o pacote atual é ficha nova experimental.
12. **Testes de homologação:** CI, Inspector MCP, conta ChatGPT real, 5 cenários positivos e 3 negativos, revisão da política OpenAI e testes de máquina física.
13. **Lançamento:** submissão e publicação oficialmente somente após validação e sua aprovação explícita.

## 12. Exemplos esperados de conversas

**Cliente:** “Quando vence minha mensalidade?”  
**Assistente Filuna:** “Vou consultar sua assinatura.” → usa `filuna_assinatura_consultar` → informa plano, vencimento e situação retornados pela API. Se indisponível: informa que não conseguiu confirmar, sem inventar data.

**Cliente:** “Minha impressora deu erro de filamento.”  
**Assistente Filuna:** consulta impressora/erros e orientação da base Filuna → pergunta apenas o necessário → propõe correção segura → verifica novamente → oferece abrir chamado.

**Cliente:** “Segue meu arquivo 3MF. Preciso imprimir na A1.”  
**Assistente Filuna:** recebe anexo → vincula ao Windows → confere 3MF, mesas, AMS e perfis → recomenda configuração → pede aceite → fatia localmente → mostra resumo e botão de confirmar impressão → aplicativo valida localmente → acompanha progresso. **Nenhum arquivo é devolvido no chat.**

**Cliente:** “Gere uma lua litofania com meus parâmetros.”  
**Assistente Filuna:** consulta as opções aprovadas, gera pelo Windows, guarda na biblioteca e informa ID/estado; não envia STL/3MF.

**Cliente:** “Preciso registrar esta nota fiscal no estoque.”  
**Assistente Filuna:** recebe referência já guardada no Filuna, analisa com parser seguro, apresenta produtos e quantidades, pede conferência, grava após confirmação e devolve recibo sem PDF.

**Cliente:** “Quero falar com alguém do suporte.”  
**Assistente Filuna:** pede consentimento para enviar o resumo do problema, cria chamado e devolve número/estado reais, sem simular conversa com humano.

## 13. Registro e controle de mudanças

Branch prevista: `filuna/chatgpt-plugin-library-only-20261119`, repositório `abcollicelli-web/FilunaSlicer`. Nenhum arquivo de `filuna-site` ou `/srv/filuna-v2` deve ser alterado ou implantado a partir desta etapa. A homologação do plugin é independente da compilação Windows e dos releases atuais.

**Conclusão para o proprietário:** a proposta é um suporte Filuna externo completo, com informação da conta e gestão 3D pelo ChatGPT; as ferramentas no código são uma base auditável, mas a liberação comercial depende de conectar com segurança os dados, o fatiador local e a confirmação de impressão, após revisão das mudanças no site.

## 14. Referências técnicas

- [OpenAI — MCP server](https://developers.openai.com/plugins/build/mcp-server)
- [OpenAI — referência fileParams](https://developers.openai.com/plugins/reference)
- [OpenAI — autenticação de plugins](https://developers.openai.com/plugins/build/auth)
- [OpenAI — submissão](https://developers.openai.com/plugins/deploy/submission)
- Repositórios GitHub Filuna já conectados e inspecionados; documentação histórica privada em `/srv/filuna-v2/src/Filuna-Slicer/`.
