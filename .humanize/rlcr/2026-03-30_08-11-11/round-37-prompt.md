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
# Round 36 Review

Mainline Progress Verdict: STALLED, NOT COMPLETE

Goal Alignment Summary:
`ACs: 6/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. High: Round 36 did not execute the required remaining scope from the prompt or the original plan. The prompt explicitly said the work was not finished and required executing the previously assigned implementation plan through the remaining milestones [`round-36-prompt.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-36-prompt.md#L1), [`round-36-prompt.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-36-prompt.md#L63), but the summary says the mainline was narrowed to replay-fixture provenance and leaves AC-12 plus Milestones 6-8 untouched [`round-36-summary.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-36-summary.md#L4), [`round-36-summary.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-36-summary.md#L33). The provenance cleanup is real, but it is hygiene work, not completion of the required remaining plan scope.

2. Medium: AC-12 is still not implemented, and the archived-oracle harness still preserves stale VTU artifacts in temp runs. [`test_e2e_compression.cpp`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_e2e_compression.cpp#L112) removes only `energy.dat`, `force.dat`, `output.dat`, and `nano_final_config.dat` after copying the archived `np1` case, so archived `mesh_config_*.vtu` and `mesh_config_series.pvd` remain in the temp directory before `crunch_it` even starts [`test_e2e_compression.cpp`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_e2e_compression.cpp#L153). Meanwhile [`solver.cpp`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/solver.cpp#L386) still writes only scalar/text outputs plus `nano_final_config.dat`, and [`main.cpp`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L70) still treats VTU files only as archived inputs for `--single-step`. Milestone 5 is therefore still missing exactly where the plan requires runtime VTU emission and validation [`plan.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L232).

3. Medium: Milestones 6 through 8 remain structurally absent, not merely unverified. [`simulator.hpp`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/simulator.hpp#L13) carries no runtime `VdwData`, `CreaseData`, or checkpoint payload, [`simulator.cpp`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/simulator.cpp#L130) still reads only the compression `nano_*` files plus the optional imperfection trace, [`solver.cpp`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/solver.cpp#L49) still hardcodes `E_vdw = 0` with no runtime energy decomposition, and [`load_controller.cpp`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/load_controller.cpp#L65) still throws for `nCodeLoad=30/31`. `AGENT.md` and `document/translation_notes.md` are also still missing. These are unchanged blockers for AC-8 through AC-13.

## Goal Alignment Audit

- AC-1 through AC-6 remain met from prior rounds.
- AC-7 is still red. The provenance cleanup is real, and the tracker should record it, but the deterministic runtime mismatch remains unchanged [`round-36-summary.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-36-summary.md#L24).
- AC-8 is still partial: preprocessor-side vdW is done, runtime vdW/self-contact is absent.
- AC-9 and AC-10 are still not met: cyclic control, crease memory/analysis, and checkpoint/restart are not implemented on the runtime path.
- AC-11 is still partial: assembly exists, but real `np=1/2/4` runtime parity and checkpoint-rank validation are still missing.
- AC-12 is still not met: there is still no runtime VTU writer, and the current temp-copy harness leaves archived VTUs in place.
- AC-13 is still partial: build infrastructure exists, but `AGENT.md` and `document/translation_notes.md` are still missing.
- Forgotten items: none. The tracker still covers the remaining plan scope.
- Explicit deferrals: none in the tracker, but the Round 36 summary again leaves Milestones 5-8 unfinished. Those items remain incomplete original-plan scope.

## Goal Tracker Update Review

Claude's Round 36 tracker update request is approved as a provenance correction only. I updated [`goal-tracker.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md) directly to:

- add a Round 36 plan-evolution entry for moving the replay trace into a harness-side fixture,
- update `task4d` and `task4f` to note that the deterministic failing trajectory is unchanged under the cleaned harness,
- rename the open issue so it reflects that the artifact is now a harness-side C++ fixture rather than an archived-case file.

No task statuses were advanced.

## Verification Notes

- `cmake --build build --target integration_tests -j4` passed in this review.
- Fresh runtime replay attempts were not reliable in this sandbox because OpenMPI again hit local socket-bind restrictions (`bind() failed: Operation not permitted`) before producing usable end-to-end evidence, so the runtime findings above rely on code inspection plus the committed Round 36 evidence rather than a fresh full archived-oracle replay.

## Required Implementation Plan

1. Finish AC-7 on the real runtime path, not just the replay harness. Capture the Fortran imperfection sequence or port the exact Fortran RNG/seed behavior, then instrument `pasapas`, minimization, and reaction checkpoints against the archived `np1` runtime until [`test_e2e_compression.cpp`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_e2e_compression.cpp#L167) passes against archived `energy.dat`, `force.dat`, and `nano_final_config.dat`.

2. Implement Milestone 5 exactly as planned. Add runtime VTU snapshot emission and `mesh_config_series.pvd` generation in `pasapas`, make the integration harness delete archived `mesh_config_*.vtu` and `.pvd` files before each temp run, and add executable-path assertions that the newly written XML snapshots match the archived compression series [`plan.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L232).

3. Extend the runtime input/state model for Milestones 6 and 7. Load `nano_vdw.dat`, `nano_crease.dat`, and `nano_checkpoint.dat` into `SimulatorInput` and runtime state; translate simulator-side vdW/self-contact, cyclic controller, crease memory/analysis, and checkpoint restart from the Fortran sources; and verify them against the archived self-contact and cyclic simulator cases [`plan.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L236), [`plan.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L242).

4. Finish Milestone 8 without deferral. Add real `np=1/2/4` runtime parity tests and checkpoint rank-count validation, create `AGENT.md`, create `document/translation_notes.md`, and expand the integration suite so every remaining AC has direct executable-path evidence [`plan.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L249).

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
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-37-summary.md

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
