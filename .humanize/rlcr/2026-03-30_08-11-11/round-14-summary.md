# Round 14 Summary

## Work Completed
- Added a new dedicated principal-curvature module under `include/fce/principal.hpp` and `src/core/principal.cpp`, translating the distinct-curvature and repeated-curvature branches from `principal.f90`, including the derivative outputs `dcurvppaldC`, `dcurvppaldk`, `dvppaldC`, and `dvppaldk`.
- Added a new dedicated exponential/deformed-bond module under `include/fce/exponential.hpp` and `src/core/exponential.cpp`, translating the `def_bonds_` path and the derivative-bearing `def_bonds` path from `exponential.f90`.
- Wired both new modules into `fce_core` and `unit_tests` in `CMakeLists.txt`.
- Added focused unit files `test/unit/test_principal.cpp` and `test/unit/test_exponential.cpp` instead of growing `test/unit/test_constitutive.cpp` further.
- Verified the new principal module on distinct-curvature, repeated-curvature, and finite-difference derivative cases.
- Verified the new exponential module on the flat reference graphene state and on finite-difference checks of the direct `dpedC` derivatives with principal-derivative inputs held fixed.

## Files Changed
- `CMakeLists.txt`
- `include/fce/principal.hpp`
- `include/fce/exponential.hpp`
- `src/core/principal.cpp`
- `src/core/exponential.cpp`
- `test/unit/test_principal.cpp`
- `test/unit/test_exponential.cpp`

## Validation
- `cmake --build build --target unit_tests -j4`
  - Red phase: failed as expected because the new dedicated module source files did not exist yet
  - Final result: pass
- `ctest --test-dir build --output-on-failure -R 'Principal|Exponential'`
  - pass `5/5`
- `cmake --build build --target unit_tests integration_tests -j4`
  - pass
- `ctest --test-dir build --output-on-failure`
  - pass `57/57`

## Remaining Items
- `task3b` and `task3e` are still missing. There is still no dedicated `geometry` module for `metric` / `curv`, and no `element_energy` / `ener_elem` translation yet.
- The new `principal` and `exponential` modules are not wired into the translated constitutive or solver path yet; they currently exist as dedicated, tested kernels but not as integrated simulator-side execution.
- `task3d` still lacks the plan-required archived simulator-state provenance for the 10 AC-6 oracle states.
- The simulator executable is still a stub, so Milestones 4-8 remain open: solver assembly, L-BFGS, load stepping, runtime vdW/self-contact, VTU output, cyclic/checkpoint logic, MPI equivalence, and AC-13 documentation artifacts.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: `bitlesson-selector` remained unavailable in this shell. I reused the existing lessons manually and did not add a new knowledge-base entry.

## Goal Tracker Update Request

### Requested Changes:
- Update `task3a` notes to reflect that `include/fce/exponential.hpp` and `src/core/exponential.cpp` now implement the translated `def_bonds` / `def_bonds_` kernel slice with focused unit coverage in `test/unit/test_exponential.cpp`.
- Update `task3f` notes to reflect that `include/fce/principal.hpp` and `src/core/principal.cpp` now implement principal-curvature extraction with focused unit coverage in `test/unit/test_principal.cpp`.
- Record the verified full-suite result of `57/57` for Round 14.
- Keep `task3b`, `task3d`, `task3e`, and all Milestone 4-8 tasks open.

### Justification:
Round 14 moves Milestone 3 forward in the exact direction requested by the Round 13 review: dedicated modules and focused unit files instead of continuing to pack new logic into `constitutive.cpp`. The new kernels are real, tested translations, but Milestone 3 is still incomplete because the geometry and element-energy layers are still missing and the new modules are not yet wired into the simulator path.
