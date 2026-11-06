# FILUNA — Bambu A1 mini: E5/E8 e paridade EXATA entre sistemas (06/11/2026)

**Decisão do proprietário:** executar comparações A → B → C nesta ordem, usando o mesmo projeto salvo pelo Bambu Studio da versão `02.09.01.70` e idênticos comandos/placa. Sem tolerâncias. Comparar SHA-256 depois de remover *somente* data/hora e identidade/versão do gerador do `HEADER_BLOCK`; se diferença, primeira linha divergente.

## Origem real da referência existente
- `/var/tmp/filuna-qa-wine/qa-a1mini-evolutions-bambu-official/plate_1.gcode`: **Bambu Studio oficial WINDOWS (bambu-studio.exe) v02.09.01.70 rodando no Ubuntu Linux via WINE e XVFB, em modo CLI**. O G-code **não foi emitido por Bambu Studio Linux nativo e não foi produzido pela GUI**.
- SHA-256 RAW oficial: `321a29de1290a42edf785674e789b9d69d41c5ba979a69144faf081aa69372df`. 73.159.248 bytes; A1 mini 0.4 mm; 701 camadas; tempo estimado 8h45min38s; filamento 160,74 g.
- **FONTE 3MF ORIGINAL:** `/srv/site/incoming/filuna-qa-a1-mini-original.3mf`. O campo `<metadata name="Application">` dentro de `3D/3dmodel.model` registra `BambuStudio-02.07.01.57`, **NÃO 02.09.01.70**. O projeto tem A1 mini e NÃO deve ser editado para P2S, nem ter perfis regravados manualmente. Para o teste A/B/C do proprietário, precisa ser reaberto e **salvo nativamente pelo Bambu Studio 02.09.01.70**, pela própria versão, preservando a A1 mini e suas definições. Sem esse projeto resalvo oficialmente, este corpus não satisfaz estritamente a condição de origem pedida.
- Versão oficial do código para rebuild: tag `bambulab/BambuStudio/v02.09.01.70` commit `56a57d00a591360cf41bf21383915ee5bf357be5`; checkout imutável de referência no Ubuntu: `/srv/site/incoming/filuna-bambu-v0290170-source`.
- Ubuntu/Wine pode servir de **ambiente de execução Windows** para os dois executáveis `exe`, desde que compiler/ABI/arquitetura Windows sejam os da referência. Um binário construído com GCC/Linux NÃO é comparável à compilação oficial Windows como evidência rigorosa de reprodução.

## Toolchain e flags extraídos DA MESMA TAG (não inventados)
Fonte exata: `.github/workflows/build_bambu.yml` linhas ~115–134, `.github/workflows/build_deps.yml` linhas ~59–70, `BuildLinux.sh` linhas ~194–212, `CMakeLists.txt` da tag.
- **Windows x64 oficial:** `windows-latest`, `microsoft/setup-msbuild@v2`, gerador CMake **Visual Studio 18 2026**, arquitetura **X64**, config **Release**, `-DBBL_RELEASE_TO_PUBLIC=1`, `-DBBL_INTERNAL_TESTING=0`, `-DCMAKE_BUILD_TYPE=Release`, `-DWIN10SDK_PATH="C:/Program Files (x86)/Windows Kits/10/Include/10.0.22000.0"`, `-DCMAKE_PREFIX_PATH=deps/build/BambuStudio_dep/usr/local`, `-DCMAKE_INSTALL_PREFIX=../install-dir`. Build: `cmake --build . --target install --config Release -- -m`.
- **Dependências Windows oficiais:** `cmake ../ -G "Visual Studio 18 2026" -A x64 -DDESTDIR="./BambuStudio_dep" -DCMAKE_BUILD_TYPE=Release -DDEP_DEBUG=OFF`, `msbuild /m ALL_BUILD.vcxproj`; Strawberry Perl, pkgconfiglite, NSIS conforme workflow. Não misturar dependências do sistema.
- **Linux oficial (não usar para comparar com .exe Windows):** `./BuildLinux.sh -isfr`; flags `-DCMAKE_BUILD_TYPE=Release -DBBL_RELEASE_TO_PUBLIC=1 -DBBL_INTERNAL_TESTING=0 -DSLIC3R_STATIC=1` com GCC da própria imagem Linux. A tag NÃO fixa uma versão GCC numérica única em `BuildLinux.sh`; reportar a do ambiente real se fizer build Linux, mas nunca afirmar que corresponde ao MSVC/Windows.
- Workflow Windows específico criado **na branch Filuna**: `.github/workflows/filuna-bambu-pinned-windows-e5.yml`, apenas build; **não publica, não integra no Filuna e não proclama paridade**. GitHub Actions execução `42161471485` tinha estado `queued` quando consultada.

## Estado da compilação Linux exploratória
Um build auxiliar Linux da tag foi iniciado na pasta isolada `/var/tmp/filuna-qa-wine/bambu-a1mini-e5-build`. A versão Linux desse build **NÃO é a referência de paridade**, pois a referência é Windows:
- CMake inicialmente falhou por falta Boost, depois TBB, GLEW, GLFW, cereal, NLopt.
- Dependências oficiais do `deps/` construídas em `/var/tmp/filuna-qa-wine/bambu-a1mini-e5-deps-build` até Boost 1.84, TBB, GLEW, GLFW, Cereal e NLopt, com logs separados, sem alterar produção.
- Linux executável **ainda não compilado**; não o usar como equivalente ao binário Windows. Prioridade no rebuild Windows da tag pelo workflow oficial.

## Comparações obrigatórias A → B → C (aceite EXATO)
| Gate | Entrada/gerador | SHA normalizado | Resultado |
|---|---|---|---|
| A | GUI Bambu Studio 02.09.01.70 × CLI Bambu Studio 02.09.01.70, **mesmo 3MF salvo nessa versão, mesma placa** | AGUARDANDO | **NOT RUN**: só há G-code CLI de arquivo salvo em 02.07.01.57 |
| B | CLI Bambu Studio Windows oficial 02.09.01.70 × CLI Windows compilado da TAG usando mesmo comando e mesma entrada | AGUARDANDO | **NOT RUN**: build Windows ainda sem binário homologável |
| C | CLI Windows compilado da TAG × Filuna com motor OEM Bambu isolado realmente carregado | AGUARDANDO | **NOT RUN**: Filuna ainda sem motor Bambu integral compilado/carregado |

**Controle NEGATIVO existente, fora A/B/C:**
- Bambu Studio Windows oficial CLI × Orca portátil anterior `2.5.0-dev` com `--allow-newer-file`, ambos sobre o 3MF original 02.07:
  - SHA-256 normalizado Bambu: `9f722a9b83367240f553b19193b5ddbdf523685ff16e64859f8e9c14fd834652`.
  - SHA-256 normalizado Orca: `457b175af81089cb7d208705318d375c87aab0cd84128dbafa892a72c7865da2`.
  - **DIFERENTES.** Primeira linha diferente *após* exclusão permitida de identidade do gerador no cabeçalho: **linha 3 de ambos**: oficial `; model printing time: 8h 39m 28s; total estimated time: 8h 45m 38s` × Orca `; model printing time: 9h 34m 41s; total estimated time: 9h 40m 52s`.
  - Primeira diferença CONFIG: oficial linha 18 `; accel_to_decel_enable = 0` × Orca linha 14 `; accel_to_decel_enable = 1`.
  - Primeira diferença EXEC: oficial linha 639 `M73 P0 R525` × Orca linha 767 `M73 P0 R580`.
  - Script versionado `FILUNA/qa/gcode/compare_oem_exact_sha.py`; relatório real no Ubuntu `/var/tmp/filuna-qa-wine/oem-7-brands-audit-20261106/a1mini-e8-old-orca-negative-control.json`. Esse arquivo **NÃO** é um E8 de Filuna com engine OEM carregado.

## Pendências rastreáveis
1. Gerar com Bambu GUI 02.09.01.70 um 3MF salvo nessa versão (sem edição de JSON), e seu G-code nativo, para completar A.
2. Rebuild Bambu **Windows** pelo workflow pinado, conferir toolchain efetivo Windows/MSVC/SDK e comparar SHA normalizado CLI com G-code da mesma entrada (B).
3. Montar motor OEM isolado Bambu como o código dessa tag no Filuna e provar que realmente foi ele quem recebeu 3MF e gerou G-code; versão/proveniência/ABI/hash; depois C.
4. Qualquer SHA divergente: informar **primeira linha** usando `compare_oem_exact_sha.py`, não tolerância, nem byte-count.
5. **Não declarar homologação ou compatibilidade de rede física** antes dos gates e testes OEM de transporte/autenticação.

## Comunicação e segurança
O AGPL não autoriza inferir que qualquer biblioteca proprietária publicada apenas em binário tenha código automaticamente disponível. Não extrair chaves, não contornar assinatura, não republicar blob de terceiro sem verificar termos. Solicitações formais de código podem ser redigidas, mas envio exige destino confirmado pelo usuário. 

## AJUSTE MAIS RECENTE: GATE B NO MESMO RUNNER WINDOWS

- Fonte oficial Windows: https://github.com/bambulab/BambuStudio/releases/tag/v02.09.01.70 ; ZIP portátil Bambu_Studio_win-v02.09.01.70-20261103183404.zip (496653167 bytes).
- Entrada 3MF original (não resalvar para B): SHA256 3649d011b82a52aceed643f7e82c22803a59001704f5e3e415e16a7ed590c365 (15846184 bytes).
- Controle Wine: SHA256 321a29de1290a42edf785674e789b9d69d41c5ba979a69144faf081aa69372df (73159248 bytes). Original foi copiado sem alterações para /srv/site/incoming/filuna-a1mini-wine-reference-02090170.gcode para transferência segura.
- Workflow versionado .github/workflows/filuna-bambu-pinned-windows-e5.yml: runner windows-latest, tag Bambu commit 56a57d00, deps oficiais, MSVC VS 2026, Release, BBL_RELEASE_TO_PUBLIC=1 e BBL_INTERNAL_TESTING=0. Registra ImageOS, ImageVersion e versão exata do cl.exe.
- Script FILUNA/qa/windows_bambu_same_runner_b_test.ps1: StageOnly recebe links temporários POST de 3MF e Wine e valida hashes; CompareOnly baixa ZIP oficial da Bambu, roda ambas CLIs com o mesmo modelo, placa 0 e parâmetros, produz oficial_win.gcode e compilado_win.gcode; compara hashes RAW e os hashes normalizados com primeiro desvio; compara Windows oficial com Wine.
- Comparador FILUNA/qa/gcode/compare_oem_exact_sha.py possui gates B (build oficial) e C (Filuna integrado) distintos. Validado com dois arquivos idênticos (PASS) e controle divergente (FAIL, primeira linha 3).
- Execução antiga 42161455007 falhou porque where.exe cl rodava antes do MSVC setup; erro corrigido. Workflow de build 42162864143 foi enfileirado via push mas NÃO recebeu arquivos, então NÃO executa o gate B.
- O acionamento manual do workflow com os links temporários foi RECUSADO pelo canal de execução do servidor; o arquivo 3MF de terceiros e o G-code de 73MB NÃO foram publicados em repositório público. Links expiram e precisam ser recriados na sessão de disparo.
- STATUS: build Windows pendente; teste B Windows oficial x compilado NÃO EXECUTADO; teste Windows x Wine NÃO EXECUTADO; teste A e teste C NÃO EXECUTADOS. Não declarar equivalência nem homologação.

## REVISÃO DE EXECUÇÃO — TESTE B AUTOMÁTICO + 3MF CIFRADO (06/11/2026)
- O teste B agora roda em duas etapas no **mesmo runner Windows**: (1) a CLI Bambu oficial cria um 3MF de cubo 20mm com os presets A1 mini e ambos os binários o fatiam; (2) somente quando a primeira etapa passa, o 3MF real criptografado é descriptografado com segredo GitHub Actions e também fatiado nos dois binários.
- Workflow `.github/workflows/filuna-bambu-pinned-windows-e5.yml` mantém compilação do commit oficial `56a57d00a591360cf41bf21383915ee5bf357be5` com Visual Studio 2026 Release e dependências da tag. Script do teste público: `FILUNA/qa/windows_bambu_public_a1mini_cube_b_test.ps1`; script cifrado: `FILUNA/qa/windows_bambu_encrypted_a1mini_b_test.ps1`.
- **Testado no Ubuntu/Wine**: CLI Bambu Studio oficial 02.09.01.70 gera 3MF próprio A1 mini usando `--export-3mf` e os perfis `Bambu Lab A1 mini 0.4 nozzle`, `0.20mm Standard @BBL A1M` e `Bambu PLA Basic @BBL A1M`, e fatia esse projeto com exit code 0. Isto valida a preparação da fixture, NÃO o gate B do novo build Windows.
- O arquivo original do cliente 3MF tem SHA256 `3649d011b82a52aceed643f7e82c22803a59001704f5e3e415e16a7ed590c365`, 15.846.184 bytes. A versão **cifrada** foi registrada sob `FILUNA/qa/fixtures/` na branch QA; o arquivo plaintext não foi adicionado ao GitHub. Há um GitHub Actions secret cadastrado para decriptação; **não expor valor da chave nos logs**.
- O valor fixo de referência do Wine é `321a29de1290a42edf785674e789b9d69d41c5ba979a69144faf081aa69372df`. **Somente o teste do 3MF privado** pode compará-lo com o G-code oficial Windows. Não comparar com o cubo próprio, pois são entradas diferentes.
- Ambos scripts agora exigem SHA-256 **bruto idêntico** e equivalência no comparador normalizado (que só ignora identificação/data do gerador no HEADER), registrando a primeira linha divergente ao falhar.
- `actions/upload-artifact` deve publicar SOMENTE arquivos JSON de SHA e diagnóstico, jamais o modelo privado descriptografado, G-code gerado, executáveis ou dependências; testes removem entradas e saídas temporárias.
- A ação de compilar e comparar está enfileirada em Windows no GitHub Actions. **Até a emissão efetiva dos dois G-codes pelo runner, B permanece `NOT RUN`, nenhum SHA novo pode ser inventado.** O gate C (Filuna integrado) permanece separado.
- Duas cópias cifradas podem constar na branch durante a migração de scripts e nomes: racionalizar para uma única fonte de verdade antes de liberar qualquer workflow; não trocar as chaves sem atualização coordenada do secret e do SHA de ciphertext.


## NOVO CRITÉRIO — timestamp estrito, G-code interno e cache (06/11/2026)
Pedido expresso: **SHA bruto pode diferir apenas pela linha de data/hora de geração**. Gate B aceita SHA normalizado igual **após remover somente o comentário de data/hora do gerador no HEADER_BLOCK**, relatando para cada lado o **número original e o texto exato removido**. NÃO remover/normalizar o nome ou a versão do programa, tempo estimado de impressão, valores de CONFIG_BLOCK, comandos ou quaisquer outros comentários. Se não houver comentário reconhecido de data/hora, nada é removido. No corpus Bambu oficial do diorama, as 15 primeiras linhas do HEADER_BLOCK não mostram comentário de data/hora; não presumir que a linha existe.

- Comparador atualizado em `FILUNA/qa/gcode/compare_oem_exact_sha.py`: `official_win_sha256_raw`, `compiled_win_sha256_raw`, `official_win_sha256_normalized`, `compiled_win_sha256_normalized`, `official_timestamp_lines_removed`, `compiled_timestamp_lines_removed`, `first_difference_after_normalization`, hashes por membro de ZIP 3MF, e flags de aceite.
- Se a saída é `.gcode.3mf`, **não comparar SHA do pacote ZIP**; comparar SHA-256 do conteúdo binário `Metadata/plate_1.gcode` e auditar os SHA-256 individuais de `slice_info.config`, `plate_1.json`, miniaturas e demais membros. Diversidade de datas de ZIP não afeta o gate de G-code; divergências de membros ficam documentadas separadamente.
- Scripts `windows_bambu_public_a1mini_cube_b_test.ps1` e `windows_bambu_encrypted_a1mini_b_test.ps1` aceitam saída `.gcode` ou `.gcode.3mf`, extraem o SHA **do G-code real**, e não rejeitam por diferença de SHA bruto quando normalizado equivale (somente timestamp). Para 3MF do cliente, Wine é referência **apenas de SHA bruto fixo** `321a29de1290a42edf785674e789b9d69d41c5ba979a69144faf081aa69372df`; se diferente, o Wine fica descartado como referência Windows (sem linha divergente, porque G-code Wine não trafega para runner).
- Workflow Windows atualizado com `actions/cache/restore@v4` e `actions/cache/save@v4`, chave `bambu-v02090170-win-x64-vs18-release-deps-${{ hashFiles('official/deps/**') }}`; cache salvo **imediatamente após a compilação oficial das deps**, antes do build do motor. Cache ainda só terá conteúdo se uma execução conseguir terminar as dependências.
- Testes sintéticos REAIS do comparador no SentinelX: **PASS** SHA bruto distinto com mudança exclusiva de timestamp (linha 3; normalizado igual); **FAIL** quando `infill = 10%` virou `15%`, informando primeira linha 7; **PASS** para dois ZIPs com timestamps internos diferentes, lendo `Metadata/plate_1.gcode` e listando `Metadata/slice_info.config` separado.
- **Matriz de quatro valores para entregar quando sair o Windows**: SHA bruto do G-code `oficial_win`; SHA bruto `compilado_win`; SHA bruto fixo `Wine`; SHA **normalizado oficial/compilado** (dois hashes e igualdade). Registrar origem Windows, `cl.exe` e ImageVersion.
- **Estado desta atualização:** o relatório Windows novo ainda NÃO foi emitido; runs da versão anterior em fila/in_progress. Não afirmar B=PASS sem JSON produzido por Windows real. Manter PR #5 draft, sem releases nem artefatos de código/modelos de terceiros.
