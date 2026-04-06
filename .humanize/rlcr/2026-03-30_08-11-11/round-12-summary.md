# Round 12 Summary

## Work Completed
- Tightened the translated Brenner kernel in `src/core/constitutive.cpp` with the plan-required cutoff-radius zero-response branch (`a >= 0.17 nm`) while preserving the existing direct oracle parity and finite-difference Hessian coverage for in-cutoff states.
- Updated `include/fce/types.hpp` so the default `MatData::nCode_Pot` now matches the currently supported Brenner inner-relaxation path (`2`), removing the default/configuration mismatch inside the constitutive slice.
- Expanded `test/unit/test_constitutive.cpp` with the missing AC-5 negative coverage: cutoff-to-zero, zero-norm bond rejection, and a regression asserting the supported default material potential code.
- Expanded the committed Newton oracle corpus from 4 to 10 Fortran-derived fixtures by extending `test/cases/tools/dump_constitutive_oracle.f90` and regenerating `test/cases/constitutive_oracle/newton_inner/case_05.dat` through `case_10.dat`.
- Added explicit Newton failure-path checks for `fail_mode=1` (singular Hessian) and `fail_mode=2` (step-limit exceeded), and tightened the fixture-backed test to require presence of `fail_mode=1/2/3` in the committed corpus while keeping exact tolerances for converged states and slightly looser tolerance only on non-converged failure-path re-evaluations.
- Updated `test/cases/constitutive_oracle/build_provenance.md` to document the expanded `newton_inner` fixture set.

## Files Changed
- `include/fce/types.hpp`
- `src/core/constitutive.cpp`
- `test/unit/test_constitutive.cpp`
- `test/cases/tools/dump_constitutive_oracle.f90`
- `test/cases/constitutive_oracle/build_provenance.md`
- `test/cases/constitutive_oracle/newton_inner/case_05.dat` … `case_10.dat`

## Validation
- `cmake --build build --target unit_tests -j4`
  - Red phase: failed on the newly added cutoff/default/Newton-count assertions as expected
  - Final result: pass
- `ctest --test-dir build --output-on-failure -R 'Brenner|NewtonInner'`
  - pass `9/9`
- `cmake --build build --target unit_tests integration_tests -j4`
  - pass
- `ctest --test-dir build --output-on-failure`
  - pass `51/51`

## Remaining Items
- `task3a`, `task3b`, `task3e`, and `task3f` are still unimplemented. The codebase still lacks dedicated C++ ports of `exponential.f90`, `geometry.f90`, `ener_elem.f90`, and `principal.f90`, so Milestone 3 remains incomplete even though AC-5/AC-6 coverage improved materially this round.
- The simulator mainline is still a stub, so Milestones 4-8 remain open: assembly, L-BFGS, load stepping, runtime vdW/self-contact, VTU output, cyclic/checkpoint logic, MPI equivalence, and AC-13 documentation artifacts.
- The expanded Newton fixtures are Fortran-derived and reproducible, but they still come from the committed constitutive helper input set rather than archived simulator-state dumps. If the tracker wants that stricter provenance before closing `task3d`, it should remain pending.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: `bitlesson-selector` was not available in this shell, so I reused the existing constitutive lessons manually without adding a new entry.

## Goal Tracker Update Request

### Requested Changes:
- Update Round 12 evidence to record that the full suite now passes `51/51`.
- Update `task3c` notes to reflect that the Brenner kernel now includes the cutoff-radius zero-response path plus explicit negative tests for cutoff-to-zero and zero-norm rejection.
- Update `task3d` notes to reflect that `newton_inner` coverage now uses 10 committed Fortran-derived fixtures and explicitly exercises `fail_mode=1`, `fail_mode=2`, and `fail_mode=3`.
- Keep `task3a`, `task3b`, `task3e`, and `task3f` open, and keep all simulator/runtime milestones open.

### Justification:
Round 12 closes the specific AC-5 gap called out in the previous review and materially strengthens AC-6 verification by expanding the reproducible Newton oracle corpus and failure-path coverage. That is real Milestone 3 progress, but it does not justify claiming the remaining constitutive or simulator milestones are finished.
