# Incidente técnico — Snapmaker U1: 24 horas sem homologação

**Data:** 2026-11-07. **Escopo:** PR #6 DRAFT, branch `filuna/oem-portable-first-qa-20261106`. **Estado:** NÃO HOMOLOGADA. **Registro:** cobrança do proprietário após aproximadamente 24 horas de investigação; duração declarada pelo proprietário, não inferida de timestamps de CI.

## Evidências verificadas
- [Stress OEM Windows v2.4.1, run #42396616586](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42396616586): 20 invocações independentes com entrada/presets fixos; 18 trajetórias do grupo A (SHA funcional `1fffba21d2cce0e0456f69836e3e8b5f22aaa35e955ddcb8305627457aca12d1`) e 2 do grupo B (`b42e9a25f5440cf7e0b7d422f36855506c8ed71a2a73e7f66dd1b479d401fa7a`). Divergências em G0–G3, E, F e arcos, não apenas timestamp. `OEM_FOUR_TOOL_STRESS_REPEATABILITY=FAIL`.
- Validação geométrica do G-code OEM original identificou risco de cruzamento da peça durante purga final. `U1_FOUR_TOOL_SAFETY=BLOCKED`. O [run #42396347068](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42396347068) passou na **detecção** do bloqueio, não na segurança do G-code.
- 3MF do próprio fabricante e exportação 3MF pela CLI apresentaram `0xC0000005` nos testes documentados; 3MF Core sintético corrigido pôde gerar G-code. `U1_ORIGINAL_BBS_3MF=FAIL`, `MINIMAL_3MF_OEM_SLICE=PASS` (escopo sintético).
- [Windows GUI #42394738274](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42394738274): dependências e cache PASS; etapa de compilação GUI FAIL. Correção CMake previamente relatada no commit `d7762d13ae915387623ad0787bf2a9d1af90adf0`, sem evidência nesta verificação de nova compilação completa PASS.

## Diagnóstico e plano de desbloqueio
1. Separar a variabilidade de trajetórias da segurança geométrica; comparar ambas as saídas originais sem editar o G-code OEM e investigar fonte/configuração/ordenação não determinística.
2. Reproduzir a colisão potencial no scanner Python e C++ com as duas variantes, mantendo envio fail-closed; distinguir falso positivo geométrico de movimento perigoso com prova auditável, sem supor firmware físico.
3. Isolar crash da CLI em 3MF BBS com stack/minidump de runner QA, confrontar com 3MF Core aceito, e testar controles reproduzíveis sem mexer no arquivo oficial.
4. Reexecutar build Windows após correção CMake; obter G-code nativo Filuna com mesmo perfil/entrada e comparar movimentos, extrusão, velocidades, macros, toolchange e purga sem mascarar divergências.
5. Preservar `U1_DEVICE_NETWORK_ACK=NOT_RUN`, `U1_PHYSICAL_BETA=NOT_RUN`, `FILUNA_NATIVE_GENERATOR_PARITY=NOT_PROVEN`, `U1_INTERNAL_HOMOLOGATION=BLOCKED`, `G4-I=BLOCKED`.

## Restrições
Somente testes, fonte e CI na branch QA. Um motor Orca com diferenças OEM. Proibido modificar G-code oficial para forçar paridade, alterar servidor, efetuar merge/deploy/release/instalador, acessar impressora ou imprimir, executar Bambuddy ou promover CI sintético a homologação física. Teste físico somente no beta com autorização expressa do proprietário.

**Conclusão:** após aproximadamente 24 horas reportadas, Snapmaker U1 continua BLOCKED por falhas concretas e independentes. Não registrar PASS geral nem mascarar a reprovação.