# Round 25 Review

Mainline Progress Verdict: STALLED

Goal Alignment Summary:
`ACs: 4/13 addressed | Forgotten items: 0 | Unjustified deferrals: 9`

## Mainline Gaps

1. `task3e` is still overclosed. The new oracle test at [test_element_energy.cpp:459](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_energy.cpp#L459) only checks `W_elem` and aggregated `f_elem` for the analytical `flag_num_diff=false` path. The provenance explicitly says the committed fixture never enters the branch changed this round at [build_provenance.md:25](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/element_energy_oracle/build_provenance.md#L25), and the Fortran driver only writes `W_elem` and `f_elem`, not `S_n` or `S_m`, at [dump_element_energy_oracle.f90:205](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_element_energy_oracle.f90#L205) and [dump_element_energy_oracle.f90:248](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_element_energy_oracle.f90#L248). That means the canonical fix in [element_energy.cpp:93](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/element_energy.cpp#L93) is still only code-inspected plus smoke-tested, not Fortran-oracle verified. Keep `task3e` open until a degenerate Fortran fixture forces `flag_num_diff=true` and the test asserts direct `S_n`, `S_m`, `W_elem`, and `f_elem` parity.

2. `task2h` / AC-2 is currently regressed. [preprocessor.cpp:730](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L730) increments `mesh_out.connect[*].neigh_vert` before calling [io.cpp:566](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/io.cpp#L566), but [io.cpp:580](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/io.cpp#L580) already performs the 0-based to 1-based conversion when writing `nano_Mesh.dat`. The double shift produces the systematic `+1` `neigh_vert` mismatches now visible in the four failing archived preprocessor-oracle tests. The tracker’s `task2h` completion claim was therefore invalid; I reopened it in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md).

3. The round summary defers plan-required work instead of completing it. `task3a`, `task3c`, `task3f`, and all of Milestone 4 remain pending, while the simulator is still a stub at [main.cpp:1](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1). Because AC-7 requires an end-to-end serial compression run at [plan.md:65](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L65), the round did not actually achieve the planned mainline progression beyond partial kernel hardening.

## Blocking Side Issues

1. Runtime vdW/self-contact is still absent. The tracker already records this, and nothing in the current `src/` tree implements the simulator-side kernel from `vdw_modules.f90`; current code only has preprocessor support. AC-8 remains blocked until the runtime path exists.

## Queued Side Issues

1. [AGENT.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/AGENT.md) is still missing and [translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md) is still missing; AC-13 remains partial but this should stay queued behind simulator functionality.

## Goal Alignment Check

- AC-1: met.
- AC-2: regressed; four archived preprocessor-oracle tests now fail because `nano_Mesh.dat` connectivity is wrong.
- AC-3: met.
- AC-4: met based on committed ghost-coordinate artifacts and focused coverage.
- AC-5: partial; standalone Brenner coverage exists, but no Fortran-backed element-energy path.
- AC-6: met.
- AC-7: not met; `task3e` is not fully closed and the simulator remains a stub.
- AC-8: partial; preprocessor-side coverage exists, runtime vdW/self-contact does not.
- AC-9: not met.
- AC-10: not met.
- AC-11: partial; MPI wrapper exists, solver path does not.
- AC-12: not met.
- AC-13: partial; documentation files are missing.

No original-plan tasks are forgotten from the tracker, but the summary’s deferred list is not justified. I updated the mutable section in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md) to reopen `task2h` and `task3e`, and to record the preprocessor regression as a blocking side issue.

## Required Implementation Plan

1. Fix the reopened AC-2 regression first. Delete the manual `neigh_vert += 1` loop in [preprocessor.cpp:732](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L732) and let [io.cpp:580](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/io.cpp#L580) remain the only index-conversion point. Re-run `./build/integration_tests --gtest_filter=PreprocessorOracle.ArchivedCompressionCaseMatchesOracle:PreprocessorOracle.ArchivedCyclicPreproInputMatchesOracleOutputs:PreprocessorOracle.ArchivedSelfContactPreproInputMatchesOracleOutputs:PreprocessorOracle.ArchivedBilayerTwistVdw1000PreproInputMatchesOracleOutputs:RoundTrip.Mesh` and do not re-close `task2h` until all five pass.

2. Re-close `task3e` with direct canonical evidence, not inference. Extend [dump_element_energy_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_element_energy_oracle.f90) to emit per-Gauss `flag_num_diff`, `S_n`, and `S_m`, then add a second committed case that is synthetic-but-Fortran-derived and forces repeated principal curvatures so `ener_elem.f90` actually executes lines 60-84. Update [test_element_energy.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_energy.cpp) to assert direct stress parity plus `W_elem` and `f_elem` for both the analytical and numerical-differentiation cases.

3. Finish the remaining Milestone 3 tasks before claiming kernel completion. Add direct Fortran fixtures for [exponential.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/exponential.cpp), [principal.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/principal.cpp), and the Brenner-through-element-energy path so `task3a`, `task3c`, and `task3f` can close on production-path evidence instead of standalone synthetic coverage.

4. Replace the simulator stub with the real Milestone 4 path. Add assembly, solver, load-controller, pasapas, and reaction-force modules under `include/fce/` and `src/core/`; wire them through [main.cpp:1](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) so the executable can load archived `nano_*.dat`, assemble energy and forces with `compute_element_energy`, run translated L-BFGS, march the `nCodeLoad=3` compression steps, and emit reactions.

5. Add the AC-7 oracle harness immediately after the solver exists. Compare stepwise energy, step-50 reaction force, and step-25 displacements against the archived compression simulator outputs, and fail fast on NaN, Inf, or corrupted mesh input.

6. Then execute Milestones 5-8 in plan order without further deferral: VTU output and validation, runtime vdW/self-contact, cyclic loading plus crease memory and checkpoint/restart, MPI consistency, and the missing documentation files.

## Verification

- `./build/unit_tests` -> 59/59 passed.
- `./build/integration_tests` -> 14 passed, 4 failed: `PreprocessorOracle.ArchivedCompressionCaseMatchesOracle`, `PreprocessorOracle.ArchivedCyclicPreproInputMatchesOracleOutputs`, `PreprocessorOracle.ArchivedSelfContactPreproInputMatchesOracleOutputs`, and `PreprocessorOracle.ArchivedBilayerTwistVdw1000PreproInputMatchesOracleOutputs`.
- The new Fortran oracle reproduction path in [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/element_energy_oracle/build_provenance.md) was rerun locally with `gfortran`; it reproduces the committed [case_01.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/element_energy_oracle/archived_compression_np1/case_01.dat) fixture exactly.
