# FILUNA — REGRA CANÔNICA: MÓDULOS DE FABRICANTE CLONADOS INTEGRALMENTE E CARREGADOS SOB DEMANDA

**Decisão expressa do proprietário:** 10/11/2026 (Europe/Rome). Tomada sob responsabilidade e risco do proprietário.
**Alcance:** todos os fabricantes e modelos do Filuna Slicer.
**Precedência:** onde esta regra conflitar com documentos anteriores, esta regra prevalece (ver §7).

> **13/11/2026:** §2 item 1 (motor clonado para dentro do Filuna) e §8 foram substituídos por `REGRA-CANONICA-ARQUITETURA-V2-MOTORES-HEADLESS-DISPOSITIVOS-20261113.md` (motor = fatiador oficial headless em processo separado; Anycubic = Orca nativo). O restante desta regra continua válido.

## 1. Princípio

O Filuna Slicer é um fork AGPL do OrcaSlicer. Cada fatiador oficial de fabricante é também um fork aberto da mesma linhagem (Slic3r → PrusaSlicer → Bambu Studio → OrcaSlicer), porém **cada um parou em uma versão diferente e alterou o próprio motor**. Por isso o G-code oficial só é reproduzido com o motor do próprio fabricante.

O Filuna **não reescreve nem adapta** o comportamento do fabricante: **clona integralmente**, a partir do código-fonte oficial em versão fixada, tudo o que é responsável por impressão, e carrega esse conjunto quando a impressora é selecionada, como se o cliente estivesse usando o fatiador oficial.

## 2. O que compõe o módulo de um fabricante

Cada módulo fica em `FILUNA/fabricantes/<id>/` e contém, clonados da fonte oficial fixada por commit:

1. **Motor** — biblioteca de fatiamento e geração de G-code do fabricante (`libslic3r` dele), sem a interface.
2. **Perfis** — máquina, filamento e processo oficiais.
3. **Rede** — descoberta automática, conexão e envio.
4. **Controle** — comandos, status e monitoramento do equipamento.
5. **Câmera**.
6. **Sistema de cores** — AMS, Canvas, ACE Pro e equivalentes.

Fica **fora** do módulo:
- a interface e o restante do fatiador do fabricante (a interface é a do Filuna, uma só);
- o que o fabricante não publica como código aberto (plugins de rede proprietários, serviços de nuvem). Nesses casos usa-se o protocolo local aberto disponível. Nada proprietário é empacotado.

Cada módulo declara em `module.json` a origem, o commit, a licença, os hashes de árvore e a lista de arquivos por componente. Nenhum arquivo clonado é editado à mão; integração com o Filuna é feita por código Filuna separado.

## 3. Distribuição: base leve e módulos no servidor

- O instalador contém **somente a base Filuna/Orca**. Ela funciona sem login e sem assinatura, como Orca livre com a personalização Filuna.
- Os módulos de fabricante ficam no **servidor Filuna** e são baixados **sob demanda**, apenas das marcas que o cliente escolhe. O instalador não carrega vários fatiadores.
- Cada pacote de módulo tem versão, origem, hash e assinatura verificados antes de carregar; fica em cache local e funciona offline depois de instalado.
- O código-fonte correspondente a cada módulo distribuído permanece público (AGPL).

## 4. Fluxo do cliente

1. Cliente baixa e usa a base Filuna.
2. Cliente logado, com plano liberado, abre **Dispositivos** e escolhe o fabricante.
3. O Filuna mostra "Aguarde, estamos atualizando a base para sua impressora" e baixa/carrega o módulo daquela marca.
4. Com o módulo carregado, a **busca em rede começa automaticamente**, usando o código de descoberta clonado do fatiador oficial.
5. Cliente clica na impressora encontrada e tem **câmera, acesso e controle**.
6. Em **Preparar**, ao selecionar a impressora, carregam-se perfil de máquina, filamentos, processos e o motor do fabricante. O fatiamento é nativo.
7. Cliente com duas ou mais marcas: ao trocar de impressora em Preparar, **o módulo ativo é trocado internamente**.
8. Nenhuma impressão começa sem confirmação explícita do usuário (regra 5 do README, mantida).

## 5. Homologação

Um módulo é **homologado** quando, de forma automática e registrada:

1. **G-code idêntico** — o G-code gerado pelo Filuna com o módulo é igual ao do fatiador oficial na mesma versão, com as mesmas entradas. Normaliza-se somente a linha de cabeçalho com data/gerador. Quando o próprio fatiador oficial produz mais de uma variante para a mesma entrada, igualdade com qualquer variante oficial registrada é aceita.
2. **Nativo** — descoberta em rede, conexão, preparo, envio e controle são executados pelo código clonado do módulo, não por substituto.

A **impressão física não bloqueia** a homologação nem o projeto. Ela continua sendo registrada separadamente quando ocorrer.

## 6. Segurança e limites que permanecem

- G-code do fabricante é imutável: não se pós-processa nem se insere movimento.
- Riscos detectados no próprio G-code oficial são mostrados ao usuário; não são "corrigidos" alterando o resultado do fabricante.
- Sem plugins proprietários empacotados; sem controle de impressoras pelo administrador Filuna.
- Merge, release e publicação seguem dependendo de ordem do proprietário.

## 7. O que esta regra substitui

- **Motor Orca único com overlays/ajustes por fabricante** (`ARQUITETURA-ORCA-UNICO-FABRICANTES-OVERLAY-2026-11-06.md` e trechos equivalentes): substituído por **motor do próprio fabricante dentro do módulo**. Evidência: o motor do Snapmaker Orca 2.4.1 difere do motor do Filuna em 571 arquivos de `libslic3r`; ajustes pontuais chegaram a similaridade ~0,14 (ver `SNAPMAKER_U1_NATIVE_PARITY_LEDGER_2026-11-08.md`).
- **Homologação física como condição** (`REGRA-FASES-HOMOLOGACAO-INTERNA-BETA-FISICA-20261107.md`, três execuções físicas): deixa de bloquear; o critério é o do §5.
- Os demais documentos permanecem válidos no que não conflitar.

## 8. Ordem de implantação

1. **Snapmaker U1** como primeiro módulo (fonte `Snapmaker/OrcaSlicer` v2.4.1).
2. Mecanismo de carga e troca de módulo no Filuna (Dispositivos e Preparar).
3. Pacote no servidor e download sob demanda.
4. Demais fabricantes, um módulo por vez, no mesmo formato.
