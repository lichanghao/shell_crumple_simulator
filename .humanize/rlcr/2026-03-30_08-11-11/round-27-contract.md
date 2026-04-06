# Round 27 Contract

## Mainline Objective

Complete Milestone 3 oracle coverage and begin Milestone 4 simulator integration.

Milestone 3 remaining gaps (from goal-tracker after Round 26 review):
1. **task3a**: Add Fortran-derived oracle fixtures for the exponential-map kernel (`def_bonds` / `def_bonds_`);
   extend `test/unit/test_exponential.cpp` to compare `pe`, `dpedC`, `dpedk`, `d²pe/dC²`
   against oracle data generated from archived simulator-state inputs.
2. **task3c**: Add Brenner-through-element-energy production validation; demonstrate that the
   Brenner kernel (`nCode_Pot=2`) is exercised through the full `compute_element_energy` path and
   compare against Fortran oracle output (extend `dump_element_energy_oracle.f90` or add a
   sibling driver for `nCode_Pot=2`).
3. **task3f**: Add Fortran-derived oracle fixtures for the principal-curvature kernel; dump
   `curvppal`, `vppal`, `dcurvppaldC`, `dcurvppaldk`, `dvppaldC`, `dvppaldk` for at least one
   non-degenerate (distinct curvatures) archived state; extend `test/unit/test_principal.cpp`
   to compare against the committed fixtures.

After all three Milestone 3 gaps are closed, proceed to Milestone 4 Phase A:
4. **task4a (start)**: Implement global energy/force assembly over all elements in
   `src/core/simulator.cpp` (or a new translation unit), using the existing
   `compute_element_energy` API; add a unit test that assembles energy over the compressed-
   graphene mesh for load-step 1 and checks the total against the archived `energy.dat` first
   row within 1e-4 relative.

## Target Acceptance Criteria

- **AC-5**: Brenner production path validated end-to-end through `compute_element_energy`.
- **AC-6**: Exponential-map kernel oracle coverage gaps closed.
- **AC-7**: Milestone 4 Phase A global assembly begun; direct oracle comparison for at least
  load-step 1 energy.

## Blocking Issues (none)

Round 26 cleared all blocking issues:
- AC-2 preprocessor double-shift: resolved.
- task3e flag_num_diff oracle: closed.

## Approach Constraints

1. All new oracle fixtures must be Fortran-derived via committed helpers under
   `test/cases/tools/`; no synthetic fixtures for these items.
2. Oracle driver compilation and run instructions must be documented in the relevant
   `build_provenance.md` file.
3. All existing tests (60 unit + 18 integration) must continue to pass after each commit.
4. task3c Brenner element-energy test may reuse `dump_element_energy_oracle.f90` with an
   additional `nCode_Pot=2` material payload, or add a new sibling driver — choose whichever
   avoids duplicating the inner-Newton machinery.

## Queued (not this round)

- `principal_` interface annotation cleanup (minor documentation)
- Runtime vdW/self-contact (task6a–task6c)
- AGENT.md + translation_notes.md (AC-13)
- Milestone 4 Phases B–F (task4b–4f) and Milestones 5–8
