# FILUNA — REGRA CANÔNICA V2: MOTORES POR PROCESSO (CLI HEADLESS), SÉRIE DE PATCHES, TESTE DOURADO, ROBÔ DE ATUALIZAÇÃO E FLUXO DE DISPOSITIVOS

**Decisão expressa do proprietário:** 13/11/2026, 18:53 (Europe/Rome). Tomada sob responsabilidade e risco do proprietário.
**Alcance:** todos os fabricantes e modelos do Filuna Slicer; Dispositivos, Preparar, servidor Filuna e app Android.
**Precedência:** onde conflitar com `REGRA-CANONICA-MODULOS-FABRICANTE-SOB-DEMANDA-20261110.md` e anteriores, esta regra prevalece (§9). O que não conflita continua valendo (critério de homologação por G-code idêntico, nada proprietário empacotado, nenhuma impressão sem confirmação do usuário, AGPL).

## 0. Por que mudar (evidência de 10–13/11)

- Cada fatiador oficial parou numa versão diferente do Orca e alterou o motor (Snapmaker 2.4.1 difere do Filuna em 571 arquivos de `libslic3r`; Bambu, QIDI, Creality, Flashforge, Prusa idem). Trazer o motor de cada um **para dentro** do Filuna faz toda atualização do Orca ou de um fabricante recompilar e re-homologar tudo (≈2 h por compilação, efeito dominó).
- Compilar cada fatiador oficial **inteiro, na tag exata**, já é o que a homologação faz no servidor (receitas em `build/compilar.sh`; 7/8 com G-code idêntico). O que falta é só o uso: em vez de clonar o motor para o Filuna, **o Filuna chama o binário oficial headless**.

## 1. Princípio: motor por fabricante = processo separado

1. **Filuna Slicer** = OrcaSlicer (base fixada em `UPSTREAM_PIN.md`, hoje 2.5.x) + camada Filuna: interface, projeto, conta, estoque, Farm, Dispositivos, Preparar, autoajuda, pt-BR, plugins, auto-update.
2. **Módulo de fabricante** = o fatiador oficial do fabricante compilado **headless** (só CLI: `--slice`, `--export-gcode`, `--load-settings`, `--load-filaments`, `--export-3mf`), na **tag exata** publicada pelo fabricante, **+ perfis oficiais + plugin de rede/controle/câmera + sistema de cores**. Empacotado como `<fabricante>-<versao>.zip` (`snapmaker-2.4.1.zip`, `elegoo-1.5.3.5.zip`, …) e servido por `/api/slicer/v1/modulos/` — exatamente o pacote que o Filuna já baixa ao selecionar a impressora (`FilunaModuleStore`).
3. **Ponte 3MF ↔ CLI ↔ preview:** ao fatiar com uma impressora de marca X selecionada, o Filuna exporta o projeto (3MF com objetos, cores por objeto/face, perfis de máquina/filamento/processo da marca), chama o CLI do módulo num processo filho, lê o G-code de volta e carrega no visualizador de G-code do Orca (o Orca já abre G-code externo). Envio/impressão usam o G-code **sem pós-processamento** (G-code do fabricante é imutável).
4. **Exceção declarada por política:** marca sem fonte do motor oficial (hoje **Anycubic**, `engine_policy.modo = ORCA_NATIVO`): o motor é o próprio Orca da base, perfis do Orca, rede/controle/câmera/cores pela camada Filuna; o catálogo declara "G-code não idêntico ao oficial". A ponte não é usada; o fatiamento é interno.
5. **Consequência:** atualizar o Orca (interface) não toca motor nenhum; atualizar um fabricante não toca o Orca nem os outros sete. Fim do efeito dominó.

## 2. Zero edição nos arquivos do Orca — série de patches

1. Tudo que é Filuna fica em `src/filuna/` (C++), `src/slic3r/Filuna/` (já existe: `FilunaFoundation`), `src/slic3r/Utils/Filuna*` (agentes de rede já existentes) e `resources/filuna/` (telas, tema, traduções pt-BR, imagens, autoajuda).
2. Ligação por **poucos pontos de gancho** no Orca (menu superior, aba Dispositivos, seleção de impressora em Preparar, `NetworkAgentFactory`, splash/nome/instalador). Cada toque em arquivo do Orca vira **patch nomeado** em `FILUNA/patches/NNNN-<assunto>.patch`, gerado com `git format-patch` sobre a tag fixada do Orca: `0001-identidade-filuna.patch`, `0002-tema-visual-c.patch`, `0003-login-filuna.patch`, `0004-gancho-dispositivos.patch`, `0005-gancho-preparar-troca-de-modulo.patch`, `0006-network-agent-factory-filuna.patch`, …
3. **Update do Orca** = `git rebase` da série sobre a tag nova (`UPDATE-PLAYBOOK.md`): patch que falha é identificado em minutos, não em 2 h de compilação. **Métrica a vigiar: tamanho da série** (número de patches e linhas tocadas no Orca). Toda nova função deve nascer em `src/filuna/`, nunca como edição espalhada.
4. O ramo `filuna/v1-base-unificada` passa a ser **gerado**: tag do Orca + série de patches + `src/filuna` + `resources/filuna`. O script `FILUNA/scripts/montar-base.sh` reconstrói o ramo do zero; se não reconstruir, a série está quebrada.

## 3. Teste dourado de G-code (homologação automática)

1. **Dourado** por impressora/bico homologado = conjunto fixo de STLs (`fixtures/cube20.stl` + fixture de 4 cores + os que forem acrescentados) + perfis + **G-code gerado pelo fatiador oficial** na versão fixada. Hoje já existe em `res/paridade-<fab>/casos/*/cubo-oficial-A` e `cores-oficial-A` (e variantes A/B quando o oficial não é determinístico): vira `FILUNA/dourado/<fab>/<versao>/<maquina>/<caso>.gcode` (ou pacote no servidor, pelo tamanho), com `prova.json`.
2. A cada mudança (Orca, patch, módulo, perfil), o CI fatia tudo de novo pelo caminho real do cliente (base Filuna → ponte → CLI do módulo) e compara com o dourado com a normalização já existente em `lab/oem_homolog/paridade.py` (cabeçalho/data/versão, ids de objeto, `IDENTICO_POR_CAMADA` quando o oficial varia).
3. **Verde** = homologação mantida, sem ninguém abrir nada. **Vermelho** = lista exata de impressoras que quebraram. Substitui a homologação manual.
4. Para `ORCA_NATIVO` (Anycubic): o dourado é o **G-code do próprio Filuna** na versão aprovada (regressão), nunca o do fabricante.

## 4. Robô de atualização (GitHub Actions agendado, semanal)

1. Verifica tags novas do Orca e de cada fabricante (fontes em `FILUNA/fabricantes/<id>/module.json` → `source`).
2. Orca novo: `rebase` da série de patches → compila base → testes dourados (todas as marcas, porque a ponte é da base).
3. Fabricante novo: rebuild do CLI headless com a receita da marca (`build/compilar.sh` migra para `FILUNA/fabricantes/<id>/receita.sh`, Linux e Windows) → sincroniza perfis (§5) → testes dourados da marca, gerando **novo dourado** com o oficial novo e comparando com o módulo novo.
4. Abre um PR com o relatório: "Orca 2.5.1: 11/11 patches OK, 94/94 G-codes iguais" ou "Creality 6.2: 3 impressoras divergem (lista)". O proprietário só decide: aceitar e publicar o pacote no servidor (`publicar_modulos.py`), ou olhar o que divergiu.

## 5. Perfis como dados, com mapeamento declarativo

1. Perfis vêm dos repositórios dos fabricantes por `lab/oem_homolog/perfis.py` (sincronização), não copiados à mão. Quando a fonte publicada difere do binário oficial, valem os do binário (regra já aplicada: `HOMOLOGADO_DO_BINARIO_OFICIAL`).
2. Diferenças de chave entre versões (campo renomeado, valor padrão novo, faixa alterada — ex.: `retraction_distances_when_cut`) ficam em `FILUNA/fabricantes/<id>/mapping.json`, aplicado pelo sincronizador, não em código.

## 6. Dispositivos e Preparar — fluxo obrigatório do cliente

1. **Dispositivos** sem impressora: tela de configuração com os **fabricantes pela logo oficial** (as imagens do Orca em `resources/profiles/<Vendor>.png`/`images`; não inventar). Clicar na logo abre a **lista de modelos homologados internamente** (do catálogo assinado: `homologacao.*` e, para `ORCA_NATIVO`, `politica_motor.impressoras_disponiveis`). Modelo fora da lista: "Impressora não disponível: este modelo … ainda não tem perfil homologado no Filuna" (texto do catálogo), sem liberar.
2. Ao selecionar o modelo: carrega a **foto oficial** da impressora (cover do Orca: `resources/profiles/<Vendor>/<modelo>_cover.png`), e o Filuna **baixa o módulo do servidor** (`FilunaModuleStore::ensure_module_for_printer`, pacote assinado, cache local, offline depois).
3. **Verificação na rede LAN** começa sozinha, pelo protocolo nativo da marca (ordem já definida em `HOMOLOGACAO-01…` §rede: descoberta → conexão → estado). Se falhar, o sistema **tenta por outros meios** (IP manual, segundo protocolo da marca, mDNS/SSDP, porta alternativa). **Só se nenhum meio do módulo conectar**, o Filuna cria a **ponte LAN**: assistente passo a passo ensinando o cliente a ligar o modo LAN/código na impressora (com imagens do próprio fabricante), tudo prático.
4. Conectada: aparece em **Preparar** já selecionada, com perfis de máquina/filamento/processo da marca. Com 2+ impressoras, a troca em Preparar **troca o módulo internamente** (motor via ponte CLI ou Orca nativo, perfis, rede) sem passo manual.
5. **Card da impressora** em Dispositivos: foto oficial, nome, modelo, estado (**ligada / imprimindo (progresso) / livre / desligada / sem rede**), **cores do sistema de cores** (AMS, CFS, ACE Pro, Canvas…) com os slots, temperaturas, botão de câmera. No topo, botão **"Conectar nova impressora"** que reabre o passo 1.
6. **Tudo interligado ao servidor**: impressora, estado, cores e histórico sincronizam com a conta (`/api/slicer/v1/dispositivos`), e o servidor replica **automaticamente ao app Android** (mesma API). Offline, o Filuna continua operando (regra dos 3 dias) e sincroniza ao voltar.

## 7. O que muda no que já está decidido

- A "pasta com clonagem integral" por fabricante (`FILUNA/fabricantes/<id>/`) continua, mas como **submódulo git apontando para a tag oficial + patches mínimos de compilação (cola) + receita headless** — não como cópia solta nem como motor dentro do Filuna.
- O instalador fica pequeno naturalmente (motores vêm do servidor) — meta já declarada.
- Custo honesto: a ponte 3MF ↔ CLI ↔ preview dá trabalho **uma vez**; depois todo update é mecânico. É o oposto do caminho atual, barato hoje e caro para sempre.
- Os agentes de rede Filuna (`FilunaBambuLanAgent`, `FilunaElegooCC2/SDCPAgent`, `FilunaFlashforgeAgent`, `FilunaPrusaLinkAgent`, `MoonrakerPrinterAgent` para família Klipper incl. Anycubic/ACE Pro) ficam **na base** na v1 (já compilam e passam nos emuladores); viram plugin do módulo em etapa posterior, sem mudar o contrato.

## 8. Plano de implementação (ordem, cada passo verificável)

**Fase A — base reproduzível (1–2 dias)**
1. Fixar o Orca em tag (`UPSTREAM_PIN.md`: v2.5.0). Extrair de `filuna/v1-base-unificada` a série de patches (`git format-patch v2.5.0..` filtrando só arquivos do Orca; o resto vai para `src/filuna`/`resources/filuna`). Verificação: `montar-base.sh` reconstrói o ramo e compila (GitHub Actions `filuna-base-windows.yml` + Linux).
2. Métrica inicial da série registrada no registro.

**Fase B — catálogo e Dispositivos (2–3 dias)**
3. `catalogo.json` (já assinado) ganha para cada marca: `logo`, por modelo `cover`, `disponivel`, `mensagem_indisponivel`. `publicar_modulos.py` já emite `politica_motor`; acrescentar `modelos[]` com cover do Orca.
4. Tela Dispositivos (`FilunaPanel`, WebView já existente): logos → modelos → foto → download do módulo → busca LAN → fallback → ponte LAN assistida → card. Estados e cores vindos de `NetworkAgent` (já expõe cores por callback). Botão "Conectar nova impressora".
5. Preparar: seleção de impressora troca módulo (`FilunaModuleSelection` + `FilunaOEMOverlay` já existentes) e perfis; para `ORCA_NATIVO`, só perfis.

**Fase C — ponte CLI (3–5 dias, uma marca piloto: Snapmaker, depois as outras pelo mesmo contrato)**
6. Pacote do módulo: `cli/` (binário headless Windows da tag oficial, compilado pela receita no GitHub Actions `windows-latest`, sem interface), `profiles/`, `manifest.json` (origem, commit, hashes, assinatura). Receitas Linux já existem em `build/compilar.sh`; a versão Windows reutiliza o `build_release_vs2022.bat` de cada fork com alvo só CLI.
7. `src/filuna/bridge/`: exportar 3MF do projeto atual + perfis → `cli --slice 0 --export-gcode` em processo filho (timeout, cancelamento, stdout/stderr para o log) → carregar G-code no visualizador → enviar pelo agente de rede. Erros do CLI aparecem ao usuário com o texto do fabricante.
8. Teste dourado pelo caminho real (base → ponte → CLI) nas marcas com dourado; verde obrigatório antes de publicar módulo.

**Fase D — robô e perfis (1–2 dias)**
9. Workflow semanal (§4) + `mapping.json` por marca (§5).

**Fase E — servidor e Android (paralelo, 2–3 dias)**
10. `/api/slicer/v1/dispositivos` (estado, cores, histórico por conta) + replicação ao app Android.

## 9. Substituições

- `REGRA-CANONICA-MODULOS-FABRICANTE-SOB-DEMANDA-20261110.md` §2 item 1 ("motor … clonado para dentro") e §8: substituídos por §1 e §8 desta regra. §3, §4, §5 e §6 daquela regra continuam válidos.
- `ARQUITETURA-ORCA-UNICO-FABRICANTES-OVERLAY-2026-11-06.md` (overlays sobre um motor Orca): continua substituída, exceto como descrição do modo `ORCA_NATIVO`.
- Critério de homologação (G-code idêntico ao oficial; nativo em rede) e a régua: mantidos; passam a rodar pelo caminho real do cliente.

---

# PRIMEIRA COMPILAÇÃO PARA OS TESTES FÍSICOS — "v1-teste-fisico"

**Objetivo (decisão do proprietário, 18:56):** um instalador Windows do Filuna Slicer **com os 8 fabricantes no estado homologado de 13/11**, para iniciar os testes físicos — a começar pela **Anycubic Kobra 3 V2 do proprietário**.

**Placar que entra na v1-teste-fisico (régua de 13/11/2026, `/var/lib/filuna-v2/modulos/catalogo.json`):**

| Fabricante | Situação | Motor na v1 |
|---|---|---|
| Snapmaker 2.4.1 | ✅ Homologado (76 máquinas) | CLI oficial headless (ponte) |
| Elegoo 1.5.3.5 | ✅ Homologado (73) | CLI oficial headless (ponte) |
| Flashforge 1.7.9 | ✅ Homologado (42) | CLI oficial headless (ponte) |
| Bambu 02.09.01.70 | ✅ Homologado (60) | CLI oficial headless (ponte) |
| QIDI 02.07.02.73 | ✅ Homologado (40) | CLI oficial headless (ponte) |
| Prusa 2.9.7 | ✅ Homologado (217) | CLI oficial headless (ponte) |
| Creality 7.3.1.6378 | Motor homologado: 97 de 99 máquinas pelo critério "comandos iguais" (decisão do proprietário); 2 fora, declaradas | CLI oficial headless (ponte) |
| Anycubic | Critério "motor Orca nativo" (13/11): 21 modelos disponíveis; Kobra 3 Max V2 e Kobra 4 "não disponível" | Orca da base (`ORCA_NATIVO`) |

Cada linha entra **exatamente como está no catálogo assinado** (estado, máquinas, exceções e notas); a v1 não re-homologa nada — consome o catálogo.

**Escopo da v1-teste-fisico**
1. Base Orca 2.5.0 + série de patches (identidade Filuna, tema Direção Visual C, pt-BR, login Filuna/plano, menu superior) + `src/filuna` + `resources/filuna`.
2. Agentes de rede Filuna na base (Bambu LAN, Elegoo CC2/SDCP, Flashforge, PrusaLink, Moonraker/Klipper com ACE Pro via `lane_data`, Snapmaker, QIDI, Creality).
3. **Ponte CLI (Fase C) para as 7 marcas com motor oficial**: binários headless Windows compilados pela receita de cada marca no GitHub Actions (`windows-latest`), empacotados em `<fab>-<versao>.zip` com perfis e manifesto assinado; teste dourado pelo caminho real antes de publicar.
4. Dispositivos conforme §6 (logos → modelos homologados → foto → módulo → LAN → fallback → ponte assistida → card com estado e cores → "Conectar nova impressora").
5. Preparar com troca automática de impressora/perfil/motor; para Anycubic, perfis do Orca e motor interno.
6. Catálogo assinado do servidor com `politica_motor` (publicado em 13/11) e `modelos[]`.
7. Sincronização de dispositivos com a conta (mínimo: lista, estado, cores) → Android.

**Fora da v1-teste-fisico (entram na v1 completa):** robô semanal, `mapping.json`, Farm no Slicer, paramétricos.

**Ordem de execução para a v1-teste-fisico:** Fase A → Fase C (piloto Snapmaker, depois as 6 restantes pelo mesmo contrato) → Fase B → Fase E; Fase D depois. Estimativa honesta: 8–12 dias de trabalho de implementação; a compilação final sai do GitHub Actions e é validada no PC do proprietário antes do teste físico.

**Roteiro do teste físico (Kobra 3 V2, modo LAN ligado na impressora):**
1. Dispositivos → logo Anycubic → "Kobra 3 V2" → foto → módulo baixado → impressora encontrada na LAN (se não: IP manual; se não: ponte assistida).
2. Card: ligada/livre, temperaturas, câmera, ACE Pro com 4 cores.
3. Preparar: cubo + fixture de 4 cores com perfil Orca "Anycubic Kobra 3 V2 0.4 nozzle" → fatiar → enviar → iniciar → pausar → retomar → parar → impressão completa. Registrar G-code enviado (dourado de regressão do modo ORCA_NATIVO).
4. Repetir com uma segunda marca disponível fisicamente para provar a troca em Preparar.
5. Resultado vai para `FILUNA/fabricantes/anycubic/module.json` → `engine_policy.teste_fisico` e para o registro; o que falhar é correção do módulo de rede/UI, não do motor.

**Critério de aceite da compilação:** o print de cada tela bate com a referência Visual C; nenhum texto em inglês; o fluxo §6 completo sem passo manual fora do assistente; catálogo verificado por assinatura; `montar-base.sh` reconstrói o ramo.
