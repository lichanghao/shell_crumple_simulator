# Code Review - Round 15

## Original Implementation Plan

**IMPORTANT**: The original plan that Claude is implementing is located at:
@document/plan.md

You MUST read this plan file first to understand the full scope of work before conducting your review.
This plan contains the complete requirements and implementation details that Claude should be following.

Based on the original plan and @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-15-prompt.md, Claude claims to have completed the work. Please conduct a thorough critical review to verify this.

---
Below is Claude's summary of the work completed:
<!-- CLAUDE's WORK SUMMARY START -->
# Round 15 Summary

## Work Completed
- Fixed the new `principal` port so non-finite or materially negative curvature discriminants now raise `std::invalid_argument` instead of being silently clamped into the repeated-curvature fallback path.
- Expanded `test/unit/test_principal.cpp` with:
  - a near-repeated-curvature regression that stays on the numerical-fallback path,
  - a non-finite discriminant regression,
  - direct finite-difference checks for `dvppaldC` and `dvppaldk`.
- Expanded `test/unit/test_exponential.cpp` with a fully coupled finite-difference regression that exercises the nonzero `dcurvppal*`, `dvppal*`, and `dpedk` paths through `compute_deformed_bonds_with_derivatives(...)`.
- Reduced the sidecar status of the new `exponential` module by routing the live constitutive path in `src/core/constitutive.cpp` through `compute_deformed_bonds(...)` for the bond-state scalars consumed by `evaluate_inner_potential(...)`, while leaving the local `eta`-derivative machinery intact.
- Investigated the archived simulator artifacts for AC-6 provenance and confirmed that the committed oracle repository currently exposes only final-state `nano_config.dat` outputs and cycle-end `nano_checkpoint.dat` outputs, not the 10 intermediate per-load-step Newton states requested by the plan.
- Updated `test/cases/constitutive_oracle/build_provenance.md` to record that archived-state provenance blocker explicitly instead of leaving it implicit.

## Files Changed
- `src/core/principal.cpp`
- `src/core/constitutive.cpp`
- `test/unit/test_principal.cpp`
- `test/unit/test_exponential.cpp`
- `test/cases/constitutive_oracle/build_provenance.md`

## Validation
- `cmake --build build --target unit_tests -j4`
- `ctest --test-dir build --output-on-failure -R 'Principal|Exponential'`
  - pass `9/9`
- `ctest --test-dir build --output-on-failure -R 'Constitutive|NewtonInner|Brenner|Principal|Exponential'`
  - pass `19/19`
- `cmake --build build --target unit_tests integration_tests -j4`
  - pass
- `ctest --test-dir build --output-on-failure`
  - pass `61/61`

## Remaining Items
- `task3b` and `task3e` are still missing. There is still no dedicated `geometry` / `curv` module and no translated `element_energy` / `ener_elem` module.
- The simulator executable is still a stub, so Milestones 4-8 remain open.
- AC-6 still does not have the plan-required 10 archived simulator-state Newton fixtures; Round 15 only tightened the provenance documentation around that blocker.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: `bitlesson-selector` is not available in this shell. I read `.humanize/bitlesson.md` before each task and proceeded with `NONE`.

## Goal Tracker Update Request

### Requested Changes:
- Add a Round 15 plan-evolution entry recording the verified principal discriminant hardening, the expanded principal/exponential derivative coverage, the constitutive-path use of `compute_deformed_bonds(...)`, and the passing full suite at `61/61`.
- Update `task3a` notes to reflect that the dedicated `exponential` module is now consumed by the live constitutive inner-potential path for bond-state scalars, not only by standalone unit tests.
- Update `task3f` notes to reflect that `test/unit/test_principal.cpp` now covers `dvppaldC` and `dvppaldk`, and that non-finite discriminants are rejected instead of silently falling back.
- Update `task3d` notes and/or the Milestone 3 blocking issue to reference the newly documented provenance fact in `test/cases/constitutive_oracle/build_provenance.md`: the archived repository currently contains final-state and cycle-end simulator states, but not the 10 intermediate per-load-step Newton states required to close AC-6.

### Justification:
Round 15 directly addresses two of the Round 14 review findings and partially addresses the third. The principal kernel now matches the intended failure semantics for invalid discriminants, the missing derivative coverage has been added, and the `exponential` kernel is no longer purely sidecar because the live constitutive path now consumes its bond-state scalars. AC-6 and the broader Milestone 3/solver milestones remain incomplete, but the tracker should reflect the verified progress and the now-explicit provenance blocker accurately.
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
- If after your investigation the actual situation does not match what Claude claims to have completed, or there is pending work to be done, output your review comments to @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-15-review-result.md.
- **CRITICAL**: Only output "COMPLETE" as the last line if ALL tasks from the original plan are FULLY completed with no deferrals
  - DEFERRED items are considered INCOMPLETE - do NOT output COMPLETE if any task is deferred
  - UNFINISHED items are considered INCOMPLETE - do NOT output COMPLETE if any task is pending
  - The ONLY condition for COMPLETE is: all original plan tasks are done, all ACs are met, no deferrals or pending work allowed
- The word COMPLETE on the last line will stop Claude.
