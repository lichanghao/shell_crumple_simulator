# Round 16 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 9/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. `task3b` is still not the plan-required canonical geometry path. The new API is real in [include/fce/geometry.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/geometry.hpp#L27) and [src/core/geometry.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/geometry.cpp#L39), but the only in-repo call sites remain the new unit tests in [test/unit/test_geometry.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_geometry.cpp#L66). Production code still bypasses geometry entirely: [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L309) expects precomputed `C_elem`, `curvppal`, and `vppal`, and [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) is still a stub. That does not satisfy the Round 15 directive to move deformation-gradient decomposition and bond preparation into one canonical translated pipeline, so `task3b` and `task3e` remain open.

2. Round 16's "verified geometry-module translation" claim is too strong. The new coverage is finite-difference self-consistency on hand-authored synthetic patches, not oracle parity against archived Fortran data. [test/unit/test_geometry.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_geometry.cpp#L22) defines synthetic `flat_patch`, `curved_patch`, `curved_dn`, and `curved_ddn` inputs, and there are still no committed geometry/metric/curvature artifacts under `test/cases/`. This is useful kernel hardening, but it is not the same evidence bar used to close earlier translated modules.

3. The original plan remains materially incomplete and Claude's "remaining items" are not acceptable deferrals. The AC-6 simulator-state provenance gap is still explicitly open in [test/cases/constitutive_oracle/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md#L48), `task3e` through `task8d` are still pending in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L81), and the simulator remains stubbed in [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1). Round 16 therefore counts as narrow kernel progress only, not plan-level completion.

## Goal Alignment Check

### Acceptance Criteria Progress

| AC | Status | Assessment |
|----|--------|------------|
| AC-1 | MET | Oracle artifacts and conventions remain in place. |
| AC-2 | MET | Preprocessor oracle parity remains covered. |
| AC-3 | MET | B-spline oracle fixtures remain covered. |
| AC-4 | MET | Ghost-node oracle artifacts remain covered. |
| AC-5 | PARTIAL | Brenner exists and geometry now has a dedicated kernel, but the canonical constitutive pipeline is still incomplete. |
| AC-6 | PARTIAL | Newton tests pass, but the required archived simulator-state provenance is still missing. |
| AC-7 | NOT MET | No element-energy kernel, no solver core, no assembly, and the simulator entry point is still a stub. |
| AC-8 | PARTIAL | Preprocessor-side vdW is complete; runtime vdW/self-contact is still absent. |
| AC-9 | NOT MET | No cyclic controller, crease-memory path, or crease analysis implementation exists. |
| AC-10 | NOT MET | No checkpoint/restart implementation or restart validation exists. |
| AC-11 | PARTIAL | MPI helpers exist, but there is still no solver to verify cross-rank parity on. |
| AC-12 | NOT MET | No VTU writer or validation path exists in the C++ codebase. |
| AC-13 | PARTIAL | Infrastructure exists, but `AGENT.md` and `document/translation_notes.md` are still missing. |

### Forgotten Items

No original-plan task IDs are untracked. The tracker still carries the full Milestone 3 through Milestone 8 backlog.

### Deferred Items

The tracker still has no explicit deferred entries. That is acceptable, but the unfinished Milestone 3 through Milestone 8 work is still incomplete work and must not be normalized as future-phase scope.

### Plan Evolution

Claude's Round 16 tracker request is justified only in narrowed form. The tracker can record the dedicated geometry kernel, the focused finite-difference coverage, and the passing `64/64` suite, but it cannot treat `task3b` as verified closure or clear the Milestone 3 blocker while the geometry module remains disconnected from the production constitutive path.

## Goal Tracker Update Requests

Approved in part and applied.

- Updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L40) to Plan Version 18 with a Round 16 evolution entry documenting the new geometry kernel and the passing `64/64` suite.
- Updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L79) so `task3b` reflects the actual Round 16 progress while remaining pending.
- Kept the Milestone 3 blocker open in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L109), but revised it so it no longer says `geometry.f90` is absent; it now states that the geometry kernel exists but is still disconnected from the constitutive/simulator path.

## Required Implementation Plan

1. Build a canonical element-state pipeline around the new geometry module. Add an element-state API that starts from `xneigh`, `DN/DDN`, and `F0`, calls `compute_metric(...)`, `compute_curvature(...)`, `compute_principal_curvature(...)`, `compute_deformed_bonds(...)`, and `solve_inner_newton(...)`, and eliminate the remaining ad hoc deformation math duplication from [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L309).
2. Translate `ener_elem.f90` into a dedicated element-energy module under `include/fce` and `src/core`, returning per-element energy, internal force contributions, and any required derivatives from that canonical element-state pipeline. Add committed element-level oracle fixtures and unit coverage for this module.
3. Instrument the frozen Fortran simulator to dump 10 archived compression load-step states, commit them under `test/cases/constitutive_oracle/`, regenerate the Newton fixtures from those archived states, and update `build_provenance.md` so AC-6 is no longer backed by helper-synthesized states.
4. Replace the simulator stub with Milestone 4 end-to-end solver code: energy/force assembly, `pre_ener`-style partitioning, translated `lbfgs.f`, simulator-side load controller for `nCodeLoad=3`, `pasapas`, and reaction-force computation. Make `crunch_it` run the archived `nano_*.dat` inputs and reproduce the compression oracle required by AC-7.
5. Implement Milestone 5 VTU output and add validation against archived serial compression snapshots.
6. Implement the runtime vdW/self-contact path from `vdw_modules.f90`, integrate it into the assembled energy path, and verify it against the archived self-contact and bilayer-twist cases plus dedicated kernel tests.
7. Implement Milestone 7 and Milestone 8 completely: cyclic controller for `nCodeLoad=31`, crease memory, crease analysis, checkpoint/restart, MPI parity and rank-count checkpoint checks, `AGENT.md`, `document/translation_notes.md`, and the final AC-1 through AC-13 integration suite.

## Verification

- `ctest --test-dir build --output-on-failure -R 'Geometry'`
- `ctest --test-dir build --output-on-failure`
- Result: pass `64/64`

Round 16 remains incomplete.
