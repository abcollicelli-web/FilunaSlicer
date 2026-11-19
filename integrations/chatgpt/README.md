# Filuna Slicer × ChatGPT — desenvolvimento isolado

**Status: protótipo para revisão, sem publicação oficial e sem alteração no site.**

Os códigos desta pasta são isolados do motor Orca e do site Filuna. A branch não realiza deploy, não toca no banco/portal e não inicia impressões.

## Regra central

Modelos 3D, paramétricos, arquivos importados, correção de malhas e resultados de ferramentas são salvos apenas na **biblioteca autenticada do cliente no Filuna Slicer**. O ChatGPT recebe identificadores de biblioteca/estado, nunca STL, 3MF, G-code, ZIP, URL de download, geometria ou caminho. Para modelos protegidos, o chat não oferece nenhuma exportação.

## 20 ferramentas contratadas

1. Recursos disponíveis; biblioteca; modelos; salvar modelo; importar modelo do cliente.
2. Paramétricos; gerar paramétrico; ferramentas; executar ferramenta; reparar modelo.
3. Estoque; prévia de nota fiscal; confirmar nota fiscal no estoque.
4. Impressoras; status; erros; preparar impressão; solicitar impressão após confirmação local; acompanhar impressão.
5. Ofertas de filamentos, impressoras e acessórios.

Nenhuma função sem certificação real do novo Hub fica liberada: a API de capacidades decide por usuário. Não existem operações com shell, rede LAN direta, downloads de arquivos, fatiamento no servidor ou envio de impressão sem autorização humana no app Windows.

## Dependências que exigem revisão prévia do proprietário

Os endpoints a seguir são **propostas, não implementações do site**:

- POST /internal/chatgpt/v1/capabilities -> objeto com certified_tools por conta e plano.
- POST /internal/chatgpt/v1/execute -> recebe tool e arguments, autoriza owner/tenant, opera no Slicer e retorna apenas campos permitidos.

O código deste diretório inclui gateway MCP SDK 2, autenticação via introspecção OAuth 2.1, contratos de 20 ferramentas e testes. Exige integração multiusuário real, backend acima e endpoint HTTPS estável antes de operar. **É proibido modificar o site, suas APIs, infraestrutura ou deploy sem apresentar as mudanças para aprovação do proprietário.**

## Testes

Rode a partir desta pasta: python -m unittest discover -s tests -v. Testes são sintéticos/locais; não homologam impressoras físicas, OAuth comercial, emissão de nota nem geração 3D real.

## Pacote para o diretório oficial

A pasta package traz manifesto portátil Agent Plugins e skill de comportamento. O mcp.json e materiais de submissão **não são gerados** até existir servidor comercial HTTPS e registro aprovado, para não anunciar endpoint beta como produção. URLs verificadas de privacidade/termos/suporte, logo e ícone oficiais, casos para revisão, SDK Inspector, aprovação do proprietário e publicação no painel ainda pendem. A ficha anterior 'Filuna' tem nome interno original desconhecido; o manifesto 'filuna-slicer' é para ficha nova, não deve atualizar a existente sem recuperar seu identificador exato.

Referências: https://developers.openai.com/plugins/build/plugins e https://developers.openai.com/plugins/deploy/submission.
