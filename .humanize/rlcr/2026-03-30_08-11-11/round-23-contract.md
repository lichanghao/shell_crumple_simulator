# Round 23 Contract (Drift Recovery)

## Root Cause of Recent Drift

Rounds 21–22 spent exclusively on helper-level API cleanup (bond-state handoffs, solver signature changes) without fixing the *broken oracle source* or implementing the missing `task3e` module. The archived constitutive corpus was rebuilt from `nano_config.dat` (flat, undeformed initial state — all positions zero in z, all eta zero), making all 10 fixtures degenerate. This blocked `task3b` and `task3d` from being verifiably closed, and prevented meaningful progress toward Milestone 3 completion.

## Recovered Mainline Objective

**Fix the archived constitutive oracle provenance (source: `nano_final_config.dat` not `nano_config.dat`), regenerate non-trivial fixtures, fix the missing bond-direction assertion, then implement `task3e` (element-level energy/force kernel — `ener_elem.f90` translation) with oracle-backed tests.**

## Target ACs

- **AC-6**: Inner Newton solver validated against 10 non-trivial archived simulator load-step states (not synthetic/undeformed). Oracle must show `|eta| > 1e-4` for at least one component in each fixture.
- **AC-5** (partial closure): Brenner + geometry pipeline now exercised through non-trivial fixtures; bond-direction coverage complete (all 3 `Ei` rows asserted).
- **AC-7** (first step): `task3e` element-level energy/force kernel implemented and tested against archived fixtures, unblocking Milestone 4 assembly.

## Blocking Issues (must fix before mainline target is reached)

1. **Wrong oracle source** — `dump_archived_constitutive_oracle.f90` line 142 reads `nano_config.dat` (initial flat state). Fix: change to `nano_final_config.dat`. Regenerate `test/cases/constitutive_oracle/archived_compression_np1/` and rewrite `build_provenance.md`.

2. **Missing third bond-direction check** — `test/unit/test_element_state.cpp` iterates `i < 2` instead of `i < 3` for `prepared_bonds.Ei`. Fix: change loop bound to `i < 3`.

## Queued Issues (out of scope this round)

- Simulator-side assembly (`task4a`) — begins next round after `task3e` verified
- L-BFGS, pasapas, reaction force, VTU, vdW runtime, cyclic, crease, checkpoint, MPI (`task4b`–`task8d`) — all queued
- AGENT.md / translation_notes.md (`task8c`) — queued

## Concrete Success Criteria

1. `dump_archived_constitutive_oracle.f90` reads `nano_final_config.dat`; regenerated fixtures have `|eta| > 1e-4` in at least one component per fixture (verified by explicit assertion in the test).
2. `build_provenance.md` accurately describes `nano_final_config.dat` as the fixture source.
3. `test_element_state.cpp` `prepared_bonds.Ei` loop asserts all three bond directions (`i < 3`).
4. `include/fce/element_energy.hpp` + `src/core/element_energy.cpp` exist — direct translation of `ener_elem.f90` using existing `geometry`, `principal`, `element_state`, `exponential`, `constitutive` kernels without formula duplication.
5. Focused tests verify per-Gauss-point `W`, `eta`, stress contributions and nodal force accumulation against archived constitutive fixtures.
6. Full test suite passes green (all prior tests plus new `task3e` coverage).
7. Goal tracker update requested: `task3b`, `task3d` closed with non-trivial evidence; `task3e` closed; Milestone 3 blocker resolved.
