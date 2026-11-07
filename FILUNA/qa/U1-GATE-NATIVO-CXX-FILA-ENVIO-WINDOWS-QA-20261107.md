# Filuna Slicer — continuidade da correção U1: validador C++ real na fila de envio

**Data:** 07/11/2026. **Branch:** `filuna/oem-portable-first-qa-20261106`, PR #6 **DRAFT**, sem merge/deploy/release/instalador.

## Código integrado de fato nesta rodada

1. `src/slic3r/Utils/FilunaGCodeSafety.hpp`: **validador geométrico C++17 autocontido**, nenhuma dependência de Python, sem reescrita do G-code, com mapa de extrusões por XY/Z, distinção de torre de purga, cálculo de travessia de XY sem extrusão, arcos G2/G3 com I/J, posições desconhecidas antes de homing, troca de ferramenta, retrações curtas, limites 270³ mm, classificação PASS/WARN/BLOCK/REVIEW e linha/camada/XYZ. **Não é o firmware Klipper**, nem auditoria de 100% dos comandos e macros.
2. `src/slic3r/Utils/PrintHost.hpp`: preserva `filuna_effective_printer_model` nas cópias por movimento de `PrintHostJob`. O construtor em `PrintHost.cpp` obtém o **modelo do preset efetivo** no momento do job, sem declarar que representa um dispositivo autenticado.
3. `src/slic3r/Utils/PrintHost.cpp`: `PrintHostJobQueue::upload_job` verifica G-code **antes** de chamar `job.printhost->upload` no piloto **Snapmaker U1**. BLOCK/REVIEW impedem o envio e descrevem linha, camada e posição quando disponíveis; 3MF não analisável é recusado; WARN comunica risco sem modificar bytes. Este gate é no caminho Orca PrintHost, **não** em todo DeviceBridge/fabricante.
4. `src/slic3r/GUI/PrintHostDialogs.cpp`: o aviso `filuna_u1_safety_warning` deixa de ser ignorado por `on_info` e é mostrado por `wxMessageBox` na thread wxWidgets. **Ainda falta assegurar exibição antes do início da impressão** em todos os fluxos e autorização explícita dos riscos comprovados do OEM por usuário/log seguro.
5. `src/slic3r/CMakeLists.txt`: header registrado como fonte da biblioteca `libslic3r_gui`. Nenhum subprocesso Python no cliente e nenhum algoritmo da Snapmaker oficial foi reescrito.

## Provas executadas, não simuladas como impressão

- [GitHub Actions #42362530764](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42362530764): compila C++17 com `-Wall -Wextra -Wpedantic -Werror`; casos sintéticos PASS/BLOCK/WARN; quatro G-codes originais checados por SHA. **Snapmaker U1 monocor PASS**, **Snapmaker U1 bicolor BLOCK** com exatamente 1 risco real na **linha 38911, camada 14, X≈63,569 Y147,185 Z3,05**, **Bambu A1 PASS** e **Creality PASS** para geometria. A proteção de volume Bambu/Creality não foi homologada (serviço fora da mesa).
- [GitHub Actions #42363101061](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42363101061): compila novamente núcleo C++17, repete originais e executa 7 testes do contrato de ligação a `PrintHost::upload_job` e aviso GUI: **PASS**. São testes de arquivo/contrato; não prova Windows ligado/ACK.
- **Windows GUI:** primeira tentativa [#42362859858](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42362859858) falhou **somente por cache de dependências inacessível** na branch; o workflow foi corrigido para construir dependências do próprio QA sem criar instalador. A execução [#42363069593](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42363069593) iniciou a reconstrução; **resultado da compilação da biblioteca/GUI não comprovado na elaboração deste relatório**.
- **U1 quatro extrusoras:** primeiro 3MF sintético foi rejeitado pelo importer do OEM, antes de gerar G-code: [#42363527057](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42363527057) **FAIL, não homologar**. Agora `lab/snapmaker_u1_fourtool_original_qa.py` clona como fixture o projeto `resources/handy_models/OrcaCube_v2.3mf` do próprio portátil 2.4.1, preservando pacote OEM e adicionando quatro objetos com extrusoras 1..4 **apenas na cópia de laboratório**. Reteste [#42363769245](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42363769245), **em andamento, sem presumir PASS**.

## Pontos que ainda impedem declarar “somente impressão física”

- No registro de dispositivo, a identidade é a do perfil selecionado, **não** serial/firmware/sessão pareada e atestada. O gate hoje cobre a fila clássica PrintHost no escopo U1, **não** todos os caminhos do DeviceBridge.
- O motor Filuna do HEAD ainda não possui **A/B nativo compilado e comprovado** com todas as versões/presets OEM, nem 4 ferramentas oficiais provadas, nem validação da configuração Moonraker real na U1. É proibido declarar paridade Filuna × Snapmaker.
- A **aceitação consciente do risco OEM** ainda existe no laboratório Python, com log por SHA; falta implementar **em UI/app** autenticação efetiva, vínculo de hash do arquivo/versão OEM/risco, persistência auditável e permissão somente quando o mesmo risco está comprovado no arquivo oficial. Nunca permitir override para divergência exclusiva Filuna.
- O verificador C++ usa análise geométrica **limitada a deslocamentos críticos e 50 MiB por arquivo** e não expande macros não provadas. São limites de QA que exigem endurecimento antes da entrega comercial (arquivos longos, ferramentas semânticas, firmware real).
- No evento WARN de volume, `on_info` agora mostra diálogo, mas a prova de aviso **antes** de executar qualquer comando de impressão ainda requer integração de UI/confirm antes de enfileirar.
- **Anycubic e Flashforge**: ainda falta compilar patches nativos, comparar novos G-codes reais e integrar módulos sob demanda, perfis e protocolos de rede com ACK. Os contratos de fonte anteriores não promovem essas marcas.
- **Estado correto:** `FILUNA_NATIVE_SAFETY_HEADER_QA=PASS`, `NATIVE_UPLOAD_SOURCE_HOOK_QA=PASS`, `WINDOWS_CPP_GUI_COMPILE=NOT_PROVEN`, `INTERNAL_HOMOLOGATION_3_OEM=0/3`, `G4-I=BLOCKED`, `G4-F_PHYSICAL=NOT_RUN`.

**Regra imutável:** o validador não edita o G-code oficial nem injeta Z-hop; se risco só aparece no Filuna, corrigir motor/perfil até equivalência, nunca mascarar com pós-processamento. Evidências e macros estão em `FILUNA/fabricantes/snapmaker/vendor.yaml` e `FILUNA/qa/SNAPMAKER-U1-GCODE-IMUTAVEL-VALIDADOR-GEOMETRICO-20261107.md`.

## Resultado mais recente: importação 3MF aprovada, slicer original falha por access violation

- Nas rodadas [#42363769245](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42363769245) e [#42364223761](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42364223761), a fixture de quatro extrusoras deixou de falhar por JSON/3MF simples e avançou no importador BBS. O arquivo de origem da fixture é o **3MF handy_models/OrcaCube_v2.3mf** do ZIP oficial SHA-256 `302cb4d3b1583aa06bb56cad94265b3faf6d16dae82f9f2a882a39b27aafe8b0`. A cópia de QA registra extrusoras 1–4 sem modificar o original.
- A rodada [#42364395913](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42364395913) preservou o namespace padrão 3MF e **comprovou via log**: `begin to assemble objects, size 4`; quatro chamadas a `_generate_volumes_new` para IDs 5,6,7,8; `plate 1`; `got plate count 1`. Portanto **a importação do 3MF passou**.
- Imediatamente depois, porém, o **executável Snapmaker Orca Windows v2.4.1 original** retornou `3221225477 = 0xC0000005` e **zero G-codes**. Este resultado **não** confirma motor Filuna e **não** é homologação de quatro ferramentas. É obrigatório investigar a falha pós-importação do OEM: configurações do projeto 3MF, atribuição dos objetos/extrusoras, pipeline de `--slice` e eventual bug original. Registrar um controle 3MF oficial sem alterações antes de atribuir a falha às quatro ferramentas.
- O CI Windows do Filuna [#42363069593](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42363069593) ainda estava **construindo dependências** no último check deste checkpoint, **sem resultado de compilação de `libslic3r_gui`**. O teste independente C++17 do validador e a ligação estática à fila de upload permanecem PASS e não devem ser promovidos a compilação Windows.
- **Todos os gates não comprovados permanecem bloqueados; nenhum G-code foi alterado, nenhum instalador, merge, deploy ou impressão física executado.**


## Controles adicionais: crash da CLI oficial em 3MF original e OCCT/lld

**Controle 3MF decisivo:** [run #42364790537](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42364790537) executou **dois projetos com o Snapmaker Orca 2.4.1 original** sob a mesma configuração de máquina/processo/filamento:
1. O 3MF **imutável `resources/handy_models/OrcaCube_v2.3mf` do próprio pacote oficial** → `3221225477 (0xC0000005)`, zero G-code;
2. A cópia de QA com 4 objetos distintos e extrusoras 1,2,3,4 → **o mesmo `3221225477`**, zero G-code, apesar de log comprovar `begin to assemble objects, size 4`, `subobject_id 5..8`, `plate 1` e import concluído.

Consequência: **a falha não foi criada pela atribuição de quatro ferramentas.** Trata-se de uma regressão na CLI 3MF do portátil oficial sob os argumentos/perfis usados, ainda não atribuída conclusivamente a componente interno específico (carregamento de projeto/configuração, pipeline fatiamento, biblioteca etc.). Não fingir 4-color pass, nem tentar “corrigir” com modificação de G-code. Artefato preservado no CI.

**Bloqueio de compilação Windows:** [run #42363069593](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42363069593) avançou na reconstrução de dependências até aproximadamente **85%**, mas falhou ao linkar OpenCascade `TKG2d.dll` com `clang-cl/lld-link`: símbolos `std::_Variant_storage_<...>` indefinidos. É erro na toolchain/dependências **antes** da compilação da interface `libslic3r_gui`, não prova de falha ou sucesso do código `PrintHost.cpp`.

O workflow `.github/workflows/filuna-native-send-qa-windows-compile.yml` foi alterado (somente QA) para a rota nativa alternativa **MSVC/MSBuild**, com cache separado para não misturar artefatos clang. Essa nova execução **não é considerada PASS enquanto não produzir resultado real de compilação**.

**Próxima prova técnica obrigatória:** se a CLI original não suporta nem o 3MF distribuído pelo fabricante com o perfil U1, obter 3MF projetado/exportado por uma versão original que a CLI aceite, ou usar UI original em QA, mantendo hash de insumos; só depois ensaiar 4 ferramentas e comparar com Filuna. O escopo de contrato Python, C++ scanner, PrintHost upload hook e DeviceBridge continua separado do teste de impressão.

## Continuidade de homologação — 07/11/2026, rodada de segurança e causalidade 3MF

### Correção do fluxo de aviso antes do envio
- Foi comprovado pelo código que `info_fn(...filuna_u1_safety_warning...)` apenas enfileirava evento da GUI e o worker podia iniciar `printhost->upload` sem aguardar leitura/aceite. Isso não é consentimento.
- Correção em `src/slic3r/Utils/PrintHost.cpp` commit `1a97e2e`: para **WARN** da U1, enfileirar a informação e **falhar fechado** antes do transporte, até existir confirmação autenticada, vinculada ao SHA e auditável. Para BLOCK/REVIEW o bloqueio pré-existente permanece. Nenhum byte do G-code é alterado.
- Regressão `test_08_warn_requires_ack_before_transfer` no commit `5f54179e`: exige que o WARN tenha `throw` antes de `job.printhost->upload`.
- Prova: [Actions #42393531370](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42393531370) **SUCCESS** — compilação isolada C++17 com avisos estritos, 8/8 testes do contrato de envio (inclusive WARN), 4 arquivos G-code OEM originais por SHA e verificação geométrica cruzada. **Não é prova de GUI/ACK/DeviceBridge Windows.**
- A verificação da GUI/compilação Windows está separada em [#42393511281](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42393511281), status **PENDENTE** no checkpoint; seu êxito deve ser lido do job, nunca presumido.

### Controle adicional do crash oficial da U1 com quatro extrusoras
- A execução anterior [#42364790537](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42364790537) demonstrou `0xC0000005` tanto no 3MF imutável fornecido pelo próprio portátil Snapmaker 2.4.1 quanto na cópia de QA quatro objetos; isso refuta a atribuição do crash exclusivamente à montagem de quatro ferramentas.
- Novo commit `68dff907` em `lab/snapmaker_u1_fourtool_original_qa.py` introduz **três controles independentes**, sempre no executável OEM de SHA pinado e com mesmo perfil efetivo: (1) slice do STL original do cubo, (2) export de 3MF pelo próprio OEM a partir deste STL, (3) reimport/slice deste 3MF exportado sem alterá-lo. O relatório preserva status/exit code/SHA/log, inclusive quando o caso quatro ferramentas falha.
- CI dedicado: [#42393665358](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42393665358), status **PENDENTE** neste checkpoint. Não interpretar início do run como PASS de quatro ferramentas.

**Gates inalterados:** `SNAPMAKER_GCODE_NATIVE_PARITY=NOT_PROVEN`, `SNAPMAKER_FOUR_TOOLS=BLOCKED`, `WINDOWS_GUI_COMPILE=NOT_PROVEN`, `DEVICE_NETWORK_ACK=NOT_RUN`, `PHYSICAL_BETA=NOT_RUN`, `HOMOLOGACAO_INTERNA_3_OEM=0/3`. Branch/PR #6 permanecem DRAFT. Sem merge, deploy, instalador, publicação ou impressão; Bambuddy autenticado segue adiado.

## Resultado factual dos controles adicionais — run #42393665358 (07/11/2026)

**GitHub Actions [#42393665358](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42393665358): FAILURE legítimo, artefato `13582403017` inspecionado.** O teste não foi abortado por sintaxe nem cache: o executável original Snapmaker Orca Windows 2.4.1 foi iniciado, manteve ZIP SHA pinado e produziu evidências específicas:

| Caso com binário e perfis OEM idênticos | Retorno | G-code | Evidência |
|---|---:|---|---|
| Cubo `fixtures/cube20.stl` original direto | **0** | **250.183 bytes**, SHA `72e5eb86…` | `oem-raw-stl-control.log` |
| `resources/handy_models/OrcaCube_v2.3mf` OEM imutável | **0xC0000005** | Nenhum | import 3MF chega a `IMPORT_STAGE_FINISH`, mas encontra `plate count 0` |
| Exportação OEM de 3MF do mesmo STL fatiável | **0xC0000005** | Nenhum 3MF exportado | após `glfwInit Success`, OpenGL Mesa 4.6 e `render_thumbnail` |
| Reimportação de 3MF recém-exportado | **NOT_RUN** | Não se aplica | exportação oficial falhou; sem arquivo de saída |
| Cópia de QA com quatro objetos/ferramentas | **0xC0000005** | Nenhum | import conclui `plate count 1`, quatro volumes gerados, falha posterior |

**O que está provado:** (a) binário OEM e perfis conseguem fatiar STL normalmente; (b) a montagem de quatro ferramentas não é condição necessária para um crash de importação 3MF, pois o 3MF OEM intocado também falha; (c) exportar 3MF usando CLI oficial também falha neste runner. **O que NÃO está provado:** se os crashes de importação e exportação compartilham uma única causa, se Mesa/thumbnail gera a exceção (somente último passo visível no log), ou se um 3MF preparado pelo aplicativo gráfico funcionará no CLI. Não classificar um erro oficial como regressão Filuna; sem paridade e sem quatro cores.

**Gates preservados:** `U1_ORIGINAL_STL=PASS`, `U1_ORIGINAL_3MF_IMPORT=FAIL`, `U1_ORIGINAL_3MF_EXPORT=FAIL`, `U1_FOUR_TOOL=BLOCKED`, `U1_NATIVE_PARITY=NOT_PROVEN`, `PRINT_SEND_NETWORK=NOT_RUN`, `PHYSICAL=NOT_RUN`. Ações somente no branch de QA; sem alterar G-code, arquivos oficiais, firmware, executável OEM, produção ou aprovação de marca.

### Triagem do ponto da exceção: inspeção da fonte OEM pinada

A fonte aberta do Snapmaker Orca pinada em `Snapmaker/OrcaSlicer@bac6fd5c1e1c23ddf057cf6471ba4ed7ed621c4c`, arquivo [`src/Snapmaker_Orca.cpp`](https://github.com/Snapmaker/OrcaSlicer/blob/bac6fd5c1e1c23ddf057cf6471ba4ed7ed621c4c/src/Snapmaker_Orca.cpp), na rotina `if (export_to_3mf)` aproximadamente linhas 5313–5620, confirma que a CLI gera miniaturas em ambiente GLFW/OpenGL quando as imagens da mesa não existem: cria contexto GLFW oculto, inicializa `OpenGLManager`, escolhe FBO e invoca `GLCanvas3D::render_thumbnail_framebuffer`. O último log do experimento de exportação é exatamente `render_thumbnail: plate_idx 0 volumes size 1`.

Isso **localiza a fase em que a execução caiu**, mas não identifica ainda a instrução exata nem comprova que o driver Mesa causou o acesso inválido. Importar um 3MF e exportá-lo são caminhos de código distintos: investigar separadamente, com stack trace/minidump e comparação UI vs CLI, sem modificar engine OEM de produção, preset nem G-code. A ausência de prova equivale a `BLOCKED`, não `PASS`.
