# Round 11 Summary

## Work Completed
- Added the first simulator-side constitutive kernel slice under `include/fce/constitutive.hpp`, `src/core/constitutive.cpp`, `include/fce/taylor.hpp`, and `src/core/taylor.cpp`.
- Ported the Fortran `Taylor.f90` series helpers, Brenner REBO energy/gradient/Hessian evaluation, `Hyper_pot_inner_alg.f90` for the Brenner path, and `newton_inner.f90` including fail modes 1/2/3 and the step damping / norm cap rules.
- Added committed Fortran-derived oracle fixtures under `test/cases/constitutive_oracle/` for 10 Brenner cases and 4 inner-Newton cases, plus the in-repo reproduction helper `test/cases/tools/dump_constitutive_oracle.f90`.
- Added `test/unit/test_constitutive.cpp` with fixture-backed Brenner and Newton parity checks, a Brenner Hessian finite-difference consistency check, and an unsupported-potential negative test.
- Documented fixture provenance in `test/cases/constitutive_oracle/build_provenance.md` and indexed the new case family in `test/cases/README.md`.

## Files Changed
- `CMakeLists.txt`
- `include/fce/constitutive.hpp`
- `include/fce/taylor.hpp`
- `src/core/constitutive.cpp`
- `src/core/taylor.cpp`
- `test/unit/test_constitutive.cpp`
- `test/cases/constitutive_oracle/build_provenance.md`
- `test/cases/constitutive_oracle/brenner/case_01.dat` … `case_10.dat`
- `test/cases/constitutive_oracle/newton_inner/case_01.dat` … `case_04.dat`
- `test/cases/tools/dump_constitutive_oracle.f90`
- `test/cases/README.md`
- `.humanize/bitlesson.md`

## Validation
- `cmake --build build --target unit_tests -j4`
  - Initial red-phase failure: `test_constitutive.cpp` could not find `fce/constitutive.hpp`
  - Final result: pass
- `ctest --test-dir build --output-on-failure -R '^(Brenner|NewtonInner)\.'`
  - pass `4/4`
- `ctest --test-dir build --output-on-failure`
  - pass `46/46`

## Remaining Items
- `task3a`, `task3b`, `task3e`, and `task3f` remain unimplemented: the exponential/deformed-bond path used by the outer constitutive kernel, geometry/metric-curvature extraction, element-energy assembly, and principal curvature extraction are still missing.
- The simulator mainline remains a stub, so Milestones 4-8 are still open: assembly, L-BFGS, load stepping, runtime vdW, VTU output, cyclic/checkpoint logic, MPI equivalence, and AC-13 documentation artifacts.
- The new constitutive modules are unit-tested only; they are not yet wired into `ener_elem`, `energy`, or `src/simulator/main.cpp`.

## BitLesson Delta
- Action: add
- Lesson ID(s): BL-20260330-brenner-hessian-fixture-fd
- Notes: the committed Brenner Hessian fixture is finite-difference-derived, so direct parity tolerances must account for oracle-side truncation error and be paired with an analytical-vs-finite-difference check.

## Goal Tracker Update Request

### Requested Changes:
- Update `task3c` to reflect that the Brenner kernel is now translated in-tree with committed Fortran fixtures (`test/cases/constitutive_oracle/brenner/`) and direct unit coverage in `test/unit/test_constitutive.cpp`.
- Update `task3d` to reflect that the Brenner-path `Hyper_pot_inner` and `newton_inner` logic are now translated in-tree with committed Fortran fixtures (`test/cases/constitutive_oracle/newton_inner/`) covering convergence and `fail_mode=3`.
- Add Round 11 evidence that the full suite now passes `46/46` after the new constitutive modules landed.
- Keep `task3a`, `task3b`, `task3e`, and `task3f` open, and keep all simulator/runtime milestones open.

### Justification:
This round moved the plan from a preprocessor-only codebase into the first real simulator-side constitutive implementation. The Brenner and inner Newton kernels now exist as translated C++ modules with reproducible Fortran oracle fixtures and passing tests, so the tracker should record that concrete Milestone 3 progress without over-claiming closure of the remaining constitutive, solver, or simulator tasks.
