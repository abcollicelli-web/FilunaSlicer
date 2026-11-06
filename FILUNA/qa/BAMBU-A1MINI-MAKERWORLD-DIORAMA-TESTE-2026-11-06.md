# FILUNA — TESTE MAKERWORLD A1 MINI: DIORAMA EVOLUÇÕES

**Data:** 2026-11-06 (Europe/Rome). **Origem:** 3MF anexado pelo proprietário; preservado sem alterar modelo, camas ou filamentos.
**Estado da sessão:** teste Bambu oficial em execução no laboratório; o documento só registra o que foi medido, não inventa G-code final ou módulos prontos.

## Fonte
- Arquivo: `Charmander-Charizard+evolutions+diorama.3mf`
- SHA-256 da transferência integral: `3649d011b82a52aceed643f7e82c22803a59001704f5e3e415e16a7ed590c365`
- Tamanho exato: 15.846.184 bytes
- Cópia de ensaio no host SentinelX: `/srv/site/incoming/filuna-qa-a1-mini-original.3mf`
- ZIP 3MF: 23 entradas, incluindo geometria, `Metadata/project_settings.config`, `Metadata/model_settings.config`, `Metadata/slice_info.config`, `Metadata/plate_1.json` e imagens de placa.
- **Impressora já definida no arquivo: Bambu Lab A1 mini**, `Bambu Lab A1 mini 0.4 nozzle`, **não** P2S. O proprietário determinou explicitamente conservar A1 mini; **não converter silenciosamente para P2S**.
- Processo `0.20mm Standard @BBL A1M`; altura de camada 0,2 mm; infill 10%; suporte habilitado; mesa texturizada.
- Cinquenta? Não: **cinco** materiais: `Bambu PETG-CF @BBL A1M 0.4 nozzle`, `Bambu PLA Matte @BBL A1M`, `Bambu PLA Basic @BBL A1M`, `Generic PETG @BBL A1M`, `Generic PLA @BBL A1M`, com diâmetro 1,75 mm. **Não assumir segurança de alternância de materiais em um único bico sem validar projeto, G-code e compatibilidade AMS**.
- Geometria do modelo principal: 1.055.850 faces (requer análise de memória e slicing prolongado); o conteúdo não é um cubo sintético.

## Versões fixadas
- Fonte oficial pública **Bambu Studio** clonada em `/srv/site/incoming/filuna-bambu-source-qa-20261106`, commit `355d71eee5a9ebc794693fe57e63674806019770`. Git clonado integra fontes abertas e recursos, **não autoriza distribuir ou integrar componentes fechados do fabricante**.
- Executável oficial Bambu Studio já presente no laboratório: `/var/tmp/filuna-qa-wine/official-bambu-v0290170/unpacked/bambu-studio.exe`, versão do pacote `02.09.01.70`; código-fonte clonado (master atual) NÃO está afirmado como binariamente idêntico à versão 02.09.01.70. Necessário pin do commit exato do build, se disponível.
- Filuna candidato: PR #5, `filuna/oem-gcode-routing-qa-20261106`, ainda em draft e sem novo binário Windows OEM validado.

## Sequência obrigatória de testes
1. **Bambu Studio oficial primeiro**: CLI com `--slice 0`, arquivo 3MF sem alterações, prefixo Wine `/var/tmp/filuna-qa-wine/prefix`, saída isolada `/var/tmp/filuna-qa-wine/qa-a1mini-evolutions-bambu-official`. Quando terminar, registrar G-code, saída/erro, tempo, filamento, configurações, integridade do container G-code. Se exigir geração de `.gcode.3mf`, preservar os dois outputs.
2. Integrar código OEM Bambu conforme contrato de rede 100% e fatiamento 100% de diferenças, **sem duplicar Orca comum**, preservando licenças, interfaces e firmware por modelo; registrar commits e dependências. **Clone de repositório ou pacote JSON sozinho não constitui integração.**
3. Build Windows completo e novo Filuna executável com o módulo A1 mini funcional selecionado por dispositivo cadastrado e modelo/firmware. Se o build falhar, registrar erro exato e corrigir.
4. Filuna com **mesmo 3MF sem alterar config**; comparar G-code semântico: XYZ/E/F/M204 por feature, retrações, deslocamentos, tempo, filamento, suportes, troca de material, pausas e `.gcode.3mf` incluindo `slice_info.config` e imagens de placa.
5. Testes de protocolos OEM de rede autenticados simulados (SSDP, MQTT/TLS, FTPS), e depois impressora física para validação final. Não iniciar impressão de forma automática.
6. A decisão de homologação, com números e evidência, pertence ao proprietário.

**Não fazer afirmações prematuras:** o manifesto do Bambu no Filuna ainda registra o motor completo como pendente; o ZIP de perfis Bambu em QA e os hooks A1 atuais NÃO resolvem a A1 mini nem equivalência OEM de todos os modelos.

## Testes em execução e resultados
A tentativa Bambu oficial do 3MF foi iniciada. A primeira invocação no Ubuntu/Wine atingiu o limite de 78 segundos sem finalizar, enquanto o processo consumia CPU e já criava `plate_1.gcode.tmp`. **Resultado G-code final ainda não verificado nesse instante.** Preservar processo/arquivo temporário e aguardar a conclusão do trabalho em curso antes de lançar outro slicing do mesmo objeto.


## RESULTADO REAL — BIMBO STUDIO OFICIAL A1 MINI (não Filuna)
**Bambu Studio binário 02.09.01.70, mesmo 3MF original sem alterações, CLI Wine --slice 0, 2026-11-06.**
- `/var/tmp/filuna-qa-wine/qa-a1mini-evolutions-bambu-official/result.json`: `return_code=0`, `error_string="Success."`, sem alerta no campo `warning_message`.
- `/var/tmp/filuna-qa-wine/qa-a1mini-evolutions-bambu-official/plate_1.gcode`: **73.159.248 bytes**, SHA-256 `321a29de1290a42edf785674e789b9d69d41c5ba979a69144faf081aa69372df`.
- **8h45m38s** total estimado, 8h39m28s modelo, **701 camadas**, altura máxima **140,20 mm**.
- **160,736404 g** de filamento reportados no `result.json`, **uma** entrada `filaments` efetivamente consumida. O 3MF original contém cinco perfis, mas o resultado CLI não é prova de impressão em cinco cores/materiais. `filament_change_times=0`.
- Comandos totais: `G0=28`, `G1=1.422.451`, `G2=250.192`, `G3=419.029`, `G92=1407`, `M204=135.457`, `M1002=2124`, `M620=3`, `M621=2`.
- `HEADER_BLOCK` preservado, inclui modelo `Bambu Lab A1 mini`, nozzle `0.4`, perfil `0.20mm Standard @BBL A1M`, tempo, camadas e altura. `CONFIG_BLOCK` preservado. Nesta execução foi gerado `.gcode`, **não** pacote `.gcode.3mf`; metadados/imagens de placa precisam de teste próprio.
- Tempo para geração real da peça grande: a primeira espera CLI encerrou ao limite 78s, mas o processo permaneceu ativo e depois produziu arquivo completo e `result.json`, sem reiniciar o fatiamento.
- Resumo do tamanho: exec G-code 65.060.885 bytes, comentários do corpo 8.049.580, config 46.978, header 1.802 e outros 3. **Não usar bytes brutos como critério de equivalência de movimento.**
- Movimento G-code usa `M83`, extrusão relativa e contém `G2/G3`; futuros comparadores devem levar em conta eixos modais, arcos e reinícios `G92`. Medir apenas G1 descartaria centenas de milhares de movimentos.
- Projetos MakerWorld não devem ser reinterpretados como P2S. **Manter A1 mini.**
- Saída da ferramenta oficial inclui `triangle_count=1.055.850`, `sliced_time=217565` ms, `peak_rss_mb=2009.62`.
- Os limites de posição precisam ser auditados por feature e arcos antes de decidir segurança de mesa; o 3MF contém `Metadata/plate_1.json` com bbox do modelo na placa. Não inferir que a máquina aceita todos os movimentos só pela ausência de warning CLI.

## FONTE EXATA DO MESMO RELEASE E AUDITORIA INICIAL DO DELTA
- Tag oficial **`v02.09.01.70`** encontrada em `bambulab/BambuStudio`, commit **`56a57d00a591360cf41bf21383915ee5bf357be5`**. Clone completo desta tag em `/srv/site/incoming/filuna-bambu-v0290170-source`. Fonte de `master` atual em `/srv/site/incoming/filuna-bambu-source-qa-20261106`, commit `355d71eee5a9ebc794693fe57e63674806019770`. **Usar commit da TAG no comparativo e port, não o master atual.**
- Auditoria preliminar do repositório público TAG vs checkout de base Orca do laboratório:
  - `src/libslic3r`: 539 arquivos oficiais; 22 arquivos com bytes idênticos, 432 diferentes no mesmo caminho, 85 exclusivos do caminho oficial.
  - `src/slic3r/GUI/DeviceCore`: 89 arquivos; 2 idênticos, 68 diferentes, 19 exclusivos do caminho oficial.
  - `src/slic3r/Utils`: 89 arquivos oficiais sem arquivo de mesmo caminho no checkout Orca comparado. **Pode ser reorganização de paths, NÃO significa 89 módulos necessários automaticamente**.
  - `resources/profiles/BBL`: 2.727 arquivos; 1.309 binariamente idênticos, 1.206 diferentes e 212 ausentes no checkout de comparação. Esta auditoria é de bytes por caminho, distinta da anterior de equivalência semântica JSON.
  - Relatório JSON no servidor: `/var/tmp/filuna-qa-wine/qa-a1mini-evolutions-bambu-official/full_bambu_tag_delta_audit.json`.
- **Clonar as fontes do fabricante ≠ incorporar todos os componentes no motor Filuna e recompilar**. Os componentes Bambu/Orca de código C++ requerem integração de APIs, ABI, opções PrintConfig e testes de compilação; rede fechada não pode ser distribuída por dedução. Ainda falta um novo executável Filuna com módulo de rede/engine A1 mini ativo. Não foi realizado fatiamento válido de comparação Filuna OEM para este projeto.
