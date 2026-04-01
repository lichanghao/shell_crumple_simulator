# Round 15 Summary

## Work Completed
- Fixed the new `principal` port so non-finite or materially negative curvature discriminants now raise `std::invalid_argument` instead of being silently clamped into the repeated-curvature fallback path.
- Expanded `test/unit/test_principal.cpp` with:
  - a near-repeated-curvature regression that stays on the numerical-fallback path,
  - a non-finite discriminant regression,
  - direct finite-difference checks for `dvppaldC` and `dvppaldk`.
- Expanded `test/unit/test_exponential.cpp` with a fully coupled finite-difference regression that exercises the nonzero `dcurvppal*`, `dvppal*`, and `dpedk` paths through `compute_deformed_bonds_with_derivatives(...)`.
- Reduced the sidecar status of the new `exponential` module by routing the live constitutive path in `src/core/constitutive.cpp` through `compute_deformed_bonds(...)` for the bond-state scalars consumed by `evaluate_inner_potential(...)`, while leaving the local `eta`-derivative machinery intact.
- Investigated the archived simulator artifacts for AC-6 provenance and confirmed that the committed oracle repository currently exposes only final-state `nano_config.dat` outputs and cycle-end `nano_checkpoint.dat` outputs, not the 10 intermediate per-load-step Newton states requested by the plan.
- Updated `test/cases/constitutive_oracle/build_provenance.md` to record that archived-state provenance blocker explicitly instead of leaving it implicit.

## Files Changed
- `src/core/principal.cpp`
- `src/core/constitutive.cpp`
- `test/unit/test_principal.cpp`
- `test/unit/test_exponential.cpp`
- `test/cases/constitutive_oracle/build_provenance.md`

## Validation
- `cmake --build build --target unit_tests -j4`
- `ctest --test-dir build --output-on-failure -R 'Principal|Exponential'`
  - pass `9/9`
- `ctest --test-dir build --output-on-failure -R 'Constitutive|NewtonInner|Brenner|Principal|Exponential'`
  - pass `19/19`
- `cmake --build build --target unit_tests integration_tests -j4`
  - pass
- `ctest --test-dir build --output-on-failure`
  - pass `61/61`

## Remaining Items
- `task3b` and `task3e` are still missing. There is still no dedicated `geometry` / `curv` module and no translated `element_energy` / `ener_elem` module.
- The simulator executable is still a stub, so Milestones 4-8 remain open.
- AC-6 still does not have the plan-required 10 archived simulator-state Newton fixtures; Round 15 only tightened the provenance documentation around that blocker.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: `bitlesson-selector` is not available in this shell. I read `.humanize/bitlesson.md` before each task and proceeded with `NONE`.

## Goal Tracker Update Request

### Requested Changes:
- Add a Round 15 plan-evolution entry recording the verified principal discriminant hardening, the expanded principal/exponential derivative coverage, the constitutive-path use of `compute_deformed_bonds(...)`, and the passing full suite at `61/61`.
- Update `task3a` notes to reflect that the dedicated `exponential` module is now consumed by the live constitutive inner-potential path for bond-state scalars, not only by standalone unit tests.
- Update `task3f` notes to reflect that `test/unit/test_principal.cpp` now covers `dvppaldC` and `dvppaldk`, and that non-finite discriminants are rejected instead of silently falling back.
- Update `task3d` notes and/or the Milestone 3 blocking issue to reference the newly documented provenance fact in `test/cases/constitutive_oracle/build_provenance.md`: the archived repository currently contains final-state and cycle-end simulator states, but not the 10 intermediate per-load-step Newton states required to close AC-6.

### Justification:
Round 15 directly addresses two of the Round 14 review findings and partially addresses the third. The principal kernel now matches the intended failure semantics for invalid discriminants, the missing derivative coverage has been added, and the `exponential` kernel is no longer purely sidecar because the live constitutive path now consumes its bond-state scalars. AC-6 and the broader Milestone 3/solver milestones remain incomplete, but the tracker should reflect the verified progress and the now-explicit provenance blocker accurately.
