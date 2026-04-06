# Code Review - Round 13

## Original Implementation Plan

**IMPORTANT**: The original plan that Claude is implementing is located at:
@document/plan.md

You MUST read this plan file first to understand the full scope of work before conducting your review.
This plan contains the complete requirements and implementation details that Claude should be following.

Based on the original plan and @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-13-prompt.md, Claude claims to have completed the work. Please conduct a thorough critical review to verify this.

---
Below is Claude's summary of the work completed:
<!-- CLAUDE's WORK SUMMARY START -->
# Round 13 Summary

## Work Completed
- Reconciled the archived `nano_general.dat` material encoding with the C++ reader/writer in `src/core/io.cpp`. The C++ path now preserves the actual Fortran parameter blocks for `nCode_Pot=1` (Morse), `nCode_Pot=2` (Brenner), and `nCode_Pot=3` (MM3) instead of discarding them behind hard-coded constants.
- Extended `src/core/constitutive.cpp` so the inner constitutive path now supports both `nCode_Pot=1` (Morse / `Inner_Morse`) and `nCode_Pot=2` (Brenner / `Inner_Brenner`) using the same `evaluate_inner_potential()` / `solve_inner_newton()` entry points. This removes the file-backed integration blocker that prevented committed compression materials from reaching the translated constitutive layer at all.
- Corrected the material-code semantics in `include/fce/types.hpp` and `document/fortran_conventions.md` to match the frozen Fortran sources: `1=Morse`, `2=Brenner`, `22=Brenner2`, `3=MM3`.
- Updated the unit tests so `ReadGeneral.GrapheneCompression` now verifies the committed Morse parameter block from `test/cases/graphene_compression_prepro/nano_general.dat`, and `NewtonInner.AcceptsCommittedCompressionMaterialPayload` proves the translated inner solver accepts that file-backed material without hand-built remapping.
- Updated `test/unit/test_constitutive.cpp` so the unsupported-potential regression now checks a truly unsupported code (`99`) instead of `1`, which is now intentionally supported.
- Updated `test/cases/README.md` so the constitutive oracle inventory reflects the Round 12 expansion to `newton_inner/case_01.dat` through `case_10.dat`.

## Files Changed
- `include/fce/types.hpp`
- `src/core/io.cpp`
- `src/core/constitutive.cpp`
- `document/fortran_conventions.md`
- `test/unit/test_io.cpp`
- `test/unit/test_constitutive.cpp`
- `test/cases/README.md`

## Validation
- `cmake --build build --target unit_tests -j4`
  - Red phase: failed on `ReadGeneral.GrapheneCompression` and `NewtonInner.AcceptsCommittedCompressionMaterialPayload` as expected
  - Final result: pass
- `ctest --test-dir build --output-on-failure -R 'ReadGeneral.GrapheneCompression|NewtonInner.AcceptsCommittedCompressionMaterialPayload|NewtonInner.RejectsUnsupportedPotentialCode|Brenner.DefaultMaterialUsesSupportedPotentialCode'`
  - pass `4/4`
- `cmake --build build --target unit_tests integration_tests -j4`
  - pass
- `ctest --test-dir build --output-on-failure`
  - pass `52/52`

## Remaining Items
- `task3d` still lacks the plan-required archived simulator-state provenance for the 10 AC-6 oracle states. The fixture-count/failure-mode coverage exists, but the main Newton corpus is still generated from helper-defined inputs rather than archived load-step dumps.
- `task3a`, `task3b`, `task3e`, and `task3f` are still unimplemented. The codebase still lacks dedicated C++ ports of `exponential.f90`, `geometry.f90`, `principal.f90`, and `ener_elem.f90`.
- The simulator executable is still a stub, so Milestones 4-8 remain open: solver assembly, L-BFGS, load stepping, runtime vdW/self-contact, VTU output, cyclic/checkpoint logic, MPI equivalence, and AC-13 documentation artifacts.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: `bitlesson-selector` was still unavailable in this shell. I reused the existing file-format and constitutive lessons manually and did not add a new entry.

## Goal Tracker Update Request

### Requested Changes:
- Remove or mark resolved the blocking issue stating that the translated constitutive slice is incompatible with committed `nano_general.dat` inputs. Round 13 reconciles the `nCode_Pot` mapping in the reader, the constitutive dispatcher, and the conventions doc, and adds a file-backed regression proving the archived compression payload is accepted.
- Update `task3c` notes to reflect that the remaining material-code semantics blocker has been addressed in addition to the Round 12 cutoff-path work.
- Keep `task3d` pending until the Newton AC-6 fixtures are sourced from archived simulator load-step outputs.
- Keep `task3a`, `task3b`, `task3e`, `task3f`, and all simulator/runtime milestones open.

### Justification:
Round 13 closes the specific semantic blocker that made the translated constitutive layer incompatible with the committed oracle inputs. That is necessary integration progress for Milestone 3 and AC-7, but it does not complete the missing geometry/element kernels or the solver/runtime milestones that still block end-to-end simulation parity.
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
- If after your investigation the actual situation does not match what Claude claims to have completed, or there is pending work to be done, output your review comments to @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-13-review-result.md.
- **CRITICAL**: Only output "COMPLETE" as the last line if ALL tasks from the original plan are FULLY completed with no deferrals
  - DEFERRED items are considered INCOMPLETE - do NOT output COMPLETE if any task is deferred
  - UNFINISHED items are considered INCOMPLETE - do NOT output COMPLETE if any task is pending
  - The ONLY condition for COMPLETE is: all original plan tasks are done, all ACs are met, no deferrals or pending work allowed
- The word COMPLETE on the last line will stop Claude.
