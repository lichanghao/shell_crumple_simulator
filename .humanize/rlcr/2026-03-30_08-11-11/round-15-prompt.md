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
# Round 14 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 4/13 met | Forgotten items: 0 task IDs omitted, 1 stale tracker classification corrected | Unjustified deferrals: 0 explicit`

## Findings

1. The new principal-curvature port changes Fortran failure semantics on invalid discriminants. In Fortran, `beta = sqrt(xmean*xmean - gauss)` is followed by an explicit NaN stop in [principal.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/principal.f90#L22), but the C++ port silently clamps negative discriminants to zero in [principal.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/principal.cpp#L39). That can turn an invalid state into the repeated-curvature fallback instead of surfacing the error, which is a real fidelity risk for later solver integration.

2. Round 14’s derivative-validation claim is broader than the tests currently support. [test_principal.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_principal.cpp#L44) checks only `dcurvppaldC` and `dcurvppaldk`; it never exercises `dvppaldC` or `dvppaldk`. [test_exponential.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_exponential.cpp#L50) checks only `dpedC` with all principal-derivative inputs forced to zero, so the coupled `dcurvppal*`, `dvppal*`, and `dpedk` paths in [exponential.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/exponential.cpp#L101) remain effectively unverified. There are also still no committed Fortran-derived fixtures or provenance for these new modules.

3. The new modules are still sidecar kernels, not live simulator behavior. The actual simulator remains a stub in [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1), and the inner constitutive path still carries its own inlined exponential logic in [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L312). So Round 14 is real Milestone 3 progress, but it does not yet move AC-7 or AC-9 beyond unit-tested kernel groundwork.

## Part 1: Goal Tracker Audit

### 1.1 Acceptance Criteria Status

| AC | Status | Evidence (if MET) | Blocker (if NOT MET) | Justification (if DEFERRED) |
|----|--------|-------------------|---------------------|----------------------------|
| AC-1 | MET | Archived compression and cyclic Fortran outputs plus conventions doc in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L116) | - | - |
| AC-2 | MET | Compression, cyclic, self-contact, and bilayer-twist preprocessor oracle tests now recorded in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L126) | - | - |
| AC-3 | MET | Committed Fortran B-spline fixtures and direct oracle test in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L123) | - | - |
| AC-4 | MET | Archived ghost-coordinate artifacts and direct comparator evidence in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L124) | - | - |
| AC-5 | PARTIAL | - | Brenner kernel exists, but the geometry/bond-vector pipeline from `task3b` is still missing and the AC-5 plan coverage is not closed through the translated Milestone 3 path | - |
| AC-6 | PARTIAL | - | `task3d` still uses helper-defined synthetic Newton states instead of archived simulator load-step states in [constitutive_oracle/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md#L9) | - |
| AC-7 | NOT MET | - | `geometry.f90`, `ener_elem.f90`, solver assembly, L-BFGS, load stepping, reaction forces, and the end-to-end simulator oracle path are still absent; simulator is still a stub in [main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) | - |
| AC-8 | PARTIAL | - | Preprocessor-side vdW is complete, but runtime vdW/self-contact kernel and integration work under `task6a`-`task6c` are still absent in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L105) | - |
| AC-9 | NOT MET | - | No cyclic controller, crease-memory update path, crease analysis, or simulator-side principal-curvature integration exists yet | - |
| AC-10 | NOT MET | - | No checkpoint/restart implementation or oracle restart test exists | - |
| AC-11 | PARTIAL | - | MPI wrapper/partition utilities exist, but there is no multi-rank solver-equivalence verification because the solver path is still missing | - |
| AC-12 | NOT MET | - | No VTU writer or validation exists | - |
| AC-13 | PARTIAL | - | [AGENT.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/AGENT.md) and [translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md) are still missing | - |

### 1.2 Forgotten Items Detection

- All original plan task IDs from [document/plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L260) are represented somewhere in the tracker; no task IDs are missing from Active, Completed, or Deferred coverage.
- One stale classification existed: `task2g` was still marked pending even though its plan-defined Milestone 2 preprocessing scope had already been completed in Rounds 9-10. I corrected that tracker drift this round so runtime vdW stays under `task6a`-`task6c`.
- No Round 14 summary item was marked complete without verification. The new sources, focused tests, and full-suite `57/57` result all matched the repository state.

### 1.3 Deferred Items Audit

- The `Explicitly Deferred` section is still empty. No additional deferrals are justified.

### 1.4 Goal Completion Summary

Acceptance Criteria: 4/13 met (0 deferred)  
Active Tasks: 26 remaining  
Estimated remaining rounds: 10+ at the current milestone-by-milestone pace  
Critical blockers: `task3b`, `task3d` provenance, `task3e`, solver core (`task4a`-`task4f`), runtime vdW (`task6a`-`task6c`), cyclic/checkpoint (`task7a`-`task7e`), docs (`task8c`)

## Part 2: Implementation Review

- Claude’s Round 14 summary is directionally accurate: the dedicated `principal` and `exponential` modules, focused unit files, build wiring, and `57/57` full-suite claim all match the repository.
- The strongest remaining technical risk inside the new code is the `principal.cpp` discriminant clamp noted above; that should be fixed before these kernels are treated as authoritative Fortran-faithful ports.
- The biggest verification gap is still derivative coverage. Before claiming these kernels are ready for integration, add direct checks for `dvppaldC`, `dvppaldk`, `dpedk`, and the fully coupled derivative path with nonzero principal-derivative inputs, ideally backed by committed Fortran-derived fixtures and provenance.
- The biggest scope gap remains unchanged: `geometry` and `element_energy` are still missing, the Newton corpus is still synthetic, and the simulator path is still nonfunctional.

## Part 3: Goal Tracker Update Requests

Approved and applied.

- Updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L42) to Plan Version 16 with a Round 14 evolution entry documenting the verified dedicated kernel work and full-suite `57/57` result.
- Updated `task3a` and `task3f` notes in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L75) to reflect the new dedicated `exponential` and `principal` modules while keeping both tasks open.
- Updated `task3d` notes in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L78) to record the verified Round 14 full-suite result while keeping the archived-state provenance blocker open.
- Corrected the stale `task2g` classification in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L73): preprocessor-side vdW is now marked complete, and runtime vdW remains under `task6a`-`task6c`.
- Updated the Milestone 3 blocking issue in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L105) so it no longer incorrectly states that `exponential.f90` and `principal.f90` are untranslated.

## Required Implementation Plan

1. Fix the `principal.cpp` discriminant handling so materially negative `xmean^2 - gauss` still surfaces as an error instead of being silently coerced into the repeated-curvature branch. Add a regression that distinguishes tolerated roundoff from invalid input.
2. Finish Milestone 3 properly: add `geometry` and `element_energy`, then wire `principal` and `exponential` into the real constitutive/solver path instead of keeping parallel implementations.
3. Replace the synthetic AC-6 Newton corpus with archived simulator-state fixtures and update [constitutive_oracle/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md#L1) accordingly.
4. Expand Round 14 coverage with direct checks for `dvppaldC`, `dvppaldk`, `dpedk`, and the nonzero coupled derivative terms in the exponential path.
5. After Milestone 3 is actually integrated, port the solver core and replace the stub simulator before claiming AC-7 progress.

## Part 4: Progress Stagnation Check

No circuit-breaker stop is warranted yet. Recent rounds are narrow and repetitive, but Rounds 11-14 still show real forward motion: constitutive translation, negative-path expansion, material-code reconciliation, and now dedicated principal/exponential kernel extraction.

## Verification

- Rebuilt: `cmake --build build --target unit_tests integration_tests -j4`
- Re-ran focused Round 14 coverage: `ctest --test-dir build --output-on-failure -R 'Principal|Exponential'`
- Re-ran full suite: `ctest --test-dir build --output-on-failure`
- Result: pass `57/57`

Round 14 remains incomplete.
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

### Post-Alignment Check Action Items

This round follows a Full Goal Alignment Check. Pay special attention to:
- **Forgotten Items**: Codex may have identified tasks that were being ignored. Address them.
- **AC Status**: If any Acceptance Criteria were marked NOT MET, prioritize work toward those.
- **Deferred Items**: If any deferrals were flagged as unjustified, un-defer them now.

---

Note: You MUST NOT try to exit by lying, editing loop state files, or executing `cancel-rlcr-loop`.

After completing the work, please:
0. If the `code-simplifier` plugin is installed, use it to review and optimize your code. Invoke via: `/code-simplifier`, `@agent-code-simplifier`, or `@code-simplifier:code-simplifier (agent)`
1. Commit your changes with a descriptive commit message
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-15-summary.md

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
