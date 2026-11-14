# Auditoria de diferenças — Snapmaker U1, 0.4 mm

**Origem comparada:** `Snapmaker/OrcaSlicer` tag `v2.4.1`, contra a árvore herdada de perfis Filuna, ambas em GitHub. Comparação JSON por chave, sem normalização de valores. Regra: **não promover o perfil herdado do Filuna como igual ao OEM**.

| Perfil | Blob OEM v2.4.1 | Blob antigo Filuna | Chaves com valores distintos/ausentes |
|---|---|---|---:|
| Máquina U1 (0.4 nozzle) | `d9a6ab12cb872af25275fab6045b05cfc4531e2f` | `08e33dea0c6c8a7dbecf9ff9b80188b5af284131` | **40** |
| Processo standard U1 | `932a52d485a20ea2042716b366733f48f40facfc` | `6db6d0281521dd82233450f8ad06abce44207f05` | **27** |
| Filamento Snapmaker PLA Basic U1 | `8e4ba5984b072578b82c842b7b465947bf82cfb2` | `040cfbe8501fde679398e498dadf67caf49c9eea` | **57** |

**Diferenças de risco técnico detectadas:**

- **Máquina:** `machine_start_gcode`, `machine_end_gcode` e `change_filament_gcode` divergem. O perfil antigo insere `SET_PRINT_AUTO_BED_LEVELING ENABLE=1` e `SET_TIME_LAPSE_CAMERA ENABLE=1` antes do `PRINT_START`; não assumir equivalência OEM em segurança, comando inicial, macros ou sequência.
- O **número de elementos por extrusora** difere em vários campos: no original U1, vetores de retração têm **4 valores**; no perfil antigo Filuna, até **8 valores**. Alterações de comprimento sem semântica comprovada podem mapear o motor/filamento errado.
- `nozzle_type` e `nozzle_volume` são scalar no OEM e arrays no histórico Filuna. Tolerância/ABI dessas opções deve ser validada pelo motor real e pelo diálogo de perfis.
- **Processo:** OEM `0.20mm Standard @Snapmaker U1 (0.4 nozzle)` versus Filuna antigo `0.20 Standard @Snapmaker U1 (0.4 nozzle)`; também divergem `support_type`, velocidades, `enable_arc_fitting`, `precise_outer_wall`, `seam_gap` e dados de variação de extrusor. Não misturar nomes silenciosamente.
- **Filamento:** diferenças em arrays de temperaturas, fluxo, retração, pressão, refrigeração e `setting_id`. Mesmo valores numéricos podem ter **cardinalidade diferente**, portanto não presumir igualdade.

**Ação implementada:** os três JSON originais do OEM v2.4.1 foram copiados byte-exatos para `fixtures/oem/snapmaker/official-profiles/{machine,process,filament}.json`. Cada um é comprovado por seu blob Git original; não alteram o perfil ativo de nenhum cliente. O runner para U1 lê o **objeto Git original** no Windows para evitar mudanças de quebra de linha feitas pelo checkout.

**Portões pendentes:** prova de CLI oficial A/B, comparação do mesmo projeto/processo/material com motor Filuna, integração UI e contrato OEM de rede em simulador. A **validação física** será no beta vigiado, após todos os testes internos e a compilação final (decisão de 07/11/2026). Não declarar homologação do modelo por inventário de perfis.
