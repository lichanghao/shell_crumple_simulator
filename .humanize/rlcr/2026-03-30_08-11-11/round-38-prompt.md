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
# Round 37 Review

Mainline Progress Verdict: PARTIAL, NOT COMPLETE

Goal Alignment Summary:
`ACs: 7/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. High: The request to close `task5a` is premature. The new runtime writer in [solver.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/solver.cpp#L254) does emit `mesh_config_XXXX.vtu` plus `mesh_config_series.pvd`, but it only writes `Points`, `Cells`, and `inner_displacement` [solver.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/solver.cpp#L276), [solver.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/solver.cpp#L309). The canonical Fortran module being translated, [paraview_vtu_output.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/paraview_vtu_output.f90#L45), also emits `PointData/atomic_density` and `CellData/W_density` [paraview_vtu_output.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/paraview_vtu_output.f90#L109). This round therefore lands a useful slice of Milestone 5, but not the full `paraview_vtu_output.f90` translation the plan requires.

2. Medium: `task5b` remains materially incomplete, and the new tests do not yet satisfy AC-12. The new integration coverage in [test_e2e_compression.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_e2e_compression.cpp#L377) checks step-0 points, step-0 `inner_displacement`, step-0/1 `TimeValue`, and PVD metadata [test_e2e_compression.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_e2e_compression.cpp#L419), but it does not validate full VTU field parity, connectivity/offset/type arrays, later-step VTU state, ParaView/XML loadability, or the AC-12 negative path for uninitialized mesh output. The full archived-oracle test only asserts VTU existence and PVD metadata before it falls back into the still-red AC-7 runtime regression [test_e2e_compression.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_e2e_compression.cpp#L302), [test_e2e_compression.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_e2e_compression.cpp#L334).

3. Medium: The round still leaves the rest of the original plan explicitly unfinished. The summary acknowledges that AC-7 remains red, AC-12 is not fully closed, and Milestones 6-8 remain unimplemented. Those are not optional follow-ups; they are still pending original-plan scope from [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L232). Runtime vdW/self-contact, cyclic/crease/checkpoint, MPI parity, and the AC-13 documentation deliverables remain absent in the current runtime path and tracker.

## Goal Alignment Audit

- AC-1 through AC-6 remain satisfied from prior rounds.
- AC-7 is still red. Round 37 did not address the deterministic solver divergence beyond adding VTU/PVD artifacts around the same failing runtime path.
- AC-8 is still partial: preprocessor-side vdW is complete, but simulator-side vdW/self-contact remains unimplemented.
- AC-9 and AC-10 are still not met: cyclic controller, crease memory, crease analysis, and checkpoint/restart are still missing on the runtime path.
- AC-11 is still partial: assembly exists, but runtime `np=1/2/4` parity and checkpoint rank-count validation are still pending.
- AC-12 has real progress this round, but it is still not met because the writer translation and validation are both partial.
- AC-13 is still partial: [AGENT.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/AGENT.md) and [translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md) still do not exist.
- Forgotten items: none. The tracker still covers the remaining plan scope.
- Deferred items: none are formally recorded in the tracker, but the summary still leaves Milestones 6-8 unfinished. Those remain incomplete work, not approved deferrals.

## Goal Tracker Update Review

Claude's tracker update request is approved only in narrowed form. I updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md) to:

- add a Round 37 plan-evolution entry recording the real VTU/PVD progress and the temp-harness cleanup,
- keep `task5a` pending with notes that the runtime writer is still missing `atomic_density` and `W_density`,
- keep `task5b` pending with notes that the validation still covers only a subset of AC-12,
- update `task4d` and `task4f` notes to record that VTU/PVD artifacts now exist around the same failing AC-7 runtime path,
- add a blocking side issue for the partial VTU translation.

I rejected the specific request to mark `task5a` complete.

## Verification Notes

- `cmake --build build --target integration_tests crunch_it -j4` passed in this review.
- Fresh executable-path runs of the new integration tests were not usable as decisive evidence in this sandbox because OpenMPI again hit the local socket-bind restriction (`bind() failed: Operation not permitted`) before producing reviewable end-to-end results.
- Because of that sandbox limit, the findings above rely on code inspection, the checked-in Fortran source contract, and the committed Round 37 test/code delta rather than a fresh successful `crunch_it` replay.

## Required Implementation Plan

1. Finish `task5a` completely. Move the VTU writer into a dedicated runtime output module, load the runtime data it needs from the simulator path, and emit the full `paraview_vtu_output.f90` contract in the same field order and names: `TimeValue`, `Points`, `PointData/atomic_density`, `Cells`, `CellData/inner_displacement`, and `CellData/W_density`. When `nvdw=0`, emit the canonical zero-valued density arrays instead of omitting them.

2. Finish `task5b` completely. Expand the executable-path integration suite so it parses the generated VTU XML and validates the full DataArray set, including connectivity/offset/type arrays and later-step field values, not just step-0 points. Add a negative test that requesting VTU output from an invalid or uninitialized mesh/runtime state fails explicitly instead of writing corrupt XML.

3. Execute Milestone 6 and Milestone 7 without further deferral. Extend `SimulatorInput` and runtime state to load `nano_vdw.dat`, `nano_crease.dat`, and `nano_checkpoint.dat`; translate the simulator-side vdW/self-contact, cyclic controller, crease-memory update, crease analysis, and checkpoint/restart paths from the Fortran sources; and verify them against the archived self-contact and cyclic simulator cases.

4. Execute Milestone 8 to close the remaining acceptance criteria. Add real runtime parity tests for `np=1`, `np=2`, and `np=4`, add checkpoint compatibility checks across rank counts, create [AGENT.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/AGENT.md) and [translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md), and keep the full integration suite as the top-level proof that AC-1 through AC-13 are satisfied.

Status: Incomplete.
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
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-38-summary.md

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
