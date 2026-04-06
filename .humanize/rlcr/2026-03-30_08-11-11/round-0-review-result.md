# Round 0 Review

Mainline Progress Verdict: STALLED

Goal Alignment Summary: ACs: 13/13 tracked, 4/13 meaningfully advanced | Forgotten items: 0 | Unjustified deferrals: 1

## Mainline Gaps

1. Round 0 did not deliver its contracted mainline objective. The round contract requires Milestone 2 implementation and explicitly lists "task2a through task2h implemented with tests" as success criterion 3, but the repository still contains only stub executables and Milestone 1 support code.
Evidence:
- `.humanize/rlcr/2026-03-30_08-11-11/round-0-contract.md` says task2a-task2h must be implemented this round.
- `git show --stat --oneline HEAD` shows commit `524fd6a` changed only `.gitignore`.
- `src/prepro/main.cpp:1-12` prints `FCE Preprocessor (C++17) — stub`.
- `find src -maxdepth 3 -type f` returns only `src/core/io.cpp`, `src/core/mpi_env.cpp`, `src/prepro/main.cpp`, and `src/simulator/main.cpp`.
Required action:
- Reject the "loop initialization only" outcome.
- Implement task2a-task2h before claiming round success.

2. AC-2, AC-3, and AC-4 were the round targets, but there is no C++ implementation for mesh generation, ghost nodes, B-splines, quadrature, reference `F0/J0`, BC/load setup, vdW preprocessing, or oracle comparison.
Evidence:
- `CMakeLists.txt:39-65` builds `fce_core` from only `src/core/io.cpp` and `src/core/mpi_env.cpp`; no preprocessor kernels are present.
- `test/integration/test_oracle_roundtrip.cpp:1-149` verifies only read-write round trips for archived oracle files; it does not generate any `nano_*.dat` output from C++.
Required action:
- Add the missing Milestone 2 modules and replace the preprocessor stub with a real pipeline that reads `data.dat`, generates all `nano_*.dat` outputs, and compares them against the archived oracle with the plan tolerances.

3. The simulator mainline has not advanced beyond a placeholder, so AC-5 through AC-12 have effectively not started. Claude's summary openly defers Milestones 3-8; that is incomplete work, not an acceptable completion state for this review.
Evidence:
- `src/simulator/main.cpp:1-12` prints `FCE Simulator (C++17) — stub`.
- No constitutive, solver, vdW, cyclic, checkpoint, VTU, or restart source files exist under `src/core/`, `src/prepro/`, or `src/simulator/`.
Required action:
- Execute the remaining milestones in plan order instead of ending the round after tracker setup.

4. AC-13 is still incomplete. The plan requires `AGENT.md` and `document/translation_notes.md`, but neither file exists in the repository.
Evidence:
- `find . -maxdepth 2 \( -name 'AGENT.md' -o -name 'translation_notes.md' \)` returns no files.
Required action:
- Create both documents immediately and keep them updated as implementation proceeds.

5. The explicit deferral of `task1e` is no longer justified. The original plan makes `task1e` part of AC-2/AC-7 validation, and task2h cannot be accepted without it.
Evidence:
- `document/plan.md` lists `task1e` as the field-by-field comparison script for oracle validation.
- `.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md` still defers `task1e`.
Required action:
- Move `task1e` back into active work and implement it as part of the preprocessor oracle-validation path.

## Blocking Side Issues

1. The current executables cannot exercise any planned behavior, so there is no end-to-end verification path for the mainline.
Evidence:
- Fresh run of `./build/PrePro` prints only `FCE Preprocessor (C++17) — stub`.
- Fresh run of `./build/crunch_it` prints only `FCE Simulator (C++17) — stub`.
Required action:
- Replace both stubs with real execution paths before any further "completed" claims.

## Queued Side Issues

1. The goal tracker's mutable section had a labeling error: `task0b` was recorded as a plan amendment instead of the archived cyclic oracle run. The reviewer corrected this directly in `goal-tracker.md`. No further action is needed unless the tracker drifts again.

## Goal Alignment Check

- AC-1: completed and evidenced by archived compression and cyclic oracle artifacts.
- AC-2: partial only. I/O types and readers/writers exist, but the actual C++ preprocessor does not exist.
- AC-3: no implementation progress.
- AC-4: no implementation progress.
- AC-5: no implementation progress.
- AC-6: no implementation progress.
- AC-7: no implementation progress.
- AC-8: no implementation progress.
- AC-9: oracle artifacts exist, but no C++ cyclic-loading or crease implementation exists.
- AC-10: no implementation progress.
- AC-11: partial only. MPI wrapper and partition helpers exist, but no parallel solver verification exists.
- AC-12: no implementation progress.
- AC-13: partial only. CMake exists, but `AGENT.md` and `document/translation_notes.md` are missing.

## Directive Implementation Plan

1. Finish the preprocessor mainline first and do not spend another round on tracker-only work.
- Create `include/fce/mesh_generator.hpp` and `src/core/mesh_generator.cpp` for `mesh_gen_square`.
- Create `include/fce/ghost_nodes.hpp` and `src/core/ghost_nodes.cpp` for boundary extrapolation and ghost connectivity.
- Create `include/fce/bspline.hpp` and `src/core/bspline.cpp` for 12-node patch basis evaluation and derivatives.
- Create `include/fce/quadrature.hpp` and `src/core/quadrature.cpp` for the Gauss rule used by the Fortran preprocessor.
- Create `include/fce/reference_config.hpp` and `src/core/reference_config.cpp` for `Def_Grad`, `F0`, and `J0`.
- Create `include/fce/preprocessor.hpp` and `src/prepro/preprocessor.cpp` to orchestrate `data.dat` parsing, mesh generation, BC/load setup, vdW preprocessing, and `nano_*.dat` output.
- Replace `src/prepro/main.cpp` with a real CLI that reads the graphene case inputs and writes the full preprocessor output set.

2. Add the missing verification for the preprocessor before moving on.
- Create `test/unit/test_bspline.cpp` covering the interior and boundary basis fixtures from the Fortran oracle, including partition-of-unity and out-of-domain failure checks.
- Create `test/unit/test_ghost_nodes.cpp` covering ghost coordinate and connectivity reproduction for the oracle boundary elements.
- Create `test/unit/test_reference_config.cpp` covering `F0` and `J0` against the archived graphene compression oracle.
- Create `test/integration/test_prepro_oracle.cpp` that runs the C++ preprocessor on the archived `data.dat` input and compares every generated `nano_*.dat` field against `test/cases/graphene_compression_prepro/`.
- Implement `task1e` as a reusable comparison helper used by `test_prepro_oracle.cpp` and later simulator tests.

3. Implement the constitutive and solver core immediately after the preprocessor is passing.
- Create `include/fce/exponential_map.hpp` and `src/core/exponential_map.cpp`.
- Create `include/fce/geometry.hpp` and `src/core/geometry.cpp`.
- Create `include/fce/brenner.hpp` and `src/core/brenner.cpp`.
- Create `include/fce/newton_inner.hpp` and `src/core/newton_inner.cpp`.
- Create `include/fce/element_energy.hpp` and `src/core/element_energy.cpp`.
- Create `include/fce/principal_curvature.hpp` and `src/core/principal_curvature.cpp`.
- Add unit tests for bond-vector generation, Brenner `W/dW/d2W`, finite-difference consistency, and inner-Newton convergence/failure modes using archived oracle fixtures.

4. Build the actual simulator path and validate AC-7 before any optional cleanup.
- Create `include/fce/assembly.hpp` and `src/core/assembly.cpp` for MPI-aware global energy/force assembly.
- Create `include/fce/lbfgs.hpp` and `src/core/lbfgs.cpp` by translating `lbfgs.f` rather than substituting a different algorithm.
- Create `include/fce/load_controller.hpp` and `src/core/load_controller.cpp` for nCodeLoad=3, then extend the same controller for 30/31.
- Create `include/fce/pasapas.hpp` and `src/core/pasapas.cpp`.
- Create `include/fce/reaction_forces.hpp` and `src/core/reaction_forces.cpp`.
- Replace `src/simulator/main.cpp` with a real driver that reads `nano_*.dat`, executes the load-stepping loop, and writes energy, force, final configuration, and restart outputs.
- Create `test/integration/test_compression_oracle.cpp` that runs the serial nCodeLoad=3 case to step 50 and compares energy, reaction force, and selected nodal displacements to the archived oracle.

5. Complete the remaining planned scope in the original plan order, with tests at each gate.
- Create `include/fce/vtu_writer.hpp` and `src/core/vtu_writer.cpp`, then add XML-validity and state-consistency tests for AC-12.
- Create `include/fce/vdw.hpp` and `src/core/vdw.cpp`, then add oracle tests for neighbor lists, cutoff behavior, and exclusion distance for AC-8.
- Create `include/fce/crease_memory.hpp` and `src/core/crease_memory.cpp`, `include/fce/crease_analysis.hpp` and `src/core/crease_analysis.cpp`, and `include/fce/checkpoint.hpp` and `src/core/checkpoint.cpp`, then add cyclic nCodeLoad=31 and restart integration tests for AC-9 and AC-10.
- Extend integration tests to run np=1, np=2, and np=4 for AC-11 once the serial solver matches the oracle.
- Create `AGENT.md` and `document/translation_notes.md` now, then update them as each milestone lands.
- Do not mark the project complete until the full integration suite covers AC-1 through AC-13 and all remaining tasks are removed from pending/deferred status.

## Fresh Verification Evidence

- `ctest --test-dir build --output-on-failure` passes 21/21 tests, but those tests only cover existing I/O and MPI helper code.
- `./build/PrePro` and `./build/crunch_it` both still report stub executables.
- `git status --short` was clean before the reviewer corrected the tracker drift; the only code/work-product change from Round 0 itself was commit `524fd6a` touching `.gitignore`.
