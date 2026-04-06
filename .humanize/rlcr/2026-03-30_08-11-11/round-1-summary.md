# Round 1 Summary

## Work Completed
- Narrowed the round scope to a single mainline objective: Milestone 2 preprocessor parity for the archived graphene compression case.
- Replaced the `PrePro` stub with a real `run_preprocessor()` entry path that reads `data.dat` and writes `nano_dims.dat`, `nano_general.dat`, `nano_zero.dat`, `nano_config.dat`, `nano_BCs.dat`, `nano_Mesh.dat`, and `nano_tub_loc.dat`.
- Added C++ implementations for mesh generation, ghost-node connectivity/extrapolation, B-spline basis/derivatives, quadrature setup, reference `F0/J0`, and preprocessor BC/load setup.
- Implemented `task1e` as a reusable oracle comparison helper in `test/support/oracle_compare.cpp` and used it in a new end-to-end preprocessor oracle test.
- Fixed several translation bugs found by the oracle test:
  - Fortran `D`-exponent values in `data.dat` were being parsed incorrectly.
  - BC node tags were stored one-based internally instead of zero-based.
  - `connect_mesh`/`connect_orig_mesh` had several 0-based/1-based translation errors in neighbor traversal and `nghost_tab` generation.
  - `nano_tub_loc.dat` needed to preserve the archived 47-point vdW quadrature span convention.
- Fixed a cyclic preprocessor crash in the `nCodeLoad=31` path: `run_preprocessor()` was re-entering `load_pre()` with a zero-sized scratch `BCData` during first-sheet metadata recomputation. The final BC metadata now reuses the already-populated first-sheet state instead.

## Files Changed
- Build/test integration:
  - `CMakeLists.txt`
  - `test/integration/test_prepro_oracle.cpp`
  - `test/support/oracle_compare.hpp`
  - `test/support/oracle_compare.cpp`
  - `test/unit/test_bspline.cpp`
  - `test/unit/test_ghost_nodes.cpp`
  - `test/unit/test_reference_config.cpp`
- Preprocessor implementation:
  - `src/prepro/main.cpp`
  - `include/fce/preprocessor.hpp`
  - `src/core/preprocessor.cpp`
  - `include/fce/mesh_generator.hpp`
  - `src/core/mesh_generator.cpp`
  - `include/fce/ghost_nodes.hpp`
  - `src/core/ghost_nodes.cpp`
  - `include/fce/bspline.hpp`
  - `src/core/bspline.cpp`
  - `include/fce/quadrature.hpp`
  - `src/core/quadrature.cpp`
  - `include/fce/reference_config.hpp`
  - `src/core/reference_config.cpp`
  - `include/fce/load_pre.hpp`
  - `src/core/load_pre.cpp`
- RLCR artifacts:
  - `.humanize/rlcr/2026-03-30_08-11-11/round-1-contract.md`
  - `.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md`

## Validation
- `cmake --build build --target unit_tests integration_tests -j4`
  - Passed after wiring the new sources/tests into CMake.
- `ctest --test-dir build --output-on-failure -R '^(BSpline|GhostNodes|ReferenceConfig|PreprocessorOracle)'`
  - Passed 6/6 targeted preprocessor tests, including the cyclic no-crash regression.
- `ctest --test-dir build --output-on-failure`
  - Passed 27/27 total tests.

## Remaining Items
- Milestones 3-8 remain pending: constitutive model, solver, VTU output, vdW physics, cyclic loading/crease memory, checkpoint/restart, MPI consistency, and documentation finalization.
- `AGENT.md` and `document/translation_notes.md` are still missing and remain queued under AC-13.
- `task2g` only covers archived compression-case parity for `nano_tub_loc.dat`; full vdW/self-contact behavior is still deferred to Milestone 6.
- The archived cyclic preprocessor input now runs without crashing, but full cyclic preprocessor parity, `nano_crease.dat`, and downstream simulator support remain outside this round’s completed scope.

## BitLesson Delta
Action: none
Lesson ID(s): NONE
Notes: Existing BitLessons were sufficient for this round. The most relevant ones were the `nano_zero.dat` per-element storage note, the BC sequential-label parsing note, and the nCodeLoad oracle-parameter note.
