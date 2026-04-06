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
- Write the current round contract to @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-27-contract.md

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
# Round 26 Review

Mainline Progress Verdict: ADVANCED

Goal Alignment Summary:
`ACs: 5/13 addressed | Forgotten items: 0 | Unjustified deferrals: 9`

## Mainline Gaps

1. Round 26 repaired the two reopened blockers, but it did not complete the original-plan mainline. The summary still leaves `task3a`, `task3c`, `task3f`, and `task4a` onward unfinished at [round-26-summary.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-26-summary.md#L65), while the plan requires Milestone 3 completion and then the full solver core for AC-7 at [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L216) and [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L224). The repository still has only a stub simulator entry point at [main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1). This round advanced kernel parity, but AC-7 is still not met.

2. `task3a` remains legitimately pending. Current exponential coverage is still synthetic self-consistency and finite-difference testing in [test_exponential.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_exponential.cpp#L45); there are no committed Fortran-derived exponential fixtures or archived-state oracle comparisons, even though Milestone 3 Phase A explicitly requires the translated `exponential.f90` kernel to be verified against the Fortran reference at [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L217) and [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L275).

3. `task3c` remains legitimately pending. Brenner is still only validated as a standalone constitutive kernel through fixtures in [test_constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_constitutive.cpp#L171). The committed element-energy oracle driver in [dump_element_energy_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_element_energy_oracle.f90#L364) still hard-stops on any non-Morse material payload, so there is still no Fortran-backed Brenner-through-`compute_element_energy(...)` proof on the production path. AC-5 therefore remains partial rather than complete.

4. `task3f` remains legitimately pending. Principal-curvature coverage is still synthetic finite-difference testing in [test_principal.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_principal.cpp#L30), with no committed Fortran-derived principal fixtures and no simulator or crease-memory path consuming the module yet. That is below the Milestone 3 Phase F requirement at [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L222) and blocks meaningful AC-9 progress.

5. The round contract and summary still treat later work as out of scope, but the original plan does not authorize stopping here. The tracker still shows every Milestone 4-8 task pending at [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L93), and the plan sequences those tasks directly after the constitutive model at [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L224) through [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L253). These tasks are not forgotten, but they are still unjustifiably deferred.

## Blocking Side Issues

1. Runtime vdW/self-contact is still unimplemented. The tracker already records this at [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L114). Preprocessor-side `nvdw=1` parity is real, but AC-8 cannot close until the simulator-side `vdw_modules.f90` path is translated and integrated into global assembly.

## Queued Side Issues

1. `AGENT.md` and `document/translation_notes.md` are still missing, so AC-13 remains partial. This is already tracked at [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L120) and should stay behind solver and runtime-physics work.

2. The Round 26 oracle helper still declares the `principal_` interface argument `flag_num_diff` as `intent(out)` at [dump_element_energy_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_element_energy_oracle.f90#L76), even though the provenance and the upstream Fortran both rely on read-before-write semantics. Current tests pass because the actual call site now supplies the already-computed flag, so this is not blocking, but the helper’s interface should be reconciled with the documented behavior in a cleanup pass.

## Goal Alignment Check

- AC-1: met.
- AC-2: met again. I reran `./build/integration_tests`, and all four archived preprocessor-oracle cases plus `RoundTrip.Mesh` pass.
- AC-3: met.
- AC-4: met.
- AC-5: partial. Standalone Brenner parity exists, but the production element-energy path is still not Fortran-oracle verified for `nCode_Pot=2`.
- AC-6: met.
- AC-7: advanced, not met. `task3e` now has direct `flag_num_diff` oracle evidence, but the solver core (`task4a`-`task4f`) is still absent and the simulator remains a stub.
- AC-8: partial. Preprocessor-side vdW is covered; runtime vdW/self-contact is still missing.
- AC-9: not met.
- AC-10: not met.
- AC-11: partial. MPI wrapper/partitioning exist, but there is no solver path to compare across ranks.
- AC-12: not met.
- AC-13: partial. The project still lacks `AGENT.md` and `document/translation_notes.md`.

No original-plan tasks are forgotten from the tracker. I did update [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md) to fix stale Milestone 3 pending-task rationale that still claimed `task3e` or the geometry/element-energy pipeline was missing after Round 26 had already closed those blockers.

## Required Implementation Plan

1. Finish the remaining Milestone 3 oracle work before claiming the constitutive model complete. Add a Fortran dump helper and committed fixtures for `exponential.f90`, then update [test_exponential.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_exponential.cpp#L45) to compare `pe`, `dpedC`, and `dpedk` directly against oracle data from archived simulator-state inputs rather than only self-consistency checks.

2. Add a Brenner-through-element-energy oracle path. Extend [dump_element_energy_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_element_energy_oracle.f90#L1) or add a sibling driver that loads a Brenner material payload, emits `W_elem`, `f_elem`, `eta`, and per-Gauss stresses for both non-degenerate and degenerate cases, and wire matching assertions into [test_element_energy.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_energy.cpp#L459).

3. Add a dedicated principal-curvature oracle driver and fixtures. Dump `curvppal`, `vppal`, `dcurvppaldC`, `dcurvppaldk`, `dvppaldC`, and `dvppaldk` for one distinct-curvature archived state and one repeated-curvature archived state, then update [test_principal.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_principal.cpp#L30) to compare direct oracle values.

4. Implement Milestone 4 under `include/fce/` and `src/core/` in the plan’s order: global energy/force assembly (`energy.f90`/`pre_ener.f90` equivalent), translated `lbfgs.f` two-loop recursion with Wolfe line search, simulator-side load controller for `nCodeLoad=3`, `pasapas` load stepping, and reaction-force computation.

5. Replace the stub simulator entry point at [main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) with a real driver that reads archived `nano_*.dat`, partitions work by MPI rank, executes the compression load loop, and writes `energy.dat`, `force.dat`, `output.dat`, `nano_final_config.dat`, and VTU outputs.

6. Add AC-7 integration coverage immediately after the solver exists. Compare per-step energy against [energy.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1/energy.dat), step-50 reaction force against [force.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1/force.dat), and step-25 displacements against [nano_final_config.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1/nano_final_config.dat) or a committed step-25 dump. Fail fast on NaN/Inf and corrupted-mesh input.

7. Continue directly into Milestones 5-8 without another deferral: add the VTU writer and XML validation, translate simulator-side vdW/self-contact and integrate it into assembly, implement cyclic loading plus crease memory plus checkpoint/restart, verify MPI consistency and checkpoint portability across rank counts, then finish `AGENT.md`, `document/translation_notes.md`, and the full end-to-end integration suite.

## Verification

- `./build/unit_tests` passed: 60/60.
- `./build/integration_tests` passed: 18/18.
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

- Keep the mainline objective from @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-27-contract.md stable for this round
- Do not let queued issues take over the round
- If Codex reported several findings, classify them into:
  - mainline gaps
  - blocking side issues
  - queued side issues
- Only mainline gaps and blocking side issues should drive the next code changes

---

Note: You MUST NOT try to exit by lying, editing loop state files, or executing `cancel-rlcr-loop`.

After completing the work, please:
0. If the `code-simplifier` plugin is installed, use it to review and optimize your code. Invoke via: `/code-simplifier`, `@agent-code-simplifier`, or `@code-simplifier:code-simplifier (agent)`
1. Commit your changes with a descriptive commit message
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-27-summary.md

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
