# Anycubic — Kobra S1 / Kobra 3 V2: origem oficial e testes internos (07/11/2026)

## Fonte efetivamente obtida
- Publicação oficial Anycubic via APT: `https://cdn-universe-slicer.anycubic.com/prod/dists/noble/main/binary-amd64/Packages`.
- Pacote Debian original: `AnycubicSlicerNext_linux-v2.0.0.6-20261030105113.deb`.
- Versão APT: `1:2.0.0.6`; versão do aplicativo declarada nos recursos: `2.0.0.6`.
- Comprimento: **153.586.194 bytes**.
- SHA-256 divulgado no índice oficial e confirmado após download: `fdb134a4e11fad7e795c8d425520ce1305321ccc0bf1375a651a1513f8854678`.
- Este **não é** um ZIP portátil Windows. É o pacote Linux OEM original, **extraído em pasta temporária** com `dpkg-deb -x`, sem instalar, compilar, imprimir ou modificar o sistema do usuário.
- O primeiro ensaio se recusou corretamente a aceitar a versão antiga `2.0.06`; os dados de pin foram atualizados apenas após capturar `Version/Filename/SHA256/Size` atuais do índice do fabricante.

## Evidência real de CI
- Runner GitHub Actions Ubuntu 24.04 `#42314817560`: **SUCCESS**, seis testes de integridade negativos/positivos PASS.
- `VENDOR_APT_INDEX=PASS`, `OFFICIAL_DEB_SHA256=PASS`, `ORIGINAL_PACKAGE_EXTRACTION=PASS`, `OFFICIAL_APPLICATION_VERSION=PASS`.
- Encontrados **1 executável** e **630 arquivos JSON** de perfis.
- Busca textual inicial no inventário: **184 referências para Kobra S1** e **50 referências para Kobra 3 V2**. Estes números medem arquivos que mencionam os nomes, **não** 184 ou 50 perfis homologados.
- Artefato e logs: https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42314817560/artifacts/13550426151

## Critérios de promoção
| Prova | Kobra S1 / ACE Pro | Kobra 3 V2 |
|---|---|---|
| Pacote OEM oficial e SHA | PASS (compartilhado) | PASS (compartilhado) |
| Candidato de perfil encontrado | PASS apenas inventário textual | PASS apenas inventário textual |
| Resolução de máquina, processo, filamento, bico e modelo exatos | PENDENTE | PENDENTE |
| CLI oficial A/B em saídas independentes | PENDENTE | PENDENTE |
| G-code real do Filuna com comparação de paridade | PENDENTE | PENDENTE |
| Rede de laboratório, capacidades e integração UI | PENDENTE | PENDENTE |
| HOMOLOGAÇÃO INTERNA do modelo | PENDENTE | PENDENTE |
| HOMOLOGAÇÃO FÍSICA | Apenas no beta vigiado após compilação final | Apenas no beta vigiado após compilação final |

**Próximo passo:** extrair hierarquias e combinações de perfis exatas de cada modelo no pacote oficial 2.0.0.6, testar CLI A/B, depois comparar saída do motor Filuna e executar testes de rede simulada. Não substituir Anycubic Kobra 3 V2 por Kobra 3 V3; não converter o sucesso de extração em sucesso de impressão.

**Governança:** PR #6 permanece DRAFT. A única validação voluntariamente adiada continua Bambu × Bambuddy. Nenhum build final, instalador, merge, upload de produção ou impressão física nesta etapa.


## Atualização comprovada — duas execuções CLI e catálogo universal (07/11/2026)

O primeiro job original revelou `libcloud_mqtt.so` não localizado, exit 127, porque as bibliotecas ELF originais do próprio pacote estavam numa pasta ainda não configurada no ambiente de processo. O runner foi corrigido para montar `LD_LIBRARY_PATH` **exclusivamente** com diretórios de `*.so` encontrados dentro do `.deb` oficial já verificado por SHA. Nenhuma DLL/SDK avulsa foi importada ou distribuída.

- GitHub Actions Linux, [execução #42319417880](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42319417880): **SUCCESS**, fonte ZIP/DEB e binário intactos, bibliotecas OEM originais disponíveis.
- Kobra S1 0,4 mm, PLA, Standard 0,20 mm: duas execuções separadas do OEM, G-code **210.094 bytes** cada, hashes brutos A=B, `cf60ff9d3412df392ed99077bf03e8a072e2b4c0fa273a76f6a3e5fdc6324401`.
- Kobra 3 V2 0,4 mm, PLA, Standard 0,20 mm: duas execuções separadas do OEM, G-code **325.731 bytes** cada, hashes brutos A=B, `4abf9fdd344d0c08f39b23b2ecd98f264bed92e25bda3be55054e6bb7ea53d5c`.
- [Execução #42319568616](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42319568616): **SUCCESS**, novo catálogo universal do mesmo pacote v2.0.0.6: **40 perfis de máquina, 191 de processo, 382 de filamento, zero erros de herança**; duas amostras S1/V2 repetem A/B com hashes idênticos dentro da mesma sessão. Isso não prova compatibilidade cruzada automática dos 613 presets.
- [Auditoria do código-fonte OEM #42319915138](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42319915138) @ `ANYCUBIC-3D/AnycubicSlicerNext` commit `6103ed8b511609658d00d0538cc7f0609cdb57da`: 1.428 arquivos de código/recurso auditados, 1.268 deltas armazenados isoladamente, **65 revisões pendentes**. O código da branch upstream contém só **19 máquinas, 42 processos e 45 filamentos** catalogáveis. **Não é a mesma árvore de perfis que a versão Debian v2.0.0.6 (40/191/382)**. A sincronização código-fonte-versão do pacote é **bloqueadora** antes de portar o módulo e anunciar cobertura de todos os modelos.

**Gates finais:** `OEM_ORIGINAL_CLI_AB=PASS` para **duas amostras**; `CATALOG_ORIGINAL_DEB=PASS_INVENTORY_AND_INHERITANCE_ONLY`; `OEM_SOURCE_DEPENDENCY_REVIEW=BLOCKED_65`; `FILUNA_NATIVE_PARITY=NOT_RUN`; `NETWORK_AUTH_CONTROL=NOT_RUN`; `HOMOLOGACAO_INTERNA=BLOCKED`; `FISICA_BETA=NOT_RUN`.

A regra universal permanece: todos os modelos/perfis por catálogo e deltas OEM completos de rede/controle/fatiamento; selecionar 2 modelos para teste não cria whitelist na implementação.


## Ampliação obrigatória para terceira máquina e normalização restrita (07/11/2026)

- O mesmo executor `lab/anycubic_kobra_models_official_qa.py`, alimentado pela **árvore universal 40/191/382** de perfis do pacote original v2.0.0.6, foi aplicado à **Anycubic Kobra 4 — 0,4 mm**, além das Kobra S1 e Kobra 3 V2. Não foi incorporado código específico da Kobra 4: é um terceiro caso amostral do sistema que lê metadados originais. Compatibilidade do preset `0.20mm Standard @Anycubic Kobra 4 0.4 nozzle` e filamento `Anycubic PLA @Anycubic Kobra 4 0.4 nozzle` conferida.
- Na primeira execução [#42321632655](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321632655), a Kobra 4 gerou **dois G-code reais independentes, 215.690 bytes cada e 8.800 linhas**. O hash SHA-256 bruto era diferente **somente porque o relógio mudou de `01:01:15` para `01:01:16` na linha 2**, `; generated by AnycubicSlicerNext 2.0.0.6 on ...`. Os bytes de todas as outras 8.799 linhas eram idênticos no artefato oficial. Essa evidência isolada permite uma exceção estrita para a data, **não** para movimentos, retração, purga ou temperatura.
- Foi criado `lab/anycubic_original_gcode_strict.py`: compara dados brutos, preserva quebras de linha, exige `HEADER_BLOCK`, verifica a assinatura **exata da versão** e permite **apenas** data de geração na segunda linha. Oito casos positivos/negativos em `lab/test_anycubic_original_gcode_strict.py` rejeitam mudanças de G-code, temperatura, cabeçalho, versão, newline e número de linhas.
- Workflow específico de **três modelos** [#42321830613](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321830613) iniciou com execução prévia desses oito testes, seguido de A/B oficial de S1, Kobra 3 V2 e Kobra 4. **O gate do CI ampliado só será PASS quando esse job encerrar com sucesso e evidências; não confundir a comparação manual do artefato anterior com sucesso do job novo.**
- Entrega futura do Filuna segundo o Plano Mestre §13: **motor-base Orca e diferenças OEM de código**, sem catálogo completo de impressoras/processos/filamentos no instalador. Consulta dos presets por modelo e capacidades via dados assinados sob demanda. **Ainda não integrada nem homologada em binário Filuna**.

**Até o resultado CI ampliado:** `OEM_S1_AB=PASS`; `OEM_KOBRA_3_V2_AB=PASS`; `OEM_KOBRA4_REAL_GCODE_IDENTICAL_EXCEPT_PROVEN_TIMESTAMP=PASS_MANUAL_BYTE_DIFF`; `OEM_KOBRA4_CI_GATE=PENDING`; `SOURCE_TO_BINARY_VERSION_MATCH=BLOCKED`; `FILUNA_NATIVE_PARITY=NOT_RUN`; `ACE_NETWORK_ACK=NOT_RUN`; `HOMOLOGACAO_INTERNA=BLOCKED`; `FISICA=NOT_RUN`.
