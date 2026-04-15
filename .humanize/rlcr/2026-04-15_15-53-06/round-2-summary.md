# Round 2 Summary

## Work Completed

- Corrected the obvious 0-based `nCodeLoad=31` corner-tag mapping bug in `LoadController::apply_increment`.
- Added committed deterministic cyclic replay fixtures for step 1:
  - `test/cases/graphene_cyclic_crumple/replay_step1_trace.dat`
  - `test/cases/graphene_cyclic_crumple/replay_step1_energy.dat`
  - `test/cases/graphene_cyclic_crumple/replay_step1_force.dat`
- Updated `.gitignore` so the new cyclic replay fixtures are tracked.
- Documented the cyclic archive-vs-replay contract in `test/cases/README.md`:
  - archived cyclic step-1 rows remain a stochastic historical oracle,
  - deterministic replay tests must use the committed replay fixtures instead.
- Reworked cyclic integration coverage:
  - added `CompressionCaseFiles.ArchivedAndReplayCyclicStepOneRowsAreDistinctContracts`,
  - replaced the unstable archive-backed first-step cyclic assertion with a deterministic replay smoke test driven by the committed trace fixture.

## Key Finding

- The remaining cyclic mismatch is not just the corner-tag mapping.
- A source-backed instrumented Fortran replay showed the cyclic post-`minimize_free` state is already non-flat, while the current C++ step-0 state remains flat.
- The measured max absolute coordinate delta between the Fortran `post_minimize_free_coords.dat` dump and the C++ step-0 VTU state is about `0.08396`.
- Under a fixed captured step-1 trace, the C++ cyclic first-step row is still wrong versus the Fortran replay row. That means the next blocker is in the shared `minimize_free` / translated `lbfgs.f` path before the first cyclic load increment, not only in `LoadController`.

## Files Modified

- `.gitignore`
- `src/core/load_controller.cpp`
- `test/cases/README.md`
- `test/cases/graphene_cyclic_crumple/replay_step1_trace.dat`
- `test/cases/graphene_cyclic_crumple/replay_step1_energy.dat`
- `test/cases/graphene_cyclic_crumple/replay_step1_force.dat`
- `test/integration/test_e2e_compression.cpp`
- `.humanize/rlcr/2026-04-15_15-53-06/goal-tracker.md`

## Validation

- `cmake --build build --target integration_tests -j4`
- `./build/integration_tests --gtest_filter='CompressionCaseFiles.ArchivedAndReplayCyclicStepOneRowsAreDistinctContracts:E2ECyclicRuntime.CrunchItReplaysCommittedCyclicStepOneTraceDeterministically'`
- Manual instrumented Fortran replay work in `/tmp/fce-cyclic-trace/` established:
  - captured cyclic step-1 trace value,
  - source-backed replay `energy.dat` / `force.dat` first-step row,
  - cyclic `post_minimize_free_coords.dat` dump showing the C++/Fortran step-0 mismatch.

## Remaining Items

- `task7a` is still incomplete: the corner-tag fix improved the cyclic controller, but first-step cyclic parity still misses the Fortran replay row under the same fixed trace.
- `task7b` / `task7c` are still open: `update_creases` and `analyse_creases` remain untranslated.
- `task7d` is still partial: checkpoint ingestion/writing exists, but MPI restore and restart parity are still missing.
- The next concrete blocker is shared with AC-7-style solver work: `minimize_free` / translated `lbfgs.f` does not reproduce the Fortran cyclic post-free state.

## Goal Tracker Update Request

### Requested Changes:
- Add a Plan Evolution entry recording that Round 2 converted cyclic step-1 testing from a stochastic archive comparison into a deterministic replay contract with committed replay fixtures.
- Update `task7a` notes to state that the corner-tag mapping fix landed, but the remaining cyclic mismatch is now localized upstream of the cyclic controller in the shared post-`minimize_free` state.
- Add a new Open Issue that the cyclic post-`minimize_free` state already diverges from the source-backed Fortran replay before load step 1, with measured max absolute coordinate delta about `0.08396`.

### Justification:
- These changes reflect the new source-backed evidence from the instrumented Fortran replay.
- They keep AC-9 honest by separating deterministic replay from the stochastic archived row and by pointing the next round at the real remaining blocker instead of the already-fixed tag-map bug.

## BitLesson Delta

- Action: none
- Lesson ID(s): NONE
- Notes: `bitlesson-selector` remains unavailable in this environment, so no selector-driven lesson update was possible this round.
