# Round 10 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 8/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. The Round 10 bilayer-twist preprocessor work is real, and it closes the two concrete Round 9 twist-path parity gaps. The `nborder >= 2` override is now present in [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L317), the second-sheet rotation now happens before both `Def_Grad` and ghost-mesh generation in [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L411) and [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L457), the committed oracle/provenance exist in [test/cases/graphene_bilayer_twist_vdw_1000/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_bilayer_twist_vdw_1000/build_provenance.md), and the new archive-backed tests are wired in at [test/integration/test_prepro_oracle.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_prepro_oracle.cpp#L183).

2. Round 10 is still incomplete against the original plan because the entire simulator/runtime half of the project remains missing. The executable entry point is still only a stub in [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1), and the C++ tree still contains only preprocessor-side core modules plus MPI/I/O support. There are no translated Milestone 3-8 solver/runtime modules for Brenner, inner Newton, element energy, L-BFGS, pasapas, runtime vdW, checkpoint/restart, or VTU output. The only vdW implementation in-tree is still preprocessor initialization in [src/core/vdw_preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/vdw_preprocessor.cpp#L150). This leaves AC-5, AC-6, AC-7, AC-10, and AC-12 not met, and AC-8, AC-9, and AC-11 only partial.

3. AC-13 remains incomplete. `AGENT.md` and `document/translation_notes.md` are both still absent in the repository root/document tree. Claude’s own Round 10 summary admits that gap, so this round cannot be accepted as full plan completion even though the new preprocessor work is valid.

## Goal Alignment Check

- AC-1, AC-3, and AC-4 remain met.
- AC-2 stays partial: the preprocessor oracle surface is stronger after the new bilayer-twist case, but full acceptance still depends on closing `task2g`.
- AC-8 stays partial: preprocessor-side `nvdw=1` coverage now includes both the single-sheet self-contact case and the bilayer-twist local-density case, but simulator-side vdW/self-contact is still absent.
- AC-9, AC-10, AC-11, and AC-12 are still blocked by the missing simulator implementation.
- Forgotten items: none. The tracker still covers the original-plan tasks.
- Deferred items: none formally. The problem is not an unjustified tracker deferral; it is that Claude is still reporting large original-plan chunks as “remaining items” instead of finishing them.

## Goal Tracker Update Requests

Approved with correction.

- I updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md) for Round 10.
- I corrected the requested AC reference from `AC-3` to `AC-2, AC-8`; `task2g` is not an AC-3 item.
- I recorded the new bilayer-twist oracle evidence and `42/42` full-suite result.
- I removed the now-stale queued twist-preprocessor issue because Round 10 closed it.
- I narrowed the remaining `task2g` blocker to the unfinished runtime vdW/self-contact/simulator scope.

## Required Implementation Plan

Claude must now execute the remaining original-plan scope instead of deferring it:

1. Implement the Milestone 3 constitutive kernels in new dedicated core modules under `include/fce/` and `src/core/`: port `exponential.f90`, `geometry.f90`, `brenner.f90`, `Hyper_pot_inner_alg.f90`, `newton_inner.f90`, `ener_elem.f90`, and `principal.f90`; add focused unit tests for Brenner parity, second-derivative consistency, inner-Newton convergence/failure modes, and element-energy parity using new committed fixtures under `test/cases/`.
2. Replace the stub simulator mainline with a real driver: create solver/load modules, port `Optim.f90`, `pre_ener.f90`, `energy.f90`, `load.f90`, `pasapas.f90`, `get_reac.f90`, and `lbfgs.f`, and rewrite [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) to read the archived `nano_*.dat` inputs, run the load-stepping loop, and emit `energy.dat`, `force.dat`, `output.dat`, and `nano_final_config.dat`. Add an end-to-end compression integration test against [test/cases/graphene_compression_simulator/np1](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1).
3. Port VTU output before claiming any solver milestone is complete: translate `paraview_vtu_output.f90` into a dedicated C++ output module, emit the same `mesh_config_XXXX.vtu` and `.pvd` files, and validate them against the archived compression oracle outputs already stored under [test/cases/graphene_compression_simulator/np1](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1).
4. Implement runtime vdW and self-contact in the simulator core: translate `grapheneCompressionOriginVersion/vdw_modules.f90`, the simulator-side `read.f90` vdW loading/broadcast path, and the exclusion logic consumed by `energy.f90`; integrate that kernel into the new energy assembly path and add dedicated kernel tests plus end-to-end `nvdw=1` simulator checks.
5. Implement cyclic loading, crease memory, and checkpoint/restart together as one coherent milestone: port the simulator-side cyclic controller from `load.f90` and `pasapas.f90`, port `crease.f90` and `crease_analysis.f90`, port the checkpoint read/write path from `read.f90`, and verify against the archived cyclic oracle artifacts under [test/cases/graphene_cyclic_crumple/simulator_run](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/simulator_run).
6. Finish the MPI-equivalence layer on the real solver, not just the wrapper: broadcast all runtime state the way `Optim.f90` does, add `np=1/2/4` consistency tests for energy and reaction forces, and add rank-mismatch checkpoint tests to close AC-10 and AC-11.
7. Add the missing documentation artifacts last but before any completion claim: create `AGENT.md` in the repo root, create `document/translation_notes.md`, and record build/run/verification evidence for every milestone that remains open today.
8. After implementation, extend `CMakeLists.txt` with the new unit/integration targets and rerun the full matrix: build, all unit tests, all preprocessor oracle tests, compression solver oracle, cyclic/checkpoint tests, and MPI consistency tests. Do not emit `COMPLETE` until every original-plan task and every AC is actually closed.

## Verification

- Rebuilt: `cmake --build build --target integration_tests unit_tests -j4`
- Re-ran targeted Round 10 tests: `ctest --test-dir build --output-on-failure -R '^(PreprocessorOracle\\.(ArchivedBilayerTwistVdw1000PreproInputMatchesOracleOutputs|BilayerTwistVdw1000ForcesNborderOverrideToTwo))$'`
- Re-ran full suite: `ctest --test-dir build --output-on-failure`
- Result: all commands passed; current suite is `42/42`

Round 10 remains incomplete.
