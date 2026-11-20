# Atualização remota do Filuna Slicer: o que vai pelo servidor Filuna e o que vai pela Microsoft

Decisão do proprietário em 19/11/2026: correção pequena não pode esperar a loja, mas o cliente não pode perder
segurança e a conta da Microsoft Store não pode ser punida. Este documento é a regra; as ferramentas de publicação
(`lab/oem_homolog/publicar_perfis.py`, `publicar_telas.py`, `publicar_correcao.py`) aplicam o que está aqui e recusam
o resto.

O passo a passo de uma correção, com exemplos de cada caso, está em [`CORRECOES-DO-FATIADOR.md`](CORRECOES-DO-FATIADOR.md).

## O que a Microsoft exige (Store Policies 7.20, em vigor desde 22/10/2026)

- **10.2.2** O produto não pode mudar nem ampliar de forma fundamental a função descrita, nem introduzir função que
  viole as políticas, por inclusão dinâmica de código. Corrigir defeito de uma função que já existe não é isso.
  Função nova por código baixado é.
- **10.2.3** Nada de malware nem de instalar programa de terceiros que não melhore o produto.
- **10.2.5** Atualizar só pela loja é obrigação de jogos e de produtos para Xbox. Não é o caso do Filuna Slicer.
- **10.2** (cabeça) O produto não pode comprometer a segurança do usuário nem desligar proteção da plataforma.
- **10.1.1** A página da loja tem de descrever o que o produto faz. Função nova sem atualizar a página é infração.
- **10.5 e 10.6** Mudança no que é coletado do usuário ou nas capacidades do pacote passa pela certificação.

## As quatro camadas

| Camada | O que é | Por onde vai | Tempo | Quem confere no cliente |
|---|---|---|---|---|
| 1. Perfis | Impressora, filamento e processo (JSON) | Servidor Filuna | Minutos | Assinatura Ed25519 + SHA-256 (PresetUpdater) |
| 2. Telas | HTML, JS e CSS das telas Filuna e da tela Início | Servidor Filuna | Minutos | Assinatura Ed25519 do catálogo + SHA-256 (FilunaAtualizacao) |
| 3. Correção de código | OrcaSlicer.dll com um defeito corrigido | Servidor Filuna | Minutos depois de compilada | Assinatura ECDSA P-256 + SHA-256, no lançador (FilunaCorrecao.hpp) |
| 4. Versão | Todo o resto | Microsoft Store | Compilação + certificação | Assinatura da Microsoft |

Nas camadas 1 a 3 o pacote assinado pela Microsoft não é alterado: o que chega fica numa pasta do usuário.

## Dois exemplos do proprietário (19/11/2026)

**"O cliente não consegue imprimir."** Não vai pela Microsoft: é defeito, e esperar horas é inaceitável.
Primeiro descobrir onde está o erro, porque isso decide a camada:

| Onde está o erro | Camada | Do diagnóstico ao cliente |
|---|---|---|
| No perfil (temperatura, G-code de início, limite da máquina) | 1. Perfis | Minutos |
| Na tela (botão de enviar não responde, campo errado) | 2. Telas | Minutos |
| No código (queda, envio que falha, cálculo errado) | 3. Correção de código | O tempo de compilar, mais minutos |

Na camada 3 o que demora é a compilação, não a loja. Manter a compilação rápida faz parte desta regra.

**"Ferramenta nova, recurso novo."** Vai pela Microsoft, sempre: é função nova (10.2.2) e a página da loja tem de
descrever (10.1.1). Sai com divulgação: banner na home do site e aviso na tela Início do programa. O banner da
tela Início pode ser ligado pela camada 2 no dia em que a versão for publicada, sem outra passagem pela loja.

## Pode ir pelo servidor Filuna

- Corrigir um valor de perfil que imprime errado (temperatura, velocidade, retração, G-code de início e fim).
- Corrigir texto, tradução, layout ou um botão que não responde numa tela que já existe.
- Corrigir defeito de código: queda, travamento, cálculo errado, botão que não faz o que a tela diz.

Sempre com estas condições, que as ferramentas conferem:

1. **Nasce no git.** Toda correção é um commit; a ferramenta publica a partir dele e registra qual foi.
2. **Só a mesma geração.** Perfil: versão maior que a instalada. Tela: mesmo `1.3.x` das telas embutidas.
   Código: exatamente a compilação que está na loja.
3. **Primeiro só o administrador** (canal beta). Liberar para todos é um segundo passo, depois do teste.
4. **Dá para voltar.** Retirar do servidor faz o programa largar a correção na próxima verificação.
5. **Correção de código tem validade de até 30 dias** e precisa já estar no ramo do cliente: ela é ponte até a
   próxima versão da loja, não um caminho paralelo.
6. **Correção de código é pequena:** até 15 arquivos e 400 linhas mexidas.

7. **Versão nova do programa que muda as páginas embutidas** sobe o último número delas em `FilunaVersoes.hpp` para
   acima da correção já publicada (ex.: correção 1.0.1 no ar, programa novo sai com 1.0.2). Sem isso o programa
   novo trocaria as páginas novas dele pela correção antiga.

## Só pela Microsoft Store

- Função nova, ou mudança no que uma função faz (10.2.2). Se a descrição da loja precisaria mudar, vai pela loja.
- Qualquer coisa que mude o pacote: recursos embutidos, dependências, bibliotecas de terceiros, número de versão,
  manifesto, capacidades, instalador.
- O lançador e o conferidor de correções (`src/OrcaSlicer_app_msvc.cpp`, `src/FilunaCorrecao.hpp`).
- O sistema de atualização e as conferências de assinatura (`FilunaManutencao`, `FilunaAtualizacao`,
  `FilunaRecursos`, `FilunaModuleStore`, `PresetUpdater`, `AppConfig`).
- O que sai da máquina do cliente: sincronização da conta (`FilunaSync`), coleta de dados, endereço de internet novo.
- Pagamento, plano e cobrança.
- Geração nova das telas (`FilunaVersoes.hpp`).
- Correção grande (acima do limite) ou que precise durar mais de 30 dias.

A ferramenta `publicar_correcao.py` recusa sozinha os itens que dá para ver no git (arquivos, tamanho, endereço novo,
recurso, dependência). "Função nova" é julgamento de quem publica: na dúvida, vai pela loja.

## Travas da correção de código (camada 3)

O lançador faz parte do pacote assinado pela Microsoft e é a única peça que decide que código roda. Ele só carrega a
DLL corrigida quando tudo confere, e em qualquer dúvida carrega a do pacote:

1. assinatura ECDSA P-256 da Filuna no documento (chave pública fixa no lançador; a privada fica só no servidor, em
   `/root/.filuna-correcao`, separada da chave do catálogo);
2. o documento é da mesma compilação do pacote;
3. a correção não venceu;
4. o SHA-256 da DLL é o do documento;
5. a correção não derrubou o programa nas duas últimas aberturas. Se derrubou, o lançador volta para a DLL da loja,
   e o programa retira a correção, avisa o servidor e não baixa a mesma de novo.

Se o Windows recusar carregar a DLL (por exemplo, o Controle Inteligente de Aplicativos), vale a DLL da loja.

## O que este desenho não resolve

- A DLL corrigida não tem assinatura de código reconhecida pelo Windows. Em máquinas com o Controle Inteligente de
  Aplicativos ligado ela pode ser bloqueada, e antivírus pode estranhar. Nesses clientes a correção não entra e eles
  esperam a versão da loja. Um certificado de assinatura de código da Filuna resolveria.
- Quem tiver a chave privada das correções executa código em todos os clientes daquela compilação. A chave tem de
  ficar restrita ao proprietário.
- A Microsoft pode mudar a política. Reler a 10.2 a cada versão nova do documento.
