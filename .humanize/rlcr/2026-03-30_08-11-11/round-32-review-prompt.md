# Code Review - Round 32

## Original Implementation Plan

**IMPORTANT**: The original plan that Claude is implementing is located at:
@document/plan.md

You MUST read this plan file first to understand the full scope of work before conducting your review.
This plan contains the complete requirements and implementation details that Claude should be following.

Based on the original plan and @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-32-prompt.md, Claude claims to have completed the work. Please conduct a thorough critical review to verify this.

---
Below is Claude's summary of the work completed:
<!-- CLAUDE's WORK SUMMARY START -->
# Round 32 Summary

## Work Completed
- Re-read the plan, goal tracker, Round 31 summary/review, and wrote `round-32-contract.md` with the AC-7 executable-path objective.
- Reworked the runtime solver state so live `eta` travels with coordinates instead of being reread from `input.initial_config.eta` on every assembly:
  - added `fce::RuntimeState { coords, eta }`
  - added `make_runtime_state(input)`
  - threaded stateful overloads through `assemble_energy_forces()`, `minimize_free()`, `minimize_constrained()`, and `pasapas()`
  - propagated the converged per-element `eta` returned by `compute_element_energy()` back into the live runtime state and into `nano_final_config.dat`
- Fixed the executable contract in `src/simulator/main.cpp`:
  - positional second argument is now optional `nloadstep`
  - archived-VTU single-step assembly stays behind explicit `--single-step <step>`
  - `crunch_it <case_dir> 50` now runs the real pasapas solver path
- Extended runtime artifact generation in `pasapas()`:
  - truncates/regenerates fresh `energy.dat` and `force.dat`
  - writes `output.dat`
  - writes `nano_final_config.dat` at the end of the solve
  - supports `iload_stop` so executable-path tests can request an exact step count
- Replaced the Round 31 self-oracle AC-7 test with an archived Fortran oracle regression:
  - creates a writable temporary copy of `test/cases/graphene_compression_simulator/np1`
  - deletes copied runtime outputs first
  - executes the real `crunch_it` binary via `CRUNCH_IT_BIN`
  - requires `energy.dat`, `force.dat`, `output.dat`, and `nano_final_config.dat`
  - compares energy, reaction-force, and final-config coordinate/eta outputs against the archived `np1` oracle
- Restored the L-BFGS stale-gradient fix in `src/core/lbfgs.cpp` after noticing an in-tree diff had regressed the old premature-exit behavior.
- Investigated the remaining solver mismatch against the local canonical Fortran source under `../finite_crystal_elasticity/grapheneCompressionOriginVersion/`:
  - confirmed `crunch_it <case_dir> 50` should go through full `pasapas`
  - confirmed canonical `write_config` writes eta into `nano_final_config.dat`
  - confirmed the archived `np1` case has `nW_hat = 0`, so it is not a direct live-eta oracle by itself
- Tested and then reverted a deterministic imperfection-port experiment after it made the archived-oracle mismatch materially worse.

## Files Changed
- `CMakeLists.txt`
- `include/fce/simulator.hpp`
- `include/fce/solver.hpp`
- `src/core/lbfgs.cpp`
- `src/core/simulator.cpp`
- `src/core/solver.cpp`
- `src/simulator/main.cpp`
- `test/integration/test_e2e_compression.cpp`
- `.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md`
- `.humanize/rlcr/2026-03-30_08-11-11/round-32-summary.md`

## Validation
- `cmake --build build --target unit_tests integration_tests crunch_it -j4`
  - PASS
- `./build/unit_tests --gtest_filter='Lbfgs*'`
  - PASS (`4/4`)
- `./build/integration_tests --gtest_filter='E2ECompression.CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts'`
  - FAIL after `1169160 ms` (~19.5 min), but now fails for the correct reason on the real executable path:
    - runtime artifacts are generated
    - archived-oracle comparison runs end-to-end
    - energy mismatch starts immediately after load step 1
    - example mismatches from the test output:
      - step 1 energy: actual `5.79788888e-05` vs oracle `5.72105277e-05`
      - step 2 energy: actual `5.91932754e-05` vs oracle `1.03739788e-04`
      - widespread final-config coordinate errors exceed the `1e-3` relative tolerance
- Manual short-run probe:
  - `./build/crunch_it <temp_copy>/np1 3`
  - confirmed fresh runtime files are created and step-0 energy is again near zero (`7.68087041e-25`)

## Remaining Items
- `task4d` remains open: the solver now carries live `eta`, but the load-step trajectory still diverges from the archived Fortran runtime after the first increment.
- `task4e` remains open: torque parity from `get_reac.f90` is still untranslated, and the reaction outputs are not yet oracle-clean because the larger runtime mismatch remains.
- `task4f` remains open: the executable-path regression is now the correct archived-oracle test, but it is still red.
- Milestone 5 onward (`task5a`-`task8d`) remains out of scope for this round and still pending.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable project lesson was added this round. The main result was converting the AC-7 harness from a false-green self-oracle into an honest archived-oracle executable regression.
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
- If after your investigation the actual situation does not match what Claude claims to have completed, or there is pending work to be done, output your review comments to @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-32-review-result.md.
- **CRITICAL**: Only output "COMPLETE" as the last line if ALL tasks from the original plan are FULLY completed with no deferrals
  - DEFERRED items are considered INCOMPLETE - do NOT output COMPLETE if any task is deferred
  - UNFINISHED items are considered INCOMPLETE - do NOT output COMPLETE if any task is pending
  - The ONLY condition for COMPLETE is: all original plan tasks are done, all ACs are met, no deferrals or pending work allowed
- The word COMPLETE on the last line will stop Claude.
