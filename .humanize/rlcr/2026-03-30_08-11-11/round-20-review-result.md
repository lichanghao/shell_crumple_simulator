# Round 20 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 9/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. The narrow Round 20 fix is real, but it does not close the plan-required constitutive evidence gap. [solve_inner_newton_for_element(...)](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/element_state.cpp#L103) now correctly writes the converged prepared state back into the returned helper object, and the new regression at [test_element_state.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_state.cpp#L130) proves the returned helper carries the final `prepared_eta` plus the expected bond payload. That resolves the specific Round 19 handoff defect. The remaining blocker is unchanged: the geometry/bond path is still validated only with hand-authored synthetic wrapper inputs, and the AC-6 provenance gap is still explicitly open in [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md#L48). Claude still needs archived simulator-derived Newton states and Fortran-derived geometry/bond fixtures before `task3b` and `task3d` can be considered complete.

2. The original plan remains materially incomplete beyond this helper-level refactor. `task3e` through `task8d` are still pending in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L83), and the simulator entry point is still the stub in [main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1). That means AC-7, AC-9, AC-10, and AC-12 are still not met, while AC-8, AC-11, and AC-13 remain only partially addressed. The Round 20 summary is honest about this, so the problem is not misreporting; the problem is that the implementation is still far from the scope required by [document/plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md).

## Goal Alignment Check

### 1.1 Acceptance Criteria Status

| AC | Status | Evidence (if MET) | Blocker (if NOT MET) | Justification (if DEFERRED) |
|----|--------|-------------------|---------------------|-----------------------------|
| AC-1 | MET | Compression and cyclic oracle artifacts remain archived under [graphene_compression_simulator](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator) and [graphene_cyclic_crumple](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple), with conventions in [fortran_conventions.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/fortran_conventions.md). | - | - |
| AC-2 | MET | Preprocessor oracle parity remains covered by the archived comparison tests tracked in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L123). | - | - |
| AC-3 | MET | Committed Fortran-derived B-spline fixtures remain covered as tracked in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L130). | - | - |
| AC-4 | MET | Archived ghost-coordinate artifacts and direct oracle comparisons remain covered as tracked in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L131). | - | - |
| AC-5 | PARTIAL | - | Brenner itself is translated and tested, but `task3b` still lacks Fortran-derived geometry/bond fixtures and `task3e` is still absent; see [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L83). | - |
| AC-6 | PARTIAL | - | The helper now returns the converged prepared state at [element_state.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/element_state.cpp#L103), but the 10-state Newton corpus is still helper-generated rather than archived simulator-state derived; see [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md#L48). | - |
| AC-7 | NOT MET | - | `ener_elem.f90`, assembly, L-BFGS, load controller, pasapas, reaction forces, and serial oracle reproduction are still absent; [main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) remains a stub. | - |
| AC-8 | PARTIAL | - | Preprocessor-side vdW parity is complete, but runtime vdW/self-contact remains unimplemented; see [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L112). | - |
| AC-9 | NOT MET | - | No cyclic controller, crease-memory path, or crease-analysis integration exists yet; see pending `task7a` through `task7e` in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L99). | - |
| AC-10 | NOT MET | - | No checkpoint/restart implementation or validation exists; see [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L102). | - |
| AC-11 | PARTIAL | - | MPI wrapper utilities exist, but there is still no solver path to verify `np=1/2/4` parity; see [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L104). | - |
| AC-12 | NOT MET | - | No VTU writer or validation path exists; see pending `task5a` and `task5b` in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L94). | - |
| AC-13 | PARTIAL | - | `AGENT.md` and `document/translation_notes.md` are still missing from the repo; verified directly and still tracked in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L118). | - |

### 1.2 Forgotten Items Detection

No forgotten original-plan task IDs were found. `task0a` through `task8d` are still represented in the current tracker across the active and completed sections.

### 1.3 Deferred Items Audit

There are still no explicit deferred items in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L137). Nothing needs to be un-deferred because nothing has been formally deferred.

### 1.4 Goal Completion Summary

Acceptance Criteria: 4/13 met (0 deferred)  
Active Tasks: 26 pending  
Critical blockers: missing `ener_elem` and solver path, missing archived simulator-derived constitutive fixtures, missing runtime vdW/self-contact, missing cyclic/crease/checkpoint work, missing VTU generation, missing AC-13 docs

## Goal Tracker Update Requests

Approved and applied.

1. Updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L42) to Plan Version 22 with a Round 20 evolution entry documenting commit `390f2b5` and the passing `69/69` suite.
2. Updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L83) so `task3b` now records that `solve_inner_newton_for_element(...)` returns the converged prepared-bond stage while remaining pending.
3. Updated the Milestone 3 blocker note in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L113) so it reflects the new helper-level handoff without overstating progress on production integration or oracle evidence.

## Required Implementation Plan

1. Close `task3b` and `task3d` with real oracle evidence, not more synthetic wrappers.
Touch [test/cases/tools/dump_constitutive_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_constitutive_oracle.f90), add a new Fortran-side dump helper if needed under `test/cases/tools/`, and archive the resulting geometry/bond and Newton-state fixtures under `test/cases/constitutive_oracle/`. The inputs must come from frozen simulator load-step states, not manually typed tables. Update [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md) with an exact reproduction recipe and replace the current provenance-gap section with concrete archived evidence.

2. Translate `ener_elem.f90` into a dedicated C++ element-energy module before touching the global solver.
Create `include/fce/element_energy.hpp` and `src/core/element_energy.cpp`. Port the Gauss-point loop from [ener_elem.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/../finite_crystal_elasticity/grapheneCompressionOriginVersion/ener_elem.f90#L1) directly: metric, curvature, crease-curvature subtraction, principal curvatures, optional inner Newton, final bond update at the converged `eta`, analytical stress path via `def_bonds` derivatives, numerical-differentiation fallback when `flag_num_diff` is set, and element-force accumulation. Reuse the existing `geometry`, `principal`, `element_state`, `constitutive`, and `exponential` kernels rather than duplicating formulas. Add focused tests that compare per-Gauss-point `W`, `dW/dpe`, `S_n`, `S_m`, `f_elem`, and `eta` against archived Fortran fixtures.

3. Wire the element-energy module into a real simulator-side energy assembly path.
Add `include/fce/energy.hpp` and `src/core/energy.cpp` for rank-local element loops and MPI reduction, plus any minimal mesh/state carrier types still missing. The assembly path must partition elements with the existing MPI utilities, call the new element-energy kernel for each owned element, accumulate total energy and nodal forces, and preserve the per-Gauss-point `eta` state needed for subsequent load steps and checkpointing. Add unit or integration coverage proving serial assembly matches manual summation and that element partitioning counts every element exactly once.

4. Replace the simulator stub with the Milestone 4 solver path in plan order.
Implement the translated L-BFGS core from `lbfgs.f`, then the simulator-side load controller for `nCodeLoad=3`, then the `pasapas` load-step loop, then reaction-force extraction. Only after those pieces exist should [main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) stop being a stub. Add an integration test for the serial compression oracle that checks energy trajectory, reaction force, nodal displacements, and clean exit without NaN/Inf.

5. Finish the feature-complete post-solver milestones instead of continuing narrow kernel refactors.
Implement VTU output (`task5a`/`task5b`), then runtime vdW and self-contact (`task6a`-`task6c`), then cyclic loading, crease memory, crease analysis, and checkpoint/restart (`task7a`-`task7e`). Each milestone needs direct oracle-backed tests, not just smoke coverage. Do not mark AC-8, AC-9, AC-10, or AC-12 as addressed until the runtime paths exist and are exercised end to end.

6. Close the remaining verification and documentation milestones.
After the solver path exists, add MPI parity tests for `np=1`, `np=2`, and `np=4`, checkpoint rank-compatibility tests, `AGENT.md`, `document/translation_notes.md`, and the full integration suite (`task8a`-`task8d`). The final pass condition is the original plan’s acceptance criteria, not another intermediate helper API cleanup.

## Progress Stagnation Check

The loop is still making forward progress, but it remains narrowly scoped. Rounds 17 through 20 advanced the constitutive state pipeline in small increments (`66/66` → `67/67` → `68/68` → `69/69`), yet the AC count is unchanged and the simulator path is still absent. This is not yet circular repetition, but the next round needs to move from helper plumbing into `ener_elem` and real oracle-backed constitutive integration.

## Verification

- `ctest --test-dir build --output-on-failure -R 'ElementState|NewtonInner|Brenner'`
- `ctest --test-dir build --output-on-failure`
- Result: pass `69/69`

Round 20 remains incomplete.
