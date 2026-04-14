# First Constrained-Step Oracle Provenance

## Scope

`element83_expected.dat` captures the authoritative same-trace Fortran oracle used to diagnose the
AC-7 blocker on the very first constrained-step energy evaluation.
`element83_state.dat` archives the reconstructed 12-node element-83 patch coordinates from that same
authoritative Fortran replay so unit tests can exercise the kernel directly without rerunning `crunch_it`.
`element83_full_oracle.dat` archives the fuller analytical Fortran surface on that same authoritative state:
`C_elem`, `curv0_elem`, `curvppal`, `vppal`, prepared `pe`, converged `eta`, `W`, `ddWdeta`,
element `W_elem`, and `f_elem`.

- Element: `83` (1-based)
- Gauss points: `2`
- Expected values:
  - `W_elem = 1.45393670059607155e-01`
  - `eta(gauss 1) = [8.61239151820701165e-05, -1.83754200797972111e-04]`
  - `eta(gauss 2) = [1.16956533467332466e-03, -1.64969318401331591e-03]`
  - `flag_num_diff = false` at both Gauss points
- Full-surface values:
  - Per-Gauss `C_elem`, `curv0_elem`, `curvppal`, `vppal`, prepared `pe`, converged `eta`,
    `W`, and `ddWdeta`
  - Element-level `W_elem` and `f_elem`

## Source

These values were captured from the authoritative same-trace Fortran replay described in:

- `.humanize/rlcr/2026-04-13_23-46-20/round-0-summary.md`
- `document/translation_notes.md`

The replay reconstructed the first constrained-step evaluation state by:

1. Replaying the archived compression case with the committed `imperfection_trace_fortran.dat`
   using a patched canonical Fortran `crunch_it` that dumps `step1_after_increment.dat` and
   `step1_after_imperfection.dat` on load step 1.
2. Restoring constrained DOFs from `step1_after_increment.dat` onto `step1_after_imperfection.dat`
   so the reconstructed state matched the `long(..., x0_BC, ...)` entry state in `minimize.f90`.
3. Replacing the coordinate block in `nano_final_config.dat` with that reconstructed state.
4. Running the Fortran helper programs based on
   `test/cases/tools/dump_element_energy_oracle.f90` and
   `test/cases/tools/dump_first_step_full_oracle.f90` on that reconstructed-state case.

Important: `element83_full_oracle.dat` is generated with the converged same-trace Fortran `eta`
for each Gauss point before evaluating the stored prepared-bond `pe` surface. This avoids the
archived-zero-eta semantics used by the broader archived constitutive fixtures.

## Current Regression Contract

- `test/integration/test_first_constrained_step_oracle.cpp` reconstructs the first constrained-step
  entry state from committed replay inputs and compares the translated kernel against the committed
  Fortran oracle.
- `test/unit/test_first_constrained_step_oracle.cpp` reads `element83_state.dat`,
  `element83_expected.dat`, and `element83_full_oracle.dat` directly and keeps a standalone
  exact-state red gate on the archived first-step kernel surface.

Both tests are expected to stay red until the analytical element kernel matches the same-trace
Fortran replay.
