Your work is not finished. Read and execute the below with ultrathink.

## Original Implementation Plan

**IMPORTANT**: Before proceeding, review the original plan you are implementing:
@document/plan.md

This plan contains the full scope of work and requirements. Ensure your work aligns with this plan.

---

For all tasks that need to be completed, please use the Task system (TaskCreate, TaskUpdate, TaskList) to track each item in order of importance.
You are strictly prohibited from only addressing the most important issues - you MUST create Tasks for ALL discovered issues and attempt to resolve each one.

Before executing each task in this round:
1. Read @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/bitlesson.md
2. Run `bitlesson-selector` for each task/sub-task
3. Follow selected lesson IDs (or `NONE`) during implementation

---
Below is Codex's review result:
<!-- CODEX's REVIEW RESULT START -->
# Round 17 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 9/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. `task3b` is still not the plan-required canonical bond-vector pipeline. The new wrapper in [include/fce/element_state.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/element_state.hpp#L9) and [src/core/element_state.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/element_state.cpp#L12) stops at metric, curvature, and principal-curvature state. It does not own the bond-vector stage that Round 16 explicitly required, and the bond preparation still happens inside [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L319) through [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L462). The new overloads at [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L482) and [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L567) only forward `state.C_elem`, `state.curvppal`, and `state.vppal` back into the old entry points. That is useful API cleanup, but it does not satisfy the required “geometry -> principal -> bonds -> Newton” canonical pipeline.

2. The Round 17 work still has no production integration, so Milestone 3 and `task3e` remain open. Repository-wide search shows the new `compute_element_state(...)` and `solve_inner_newton_for_element(...)` APIs are only used in [test/unit/test_element_state.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_state.cpp#L71). There is still no translated `ener_elem.f90` module anywhere under `include/fce` or `src/core`, and the simulator entry point remains the stub in [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1). Claude’s summary correctly says later milestones remain open, but those are not acceptable “remaining items”; they are still incomplete plan work that must be implemented.

3. The new tests do not raise the evidence bar to oracle parity. [test/unit/test_element_state.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_state.cpp#L34) defines hand-authored `curved_patch`, `curved_dn`, and `curved_ddn` inputs, then checks that the wrapper reproduces the same C++ manual composition path at [test/unit/test_element_state.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_state.cpp#L78) and [test/unit/test_element_state.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_state.cpp#L107). That is a wrapper-regression test, not Fortran-derived geometry evidence. The AC-6 provenance gap also remains explicitly open in [test/cases/constitutive_oracle/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md#L48), so there is still no basis for treating Milestone 3 as verified closure.

4. The original plan remains materially incomplete. The active tracker still has `task3e` through `task8d` pending in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L83), runtime vdW remains a blocker in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L109), and AC-13 documentation deliverables are still missing in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L115). Round 17 therefore counts as narrow kernel/API progress only, not plan-level completion.

## Goal Alignment Check

### Acceptance Criteria Progress

| AC | Status | Assessment |
|----|--------|------------|
| AC-1 | MET | Oracle artifacts and conventions remain in place. |
| AC-2 | MET | Preprocessor oracle parity remains covered. |
| AC-3 | MET | B-spline oracle fixtures remain covered. |
| AC-4 | MET | Ghost-node oracle artifacts remain covered. |
| AC-5 | PARTIAL | Brenner exists and Round 17 adds a reusable state wrapper, but the canonical bond-vector and element-energy pipeline is still incomplete. |
| AC-6 | PARTIAL | Newton tests still pass, and Round 17 improves the API surface, but the archived simulator-state provenance gap remains open. |
| AC-7 | NOT MET | No `ener_elem` translation, no solver core, no assembly, and the simulator is still stubbed. |
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

Claude’s Round 17 tracker request is justified only in narrowed form. The tracker can record the new `ElementState` wrapper and state-based constitutive overloads, but it cannot treat `task3b` as complete or clear the Milestone 3 blocker while bond preparation still lives inside `evaluate_inner_potential(...)`, `ener_elem.f90` is still missing, and there are still no production callers.

## Goal Tracker Update Requests

Approved in part and applied.

- Updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L42) to Plan Version 19 with a Round 17 evolution entry documenting commit `e479003` and the passing `66/66` suite.
- Updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L80) so `task3b` records the new wrapper progress while remaining pending.
- Narrowed but kept the Milestone 3 blocker open in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L110) so it reflects the new core API connection without overstating production readiness.

## Required Implementation Plan

1. Extend the canonical element-state pipeline through the bond-vector stage. Add bond-state data and any needed derivative state to `ElementState`, move the bond preparation that still lives inside [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L319) into reusable helpers, and make the state-based constitutive path consume that canonical bond-prepared state instead of forwarding back to the legacy scalar entry points.
2. Translate `ener_elem.f90` into a dedicated element-energy module under `include/fce` and `src/core`. It must consume the canonical element state, run the inner Newton relaxation, and return per-element energy, internal force contributions, and any derivatives required by the outer solver. Add committed element-level oracle fixtures and direct unit coverage for this module.
3. Instrument the frozen Fortran simulator to dump 10 archived compression load-step states, commit them under `test/cases/constitutive_oracle/`, regenerate the Newton fixtures from those archived states, and update [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md#L48) so AC-6 is no longer backed by helper-synthesized states. At the same time, commit Fortran-derived geometry/principal fixtures so `task3b` is verified against oracle data rather than synthetic wrapper tests.
4. Replace the simulator stub in [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) with the Milestone 4 solver path: global energy/force assembly, `pre_ener`-style partitioning, translated `lbfgs.f`, simulator-side `nCodeLoad=3` controller, `pasapas`, and reaction-force computation. Make `crunch_it` run the archived `nano_*.dat` inputs and reproduce the compression oracle required by AC-7.
5. Implement Milestone 5 VTU output and add validation against archived serial compression snapshots.
6. Implement the runtime vdW/self-contact path from `vdw_modules.f90`, integrate it into the assembled energy path, and verify it against the archived self-contact and bilayer-twist cases plus dedicated kernel tests.
7. Implement Milestone 7 and Milestone 8 completely: cyclic controller for `nCodeLoad=31`, crease memory, crease analysis, checkpoint/restart, MPI parity and rank-count checkpoint checks, `AGENT.md`, `document/translation_notes.md`, and the final AC-1 through AC-13 integration suite.

## Verification

- `cmake --build build --target unit_tests`
- `ctest --test-dir build --output-on-failure -R 'ElementState|Constitutive|Geometry|Principal|Exponential'`
- `ctest --test-dir build --output-on-failure`
- Result: pass `66/66`

Round 17 remains incomplete.
<!-- CODEX's REVIEW RESULT  END  -->
---

## Goal Tracker Reference (READ-ONLY after Round 0)

Before starting work, **read** @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md to understand:
- The Ultimate Goal and Acceptance Criteria you're working toward
- Which tasks are Active, Completed, or Deferred
- Any Plan Evolution that has occurred
- Open Issues that need attention

**IMPORTANT**: You CANNOT directly modify goal-tracker.md after Round 0.
If you need to update the Goal Tracker, include a "Goal Tracker Update Request" section in your summary (see below).

---

Note: You MUST NOT try to exit by lying, editing loop state files, or executing `cancel-rlcr-loop`.

After completing the work, please:
0. If the `code-simplifier` plugin is installed, use it to review and optimize your code. Invoke via: `/code-simplifier`, `@agent-code-simplifier`, or `@code-simplifier:code-simplifier (agent)`
1. Commit your changes with a descriptive commit message
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-18-summary.md

## Task Tag Routing Reminder

Follow the plan's per-task routing tags strictly:
- `coding` task -> Claude executes directly
- `analyze` task -> execute via `/humanize:ask-codex`, then integrate the result
- Keep Goal Tracker Active Tasks columns `Tag` and `Owner` aligned with execution

**If Goal Tracker needs updates**, include this section in your summary:
```markdown
## Goal Tracker Update Request

### Requested Changes:
- [E.g., "Mark Task X as completed with evidence: tests pass"]
- [E.g., "Add to Open Issues: discovered Y needs addressing"]
- [E.g., "Plan Evolution: changed approach from A to B because..."]
- [E.g., "Defer Task Z because... (impact on AC: none/minimal)"]

### Justification:
[Explain why these changes are needed and how they serve the Ultimate Goal]
```

Codex will review your request and update the Goal Tracker if justified.
