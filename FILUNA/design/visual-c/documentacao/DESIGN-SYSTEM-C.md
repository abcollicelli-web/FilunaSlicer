# DESIGN-SYSTEM-C — referência visual Filuna

Estado: DIREÇÃO VISUAL APROVADA pelo proprietário t26u em 08/11/2026 13:40 Europe/Rome. Consulte APROVACAO-VISUAL-C.json e os dois modelos originais antes de criar ou alterar telas. **Todas as derivadas deste lote são propostas**, não extensão automática de aprovação para detalhes ou funcionalidades. “Precision Atelier” é nome interno da direção, não marca do produto.

## Construção obrigatória
Porcelana para leitura, grafite para geometria, rosa para ação/seleção; original Filuna em well proporcional. Sem moldura bordô, dashboard de caixas idênticas, títulos de página duplicados ou mascote sobre ferramentas técnicas. Layout deve servir à tarefa: viewport, tutorial, formulário, grade e instrumento têm composições próprias. Nada de recolorir B e chamar de C.

## Tokens e tipografia
Valores em tokens.json. Inter 400/500/600; escala base 11/12/13/14/15/17/19/24/28/34/49/82 px no viewBox 1920×1280, render final 1,5× ou superior. Títulos concisos, rótulos pequenos mas contrastantes; números alinhados por unidade e função, preferindo tabular-nums e alinhamento direito. Fonte OFL original no pacote; sem SF Pro/SF Symbols proprietários copiados.

## Grade, espaçamento e superfícies
Prancha 1920×1280; janela 40,80,1840,1080; shell de 76px. Margens do conteúdo 24–32; espaçamento 4/8/12/16/24/32/48; raio controles 9–13, grupos/câmera 18–20. Grade de 8px com ajustes ópticos. Mesa Preparação ocupa toda a largura à direita do inspetor de 360px; painel de arquivo removido não retorna. Apenas uma barra funcional elevada, borda luminosa sutil e fundo suficiente para contraste. Formulários e instrumentos opacos; não aplicar vidro a toda a UI.

## Componentes e estados candidatos
Botão primário: rosa e branco. Secundário: superfície branca, contorno neutro e texto escuro. Hover realça moderadamente, sem salto de layout. Foco de teclado: anel externo 2px com afastamento; não apenas mudança de cor. Disabled: texto legível e razão disponível; não parecer acionável. Loading preserva largura e explicita progresso; não transforma clique em sucesso. Estados hover/foco/loading são PROPOSTOS, representados em DS-01, não implementação.

Tabs mantêm nomes e ordem Início / Preparação / Visualização / Dispositivo / Projeto / Calibração. Seleção associa peso, forma e texto à cor. Formulários usam rótulo persistente, unidade e erro junto ao campo; placeholder não é rótulo. Checklists diferenciam medição automática, confirmação humana e N/A comprovado. Listas/tabelas alinham colunas, unidades, cabeçalhos e seleção; não esconder ação destrutiva como ação comum. Empty orienta uma primeira ação real; loading não fabrica itens; permissão negada não oferece bypass; erro dá causa e recuperação sem sucesso simulado.

## Ícones
Pictogramas próprios em grade 32, traço 1,8, cantos arredondados e escala coerente. Ferramenta = um pictograma. Sem emojis/letras, logos Apple ou ícones administrativos em controles de impressão. Toolbar herdada de B separa 14 funções nos grupos nativos de três atalhos Filuna propostos (selecionar/alinha/espelhar); ligação específica de alinhar ainda pendente. Girar inicialmente ativo. Tooltip e nome acessível são requisitos futuros; o SVG estático não prova hover.

## Cor, acesso e adaptação
Contraste de tokens: texto principal/porcelana 13,52:1; secundário 5,04:1; branco/rosa 6,54:1; ícones claros/grafite 13,32:1. Alvos: texto normal ≥4,5:1, grande e controles essenciais ≥3:1. Revalidar combinações reais/estados, não declarar auditoria AA inteira. Erro/seleção não dependem só de cor. Redução de transparência usa superfície opaca equivalente; reduce-motion substitui animação por estado estático sem perder informação. Interação teclado, foco, leitores de tela, escala 125/150/200%, alto DPI e contraste elevado exigem verificação futura no Windows.

Layout responsivo é regra de projeto, não teste aprovado: preservar viewport e ações; rolagem interna da lateral/grade, não documento infinito; adaptar inspetores em drawers/painéis reversíveis se necessário sem esconder Global/Objetos. As imagens não comprovam responsividade ou acessibilidade implementadas.

## Dados, imagens e segurança
Disclaimer global visível por prancha: conceito não funcional/dados ilustrativos. Não repetir “exemplo” em cada número. Nome de cenário, percentual e temperatura não são telemetria. Render de câmara sempre identificado como ilustração genérica, nunca foto falsa OEM/stream real. Assets autorais, logo original e suas licenças preservados. Estado técnico sem evidência permanece desconhecido na implementação futura. Perfil não é descoberta/cadastro/pareamento/online. Enviar não inicia; fila/agenda não iniciam; comandos exigem titular, sessão, capacidade, confirmação e ACK verdadeiro. Admin/servidor não controla impressora. Remoto futuro separado e OFF. Catálogo print-only não autoriza exportação original nem promete DRM absoluto; arquivos próprios preservam direitos.

## Hierarquia e fontes atuais
Pedido t26u governa aparência, não reabre engenharia. Head documental atual 83efc13ee2bfdb208859ceb6095fcbd69fe5bef9. Novo adendo U1 exige G-code imutável, análise somente leitura, distinção de risco OEM e divergência Filuna; telas de risco desenhadas em VIS-06 (OEM/aceite por SHA) e VIS-07 (ENGINE_DIVERGENCE/sem override), ainda propostas não implementadas. CONT/PLANO foram comparados integralmente à base antes lida e todos os deltas novos examinados; AGENTS/QUAL/identidade e contratos UX selecionados relidos. Fontes e cobertura em FONTES.json. Não se afirma releitura integral de todos os repositórios.

## Referências modernas verificadas
Play — Apple Design Awards 2025 Innovation; Feather — Visuals and Graphics; Watch Duty — Social Impact. Prêmios de 2025, não 2026. https://www.apple.com/newsroom/2025/06/apple-unveils-winners-and-finalists-of-the-2025-apple-design-awards/ e https://developer.apple.com/design/awards/2025/ . Liquid Glass é referência de linguagem de 09/06/2025, não prêmio: https://www.apple.com/newsroom/2025/06/apple-introduces-a-delightful-and-elegant-new-software-design/ . Shapr3D Display Modes, documento atualizado 14/09/2026, produto análogo sem prêmio atribuído: https://support.shapr3d.com/hc/en-us/articles/26807298276124-Display-Modes . Transferência: conteúdo dominante, instrumentos hierárquicos, camada funcional contida, geometria legível. Fontes/imagens analisadas no trabalho C original; não há screenshots rivais nas pranchas. iF bloqueado foi descartado como prova.

## Governança
Aprovação visual não é G1 release, implementação, teste, homologação física, deploy ou merge. G2/G3 BLOCKED, G4 NOT RUN, G5/G6 BLOCKED neste workstream. Nenhum código de aplicativo. Qualquer futura IA deve ler este documento, aprovação, referências e matriz antes de desenhar; se faltar fonte ou imagem, registrar pendência, não declarar cobertura completa.


## Cobertura consolidada desta revisão
123 pranchas 2880×1920, em 11 famílias. A matriz rastreia 208 linhas (205 recebidas e três continuações explícitas). “Cobertura visual” significa desenho estático do inventário delimitado, não completude do software, validação de interação, aprovação individual das derivadas ou liberação de gates. Não há chamada de gerador, motor, impressora ou servidor.

### Entradas e variantes
Cada gerador tem sua própria composição e campos provenientes de UX-P §4; valores numéricos sem default no contrato são exemplos propostos, não especificação final. Itens com imagem têm variante de entrada sem peça, render ilustrativo após imagem, gerando sem arquivo válido, erro sem sucesso, e arquivo gerado com próxima ação. Os quatro subestados menores são desenhados em faixa inferior e localizados por coordenadas na matriz, não contados só por texto de checklist. Capas são arte autoral demonstrativa: não substituem as capas reais do catálogo na implementação.

### Continuidade funcional
Revisão de risco não modifica G-code. Risco do próprio OEM admite revisão de aceite consciente e auditável por usuário/SHA; uma alteração no hash invalida o aceite. ENGINE_DIVERGENCE bloqueia sem override e nunca recebe aceite de fabricante. Campos de G-code em presets não são convite para corrigir arquivos gerados. Fila e agenda só abrem revisão/aviso; não iniciam. Sessão, titularidade, capacidade e confirmação continuam necessárias.

### Lacunas documentais não disfarçadas
Visualização, Projeto, Ateliê, ferramentas auxiliares, conta/temas e telas web possuem detalhamento visual candidato em relação às fontes gerais; não se inventou confirmação contratual. Sincronização automática não foi confirmada e não aparece como ativa. O assistente usa uma ordem proposta, porque fontes antigas divergem; confirmar ordem antes de implementar. Os cenários não promovem recursos beta a estáveis. Guia de calibração não tem arquivo e termina pedindo sair do modo de calibração antes de peça comum.

## Adendo vinculante de fidelidade — 08/11/2026

[FLN-VISUAL-C-100](REGRA-FIDELIDADE-VISUAL-C.md) torna obrigatória a reprodução 100% idêntica do modelo aprovado, não mera inspiração. [Contrato de montagem](CONTRATO-MONTAGEM-VISUAL-C.md) / [JSON](CONTRATO-MONTAGEM-VISUAL-C.json) fixa baseline, hashes e medidas reais. Toda sugestão de adaptação/estado neste documento que não tenha aprovação individual permanece proposta: em conflito ou lacuna, bloquear, não interpretar como liberdade de redesenhar. Nova mudança exige aprovação explícita do proprietário. [Ordem nova](ORDEM-FIDELIDADE-VISUAL-C-20261108.json) não altera a aprovação anterior nem aprova derivadas automaticamente. Conformidade real da aplicação NOT RUN; aceitação segue evidência visual bruta, sobreposição/diff e aceite humano, sem tolerância autoconcedida.
