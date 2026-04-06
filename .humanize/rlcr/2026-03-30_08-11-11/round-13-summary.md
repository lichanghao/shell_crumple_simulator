# Round 13 Summary

## Work Completed
- Reconciled the archived `nano_general.dat` material encoding with the C++ reader/writer in `src/core/io.cpp`. The C++ path now preserves the actual Fortran parameter blocks for `nCode_Pot=1` (Morse), `nCode_Pot=2` (Brenner), and `nCode_Pot=3` (MM3) instead of discarding them behind hard-coded constants.
- Extended `src/core/constitutive.cpp` so the inner constitutive path now supports both `nCode_Pot=1` (Morse / `Inner_Morse`) and `nCode_Pot=2` (Brenner / `Inner_Brenner`) using the same `evaluate_inner_potential()` / `solve_inner_newton()` entry points. This removes the file-backed integration blocker that prevented committed compression materials from reaching the translated constitutive layer at all.
- Corrected the material-code semantics in `include/fce/types.hpp` and `document/fortran_conventions.md` to match the frozen Fortran sources: `1=Morse`, `2=Brenner`, `22=Brenner2`, `3=MM3`.
- Updated the unit tests so `ReadGeneral.GrapheneCompression` now verifies the committed Morse parameter block from `test/cases/graphene_compression_prepro/nano_general.dat`, and `NewtonInner.AcceptsCommittedCompressionMaterialPayload` proves the translated inner solver accepts that file-backed material without hand-built remapping.
- Updated `test/unit/test_constitutive.cpp` so the unsupported-potential regression now checks a truly unsupported code (`99`) instead of `1`, which is now intentionally supported.
- Updated `test/cases/README.md` so the constitutive oracle inventory reflects the Round 12 expansion to `newton_inner/case_01.dat` through `case_10.dat`.

## Files Changed
- `include/fce/types.hpp`
- `src/core/io.cpp`
- `src/core/constitutive.cpp`
- `document/fortran_conventions.md`
- `test/unit/test_io.cpp`
- `test/unit/test_constitutive.cpp`
- `test/cases/README.md`

## Validation
- `cmake --build build --target unit_tests -j4`
  - Red phase: failed on `ReadGeneral.GrapheneCompression` and `NewtonInner.AcceptsCommittedCompressionMaterialPayload` as expected
  - Final result: pass
- `ctest --test-dir build --output-on-failure -R 'ReadGeneral.GrapheneCompression|NewtonInner.AcceptsCommittedCompressionMaterialPayload|NewtonInner.RejectsUnsupportedPotentialCode|Brenner.DefaultMaterialUsesSupportedPotentialCode'`
  - pass `4/4`
- `cmake --build build --target unit_tests integration_tests -j4`
  - pass
- `ctest --test-dir build --output-on-failure`
  - pass `52/52`

## Remaining Items
- `task3d` still lacks the plan-required archived simulator-state provenance for the 10 AC-6 oracle states. The fixture-count/failure-mode coverage exists, but the main Newton corpus is still generated from helper-defined inputs rather than archived load-step dumps.
- `task3a`, `task3b`, `task3e`, and `task3f` are still unimplemented. The codebase still lacks dedicated C++ ports of `exponential.f90`, `geometry.f90`, `principal.f90`, and `ener_elem.f90`.
- The simulator executable is still a stub, so Milestones 4-8 remain open: solver assembly, L-BFGS, load stepping, runtime vdW/self-contact, VTU output, cyclic/checkpoint logic, MPI equivalence, and AC-13 documentation artifacts.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: `bitlesson-selector` was still unavailable in this shell. I reused the existing file-format and constitutive lessons manually and did not add a new entry.

## Goal Tracker Update Request

### Requested Changes:
- Remove or mark resolved the blocking issue stating that the translated constitutive slice is incompatible with committed `nano_general.dat` inputs. Round 13 reconciles the `nCode_Pot` mapping in the reader, the constitutive dispatcher, and the conventions doc, and adds a file-backed regression proving the archived compression payload is accepted.
- Update `task3c` notes to reflect that the remaining material-code semantics blocker has been addressed in addition to the Round 12 cutoff-path work.
- Keep `task3d` pending until the Newton AC-6 fixtures are sourced from archived simulator load-step outputs.
- Keep `task3a`, `task3b`, `task3e`, `task3f`, and all simulator/runtime milestones open.

### Justification:
Round 13 closes the specific semantic blocker that made the translated constitutive layer incompatible with the committed oracle inputs. That is necessary integration progress for Milestone 3 and AC-7, but it does not complete the missing geometry/element kernels or the solver/runtime milestones that still block end-to-end simulation parity.
