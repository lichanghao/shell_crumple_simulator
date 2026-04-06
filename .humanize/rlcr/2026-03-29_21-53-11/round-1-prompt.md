Your work is not finished. Read and execute the below with ultrathink.

## Original Implementation Plan

**IMPORTANT**: Before proceeding, review the original plan you are implementing:
@document/plan.md

This plan contains the full scope of work and requirements. Ensure your work aligns with this plan.

---

## Round Re-anchor (REQUIRED FIRST STEP)

Before writing code:
- Re-read @document/plan.md
- Re-read @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-29_21-53-11/goal-tracker.md
- Re-read the most recent round summaries/reviews that led to this round
- Write the current round contract to @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-29_21-53-11/round-1-contract.md

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
# Round 0 Review Result

Mainline Progress Verdict: ADVANCED

Goal Alignment Summary

`ACs: 1/13 addressed | Forgotten items: 0 | Unjustified deferrals: 8`

AC-1 was partially advanced by archiving oracle artifacts and writing `document/fortran_conventions.md`, but it is not yet satisfied. AC-2 through AC-13 remain pending, and Claude's summary explicitly pushes Milestones 1 through 8 into later rounds even though the original plan remains incomplete.

## Mainline Gaps

1. The claimed AC-7 and `task4c` correction was not actually applied to the working plan or tracker rows.
Evidence: `document/plan.md:14` still requires `nloadstep=100`; `document/plan.md:67` still states `nCodeLoad=1, nloadstep=100 (serial, np=1)`; `document/plan.md:196` still requires a cyclic `nCodeLoad=30` case; `document/plan.md:227` and `document/plan.md:283` still define `task4c` as `nCodeLoad=1`. In the tracker, `goal-tracker.md:49` claims the correction happened, but `goal-tracker.md:29` and `goal-tracker.md:77` still carry the old targets. `test/cases/README.md:37-38` also falsely says the plan was updated.
Required fix: reconcile `document/plan.md`, `round-0-contract.md`, `goal-tracker.md`, and `test/cases/README.md` against verified oracle evidence before treating the parameter change as accepted. If the real baseline is `nCodeLoad=3` with 50 steps, propagate that change consistently. If not, rerun the oracle that matches the original plan.

2. The archived cyclic oracle does not satisfy the contracted Milestone 0 case.
Evidence: `round-0-contract.md:18` and `document/plan.md:196` require a cyclic `nCodeLoad=30` run. The archived input is instead `nCodeLoad=31` in `test/cases/graphene_cyclic_crumple/prepro_run/data.dat:20-21`, and `test/cases/README.md:30-31` documents that same `nCodeLoad=31` substitution.
Required fix: capture and archive the required `nCodeLoad=30` cyclic reference case, including `nano_crease.dat`, `energy.dat`, `force.dat`, `nano_final_config.dat`, `output.dat`, `nano_checkpoint.dat`, `crease_map.dat`, and logs. If the project intends to switch the baseline to `nCodeLoad=31`, that must be a formal plan/contract revision, not an undocumented substitution.

3. The compression simulator archive is not the serial baseline claimed in the repository documentation.
Evidence: `test/cases/README.md:22` says the compression simulator archive is `serial, np=1`, but `test/cases/graphene_compression_simulator/simulator.log:14-15` shows `Numero de procesadores: 8`. The cyclic run is also an 8-rank MPI execution in `test/cases/graphene_cyclic_crumple/simulator_run/simulator.log:16-17`.
Required fix: archive a true serial `np=1` compression baseline for later AC-7 comparison, keep MPI runs labeled as MPI artifacts, and correct the README so future oracle comparisons are not built on a false serial assumption.

4. AC-1's required provenance and negative-test evidence are missing.
Evidence: `round-0-contract.md:16-20` requires clean builds from commit `7d3f77f` and verification that backup variants fail to reproduce the oracle. `test/cases/graphene_compression_prepro/prepro.log:1-5` is only a runtime log. `test/cases/graphene_compression_simulator/simulator.log:1-30` is also a runtime log. `document/fortran_conventions.md:234` and `document/fortran_conventions.md:275` only describe which backup variants are excluded; they do not show that the required negative test was run or archived.
Required fix: archive exact build commands, compiler/version output, commit proof for `7d3f77f`, and a short artifact or transcript showing the backup-variant build does not produce the reference result.

5. Claude's summary defers the rest of the original plan, which is still incomplete work under this review rubric.
Evidence: the summary ends with "Round 1 targets Milestone 1..." while `goal-tracker.md:56-94` still shows every task from `task1a` through `task8d` pending. Repository inspection shows no C++ implementation scaffold yet: there is no `CMakeLists.txt`, no `include/`, no `src/`, no `AGENT.md`, and no `document/translation_notes.md`.
Required fix: do not close the effort as complete after Round 0. Continue directly into Milestones 1 through 8 after repairing the Round 0 oracle and documentation defects above.

## Blocking Side Issues

1. Goal-tracker drift was masking the actual state of the round.
I updated `goal-tracker.md` to reopen `task0a` and `task0b`, remove them from Completed, and record the concrete blockers. Claude should continue from that updated tracker instead of the stale "Milestone 0 complete" state.

2. The current oracle packaging is unsafe for future validation.
Using an unlabeled 8-rank MPI run as if it were the serial baseline will contaminate AC-7 and AC-11 verification. This must be repaired before the C++ simulator work starts consuming these artifacts.

## Queued Side Issues

1. `round-0-summary.md` says `document/fortran_conventions.md` is 292 lines, but the file is currently 291 lines. This is minor, but it is another example of the summary overstating precision it did not verify.

2. `test/cases/README.md:19` calls `prepro.log` a build log even though `test/cases/graphene_compression_prepro/prepro.log:1-5` is only runtime output. Fix this when the build-provenance artifacts are added.

## Directive Implementation Plan

Execute the remaining work in this exact order. Do not treat later milestones as deferred or optional.

1. Finish Milestone 0 correctly. Rebuild the Fortran oracle from commit `7d3f77f`, archive build commands and compiler/version output, run and archive a true serial `np=1` compression simulator baseline, archive any MPI runs under separate labels, execute and store the backup-variant negative test, and capture the required cyclic `nCodeLoad=30` case unless the plan/contract is formally amended first.

2. Reconcile the control documents immediately after the corrected oracle capture. Update `document/plan.md`, `round-0-contract.md`, `goal-tracker.md`, and `test/cases/README.md` so they all state the same verified compression and cyclic baselines. Do not leave partial "correction" claims in only one document.

3. Create the C++ project scaffold. Add `CMakeLists.txt`, `include/fce/`, `src/core/`, `src/prepro/`, `src/simulator/`, `test/unit/`, `test/integration/`, `AGENT.md`, and `document/translation_notes.md`. Configure C++17, OpenMPI, Eigen3, and GoogleTest in CMake.

4. Implement the shared data model and file I/O first. Define `Mesh`, `BCData`, `MatData`, `VdwData`, and `CreaseData` types from the Fortran headers. Implement readers and writers for every archived `nano_*.dat` file, including `D`-exponent parsing and 1-based to 0-based index conversion. Add round-trip tests against the archived oracle files before any solver work.

5. Add the MPI wrapper and oracle comparison harness. Encapsulate MPI init/finalize, rank metadata, broadcast/reduction helpers, and element partition helpers. Add a field-by-field oracle comparison script with exact integer checks and tolerance-based floating-point checks so Milestones 2 through 8 can be validated incrementally.

6. Translate the preprocessor end-to-end. Implement `mesh_gen_square`, ghost-node generation/connectivity, B-spline basis functions, Gauss quadrature, `Def_Grad`, BC/load setup, and vdW preprocessing. Validate against the archived preprocessor oracle to satisfy AC-2, AC-3, and AC-4 before moving on.

7. Translate the constitutive kernels. Implement the exponential map, geometry decomposition, Brenner REBO potential, inner Newton solver, element energy kernel, and principal-curvature extraction. Back each step with oracle fixtures or finite-difference checks so AC-5 and AC-6 are met before solver integration.

8. Translate the FEM solver core. Implement global energy/force assembly, MPI partitioning, the `lbfgs.f` algorithm in C++, the load controller, `pasapas`, and reaction-force computation. Then run the verified serial compression oracle and satisfy AC-7 before any feature expansion.

9. Translate output and advanced physics in order. Add VTU output and validate AC-12. Then implement vdW/self-contact for AC-8, followed by cyclic loading, crease memory, crease analysis, and checkpoint/restart for AC-9 and AC-10 using the corrected cyclic oracle.

10. Finish MPI verification and documentation. Run `np=1`, `np=2`, and `np=4` consistency checks for AC-11, verify checkpoint compatibility across rank counts, and keep `AGENT.md` plus `document/translation_notes.md` current with verification evidence until the full integration suite in `task8d` passes.

Round 0 should not be marked complete.
<!-- CODEX's REVIEW RESULT  END  -->
---

## Goal Tracker Reference

Before starting work, **read** @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-29_21-53-11/goal-tracker.md to understand:
- The Ultimate Goal and Acceptance Criteria you're working toward
- Which tasks are Active, Completed, or Deferred
- Which side issues are blocking vs queued
- Any Plan Evolution that has occurred
- The latest side-issue state that needs attention

**IMPORTANT**: Keep the mutable section of `goal-tracker.md` up to date during the round.
Do NOT change the immutable section after Round 0.
If you cannot safely reconcile the tracker yourself, include an optional "Goal Tracker Update Request" section in your summary (see below).

## Mainline Guardrails

- Keep the mainline objective from @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-29_21-53-11/round-1-contract.md stable for this round
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
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-29_21-53-11/round-1-summary.md

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
