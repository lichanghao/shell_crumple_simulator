# First Constrained-Step Oracle Provenance

## Scope

`element83_expected.dat` captures the temporary same-trace Fortran oracle used to diagnose the
Round 0 / Round 1 AC-7 blocker on the very first constrained-step energy evaluation.

- Element: `83` (1-based)
- Gauss points: `2`
- Expected values:
  - `W_elem = 1.51680195367486864e-01`
  - `eta(gauss 1) = [1.7059377382830062e-04, -1.3871993792757202e-04]`
  - `eta(gauss 2) = [1.1767640905118179e-03, -1.5551411794653993e-03]`
  - `flag_num_diff = false` at both Gauss points

## Source

These values were captured from the temporary same-trace Fortran replay described in:

- `.humanize/rlcr/2026-04-13_23-46-20/round-0-summary.md`
- `document/translation_notes.md`

The replay reconstructed the first constrained-step evaluation state by:

1. Replaying the archived compression case with the committed `imperfection_trace_fortran.dat`.
2. Taking the traced `step1_after_imperfection.dat`.
3. Restoring constrained DOFs from `step1_after_increment.dat` so the state matched the
   `long(..., x0_BC, ...)` entry state in `minimize.f90`.
4. Running the temporary Fortran `dump_element_energy_oracle` helper on that reconstructed state.

## Current Regression Contract

The integration regression uses the committed archived compression inputs plus the committed replay
trace to reconstruct the comparable first constrained-step entry state from the current C++
runtime, then compares the translated kernel against this committed Fortran oracle. The test is
expected to stay red until the analytical element kernel matches the same-trace Fortran replay.
