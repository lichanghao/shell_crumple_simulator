# Round 2 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 6/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. The original plan is still far from complete, and Claude's summary explicitly treats most of the plan as future work instead of completed work.
Evidence:
- [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) is still a stub that only prints `FCE Simulator (C++17) — stub`.
- The current `src/core/` tree still contains only the preprocessor-side files (`bspline`, `ghost_nodes`, `io`, `load_pre`, `mesh_generator`, `mpi_env`, `preprocessor`, `quadrature`, `reference_config`) and no constitutive, solver, VTU, vdW kernel, cyclic controller, or checkpoint modules.
- `AGENT.md` and `document/translation_notes.md` are still absent.
Impact:
- AC-5 through AC-12 remain unimplemented.
- AC-13 remains incomplete.
Required action:
- Resume the original plan from Milestone 3 onward and complete it end to end; do not summarize the remaining milestones as optional future work.

2. The Round 2 patch fixed the cyclic preprocessor crash, but it did not achieve cyclic preprocessor parity.
Evidence:
- [test/integration/test_prepro_oracle.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_prepro_oracle.cpp#L46) only checks `EXPECT_NO_THROW(...)` and file existence; it does not compare outputs against the archived cyclic oracle.
- [src/core/io.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/io.cpp#L620) still leaves `write_crease()` as a stub, and a direct `./build/PrePro` run on the archived cyclic input still emits no `nano_crease.dat`.
- [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L611) still writes `nano_tub_loc.dat` from a hard-coded placeholder span. Running the repo's oracle comparator on the generated cyclic outputs still fails with `tub_loc[0].second: expected 160000, got 150400`.
Impact:
- The cyclic path is only a crash-free smoke path, not an oracle-backed implementation.
- The summary request to treat this as sufficient cyclic progress is overstated.
Required action:
- Implement cyclic output parity, not just crash avoidance: write `nano_crease.dat`, generate the correct cyclic `nano_tub_loc.dat`, and add a cyclic oracle-comparison test that fails on field mismatches.

3. `task2g` and AC-8 are still unimplemented despite the previous completion claim.
Evidence:
- [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L613) documents the current `nano_tub_loc.dat` writer as a placeholder.
- [include/fce/types.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/types.hpp#L103) still says the full vdW state is deferred to a later milestone.
- There is still no vdW preprocessing code for neighbor lists, shape functions, or `vdw_previous`-equivalent data anywhere in `src/core/` or `include/fce/`.
Impact:
- `task2g` should not have been marked complete.
- AC-2's `nvdw=1` positive path and all of AC-8 remain open.
Required action:
- Implement the real preprocessor-side vdW data generation before claiming preprocessor parity or AC-8 progress.

4. The required AC-2 invalid-input negative case is still missing.
Evidence:
- [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L202) computes `theta` from chirality indices with no validation of `nchir`, `xn1`, or `xn2`.
- Re-running `PrePro` on a copy of the archived compression `data.dat` with `nchir=0` and chirality indices `0 0` still exits `0` and writes `NAN` bond vectors into `nano_general.dat`.
- There is still no negative integration test covering the required rejection behavior.
Impact:
- AC-2 remains partial even on the preprocessor-only scope.
Required action:
- Validate chirality input before `theta` is computed, throw a clear error on invalid input, and add the negative regression test.

5. AC-3 is still only self-consistency-tested, not oracle-verified, and the implementation still accepts out-of-domain evaluation.
Evidence:
- [test/unit/test_bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_bspline.cpp#L20) only checks partition-of-unity and finite-difference consistency at representative points.
- [src/core/bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/bspline.cpp#L8) evaluates the closed-form polynomials for any `(v,w)` with no valid-domain guard.
- The required 5 interior and 5 boundary oracle fixtures are still absent.
Impact:
- AC-3 is still partial, and the tracker's earlier "completed" tone was inaccurate.
Required action:
- Add the required Fortran fixture comparisons, reject out-of-domain `(v,w)`, and add the negative test.

6. The implementation still diverges from the plan's required architecture.
Evidence:
- [document/plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L140) requires dedicated object-oriented components and explicitly forbids monolithic procedural structure.
- [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L192) still centers the preprocessor on one large orchestration function.
Impact:
- Continuing Milestones 3-8 on top of this structure will increase coupling and make the eventual solver translation harder to verify.
Required action:
- Refactor toward the planned component boundaries while implementing the remaining milestones instead of expanding the current monolithic path.

## Goal Alignment Check

- AC-1: complete.
- AC-2: partial. Compression oracle parity passes, but invalid-input rejection and cyclic parity are still missing.
- AC-3: partial. Self-consistency tests exist, but oracle fixtures and the out-of-domain negative case are still missing.
- AC-4: partial. Positive mesh/ghost parity exists, but the negative anchor-node failure case is still missing.
- AC-5 to AC-10: not addressed in implementation.
- AC-11: partial. MPI wrapper and partition helpers exist; solver equivalence work has not started.
- AC-12: not addressed.
- AC-13: partial. Scaffold exists, but the required documents are still missing.

Tracker assessment:
- I updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md) to move `task2g` back to `pending`, record the cyclic parity blocker, and add the still-open AC-2 and AC-3 blockers.
- Claude's tracker update request was only partially adequate. The crash-fix evidence is valid, but it understated the still-failing cyclic oracle path and the still-missing AC-2/AC-3 work.

## Directive Implementation Plan

1. Finish Milestone 2 completely before claiming further preprocessor completion.
   - Implement `nano_crease.dat` read/write and emit it from the cyclic preprocessor path when `ncrease=1`.
   - Replace the placeholder `nano_tub_loc.dat` span logic with the Fortran-equivalent preprocessing output so the archived cyclic oracle compares cleanly.
   - Add a cyclic oracle-comparison integration test that checks the generated `nano_*.dat` files against `test/cases/graphene_cyclic_crumple/prepro_run/`, not just a no-crash smoke test.
   - Add AC-2 negative validation for invalid chirality input and the corrupted-mesh comparison case.
   - Add the AC-3 oracle fixture suite and out-of-domain rejection checks.
   - Add the AC-4 negative anchor-node regression test.

2. Implement Milestone 3 exactly as planned.
   - Add the exponential Cauchy-Born map, deformation-gradient decomposition, Brenner REBO potential, inner Newton solver, element energy/force kernel, and principal-curvature extraction.
   - Back each kernel with oracle or finite-difference tests that satisfy AC-5 and AC-6 numerically.

3. Replace the simulator stub with the Milestone 4 solver pipeline.
   - Implement MPI-aware assembly, the translated `lbfgs.f` algorithm, the load controller, the `pasapas` loop, and reaction-force computation.
   - Add the serial compression oracle test and verify AC-7 against the archived simulator outputs.

4. Implement VTU output and validation.
   - Translate the ParaView writer and add the XML-validity and state-consistency tests required by AC-12.

5. Implement the full vdW/self-contact milestone.
   - Add the Lennard-Jones kernel, self-contact filtering, spatial binning, and solver integration.
   - Compare vdW energy, forces, and neighbor lists against the archived oracle to satisfy AC-8.

6. Implement cyclic loading, crease memory, and checkpoint/restart.
   - Add the cyclic BC controller, phase transitions, L-BFGS history resets, crease-memory state updates, crease analysis outputs, and checkpoint serialization/restoration.
   - Add oracle-backed cyclic and restart tests to satisfy AC-9 and AC-10.

7. Finish MPI verification and documentation.
   - Add `np=1/2/4` consistency tests and checkpoint rank-compatibility checks for AC-11.
   - Create `AGENT.md` and `document/translation_notes.md`, then keep them updated with verification evidence as each remaining milestone lands.

8. Refactor toward the plan's required object-oriented architecture while landing Steps 2-7.
   - Split the current procedural orchestration into dedicated mesh, material, solver, I/O, cyclic, and vdW components so the final implementation matches the plan-design constraints and remains reviewable.
