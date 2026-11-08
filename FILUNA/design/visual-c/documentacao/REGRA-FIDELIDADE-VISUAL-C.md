# FLN-VISUAL-C-100 — fidelidade visual obrigatória

**Versão documental:** FLN-VISUAL-C-100-20261108.1. **Autoridade:** declaração expressa do proprietário em 08/11/2026, 14:16 Europe/Rome, preservada literalmente em [ORDEM-FIDELIDADE-VISUAL-C-20261108.json](ORDEM-FIDELIDADE-VISUAL-C-20261108.json). **Natureza:** contrato documental vinculante de aparência e montagem; não é novo gate de produto nem autorização para retomar implementação.

## 1. Obrigação sem reinterpretação

Toda IA e toda pessoa que construir, integrar, corrigir ou revisar a UI Filuna DEVE produzir resultado **100% visualmente idêntico às imagens e regras do modelo aprovado**, com os recursos/estrutura correspondentes. “Inspirado”, aproximado, semelhante ou “modernizado de novo” não atende. É proibido reinterpretar, redesenhar ou substituir por layout genérico, padrão nativo do sistema ou padrão de framework.

Nenhuma IA pode mudar estética, estrutura, cores, tipografia, logo, pictogramas, posições, proporções, espaçamento, superfícies, hierarquia, estados ou recursos sem **NOVA aprovação EXPLÍCITA do proprietário**, registrada por versão, escopo e hashes. Não há autorização implícita por conveniência, limitação técnica, biblioteca, performance, responsividade ou acessibilidade. Se um requisito desses conflitar, bloquear o item e apresentar o conflito com solução candidata para aprovação, nunca reduzir a meta por conta própria.

O universo de rastreio é a [matriz](../MATRIZ-COBERTURA.md): 208 cenários e 123 pranchas. A obrigação aplica-se a todo esse universo e a qualquer futura UI; porém **não transforma propostas em modelos individualmente aprovados**. Apenas duas linhas da matriz correspondem às duas âncoras aprovadas. As demais 206 linhas continuam propostas (190) ou propostas com contrato detalhado pendente (16). Sem referência/estado homologado, o item fica **BLOCKED para implementação visual e aceite**, não é inventado por IA.

Preservar todas as funções e recursos dos contratos do projeto. Nenhuma remoção silenciosa é permitida para facilitar aparência. Segurança, AGPL, dados, motor Orca único e regras OEM continuam obrigatórios: em conflito, bloquear e registrar divergência. **Não maquiar aplicativo funcional com screenshot, vídeo ou render ilustrativo como fundo substituindo controles, viewport ou estado real.**

## 2. Referência versionada e precedência

Baseline visual imutável: commit `720291b8263e0898f7d6723bc904112ba2c6b3eb`. A aprovação anterior permanece íntegra em [APROVACAO-VISUAL-C.json](APROVACAO-VISUAL-C.json). O [contrato de montagem JSON](CONTRATO-MONTAGEM-VISUAL-C.json) fornece caminhos completos, SHA-256, dimensões extraídas, assets e mapeamento de cada cenário. [Guia de montagem](CONTRATO-MONTAGEM-VISUAL-C.md) reúne instruções operacionais.

As referências vivas continuam nos mesmos caminhos; antes de usá-las, comparar bytes com a versão autorizada. Uma diferença de hash exige nova aprovação registrada; não seguir silenciosamente o arquivo mais recente. O novo pedido torna a fidelidade obrigatória, mas não retroage para declarar aprovadas todas as derivadas. Este adendo supera somente interpretações anteriores de liberdade estética; preserva a história, os recursos e os bloqueios funcionais.

## 3. Montagem concreta obrigatória

- Construir shell, navegação, conteúdo, controles e viewport funcionais separados, sem achatar a aplicação em imagem. Ordem das abas: Início, Preparação, Visualização, Dispositivo, Projeto, Calibração.
- Reprodução da referência nas mesmas coordenadas/escala quando o ambiente de comparação for aprovado: SVG `viewBox="0 0 1920 1280"`, PNG 2880×1920, exportação geométrica 1,5×. Janela desenhada `(40,80,1840,1080)` e cabeçalho `(40,80,1840,76)`; unidades de viewBox, não DPI Windows presumido.
- Preparação: inspetor esquerdo `(40,157,360,929)`; área 3D `(400,157,1480,929)`; barra flutuante `(582,234,1116,66)`; rodapé `(40,1086,1840,74)`. **Não recriar o painel direito nº 4/informações do arquivo.** Preservar ampla área 3D, Global/Objetos visíveis, Avançado/Resistência/Girar como na âncora e todas as funções pertinentes.
- Ferramentas superiores: um pictograma por função, sem rótulos por cima do render. Reutilizar paths efetivos dos SVGs e `fontes/_base_C.py`: grade 32, traço 1,8, pontas/junções arredondadas. Nenhum emoji, glifo genérico ou símbolo proprietário copiado. Ligação funcional dos pictogramas deve respeitar os contratos; dúvida bloqueia, não autoriza trocar função.
- Dispositivo: câmera/vista dominante `(72,338,1188,665)` e instrumentos a partir de `(1312,362)`, com identidade, sessão e revisão/confirmação local; não substituir pelo dashboard genérico. Frame ilustrativo não é câmera funcional.
- Tokens existentes: porcelana `#F8F7F3`, grafite `#1B2024`, rosa `#A82F57`, seleção `#F2E1E7`, texto `#252A2E`, secundário `#646B72`, laranja `#E88759`, estado `#26735D`. Aplicar `tokens.json`; não aproximar cores por amostra JPEG.
- Inter nos pesos 400/500/600, usando `fontes/Inter-VariableFont.ttf` e licença `fontes/Inter-LICENSE.txt`, ambos identificados por SHA. Não substituir por fonte “parecida”. Logo original preservado nos SVGs e em `FILUNA/brand/assets/original/`; a imagem do logo na referência ocupa `(68,98,144,41)`, sem distorção ou redesenho. “Precision Atelier” não renomeia Filuna.
- PNGs em assets são recursos de estudo autorais, não fixtures 3D executáveis ou telemetria. SVGs são editáveis de referência, não componentes nativos já implementados. Não alegar que todo recurso runtime está pronto; respeitar os bloqueios do contrato de montagem.

## 4. Aceitação futura 100% auditável e fail-closed

**Estado atual: CONTRATO VISUAL DOCUMENTADO; APLICAÇÃO CONFORME = NOT RUN.** Nenhum teste do produto é executado por este adendo; nenhuma alegação de pixel-perfect real ou promoção G2/G3/G4/G5/G6.

Antes de executar o futuro aceite, fixar e obter aprovação do proprietário para: versão/SHA/binário, sistema Windows e build, tamanho físico da captura, viewport, DPI, escala do sistema e do app, monitor/renderizador, antialiasing, fonte/arquivo/pesos, tema, locale, tamanho e estado da janela, dados/fixture/hash, câmera/3D determinísticos e área de comparação. O PNG de estudo não declara DPI; não converter 1,5× em DPI homologado. Moldura, legendas e dados ilustrativos externos fazem parte da prancha: um recorte para compará-la ao app requer definição e aprovação explícitas, nunca exclusão automática.

Para **cada tela e estado obrigatório**, guardar: ID da matriz, requisitos e capacidades, referência autorizada/hash, screenshot real bruto/hash do aplicativo, ambiente fixado, fixture/hash, passos executados, sobreposição e diff verificáveis, métricas sem limiar autoconcedido, divergências e aceite humano expresso do proprietário. Imagens da aplicação, diffs e logs são evidências distintas dos desenhos. Um arquivo existente, palavras em documento, teste de texto, nome de screenshot, score agregado ou “parece igual” não dá PASS.

Critério padrão: **nenhuma tolerância visual autoconcedida**. Diferença ou evidência ausente = FAIL/BLOCKED/NOT RUN, conforme o fato; nenhum deles é PASS. Não ocultar deslocamento, cor, fonte ou geometria com máscaras, resize, alinhamento automático, blur, compressão, recorte ou normalização. Conteúdo dinâmico exige fixture determinística; máscara/exclusão somente se o proprietário aprovar expressamente região, motivo, versão e efeito, preservando a captura bruta. Essa autorização constitui alteração formal do protocolo, não dispensa silenciosa da meta.

100% exige todos os cenários obrigatórios aplicáveis com evidência íntegra e aceite humano do dono. Uma linha obrigatória sem execução/aceite bloqueia a declaração de conformidade do escopo. Não excluir casos por conta própria nem alterar o denominador. Se 100% não puder ser demonstrado, manter **BLOCKED até correção ou nova decisão explícita do proprietário**; nenhuma IA pode baixar a meta. O [template](ACEITE-VISUAL-C-TEMPLATE.json) registra campos necessários, não resultados executados.

## 5. Política de mudança e pendências

Qualquer proposta de mudança informa antes/depois, motivo técnico/funcional, cenário afetado, riscos, alternativas, recursos/versões/hashes e evidências. Até aprovação explícita nova, a referência anterior permanece vinculante e o item conflitante bloqueado. Após aprovação, criar versão nova identificada, atualizar matriz, contrato/manifesto e continuidade sem sobrescrever a evidência anterior.

Bloqueadores atuais: aprovação individual das derivadas; contratos detalhados indicados na matriz; ambiente de captura/DPI/renderização; fixtures geométricas executáveis da peça/câmera não presentes no pacote; estados interativos/responsivos; ordem do assistente e ligação específica de alinhar. Não inventar dependências, SDKs, versões de pacotes, tolerâncias ou medições ausentes. Publicar documentação não supre essas lacunas.

## 6. Fronteira operacional

Este documento não autoriza implementar UI, alterar `src`, motor, CI, instalador, servidor/website, conectar impressoras, imprimir, fazer merge ou lançar produto. No workstream pausado: G2/G3 BLOCKED, G4 NOT RUN, G5/G6 BLOCKED. O identificador FLN-VISUAL-C-100 é documental e não renomeia nem substitui os gates existentes.
