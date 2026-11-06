# FILUNA — Teste B Bambu 02.09.01.70 sem expor entradas: cubo público e A1 mini cifrada

**Data:** 2026-11-06. **PR #5:** branch `filuna/oem-gcode-routing-qa-20261106` (não mesclada). Regra do proprietário: primeiro o teste B com 3MF público criado no próprio runner pela CLI Bambu 02.09.01.70; **somente se B público passar**, confirmar com o 3MF do cliente cifrado, usando o mesmo Windows runner. Não trabalhar mais no controle Orca 2.5 antigo.

## Arquitetura do ensaio
1. Checkout exato `bambulab/BambuStudio` commit `56a57d00a591360cf41bf21383915ee5bf357be5` / tag `v02.09.01.70` em `windows-latest`.
2. Compilar dependências próprias de `deps/` com Visual Studio 18 2026 x64, Release e flags oficiais; compilar Bambu Studio com `BBL_RELEASE_TO_PUBLIC=1`, `BBL_INTERNAL_TESTING=0`. Registrar `ImageOS`, `ImageVersion` e `cl.exe`. Nenhum motor Orca externo substitui Bambu nesta prova.
3. Baixar o ZIP público `Bambu_Studio_win-v02.09.01.70-20261103183404.zip` diretamente do release oficial da Bambu.
4. **B público:** usar `official/tests/data/20mm_cube.obj` (340 bytes) e perfis oficiais `BBL` para Bambu A1 mini bico 0,4 mm, processo `0.20mm Standard @BBL A1M`, filamento `Bambu PLA Basic @BBL A1M`; gerar um 3MF NATIVO com a CLI do release (`--export-3mf`) e inspecionar `Metadata/project_settings.config` para não confundir perfis. As duas CLIs do Windows recebem esse mesmo 3MF com `--slice 0` e os mesmos argumentos; saídas/diretorios temporários independentes; comparar SHA256 RAW e SHA256 normalizado (excluindo somente dados do gerador definidos no E8) e imprimir primeira linha divergente. Só relatório JSON pode ser artifact.
5. **B privado após sucesso do público:** checkout contém somente `FILUNA/qa/fixtures/a1mini-makerworld-original.3mf.gpg`; `gpg` no runner descriptografa com secret `FILUNA_A1MINI_3MF_GPG_PASSPHRASE` e valida SHA256 do 3MF original antes do slice; duas CLIs executam a mesma entrada, com saída G-code local e exclusão ao final. Relatório contém SHA dos dois novos G-codes, desvio/primeira linha, comparação do hash Windows oficial com SHA fixo do Wine. Se divergir do Wine, Wine é descartado como referência para Windows.

## Evidências de preparação/execução
- Validação prática no SentinelX da geração nativa 3MF no Bambu Studio Windows oficial 02.09.01.70 via Wine: `20mm_cube.obj` + três perfis oficiais produziram `a1mini_20mm_cube.3mf`, **21.422 bytes**, SHA256 `f72fa771e7c16c20c2db66ff2b44ad9ffcc0fcbc1212ca3b6c25d69d01812e1e`. O arquivo passou por inspeção ZIP JSON e informou A1 mini 0,4, `0.20mm Standard @BBL A1M`, Bambu PLA Basic; fatiamento oficial subsequente deu **exit=0**, G-code **209.331 bytes**. Este é ensaio no Wine de geração de fixture, NÃO resultado do Windows runner ou igualdade B.
- Script do cubo Windows: `FILUNA/qa/windows_bambu_public_a1mini_cube_b_test.ps1`.
- Script da confirmação privada: `FILUNA/qa/windows_bambu_encrypted_a1mini_b_test.ps1`; `try/finally` remove arquivo descifrado, credencial temporária e arquivos G-code do trabalho.
- Workflow de build/QA: `.github/workflows/filuna-bambu-pinned-windows-e5.yml`. Novo run iniciado por push foi registrado como `42163892088`, estado `queued` na consulta inicial. A fila pode evoluir; verificar antes de declarar resultado.
- 3MF privado original (não publicado): **15.846.184 bytes**, SHA256 `3649d011b82a52aceed643f7e82c22803a59001704f5e3e415e16a7ed590c365`.
- Arquivo cifrado GPG AES-256, sem compressão, presente em GitHub público **somente em forma cifrada**: `FILUNA/qa/fixtures/a1mini-makerworld-original.3mf.gpg`; **15.846.287 bytes**, SHA256 `1bdfc1bc7b9482d4f393a0701fda7f765f1ce81195ca203f693f9fb4850e4043`. Secret Actions `FILUNA_A1MINI_3MF_GPG_PASSPHRASE` cadastrado e presença verificada; valor NÃO registrar em texto, commit, log, issues nem relatório.
- Referência Wine anterior: G-code Bambu Studio Windows rodado sob Wine, SHA256 RAW `321a29de1290a42edf785674e789b9d69d41c5ba979a69144faf081aa69372df`. A comparação B usa SHA Windows recém-gerado; controle Wine é comparação direta do hash fixo, NÃO exige upload do G-code do Wine.
- Comparador `FILUNA/qa/gcode/compare_oem_exact_sha.py`: testado com arquivos iguais e diferentes; o gate B distingue os resultados e imprime primeira linha no relatório. Não confundir PASS do teste de ferramenta com PASS do slicing no Windows.
- O workflow foi alterado para **não publicar os binários compilados, não publicar modelo, não publicar G-code**. Apenas relatório JSON de SHA dos testes público e privado. É obrigatório não incluir segredos ou dados brutos nas mensagens de Actions.

## Estado verificável / próximos critérios
| Item | Estado |
|---|---|
| Build oficial da tag no Windows, flags corretas | Workflow preparado, em fila ou execução; aguarda resultado final |
| Fixture pública Bambu A1 mini feita por CLI oficial | PASS em ensaio Wine de preparação; etapa Windows ainda não concluída |
| B público SHA oficial Windows × compilado Windows | **NOT RUN/NO RESULT** até execução terminar |
| Fixture privada criptografada + Actions secret | **PRONTO** em branch de QA; ainda não descriptografada no runner |
| B privado SHA oficial Windows × compilado Windows | **NOT RUN/NO RESULT** até B público PASS |
| Oficial Windows × Wine (hash fixo) | **NOT RUN/NO RESULT** |
| A (GUI oficial × CLI oficial) | Não exigido para B; ainda NOT RUN |
| C (Bambu motor instalado/carregado no Filuna) | NOT RUN; não declarar 100% incorporado |

**Atenção:** o fato de os dois programas serem originados da mesma tag não garante SHA-256 igual sem teste; `cl.exe`, configurações de compilação, recursos e perfis efetivos continuam documentados por runner. O proprietário exige teste real e decide homologação.
