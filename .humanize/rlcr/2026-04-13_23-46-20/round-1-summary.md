## Round Summary

- Added a deterministic first constrained-step regression at `test/integration/test_first_constrained_step_oracle.cpp`.
  It replays the archived compression case with the committed `imperfection_trace_fortran.dat`, reconstructs the
  step-1 constrained entry state by restoring constrained DOFs from `step1_after_increment.dat` onto
  `step1_after_imperfection.dat`, and compares element 83 against the committed same-trace Fortran oracle.
- Committed the first constrained-step oracle values in
  `test/cases/first_constrained_step_oracle/element83_expected.dat` and documented their source in
  `test/cases/first_constrained_step_oracle/build_provenance.md`.
- Fixed `test/cases/tools/dump_archived_constitutive_oracle.f90` so it can read current archived
  `nano_final_config.dat` files whether or not they include an explicit `Inner displacements` label before the
  eta block. This unblocked reproducing constitutive dumps against a reconstructed temp case.

## Verification

- `cmake --build build --target integration_tests unit_tests -j4`
  - Passed.
- `./build/integration_tests --gtest_filter='FirstConstrainedStepOracle.Element83ReplayMatchesCommittedFortranOracle:E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace'`
  - Failed as expected.
  - New deterministic first-step regression now reproduces the kernel blocker directly from committed inputs:
    - `W_elem`: actual `1.4539364867024104e-01`, expected `1.5168019536748686e-01`
    - `eta(gauss 1)`: actual `[8.6123916408285331e-05, -1.8375415984645584e-04]`,
      expected `[1.7059377382830062e-04, -1.3871993792757202e-04]`
    - `eta(gauss 2)`: actual `[1.1695652522516237e-03, -1.6496930596894301e-03]`,
      expected `[1.1767640905118179e-03, -1.5551411794653993e-03]`
  - Existing AC-7 executable-path gate remains red:
    - step-1 energy relative error `1.3407384932259225`
    - step-1 `GNORM` relative error `0.068245136318090996`
    - step-1 force-column relative errors `1.3407271675185646`, `0.91213791706456004`, `0.55602311548162575`
- `./build/unit_tests --gtest_filter='ElementState.MatchesArchivedCompressionSimulatorOracleFixtures:ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures:ElementEnergy.FElemMatchesFortranOracle'`
  - Failed.
  - The archived constitutive/kernel surface is still red before the executable-path replay branches:
    - `ElementState.MatchesArchivedCompressionSimulatorOracleFixtures` still diverges in
      `C_elem`, `curv0_elem`, `curvppal`, `vppal`, `eta`, and prepared bond data.
    - `ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures` still diverges on archived
      `eta` and `W_elem` for elements 83-87.
    - `ElementEnergy.FElemMatchesFortranOracle` still diverges on `W_elem` and many `f_elem`
      components for element 83.

## Findings

- The first constrained-step mismatch is now reproducible in-repo from committed artifacts instead of only from
  disposable `/tmp` outputs. The new regression confirms the element-kernel divergence on element 83 before the
  outer L-BFGS trajectory meaningfully branches.
- The broader archived constitutive surface remains red as well, so the active implementation target is still
  upstream of outer replay control flow: `compute_element_state`, `compute_principal_curvature`,
  `Hyper_pot_inner`, and `solve_inner_newton`.
- `bitlesson-selector` was not available in this environment (`zsh:1: command not found: bitlesson-selector`),
  so I could not execute that routing step literally.

## Goal Tracker Update Request

### Requested Changes:
- Add to completed round history: Round 1 now has a committed first constrained-step oracle/regression under
  `test/cases/first_constrained_step_oracle/` and `test/integration/test_first_constrained_step_oracle.cpp`.
- Keep `task3d`, `task3e`, and `task4d` open with stronger evidence that the blocker is inside the analytical
  constitutive/kernel path before outer-solver branching.
- Add to Open Issues: the first constrained-step mismatch is now reproducible from committed repo artifacts, not
  just temporary `/tmp` outputs.

### Justification:
- This round closes the review complaint that the first constrained-step diagnosis was not reproducible from
  committed artifacts. It does not solve AC-7, but it makes the blocker durable, reviewable, and directly testable
  from the repo.

## BitLesson Delta

- Action: none
- Lesson ID(s): NONE
