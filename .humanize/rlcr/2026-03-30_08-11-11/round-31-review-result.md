# Round 31 Review

Mainline Progress Verdict: ADVANCED

Goal Alignment Summary:
`ACs: 2/13 addressed | Forgotten items: 0 | Unjustified deferrals: 5`

## Mainline Gaps

1. AC-7 is not closed because the new end-to-end check validates the solver against a self-generated C++ oracle instead of the archived Fortran oracle required by the plan. The test says this explicitly in `test/integration/test_e2e_compression.cpp:27-31`, then compares only against `graphene_compression_simulator/np1_cpp_oracle` at `test/integration/test_e2e_compression.cpp:88-126`. The committed `np1_cpp_oracle` data materially diverges from the archived Fortran `np1` trajectory across most load steps and reactions, so the tracker’s Round 31 AC-7 closure was an overclaim.

2. The pasapas path is not carrying the converged inner relaxation state forward. `assemble_energy_forces()` still feeds every element the immutable `input.initial_config.eta` at `src/core/simulator.cpp:166-178`, even though `compute_element_energy()` returns updated per-Gauss `eta`. That means each line-search evaluation and each load step restarts from the initial inner state instead of the previous converged state, which breaks the Fortran solver semantics and reopens `task4d`.

3. The required runtime CLI and output contract is still incomplete. `src/simulator/main.cpp:49-56` treats a positional second argument as legacy single-step mode, and `src/simulator/main.cpp:61-71` proves `./build/crunch_it <case_dir> 50` still does a single archived-VTU assembly instead of a 50-step solve. I confirmed that with `./build/crunch_it test/cases/graphene_compression_simulator/np1 50`, which printed only `assembled_energy`, `inner_fail`, and `force_dofs`. On top of that, `src/core/solver.cpp:303-381` writes only `energy.dat` and `force.dat`; it never writes `output.dat` or `nano_final_config.dat`, both of which were explicitly required in the Round 31 prompt.

4. `task4e` is still overclaimed. The only translated reaction path is the side-wise force sum in `src/core/load_controller.cpp:120-149`. The tracker says “reaction force and torque computation” is complete, but no torque computation from `get_reac.f90` was translated, so the task should stay open until the missing path is implemented or the scope is narrowed with justification.

## Blocking Side Issues

1. The new AC-7 integration test is not reproducible in the current environment. `test/integration/test_e2e_compression.cpp:60-69` uses `std::tmpnam()` and `std::system("cp -r ...")`; running `./build/integration_tests --gtest_filter='E2ECompression.EnergyAndForceMatchOracle'` failed here because the copy target under `/var/tmp` was not permitted. Even if the self-oracle problem were ignored, this test is not a stable verification artifact yet.

## Queued Side Issues

1. Round 31 usefully closed the direct `flag_num_diff=true` principal-kernel fixture gap, but AC-9 still lacks a committed cyclic-state provenance fixture for the repeated-curvature fallback branch. That should be revisited together with `task7a` through `task7e`, not instead of the reopened AC-7 runtime work.

## Goal Alignment Check

- AC-1 through AC-6: maintained. No regressions found in the targeted legacy tests I reran.
- AC-7: advanced, but not closed. `task4b` and `task4c` are real progress; `task4d` through `task4f` remain incomplete for the reasons above.
- AC-8: ignored this round.
- AC-9: slight progress from `task3f` only. The cyclic controller, crease memory, and checkpoint stack remain untouched.
- AC-10: ignored this round.
- AC-11: maintained only. No new multi-rank solver verification landed.
- AC-12: ignored this round.
- AC-13: ignored this round.
- Forgotten items: none. All original-plan tasks still exist in the tracker.
- Deferred items: the summary’s “Remaining Items” section is not justified as a completion claim. `task5a` through `task8d` remain mandatory plan work and must be finished, not treated as acceptable leftovers.
- Plan evolution: the Round 31 tracker entry that closed AC-7 was invalid. I corrected the mutable tracker section and reopened `task4d`, `task4e`, and `task4f`.

## Required Implementation Plan

1. Repair the runtime state model before touching more oracle data. Introduce a mutable solver-state object that owns both nodal coordinates and the current `eta` field, thread it through `assemble_energy_forces()`, `minimize_free()`, `minimize_constrained()`, and `pasapas()`, and update the stored per-element `eta` from `ElementEnergyResult::eta` after every successful assembly/minimization pass. Do not continue using `input.initial_config.eta` as the live runtime state.

2. Fix the executable contract. Keep archived-VTU assembly behind an explicit `--single-step` flag only, and make `./build/crunch_it <case_dir> 50` run the actual 50-step solver path. On fresh runs, truncate and regenerate `energy.dat`, `force.dat`, `output.dat`, and `nano_final_config.dat`; do not rely on stale copied artifacts already present in the case directory.

3. Replace the self-oracle AC-7 test with real Fortran-oracle verification. Run the executable or the exact executable-path entrypoint in a temporary writable directory, compare produced `energy.dat` and `force.dat` against `test/cases/graphene_compression_simulator/np1/`, and add the missing nodal/final-config parity check required by AC-7. Remove or demote `np1_cpp_oracle` so it cannot be mistaken for acceptance evidence.

4. Finish `task4e` honestly. Translate the remaining `get_reac.f90` logic needed for torque/reaction parity, or explicitly narrow the task scope in the tracker with source-backed justification. Do not leave the tracker claiming full parity when only the force-summing slice exists.

5. Complete Milestone 5 immediately after AC-7 is truly closed. Translate `paraview_vtu_output.f90`, write VTU/PVD outputs during the runtime solve, and add validation that the generated XML is readable and that the nodal/element fields match the solver state.

6. Complete Milestone 6 next. Translate the simulator-side vdW and self-contact code from `vdw_modules.f90`, integrate it into global assembly, and verify against the archived self-contact and bilayer oracle cases rather than helper-generated fixtures.

7. Complete Milestone 7 after vdW is in place. Implement cyclic `nCodeLoad=30/31` control, L-BFGS history reset at phase transitions, crease-memory updates, checkpoint read/write for `x0`, `eta`, and `K0_ref`, and the cyclic/restart oracle tests against `graphene_cyclic_crumple`.

8. Complete Milestone 8 last. Add np=1/2/4 solver-consistency tests, checkpoint rank-count mismatch handling, create `AGENT.md` and `document/translation_notes.md`, and then run the final full-suite acceptance audit across all ACs.

## Verification Evidence

- `./build/unit_tests --gtest_filter='Lbfgs*:Principal.FlagNumDiffMatchesFortranOracle:SimulatorAssembly.*'` passed.
- `./build/integration_tests --gtest_filter='E2ECompression.EnergyAndForceMatchOracle'` failed in this environment because the test’s `tmpnam`/`cp -r` setup could not create its temp copy target.
- `./build/crunch_it test/cases/graphene_compression_simulator/np1 50` still executed legacy single-step assembly mode and printed only `assembled_energy`, `inner_fail`, and `force_dofs`.

## Goal Tracker Update

Updated `.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md` to:
- add a Round 31 review plan-evolution entry,
- reopen `task4d`, `task4e`, and `task4f`,
- remove the unverified AC-7 completion evidence row,
- record the runtime-`eta` and CLI/oracle issues as blocking,
- record the cyclic `flag_num_diff` provenance refinement as queued.
