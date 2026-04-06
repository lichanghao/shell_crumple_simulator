# Round 23 Review

Mainline Progress Verdict: ADVANCED

Goal Alignment Summary:
`ACs: 10/13 addressed | Forgotten items: 0 | Unjustified deferrals: 24`

## Mainline Gaps

1. `task3e` is overclaimed as complete. The new kernel exists, but the new tests only validate `eta` and `W_elem`; they never assert `result.f_elem`, so the "force kernel" half of the task still has no oracle coverage. See [test/unit/test_element_energy.cpp:251](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_energy.cpp#L251) and [test/unit/test_element_energy.cpp:327](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_energy.cpp#L327). I updated [goal-tracker.md:87](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L87) to reopen `task3e`.

2. `compute_element_energy()` is not yet a faithful translation of the canonical `flag_num_diff` branch. The C++ bending-stress loop perturbs `curv0_elem` in [src/core/element_energy.cpp:96](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/element_energy.cpp#L96), but the canonical Fortran still perturbs `C_elem_` in the corresponding `S_m` loop at [ener_elem.f90:76](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/ener_elem.f90#L76). If this is meant to be an intentional bug fix, it needed a plan-evolution entry and a regression proving the new behavior; if not, it is a translation mismatch. Either way, `task3e` cannot be closed yet.

3. Claude’s summary explicitly defers the rest of the original plan, and those deferrals still block the target ACs. The tracker still has [goal-tracker.md:83](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L83) through [goal-tracker.md:108](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L108) pending, [src/simulator/main.cpp:1](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) is still a stub, and [CMakeLists.txt:39](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/CMakeLists.txt#L39) through [CMakeLists.txt:80](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/CMakeLists.txt#L80) still contain no `energy`, `lbfgs`, `pasapas`, `reaction`, runtime `vdw`, `checkpoint`, `crease`, or `vtu` modules. Round 23 advanced Milestone 3, but it did not complete the original plan.

## Blocking Side Issues

1. The Round 23 `read_mesh()` fix regressed mesh round-tripping. [src/core/io.cpp:510](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/io.cpp#L510) now correctly treats column 1 as a 1-based node index for every positive entry, but [src/core/io.cpp:566](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/io.cpp#L566) still writes neighbor indices with the obsolete ghost-flag rule. That contradicts [test/integration/test_oracle_roundtrip.cpp:141](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_oracle_roundtrip.cpp#L141), and I reproduced the failure with `./build/integration_tests --gtest_filter=RoundTrip.Mesh`. The current failure is directly explained by the Round 23 diff, so the summary’s "pre-existing unrelated failure" claim is not credible.

## Queued Side Issues

1. `task3f` remains pending and was omitted from Claude’s "Remaining Items" summary even though it is still tracked at [goal-tracker.md:88](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L88). This did not block the Round 23 mainline movement, but it must not disappear from the next round’s execution plan.

## Goal Alignment Check

- AC-1 to AC-6 still have completed or partial evidence, and Round 23 materially improved AC-6 by switching the archived constitutive corpus to non-trivial final-state geometry.
- AC-7 has progressed but is still not met: `task3e` is reopened, global assembly is absent, and the simulator remains a stub.
- AC-8 remains partial: only the preprocessor-side vdW slice is implemented.
- AC-9, AC-10, and AC-12 are still not met.
- AC-11 remains partial: MPI utilities exist, but there is still no solver path to compare `np=1/2/4`.
- AC-13 remains partial: [goal-tracker.md:107](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L107) is still open.
- Forgotten items: none. Every original-plan task is still represented in the tracker.
- Deferred items: the remaining `task3*`, `task4*`, `task5*`, `task6*`, `task7*`, and `task8*` work is still deferred by summary or pending tracker state, and those deferrals are not justified as "complete" because they continue to block AC-7 through AC-13.
- Plan evolution: no new design change justifies diverging from the canonical `ener_elem.f90` branch semantics; the only justified tracker evolution this round was to correct overclaims. I updated [goal-tracker.md:70](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L70), [goal-tracker.md:87](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L87), and [goal-tracker.md:114](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L114).

## Required Implementation Plan

1. Fix the mesh-I/O regression first. Update `write_mesh()` so every positive `neigh_vert` is written back as 1-based regardless of `neigh_elem`, then rerun `RoundTrip.Mesh` and the preprocessor oracle tests.

2. Refinish `task3e` before moving on. Make the `flag_num_diff` branch match the canonical `ener_elem.f90` semantics, add a focused regression that forces `flag_num_diff == true`, and extend the archived element fixtures so `compute_element_energy()` is asserted on `f_elem` component-by-component as well as `eta` and `W_elem`.

3. Close the remaining Milestone 3 gap in `task3f`. Add Fortran-derived principal-curvature oracle fixtures, verify the repeated-curvature branch against archived data, and keep the tracker entry pending until that evidence exists.

4. Implement Milestone 4 in plan order without skipping ahead: add `energy` assembly with persistent per-Gauss `eta`, `J0/2` scaling, ghost-node handling, and MPI reductions; translate `lbfgs.f`; add the simulator-side `nCodeLoad=3` controller; translate `pasapas`; add reaction-force extraction; replace the simulator stub; then run the archived serial compression oracle to close AC-7.

5. After the serial solver path is real, implement Milestones 5 through 7 in order: VTU output, runtime vdW/self-contact integration, cyclic loading, crease memory, checkpoint/restart, and the archived cyclic oracle tests.

6. Finish Milestone 8 last: multi-rank parity, checkpoint rank-count compatibility, `AGENT.md`, `document/translation_notes.md`, and the full AC-spanning integration suite.

## Verification

- `./build/unit_tests --gtest_filter=ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures:ElementState.MatchesArchivedCompressionSimulatorOracleFixtures` passes.
- `./build/integration_tests --gtest_filter=RoundTrip.Mesh` fails because of the new `write_mesh()` mismatch described above.
