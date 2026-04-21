# Round 1 Summary
## Round 1 Summary

### What was implemented

- Corrected the cyclic replay contract after verifying that the plain canonical Fortran runtime is **not** a same-trace replay oracle.
- Verified directly from `../finite_crystal_elasticity/grapheneCompressionOriginVersion/pasapas.f90` that the canonical cyclic runtime still does:
  - `call random_seed()`
  - `call random_number(a)`
  on every cyclic load step and never reads `imperfection_trace.dat`
- Reverted the replay-only cyclic step-one fixtures back to the deterministic replay values that were captured from the instrumented same-trace path:
  - [test/cases/graphene_cyclic_crumple/replay_step1_energy.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/replay_step1_energy.dat)
  - [test/cases/graphene_cyclic_crumple/replay_step1_force.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/replay_step1_force.dat)
- Rewrote [test/cases/graphene_cyclic_crumple/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/build_provenance.md) so it no longer claims those replay fixtures come from a plain source-built canonical runtime.
- Updated [document/translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md) to remove the invalid “source-built canonical runtime refreshed the replay fixtures” claim and to record the actual contract boundary: the deterministic replay fixtures still come from a separate instrumented replay capture path.

### Files changed

- `test/cases/graphene_cyclic_crumple/replay_step1_energy.dat`
- `test/cases/graphene_cyclic_crumple/replay_step1_force.dat`
- `test/cases/graphene_cyclic_crumple/build_provenance.md`
- `document/translation_notes.md`
- `.humanize/rlcr/2026-04-21_15-18-42/round-1-summary.md`

### Validation

- `rg -n "random_seed\\(|random_number\\(|imperfection_trace" ../finite_crystal_elasticity/grapheneCompressionOriginVersion`
  - confirmed the canonical Fortran cyclic runtime seeds and draws fresh randomness every load step and never reads `imperfection_trace.dat`
- `sed -n '1,220p' ../finite_crystal_elasticity/grapheneCompressionOriginVersion/pasapas.f90`
  - verified the exact random imperfection path in source
- Confirmed the deterministic replay fixture contract and caveats in `test/cases/README.md`

### Remaining Items

- `GT-AC1` remains open. This round repaired the replay-fixture provenance mistake, but it did not change the live C++ cyclic constrained-step runtime path yet.
- The next runtime step is still to fix the executable-path `nCodeLoad=31` constrained-step mismatch against the committed deterministic replay fixtures, especially `GNORM` and reaction output.
- `GT-AC2`, `GT-AC3`, `GT-AC4`, and runtime `GT-AC5` remain open with no solver-side vdW/self-contact, archived replay-lane restart acceptance, or `np=1/2/4` parity implementation landed in this round.

## BitLesson Delta

Action: none
Lesson ID(s): NONE
Notes: Re-read `.humanize/bitlesson.md` before the work. `bitlesson-selector` was not available in the current shell environment, so there was no selector output or new BitLesson entry for this round.

## Goal Tracker Update Request

### Requested Changes:
- Replace the current open issue that says the committed step-one replay contract is source-backed on both `replay_step1_force.dat` and `replay_step1_energy.dat` via the plain canonical Fortran runtime.
- Add a new open issue stating that the plain source-built canonical Fortran runtime is **not** a same-trace replay oracle because it ignores `imperfection_trace.dat`, so the deterministic replay fixtures must continue to be treated as coming from a separate instrumented replay capture path.

### Justification:
- The current tracker text is technically wrong after checking `pasapas.f90` directly. Keeping that incorrect provenance in the tracker would misdirect the next runtime-debugging round away from the real deterministic replay contract.
## Work Completed
- [Describe what was implemented in this phase]

## Files Changed
- [List created/modified files]

## Validation
- [List tests/commands run and outcomes]

## Remaining Items
- [List unresolved items, if any]

## BitLesson Delta
- Action: none|add|update
- Lesson ID(s): NONE
- Notes: [what changed and why]
