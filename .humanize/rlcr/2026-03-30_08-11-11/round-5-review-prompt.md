# Code Review - Round 5

## Original Implementation Plan

**IMPORTANT**: The original plan that Claude is implementing is located at:
@document/plan.md

You MUST read this plan file first to understand the full scope of work before conducting your review.
This plan contains the complete requirements and implementation details that Claude should be following.

Based on the original plan and @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-5-prompt.md, Claude claims to have completed the work. Please conduct a thorough critical review to verify this.

---
Below is Claude's summary of the work completed:
<!-- CLAUDE's WORK SUMMARY START -->
# Round 5 Summary

## Work Completed
- Committed `eef0ab9` (`Harden preprocessor oracle evidence`).
- Made `PreprocessorOracle` tests create unique temporary directories instead of sharing one fixed path, removing the known race risk for parallel or overlapping runs.
- Added direct archived ghost-coordinate verification to the preprocessor oracle comparator by materializing ghost nodes from both actual and archived configs and comparing the appended coordinates edge-by-edge.

## Files Changed
- `test/integration/test_prepro_oracle.cpp`
- `test/support/oracle_compare.cpp`

## Validation
- `cmake --build build --target integration_tests -j4 && ctest --test-dir build --output-on-failure -R '^PreprocessorOracle'` -> PASS (`5/5`) after the temp-dir hardening change
- `cmake --build build --target integration_tests -j4 && ctest --test-dir build --output-on-failure -R '^PreprocessorOracle'` -> PASS (`5/5`) after the ghost-coordinate oracle comparison change
- `ctest --test-dir build --output-on-failure` -> PASS (`32/32`)

## Remaining Items
- `task2g` remains pending: the real `nvdw=1` preprocessing path, neighbor-list generation, shape functions, and `vdw_previous`-equivalent state are still not translated.
- AC-3 remains partial: the required 5 interior and 5 boundary Fortran B-spline oracle fixtures are still missing.
- Milestones 3 through 8 remain pending, including the simulator mainline, vdW runtime, cyclic runtime controller, checkpoint/restart, MPI equivalence, VTU output, and final documentation.

## Goal Tracker Update Request

### Requested Changes:
- Remove the AC-4 blocker stating that positive ghost-position evidence is still indirect.
- Remove the queued side issue stating that `PreprocessorOracle` uses a fixed temporary directory.
- Update the AC-4 evidence row to cite the direct archived ghost-coordinate comparison in `test/support/oracle_compare.cpp` together with the passing `PreprocessorOracle` run and `32/32` full-suite regression.
- Update the Plan Evolution / blocker notes to reflect that Round 5 closed the remaining Milestone 2 ghost-evidence and temp-dir harness gaps, while keeping `task2g` and the AC-3 B-spline-fixture gap open.

### Justification:
Round 5 directly addressed the two concrete review findings that were still preventing AC-4 from moving beyond indirect evidence and leaving the harness unsafe for parallel execution. The new comparator path now checks generated ghost coordinates against the archived oracle itself, and the temp-dir factory no longer reuses a shared path. Those issues should no longer remain listed as open blockers, while the larger unresolved Milestone 2 and simulator tasks should stay open.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable failure pattern was discovered; the round closed two already-identified review items.
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
- If after your investigation the actual situation does not match what Claude claims to have completed, or there is pending work to be done, output your review comments to @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-5-review-result.md.
- **CRITICAL**: Only output "COMPLETE" as the last line if ALL tasks from the original plan are FULLY completed with no deferrals
  - DEFERRED items are considered INCOMPLETE - do NOT output COMPLETE if any task is deferred
  - UNFINISHED items are considered INCOMPLETE - do NOT output COMPLETE if any task is pending
  - The ONLY condition for COMPLETE is: all original plan tasks are done, all ACs are met, no deferrals or pending work allowed
- The word COMPLETE on the last line will stop Claude.
