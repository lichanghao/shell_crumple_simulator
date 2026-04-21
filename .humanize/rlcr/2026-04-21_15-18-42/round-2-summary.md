# Round 2 Summary
## Round 2 Summary

### What was implemented

- Added a reproducible deterministic cyclic replay builder at [test/cases/tools/build_cyclic_replay_runtime.sh](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/build_cyclic_replay_runtime.sh).
- The new helper:
  - copies the canonical Fortran simulator source tree into a scratch build directory
  - patches only the copied `pasapas.f90` so cyclic imperfections can be read from `imperfection_trace.dat`
  - compiles a replay-only executable `crunch_it_replay`
- Used that replay-only runtime to regenerate the deterministic step-one replay lane on the archived cyclic case and verified that it reproduces the committed replay fixtures:
  - `replay_step1_energy.dat`
  - `replay_step1_force.dat`
- Updated [test/cases/graphene_cyclic_crumple/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/build_provenance.md) to document the exact source-backed replay build/run path.
- Updated [document/translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md) so the deterministic replay fixtures are no longer described as coming from a vague “instrumented path”; they now have an in-repo reproducible build path.

### Files changed

- `test/cases/tools/build_cyclic_replay_runtime.sh`
- `test/cases/graphene_cyclic_crumple/build_provenance.md`
- `document/translation_notes.md`
- `.humanize/rlcr/2026-04-21_15-18-42/round-2-summary.md`

### Validation

- Built replay-capable Fortran runtime:
  - `test/cases/tools/build_cyclic_replay_runtime.sh ../finite_crystal_elasticity/grapheneCompressionOriginVersion /tmp/fce_fortran_replay_runtime`
- Ran deterministic replay on a fresh copy of the archived cyclic case with `replay_step1_trace.dat` installed as `imperfection_trace.dat`
- Verified the replay-only runtime reproduced the committed deterministic step-one contracts:
  - stdout `Equilibrium energy`: `3.0445358806477097E-004`
  - `force.dat`: `0.000022279  0.001250697`

### Remaining Items

- `GT-AC1` remains open. This round repaired the replay oracle contract, but it did not change the live C++ constrained-step runtime path yet.
- The next runtime step is still to fix the executable-path `nCodeLoad=31` constrained-step mismatch against the now-reproducible deterministic replay fixtures.
- `GT-AC2`, `GT-AC3`, `GT-AC4`, and runtime `GT-AC5` remain open with no solver-side vdW/self-contact, archived replay-lane restart acceptance, or `np=1/2/4` parity implementation landed in this round.

## BitLesson Delta

Action: none
Lesson ID(s): NONE
Notes: Re-read `.humanize/bitlesson.md` before the work. `bitlesson-selector` was not available in the current shell environment, so there was no selector output or new BitLesson entry for this round.

## Goal Tracker Update Request

### Requested Changes:
- Replace the current open issue that says the deterministic cyclic replay fixtures must still be treated as coming from an unspecified separate instrumented replay path.
- Add a new open issue stating that the deterministic cyclic replay fixtures are now reproducible in-repo through `test/cases/tools/build_cyclic_replay_runtime.sh`, and that the remaining `GT-AC1` blocker is the live C++ constrained-step mismatch against that repaired source-backed replay contract.

### Justification:
- This materially strengthens `GT-AC1`. The step-one replay contract is no longer just a committed artifact set with narrative provenance; it is now reproducible from source within the repository, which removes a major source of ambiguity before the live runtime fix.
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
