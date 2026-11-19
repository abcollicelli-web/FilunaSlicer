# FILUNA — laboratórios reais, portáteis originais e porte seletivo do motor OEM
Data: 2026-11-07. Produto: fork nativo `abcollicelli-web/FilunaSlicer`, base Orca 2.5.0-dev. PR #6 DRAFT.

## Evidências efetivamente consultadas (sem alterar servidor)
- Servidor SentinelX: `/srv/site/incoming/filuna-local-oem-qa-20261106`.
- Laboratório 8 fabricantes: `run-20261106T153215Z/R3/HOMOLOGACAO-LAB-FILUNA-8-FABRICANTES-REAL-2026-11-06-R3.md`; corpus cubo, ponte, parede fina, balanço, várias peças, 3MF por peça, rede simulada.
- Origens de pacotes locais: `/srv/site/incoming/filuna-official-portables-20261106/`, incluindo `snapmaker-v2.4.1.zip`, `flashforge-1.7.15.exe` e a árvore `unpacked/`.
- **Não confundir a bateria R3 de 06/11 com as baterias GitHub Actions posteriores de 07/11:** R3 acusa portátil Anycubic/Flashforge ausente e Snapmaker com caminho de executável errado; Actions rodaram Anycubic original 2.0.0.6 e Flashforge original Linux 1.7.9.

## Snapmaker U1 — duas falhas diferentes
1. **Erro de inventário do laboratório Ubuntu (não de motor):** `lab/filuna_lab_run.sh` ainda aponta para
   `snapmaker-v2.4.1/Snapmaker_Orca_Windows_V2.4.1_portable/Snapmaker Orca.exe`.
   A listagem efetiva do ZIP extraído contém
   `snapmaker-v2.4.1/Snapmaker_Orca_Windows_V2.4.1_portable/snapmaker-orca.exe`
   e `Snapmaker_Orca.dll`, portanto `official-manifest.json exists:false` é um **falso negativo de caminho**. O ZIP oficial permanece SHA-256 `1f002e2168c576f800bcfd0509b5e164b5ad65a0963abc2bef8317d929d48c9d`. O arquivo no servidor não foi editado nesta rodada: publicar a correção do executor, quando autorizada, exige preservar evidências e usar o canal de laboratório escolhido.
2. **Falhas reais do fatiador oficial:** 2.4.1 e 2.4.0 geram diferentes trajetórias em execuções repetidas com os mesmos insumos; problema independente de timestamp/afinidade/nomes de arquivos. Upstream [#1149](https://github.com/Snapmaker/OrcaSlicer/issues/1149): o final purge pode cruzar a peça sem Z-hop. Código analisado: `Snapmaker/OrcaSlicer@bac6fd5c1e1c23ddf057cf6471ba4ed7ed621c4c`, `src/libslic3r/GCode.cpp`, `WipeTowerIntegration::append_tcr2`.
3. **Filuna C++ presente:** cálculo OEM `chamber_cooling_mode` de acordo com filamentos; quando o perfil contém essa macro, a purga final não validada agora **falha fechado** (bloqueia exportação nesse caso). O patch upstream candidato `FILUNA/fabricantes/snapmaker/patches/0002-candidate-final-purge-z-lift-safety.patch` permanece só candidato. **Ainda não temos Z-hop nativo corrigido e provado nem G-code seguro/reprodutível.**

## Anycubic — fonte versus binário e valor 0 de retração
- Laboratório R3 do servidor não possuía executável oficial válido de Anycubic; **o binário original Linux 2.0.0.6 foi testado em GitHub Actions** para Kobra S1, Kobra 3 V2 e Kobra 4. Ensaio estendido: 20 slicings oficiais por modelo, sem variações funcionais, **não** é o Filuna.
- Fonte `ANYCUBIC-3D/AnycubicSlicerNext@6103ed8b511609658d00d0538cc7f0609cdb57da` não tem build-equivalência comprovada com 2.0.0.6. **Não** portar arbitrariamente qualquer diferença da branch `main` para fingir equivalência com o binário.
- **Filuna C++ presente:** opção `retraction_distances_when_cut=0` aceita exclusivamente quando a retração longa está desativada; se ativa, exige distância finita entre 10–18 mm. Sem coerção de 0 para 10. Código `src/libslic3r/PrintConfig.cpp` e `src/libslic3r/Print.cpp`, source QA PASS. Ainda não há fatia Filuna recompilada para os três modelos nem ACE/ACK da aplicação.

## Flashforge — algoritmo original foi transportado para core Orca único
- No servidor existe instalador Windows `flashforge-1.7.15.exe`, SHA-256 `b3cc8804e0426e3f4bb2c0953667e27512c0047af51099c0e1af5c5d1b3ef8f3`; árvore de perfis 1.7.15 está presente, mas o relatório da própria árvore diz `SOURCE_GIT_TAG_NOT_INSTALLER`, sem equivalência comprovada. Os tests GitHub usam a versão original *Linux* 1.7.9 com SHA pinado. A versão Windows 1.7.20 permanece sem prova.
- Fonte oficial comparada: `FlashForge/Orca-Flashforge@70f2359ba8b4bc8ff9e54818a4bb1d05e56e7856`, `src/libslic3r/Fill/FillRectilinear.cpp`, blob `59c69a9d321042fef7a8a38a64df32a6006da8f9`.
- **Diferença exata:** com `has_consistent_pattern()`, Flashforge 1.7.9 usa `extended_object_bounding_box()` **sem translação**, enquanto Orca 2.5 aplica translação em função do centro/ângulo após a caixa estendida.
- **Porte realizado na branch QA (código C++ integrado ao motor Filuna):** `src/libslic3r/Fill/FillRectilinear.cpp` agora oferece ambos os caminhos, isolados por `PrintConfig::filuna_oem_legacy_infill_anchor`, default `false` definido em `PrintConfig.cpp` e membro na classe correta em `PrintConfig.hpp`. A seleção tem de ser ativada por **módulo OEM autenticado e compatível**, não por nome de impressora fixo. Não distribui outro executável. Mantém algoritmo Orca inalterado em perfis comuns. Também foi removida a migração indevida de `wall_direction=auto` para CCW: o Auto explícito continua disponível, com CCW default global.
- **Ainda não comprovado:** que essa diferença específica seja causal para os 534 grupos de trajetórias divergentes da AD5X; o G-code Filuna *anterior aos patches* possuía 5.753 movimentos G0–G3 contra 5.759 oficiais, 182 extrusões e 374 velocidades divergentes. Defaults reais 1.7.9 (`wall_direction=auto`, `filament_change_length=0`, `wipe_tower_type=type1`, `flush_volumes_matrix` original 4×4) precisam ser materializados pelo **loader de módulos/perfis**, sem editar os dados oficiais nem mudar defaults globais. Comparação C++ e testes negativos em AD5X/5M **NOT_RUN**.

## Evidências QA e limites
- `lab/test_oem_native_source_contract.py`: **17/17 testes de fonte PASS**, GitHub Actions [#42342812612](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42342812612). Inclui guarda que detecta campo `PrintConfig` no tipo errado, ausência de ativação OEM por nome de máquina e regressão do Orca padrão.
- **Isto não compila o novo motor**, não gera novo G-code Filuna e não aprova a correção funcional ou a segurança das trajetórias. `G4-I=BLOCKED`, `FILUNA_NATIVE_GCODE_PARITY=NOT_PROVEN`, `INTERNAL_HOMOLOGATION=0/3`, `HARDWARE_BETA=NOT_RUN`.
- A análise de opções encontrou 53 nomes OEM extras Snapmaker, 5 Anycubic e 4 Flashforge nos pares de fontes selecionados. **Não** copiá-los todos às cegas: é preciso seguir chamadas reais, dependências, versões, ABI e revisão de licenças/terceiros. Código aberto AGPL pode ser incorporado com atribuições e cumprimento de licença; binários de rede/plugins proprietários não são presumidos livres.

## Próximos passos bloqueadores, antes de avançar no Plano Mestre
1. Extrair/validar a versão exata executável/fonte por fabricante e revisar licença por arquivo; corrigir o falso caminho da Snapmaker no executor do laboratório ao sincronizar os scripts de QA, sem alterar logs históricos.
2. Portar demais deltas **causais** de planejamento/torre/macros/perfis do OEM para o Orca compartilhado, sem trazer outro motor completo. Incluir fallback seguro, isolamento e regressão Orca/8 marcas.
3. Ligar o `PrintConfig` OEM e defaults originais ao carregador de perfil sob demanda, com esquema, assinatura, cache e ABI; **não** ativar modo legado sem módulo verificado.
4. Compilar/testar o **motor nativo de QA** (não o instalador final); produzir G-code Filuna real das mesmas fixtures nos mesmos perfis, comparar com cada OEM original (G0–G3, E, F, temperaturas, ventilação, macros, purga), A/B repetido, com falhas negativas sem normalização enganosa.
5. Integrar e testar DeviceBridge/portas/auth/ACK/IFS/ACE/SnapSwap/câmera nos simuladores do próprio Filuna e G4-I. Só com tudo PASS prosseguir ao build final único e beta físico autorizado.
6. A próxima **onda estrutural global** registrada no Plano Mestre ainda é 01 — Fundação, seguida 02 — Qualidade Nativa Orca e 03/04 UI. Estas correções OEM parciais não promovem ondas nem gates.

**Governança:** apenas PR #6 DRAFT; servidor de produção, laboratório histórico, branch padrão e Windows dos clientes inalterados; sem release/merge/deploy/instalador/impressões. Bambuddy autenticado fica adiado por ordem do proprietário.
