# Code Review - Round 28

## Original Implementation Plan

**IMPORTANT**: The original plan that Claude is implementing is located at:
@document/plan.md

You MUST read this plan file first to understand the full scope of work before conducting your review.
This plan contains the complete requirements and implementation details that Claude should be following.

Based on the original plan and @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-28-prompt.md, Claude claims to have completed the work. Please conduct a thorough critical review to verify this.

---
Below is Claude's summary of the work completed:
<!-- CLAUDE's WORK SUMMARY START -->
# Round 28 Summary

## Work Completed
- Implemented Milestone 4 Phase A assembly in `include/fce/simulator.hpp` and `src/core/simulator.cpp`.
- Added archived simulator-case loading, ASCII VTU point parsing, per-element assembly over owned ranges, canonical `J0/2` scaling, ghost-force folding, and MPI all-reduce aggregation.
- Replaced the simulator executable stub in `src/simulator/main.cpp` with a real assembly driver for archived VTU steps.
- Added `test/unit/test_simulator.cpp` coverage for load-step-1 energy parity, split-range accumulation, and corrupted-VTU rejection.
- Updated the RLCR goal tracker to record `task4a` as completed while keeping `task3f`'s repeated-curvature Fortran oracle gap pending.

## Files Changed
- `CMakeLists.txt`
- `.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md`
- `.humanize/rlcr/2026-03-30_08-11-11/round-28-contract.md`
- `include/fce/simulator.hpp`
- `src/core/simulator.cpp`
- `src/simulator/main.cpp`
- `test/unit/test_simulator.cpp`

## Validation
- `cmake --build build --target unit_tests` -> passed
- `./build/unit_tests '--gtest_filter=SimulatorAssembly.*'` -> passed all 3 simulator assembly tests
- `./build/unit_tests` -> passed all 66 unit tests
- `cmake --build build --target crunch_it integration_tests` -> passed
- `./build/integration_tests` -> passed all 18 integration tests
- `./build/crunch_it test/cases/graphene_compression_simulator/np1 1` -> printed `assembled_energy 5.7210527678532267e-05`, `inner_fail 0`, `force_dofs 5541`

## Remaining Items
- `task3f` remains pending because the repeated-curvature `flag_num_diff=true` principal branch still lacks a Fortran-backed archived oracle fixture.
- Milestone 4 Phases B-F (`task4b`-`task4f`) remain open: L-BFGS, load controller, pasapas loop, reaction force, and the full end-to-end serial simulator path.
- Runtime vdW/self-contact (`task6a`-`task6c`) and later milestones remain untouched this round.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable process lesson was identified beyond the existing RLCR guidance.
<!-- CLAUDE's WORK SUMMARY  END  -->
---

## Part 1: Implementation Review

- Your task is to conduct a deep critical review, focusing on finding implementation issues and identifying gaps between "plan-design" and actual implementation.
- Relevant top-level guidance documents, phased implementation plans, and other important documentation and implementation references are located under @docs.
- If Claude planned to defer any tasks to future phases in its summary, DO NOT follow its lead. Instead, you should force Claude to complete ALL tasks as planned.
  - Such deferred tasks are considered incomplete work and should be flagged in your review comments, requiring Claude to address them.
  - If Claude planned to defer any tasks, please explore the codebase in-depth and draft a detailed implementation plan. This plan should be included in your review comments for Claude to follow.
  - Your review should be meticulous and skeptical. Look for any discrepancies, missing features, incomplete implementations.
- If Claude does not plan to defer any tasks, but honestly admits that some tasks are still pending (not yet completed), you should also include those pending tasks in your review.
  - Your review should elaborate on those unfinished tasks, explore the codebase, and draft an implementation plan.
  - A good engineering implementation plan should be **singular, directive, and definitive**, rather than discussing multiple possible implementation options.
  - The implementation plan should be **unambiguous**, internally consistent, and coherent from beginning to end, so that **Claude can execute the work accurately and without error**.

## Part 2: Goal Alignment Check (MANDATORY)

Read @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md and verify:

1. **Acceptance Criteria Progress**: For each AC, is progress being made? Are any ACs being ignored?
2. **Forgotten Items**: Are there tasks from the original plan that are not tracked in Active/Completed/Deferred?
3. **Deferred Items**: Are deferrals justified? Do they block any ACs?
4. **Plan Evolution**: If Claude modified the plan, is the justification valid?

Include a brief Goal Alignment Summary in your review:
```
ACs: X/Y addressed | Forgotten items: N | Unjustified deferrals: N
```

## Part 3: ## Goal Tracker Update Requests (YOUR RESPONSIBILITY)

**Important**: Claude cannot directly modify `goal-tracker.md` after Round 0. If Claude's summary contains a "Goal Tracker Update Request" section, YOU must:

1. **Evaluate the request**: Is the change justified? Does it serve the Ultimate Goal?
2. **If approved**: Update @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md yourself with the requested changes:
   - Move tasks between Active/Completed/Deferred sections as appropriate
   - Add entries to "Plan Evolution Log" with round number and justification
   - Add new issues to "Open Issues" if discovered
   - **NEVER modify the IMMUTABLE SECTION** (Ultimate Goal and Acceptance Criteria)
3. **If rejected**: Include in your review why the request was rejected

Common update requests you should handle:
- Task completion: Move from "Active Tasks" to "Completed and Verified"
- New issues: Add to "Open Issues" table
- Plan changes: Add to "Plan Evolution Log" with your assessment
- Deferrals: Only allow with strong justification; add to "Explicitly Deferred"

## Part 4: Output Requirements

- In short, your review comments can include: problems/findings/blockers; claims that don't match reality; implementation plans for deferred work (to be implemented now); implementation plans for unfinished work; goal alignment issues.
- If after your investigation the actual situation does not match what Claude claims to have completed, or there is pending work to be done, output your review comments to @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-28-review-result.md.
- **CRITICAL**: Only output "COMPLETE" as the last line if ALL tasks from the original plan are FULLY completed with no deferrals
  - DEFERRED items are considered INCOMPLETE - do NOT output COMPLETE if any task is deferred
  - UNFINISHED items are considered INCOMPLETE - do NOT output COMPLETE if any task is pending
  - The ONLY condition for COMPLETE is: all original plan tasks are done, all ACs are met, no deferrals or pending work allowed
- The word COMPLETE on the last line will stop Claude.
