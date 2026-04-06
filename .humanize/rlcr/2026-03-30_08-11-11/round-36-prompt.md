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
# Round 35 Review

Mainline Progress Verdict: ADVANCED, NOT COMPLETE

Goal Alignment Summary:
`ACs: 6/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. High: The new reproducibility harness changes the AC-7 input contract instead of replaying the frozen archived case. [`load_simulator_input()`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/simulator.cpp#L130) now consumes optional `imperfection_trace.dat` from the case directory, the archived compression case now ships that file and documents it as a C++ replay aid in [`test/cases/README.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/README.md#L21), and the main archived-oracle regression copies that extra file into its temp case without removing it in [`test_e2e_compression.cpp`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_e2e_compression.cpp#L142). That is useful for reproducibility, but a future green result under this harness would still not prove parity with the original Fortran inputs unless the trace is extracted from Fortran or the exact RNG behavior is ported.

2. Medium: AC-12 remains untouched. [`main.cpp`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L70) still only reads archived `mesh_config_*.vtu` snapshots in `--single-step` mode, while [`solver.cpp`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/solver.cpp#L185) and [`solver.cpp`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/solver.cpp#L440) still only write `energy.dat`, `force.dat`, `output.dat`, and `nano_final_config.dat`. There is still no runtime VTU writer or `.pvd` series generation, even though Milestone 5 is explicit in [`document/plan.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L232).

3. Medium: Milestones 6 through 8 are still structurally absent, not merely unverified. [`SimulatorInput`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/simulator.hpp#L13) still carries no runtime `VdwData`, `CreaseData`, or checkpoint payload, and [`load_simulator_input()`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/simulator.cpp#L130) still never reads `nano_vdw.dat`, `nano_crease.dat`, or `nano_checkpoint.dat`. The tracker still correctly shows `task6a` through `task8d` pending in [`goal-tracker.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L110). This is incomplete original-plan scope, not optional future work.

## Goal Alignment Audit

- AC-1 through AC-6 remain met from prior rounds.
- AC-7 advanced this round because the executable-path mismatch is now reproducible, but it is still red and still depends on a synthetic C++ trace artifact.
- AC-8 is only partial: preprocessor-side vdW is done, runtime vdW/self-contact is still absent.
- AC-9, AC-10, and AC-12 are still not met.
- AC-11 is still partial: assembly exists, but multi-rank runtime parity and checkpoint-rank validation are still missing.
- AC-13 is still partial: build infrastructure exists, but `AGENT.md` and `document/translation_notes.md` are still missing.
- Forgotten items: none. The tracker still represents the remaining plan scope.
- Explicit deferrals: none in the tracker, but Claude's summary still leaves Milestones 5 through 8 incomplete. Those items must now be finished, not deferred again.

## Goal Tracker Update Review

Claude's Round 35 tracker update request was approved with a provenance correction. I updated [`goal-tracker.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L42) directly to:

- add a Round 35 plan-evolution entry,
- update `task4d` and `task4f` with the new deterministic-trace evidence,
- replace the old stochastic-runtime blocker with the narrower deterministic-trajectory blocker,
- record that the checked-in `imperfection_trace.dat` is a C++ replay artifact, not frozen Fortran output.

## Verification Notes

- `cmake --build build --target unit_tests integration_tests crunch_it -j4` passed in this review.
- Fresh runtime executions were not reliable to re-run end-to-end inside this sandbox because OpenMPI hit local socket-bind restrictions, so the findings above rely on code inspection plus the checked-in evidence rather than a fresh full archived-oracle replay.

## Required Implementation Plan

1. Remove the synthetic helper from the archived-case contract. Keep deterministic replay support, but make the integration harness write `imperfection_trace.dat` into the temp copy explicitly instead of checking it into the archived case, and in parallel either capture the per-step `a` sequence from the Fortran oracle or port the exact Fortran RNG/seed behavior so the replay becomes source-backed.

2. Finish AC-7 on the real runtime path. Instrument `pasapas` at the same checkpoints as canonical Fortran, compare traced step-by-step state against the archived runtime, and fix the remaining load-step, minimization, and reaction semantics until [`test_e2e_compression.cpp`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_e2e_compression.cpp#L156) passes against archived `energy.dat`, `force.dat`, and `nano_final_config.dat`.

3. Implement Milestone 5 exactly as planned in [`document/plan.md`](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L232): add a runtime VTU snapshot writer plus `mesh_config_series.pvd`, emit step 0 through final step during `pasapas`, and add integration coverage that validates XML loadability and field parity against the archived compression snapshots.

4. Widen the runtime input/state model before continuing Milestones 6 and 7. Extend `SimulatorInput` and runtime state to load and carry `nano_vdw.dat`, `nano_crease.dat`, and checkpoint state, then translate simulator-side vdW/self-contact, cyclic controller, crease memory/analysis, and checkpoint restart from the Fortran sources using the archived cyclic and self-contact cases as the oracle baselines.

5. Finish Milestone 8 without deferral. Add real `np=1/2/4` runtime parity tests, checkpoint compatibility across rank counts, create `AGENT.md`, create `document/translation_notes.md`, and expand the integration suite so every remaining AC has direct executable-path evidence.

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
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-36-summary.md

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
