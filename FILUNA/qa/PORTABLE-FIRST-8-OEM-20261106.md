# FILUNA — Ciclo inicial OEM: portáteis oficiais no Windows

**Execução iniciada em 06/11/2026.** Escopo isolado na branch `filuna/oem-portable-first-qa-20261106`. Nenhuma publicação, instalação de OEM, alteração em produção ou compilação de código de fabricantes.

## Origem e regra de aprovação

O código do laboratório `lab/portable_first_qa.py` baixa os arquivos ZIP oficiais das releases GitHub, fixa o SHA-256 da própria release e só extrai após conferir o hash. Mantém os oito fabricantes na matriz. **A presença de um ZIP não certifica equivalência do Filuna**, nem confirma comunicação com impressora.

Os cinco ZIPs oficiais pinados são:

| Marca | Versão fixada | Fonte | SHA-256 do ZIP |
|---|---|---|---|
| Bambu | 02.09.01.70 | bambulab/BambuStudio | `a4c0d5f94109cb2c2c07fb02ca1f740e96b586fbf3ae557a1647eeb5503a15b8` |
| Elegoo | 1.5.3.5 | elegooofficial/ElegooSlicer | `0215cd958d6a52ae1bd33a5ad7509ecd1b156baf63fbe04333e04cf0305793e5` |
| QIDI | 2.07.02.73 | QIDITECH/QIDIStudio | `30f276c5390e4ca755c0a08d2115d6ea5feea2a2ee7ec7a3a7e3a729d0094224` |
| Snapmaker | 2.4.1 | Snapmaker/OrcaSlicer | `1f002e2168c576f800bcfd0509b5e164b5ad65a0963abc2bef8317d929d48c9d` |
| Prusa | 2.9.7 | prusa3d/PrusaSlicer | `5d4f395aeb6c344b300e0e1be37a0e1931622b642c3c3085eb19cdae211bc008` |

**Pendente de pacote ZIP oficial identificado e verificado:** Flashforge 1.7.15, Creality Print 7.3.1 e Anycubic Slicer Next. Não usar instaladores como se fossem portáteis.

## Corpus fiel do laboratório anterior

Os arquivos `fixtures/oem/{bambu,elegoo,qidi}/cube-profiled.3mf.b64` contêm somente cubos de diagnóstico gerados por CLI oficial. Estão em Base64 porque a interface de commits disponível é textual; decodificam para ZIP 3MF exato, verificado com hash original.

- Bambu A1: `212202ece5982aaceccdbe57359c070ecec3c4ddb00d4baf211c80556f50baa4`
- Elegoo CC2: `c5a6bb377548c129cb1c14d2b72ea1cc49197ecccc339da99c3c37b75ea68450`
- QIDI Q2: `2e5a8d293bac8846d547e16a0d4d04ee07e67b63ed1eea6f3b1844b38266ac04`

Origem: rodada local `run-20261106T153215Z` documentada em `FILUNA/qa/HOMOLOGACAO-LAB-8-FABRICANTES-R3-20261106.md`. Os 3MF são de cubos anteriores, **não** substituem o multicor original do proprietário nem validam o CANVAS/CFS/AMS.

## Decisão de escopo do proprietário — Bambuddy fica para depois

**Única validação deliberadamente adiada:** linha **Bambu com Bambuddy**, incluindo o contrato autenticado e a integração com impressora virtual Bambuddy. Marcar `BAMBU_BAMBUDDY = PENDENTE (ADIADO PELO PROPRIETÁRIO)`, nunca `PASS`, `FALHA` ou `HOMOLOGADO` por ausência de execução.

A decisão **não suspende** os testes do portátil Bambu Studio, G-code oficial A×B, comparação Filuna×Bambu, perfis, reconhecimento normal do dispositivo e demais verificações que possam ocorrer sem Bambuddy. Tampouco adia outros fabricantes. Todos os outros bloqueios técnicos, lacunas de corpus e falhas descobertas devem ser investigados e resolvidos agora, registrando evidência real para cada gate; não promovê-los a PASS sem prova.

Não iniciar ou configurar testes Bambuddy, nem tentar autenticação ou ações na impressora virtual nesta fase. Retomar Bambuddy somente após decisão expressa do proprietário.

## Critérios de QA e gates que NÃO podem ser confundidos

1. `PACKAGE_SHA256`: ZIP oficial pinado e íntegro.
2. `PORTABLE_EXE`: executável do ZIP detectado e dependências mantidas na pasta.
3. `PROFILED_FIXTURE`: arquivo 3MF arquivado, com SHA correto.
4. `OFFICIAL_A_B`: duas execuções independentes do mesmo portátil sobre o mesmo 3MF, em diretórios diferentes, exit 0, G-code presente e SHA-256 bruto idêntico. Se houver diferença, registrar **FALHA** antes de autorizar normalização de linhas comprovadamente variáveis.
5. `FILUNA_PARITY`: somente após produzir G-code pelo **aplicativo Filuna**, com execução e presets comprovados, comparar com OEM. O uso do próprio executável OEM duas vezes NÃO conta como paridade Filuna.
6. `NETWORK`: somente após Dispositivos → Preparar → Fatiar → Enviar com SDK real e logs/ACK de simulador ou hardware; provas experimentais de rede não aprovam o aplicativo.
7. `HOMOLOGACAO`: exige todos os gates aplicáveis, decisão explícita do proprietário e prova de todas as capacidades previstas para o modelo.

## DECISÃO DO PROPRIETÁRIO — única validação intencionalmente adiada

**06/11/2026 — Escopo da espera:** somente a validação **autenticada da linha Bambu por meio do Bambuddy** será executada em etapa posterior, conforme decisão do proprietário. O laboratório não deverá tentar ou simular essa autenticação para promovê-la a PASS prematuramente.

**Não adiar os demais trabalhos:** continuam agora as provas portáteis, os perfis, o G-code oficial A×B, a paridade real Filuna×OEM, a identificação e o registro dos dispositivos, os testes com simuladores adequados e a integração das demais marcas, incluindo os testes Bambu que não dependem da autenticação com Bambuddy.

**Sem falsos aprovados:** problemas concretos como pacote indisponível, 3MF multicor original ainda ausente, teste de rede não realizado ou função não integrada continuam registrados como **FALHA/PENDENTE técnico**, com solução buscada nesta fase. Eles não constituem novas etapas propositalmente adiadas nem equivalem a homologação. A aprovação final de cada fabricante depende de evidência efetiva e da decisão do proprietário.

**Manifesto:** `fabricantes/bambu/vendor.yaml → flags.defer_bambuddy_authenticated=true`. O gate separado `BAMBUDDY_AUTH` permanece `PENDENTE`; os outros gates Bambu T3–T9 não recebem mais a etiqueta indevida `OWNER_DEFERRED_BAMBUDDY_AUTHENTICATED`.

## GitHub Actions

Workflow: `.github/workflows/filuna-oem-portable-first.yml`. Runner Windows 2022, apenas Python e binários ZIP oficiais, sem CMake/MSVC/compilar fatiadores. Artefato `filuna-portable-first-five-official-eight-brands`: relatório `RESUMO.md`, `results.csv`, `results.json`, lock de cada fabricante, logs A/B e hashes. Arquivos ZIP de centenas de MB permanecem apenas no cache do GitHub Actions, não entram no artefato.

### Limites explícitos

A homologação integral dos oito fabricantes não é possível só com ZIPs: requer módulos Filuna funcionando, modelos e firmware corretos, interfaces de dispositivo, casos multicor oficiais e provas de rede. Não divulgar como recurso homologado nem publicar produto enquanto isso faltar.

## Continuação de execução — evidências após a primeira rodada aprovada

### Windows: execução real #42308303399 — SUCCESS

- Bambu A1 (portátil 02.09.01.70): duas execuções do oficial em diretórios distintos, G-code com 374.507 bytes, mesmo SHA bruto `ac7731e7653bd9a46531217307d745a1f1996d20bb14e065a50bf691d7db96d5`.
- Elegoo CC2 (portátil 1.5.3.5): duas execuções independentes, 208.003 bytes, mesmo SHA bruto nessa rodada `59d57a0d0ddeefc04c850eb9d310f836b5969e3c9193029956f9b9ac8195802b`.
- QIDI Q2 (portátil 2.07.02.73): duas execuções independentes, 258.131 bytes, mesmo SHA bruto `375e1e3334391267d5079e95877d5eeb2fcc5845f5897a93c34c19a1f1fdb5c0`.
- Snapmaker 2.4.1: executável portátil **exato** `snapmaker-orca.exe` confirmado e corrigido no seletor. Prusa 2.9.7: `prusa-slicer-console.exe` encontrado. Ambos ainda precisam de 3MFs efetivos do modelo para fatiamento A/B específico.
- Prova: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42308303399/artifacts/13548070554

### Elegoo: prova posterior de metadado variável, sem alterar comandos de impressão

A execução Windows #42308506808 produziu duas saídas Elegoo com **9.131 linhas cada**, e exatamente **uma diferença**, na linha 2 do cabeçalho:

```text
A: ; generated by ElegooSlicer 1.5.3.5 on 2026-11-06 at 22:58:48
B: ; generated by ElegooSlicer 1.5.3.5 on 2026-11-06 at 22:58:49
```

O programa de QA passa a permitir **somente** essa linha e esse regex para a versão 1.5.3.5, e **apenas se a alteração for observada entre as duas saídas reais do mesmo fatiador oficial**. Os hashes brutos dessa rodada foram, respectivamente, `e6a09a04df3603d8b60ca78acd2779c6562ccc17959aeded238249b697db0f2b` e `3de0aa7b929994071d28ed8d0cad3b66fbbb2da8dcb898e85792dc7e5e7bb4df`. Nenhuma diferença funcional é permitida. A prova completa é gerada em `official-A-B-proof.json`; testes unitários do laboratório verificam a rejeição de linhas extras, outros modelos/versionamentos e mudanças de movimento/extrusão.

### Creality K2 Plus: avanço com AppImage Linux oficial e perfis anteriores

A release oficial `CrealityPrint-V7.3.1.6378-x86_64-Release.AppImage` foi fixada pelo SHA-256 do ativo do GitHub `f8c8dbf18a6b0d5878961b24490fc20958eefe2f03f6a89f524cb9b21e55a107` (191.511.032 bytes). O runner baixa o AppImage, verifica a assinatura de integridade SHA, extrai com `--appimage-extract` sem instalar/compilar, e tenta fatiar com os três perfis oficiais K2 Plus recuperados do QA anterior: `machine.json`, `process.json` e `filament.json`, além do cubo original `cube20.stl`.

- Execução #42308643378: hash e extração PASS; execução bloqueada pela dependência de runtime ausente `libEGL.so.1`.
- Execução #42308781587: hash e extração PASS; execução bloqueada por `libgstvideo-1.0.so.0`.
- As dependências passaram a ser providas via pacotes de runtime do Ubuntu, sem recompilar o OEM. O diagnóstico `ldd-CrealityPrint.log` foi adicionado.
- **Ainda não existe PASS confirmado de fatiamento A/B Creality nesta etapa**, nem paridade Filuna. Executar e inspecionar os próximos artefatos antes de qualquer conclusão.

O laboratório continua com status real de cada gate. **A única validação adiada por decisão do proprietário é a autenticada Bambu + Bambuddy.** Falhas técnicas das demais linhas são registradas e trabalhadas; nenhuma é marcada como homologada sem prova.

### Resultado comprovado: Creality K2 Plus Linux AppImage (#42309346648)

**SUCCESS** no Ubuntu 24.04 com **AppImage oficial da Creality** `v7.3.1.6378`, executado em modo CLI sob Xvfb, com perfis originais de K2 Plus. Sem compilação ou instalação. A/B independentes em diretórios distintos:

- Cada G-code: **273.858 bytes**, códigos de saída 0/0.
- SHA-256 bruto A: `b6452d5d2adca0906a9c55e2b1b9d476ddec42448d7b231c6a963e835b98cbf0`.
- SHA-256 bruto B: `0f45246939254c96a355701367b5abc8f0256ea188d965b91fb8037bfd4a13bf`.
- **Única diferença A×B: linha 7** `; creality_uuid: <uuid>` com identificadores diferentes; a comparação conservadora não removeu nem ignorou qualquer movimento, temperatura ou extrusão.
- SHA-256 estritamente normalizado A=B: `08b599d64c88db4f42d44ad8ec739194a052af58f7bedd0d397447e8ff47a968`.
- Artefato integral: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42309346648/artifacts/13548226124

**Significado do PASS:** fatiamento oficial Creality reproduzível, não equivalência com Filuna nem prova da integração de rede CFS. G-code Filuna ainda precisa rodar separadamente.

### Flashforge 1.7.15: extração controlada de pacote oficial

O pacote oficial `Flash_Studio_Desktop_1.7.15_x64.exe` teve SHA-256 confirmado `b3cc8804e0426e3f4bb2c0953667e27512c0047af51099c0e1af5c5d1b3ef8f3`. O primeiro teste com `innoextract` resultou `Not a supported Inno Setup installer!` (artefato #42309552276). O laboratório testa em seguida **7-Zip** em diretório isolado, sem instalar o aplicativo nem compilar. A extração não será rotulada como release portátil do fabricante; a capacidade de executar o binário, possuir os resources e produzir G-code com perfil oficial exige provas adicionais.
