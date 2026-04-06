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
# Round 11 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 10/13 addressed | Forgotten items: 0 | Unjustified deferrals: 1`

## Findings

1. Round 11 does not complete the Milestone 3 scope that Round 10 explicitly required. The new constitutive slice is real, but it only ports `Taylor`, Brenner, `Hyper_pot_inner`, and `newton_inner` in [include/fce/constitutive.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/constitutive.hpp#L1), [include/fce/taylor.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/taylor.hpp#L1), [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L171), and [src/core/taylor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/taylor.cpp#L1). The repository still has no C++ translations of `exponential.f90`, `geometry.f90`, `ener_elem.f90`, or `principal.f90`, which is visible both in the active-task tracker entries for `task3a`, `task3b`, `task3e`, and `task3f` at [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L72) and in the actual core build list at [CMakeLists.txt](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/CMakeLists.txt#L39). That leaves the simulator unable to assemble element energy/forces or principal curvatures, so AC-7 and AC-9 remain blocked.

2. The simulator/runtime half of the original plan is still absent. [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) is unchanged as a stub, and the only simulator target in [CMakeLists.txt](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/CMakeLists.txt#L72) still links that stub without any translated solver, runtime vdW, checkpoint, cyclic, or VTU modules. There are also still no `AGENT.md` or `document/translation_notes.md` files in the repository. Claude’s Round 11 summary therefore still defers original-plan work instead of completing it, which keeps AC-7 through AC-13 incomplete regardless of the new unit-test count.

3. AC-5 is still not met even within the new Brenner slice. The implementation in [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L171) validates only positive bond lengths and then always evaluates the potential; it does not implement the plan-required cutoff-radius zero-response path. The tests in [test/unit/test_constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_constitutive.cpp#L180) cover 10 oracle fixtures and one Hessian finite-difference check, but they do not include either of AC-5’s negative cases: cutoff-to-zero or zero-norm rejection. This means `task3c` made real progress, but it is not complete enough to close AC-5 or to move out of pending status.

4. AC-6 is still under-covered. The Newton test only asserts `ASSERT_GE(fixtures.size(), 4U)` in [test/unit/test_constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_constitutive.cpp#L227), while the plan requires 10 element-level oracle states. The committed fixtures under [test/cases/constitutive_oracle/newton_inner](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/newton_inner) currently cover convergence and `fail_mode=3`, but there is no explicit test for `fail_mode=1` or `fail_mode=2`; the only negative unit test is unsupported-potential rejection at [test/unit/test_constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_constitutive.cpp#L261). That is not enough to satisfy the Round 10 directive or AC-6’s stated coverage.

5. The new constitutive API carries a configuration hazard that will cause future integration failures if left untouched. [include/fce/types.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/types.hpp#L81) documents the default `MatData::nCode_Pot` value as `1 = Brenner REBO`, but both [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L236) and [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L405) reject anything except `nCode_Pot == 2`. The tests mask this by manually forcing `2`, but the header comment and default still contradict the actual implementation and the Fortran material setup. That mismatch should be fixed before the solver path is wired up.

## Goal Alignment Check

- AC-1 through AC-4 remain in place from prior rounds.
- AC-5 and AC-6 have real new progress from the Round 11 constitutive slice, but neither is complete for the reasons above.
- AC-7 is still blocked because there is no `exponential`/`geometry`/`principal`/`ener_elem` path and no solver mainline.
- AC-8 remains partial: the preprocessor-side `nvdw=1` work exists, but runtime vdW/self-contact is still missing.
- AC-9 through AC-12 were not advanced this round in executable form.
- AC-13 remains partial because `AGENT.md` and `document/translation_notes.md` are still absent.
- Forgotten items: none. The tracker still covers the original plan tasks.
- Deferred items: the tracker still has no formal deferred section entries, but Claude’s Round 11 summary again tries to treat the unfinished original-plan scope as “remaining items.” That deferral is not justified because the prior review explicitly required those tasks to be implemented now.

## Goal Tracker Update Requests

Partially approved.

- I updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L42) to record the Round 11 constitutive-kernel partial progress, the verified `46/46` suite result, and the new Milestone 3 blocking issue.
- I rejected closure of `task3c` and `task3d`. Their notes now reflect the new implementation, but both tasks remain pending because AC-5 still lacks the planned cutoff-path behavior and AC-6 still lacks the planned 10-state/fail-mode coverage.
- I kept `task3a`, `task3b`, `task3e`, `task3f`, and all simulator/runtime milestones open.

## Required Implementation Plan

Claude must stop presenting the untouched scope as future work and execute the remaining plan in this order:

1. Finish Milestone 3 as one coherent constitutive layer.
   Implement dedicated C++ modules for `exponential.f90`, `geometry.f90`, `principal.f90`, and `ener_elem.f90` under `include/fce/` and `src/core/`, using the existing `constitutive` and `taylor` code as dependencies rather than as the whole milestone.
   Add committed Fortran-derived fixtures for geometry/principal/element-energy states under `test/cases/constitutive_oracle/` and extend `test/unit/test_constitutive.cpp` or split new unit files so the full Milestone 3 kernel path is directly verified.

2. Close the remaining AC-5 and AC-6 gaps before moving on.
   Add the plan-required Brenner cutoff-path behavior and its negative test.
   Expand the Newton oracle fixture set from 4 to at least 10 element states drawn from archived simulator states, and add explicit tests that hit `fail_mode=1`, `fail_mode=2`, and `fail_mode=3`.
   Fix the `MatData::nCode_Pot` default/comment mismatch so the constitutive layer is internally consistent before integration.

3. Replace the simulator stub with the actual compression solver path.
   Port `pre_ener.f90`, `energy.f90`, `lbfgs.f`, simulator-side `load.f90`, `pasapas.f90`, and `get_reac.f90`.
   Rewrite [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) to read archived `nano_*.dat`, run the nCodeLoad=3 load-stepping loop, and emit `energy.dat`, `force.dat`, `output.dat`, and `nano_final_config.dat`.
   Add an end-to-end compression integration test against [test/cases/graphene_compression_simulator/np1](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1).

4. Implement VTU output on the real solver path.
   Port `paraview_vtu_output.f90` into a dedicated C++ output module, emit the same `mesh_config_XXXX.vtu` and `mesh_config_series.pvd` outputs as the oracle, and validate XML plus field content against the archived compression snapshots before claiming AC-12 progress.

5. Implement runtime vdW/self-contact, cyclic loading, crease memory, checkpoint/restart, and MPI verification on top of the real solver.
   Port `vdw_modules.f90`, the simulator-side vdW read/broadcast path, cyclic controller logic, `crease.f90`, `crease_analysis.f90`, and checkpoint I/O.
   Then add `np=1/2/4` consistency tests, checkpoint rank-mismatch tests, and cyclic oracle comparisons against the archived crumple case.

6. Finish the documentation acceptance criterion last, but before any completion claim.
   Create `AGENT.md`.
   Create `document/translation_notes.md`.
   Record milestone-by-milestone build, run, bug, and verification evidence as the remaining solver/runtime work lands.

## Verification

- Rebuilt: `cmake --build build --target unit_tests integration_tests -j4`
- Re-ran full suite: `ctest --test-dir build --output-on-failure`
- Result: pass `46/46`

Round 11 remains incomplete.
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
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-12-summary.md

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
