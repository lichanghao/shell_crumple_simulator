# Round 3 Summary

## Work Completed

- Repaired the replay-only cyclic force-row contract to use the source-built canonical Fortran runtime result instead of the stale previously committed row.
- Updated [test/cases/graphene_cyclic_crumple/replay_step1_force.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/replay_step1_force.dat) from:
  - `0.000022279  0.001250697`
  to the source-built canonical Fortran replay row:
  - `-0.000064340  0.001056593`
- Updated [test/cases/graphene_cyclic_crumple/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/build_provenance.md) to document that fixture refresh and to make the remaining ambiguity explicit: `replay_step1_force.dat` is now source-backed, while `replay_step1_energy.dat` still is not.
- Updated [document/translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md) so the status note no longer treats the replay-only force fixture as current truth and now distinguishes:
  - the repaired source-backed force-row contract
  - the still-unsettled source-backed capture path for the replay-only energy row

## Files Changed

- `test/cases/graphene_cyclic_crumple/replay_step1_force.dat`
- `test/cases/graphene_cyclic_crumple/build_provenance.md`
- `document/translation_notes.md`
- `.humanize/rlcr/2026-04-21_13-31-12/round-3-summary.md`

## Validation

- Source-built canonical Fortran replay row already documented in the prior round:
  - `-0.000064340  0.001056593`
- Re-ran the main cyclic executable-path regression after refreshing `replay_step1_force.dat`:
  - `./build/integration_tests --gtest_filter='E2ECyclicRuntime.CrunchItReplaysCommittedCyclicStepOneTraceDeterministically' --gtest_color=no`
- Post-refresh mismatch surface:
  - `energy` cols `3/4`: relative error still about `3.41e-4`
  - `GNORM` col `7`: relative error still about `4.08e-2`
  - `force` col `3`: relative error reduced but still about `4.22`
  - `force` col `4`: relative error still about `3.68e-1`

## Remaining Items

- `GT-AC1` remains open. Refreshing the stale replay-only force-row fixture improved the reaction mismatch, but the live cyclic executable path is still wrong against the repaired force-row contract and the still-unsettled replay energy-row contract.
- `replay_step1_energy.dat` still lacks an equally clean source-backed regeneration path from the canonical Fortran runtime.
- `GT-AC2`, `GT-AC3`, `GT-AC4`, and runtime `GT-AC5` remain open with no runtime vdW/self-contact, archived replay-lane restart completion, or `np=1/2/4` parity work landed in this round.

## BitLesson Delta

Action: none
Lesson ID(s): NONE
Notes: Re-read `.humanize/bitlesson.md` before the work. `bitlesson-selector` was not available in the current shell environment, so there was no selector output or new BitLesson entry for this round.

## Goal Tracker Update Request

### Requested Changes:
- Update the stale replay-force open issue to note that `replay_step1_force.dat` has now been refreshed from the source-built canonical Fortran runtime and is no longer the ambiguous part of the cyclic replay contract.
- Add to Open Issues: “`replay_step1_energy.dat` still lacks a source-backed regeneration path from the canonical Fortran runtime, so the cyclic replay contract remains partially mixed even after the force-row refresh.”

### Justification:
- This narrows `GT-AC1` materially. The replay-only force-row contract is no longer the main provenance problem; the remaining contract uncertainty is now concentrated in the replay-only energy row and the live C++ constrained-step runtime mismatch.
