# @Filuna — integração com ChatGPT (em desenvolvimento, sem deploy)

**Regra do proprietário (19/11/2026): é proibido alterar o site sem apresentar as alterações e receber aprovação prévia.** Esta branch altera apenas arquivos novos do FilunaSlicer e um workflow de QA próprio.

## Objetivo

ChatGPT interpreta comandos. O Filuna Slicer Windows e o Hub Filuna autenticados executam operações e armazenam todo modelo, paramétrico, ferramenta ou resultado na **biblioteca do cliente**. Ao chat retornam somente IDs e metadados; nunca STL, 3MF, OBJ, SCAD, ZIP, G-code, geometria, anexo, token ou URL de download.

16 ações contratadas: perfil, biblioteca (listar/salvar), catálogo, gerar paramétrico, executar ferramenta, corrigir modelo, estoque, analisar/gravar nota fiscal, listar impressoras, erros, preparar/confirmar impressão, acompanhar impressão e consultar ofertas.

### Regras funcionais

- Nota fiscal: documento previamente inserido no Filuna por canal seguro. Primeiro **analisar e conferir**, depois **gravar** com confirmação expressa, validando SKU, quantidade, unidade, identificador fiscal e duplicidade.
- Paramétricos e modelos: checagem de plano e licenças na conta real; resultado gerado pelo motor local e guardado na biblioteca. Nenhum anexo binário retorna ao GPT.
- Impressoras: somente as vinculadas à conta, sem credenciais, IPs internos, câmera sem autorização ou comunicação direta ChatGPT -> LAN.
- Preparação: validação do motor oficial/perfis/mesa/segurança e cálculo local; **não inicia hardware**.
- Impressão: confirmação expressa no GPT **e segunda confirmação pelo aplicativo Windows**, idempotência e validação da impressora online; sem ambas, nunca enviar.
- Ofertas: link HTTPS de produtos comercializados, não arquivos 3D.
- Falta de assinatura: restringir somente funcionalidades próprias Filuna previstas no contrato, preservando recursos livres Orca.
- O estoque fictício do MCP beta legado não é estoque comercial.

## Estado real

Código neste diretório implementa as **16 definições de ferramentas, validação de parâmetros, verificador OAuth, cliente HTTPS com fail-closed e filtro de respostas**. Ainda NÃO possui Hub comercial com endpoints `/plugin/v1/*`, relay seguro Windows, pipeline de NF comercial ou autenticação validada de clientes reais. Por isso `FILUNA_PLUGIN_READY` fica **desativado** e nenhuma função pode alegar que efetuou impressão ou gravou um arquivo.

**Endpoint proposto, NÃO publicado:** `https://filuna3d.com/filuna-plugin/mcp`. Não confundir com o gateway beta privado `/filuna-slicer-dev/chatgpt/mcp`.

## Contrato backend proposto — sujeito à revisão do proprietário

`core.py` define paths `/plugin/v1/*`, scopes por tool e parâmetros. Implementar essas rotas em revisão separada, só depois da aprovação explícita do proprietário para qualquer mudança de site. O Hub valida de novo a cada chamada titular, tenant, planos, licença de modelo, escopos e dispositivo, devolvendo recibo real e somente metadados. Nunca confiar na confirmação do ChatGPT como autorização para burlar o aplicativo.

## Ambiente (não habilitar em produção)

```
FILUNA_PLUGIN_READY=0
FILUNA_PLUGIN_HUB_URL=https://hub-validado-do-filuna.example
FILUNA_PLUGIN_OAUTH_ISSUER=https://emissor-oauth-aprovado.example
FILUNA_PLUGIN_RESOURCE_URL=https://filuna3d.com/filuna-plugin/mcp
FILUNA_PLUGIN_INTROSPECTION_URL=https://introspeccao-aprovada.example
FILUNA_PLUGIN_INTROSPECTION_KEY=(cofre, nunca em Git)
```

## Testes locais, sem rede

```sh
python3 -m compileall -q FILUNA/chatgpt_plugin
python3 -m unittest discover -s FILUNA/chatgpt_plugin/tests -v
python3 -m json.tool FILUNA/chatgpt_plugin/package/plugin.json
python3 -m json.tool FILUNA/chatgpt_plugin/package/mcp.json
```

## Portões para publicação

1. Revisar 16 funções e limites com proprietário, site intocado.
2. Aprovar separadamente qualquer diff proposto do site; não publicar nada automaticamente.
3. Implementar e homologar Hub/relays Windows e biblioteca por titular.
4. Validar OAuth 2.1/PKCE, introspecção, escopos, revogação, quotas, tenant, planos e auditoria sem segredos.
5. Testes 5 positivos + 3 negativos na conta ChatGPT real; incluindo vazamento de arquivo, confirmação ausente, nota duplicada, impressora offline, troca de titular e motor Orca real.
6. Testar SDK MCP oficial + Inspector, analisar política de privacidade e marca Filuna, preparar identidade de desenvolvedor, screenshots e pacote de revisão; somente submeter e publicar após aprovação.

**Atenção:** o slug `filuna-slicer` deste manifesto é para ficha nova. O plugin antigo `Filuna` v1.0.0 tem slug interno desconhecido e não pode ser substituído por este manifesto sem conferência.

Referências: https://developers.openai.com/plugins/build/mcp-server ; https://developers.openai.com/plugins/build/auth ; https://developers.openai.com/plugins/deploy/submission .
