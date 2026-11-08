# Snapmaker U1 — Native four-tool parity ledger (2026-11-08)

## Scope and evidence
- Branch: `filuna/oem-portable-first-qa-20261106`, PR #6 DRAFT.
- Filuna executable: Windows QA artifact from run `42421322858`; original Snapmaker OEM 2.4.1 portable CLI, SHA-verified.
- Last complete Filuna proof: [run 42431441498](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42431441498), commit `a4cf8e895cf2d437673e7fc15c52b077e2f8804a`; jobs `slice-with-filuna` and `native-cxx-validator` succeeded.
- OEM 20-run grid fixture: [run 42431084587](https://github.com/abcollicelli-web/FilunaSlicer/actions/runs/42431084587); 20/20 completed, two functional variants, matching existing OEM references `1fffba21d2cce0e0456f69836e3e8b5f22aaa35e955ddcb8305627457aca12d1` and `b42e9a25f5440cf7e0b7d422f36855506c8ed71a2a73e7f66dd1b479d401fa7a`.
- Evidence: artifact `filuna-u1-native-fourtool-proof`, `proof.json`, Filuna G-code and logs, native C++ job log. No hardware, network ACK, physical print or release validation.

## Input and arrangement
The QA fixture is built by `build_four_from_core_3mf` in `lab/snapmaker_u1_fourtool_original_qa.py`. Four independent 20-mm cubes retain distinct extruder assignments 1–4. Their 3MF transforms are at Z=10 mm, centers:
1. Tool 1: (146.5, 147), polygon [[136.5,137],[156.5,137],[156.5,157],[136.5,157],[136.5,137]].
2. Tool 2: (146.5, 125), polygon [[136.5,115],[156.5,115],[156.5,135],[136.5,135],[136.5,115]].
3. Tool 3: (124.5, 147), polygon [[114.5,137],[134.5,137],[134.5,157],[114.5,157],[114.5,137]].
4. Tool 4: (124.5, 125), polygon [[114.5,115],[134.5,115],[134.5,135],[114.5,135],[114.5,115]].

The `EXCLUDE_OBJECT_DEFINE` NAME, CENTER and POLYGON lines match byte-for-byte between Filuna and OEM A/B for all four objects. Filuna uses a local `--arrange 0` CLI helper, leaving the OEM shared `run_cli` untouched. OEM results remain the two established functional SHA variants. Do not modify or normalize G-code to force a match.

## Safety (last completed proof)
- `status=SAFE_BUT_DIVERGES_FROM_OEM`.
- `final_purge.status=PASS_Z_LIFT_BEFORE_XY`: layer Z before = 20.05 mm; first lift Z=20.267 mm at line 122279, coincident with first XY travel at that raised Z; last toolchange marker line 122312.
- Python spatial validator: `PASS`, zero issues.
- C++ native validator: `NATIVE_STATUS=PASS ERRORS=0 WARNINGS=0 OBJECT_EXTRUSIONS=14825 TOWER_EXTRUSIONS=46259 REASON=NO_DETECTED_RISK_WITHIN_SCOPED_ANALYSIS`.
- This is *scoped static safety*, not proof of physical/firmware safety.

## Remaining OEM functional differences
- OEM A: `edit_groups=4811`, `similarity_ratio=0.122865`, `first_divergence` at Filuna/OEM functional line 99.
- OEM B: `edit_groups=4717`, `similarity_ratio=0.120804`, same first divergence at line 99.
- First mismatch: Filuna `M109 S220 T0` (wait for nozzle temperature), OEM `M104 S220 T0` (set temperature without waiting). These are **not semantically interchangeable**. Identify whether this comes from startup G-code, preset inheritance, engine behavior, or order of generated commands before changing anything.
- Large remaining movement differences (Filuna 97,478 moves; OEM A 79,629; OEM B 79,477). Fixing only the first command does not establish parity.

## Controlled continuation / acceptance gates
1. Inspect the two G-codes and their complete early startup sequences, including surrounding temperature commands and effective OEM/Filuna presets. Attribute each mismatch to an exact source location and setting; distinguish legitimate safety differences from unported OEM behavior.
2. Make a minimal *native* Snapmaker-specific correction only with evidence; do not postprocess G-code, change OEM binaries, or break other manufacturers. Keep the licensed Orca core accessible.
3. One commit per isolated hypothesis/test. Never push while a workflow run is in progress; inspect completion and artifacts before another push. No merge, release, deploy, network action or physical print.
4. Re-run identical fixture with `--arrange 0`, both pinned OEM A/B comparisons, Python and native C++ validators; preserve full evidence and SHA values.
5. Accept parity only when all four `EXCLUDE_OBJECT_DEFINE` lines match, scoped safety remains PASS, and any remaining functional differences are *exhaustively classified* and specifically authorized. A high similarity ratio alone is not parity; differences confined to approved final-purge Z-lift must be demonstrated rather than assumed.
6. If validation fails or a mismatch remains unexplained, record BLOCKED and the next reproducible hypothesis. Do not report `PASS_PARITY`.

## Change history
- `c4dde9ea`: fixture coordinates changed to OEM grid.
- `c99136cd`: OEM 20-run test and functional variants.
- `a4cf8e89`: Filuna grid fixture, local `--arrange 0`, side-by-side OEM definitions.

## Follow-up investigation: early G-code, no motor change (2026-11-08)
Evidence directly inspected from artifacts of Filuna run `42431441498` and OEM run `42431084587`, G-code functional lines 81–145:
- Both outputs already contain `M109 S220` at line 81, `M104 S220 T1` at line 84, `T0` at line 97 and `G90` at line 98. Filuna then emits `M109 S220 T0` at line 99, OEM `M104 S220 T0`. Do not infer equivalent heating semantics from equal target temperature: one blocks, the other does not.
- The next independent mismatch is line 107: Filuna `SET_VELOCITY_LIMIT ACCEL=500 ACCEL_TO_DECEL=250 MINIMUM_CRUISE_RATIO=0.5`, OEM `SET_VELOCITY_LIMIT ACCEL=500 ACCEL_TO_DECEL=250`; repeated for other accelerations at lines 108, 115, 121, 123, 129, 137. This is another real Klipper command semantics difference, not a harmless textual change.
- The effective machine profile declares `gcode_flavor=klipper`; the Filuna C++ source `src/libslic3r/GCode.cpp` has a toolchange-temperature override path that filters selected `M109` commands. This establishes a **candidate source area**, not proof that this specific first-layer line was generated by that path. Inspect call sites, startup and generated toolchange sequence and the exact executable's build commit before any change.
- The existing Filuna executable is a previously built QA artifact from run `42421322858`, not automatically rebuilt from the current branch. Any future C++ change requires a new, pinned Windows executable build and explicit provenance; reusing the old artifact cannot validate the patch.
- Next gate: attribute line 99 and `MINIMUM_CRUISE_RATIO` to exact native source and conditions; compare thermal/acceleration safety consequences; add focused regression tests for Snapmaker only; build a new native executable; re-run OEM A/B plus final-purge and both safety validators. Preserve all old and new SHA evidence. No physical tests or release.
