# Round 34 Summary

## Work Completed
- Re-read the plan, goal tracker, and Round 33 review, then wrote `round-34-contract.md` to keep the round anchored on AC-7 runtime semantics.
- Replaced the deterministic imperfection surrogate in `src/core/solver.cpp` with a source-shape-equivalent runtime path:
  - each load step now reseeds a fresh RNG state
  - draws one scalar `a`
  - perturbs all real nodes by the same `mat.A0 * 2 * (a - 0.5) * fact_imp` offset after `load_doit(...)` and before constrained minimization
- Rebuilt `crunch_it` and `integration_tests` against that change.
- Reran the full archived-oracle executable-path regression with the new imperfection path and captured the earliest still-failing outputs.

## Files Changed
- `src/core/solver.cpp`
- `.humanize/rlcr/2026-03-30_08-11-11/round-34-contract.md`
- `.humanize/rlcr/2026-03-30_08-11-11/round-34-summary.md`

## Validation
- `cmake --build build --target crunch_it integration_tests -j4`
  - PASS
- `./build/unit_tests --gtest_filter='LoadController.*:SimulatorAssembly.StatefulAssemblyUsesRuntimeEtaInsteadOfInitialConfig'`
  - PASS (`3/3`)
- `./build/integration_tests --gtest_filter='E2ECompression.CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts'`
  - FAIL after `741680 ms` (~12.4 min), but now on the post-surrogate imperfection path
  - earliest failing energy row:
    - step 1 actual `5.59839e-05`
    - step 1 oracle `5.72105e-05`
    - relative error `2.144e-02`
  - first failing force row:
    - row 0 col 1 relative error `2.145e-02`
    - row 0 col 2 relative error `1.6101`
    - row 0 col 3 relative error `1.3365`
  - final configuration still diverges heavily; the first large tail failures remain in the z-coordinate field, with many nodes far outside the `1e-3` tolerance
  - compared to Round 33’s deterministic-surrogate run, the early energy trajectory moved closer again:
    - Round 33 step 1: `5.03028e-05`
    - Round 34 step 1: `5.59839e-05`
    - Oracle step 1: `5.72105e-05`

## Remaining Items
- AC-7 is still open. The imperfection path is now closer in structure to canonical `pasapas.f90`, but the archived-oracle executable regression still fails from step 1 onward and diverges badly in reaction forces and final configuration.
- The RNG semantics are still not Fortran-identical: the C++ path now mirrors “reseed then draw one scalar per step,” but it still uses the C++ standard-library RNG stack rather than the Fortran runtime’s generator.
- Milestone 5 onward remains pending original-plan work: VTU output, runtime vdW/self-contact, cyclic/crease/checkpoint features, MPI runtime verification, and repository documentation are all still incomplete.

## Goal Tracker Update Request

### Requested Changes:
- Add a Round 34 plan-evolution entry noting that the deterministic imperfection surrogate was removed and replaced with a source-shape-equivalent “reseed, draw one scalar, perturb all nodes” runtime path.
- Update `task4d` notes to record the new post-change evidence:
  - the first failing archived-oracle energy row is still step 1
  - step 1 moved materially closer to the oracle than the Round 33 deterministic-surrogate run
  - the solver trajectory still diverges badly after the first few increments
- Update `task4f` notes to record that the full executable-path archived-oracle regression was rerun after the imperfection-path change and still fails on energy, force, and final configuration.
- Add or update a blocking issue noting that the C++ RNG implementation is still not the same generator as the Fortran runtime, even though the step placement and “one scalar per step” structure now match more closely.

### Justification:
- These changes keep the tracker aligned with the latest repository evidence without overstating AC-7 progress.
- They distinguish the resolved surrogate-review issue from the still-open RNG-equivalence and solver-trajectory problems.
- They also preserve the concrete first-failing-step evidence needed to drive the next runtime reconciliation round.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable project lesson was added in this round. The main work was replacing the imperfection surrogate with a closer source-shaped runtime path and measuring its archived-oracle effect.
