# Round 22 Contract

## Mainline Objective

Fix the archived constitutive oracle provenance (replace `nano_config.dat` source with `nano_final_config.dat`-derived load-step states), then implement the element-level energy/force kernel (`task3e` / `ener_elem.f90`) and close Milestone 3.

## Target ACs

- **AC-5**: Brenner REBO potential fully integrated through the element-state pipeline (non-trivial archived geometry/bond fixtures).
- **AC-6**: Inner Newton solver validated against 10 archived simulator load-step states (not synthetic/undeformed fixtures).
- **AC-7**: Element-level energy/force kernel (`ener_elem`) implemented and tested; global assembly path begun.

## Blocking Issues

1. **Wrong oracle source for constitutive fixtures** — `dump_archived_constitutive_oracle.f90` reads `nano_config.dat` (initial, undeformed state) instead of mid-simulation load-step data. All 10 `archived_compression_np1/case_*.dat` fixtures degenerate to the undeformed identity/zero-curvature/zero-eta state. This directly blocks closing `task3b` and `task3d`. Must be fixed before any tracker update claims archived-state provenance.

2. **`test_element_state.cpp` missing bond-direction coverage** — the archived regression only iterates `i < 2` for `prepared_bonds.Ei`, leaving `Ei[2]` (third bond direction) unchecked.

3. **`task3e` (`ener_elem`) is absent** — no `include/fce/element_energy.hpp` or `src/core/element_energy.cpp` exists; Milestone 3 cannot close without it.

## Queued Issues (Out of Scope This Round)

- Simulator-side assembly (`task4a`) and L-BFGS (`task4b`) — begin only after `task3e` is verified.
- Loading controller, pasapas, reaction-force extraction (`task4c`–`task4f`) — downstream of `task4a`/`task4b`.
- VTU output, runtime vdW, cyclic loading, crease memory, checkpoint (`task5a`–`task7e`) — deferred.
- MPI multi-rank parity and documentation finalization (`task8a`–`task8d`) — deferred.
- AGENT.md / translation_notes.md (`task8c`) — queued, non-blocking.

## Concrete Success Criteria

1. `dump_archived_constitutive_oracle.f90` updated to read from load-step simulator output (`nano_final_config.dat` or equivalent mid-simulation state dump) rather than `nano_config.dat`.
2. `test/cases/constitutive_oracle/archived_compression_np1/` regenerated: all 10 fixtures contain non-trivial, non-identity deformation states; at least one explicit assertion in the test confirms fixtures are nontrivial (e.g., `|eta| > threshold`).
3. `build_provenance.md` updated to accurately describe the new fixture source.
4. `test_element_state.cpp` fixed to assert all three `Ei` directions.
5. `include/fce/element_energy.hpp` and `src/core/element_energy.cpp` exist, translating `ener_elem.f90` directly, reusing existing `geometry`, `principal`, `element_state`, `exponential`, and `constitutive` kernels without duplicating formulas.
6. Focused tests compare per-Gauss-point energy, `eta`, stress terms, and nodal force contributions against archived Fortran fixtures.
7. Full test suite passes (all existing tests remain green, new tests added for `task3e`).
8. Goal tracker update requested: `task3b` and `task3d` marked completed with archived-state evidence; `task3e` marked completed; Milestone 3 blocker resolved.
