# Round 7 Summary

## Work Completed
- Archived the reconstructed first-step simulator inputs under
  `test/cases/first_constrained_step_oracle/reconstructed_case/`:
  - `nano_dims.dat`
  - `nano_general.dat`
  - `nano_zero.dat`
  - `nano_Mesh.dat`
  - `nano_final_config.dat`
- Updated `test/cases/first_constrained_step_oracle/build_provenance.md` so it no longer claims
  the first-step replay/unit gates are expected to stay red.
- Updated the provenance note to point at the new committed `reconstructed_case/` inputs and a
  repo-root regeneration command for `element83_full_oracle.dat`.

## Files Changed
- `test/cases/first_constrained_step_oracle/build_provenance.md`
- `test/cases/first_constrained_step_oracle/reconstructed_case/nano_dims.dat`
- `test/cases/first_constrained_step_oracle/reconstructed_case/nano_general.dat`
- `test/cases/first_constrained_step_oracle/reconstructed_case/nano_zero.dat`
- `test/cases/first_constrained_step_oracle/reconstructed_case/nano_Mesh.dat`
- `test/cases/first_constrained_step_oracle/reconstructed_case/nano_final_config.dat`

## Validation
- Re-ran the repo-local regeneration path documented in `build_provenance.md` from the C++ repo root:
  - rebuilt `dump_first_step_full_oracle.f90` from committed source
  - ran it on `test/cases/first_constrained_step_oracle/reconstructed_case`
  - verified the regenerated output matches `test/cases/first_constrained_step_oracle/element83_full_oracle.dat`
- Result: `MATCH`

## Remaining Items
- The first-step provenance path is now committed and reproducible from repo artifacts.
- The remaining analytical blockers are still:
  - `ElementState.MatchesArchivedCompressionSimulatorOracleFixtures`
  - `ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures`
  - `ElementEnergy.FElemMatchesFortranOracle`
- After those are green, the next blocker remains
  `E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace`.
- AC-8 through AC-12 runtime work is still pending.
- `bitlesson-selector` remained unavailable in this environment (`zsh:1: command not found: bitlesson-selector`).

## Goal Tracker Update Request

### Requested Changes:
- Close the open issue that says `element83_full_oracle.dat` is not reproducible from committed repo
  artifacts alone. The reconstructed-case inputs are now committed under
  `test/cases/first_constrained_step_oracle/reconstructed_case/`, and the provenance note includes
  a working repo-root regeneration command.
- Close the open issue that says `build_provenance.md` still reports the first-step replay/unit gates
  as red. The provenance note now reflects the measured green state for both first-step gates.
- Record Round 7 progress as first-step provenance hardening: committed reconstructed-case archive
  plus verified repo-local regeneration path.

### Justification:
- This round closes the remaining AC-1/AC-13 provenance hardening gap on the first-step oracle path.
  The first-step fixtures are now internally consistent, the replay and unit gates are green, and
  the full-oracle reproduction path is committed and verified from repo artifacts alone.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable lesson was added; this round archived the reconstructed first-step case and
  verified the repo-local oracle regeneration path.
