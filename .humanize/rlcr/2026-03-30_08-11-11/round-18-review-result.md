# Round 18 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 9/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. Claude's summary overstates the new state-based constitutive integration. `PreparedBondState` and `PreparedBondStateWithDerivatives` were added in `include/fce/element_state.hpp` and `src/core/element_state.cpp`, and the scalar `evaluate_inner_potential(...)` path now calls `prepare_bond_state(...)` at `src/core/constitutive.cpp:329`. But `evaluate_inner_potential(const ElementState&, ...)` is still only a one-line forwarder to the legacy scalar overload at `src/core/constitutive.cpp:485`, and `solve_inner_newton(const ElementState&, ...)` still just iterates by repeatedly calling that forwarding overload at `src/core/constitutive.cpp:584`, `src/core/constitutive.cpp:629`, and `src/core/constitutive.cpp:638`. That is useful refactoring, but it does not satisfy the Round 17 required implementation plan item to make the state-based path consume a canonical prepared-bond state instead of routing back through the scalar interface.

2. `task3b` remains incomplete plan work, not a closed canonical pipeline. `ElementState` still stores only metric, curvature, principal-curvature, and derivative data at `include/fce/element_state.hpp:10`; the prepared bond stage is exposed only as helper return values at `include/fce/element_state.hpp:29` and `src/core/element_state.cpp:58`, not as persisted canonical state. Repository-wide usage still shows no production callers for `compute_element_state(...)` or the bond helpers beyond tests and constitutive internals, there is still no translated `ener_elem` module anywhere under `include/fce` or `src/core`, and the simulator entry point is still the stub in `src/simulator/main.cpp:1`. This round advances Milestone 3 refactoring, but it does not advance Milestone 3 to closure or unlock AC-7.

3. The new tests still do not supply the missing Fortran-derived evidence. `test/unit/test_element_state.cpp` builds the coverage around hand-authored synthetic patch data at `test/unit/test_element_state.cpp:35`, `test/unit/test_element_state.cpp:44`, and `test/unit/test_element_state.cpp:53`, then verifies that the new helpers reproduce the same C++ manual composition path at `test/unit/test_element_state.cpp:130`. That remains wrapper-regression coverage, not oracle parity. The archived-state provenance gap for AC-6 is still explicitly open in `test/cases/constitutive_oracle/build_provenance.md:48`, so both the task3b geometry/bond provenance gap and the task3d simulator-state provenance gap remain unresolved.

4. The original plan is still materially incomplete, and Claude's "Remaining Items" section continues to normalize pending milestones as future work instead of incomplete work that must be finished. The tracker still carries `task3e` through `task8d` as pending, runtime vdW remains unimplemented, `AGENT.md` and `document/translation_notes.md` are still missing, and the simulator still does not execute any solver path. The round therefore remains narrow kernel/API progress only.

## Goal Alignment Check

### Acceptance Criteria Progress

| AC | Status | Assessment |
|----|--------|------------|
| AC-1 | MET | Oracle artifacts and conventions remain in place. |
| AC-2 | MET | Preprocessor oracle parity remains covered. |
| AC-3 | MET | B-spline oracle fixtures remain covered. |
| AC-4 | MET | Ghost-node oracle artifacts remain covered. |
| AC-5 | PARTIAL | Brenner remains implemented, and Round 18 extends the reusable bond-preparation slice, but the canonical bond-stage state path and element-energy integration remain incomplete. |
| AC-6 | PARTIAL | Newton tests still pass, and Round 18 narrows the bond-preparation gap, but the archived simulator-state provenance required by the plan remains missing. |
| AC-7 | NOT MET | No `ener_elem` translation, no assembly, no solver path, and `src/simulator/main.cpp` is still a stub. |
| AC-8 | PARTIAL | Preprocessor-side vdW parity is complete, but runtime vdW/self-contact remains absent. |
| AC-9 | NOT MET | No cyclic controller, crease-memory path, or crease analysis implementation exists. |
| AC-10 | NOT MET | No checkpoint/restart implementation or restart validation exists. |
| AC-11 | PARTIAL | MPI helpers exist, but there is still no solver to verify cross-rank parity on. |
| AC-12 | NOT MET | No VTU writer or validation path exists. |
| AC-13 | PARTIAL | Infrastructure exists, but `AGENT.md` and `document/translation_notes.md` are still missing. |

### Forgotten Items

No original-plan task IDs are untracked. The tracker still carries the full Milestone 3 through Milestone 8 backlog.

### Deferred Items

The tracker still has no explicit deferred entries. That is acceptable, but the unfinished Milestone 3 through Milestone 8 work remains incomplete work and must not be normalized as later-phase scope.

### Plan Evolution

Claude's Round 18 tracker request is justified only in narrowed form. The tracker can record the extracted bond-preparation helpers and the additional tests, but it cannot claim that the state-based constitutive path now directly consumes a canonical prepared-bond state, and it cannot narrow Milestone 3 any further than the still-open gaps in canonical state ownership, production integration, `ener_elem`, and archived provenance.

## Goal Tracker Update Requests

Approved in part and applied.

- Updated `.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md` to Plan Version 20 with a Round 18 evolution entry documenting commit `8840de2` and the passing `67/67` suite.
- Updated `task3b` so it records the new reusable bond-preparation helpers while remaining pending.
- Narrowed the Milestone 3 blocker so it no longer incorrectly says bond preparation still lives inside `evaluate_inner_potential(...)`; it now reflects the actual remaining gaps.
- Rejected the implied claim that the state-based constitutive path already consumes a canonical prepared-bond state directly, because `evaluate_inner_potential(const ElementState&, ...)` still forwards to the legacy scalar overload.

## Required Implementation Plan

1. Finish `task3b` as an actual canonical state pipeline. Extend the reusable element-state path so the prepared bond stage is owned by the canonical state object itself, including derivative-bearing bond data, and make `evaluate_inner_potential(const ElementState&, ...)` and `solve_inner_newton(const ElementState&, ...)` operate on that state directly without routing through the scalar overloads or rebuilding scalar-only views.

2. Translate `ener_elem.f90` into a dedicated element-energy module under `include/fce` and `src/core`. It must consume the canonical element state, run the inner Newton relaxation, and return per-element energy, force contributions, and the derivatives required by the outer solver. Add committed element-level oracle fixtures and direct unit coverage for this module.

3. Capture archived simulator-derived constitutive fixtures from the frozen Fortran code. Instrument the simulator to dump 10 compression load-step states plus the geometry, principal-curvature, and bond intermediates needed for Milestone 3 verification, commit those artifacts under `test/cases/constitutive_oracle/`, replace the synthetic helper-generated task3b/task3d inputs with the archived states, and update `test/cases/constitutive_oracle/build_provenance.md` accordingly.

4. Replace the simulator stub in `src/simulator/main.cpp` with the Milestone 4 solver path: global energy/force assembly, `pre_ener`-style partitioning, translated `lbfgs.f`, simulator-side `nCodeLoad=3` controller, `pasapas`, and reaction-force computation. Make `crunch_it` run the archived `nano_*.dat` inputs and reproduce the compression oracle required by AC-7.

5. Implement Milestone 5 completely by translating `paraview_vtu_output.f90` and adding validation against archived serial compression snapshots.

6. Implement Milestone 6 completely by translating `vdw_modules.f90`, adding self-contact exclusion logic, integrating runtime vdW into the assembled energy path, and verifying against the archived self-contact and bilayer-twist cases plus dedicated kernel tests.

7. Implement Milestone 7 and Milestone 8 completely: cyclic controller for `nCodeLoad=31`, crease memory, crease analysis, checkpoint/restart, MPI parity and cross-rank checkpoint checks, `AGENT.md`, `document/translation_notes.md`, and the final AC-1 through AC-13 integration suite.

## Verification

- `cmake --build build --target unit_tests`
- `ctest --test-dir build --output-on-failure -R 'ElementState|Constitutive|Geometry|Principal|Exponential'`
- `ctest --test-dir build --output-on-failure`
- Result: pass `67/67`

Round 18 remains incomplete.
