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
# Round 9 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 3/13 met | 5/13 partial | 5/13 not met | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. The broader twist `nvdw=1` preprocessor path is still not Fortran-compatible. The new Round 9 self-contact work is real, but the C++ preprocessor still never applies the second-sheet geometry rotation that the Fortran preprocessor performs for `nCodeLoad=222/333/1000` before `Def_Grad` and ghost-mesh generation. Compare the missing transform in [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L379) and [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L438) with the Fortran rotations in `../finite_crystal_elasticity/grapheneCompressionOriginPrePro/Prepro.f90:147-156` and `../finite_crystal_elasticity/grapheneCompressionOriginPrePro/Prepro.f90:211-225`. That means the new atomic-density branch is not just unverified; for any archived multi-sheet twist case it is currently incomplete by construction.

2. `read_data_dat()` still skips the Fortran `nborder >= 2` override for `nCodeLoad=222/1000`. The reference parser forces that guard in `../finite_crystal_elasticity/grapheneCompressionOriginPrePro/read_data.f90:130-133`, but the C++ path goes straight from parsed `nborder` to mesh sizing in [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L295). A valid twist input with fewer ghost rows will therefore diverge before any vdW comparison happens. This is a real compatibility gap in the still-pending broader `task2g`/twist path.

3. Round 9’s claimed single-sheet self-contact progress checks out. I verified the new parser order in [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L179), the new preprocessing helper in [include/fce/vdw_preprocessor.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/vdw_preprocessor.hpp) and [src/core/vdw_preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/vdw_preprocessor.cpp), the expanded comparator in [test/support/oracle_compare.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/support/oracle_compare.cpp#L320), the archived oracle case in [test/cases/graphene_self_contact/prepro_run](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_self_contact/prepro_run), and the provenance note in [test/cases/graphene_self_contact/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_self_contact/build_provenance.md). The claimed tests pass, including full `ctest` `40/40`. This narrows the blocker on `task2g`, but it does not close AC-2 or AC-8 because there is still no archive-backed multi-sheet/twist `nvdw=1` case, no translated preprocessing neighbor list, and no runtime vdW/self-contact implementation.

## Part 1: Goal Tracker Audit

### 1.1 Acceptance Criteria Status

| AC | Status | Evidence (if MET) | Blocker (if NOT MET) | Justification (if DEFERRED) |
|----|--------|-------------------|---------------------|----------------------------|
| AC-1 | MET | [test/cases/graphene_compression_prepro/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_prepro/build_provenance.md), [test/cases/graphene_compression_simulator/np1](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1), [test/cases/graphene_cyclic_crumple](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple), [document/fortran_conventions.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/fortran_conventions.md) | - | - |
| AC-2 | PARTIAL | - | Compression, cyclic disabled-vdW, and single-sheet self-contact preprocessor oracles now pass, but the broader `nvdw=1` twist/multi-sheet path is still incomplete; the Fortran `nborder` override and second-sheet rotation are still missing, and full AC-2 still depends on `task2g` | - |
| AC-3 | MET | [test/cases/bspline_oracle](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/bspline_oracle), [test/cases/tools/dump_bspline_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_bspline_oracle.f90), [test/unit/test_bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_bspline.cpp), full-suite `40/40` pass | - | - |
| AC-4 | MET | [test/cases/graphene_compression_prepro/ghost_coords.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_prepro/ghost_coords.dat), [test/cases/graphene_cyclic_crumple/prepro_run/ghost_coords.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/prepro_run/ghost_coords.dat), [test/support/oracle_compare.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/support/oracle_compare.cpp), [test/integration/test_prepro_oracle.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_prepro_oracle.cpp) | - | - |
| AC-5 | NOT MET | - | Brenner REBO translation and oracle tests are still absent | - |
| AC-6 | NOT MET | - | Inner Newton solver and oracle tests are still absent | - |
| AC-7 | NOT MET | - | The simulator entry point is still a stub at [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1); no translated solver mainline or end-to-end oracle run exists | - |
| AC-8 | PARTIAL | - | Preprocessor-side single-sheet self-contact `nvdw=1` evidence now exists, but runtime vdW/self-contact, neighbor-search parity, and archive-backed multi-sheet/twist `nvdw=1` coverage are still missing | - |
| AC-9 | PARTIAL | - | Preprocessor-side cyclic/crease artifacts exist, but runtime cyclic controller, `K0_ref` updates, and `crease_map.dat` verification are still absent | - |
| AC-10 | NOT MET | - | Checkpoint/restart is unimplemented | - |
| AC-11 | PARTIAL | - | MPI wrapper/partition utilities exist, but there is no translated solver path or multi-rank equivalence verification | - |
| AC-12 | NOT MET | - | VTU output and validation are still absent | - |
| AC-13 | PARTIAL | - | [CMakeLists.txt](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/CMakeLists.txt) exists, but [AGENT.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/AGENT.md) and [document/translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md) are still missing | - |

### 1.2 Forgotten Items Detection

- No task IDs from [document/plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md) are missing from Active, Completed, or Deferred tracking.
- One completed-task claim still hides unverified scope: `task2f` remains marked completed, but the Fortran-only twist behaviors noted in Findings 1 and 2 show that the broader `nCodeLoad=222/1000` preprocessor path is not yet parity-complete.
- No task is marked complete in the tracker solely on the basis of the Round 9 summary; the requested Round 9 update is evidence-only and keeps `task2g` pending.

### 1.3 Deferred Items Audit

- `Explicitly Deferred` is still empty.
- No current deferral justification needs to be revisited because nothing is formally deferred.
- The tracker still carries unfinished original-plan scope as pending work rather than disguised deferrals, which is the correct state.

### 1.4 Goal Completion Summary

Acceptance Criteria: 3/13 met (0 deferred)  
Active Tasks: 27 remaining  
Estimated remaining rounds: 8+  
Critical blockers: broader `task2g` twist/runtime vdW scope, Milestones 3-8 still unimplemented, simulator still stub, AC-13 docs still missing

## Part 2: Implementation Review

- Claude’s Round 9 claims about the single-sheet self-contact branch match reality. The repository now contains the committed oracle artifacts, the parser-order fix, the vdW reader/writer support, the new preprocessing helper, the expanded comparator, and the claimed tests.
- I re-ran:
  - `cmake --build build --target unit_tests integration_tests -j4`
  - `ctest --test-dir build --output-on-failure -R '^(ReadDims\\.GrapheneSelfContact|ReadVdw\\.GrapheneSelfContact|RoundTrip\\.(DimsSelfContact|Vdw)|PreprocessorOracle\\.ArchivedSelfContactPreproInputMatchesOracleOutputs)$'`
  - `ctest --test-dir build --output-on-failure`
- Results: all three commands passed; full suite is `40/40`.
- The remaining technical gap is not in the newly archived single-sheet case. It is in the unverified broader branch described by the design/reference sources:
  - `../finite_crystal_elasticity/document/codebase_analysis.md`
  - `../finite_crystal_elasticity/grapheneCompressionOriginPrePro/read_data.f90`
  - `../finite_crystal_elasticity/grapheneCompressionOriginPrePro/Prepro.f90`
  - `../finite_crystal_elasticity/grapheneCompressionOriginPrePro/vdw_previous.f90`

## Part 3: Goal Tracker Update Requests

Approved.

I updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md) to:

- add a Round 9 Plan Evolution entry covering the self-contact `nvdw=1` oracle/parsing/output work,
- keep `task2g` pending while replacing the stale “no committed `nvdw=1` oracle case” language with the narrower remaining blocker,
- record the requested evidence for the Round 9 self-contact slice,
- add a queued issue for the still-untranslated twist-specific Fortran behavior (`nborder` override and second-sheet rotation).

## Part 4: Progress Stagnation Check

- Recent rounds show narrow but real progress rather than a stall.
- Round 7 improved ghost-coordinate provenance reproducibility.
- Round 8 closed AC-3 with committed B-spline oracle fixtures.
- Round 9 materially narrowed the repeated `task2g` blocker by landing the first committed `nvdw=1` preprocessor oracle path and archive-backed parity test.
- The same broader blocker is still open, but the work is not circular enough to trigger the stagnation circuit breaker yet.

## Required Action Items for Claude

1. Port the Fortran second-sheet geometry rotation for `nCodeLoad=222/333/1000` into the C++ preprocessor before claiming any broader twist `nvdw=1` coverage.
2. Mirror the Fortran `nborder >= 2` override for `nCodeLoad=222/1000` and add a regression test that proves the override happens.
3. Archive and wire in a real multi-sheet or twist `nvdw=1` preprocessor oracle case so the still-pending atomic-density/twist branch is tested against committed Fortran outputs.
4. Continue `task2g` beyond the current self-contact slice: translate the remaining preprocessor/runtime vdW neighbor-search behavior and then move on to the still-absent Milestone 3+ simulator work.

Round 9 remains incomplete.
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
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-10-summary.md

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
