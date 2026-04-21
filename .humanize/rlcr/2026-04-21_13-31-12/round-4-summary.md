# Round 4 Summary

## Work Completed

- Finished the source-backed cyclic replay fixture repair that Round 3 only partially completed.
- Refreshed [test/cases/graphene_cyclic_crumple/replay_step1_force.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/replay_step1_force.dat) from the completed source-built canonical Fortran replay row:
  - `-0.000000985  0.001052467`
- Refreshed [test/cases/graphene_cyclic_crumple/replay_step1_energy.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/replay_step1_energy.dat) from the same source-built canonical Fortran replay:
  - `E_total = E_internal = 2.7438748e-04`
  - `GNORM = 9.646e-06`
- Updated [test/cases/graphene_cyclic_crumple/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/build_provenance.md) so both the force row and the energy row now have explicit source-backed capture paths from the canonical runtime.
- Updated [document/translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md) so the cyclic blocker is no longer described as a mixed-fixture provenance issue. The replay-only step-one force/energy rows are now treated as source-backed contracts, and the remaining problem is the live C++ runtime mismatch against them.

## Files Changed

- `test/cases/graphene_cyclic_crumple/replay_step1_force.dat`
- `test/cases/graphene_cyclic_crumple/replay_step1_energy.dat`
- `test/cases/graphene_cyclic_crumple/build_provenance.md`
- `document/translation_notes.md`
- `.humanize/rlcr/2026-04-21_13-31-12/round-4-summary.md`

## Validation

- Reused the source-built canonical Fortran runtime (`/tmp/fce_fortran_runtime/crunch_it_built`) on a fresh copy of the archived cyclic replay case with `replay_step1_trace.dat` installed as `imperfection_trace.dat`.
- Captured the final step-one source-backed contracts from that run:
  - `force.dat`: `-0.000000985  0.001052467`
  - stdout `Equilibrium energy`: `2.7438748296784488E-004`
  - last printed `CRITC` before the step-2 banner: `9.646D-06`
- Re-ran the main cyclic executable-path regression against the fully refreshed source-backed fixtures:
  - `./build/integration_tests --gtest_filter='E2ECyclicRuntime.CrunchItReplaysCommittedCyclicStepOneTraceDeterministically' --gtest_color=no`
- Updated post-refresh mismatch surface:
  - `energy` cols `3/4`: relative error now about `1.10e-1`
  - `GNORM` col `7`: relative error now about `2.82e-2`
  - `force` col `3`: relative error now about `2.11e2`
  - `force` col `4`: relative error now about `3.73e-1`

## Remaining Items

- `GT-AC1` remains open. With both step-one replay rows now source-backed, the cyclic executable-path mismatch is no longer a fixture-provenance problem; it is a direct C++ runtime-vs-canonical-Fortran mismatch.
- The updated failure surface suggests `reaction1` is now the most obviously wrong quantity on the live step-one row, with total/internal energy and `GNORM` also still off.
- `GT-AC2`, `GT-AC3`, `GT-AC4`, and runtime `GT-AC5` remain open; no runtime vdW/self-contact, archived replay-lane restart completion, or `np=1/2/4` parity work landed in this round.

## BitLesson Delta

Action: none
Lesson ID(s): NONE
Notes: Re-read `.humanize/bitlesson.md` before the work. `bitlesson-selector` was not available in the current shell environment, so there was no selector output or new BitLesson entry for this round.

## Goal Tracker Update Request

### Requested Changes:
- Update the stale replay-energy open issue to note that `replay_step1_energy.dat` is now source-backed from the canonical Fortran runtime and is no longer the ambiguous part of the cyclic replay contract.
- Update the main cyclic replay blocker issue to note that the step-one contract is now source-backed on both energy and force rows, and that the remaining gap is a direct C++ runtime mismatch against that repaired contract.

### Justification:
- This materially sharpens `GT-AC1`. The replay-only fixture provenance ambiguity has now been removed for both step-one rows, so the next round can focus solely on fixing the live constrained-step runtime path.
