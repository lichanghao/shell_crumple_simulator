# Code Review - Round 33

## Original Implementation Plan

**IMPORTANT**: The original plan that Claude is implementing is located at:
@document/plan.md

You MUST read this plan file first to understand the full scope of work before conducting your review.
This plan contains the complete requirements and implementation details that Claude should be following.

Based on the original plan and @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-33-prompt.md, Claude claims to have completed the work. Please conduct a thorough critical review to verify this.

---
Below is Claude's summary of the work completed:
<!-- CLAUDE's WORK SUMMARY START -->
# Round 33 Summary

## Work Completed
- Re-read the plan, goal tracker, Round 32 summary, Round 32 review, and wrote `round-33-contract.md` to keep the mainline on AC-7 runtime semantics.
- Fixed the shortened executable-path semantics in `src/simulator/main.cpp`:
  - the positional second argument is now treated as a stop-step limit, not as a replacement `BCs%nloadstep`
  - `crunch_it <case_dir> 3` now executes the first three steps of the canonical 50-step path and writes load parameters `0.02`, `0.04`, `0.06`
  - values larger than the file-loaded `BCs%nloadstep` are rejected
- Ported the missing imperfection step into `pasapas()` in the correct place:
  - the perturbation is applied after `load_doit(...)` and before `minimize_constrained(...)`
  - the constrained-DOF snapshot is no longer refreshed after imperfection injection, so BC positions remain the load-controlled values while the free-state guess is perturbed
  - used a deterministic scalar-per-step sequence so the archived-oracle regression remains reproducible
- Added direct regression coverage for live runtime `eta` state:
  - `SimulatorAssembly.StatefulAssemblyUsesRuntimeEtaInsteadOfInitialConfig` uses the committed `graphene_bilayer_twist_vdw_1000/prepro_run` case (`nW_hat=1`)
  - forces the inner solver to accept the provided eta immediately via a huge `crit_local`
  - proves the stateful assembly path reads and preserves `RuntimeState::eta` instead of silently reverting to `input.initial_config.eta`
- Closed the `nCodeLoad=3` reaction-side mapping bug in `LoadController::compute_reaction()`:
  - added `test/unit/test_load_controller.cpp`
  - `LoadController.ComputeReactionMatchesGetReacNCodeLoad3Semantics` exposed that the side tags were interpreted as if they were still 1-based
  - fixed the mapping so reaction side 1 corresponds to stored tag `0` and side 2 to stored tag `1`
- Added explicit unit coverage for the load-step denominator:
  - `LoadController.ApplyIncrementUsesStoredNloadstepDenominator` verifies `apply_increment()` still uses the file-loaded `BCs%nloadstep` denominator
- Inspected canonical `get_reac.f90` and confirmed the AC-7 `nCodeLoad=3` path has no torque term. Torque handling is required for other loading modes, but not for this archived compression oracle.

## Files Changed
- `CMakeLists.txt`
- `src/core/load_controller.cpp`
- `src/core/solver.cpp`
- `src/simulator/main.cpp`
- `test/unit/test_simulator.cpp`
- `test/unit/test_load_controller.cpp`
- `.humanize/bitlesson.md`
- `.humanize/rlcr/2026-03-30_08-11-11/round-33-contract.md`
- `.humanize/rlcr/2026-03-30_08-11-11/round-33-summary.md`

## Validation
- `cmake --build build --target unit_tests crunch_it integration_tests -j4`
  - PASS
- `./build/unit_tests --gtest_filter='LoadController.*:SimulatorAssembly.StatefulAssemblyUsesRuntimeEtaInsteadOfInitialConfig'`
  - PASS (`3/3`)
- `./build/unit_tests --gtest_filter='SimulatorAssembly.*:Lbfgs*'`
  - PASS (`9/9`) before the reaction-side unit-test addition
- Manual executable-path stop-step probe:
  - `./build/crunch_it <temp_copy>/np1 3`
  - PASS
  - load parameters written to `energy.dat` are `0.02`, `0.04`, `0.06`, confirming the denominator fix
  - early energies now move from the Round 32 values toward the archived oracle:
    - Round 32 step 1: `5.79788888e-05`
    - Round 33 step 1: `5.03028429e-05`
    - Oracle step 1: `5.72105277e-05`
    - Round 32 step 2: `5.91932754e-05`
    - Round 33 step 2: `8.50012013e-05`
    - Oracle step 2: `1.03739788e-04`
- `./build/integration_tests --gtest_filter='E2ECompression.CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts'`
  - FAIL after `754292 ms` (~12.6 min), but the correct archived-oracle executable-path regression still runs end-to-end
  - key current failures:
    - energy remains outside tolerance across the trajectory, though early steps are materially closer than in Round 32
    - reaction-force rows still diverge from the archived oracle
    - final configuration still diverges substantially in many coordinates
  - note: this full 50-step run was captured immediately before the final `LoadController::compute_reaction()` side-tag fix; the post-fix evidence is the passing unit test plus the post-fix 3-step executable probe

## Remaining Items
- AC-7 remains open: runtime solver semantics are still not Fortran-equivalent even after restoring the stop-step contract, imperfection placement, live-`eta` coverage, and the reaction-side fix.
- The deterministic scalar imperfection sequence is a reproducible surrogate for the missing runtime path, but it is not yet proven to be the exact archived-oracle source. The checked-in graphene `pasapas.f90` and the project notes still disagree on the imperfection implementation.
- VTU output (`task5a`-`task5b`), runtime vdW/self-contact (`task6a`-`task6c`), cyclic/crease/checkpoint work (`task7a`-`task7e`), MPI consistency (`task8a`-`task8b`), and documentation (`task8c`-`task8d`) are still pending original-plan work.

## Goal Tracker Update Request

### Requested Changes:
- Add a Round 33 plan-evolution entry noting that:
  - shortened executable probes now preserve the file-loaded `BCs%nloadstep` denominator and use only `iload_stop` to terminate early
  - the runtime path now includes a source-backed imperfection step in the canonical position
  - the new direct `nW_hat=1` test proves `RuntimeState::eta` is consumed by stateful assembly
  - `task4e` for AC-7 should be narrowed to the `nCodeLoad=3` force-summing path because `get_reac.f90` has no torque branch for that loading mode
- Update `task4d` notes to record that step ordering is closer to Fortran now, but AC-7 is still red on the archived executable-path oracle.
- Update `task4e` notes to record the reaction-side mapping bug fix plus the new unit coverage for `get_reac.f90` `nCodeLoad=3` semantics.
- Update `task4f` notes to record the Round 33 archived-oracle evidence:
  - full executable-path regression still fails
  - runtime improved from the Round 32 trajectory
  - final configuration and reaction outputs remain materially off
- Add the side-tag mapping bug as a resolved lesson-backed issue if the tracker keeps issue-level notes for AC-7 runtime mismatches.

### Justification:
- These changes keep the mutable tracker honest without overclaiming AC-7 closure.
- They distinguish resolved contract bugs (stop-step semantics, live eta usage, reaction side mapping) from the still-open solver trajectory mismatch.
- They also prevent `task4e` from staying artificially open for missing torque logic that does not exist on the archived `nCodeLoad=3` path.

## BitLesson Delta
- Action: add
- Lesson ID(s): BL-20260406-bc-side-tags-zero-based
- Notes: `BCData::mnodBC` side tags are already 0-based after I/O conversion. The `nCodeLoad=3` reaction code must treat stored tag `0` as Fortran side 1 and stored tag `1` as Fortran side 2.
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
- If after your investigation the actual situation does not match what Claude claims to have completed, or there is pending work to be done, output your review comments to @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-33-review-result.md.
- **CRITICAL**: Only output "COMPLETE" as the last line if ALL tasks from the original plan are FULLY completed with no deferrals
  - DEFERRED items are considered INCOMPLETE - do NOT output COMPLETE if any task is deferred
  - UNFINISHED items are considered INCOMPLETE - do NOT output COMPLETE if any task is pending
  - The ONLY condition for COMPLETE is: all original plan tasks are done, all ACs are met, no deferrals or pending work allowed
- The word COMPLETE on the last line will stop Claude.
