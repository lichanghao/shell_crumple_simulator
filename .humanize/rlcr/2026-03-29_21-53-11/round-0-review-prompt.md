# Code Review - Round 0

## Original Implementation Plan

**IMPORTANT**: The original plan that Claude is implementing is located at:
@document/plan.md

You MUST read this plan file first to understand the full scope of work before conducting your review.
This plan contains the complete requirements and implementation details that Claude should be following.

Based on the original plan and @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-29_21-53-11/round-0-prompt.md, Claude claims to have completed the work. Please conduct a thorough critical review to verify this.

---
Below is Claude's summary of the work completed:
<!-- CLAUDE's WORK SUMMARY START -->
# Round 0 Summary — Fortran Archaeology and Oracle Capture

## What Was Implemented

### task0a — Fortran oracle baseline, graphene compression case

The Fortran oracle at commit `7d3f77f` of `../finite_crystal_elasticity/` was confirmed built (gfortran 15.2.0 + OpenMPI). The preprocessor (`PrePro`) and simulator (`crunch_it`) reference outputs were archived for the standard graphene compression case:

- Config: 40×40 mesh, 20 nm×20 nm, nCodeLoad=3 (uniaxial compression), nloadstep=50, ngauss=2, no vdW
- `test/cases/graphene_compression_prepro/`: data.dat + 7 nano_*.dat files + meshini.msh + prepro.log
- `test/cases/graphene_compression_simulator/`: energy.dat (53 lines = header + 2 init + 50 steps), force.dat, nano_final_config.dat, output.dat, simulator.log

**Plan correction**: AC-7 and task4c originally referenced nCodeLoad=1 with nloadstep=100. Oracle archaeology revealed nCodeLoad=3 with nloadstep=50. Both were corrected in plan.md and goal-tracker.

### task0b — Fortran cyclic crumple case

The cyclic crumple oracle outputs were confirmed and archived:

- Config: 40×40 mesh, 20 nm×20 nm, nCodeLoad=31 (biaxial corner-loaded cyclic), 5 cycles, nloadstep_comp=20, nloadstep_rel=20, ncrease=1
- `test/cases/graphene_cyclic_crumple/prepro_run/`: data.dat + 8 nano_*.dat files (including nano_crease.dat) + prepro.log
- `test/cases/graphene_cyclic_crumple/simulator_run/`: energy.dat, force.dat, crease_map.dat, nano_final_config.dat, output.dat, nano_checkpoint.dat, simulator.log

### task0c — document/fortran_conventions.md

Written directly (Codex route failed due to zsh shell-escaping of Fortran D-exponent notation in ask-codex.sh). Covers:

1. 1-based Fortran → 0-based C++ for all indices
2. Unit system: nm / eV / eV·nm / 1/nm; Jacobians in nm²
3. D-exponent float parsing: replace `D`/`d` with `E`/`e` before strtod
4. All nano_*.dat formats with exact field ordering (nano_dims, nano_general, nano_zero, nano_config, nano_BCs, nano_Mesh, nano_tub_loc, nano_crease)
5. BC sign conventions (nCodeLoad=3: right edge compressed in −x)
6. MPI conventions (rank-0 I/O only, MPI_ALLREDUCE for global assembly)
7. nCodeLoad reference table (0,1,2,3,10,11,13,30,31,222,1000)
8. Active source file lists for prepro (11 files) and simulator (31 files); backup variants excluded
9. Key C++ implementation notes: D-exponent parsing, COMMON blocks → class members, ghost node encoding

## Files Changed

| File | Action |
|------|--------|
| `test/cases/graphene_compression_prepro/` | All oracle files archived (data.dat, 7 nano_*.dat, meshini.msh, prepro.log) |
| `test/cases/graphene_compression_simulator/` | All oracle files archived (energy.dat, force.dat, nano_final_config.dat, output.dat, simulator.log) |
| `test/cases/graphene_cyclic_crumple/prepro_run/` | All oracle files archived (data.dat, 8 nano_*.dat, prepro.log) |
| `test/cases/graphene_cyclic_crumple/simulator_run/` | All oracle files archived (energy.dat, force.dat, crease_map.dat, nano_final_config.dat, output.dat, nano_checkpoint.dat, simulator.log) |
| `test/cases/README.md` | Written: documents oracle commit, compiler, case inventory, nCodeLoad reference |
| `document/fortran_conventions.md` | Written: 292-line conventions reference for C++ translators |
| `document/plan.md` | Updated: AC-7 nCodeLoad=1→3, nloadstep=100→50; task4c description corrected |
| `.humanize/rlcr/2026-03-29_21-53-11/goal-tracker.md` | Updated: tasks 0a/0b/0c moved to Completed; plan evolution log updated |
| `.humanize/bitlesson.md` | Updated: 2 lessons added |

## Validation

| Check | Result |
|-------|--------|
| graphene_compression_prepro/ has all 7 nano_*.dat | PASS |
| graphene_compression_simulator/energy.dat has 53 lines (50 load steps) | PASS |
| graphene_cyclic_crumple/simulator_run/ has nano_checkpoint.dat and crease_map.dat | PASS |
| document/fortran_conventions.md covers all 9 required topics per round-0-contract | PASS |
| plan.md AC-7 now references nCodeLoad=3, nloadstep=50 | PASS |
| goal-tracker tasks 0a/0b/0c marked completed | PASS |

## Remaining Items

None for Round 0. Round 1 targets Milestone 1: CMake scaffold (task1a), C++ data types (task1b), nano_*.dat readers/writers (task1c), MPI wrapper (task1d), oracle comparison script (task1e).

## BitLesson Delta

Action: add
Lesson ID(s): BL-20260329-codex-shell-escape, BL-20260329-nCodeLoad-mismatch
Notes: Added two lessons from Round 0 failures: (1) ask-codex.sh fails in zsh with Fortran D-exponent and percent characters — use Write tool directly for complex analyze-tagged docs; (2) always verify loading parameters from actual oracle data.dat/nano_BCs.dat before writing ACs — plan had wrong nCodeLoad=1 and nloadstep=100 before archaeology confirmed nCodeLoad=3 and nloadstep=50.
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

Read @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-29_21-53-11/goal-tracker.md and verify:

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
2. **If correction is needed**: Update @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-29_21-53-11/goal-tracker.md yourself with the requested changes:
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
- If after your investigation the actual situation does not match what Claude claims to have completed, or there is pending work to be done, output your review comments to @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-29_21-53-11/round-0-review-result.md.
- **CRITICAL**: Only output "COMPLETE" as the last line if ALL tasks from the original plan are FULLY completed with no deferrals
  - DEFERRED items are considered INCOMPLETE - do NOT output COMPLETE if any task is deferred
  - UNFINISHED items are considered INCOMPLETE - do NOT output COMPLETE if any task is pending
  - The ONLY condition for COMPLETE is: all original plan tasks are done, all ACs are met, no deferrals or pending work allowed
- The word COMPLETE on the last line will stop Claude.
