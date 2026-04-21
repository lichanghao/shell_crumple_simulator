# Round 2 Summary

## Work Completed

- Re-ran the live cyclic executable-path regression and confirmed it now terminates with concrete numeric mismatches rather than only appearing as a generic stall:
  - `energy` step-one row still differs from the committed replay fixture in total/internal energy and `GNORM`
  - `force` step-one row still differs materially in both reaction columns
- Ran a standalone traced C++ `crunch_it` step-one replay on the archived cyclic case and captured the actual emitted runtime row:
  - `energy.dat`: `3.04557441e-04 ... GNORM 9.374016465e-06`
  - `force.dat`: `0.000206887  0.001445050`
- Built the canonical Fortran simulator from source in a scratch directory using the local MPI/GCC toolchain with `-fallow-argument-mismatch`, then ran that source-built runtime on the same cyclic replay case and `replay_step1_trace.dat`.
- Documented a new source-backed inconsistency:
  - the source-built Fortran `force.dat` step-one row (`-0.000064340`, `0.001056593`) matches the archived `simulator_run/force.dat` step-one row much more closely than the committed replay-only fixture `replay_step1_force.dat`
  - this means the remaining cyclic blocker is no longer “just the C++ runtime vs replay fixture”; it is now split between the live C++ vs source-built-Fortran runtime mismatch and the stale provenance of the replay-only force-row fixture
- Recorded that new evidence in:
  - [test/cases/graphene_cyclic_crumple/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/build_provenance.md)
  - [document/translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md)

## Files Changed

- `test/cases/graphene_cyclic_crumple/build_provenance.md`
- `document/translation_notes.md`
- `.humanize/rlcr/2026-04-21_13-31-12/round-2-summary.md`

## Validation

- `./build/integration_tests --gtest_filter='E2ECyclicRuntime.CrunchItReplaysCommittedCyclicStepOneTraceDeterministically' --gtest_color=no`
  - produced concrete mismatches in energy columns `3/4/7` and force columns `3/4`
- Standalone traced C++ cyclic replay run on a fresh temp copy of `test/cases/graphene_cyclic_crumple/prepro_run`
  - confirmed the runtime completes step 1 and emits `energy.dat`, `force.dat`, and trace artifacts under `FCE_TRACE_COORD_DUMPS`
- `./build/integration_tests --gtest_filter='E2ECyclicRuntime.AcceptedState2FixtureReassemblyStillShowsFreeGradientDivergence:E2ECyclicRuntime.AcceptedState2FixtureContributionProbeLocalizesTopRightCorner:E2ECyclicRuntime.AcceptedState2OracleDifferenceProbeMapsTopOffendersThroughMdofOp' --gtest_color=no`
  - all 3 passed
- Source-built canonical Fortran runtime replay check:
  - copied `../finite_crystal_elasticity/grapheneCompressionOriginVersion/*` into `/tmp/fce_fortran_runtime`
  - compiled with `mpif77 -w -O3 -fallow-argument-mismatch`
  - ran `crunch_it_built` from inside a copied cyclic replay case directory with `replay_step1_trace.dat` installed as `imperfection_trace.dat`
  - observed source-built `force.dat` rows beginning with `-0.000064340  0.001056593`

## Remaining Items

- `GT-AC1` remains open. The live cyclic executable path is still mismatching source-backed runtime evidence on the step-one constrained lane.
- The replay-only fixture `test/cases/graphene_cyclic_crumple/replay_step1_force.dat` is now suspect and needs either source-backed regeneration or an explicit provenance reconstruction before it can remain a trusted contract.
- `GT-AC2`, `GT-AC3`, `GT-AC4`, and runtime `GT-AC5` are still open; no runtime vdW/self-contact, archived replay-lane restart completion, or `np=1/2/4` parity work landed in this round.

## BitLesson Delta

Action: none
Lesson ID(s): NONE
Notes: Re-read `.humanize/bitlesson.md` before the work. `bitlesson-selector` was not available in the current shell environment, so there was no selector output or new BitLesson entry for this round.

## Goal Tracker Update Request

### Requested Changes:
- Add to Open Issues: “The committed cyclic replay-only force-row fixture `replay_step1_force.dat` appears stale relative to a source-built canonical Fortran runtime replay on the same archived cyclic case and trace input.”

### Justification:
- This is now a source-backed issue, not just suspicion. The source-built Fortran runtime produces a step-one force row that aligns with the archived `simulator_run/force.dat` row much more closely than with the committed replay-only fixture. That changes the debugging surface for `GT-AC1`: the next runtime fix needs to target the source-built Fortran contract, and the replay-only force fixture needs regeneration or provenance repair.
