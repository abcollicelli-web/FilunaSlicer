# FILUNA — ESTADO REAL DE LIBERAÇÃO TÉCNICA: SNAPMAKER, ANYCUBIC, FLASHFORGE (07/11/2026)

**Escopo:** três fabricantes solicitados pelo proprietário; sem build final, sem distribuição de fatiador OEM completo, sem instalação/atualização em computadores dos clientes e sem impressão física.

**Regra principal:** [Plano Mestre, §1.1](../PLANO-MESTRE-ENGENHARIA-E-CRONOGRAMA.md) — **um só motor-base Orca + deltas OEM necessários**, perfis de dados buscados por modelo sob demanda na conexão, assistente de portas/AMS/câmera/perfis e **3 testes físicos completos e independentes** por modelo/coorte no beta para homologação técnica exibida. Dados de inventário/pacote oficial NÃO constituem homologação da marca inteira.

| Marca | CLI OEM oficial sem hardware | Fonte/perfis OEM inventariados | Integração Filuna nativa | Rede/controle/câmera/multimaterial | Decisão |
|---|---|---|---|---|---|
| **Snapmaker U1 0,4** | **FAIL em estresse**: v2.4.1, 40 execuções, quatro hashes com movimentos/retrações distintos. A/B de 11/11 tinha sido PASS isolado, mas não afasta falha rara | Código/catálogo candidatos inventariados; patch de acesso nulo aplica ao upstream, ainda não compilado | NOT_RUN | NOT_RUN | **BLOQUEADA** |
| **Anycubic Kobra S1 e Kobra 3 V2 0,4** | **PASS**: binário original .deb 2.0.0.6, duas invocações cada, G-code bruto idêntico | Catálogo original: 40 máquinas, 191 processos, 382 filamentos, zero heranças quebradas; source Git main atual 19/42/45, versão não equivalente; 65 revisões de código/dependências | NOT_RUN | NOT_RUN | **OEM_ORIGINAL_2_SAMPLES_PASS, FILUNA_BLOQUEADA** |
| **Flashforge AD5X e Adventurer 5M 0,4** | **PASS**: binário original AppImage 1.7.9; AD5X bytes idênticos; AD5M somente linha de data/hora no cabeçalho difere | 26/60/123 perfis interpretados pela árvore de fonte; 64 revisões de código/dependências. É versão anterior à Windows 1.7.20 | NOT_RUN | NOT_RUN | **OEM_ORIGINAL_2_SAMPLES_PASS, FILUNA_BLOQUEADA** |

**Evidências principais:**
- [Snapmaker U1 source guard git apply PASS #42319819540](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42319819540), [11-run PASS de repetibilidade limitada #42321507580](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321507580), [mesmo caminho com G-code divergente em 10 runs #42321603426](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321603426), [40-run FAIL real #42321783683](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321783683). 40 execuções: 35 G-codes de 250.183 bytes com mesmo hash normalizado; 3 de 249.098 bytes com hash distinto; 2 de 249.212 bytes com hashes distintos. Não normalizar movimentos/wipes/M73. Origens da variação AINDA NÃO CAUSAIS.
- [Anycubic S1+Kobra 3 V2 A/B #42319568616](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42319568616) PASS; [fonte auditada #42319915138](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42319915138) PASS inventário. **Kobra 4 0,4** adicionada como terceira amostra ao mesmo runner, [execução #42321830613](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321830613) **EM EXECUÇÃO NO MOMENTO DESTE REGISTRO, NÃO DECLARAR PASS ANTES DOS LOGS**.
- [Flashforge AD5X+AD5M A/B e regras negativas #42321095370](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321095370) PASS; [fonte auditada #42319915138](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42319915138) PASS inventário. Nada de cópia de plugin fechado de rede.
- [Pacotes de perfil por máquina sob demanda, 7/7 testes negativos e positivos #42321919685](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321919685) PASS: \`FILUNA/tools/oem_on_demand_profiles.py\`, QA **data-only**, filtra apenas perfis explicitamente compatíveis, bloqueia predicados desconhecidos, marca necessidade de assinatura e da futura aplicação Windows. **Não** publica API nem assina/instala dados.

**Bloqueadores da liberação das três marcas:** porte real de todos os deltas OEM necessários para o Orca compartilhado, resolução da correspondência entre código-fonte e binário do fabricante, fechamento dos reviews de ABI/licença, compilação e comparação G-code **da aplicação Filuna** contra OEM por 2–3 modelos piloto e combinações compatíveis, testes de protocolo do próprio app de descoberta/portas/auth/status/upload ACK/multimaterial/câmera em simulador, carregador de perfis do dispositivo/catálogo assinado, assistente guiado e garantias de segurança. Somente depois: **build final leve e beta vigiado**, sem homologação física antecipada.

**Fase física:** apenas beta, sob comando explícito do cliente; 0 testes completos → liberada para TESTE; 1–2 → parcialmente homologada; 3 independentes e completos com todos os critérios aplicáveis → homologada tecnicamente por modelo/coorte. O status técnico beta não publica automaticamente software estável, não comanda impressoras nem substitui revisão de segurança.

**Governança:** PR draft #6 ainda isolado, sem merge, release ou deploy; nenhum dos três fabricantes marcado como integralmente homologado. Este arquivo é checkpoint com os estados vistos no momento, não monitor automático.


## Checkpoint posterior — três modelos Anycubic e bloqueio da segurança U1 (07/11/2026)

**Anycubic Slicer Next 2.0.0.6:** correção do bug do *próprio runner* (\`dest\` versus \`out\` ao abrir os dois arquivos G-code) aplicada no commit \`1ac0e678\`. [CI **#42323143169**](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42323143169) e rerun [**#42323305372**](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42323305372) **SUCCESS**. Kobra S1 0,4 = **210094 B** A/B hash bruto idêntico; Kobra 3 V2 0,4 = **325731 B** A/B idêntico; Kobra 4 0,4 = **215690 B** A/B idêntico. O comparador executou **8 testes negativos/positivos**: diferenças funcionais não podem ser normalizadas. \`3_SAMPLE_OEM_CLI=PASS\`, \`NATIVE_FILUNA_PARITY=NOT_RUN\`, \`ANYCUBIC_DEVICE_NETWORK=NOT_RUN\`. 3 modelos de QA **não** são whitelist, suporte a toda marca ou homologação interna.

**Snapmaker U1:** CPU multiprocessador versus fixação **real de afinidade Windows a 1 único núcleo** testadas em [#42323435503](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42323435503): **3 variantes de G-code em 24 execuções normais** e **2 variantes em 24 execuções num núcleo**. Portanto, a hipótese de cura com CPU único **não passa**. No experimento [#42323700744](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42323700744), com **80 slicings OEM reais** organizados em 20 original, 20 sem wipe, 20 com costura nearest, 20 ambos, todos os grupos falharam com **2, 3, 2, 3 versões respectivamente**; mudança de perfil **não** é fix homologado e não é incorporada à produção.

**Novíssimo bloqueio P0 de colisão U1 multicolor:** upstream [Snapmaker/OrcaSlicer issue #1149](https://github.com/Snapmaker/OrcaSlicer/issues/1149) **aberta**; versão 2.4.1 pode mover o bico sobre o topo da peça sem Z-hop ao terminar purga da torre. Código responsável \`src/libslic3r/GCode.cpp/WipeTowerIntegration::append_tcr2\`; origem do delta documentada pelo upstream. Candidate \`FILUNA/fabricantes/snapmaker/patches/0002-candidate-final-purge-z-lift-safety.patch\` [#42323500007](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42323500007) **PASS APLICAÇÃO/REVERSÃO DO PATCH SOMENTE**, **NÃO** compilado, não testado com G-code de U1 e firmware nem ativado. Não assumir segurança resolvida.

**Flashforge:** permanece [**#42321095370**](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321095370) **PASS OEM original** nas duas amostras AD5X (hash bruto igual) e Adventurer 5M (somente timestamp de cabeçalho diferente), com 6 testes negativos do comparador; **sem** integração nativa LAN/câmera/IFS/ACK ou paridade Filuna. Flash Studio 1.7.9 Linux não substitui verificação da linha atual 1.7.20.

**Implantação leve/assistente beta:** protótipo de dados por impressora sob demanda [7/7 testes #42321919685](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321919685) PASS apenas para integridade/filtro de dados, ainda não plugin Windows assinado/baixado. O simulador do **contrato de assistente/3 testes físicos** [15/15 testes #42323736471](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42323736471) PASS **sintético apenas** (zero hardware). Nenhum registro simulado conta no contador físico. \`HOMOLOGAÇÃO_INTERNA_DE_TODAS_AS_3=BLOCKED\`; testes nativos e de rede não substituíveis por CI da CLI oficial, nem por documentação. Beta e prints físicos somente após gates e build final.


## Gate negativo real da aplicação Filuna e proteção antiaprovação indevida (07/11/2026)

O [CI #42324852127](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42324852127) executou o **portátil Windows pré-OEM Filuna existente** de [#42102469888](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42102469888), sem compilar novas versões, com **perfis/G-code oficiais já verificados** para Kobra 3 V2, AD5X e Snapmaker U1. Caminhos foram convertidos em absolutos antes do teste. **Nenhum G-code Filuna foi gerado**, por bloqueios reais:

- **Anycubic:** \`can not resolve preset ... process.json: Preset was not found in the loaded bundle\`.
- **Flashforge:** \`can not resolve preset ... machine.json: Preset was not found in the loaded bundle\`.
- **Snapmaker:** \`machine_start_gcode Parsing error at line 10: Not a variable name {if chamber_cooling_mode==0}\`.

Esses erros pertencem ao **Filuna pré-integração**, não aos executáveis OEM oficiais; demonstram lacunas de carregamento sob demanda de presets e compatibilidade de macros que precisam de código nativo. O job foi marcado SUCCESS por **coletar os diagnósticos**, não porque o Filuna tenha aprovado os 3 modelos. **\`FILUNA_GCODE_PARITY_3=FAIL/NOT_READY\`**.

Foi criado o bloqueio preventivo \`FILUNA/tools/oem_three_brand_promotion_guard.py\`, executado no [CI #42324838562](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42324838562), **9/9 controles positivos/negativos PASS**. Ele fiscaliza os manifests e arquivos reais dos adaptadores C++ para impedir declarar homologação interna enquanto ainda instanciam \`PendingVendorAdapter\`, sem provas de G-code Filuna, rede/ACK e segurança, incluindo o bug U1 #1149. **PASS do verificador significa que as marcas continuam corretamente bloqueadas, NÃO homologadas**.

**Contagem atual:** Anycubic 3 amostras OEM original A/B PASS, Flashforge 2 amostras OEM original A/B PASS, Snapmaker U1 G-code variável tanto 2.4.0 quanto 2.4.1 e risco de Z-hop #1149. **Homologação interna Filuna completa: 0/3**. **Homologação física: 0/3** (somente beta futuro). Não publicar release, não liberar impressão nem promover PR #6 por causa do resultado do verificador.


## Checkpoint novo: 100 fatiamentos oficiais adicionais e comparação nativa Filuna (07/11/2026)

**A homologação interna continua 0/3.** Teste do programa oficial, simulador de LAN e diagnóstico do executável Filuna antigo são gates diferentes.

### Anycubic: três amostras, 60 execuções adicionais PASS OEM
- [Workflow #42325230868](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42325230868) SUCCESS: Kobra S1 20/20, Kobra 3 V2 20/20, Kobra 4 20/20, além dos pares de referência originais. G-code idêntico fora da linha de data/hora comprovada quando aplicável; nenhuma diferença funcional autorizada.
- [Filuna pre-OEM #42325032419](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42325032419): perfil OEM não reconhecido no preset bundle. Após cópia TEMPORÁRIA de perfis já resolvidos como User (somente para diagnóstico), a CLI rejeita o valor de retraction_distances_when_cut=0 por intervalo Orca 10–18. Não alterar retracção para forçar aprovação. G-code nativo Anycubic do Filuna: NOT_GENERATED; OEM_ADAPTER: PENDING.

### Flashforge: duas amostras, 40 execuções adicionais PASS OEM
- [Workflow #42325288710](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42325288710) SUCCESS: AD5X 20/20; Adventurer 5M 20/20; comparação estrita que desconsidera somente data/hora OEM comprovada.
- [Rede simulada #42324056247](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42324056247): 10/10 PASS de protocolo local somente leitura (checkCode/detail/product), com fonte documental pública de firmware comunitária; NÃO é o adaptador nativo, nem LAN físico, IFS ou câmera.
- [Filuna pre-OEM #42325032419](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42325032419): presets OEM não carregados pelo bundle nativo. Após cópia TEMPORÁRIA já resolvida como User, gerou **G-code Filuna real de 214.319 bytes/9.048 linhas versus OEM AD5X 209.677 bytes/8.905 linhas**. Há diferenças reais de macros/trajetórias; sem paridade. Não é homologação da AD5X, mesmo com CLI verde.

### Snapmaker: duas versões oficiais falham no teste prolongado
- [OEM v2.4.0 #42324349445](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42324349445): 30 processos originais independentes e 4 hashes G-code funcionais distintos, **FAIL**. A v2.4.1 já falhou em 40 execuções e nas análises de afinidade CPU e costura/wipe. Não ocultar divergência de retração/movimento.
- [Gateway local simulado #42324207144](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42324207144): 14/14 PASS de health, cache, modelo e bicos, mas o serviço gateway do aplicativo NÃO é conexão LAN da impressora nem transporte real do Filuna.
- [Filuna pre-OEM #42325032419](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42325032419): parser não reconhece a variável OEM chamber_cooling_mode no G-code inicial U1; não gerou G-code. A correção fonte apenas do issue #1149 de purge/Z-hop teve teste git apply PASS, não execução.
 
### Base Filuna e requisito de beta

[On-demand data 7/7 #42321919685](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42321919685) e [assistente/3 provas beta 15/15 #42323736471](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42323736471) são testes isolados, sem API assinada, interface vinculada, impressão física ou motor nativo homologado. [Anti-falsa homologação 9/9 #42324838562](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42324838562) confirma que os três adaptadores C++ ainda instanciam PendingVendorAdapter, sem integração OEM completa. O PR #6 deve ficar draft e não deve ser promovido, compilado em instalador final ou utilizado para liberar a primeira impressão física. Status: HOMOLOGACAO_INTERNA=BLOCKED para 3/3, BETA_FISICO=NOT_RUN para 3/3.


## Continuação pós-100 amostras OEM — diagnóstico real do Filuna pré-OEM (07/11/2026 04:00 UTC)

**Execução nova e auditável:** [GitHub Actions #42336460665](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42336460665), commit `73bb413a7d17d21b61a6b9fcb7d8d8fadaee336e` da branch DRAFT, **SUCCESS** como execução de laboratório, sem recompilar instalador, motor nem OEM. Os 3 testes existentes e os **7 novos testes negativos/positivos** passaram. O artefato `filuna-existing-windows-base-versus-3-official-oem-gcodes` (ID 13560140347) contém os logs, `verdicts.json` e `functional-diagnostic.json` para cada marca.

**Mudanças realizadas só no laboratório:** `lab/oem_functional_gcode_diff.py` (`d3c9267`), `lab/test_oem_functional_gcode_diff.py` (`dd70600`) e workflow `.github/workflows/filuna-existing-portable-vs-three-oem.yml` (`73bb413`). O diagnóstico calcula SHA bruto, SHA de comandos excluindo comentários **somente para localizar divergências**, quantidades por família e o primeiro comando diferente. A exclusão de comentários **não pode** ser usada para aprovar paridade: alguns firmwares usam comentários para semântica. Mesmo uma amostra idêntica é sempre `DIAGNOSTIC_ONE_SAMPLE_IDENTICAL_NOT_HOMOLOGATED`, nunca PASS OEM. Nenhum arquivo do fabricante foi alterado.

### Resultado Filuna real contra OEM oficial, preservando falhas

- **Anycubic Kobra 3 V2 0,4:** `BLOCKED_FILUNA_GCODE_NOT_GENERATED`. CLI original sem presets reconhecidos no bundle; o experimento temporário de dados efetivos User falhou em `retraction_distances_when_cut: 0 not in range [10, 18]`. Nenhum ajuste de retração foi feito e não existe G-code Filuna comparável. Necessária identificação exata de tipo/esquema/semântica de preset e código nativo.
- **Flashforge AD5X 0,4:** `FAIL_GCODE_COMMAND_STREAM_DIFFERENT`. OEM Flash Studio 1.7.9 = **209.677 bytes, SHA256 `68ed51e6e830ed7ad1d2159a26640b4d54d046f857851f3ad9054b3d045063d6`**. Filuna portátil C++ pré-integração com perfil efetivo temporário User = **214.319 bytes, SHA256 `254904898873768d77bca7f2c03387c92fe6b8ac308120d330fcdf932c65cf70`**. Comandos não comentados = **6.564 OEM vs 6.563 Filuna**. **G1: 5.658 vs 5.652**, `SET_VELOCITY_LIMIT`: **373 vs 379**; `M106`: **9 vs 8**. O primeiro comando divergente é `M73 P5 R15`, ausente naquele ponto da saída Filuna. Há divergências reais de E/velocidade como `G1 X101.185 Y101.185 E.02839` contra `E.0284` e `G1 F7947` contra `G1 F7970`; não ignorar arredondamentos, movimentos ou macros. Paridade `NOT_PROVEN`.
- **Snapmaker U1 0,4:** `BLOCKED_FILUNA_GCODE_NOT_GENERATED`; parsing `machine_start_gcode` na linha 10, expressão `{if chamber_cooling_mode==0}` não resolvida. Diagnóstico não contorna macro oficial. Independentes deste bloqueio permanecem FAIL de reprodutibilidade oficial em 2.4.0/2.4.1 e risco crítico aberto de purga/Z-hop (#1149). A falta de G-code do Filuna não pode ser confundida com aprovação da U1.

### Decisão de engenharia após o novo teste

1. Resolver **carregamento nativo de presets OEM por modelo/firmware/bico** com heranças e esquema verificados, sem empacotar todos os perfis de fabricante no instalador e sem converter presets `User` como integração definitiva.
2. Inspecionar origem C++ das diferenças Flashforge (velocidade, extrusão, ventilação, `M73`) e implementar apenas deltas legais necessários sobre o único Orca compartilhado; comparar 3MF, perfis e G-code byte a byte, com stress por modelo e testes negativos.
3. Corrigir depois a avaliação do contexto da macro da U1 e testar seu caminho de purga/Z-hop com prova de segurança; não usar perfil alterado para mascarar o defeito. Simultaneamente manter testes de rede/ACK/câmera/multicor simulados como gates separados do executável real.
4. `HOMOLOGACAO_INTERNA_FILUNA=BLOCKED (0/3)`, `PARIDADE_NATIVA=FAIL_NOT_READY`, `HOMOLOGACAO_FISICA=NOT_RUN`, `BAMBUDDY_AUTH=OWNER_DEFERRED`. A fase física será no beta vigiado, após gates internos e compilação final. **Não fazer merge, release, deploy, instalar motor OEM completo ou iniciar impressão física.**

**Site e servidor:** consulta somente leitura ao SentinelX em 07/11: diretório legado `/srv/site` preservado; `https://filuna3d.com/filuna-slicer-dev/` respondeu HTTP **401** sem autenticação, comportamento compatível com a rota beta privada protegida. Essa resposta não certifica funcionamento interno do app, login nem release; nenhum deploy foi executado.


## Correções OEM nativas candidatas — 07/11/2026 (sem compilar versões intermediárias)

**Ordem de escopo:** resolver falhas encontradas no motor Filuna, sem trocar preset do fabricante, sem G-code pós-processado para forçar igualdade, sem distribuir motor OEM completo. O código-fonte do Orca Filuna foi alterado **apenas na branch DRAFT**. Os testes de fonte foram executados, mas **o executável Filuna imutável de 05/11 ainda não incorpora estas alterações**: qualquer diagnóstico obtido dele continua sendo a linha-base anterior, não prova do patch.

### 1. Anycubic — valor 0 com corte longo desativado

- Evidência real: Kobra 3 V2 oficial, `retraction_distances_when_cut=["0"]`, `long_retractions_when_cut=["0"]`, `enable_long_retraction_when_cut=0`; Filuna portátil anterior recusava `0 not in range [10,18]`.
- Código candidato: `src/libslic3r/PrintConfig.cpp` aceita **zero sem alterar seu valor** no esquema. `src/libslic3r/Print.cpp` mantém gate obrigatório de **10 a 18 mm se a opção estiver ativa**. Correção independente: o vetor de override nullable de retração era preenchido por engano no vetor de outra opção, agora corrigido.
- **Garantia limitada:** o teste unitário de inspeção da fonte passa; o schema sozinho não garante que a máquina carregue o preset, nem que o Filuna gere G-code ou seja equivalente ao Anycubic 2.0.0.6. Verificação real do motor recompilado final continua `NOT_RUN`.

### 2. Snapmaker U1 — contexto original da macro `chamber_cooling_mode`

- Fonte oficial auditada: `Snapmaker/OrcaSlicer` commit `bac6fd5c1e1c23ddf057cf6471ba4ed7ed621c4c`, `src/libslic3r/GCode.cpp`, cálculo do modo de resfriamento a partir de `filament_is_high_temperature` e `temperature_vitrification` de cada filamento em uso.
- Código candidato: adicionada definição `filament_is_high_temperature` ao esquema/default, e injetado `chamber_cooling_mode` imediatamente antes de processar o `machine_start_gcode`, **somente quando o template pede essa variável**. Valores OEM: 0 = conservar aquecimento, 1 = resfriamento fraco em temperatura de amolecimento até 70 °C, 2 = resfriamento forte até 50 °C. O perfil oficial de teste U1 PLA (45 °C, high-temperature=0) seleciona modo **2** no vetor de referência.
- **Garantia limitada:** somente inspeção estática e casos unitários de regra; não há ainda saída G-code do Filuna com o patch, nem segurança comprovada. A questão upstream de purga/elevação Z `Snapmaker/OrcaSlicer#1149` e a **não-repetibilidade em 2.4.0/2.4.1** seguem bloqueios separados. Não acionar U1 real.

### 3. Flashforge AD5X — diferença de trajetória verdadeira, não apenas cabeçalho

- Comparação de arquivos **efetivamente gerados**: oficial 1.7.9 `209677` bytes / SHA `68ed51e6e830ed7ad1d2159a26640b4d54d046f857851f3ad9054b3d045063d6`; Filuna pré-OEM `214319` bytes / SHA `254904898873768d77bca7f2c03387c92fe6b8ac308120d330fcdf932c65cf70`.
- G0–G3: **5759 movimentos oficiais vs 5753 Filuna**; G1 **5658 vs 5652**; M73 110 vs 110 (deslocados de posição), `SET_VELOCITY_LIMIT` 373 vs 379, M106 9 vs 8. Primeiro movimento divergente: **E.02839 no oficial vs E.0284 no Filuna**. Em posições seguintes, velocidades 7947 vs 7970, geometrias XY e trajetórias diferem. Não é apenas arredondamento, pois existem deslocamentos reais no plano.
- Atualizado o comparador `lab/oem_functional_gcode_diff.py` para registrar hashes e divergência dos movimentos **sem reescrever saídas**; adicionados testes negativos para rejeitar falso PASS mesmo se só movimentos coincidirem e a temperatura diferir.
- **Correção de paridade do motor ainda NÃO aplicada**: requer porte e teste dos deltas do gerador/planner da fonte oficial, auditoria de ABI e licença; não converter G-code OEM em suposto G-code Filuna, nem manipular postprocessor para esconder divergência.

### Evidência e gates

- [CI fonte #42337828214](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42337828214): **9/9 testes de fonte PASS**, incluindo os vetores da macro oficial Snapmaker, corte longo ativo/inativo e proteção contra falso PASS. **Não compila nem executa o motor novo.**
- [CI diferencial Windows inicial #42336460665](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42336460665): teste real do **portátil anterior**, não dos patches C++ da branch; fonte do corpus e relatórios preservados.
- Teste diferencial com análise detalhada de movimentos: CI `.github/workflows/filuna-existing-portable-vs-three-oem.yml` acionado por atualização do comparador; status sujeito à inspeção da execução correspondente (nunca inferir paridade de `SUCCESS`).
- `SOURCE_PATCH_ANYCUBIC=IMPLEMENTADO_NAO_COMPILADO`; `SOURCE_PATCH_SNAPMAKER_MACRO=IMPLEMENTADO_NAO_COMPILADO`; `FLASHFORGE_NATIVE_MOTION_PARITY=FAIL`; `ALL_THREE_INTERNAL_HOMOLOGATION=BLOCKED`; `DEVICE_NETWORK=NOT_RUN`; `PHYSICAL_BETA=NOT_RUN`; `BAMBUDDY_AUTH=OWNER_DEFERRED`.
- A sequência permanece **gates internos → compilação final → beta vigiado com validação física → aprovação estável**, sem reclassificar patch estático como homologação. Nenhum deploy de site/instalador/dispositivos foi efetuado.

 
## Rastreio da triagem funcional Flashforge e prova dos dois bloqueios de CLI (07/11/2026, 04:27 UTC)

**Workflow real [#42338506954](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42338506954)**, SHA `273cd1a5709ba019dcf14964766f9088240a2329`, preservou executável portátil Filuna antigo e G-code oficial Flashforge 1.7.9, sem recompilar. O laboratório passou nos **8 novos testes de classificação** de movimento (arco, XYZ, extrusão, feedrate, temperatura, fan, ausência de G-code e bloqueio de falso PASS), além dos testes anteriores. O artefato `filuna-existing-windows-base-versus-3-official-oem-gcodes` (ID 13559854682) guarda `flashforge/flashforge-motion-triage.json`, `model-manifest-probe/verdicts.json` e logs reais.

**Modelo de fonte OEM sob demanda — ensaio com um único modelo, SEM parâmetros inventados:** o probe criado no mesmo PR gerou arquivos de fonte temporários com presets efetivos oficiais por máquina, mantendo valores de temperatura, retração, velocidade, G-code e herança já resolvida. **Não é um carregador de produção**, mas confirmou no binário pré-OEM:
- Anycubic Kobra 3 V2: sistema carrega até a validação e recusa `retraction_distances_when_cut: 0 not in range [10,18]`. Patch C++ de aceitar zero desativado está na branch e não no binário antigo; **ainda não revalidado em executável gerado do código novo**.
- Snapmaker U1: presets carregam até a macro e o parser antigo não reconhece `chamber_cooling_mode`. Patch C++ de macro está na branch, mas **ainda não revalidado em executável gerado do código novo**. U1 segue também bloqueada por não-repetibilidade do próprio OEM e risco de purga/Z-hop.
- Flashforge AD5X: fonte temporária por máquina produz G-code do Filuna portátil, **mas não igual ao oficial**. A propriedade `from=system` e a indexação de um só modelo não corrigem o planejador; nome/tipo/perfil não são a explicação suficiente.

**Triagem por trajetória original, sem arredondar ou ocultar desvios:**
- Movimentos `G0/G1/G2/G3`: **5759 oficial versus 5753 Filuna**.
- Na comparação espacial por camada, **182 extrusões numéricas diferentes, 374 diferenças de feedrate, 534 grupos de edições geométricas**, atingindo **100 de 100 camadas**. Os grupos de edição não são defeitos independentes e não provam 534 caminhos diferentes; a contagem é diagnóstico de alinhamento de sequências, preservado em JSON com primeiro exemplo por categoria.
- Diferenças incluem feedrate, E, XY/arc, fan `M106` e planejamento. Não normalizar nem editar G-code gerado para falsificar paridade. A solução exige portar e revisar os deltas de geração e planejamento da versão OEM Flashforge sobre o motor Orca compartilhado, com proteção por fabricante/modelo/firmware e regressão total.

**Decisão:** `ANYCUBIC_SOURCE_CANDIDATE=IMPLEMENTADO_NAO_EXECUTADO`, `SNAPMAKER_MACRO_SOURCE_CANDIDATE=IMPLEMENTADO_NAO_EXECUTADO`, `FLASHFORGE_NATIVE_PARITY=FAIL`, `THREE_OEM_INTERNAL_HOMOLOGATION=0/3`. Corrigir fonte e comparar binário Filuna real com provas repetidas antes de promoção. Não publicar motor OEM inteiro, pacote final, website ou impressora real; beta físico permanece após gates internos e compilação final. Bambuddy autenticado continua adiado por ordem do proprietário.


## Prova adicional: carregamento com manifesto mínimo por modelo (07/11/2026 04:28 UTC)

Para distinguir **resolução de caminho de perfil** de **incompatibilidade efetiva do motor**, foi escrito o laboratório `lab/oem_model_scoped_manifest_probe.py` (commit `500c6bb`) com 7 testes positivos/negativos `lab/test_oem_model_scoped_manifest_probe.py` (commit `525e07b`); o workflow Windows existente foi ampliado em `2fbc42b`. Os perfis originais OEM e seus SHA são preservados. O ensaio cria **exclusivamente no runner** um manifesto mínimo `<Fabricante>.json` e três presets `from: system` com os valores finais oficiais **inalterados**; somente o metadado `inherits` é apagado nas cópias EFETIVAS já resolvidas. Não usa a conversão para `User`, não altera retração, macros, velocidade, temperatura ou material, não distribui catálogos de outros modelos e **não** é módulo de produção.

**GitHub Actions real:** [#42338506954](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42338506954) SUCCESS, artefato `filuna-existing-windows-base-versus-3-official-oem-gcodes` (ID `13559854682`). Foram aprovados **7/7** testes novos de manifesto, **9/9** diferenciais anteriores, **3/3** baseline e **8/8** de análise espacial Flashforge de outro commit da mesma branch. Isso comprova apenas execução e diagnóstico corretos, não paridade.

| Modelo | CLI anterior invocada com manifesto por modelo | Falha real |
| --- | --- | --- |
| **Anycubic Kobra 3 V2 0,4** | Chegou à validação real dos valores sem erro `Preset was not found` | `retraction_distances_when_cut: 0 not in range [10, 18]`, exit não-zero, **0 G-code**. O patch C++ candidato amplia mínimo do **esquema** para 0 com opção desabilitada, e mantém 10–18 se ativa; sem binário corrigido ainda |
| **Flashforge AD5X 0,4** | **Gerou 1 G-code**, exit 0 com a árvore mínima e perfil OEM efetivo | **FAIL_GCODE_COMMAND_STREAM_DIFFERENT**. OEM 209.677 B, Filuna 214.319 B; 5.759 movimentos OEM vs 5.753 Filuna, caminho/velocidade/extrusão divergentes. A prova espacial detectou `geometry_edit_groups=534`, `extrusion_different=182`, `feedrate_different=374`, `mismatching_layers=100`. Isso exige porte real de algoritmo do fabricante após revisão, não só de perfil |
| **Snapmaker U1 0,4** | Chegou à geração do start G-code sem erro `Preset was not found` | `machine_start_gcode Parsing error ... chamber_cooling_mode`, exit não-zero, **0 G-code**. O código-fonte C++ candidato calcula e fornece a macro, mas o portátil anterior não contém a mudança; paridade não testada e risco de Z-hop e variabilidade OEM seguem ativos |

**Conclusão do runner:** corrigida no laboratório a **forma de empacotar e localizar dados de perfis**, não corrigidos os três motores executáveis do Filuna. A falha de Anycubic e Snapmaker permanece no executável antigo e requer teste de patch em binário futuro. A Flashforge exige revisão e porte dos deltas do planner/fan/G-code do release oficial, com preservação de valores e sem pós-processamento que fabrique equivalência. **`NATIVE_OEM_PARITY=0/3`, `INTERNAL_HOMOLOGATION=0/3`, `DEVICE_NETWORK=NOT_RUN`, `PHYSICAL_BETA=NOT_RUN`.** Continuam proibidos merge, release e impressão real; Bambuddy autenticado permanece adiado por ordem do proprietário.
