Your work is not finished. Read and execute the below with ultrathink.

## Original Implementation Plan

**IMPORTANT**: Before proceeding, review the original plan you are implementing:
@document/plan.md

This plan contains the full scope of work and requirements. Ensure your work aligns with this plan.

---

## Round Re-anchor (REQUIRED FIRST STEP)

Before writing code:
- Re-read @document/plan.md
- Re-read @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md
- Re-read the most recent round summaries/reviews that led to this round
- Write the current round contract to @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-25-contract.md

Your round contract must contain:
- Exactly one **mainline objective**
- The 1-2 target ACs for this round
- Which issues are truly **blocking** that mainline objective
- Which issues are **queued** and explicitly out of scope
- Concrete success criteria for this round

Do not start implementation until the round contract exists.

## Task Lane Rules

Use the Task system (TaskCreate, TaskUpdate, TaskList) with one required tag per task:
- `[mainline]` for plan-derived work that directly advances this round's objective
- `[blocking]` for issues that prevent the mainline objective from succeeding safely
- `[queued]` for non-blocking bugs, cleanup, or follow-up work

Rules:
- `[mainline]` work is the round's primary success condition
- `[blocking]` work is allowed only when it truly blocks the mainline objective
- `[queued]` work must be documented but must NOT replace the round objective
- If a new bug does not block the current objective, tag it `[queued]` and keep moving on mainline work

Before executing each task in this round:
1. Read @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/bitlesson.md
2. Run `bitlesson-selector` for each task/sub-task
3. Follow selected lesson IDs (or `NONE`) during implementation

---
Below is Codex's review result:
<!-- CODEX's REVIEW RESULT START -->
# Round 24 Review

Mainline Progress Verdict: ADVANCED

Goal Alignment Summary:
`ACs: 5/13 met | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. `task3e` is still overclaimed as complete because the C++ `flag_num_diff` bending-stress branch does not match the checked-in canonical Fortran. In C++, `S_m` perturbs `curv0_elem` at [src/core/element_energy.cpp:96](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/element_energy.cpp#L96). In the checked-in canonical source, the `S_m` loop still perturbs `C_elem_` at [ener_elem.f90:76](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/ener_elem.f90#L76) and [ener_elem.f90:79](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/ener_elem.f90#L79). The Round 24 summary’s rebuttal was based on a misread of the Fortran lines, so the tracker entry claiming this blocker is closed was not valid. I corrected [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md) to reopen `task3e`.

2. The new Round 24 tests improve internal coverage, but they still do not supply the direct Fortran-backed oracle evidence needed to close the element energy/force kernel. [test_element_energy.cpp:362](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_energy.cpp#L362) proves `f_elem` is self-consistent with the C++ energy by centered finite difference, and [test_element_energy.cpp:411](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_energy.cpp#L411) proves the `flag_num_diff` path executes without NaN/Inf. Neither test checks canonical `S_n`, `S_m`, or `f_elem` against Fortran-emitted fixtures, which is still the missing verification gap for Milestone 3 in [document/plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md).

## Goal Alignment Check

### 1.1 Acceptance Criteria Status

| AC | Status | Evidence (if MET) | Blocker (if NOT MET) | Justification (if DEFERRED) |
|----|--------|-------------------|---------------------|-----------------------------|
| AC-1 | MET | Compression and cyclic oracle artifacts remain archived under [test/cases/graphene_compression_simulator](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator) and [test/cases/graphene_cyclic_crumple](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple), with conventions documented in [document/fortran_conventions.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/fortran_conventions.md). | - | - |
| AC-2 | MET | Preprocessor oracle parity and negative coverage remain in place through the archived comparison tests recorded in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md). `RoundTrip.Mesh` now also passes after the real `write_mesh()` fix. | - | - |
| AC-3 | MET | Committed Fortran-derived B-spline fixtures and partition-of-unity / derivative / out-of-domain coverage remain in [test/unit/test_bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_bspline.cpp). | - | - |
| AC-4 | MET | Archived ghost-coordinate artifacts and direct comparisons remain covered as recorded in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md). | - | - |
| AC-5 | PARTIAL | - | Standalone Brenner oracle and Hessian FD tests in [test/unit/test_constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_constitutive.cpp) are strong, but the tracker still lacks production-path evidence for Brenner through the translated element-energy pipeline; the archived simulator corpus remains Morse-only. | - |
| AC-6 | MET | [test/unit/test_constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_constitutive.cpp) checks 10 committed Newton fixtures, iteration counts, and fail modes; [test/unit/test_element_state.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_state.cpp#L501) validates 10 non-trivial archived simulator states from `nano_final_config.dat`. | - | - |
| AC-7 | NOT MET | - | The simulator assembly/solver path is still missing: [src/simulator/main.cpp:1](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) is still a stub, `task4a` through `task4f` remain pending in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md), and `task3e` still has the canonical-parity gap described above. | - |
| AC-8 | PARTIAL | - | Preprocessor-side vdW work is complete, but runtime vdW/self-contact remains unimplemented and is still tracked as a blocker in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md). | - |
| AC-9 | NOT MET | - | No cyclic controller, crease memory, or crease-analysis runtime path exists yet; `task7a` through `task7e` remain pending in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md). | - |
| AC-10 | NOT MET | - | No checkpoint/restart implementation or validation exists yet; `task7d` and `task7e` remain pending in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md). | - |
| AC-11 | PARTIAL | - | MPI wrapper/partition utilities exist, but there is still no translated solver path to compare `np=1/2/4`; `task4a`, `task8a`, and `task8b` remain pending in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md). | - |
| AC-12 | NOT MET | - | No VTU writer or validation path exists; `task5a` and `task5b` remain pending in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md). | - |
| AC-13 | PARTIAL | - | [AGENT.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/AGENT.md) and [document/translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md) are still missing. | - |

### 1.2 Forgotten Items Detection

No original-plan task is semantically lost from the tracker. The only representation wrinkle is that `task1c` is bundled into the combined completed entry `task1b+1c` rather than listed as its own exact task ID.

Tasks marked complete in Round 24 but not actually verified:

- `task3e` was marked complete in the mutable tracker and in Claude’s summary, but that closure is not supportable because the canonical `flag_num_diff` mismatch still exists and there is still no direct Fortran-backed stress/force oracle. I corrected the tracker accordingly.

### 1.3 Deferred Items Audit

There are still no explicit deferred items in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md). Nothing needs to be un-deferred because nothing has been formally deferred.

### 1.4 Goal Completion Summary

Acceptance Criteria: 5/13 met (0 deferred)  
Active Tasks: 24 remaining  
Estimated remaining rounds: 15-20 at the current pace  
Critical blockers: canonical `task3e` parity gap, missing Milestone 4 assembly/solver path, missing runtime vdW/self-contact, missing cyclic/crease/checkpoint work, missing VTU output, missing AC-13 docs

## Mainline Drift Audit

The current round’s objective was clear and singular: close the Round 23 blockers around `task3e`. The round did move the codebase forward narrowly by fixing the real `write_mesh()` regression and by adding useful self-consistency/smoke coverage around the element-energy kernel. But most of the work remained in Milestone 3 cleanup rather than moving into Milestone 4, and Claude overclaimed closure by asserting a Fortran parity point that the checked-in canonical source does not support.

Mainline Progress Verdict: ADVANCED
Blocking Side Issues: 1
Queued Side Issues: 1

Blocking side issue:

- Runtime vdW/self-contact is still unimplemented and continues to block AC-8.

Queued side issue:

- `AGENT.md` and `document/translation_notes.md` are still missing and should be revisited when the documentation milestone resumes.

## Implementation Review

Verified claims:

- The `write_mesh()` fix is real at [src/core/io.cpp:566](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/io.cpp#L566), and `./build/integration_tests --gtest_filter=RoundTrip.Mesh` now passes.
- The two new tests are present at [test/unit/test_element_energy.cpp:362](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_energy.cpp#L362) and [test/unit/test_element_energy.cpp:411](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_energy.cpp#L411), and `./build/unit_tests` passes with `58/58`.

Rejected claim:

- The summary’s statement that the canonical Fortran `S_m` branch perturbs `curv0_elem_` is not supported by the checked-in source. The file on disk still perturbs `C_elem_` in both numerical-differentiation loops, so the C++ branch at [src/core/element_energy.cpp:96](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/element_energy.cpp#L96) remains a translation mismatch until Claude either fixes it or produces a higher-priority canonical source proving the opposite.

## Goal Tracker Update Requests

Approved in part and applied in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md):

1. Accepted the real `write_mesh()` regression fix and the new Round 24 tests.
2. Rejected the request to mark `task3e` complete because the `flag_num_diff` parity claim was incorrect and the new tests do not provide direct Fortran-backed stress/force oracle evidence.
3. Updated the mutable tracker section: Plan Version bumped to 25, the Round 24 plan-evolution entry was rewritten, `task3e` was reopened, and the duplicated runtime-vdW blocker row was removed.

## Progress Stagnation Check

The loop is not yet stagnating. Round 23 added a real element-energy module and Round 24 fixed a real mesh-I/O regression while adding new kernel coverage. The problem is not circular non-work; the problem is that progress remains narrowly confined to Milestone 3 and the round still stopped short of a valid `task3e` closure. No STOP trigger yet.

## Required Implementation Plan

### Mainline Gaps

1. Fix the `flag_num_diff` `S_m` branch in [src/core/element_energy.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/element_energy.cpp) so it matches the checked-in canonical [ener_elem.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/ener_elem.f90), or cite and archive a higher-priority canonical source if the checked-in file is known-wrong.
2. Add direct Fortran-backed oracle fixtures for element-energy stresses and forces. `task3e` should not close until `S_n`, `S_m`, and `f_elem` are compared against canonical fixtures, not just against C++ finite differences.
3. After `task3e` is canonically verified, continue Milestone 4 in the original plan order: `task4a` assembly, `task4b` L-BFGS, `task4c` load controller, `task4d` `pasapas`, `task4e` reaction forces, `task4f` serial oracle run.

### Blocking Side Issues

1. Implement runtime vdW/self-contact (`task6a`-`task6c`) after the solver path exists; AC-8 is still blocked until the simulator-side kernel and exclusion logic are translated and verified.

### Queued Side Issues

1. Add `AGENT.md` and `document/translation_notes.md` when Milestone 8 documentation work resumes.

Round 24 remains incomplete.
<!-- CODEX's REVIEW RESULT  END  -->
---

## Goal Tracker Reference

Before starting work, **read** @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md to understand:
- The Ultimate Goal and Acceptance Criteria you're working toward
- Which tasks are Active, Completed, or Deferred
- Which side issues are blocking vs queued
- Any Plan Evolution that has occurred
- The latest side-issue state that needs attention

**IMPORTANT**: Keep the mutable section of `goal-tracker.md` up to date during the round.
Do NOT change the immutable section after Round 0.
If you cannot safely reconcile the tracker yourself, include an optional "Goal Tracker Update Request" section in your summary (see below).

## Mainline Guardrails

- Keep the mainline objective from @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-25-contract.md stable for this round
- Do not let queued issues take over the round
- If Codex reported several findings, classify them into:
  - mainline gaps
  - blocking side issues
  - queued side issues
- Only mainline gaps and blocking side issues should drive the next code changes

### Post-Alignment Check Action Items

This round follows a Full Goal Alignment Check. Pay special attention to:
- **Forgotten Items**: Codex may have identified tasks that were being ignored. Address them.
- **AC Status**: If any Acceptance Criteria were marked NOT MET, prioritize work toward those.
- **Deferred Items**: If any deferrals were flagged as unjustified, un-defer them now.
- **Queued Issues**: Keep non-blocking follow-up work queued unless it now clearly blocks mainline progress.

---

Note: You MUST NOT try to exit by lying, editing loop state files, or executing `cancel-rlcr-loop`.

After completing the work, please:
0. If the `code-simplifier` plugin is installed, use it to review and optimize your code. Invoke via: `/code-simplifier`, `@agent-code-simplifier`, or `@code-simplifier:code-simplifier (agent)`
1. Commit your changes with a descriptive commit message
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-25-summary.md

## Task Tag Routing Reminder

Follow the plan's per-task routing tags strictly:
- `coding` task -> Claude executes directly
- `analyze` task -> execute via `/humanize:ask-codex`, then integrate the result
- Keep Goal Tracker Active Tasks columns `Tag` and `Owner` aligned with execution

**Optional fallback**: if you could not safely update the mutable section of `goal-tracker.md` directly, include this section in your summary:
```markdown
## Goal Tracker Update Request

### Requested Changes:
- [E.g., "Mark Task X as completed with evidence: tests pass"]
- [E.g., "Add to Blocking Side Issues: bug Y blocks AC-2"]
- [E.g., "Add to Queued Side Issues: cleanup Z is non-blocking"]
- [E.g., "Plan Evolution: changed approach from A to B because..."]
- [E.g., "Defer Task Z because... (impact on AC: none/minimal)"]

### Justification:
[Explain why these changes are needed and how they serve the Ultimate Goal]
```

Codex will review your request and reconcile the Goal Tracker if justified.
