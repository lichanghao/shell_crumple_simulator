# Round 4 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 1/13 met | Forgotten items: 0 | Unjustified deferrals: 0 | Stagnation: not triggered`

## 1. Goal Tracker Audit

### 1.1 Acceptance Criteria Status

| AC | Status | Evidence (if MET) | Blocker (if NOT MET) | Justification (if DEFERRED) |
|----|--------|-------------------|----------------------|-----------------------------|
| AC-1 | MET | Archived compression + cyclic oracle artifacts and conventions doc are present in [test/cases/graphene_compression_prepro/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_prepro/build_provenance.md), [test/cases/graphene_cyclic_crumple/prepro_run/data.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/prepro_run/data.dat), and [document/fortran_conventions.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/fortran_conventions.md) | - | - |
| AC-2 | PARTIAL | - | Archived compression and archived cyclic disabled-vdW preprocessor oracle tests pass, and Round 4 added invalid-input/corrupted-output negatives, but the real `nvdw=1` preprocessing path is still missing; [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L190) still writes `nano_tub_loc.dat` from preserved span constants and [include/fce/types.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/types.hpp#L108) still defers full vdW state. | - |
| AC-3 | PARTIAL | - | Round 4 correctly added the out-of-domain guard, but the required 5 interior and 5 boundary Fortran oracle fixtures are still absent; current evidence is only self-consistency plus rejection tests in [test/unit/test_bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_bspline.cpp#L20). | - |
| AC-4 | PARTIAL | - | The wrong-anchor negative regression now exists, but positive all-boundary ghost-position evidence is still indirect because the oracle comparator does not check ghost coordinates; see [test/unit/test_ghost_nodes.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_ghost_nodes.cpp#L37) and [test/support/oracle_compare.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/support/oracle_compare.cpp#L134). | - |
| AC-5 | NOT MET | - | No Brenner REBO implementation or oracle/FD test fixture exists yet; the simulator-side Milestone 3 files are still absent and the project remains preprocessor-only. | - |
| AC-6 | NOT MET | - | No inner Newton solver or element-level oracle state fixtures exist yet. | - |
| AC-7 | NOT MET | - | [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) is still a stub, so there is no end-to-end serial compression implementation to compare against the archived oracle. | - |
| AC-8 | NOT MET | - | No vdW runtime kernel, no self-contact logic, and no `nvdw=1` preprocessor/runtime oracle coverage exist. | - |
| AC-9 | NOT MET | - | Archived cyclic preprocessor artifacts exist, but there is still no cyclic runtime controller, `K0_ref` update logic, or `crease_map.dat` generation in C++. | - |
| AC-10 | NOT MET | - | No checkpoint/restart implementation or restart equivalence test exists. | - |
| AC-11 | PARTIAL | - | MPI scaffolding exists, but there is no solver path and therefore no `np=1/2/4` equivalence evidence. | - |
| AC-12 | NOT MET | - | No VTU writer or ParaView validation path exists in the C++ implementation. | - |
| AC-13 | PARTIAL | - | [document/plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md) and [document/fortran_conventions.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/fortran_conventions.md) exist, but `AGENT.md` and `document/translation_notes.md` are still missing. | - |

### 1.2 Forgotten Items Detection

- No plan task IDs are missing from the tracker after the Round 4 update.
- No current Round 4 completion claim is unverified. Commit `91e6fa8` is present, the guard/test changes exist in the repo, and serial verification reproduced `9/9` targeted passes plus `31/31` full-suite passes.
- Bookkeeping note: the `Active Tasks` table still contains completed Milestone 2 items with `status=completed`, so the section title overstates the number of truly remaining tasks, but the task IDs themselves are not forgotten.

### 1.3 Deferred Items Audit

- `Explicitly Deferred` is empty. No deferred-item contradiction exists at this checkpoint.

### 1.4 Goal Completion Summary

```text
Acceptance Criteria: 1/13 met (0 deferred)
Active Tasks: 27 remaining
Estimated remaining rounds: at least 8, likely more once Milestones 3-8 are in flight
Critical blockers: simulator still stub; real vdW preprocessing absent; B-spline oracle fixtures absent; direct ghost-position oracle coverage absent; documentation still missing
```

## 2. Implementation Review

Round 4's specific claim set is materially accurate. Commit `91e6fa8` adds the chirality guard in [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L205), the B-spline domain guard in [src/core/bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/bspline.cpp#L12), and the new regressions in [test/integration/test_prepro_oracle.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_prepro_oracle.cpp#L89), [test/unit/test_bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_bspline.cpp#L101), and [test/unit/test_ghost_nodes.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_ghost_nodes.cpp#L37). Serial verification also matches the summary: `ctest --test-dir build --output-on-failure -R '^(BSpline|GhostNodes|PreprocessorOracle)'` passes `9/9`, and `ctest --test-dir build --output-on-failure` passes `31/31`.

### Findings

1. The original plan is still far from complete; Round 4 tightened Milestone 2 coverage but did not advance the simulator mainline at all. [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) is still a stub, and the repository still has no constitutive, solver, checkpoint, cyclic-runtime, vdW-kernel, or VTU implementation files. This keeps AC-5 through AC-12 unimplemented and AC-13 partial.

2. AC-2 remains partial because the real preprocessor-side vdW path is still absent. The disabled-vdW bridge in [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L190) is still the mechanism behind `nano_tub_loc.dat`, and [include/fce/types.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/types.hpp#L108) still explicitly defers the full vdW state. Round 4 correctly closed two AC-2 negative-coverage gaps, but it did not close the remaining positive-path requirement for `nvdw=1`.

3. AC-3 is still only partially verified. The new out-of-domain rejection is the right fix for the negative case, but the test file [test/unit/test_bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_bspline.cpp#L20) still contains only partition-of-unity, finite-difference consistency, and out-of-domain checks. The required 5 interior and 5 boundary Fortran oracle fixtures are still missing, so basis-value parity is not yet demonstrated.

4. AC-4 is still only partially verified. The new negative regression is useful, but the positive evidence is not yet the acceptance criterion the plan asked for. The comparator in [test/support/oracle_compare.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/support/oracle_compare.cpp#L134) reads `nano_config.dat` only for `oracle_dims.numnods`, so ghost coordinates themselves are never compared against the archived oracle. The current AC-4 evidence is therefore still indirect.

5. The Round 4 changes increased a pre-existing test-infrastructure risk: [test/integration/test_prepro_oracle.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_prepro_oracle.cpp#L23) uses one fixed temp directory for every `PreprocessorOracle` test. Serial RLCR verification is fine, but `ctest -j` or overlapping local invocations can race on that directory and create flaky failures. This is not blocking the round, but it should be corrected before parallel test execution is adopted.

## 3. Goal Tracker Update Request Assessment

Claude's tracker update request is approved, with one addition.

- I updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md) to remove the stale blockers for invalid chirality acceptance, missing corrupted-mesh negative coverage, and missing wrong-anchor negative coverage.
- I updated the AC-2, AC-3, and AC-4 evidence rows to reflect the Round 4 tests and the passing `31/31` serial regression run.
- I kept `task2g` pending and kept AC-2/AC-8 open because the real `nvdw=1` preprocessing path is still missing.
- I added a new AC-4 blocker capturing what still prevents closure: the positive ghost-position evidence is indirect because the oracle comparator does not validate ghost coordinates.
- I also added a queued issue for the fixed-temp-dir test flake risk.

## 4. Stagnation Check

The stagnation circuit breaker is not triggered.

Reasoning:
- Round 2, Round 3, and Round 4 each introduced real code changes and retired specific review findings (`dd81f2d`, `2bd3166`, `91e6fa8`).
- The same issues did recur across rounds, but they were being worked down rather than ignored.
- Progress is narrow and still confined to Milestone 2, which is a concern for schedule realism, but it is not circular or static enough to justify `STOP` at this checkpoint.

## Required Action Items

1. Finish Milestone 2 closure precisely, not rhetorically.
   Add the real `vdw_previous`-equivalent preprocessing path, neighbor-list/shape-function generation, and `nvdw=1` oracle coverage so AC-2 and AC-8 can move beyond the disabled-vdW bridge.

2. Close the remaining AC-3 and AC-4 evidence gaps.
   Add the required 5 interior and 5 boundary Fortran B-spline fixtures, and add direct archived ghost-coordinate verification for the 20 nm×20 nm boundary elements rather than relying on indirect evidence through downstream quantities.

3. Resume the original mainline from Milestones 3 and 4.
   Replace the simulator stub with the constitutive kernels, inner Newton solver, assembly, translated `lbfgs.f`, load controller, and end-to-end serial oracle path required by AC-5 through AC-7.

4. Finish the missing documentation work.
   Create `AGENT.md` and `document/translation_notes.md`, then keep them current as the remaining milestones land.

5. Harden the test harness before parallel execution.
   Make each `PreprocessorOracle` test use a unique temp directory so `ctest -j` and overlapping local runs are safe.

Stagnation circuit breaker: not triggered.
