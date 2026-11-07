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
