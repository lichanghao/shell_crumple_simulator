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
# Round 15 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 9/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. The original plan is still substantially incomplete, and Round 15 does not change that status. There are still no translated `geometry` or `ener_elem` modules anywhere under [include/fce](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce) or [src/core](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core), and the simulator remains a stub in [src/simulator/main.cpp:1](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp:1). That leaves `task3b`, `task3e`, all of Milestone 4, Milestone 5, the runtime half of Milestone 6, all of Milestone 7, and most of Milestone 8 still unimplemented. Claude’s summary admits this, so this round cannot be treated as more than narrow kernel/test progress.

2. AC-6 is still not satisfied. [test/cases/constitutive_oracle/build_provenance.md:48](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md:48) now documents the blocker clearly, but the blocker itself remains: the committed `newton_inner` corpus is still helper-generated rather than derived from 10 archived simulator load-step states. The tracker already says this in [goal-tracker.md:79](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md:79). Documentation is not closure.

3. The Round 15 constitutive-path reuse is real but narrow. [src/core/constitutive.cpp:348](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp:348) now calls `compute_deformed_bonds(...)` to populate `pe`, and the new principal/exponential derivative tests are materially better in [test/unit/test_principal.cpp:106](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_principal.cpp:106) and [test/unit/test_exponential.cpp:101](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_exponential.cpp:101). But this is still not plan-level constitutive integration: the η-derivative path remains duplicated locally in [src/core/constitutive.cpp:350](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp:350), there are still no Fortran-derived oracle fixtures for the dedicated `principal` and `exponential` modules, and none of this advances the stub simulator toward AC-7 or AC-9 on its own.

## Goal Alignment Check

### Acceptance Criteria Progress

| AC | Status | Assessment |
|----|--------|------------|
| AC-1 | MET | Oracle artifacts and conventions remain in place. |
| AC-2 | MET | Preprocessor oracle parity remains covered. |
| AC-3 | MET | B-spline oracle fixtures remain covered. |
| AC-4 | MET | Ghost-node oracle artifacts remain covered. |
| AC-5 | PARTIAL | Brenner kernel is translated and verified, but the missing `geometry` and `ener_elem` path means the planned constitutive pipeline is still incomplete. |
| AC-6 | PARTIAL | Newton kernel exists and tests pass, but the required archived simulator-state provenance is still missing. |
| AC-7 | NOT MET | No solver core, no assembly, no reaction-force path, and [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp) is still a stub. |
| AC-8 | PARTIAL | Preprocessor-side vdW is complete, runtime vdW/self-contact is still absent. |
| AC-9 | NOT MET | No cyclic controller, crease-memory path, or crease analysis implementation exists. |
| AC-10 | NOT MET | No checkpoint/restart implementation or restart validation exists. |
| AC-11 | PARTIAL | MPI wrappers exist, but there is still no solver to verify cross-rank parity on. |
| AC-12 | NOT MET | No VTU writer or validation path exists in the C++ codebase. |
| AC-13 | PARTIAL | Infrastructure exists, but [AGENT.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/AGENT.md) and [document/translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md) are still missing. |

### Forgotten Items

- No original-plan task IDs are untracked. `task1c` is still represented through the combined completed entry `task1b+1c` in [goal-tracker.md:122](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md:122), so this is bookkeeping compression, not a missing task.

### Deferred Items

- The tracker still has no explicit deferred section entries. That is acceptable, but the unfinished Milestone 3 through Milestone 8 work is still incomplete work and must not be normalized as future-phase scope.

### Plan Evolution

- Claude’s Round 15 update request is mostly justified. The principal hardening, expanded derivative coverage, constitutive reuse of `compute_deformed_bonds(...)` for scalar bond states, and `61/61` suite result are real.
- I approved those tracker edits, but kept `task3a`, `task3d`, and `task3f` pending and kept the Milestone 3 blocking issue open because the original-plan gaps remain.

## Goal Tracker Update Requests

Approved and applied.

- Updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md) to Plan Version 17 with a Round 15 evolution entry documenting the verified principal hardening, expanded principal/exponential derivative coverage, constitutive reuse of `compute_deformed_bonds(...)`, and the passing `61/61` full suite.
- Updated `task3a`, `task3d`, and `task3f` notes so the tracker reflects the actual Round 15 progress without closing those tasks.
- Updated the Milestone 3 blocking issue to reference the newly documented AC-6 provenance fact in [test/cases/constitutive_oracle/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md).

## Required Implementation Plan

1. Finish Milestone 3 as designed, not as sidecar kernels. Add a dedicated geometry module and a dedicated element-energy module under [include/fce](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce) and [src/core](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core). Move the deformation-gradient decomposition, bond-vector preparation, and element-level energy/force workflow out of the ad hoc constitutive-only path so there is one canonical translated pipeline feeding Brenner, principal curvature, exponential bond deformation, and inner Newton.
2. Close AC-6 with archived simulator-state provenance. Instrument the frozen Fortran simulator path to dump 10 intermediate load-step states from the standard compression oracle, archive those states under `test/cases/constitutive_oracle/`, regenerate the `newton_inner` fixtures from those archived states, and update [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md) so the fixtures are no longer helper-defined synthetic inputs.
3. Replace the stub simulator and implement Milestone 4 end to end. Add translated modules for global energy/force assembly, L-BFGS, simulator-side load control, `pasapas`, and reaction-force computation. Then make [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp) run the actual solver on archived `nano_*.dat` input and reproduce the compression oracle outputs needed for AC-7.
4. Implement Milestone 5 instead of leaving VTU output absent. Translate the ParaView writer, emit the expected `.vtu` and `.pvd` outputs during simulator runs, and add validation that matches the archived serial compression snapshots.
5. Implement the runtime half of Milestone 6. Translate the simulator-side vdW kernel and self-contact exclusion logic, integrate them into the assembled energy path, and validate them against the archived self-contact and bilayer-twist oracle cases plus dedicated kernel tests.
6. Implement Milestone 7 completely. Add cyclic load control for `nCodeLoad=31`, irreversible crease-memory updates, crease analysis, and checkpoint/restart handling, then verify them against the archived cyclic crumpling oracle outputs.
7. Finish Milestone 8 cleanly. Add serial-vs-MPI consistency tests for `np=1/2/4`, verify checkpoint portability across rank counts, create [AGENT.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/AGENT.md) and [document/translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md), and only then claim final integration completeness.

## Verification

- `ctest --test-dir build --output-on-failure -R 'Principal|Exponential'`
- `ctest --test-dir build --output-on-failure -R 'Constitutive|NewtonInner|Brenner|Principal|Exponential'`
- `ctest --test-dir build --output-on-failure`
- Result: pass `61/61`

Round 15 remains incomplete.
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
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-16-summary.md

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
