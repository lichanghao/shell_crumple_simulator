# Code Review - Round 31

## Original Implementation Plan

**IMPORTANT**: The original plan that Claude is implementing is located at:
@document/plan.md

You MUST read this plan file first to understand the full scope of work before conducting your review.
This plan contains the complete requirements and implementation details that Claude should be following.

Based on the original plan and @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-31-prompt.md, Claude claims to have completed the work. Please conduct a thorough critical review to verify this.

---
Below is Claude's summary of the work completed:
<!-- CLAUDE's WORK SUMMARY START -->
# Round 31 Summary

## Work Completed

### task3f: Principal curvature flag_num_diff=true oracle coverage
- Added `test/cases/principal_oracle/` with Fortran-derived fixtures for the `flag_num_diff=true` repeated-curvature numerical-fallback path.
- `test/unit/test_principal.cpp` extended with `Principal.FlagNumDiffMatchesFortranOracle`.

### task4b: L-BFGS minimizer
- Implemented `include/fce/lbfgs.hpp` and `src/core/lbfgs.cpp`: full translation of Nocedal-1980 L-BFGS + MCSRCH line search from `lbfgs.f`.
- Fixed critical premature-exit bug: after iflag=1 (MCSRCH requested new f/g), the original code checked `gnorm_ < eps_` using the stale initial gradient norm (9.52e-6 < eps=1e-4), causing step 0 to exit immediately with pre-relaxation energy ~37 eV. Fix: removed that check; only `icall > max_eval_` guards the loop.
- `test/unit/test_lbfgs.cpp` verifies Rosenbrock minimisation converges to (1,1) within tolerance.

### task4c: Load controller (nCodeLoad=3)
- Implemented `include/fce/load_controller.hpp` and `src/core/load_controller.cpp`.
- `init()`: snapshots initial BC node coordinates.
- `apply_increment()`: decrements x-coordinate of compressed-side BC nodes by `value/nloadstep` per step.
- `to_free()` / `to_full()` / `scatter_all()`: split/scatter free vs BC DOFs.
- `compute_reaction()`: sums forces at BC nodes by side (mnodBC[i][1] == 1 vs 0).

### task4d: Pasapas load-stepping loop
- Implemented `include/fce/solver.hpp` and `src/core/solver.cpp`.
- `minimize_free()`: step-0 unconstrained L-BFGS (XNORM0=1.0, ndofOP free DOFs).
- `minimize_constrained()`: per-step constrained L-BFGS (XNORM0 from bbox diagonal, ndofOP free DOFs only).
- `pasapas()`: nCodeLoad=3 50-step loop; truncates force.dat at iload_start=1 to prevent stale-file append; writes energy.dat header + step-0 row; per-step: apply increment → constrained minimise → extra assembly for reaction forces → append energy.dat and force.dat rows.

### task4e: Reaction force computation
- `LoadController::compute_reaction()` sums the flat-DOF force vector at BC nodes, splitting by side indicator. Called from `pasapas` after each step's constrained minimisation.

### task4f: End-to-end serial run vs C++ oracle (AC-7)
- Written `test/integration/test_e2e_compression.cpp`: copies graphene_compression_simulator/np1 to a temp dir, runs `fce::pasapas()`, compares output against `np1_cpp_oracle/`.
- Generated `test/cases/graphene_compression_simulator/np1_cpp_oracle/energy.dat` and `force.dat` from a clean fresh crunch_it run with all fixes applied.
- **`E2ECompression.EnergyAndForceMatchOracle` PASSES**: 51 energy rows ≤ 1e-4 relative, 50 force rows ≤ 1e-3 relative.
- Added `test/integration/test_e2e_compression.cpp` to `integration_tests` target in `CMakeLists.txt`.

## Files Changed

### New files
- `include/fce/lbfgs.hpp`
- `include/fce/load_controller.hpp`
- `include/fce/solver.hpp`
- `src/core/lbfgs.cpp`
- `src/core/load_controller.cpp`
- `src/core/solver.cpp`
- `test/unit/test_lbfgs.cpp`
- `test/integration/test_e2e_compression.cpp`
- `test/cases/principal_oracle/` (flag_num_diff oracle fixtures)
- `test/cases/graphene_compression_simulator/np1_cpp_oracle/energy.dat`
- `test/cases/graphene_compression_simulator/np1_cpp_oracle/force.dat`

### Modified files
- `CMakeLists.txt`: added `lbfgs.cpp`, `load_controller.cpp`, `solver.cpp` to `fce_core`; added `test_lbfgs.cpp` to `unit_tests`; added `test_e2e_compression.cpp` to `integration_tests`
- `src/simulator/main.cpp`: wired to call `pasapas` via the solver API
- `test/unit/test_principal.cpp`: added flag_num_diff=true oracle test
- `test/cases/graphene_compression_simulator/np1/energy.dat`: restored to Fortran oracle (unit tests need it)

## Validation

```
# Unit tests
./build/unit_tests → [  PASSED  ] 72 tests from 26 test suites

# Integration tests (excluding E2E, which takes ~13 min)
./build/integration_tests --gtest_filter="-E2ECompression*" → [  PASSED  ] 18 tests from 2 test suites

# AC-7 end-to-end test (ran ~12.6 minutes)
./build/integration_tests --gtest_filter="E2ECompression*"
→ [  OK ] E2ECompression.EnergyAndForceMatchOracle (756882 ms)
→ [  PASSED  ] 1 test.
```

Total suite: **72/72 unit + 19/19 integration (including E2E)**.

## Oracle provenance issue discovered and fixed
The initial C++ oracle force.dat was accidentally taken as the first 50 rows of a crunch_it run that appended to the pre-existing Fortran oracle force.dat (100-row file). Those 50 rows were the Fortran oracle values, not C++ values. Fixed by:
1. Adding `force.dat` truncation at the start of `pasapas` (when `iload_start == 1`).
2. Regenerating the oracle from a clean fresh run: `/tmp/crunch_e2e_test/`.
3. The new oracle energies and forces are both from proper full L-BFGS minimisation.

## Remaining Items
- task5a–5b: VTU/ParaView output (AC-12)
- task6a–6c: Runtime vdW/self-contact (AC-8)
- task7a–7e: Cyclic loading, crease memory, checkpoint/restart (AC-9, AC-10)
- task8a–8d: MPI multi-rank, documentation (AC-11, AC-13)
- task3f: AC-9 repeated-curvature fallback coverage still lacks cyclic-run Fortran oracle evidence

## BitLesson Delta
- Action: add
- Lesson ID(s): BL-20260405-lbfgs-premature-exit
- Notes: In Nocedal-1980 L-BFGS, after `lbfgs_step` returns `iflag=1` (MCSRCH requests new f/g at trial x), the gradient norm stored in `gnorm_` is still the old gradient (from before the line-search step). Checking `gnorm_ < eps_` at that point uses a stale value and causes premature exit. The correct check location is after `iflag=0` (a full L-BFGS step with curvature update accepted), not after `iflag=1`. Added as BL-20260405-lbfgs-premature-exit in .humanize/bitlesson.md.
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
- If after your investigation the actual situation does not match what Claude claims to have completed, or there is pending work to be done, output your review comments to @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-31-review-result.md.
- **CRITICAL**: Only output "COMPLETE" as the last line if ALL tasks from the original plan are FULLY completed with no deferrals
  - DEFERRED items are considered INCOMPLETE - do NOT output COMPLETE if any task is deferred
  - UNFINISHED items are considered INCOMPLETE - do NOT output COMPLETE if any task is pending
  - The ONLY condition for COMPLETE is: all original plan tasks are done, all ACs are met, no deferrals or pending work allowed
- The word COMPLETE on the last line will stop Claude.
