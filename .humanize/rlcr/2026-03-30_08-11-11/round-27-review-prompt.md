# Code Review - Round 27

## Original Implementation Plan

**IMPORTANT**: The original plan that Claude is implementing is located at:
@document/plan.md

You MUST read this plan file first to understand the full scope of work before conducting your review.
This plan contains the complete requirements and implementation details that Claude should be following.

Based on the original plan and @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-27-prompt.md, Claude claims to have completed the work. Please conduct a thorough critical review to verify this.

---
Below is Claude's summary of the work completed:
<!-- CLAUDE's WORK SUMMARY START -->
# Round 27 Summary

## Objective

Close Milestone 3 oracle gaps: task3a (exponential-map oracle fixtures), task3f (principal curvature oracle fixtures), and task3c (Brenner-through-element-energy production path validation).

## Work Completed

### task3f + task3a — Principal Curvature and Exponential-Map Oracle Fixtures

Created `test/cases/tools/dump_principal_exponential_oracle.f90`, a standalone Fortran oracle driver that:
- Reads the archived compression state (nano_dims, nano_general, nano_zero, nano_final_config, nano_Mesh)
- Iterates over interior element patches to find 10 cases (elements 83–87, Gauss points 1–2)
- For each case: calls `metric()` → `curv()` → `principal()` (full derivatives), computes bond geometry with eta=0 (A_norm, Ei independent of Newton), calls `def_bonds()` (full derivatives)
- Writes 36-row fixture: header + C_elem + curv0_elem + flag_num_diff + curvppal + vppal(2×2) + dcurvppaldC(2×3) + dcurvppaldk(2×3) + dvppaldC(2×2×3) + dvppaldk(2×2×3) + A_norm(3) + Ei(3×2) + pe(6) + dpedC(6×3) + dpedk(6×3)

Generated 10 fixture files: `test/cases/principal_exponential_oracle/case_01.dat` … `case_10.dat`.

Created `test/cases/principal_exponential_oracle/build_provenance.md` documenting oracle commit, fixture scope, row format, tolerance rationale, and reproduction recipe.

Added oracle tests:
- `test/unit/test_principal.cpp`: `Principal.MatchesArchivedCompressionFortranOracle` — compares all principal outputs (curvppal, vppal, all 4 derivative tensors) at 1e-12 absolute tolerance
- `test/unit/test_exponential.cpp`: `Exponential.MatchesArchivedCompressionFortranOracle` — compares pe[6], dpedC[6][3], dpedk[6][3] at 1e-12 absolute tolerance

Both tests pass over all 10 fixtures.

### task3c — Brenner Material through Element-Energy Production Path

Created `test/cases/tools/dump_element_energy_brenner_oracle.f90`, a Fortran oracle driver that:
- Uses element 83's archived geometry from `nano_final_config.dat`
- Hardcodes Brenner REBO material (nCode_Pot=2, A0=0.142, A1=0.142, Vs=[0.60310500860214233, 26.25, 0.9], Va=[0.75400000810623169, 0.149, 0.25]) — same as `dump_constitutive_oracle.f90`
- Runs the full production path: metric → curv → principal → newton_inner (Inner_Brenner) → def_bonds/def_bonds_ → Brenner (outer) → Stresses → f_elem assembly
- Writes 14-row fixture to `test/cases/element_energy_oracle/brenner_geom_np1/case_01.dat`

Added `ElementEnergy.BrennerMaterialMatchesFortranOracle` test in `test/unit/test_element_energy.cpp`:
- Assembles MatData with nCode_Pot=2 and Brenner parameters
- Loads element 83 geometry from the archived compression state
- Calls `compute_element_energy` and compares W_elem at 1e-6, all 12×3 f_elem components at max(1e-6, |expected|×1e-6)
- Test passes

Updated `test/cases/element_energy_oracle/build_provenance.md` with the new fixture section.

## Files Changed

### New Files
1. `test/cases/tools/dump_principal_exponential_oracle.f90` — Fortran oracle driver for principal + exponential map
2. `test/cases/principal_exponential_oracle/build_provenance.md` — fixture provenance documentation
3. `test/cases/principal_exponential_oracle/case_01.dat` … `case_10.dat` — 10 Fortran-derived fixture files (36 rows each)
4. `test/cases/tools/dump_element_energy_brenner_oracle.f90` — Fortran oracle driver for Brenner element-energy
5. `test/cases/element_energy_oracle/brenner_geom_np1/case_01.dat` — Brenner element-energy fixture (14 rows)

### Modified Files
6. `test/unit/test_principal.cpp` — added `PrincipalExponentialFixture`, `read_pexp_fixture`, `Principal.MatchesArchivedCompressionFortranOracle`
7. `test/unit/test_exponential.cpp` — added `PExpFixture`, `read_pexp_fixture_exp`, `Exponential.MatchesArchivedCompressionFortranOracle`
8. `test/unit/test_element_energy.cpp` — added `ElementEnergy.BrennerMaterialMatchesFortranOracle`
9. `test/cases/element_energy_oracle/build_provenance.md` — added `brenner_geom_np1/case_01.dat` section
10. `.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md` — task3a, task3c, task3f marked completed; plan version 30; Round 27 evolution log entry

## Validation

- **Unit tests**: 63/63 pass
- **Integration tests**: 18/18 pass
- All oracle comparisons pass at stated tolerances (1e-12 for principal/exponential, 1e-6 for Brenner element-energy)

## Acceptance Criteria Addressed

- **AC-5** (principal curvature oracle fixtures and test): CLOSED — task3f done
- **AC-6** (exponential-map oracle fixtures and test): CLOSED — task3a done
- **AC-7** (Brenner material through element-energy production path): CLOSED — task3c done

## Remaining Items

- **task4a** (global energy/force assembly, Milestone 4 Phase A): Deferred to Round 28 per Round 27 contract.
- **task6a-6c** (runtime vdW/self-contact): Not started, blocked side issue.
- **task8c** (AGENT.md + translation_notes.md): Not started.

## BitLesson Delta

Action: none
Lesson ID(s): NONE
Notes: No new non-obvious problems encountered. The `principal_` flag-as-input lesson (BL-20260404-principal-flag-input) was captured in Round 26 and applied correctly here.
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

## Part 3: Required Finding Classification

You MUST classify your findings into these lanes:
- **Mainline Gaps**: plan-derived work or AC progress that is missing, incomplete, or regressing
- **Blocking Side Issues**: bugs or implementation issues that block the current mainline objective from succeeding safely
- **Queued Side Issues**: valid non-blocking follow-up issues that should be documented but must NOT take over the next round

Also include a one-line verdict:
```
Mainline Progress Verdict: ADVANCED / STALLED / REGRESSED
```

This verdict line is mandatory. If you omit it, the Humanize stop hook will block the round and require the review to be rerun.

If Claude mostly worked on queued side issues and failed to advance the mainline, say so explicitly.

## Part 4: ## Goal Tracker Update Requests (YOUR RESPONSIBILITY)

Claude should normally keep the **mutable section** of `goal-tracker.md` up to date directly. If Claude's summary contains a "Goal Tracker Update Request" section, or if you detect tracker drift during review, YOU must:

1. **Evaluate the tracker state**: Is the mutable section still aligned with the Ultimate Goal and current AC progress?
2. **If correction is needed**: Update @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md yourself with the requested changes:
   - Move tasks between Active/Completed/Deferred sections as appropriate
   - Add entries to "Plan Evolution Log" with round number and justification
   - Add new issues to "Blocking Side Issues" or "Queued Side Issues" as appropriate
   - **NEVER modify the IMMUTABLE SECTION** (Ultimate Goal and Acceptance Criteria)
3. **If you reject a requested tracker change**: Include in your review why it was rejected

Common update requests you should handle:
- Task completion: Move from "Active Tasks" to "Completed and Verified"
- New blocking issues: Add to "Blocking Side Issues"
- New queued issues: Add to "Queued Side Issues"
- Plan changes: Add to "Plan Evolution Log" with your assessment
- Deferrals: Only allow with strong justification; add to "Explicitly Deferred"

## Part 5: Output Requirements

- In short, your review comments can include: problems/findings/blockers; claims that don't match reality; implementation plans for deferred work (to be implemented now); implementation plans for unfinished work; goal alignment issues.
- Your output should be structured so Claude can tell which items are mainline gaps, blocking side issues, and queued side issues.
- If after your investigation the actual situation does not match what Claude claims to have completed, or there is pending work to be done, output your review comments to @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-27-review-result.md.
- **CRITICAL**: Only output "COMPLETE" as the last line if ALL tasks from the original plan are FULLY completed with no deferrals
  - DEFERRED items are considered INCOMPLETE - do NOT output COMPLETE if any task is deferred
  - UNFINISHED items are considered INCOMPLETE - do NOT output COMPLETE if any task is pending
  - The ONLY condition for COMPLETE is: all original plan tasks are done, all ACs are met, no deferrals or pending work allowed
- The word COMPLETE on the last line will stop Claude.
