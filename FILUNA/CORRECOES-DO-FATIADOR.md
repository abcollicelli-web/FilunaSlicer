# Como corrigir um erro do Filuna Slicer que já está com os clientes

Guia de trabalho para qualquer pessoa ou conversa do Claude que receba "o cliente tem um erro no fatiador". Ele diz,
na ordem, o que fazer do diagnóstico até a correção chegar ao cliente, e onde cada coisa fica.

A regra (o que pode ir pelo servidor Filuna e o que só vai pela Microsoft) está em
[`ATUALIZACAO-REMOTA.md`](ATUALIZACAO-REMOTA.md). Este guia não a repete: leia as duas antes da primeira correção.

## 0. Regras que valem em todo o caminho

- **Publicar para clientes só com o OK expresso do proprietário.** O canal `beta` (só a conta de administrador) pode
  ser usado para testar sem perguntar; o canal `todos` e o envio à Microsoft, não.
- **Não derrubar o trabalho de outra sessão.** Várias conversas do Claude trabalham no mesmo ramo e no mesmo servidor.
  Antes de commitar: `git fetch` e juntar o que chegou; nunca `push --force`; nunca cancelar uma compilação cujo
  `headSha` não é seu; só publicar a compilação mais nova do ramo. Regras completas no servidor:
  `/srv/filuna-homolog/LEIA-ANTES-DE-PUBLICAR.md`.
- **Não tocar a instalação nem os dados do proprietário** (`%APPDATA%\FilunaSlicer`, o app da Store dele). Teste com
  pasta de dados própria (seção 5).
- **Senhas, chaves e códigos são digitados pelo proprietário.** As chaves de assinatura ficam só no servidor:
  `/root/.filuna-modulos/ed25519.pem` (perfis e telas) e `/root/.filuna-correcao/p256.pem` (código).
- **Toda correção nasce no git** e vai também para a próxima versão da loja. Correção que só existe no servidor some
  quando o cliente atualiza o programa.

## 1. Onde fica cada coisa

| O quê | Onde |
|---|---|
| Repositório | `abcollicelli-web/FilunaSlicer` |
| Ramo do cliente (tudo o que vai para a loja sai daqui) | `filuna/v1-teste-fisico` |
| Ramo de onde sai o pacote da loja (workflow MSIX) | `filuna/main` |
| Servidor | `ssh filuna` (as ferramentas rodam lá, como root) |
| Ferramentas de publicação | `lab/oem_homolog/publicar_perfis.py`, `publicar_telas.py`, `publicar_correcao.py` |
| O que o servidor entrega | `/var/lib/filuna-v2/modulos` (perfis e telas) e `/var/lib/filuna-v2/correcao` (código) |
| Registro do que foi publicado | `publicacoes.log` em cada uma dessas pastas |
| App na loja | Microsoft Store 9MV6C5DJ3201, Partner Center "Filuna Slicer" |

As ferramentas usam um checkout do repositório no servidor (`--repo`). Use um checkout seu, com `git fetch` feito,
e confira que ele está no commit certo antes de publicar.

## 2. Descobrir qual versão o cliente tem

Cada executável traz `filuna-build.json` ao lado (`{"build":"<9 dígitos do commit>","versao":"2.5.0-beta.NNN"}`). A
tela Início mostra a mesma coisa no rodapé ("Versão … · <build>").

- **Cliente da Microsoft Store:** o build é o da última versão publicada da loja. Para saber qual commit foi: o
  pacote MSIX é montado pelo workflow `filuna-loja-msix.yml` a partir de uma compilação completa; o `filuna-build.json`
  dentro do pacote diz o commit. Esse commit é o `--base` de uma correção de código (seção 6.3).
- **Cliente do instalador do site:** recebe versões novas pelo canal do site (`publicar_app.py`, regras em
  `/srv/filuna-homolog/LEIA-ANTES-DE-PUBLICAR.md`). Para ele a correção de código vai como versão normal do site, não
  pela camada 3.

Com o suporte remoto autorizado pelo cliente (tela Início › "Suporte remoto"), a Filuna pode ler a versão, as
impressoras, os módulos e o registro de erros do programa dele.

## 3. Achar onde está o erro (isso decide a camada)

| Sintoma | Onde costuma estar | Camada |
|---|---|---|
| Imprime errado numa impressora ou filamento: temperatura, velocidade, retração, G-code de início ou fim, limite da mesa | Perfil em `resources/profiles/<Fabricante>/` | 1. Perfis |
| Tela Filuna com texto errado, botão que não responde, campo errado, layout cortado (Meu espaço, Estoque, Dispositivo, Farm, Modelos 3D) | `resources/web/filuna/` | 2. Telas (`telas`) |
| O mesmo na tela Início | `resources/web/homepage/` e o que ela usa de `resources/web/include/` | 2. Telas (`inicio`) |
| Queda, travamento, envio que falha, cálculo errado, botão nativo (barra de cima, Preparação) que não faz o que diz | `src/` | 3. Código |
| Precisa de função nova, mudar o que uma função faz, recurso embutido, dependência, instalador | — | 4. Loja |

Na dúvida entre "defeito" e "função nova", vai pela loja (Store Policies 10.2.2).

## 3.1 Exemplos: direto no fatiador do cliente ou pela Microsoft?

A pergunta é sempre a mesma: **a função já existe no programa da loja e está com defeito** (vai direto, pelo servidor
Filuna) **ou o programa passa a fazer algo que não fazia** (vai pela Microsoft)?

### Exemplo completo: "estou com uma câmera que não aparece"

A tela Dispositivo mostra a câmera de impressoras que o programa já sabe ver: Bambu Lab pela rede local (repasse em
`src/slic3r/Utils/FilunaCameraRelay`), as que o agente da marca descobre (`FilunaCameraUrls`) e vídeo FLV (Anycubic)
pelo tocador da página. Então "a câmera não aparece" quase sempre é defeito de algo que já existe, e vai direto:

| O que o diagnóstico mostrou | Onde corrigir | Camada | Chega ao cliente |
|---|---|---|---|
| A câmera é descoberta, mas a tela não mostra (vídeo não toca, imagem cortada, aviso errado, botão que não abre) | `resources/web/filuna/dispositivo.html`, `camera-conta.js` | 2. Telas (`telas`) | Minutos |
| O programa monta o endereço da câmera errado para essa impressora (porta, caminho, protocolo) ou o repasse local falha | `src/slic3r/GUI/FilunaDispositivos.cpp`, `src/slic3r/Utils/FilunaCamera*` | 3. Código | Compilar + minutos |
| A câmera depende de um dado do perfil da impressora (modelo marcado sem câmera, endereço no perfil) | `resources/profiles/<Fabricante>/` | 1. Perfis | Minutos |
| A marca ou o modelo nunca teve câmera no Filuna (protocolo novo, marca nova, câmera USB que o programa não lê) | — | 4. Microsoft | Versão nova |

Como diagnosticar: com o suporte remoto autorizado, ler no registro do cliente as linhas de câmera e de diagnóstico
(a tela manda "diagnostico.enviar" com etapa `camera.flv` quando o tocador falha) e o que a tela Dispositivo recebeu
(`camera`, `camera_url`, `camera_repasse`, `camera_flv`). Se o endereço chega certo e a tela não toca, é tela; se o
endereço chega errado ou vazio, é código ou perfil.

Cuidado na camada 3: a ferramenta recusa endereço de internet novo no código. Endereço de câmera na rede local do
cliente vem da impressora, não fica escrito no código; se a correção precisar escrever um endereço fixo novo, leia o
motivo da recusa e, na dúvida, vá pela loja.

### Outros exemplos

| Caso | Por onde | Por quê |
|---|---|---|
| Impressora imprime a primeira camada fria com PETG | Direto (perfil) | Valor de perfil errado |
| G-code de início de uma impressora bate o bico na mesa | Direto (perfil) | Perfil errado |
| Texto cortado ou botão fora do lugar numa tela Filuna ou na Início | Direto (tela) | Defeito de tela que já existe |
| Botão "Enviar para a impressora" não faz nada | Direto (tela ou código, conforme o diagnóstico) | Função existente quebrada |
| Programa fecha sozinho ao escolher uma impressora | Direto (código) | Queda é defeito |
| Cálculo de preço, peso ou tempo errado | Direto (código) | Cálculo errado é defeito |
| Envio para uma impressora que já é suportada falha depois de atualização do firmware dela | Direto (código), se o ajuste é pequeno | Manter funcionando o que já é anunciado |
| Tradução errada numa tela Filuna | Direto (tela) | Texto |
| Tradução errada num texto nativo (`localization/`) | Microsoft | Recurso embutido no pacote |
| Botão cortado na barra de cima ou na Preparação (parte nativa) | Direto (código), se couber no limite; senão Microsoft | Defeito de layout |
| Impressora de marca nova, ou modelo novo sem perfil no programa | Microsoft (ou perfil, se a marca já existe e só falta o modelo dentro dela) | Marca nova é função nova |
| Ferramenta nova, aba nova, recurso novo | Microsoft, com banner de divulgação | Função nova (10.2.2) e a página da loja tem de dizer (10.1.1) |
| Mudar o que é enviado para a internet, conta, sincronização, pagamento, plano | Microsoft | Dados e cobrança passam pela certificação |
| Corrigir o próprio atualizador, o lançador ou a conferência de assinatura | Microsoft | Quem decide que código roda nunca é trocado de fora |
| Correção grande (mais de 15 arquivos ou 400 linhas) ou que precise durar mais de 30 dias | Microsoft | Limite da camada 3 |

Mesmo o que vai direto ao cliente entra no ramo do cliente e sai na próxima versão da Microsoft (seção 7).

## 4. Corrigir no git

```bash
git fetch origin
git switch -c filuna/corrige-<assunto> origin/filuna/v1-teste-fisico   # perfis e telas
```

- Commit pequeno, mensagem dizendo o defeito e a causa. Autor e coautor como os commits anteriores do ramo.
- **Perfil:** suba o `version` em `resources/profiles/<Fabricante>.json`; a ferramenta recusa versão que não sobe.
- **Tela:** se a correção mexe em página que já tem correção publicada, a próxima tem o último número maior
  (`telas` 1.3.1 → 1.3.2).
- **Código:** a correção parte do commit da loja, não da ponta do ramo (ver 6.3).
- Junte no ramo do cliente (`git merge --no-ff`, depois `git fetch` de novo e `git push origin HEAD:filuna/v1-teste-fisico`).
  As três ferramentas recusam commit que não está no ramo do cliente ou que não parte dele.
- Pegadinha conhecida: gerar arquivo C++ por heredoc do Bash pode reduzir `\\` a `\`. Escreva C++ com o editor de
  arquivos, não com `cat <<EOF`.

## 5. Compilar e testar no Windows

### Compilar

- **Rápida** (só o que mudou, 16 a 40 min): artefato `filuna-rapido-windows-exe-<sha>`.
  ```bash
  gh workflow run filuna-windows-rapido.yml -R abcollicelli-web/FilunaSlicer --ref filuna/v1-teste-fisico -f ramo=<ramo>
  ```
- **Completa** (60 a 80 min): dispara sozinha a cada push no ramo do cliente que mexe em `src/**`, `CMakeLists`,
  `deps`, `build_win.bat` ou `version.inc`; à mão:
  `gh workflow run filuna-windows-qa-exe.yml -R abcollicelli-web/FilunaSlicer --ref filuna/v1-teste-fisico`.
  Mudança só em `resources/` não dispara a completa: dispare à mão quando ela for para a loja.
- O pacote da loja sai **sempre da completa**, nunca da rápida.
- Baixe os artefatos no PC (`gh run download <run> -R …`); no servidor o `gh run download` falha. Para levar ao
  servidor, use `scp`.
- Compilação antiga sua, que um commit novo seu tornou inútil, pode ser cancelada. A de outra sessão, nunca.

### Testar sem tocar no do proprietário

Abra o executável baixado com pasta de dados própria e, se for testar correção de código, pasta de correções
própria:

```powershell
$env:WEBVIEW2_USER_DATA_FOLDER = "<pasta de teste>\webview"   # senão a conta logada é a mesma de todas as instalações
$env:FILUNA_CORRECAO_DIR      = "<pasta de teste>\correcao"  # só para testar a camada 3
& "<pasta do executável>\FilunaSlicer.exe" --datadir "<pasta de teste>\dados"
```

- Fotografe a tela que tinha o erro **em mais de uma largura de janela** (por exemplo 1100, 1366 e 1920 px). Texto
  cortado quase sempre só aparece em janela estreita.
- O registro do programa fica em `<pasta de teste>\dados\log`. As linhas "Filuna: …" dizem de onde veio cada
  correção ("tela … vem da correção do servidor", "programa rodando com a correção de código N do servidor").
- Para telas, dá para conferir o HTML antes de compilar servindo a pasta `resources/web` num servidor local e abrindo
  `homepage/index.html` num navegador na largura da janela. O teste que vale é o do programa.

## 6. Publicar a correção

Sempre em duas etapas: **beta** (só o administrador recebe; testar com a conta admin) e, com o OK do proprietário,
**todos**. As três ferramentas recusam com o motivo escrito (saída 4) e não rodam duas ao mesmo tempo (saída 3).
Leia o motivo e corrija; não contorne.

### 6.1 Perfis

```bash
sudo python3 lab/oem_homolog/publicar_perfis.py --repo <checkout> --ref <commit> --fabricante <Pasta> \
    --versao <maior que a publicada> --notas "<o que muda, para o cliente>" --canal beta
```

Liberar: o mesmo comando com `--canal todos`. O programa busca perfis ao abrir; teste fechando e abrindo.

### 6.2 Telas

```bash
sudo python3 lab/oem_homolog/publicar_telas.py --repo <checkout> --ref <commit> --recurso telas|inicio \
    --versao <mesma geração, último número maior> --notas "<o que muda>"
```

- A geração é a de `src/slic3r/Filuna/FilunaVersoes.hpp` no programa que está na loja (`telas` 1.3.x → id
  `telas-1-3`; `inicio` 1.0.x → id `inicio-1-0`). Versão de outra geração é recusada.
- Nasce no canal beta. Liberar para todos é no admin do site: `/admin/modulos`.
- O programa aplica na abertura seguinte. Ao testar, abra duas vezes.

### 6.3 Código (só para a versão da Microsoft Store)

Uma correção de código é uma `OrcaSlicer.dll` compilada **a partir do commit que está na loja**, com só a correção
por cima. O lançador do pacote só a carrega se a assinatura, a compilação, a validade e o SHA-256 conferirem
(`src/FilunaCorrecao.hpp`), e volta sozinho para a DLL da loja se ela derrubar o programa duas vezes.

1. Ramo a partir do commit da loja (o `build` do `filuna-build.json` do pacote publicado):
   ```bash
   git switch -c filuna/correcao-<build>-<n> <commit da loja>
   ```
2. Corrija só em `src/`, fora das áreas só-pela-loja (lista em `ATUALIZACAO-REMOTA.md`), até 15 arquivos e 400
   linhas, sem endereço de internet novo. Commit e push desse ramo.
3. Junte esse ramo no ramo do cliente (a correção tem de ir na próxima versão da loja; a ferramenta confere).
4. Compile a rápida **do ramo da correção** (`-f ramo=filuna/correcao-<build>-<n>`). Baixe o artefato e confira que o
   `filuna-build.json` dele diz o commit da correção.
5. Leve a pasta do artefato ao servidor (`scp -r`) e publique em beta:
   ```bash
   sudo python3 lab/oem_homolog/publicar_correcao.py --repo <checkout> --base <commit da loja> \
       --ref <commit da correção> --artefato <pasta com OrcaSlicer.dll e filuna-build.json> \
       --versao <1, 2, 3… dentro dessa versão da loja> --motivo "<o defeito, em uma frase>" --canal beta
   ```
6. Teste no app da Store com a conta admin, isolado (variáveis da seção 5 apontando para pastas de teste). O
   programa da loja procura correção a cada 20 minutos: na primeira abertura o registro diz "correção N do programa
   instalada"; na seguinte, "programa rodando com a correção de código N do servidor".
7. Com o OK do proprietário: o mesmo comando com `--canal todos` (mesma versão e mesma DLL; `todos` também atualiza
   o beta).
8. Se der problema: `sudo python3 lab/oem_homolog/publicar_correcao.py retirar <build de 9 dígitos> [beta|todos]`.
   O programa larga a correção na verificação seguinte e a próxima abertura usa a DLL da loja.

A correção vale no máximo 30 dias. Antes disso a mesma correção tem de sair numa versão da loja (seção 7).

Limites a dizer ao proprietário quando usar a camada 3: a DLL corrigida não tem assinatura de código do Windows
(Smart App Control ou antivírus podem bloquear, e aí o cliente fica com a DLL da loja até a próxima versão); quem
tem a chave privada executa código em todos os clientes daquela versão.

## 7. Levar à Microsoft Store (camada 4, e o fim de toda correção)

Só com o OK do proprietário.

1. Compilação **completa** do ramo do cliente com tudo junto, testada no Windows como na seção 5.
2. Se a versão nova muda páginas embutidas que já têm correção publicada, suba antes o último número delas em
   `FilunaVersoes.hpp` acima da correção publicada (senão o programa novo troca as páginas dele pela correção velha).
3. Montar o MSIX com número de versão maior que o publicado:
   ```bash
   gh workflow run filuna-loja-msix.yml -R abcollicelli-web/FilunaSlicer --ref filuna/main \
       -f run_id=<run da compilação completa> -f versao=X.Y.Z.0 -f enviar=false
   ```
   Baixe o artefato `FilunaSlicer_X.Y.Z.0_x64_msix` e confira que a `OrcaSlicer.dll` dentro dele é a da compilação
   testada (mesmo SHA-256) e que o `AppxManifest.xml` tem a versão nova.
4. Partner Center › Filuna Slicer › "Start update" › Packages: enviar o `.msix` (a página tira o pacote anterior
   sozinha), Save, "Submit for certification". O resto da submissão (mercados, página da loja) fica como está, a
   menos que o proprietário peça mudança. O arquivo tem mais de 200 MB: se a ferramenta do navegador não aceitar o
   arquivo, sirva-o em `127.0.0.1` e entregue-o à página pelo próprio navegador.
5. A certificação de uma atualização tem levado menos de 1 hora; a publicação é automática. A Store não atualiza o
   PC do cliente na hora: pode levar horas. Conferir a versão por mercado em
   `https://displaycatalog.mp.microsoft.com/v7.0/products/9MV6C5DJ3201?market=BR&languages=pt-BR&MS-CV=x.1`.
6. Correções de código publicadas para a versão anterior deixam de valer sozinhas (são de outra compilação). Retire-as
   do servidor para não ficar entulho.

Função nova sai com divulgação (banner na home do site e aviso na tela Início) e a página da loja tem de descrever o
que mudou (Store Policies 10.1.1).

## 8. Registrar

- O `publicacoes.log` de cada camada já guarda o que foi publicado. Não apague.
- Ao terminar, diga ao proprietário: o defeito, a causa, a camada usada, o commit, o canal em que está e o que ainda
  não foi testado.
