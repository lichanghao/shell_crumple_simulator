# Round 13 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 10/13 addressed | Forgotten items: 0 | Unjustified deferrals: 1`

## Findings

1. Round 13 closes the previous `nCode_Pot` compatibility blocker, but the original plan is still nowhere near complete. The shared core library still contains no C++ translations of `exponential.f90`, `geometry.f90`, `ener_elem.f90`, `principal.f90`, solver assembly, L-BFGS, runtime vdW, cyclic, checkpoint, or VTU modules in [CMakeLists.txt](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/CMakeLists.txt#L39), and the simulator target is still a stub in [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1). The tracker still lists `task3a`, `task3b`, `task3e`, `task3f`, and every Milestone 4-8 task as pending in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L74). Claude’s “Remaining Items” section is therefore still an unjustified deferral of required plan work, not completion.

2. `task3d` still does not satisfy the plan-designed AC-6 oracle provenance requirement. Round 13 correctly makes the archived compression `nano_general.dat` payload usable through the C++ reader and inner Newton path, but the committed Newton parity corpus is still documented as fixed-output fixtures regenerated from helper-defined inputs in [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md#L11), and the helper still hard-codes the 10 Newton states in [dump_constitutive_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_constitutive_oracle.f90#L177). The original plan requires those 10 AC-6 states to be drawn from archived simulator load-step outputs, not synthetic helper inputs, so this remains a substantive plan gap.

## Goal Alignment Check

- AC-1 through AC-4 remain in the same verified state as prior rounds.
- AC-5 made real integration progress this round: the C++ reader/writer now preserves the archived `nano_general.dat` code-1 Morse payload, and the constitutive layer no longer rejects committed file-backed materials.
- AC-6 also made real integration progress this round: [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L312) now accepts both `nCode_Pot=1` and `nCode_Pot=2`, and [test/unit/test_constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_constitutive.cpp#L360) proves the archived compression payload is accepted. AC-6 is still not plan-complete because the main 10-case corpus is synthetic rather than archived simulator-state provenance.
- AC-7 is still blocked because the required Milestone 3 and Milestone 4 translations are absent and the simulator executable is still a stub.
- AC-8 has no new runtime progress this round; only the earlier preprocessor-side work exists.
- AC-9 through AC-12 saw no executable implementation progress this round.
- AC-13 remains partial: `AGENT.md` and `document/translation_notes.md` are still absent.
- Forgotten items: none. The tracker still covers the original task list.
- Deferred items: not justified. Claude still treats the untouched original-plan remainder as future work even though the plan does not authorize stopping after the constitutive slice.
- Plan evolution: the Round 13 tracker evolution is justified only for recording the resolved `nCode_Pot` blocker and the fresh `52/52` verification. No broader reinterpretation of the remaining scope is justified.

## Goal Tracker Update Requests

Approved and applied.

- Updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L42) to Plan Version 15 with a Round 13 plan-evolution entry describing the verified material-potential semantics reconciliation and fresh `52/52` suite result.
- Updated the `task3c` and `task3d` notes in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L77) to reflect the resolved file-backed material-code blocker while keeping both tasks pending for the remaining Milestone 3 work.
- Removed the resolved `nCode_Pot` compatibility blocking issue from [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L102).
- Kept `task3d`, `task3a`, `task3b`, `task3e`, `task3f`, and all Milestone 4-8 tasks open because the codebase still does not implement them.

## Required Implementation Plan

Claude must execute the remaining original-plan scope in this order:

1. Finish the missing Milestone 3 kernels as dedicated modules, not by further expanding `constitutive.cpp`.
   Add `include/fce/exponential.hpp` plus `src/core/exponential.cpp` for the translated `def_bonds` and derivative path from `exponential.f90`.
   Add `include/fce/geometry.hpp` plus `src/core/geometry.cpp` for the metric, curvature, bond-vector, and derivative computations from `geometry.f90`.
   Add `include/fce/principal.hpp` plus `src/core/principal.cpp` for principal-curvature extraction from `principal.f90`.
   Add `include/fce/element_energy.hpp` plus `src/core/element_energy.cpp` for `ener_elem.f90`, including inner-failure handling and crease-curvature subtraction.
   Wire all four into [CMakeLists.txt](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/CMakeLists.txt#L39) and add focused unit files instead of continuing to grow [test/unit/test_constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_constitutive.cpp#L1).

2. Replace the synthetic AC-6 fixtures with plan-compliant archived-state fixtures.
   Instrument the frozen Fortran simulator or add a dedicated extractor so 10 representative element states are dumped from archived load-step outputs under `test/cases/constitutive_oracle/`.
   Keep explicit failure fixtures for `fail_mode=1/2/3`, but make the main AC-6 parity corpus come from archived simulator states.
   Update `test/cases/constitutive_oracle/build_provenance.md` so the provenance describes archived simulator-state extraction rather than helper-defined inputs.

3. Port the solver core and replace the simulator stub with the real compression path.
   Translate `pre_ener.f90`, `energy.f90`, `lbfgs.f`, simulator-side `load.f90`, `pasapas.f90`, and `get_reac.f90` into dedicated C++ modules under `include/fce/` and `src/core/`.
   Rewrite [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) so `crunch_it` reads the archived `nano_*.dat` files, runs the `nCodeLoad=3` load-stepping loop, and writes `energy.dat`, `force.dat`, `output.dat`, and `nano_final_config.dat`.

4. Add the missing AC-7 end-to-end simulator oracle test immediately after the solver port lands.
   Create a new integration test that runs the translated solver against `test/cases/graphene_compression_simulator/np1` and checks the energy trajectory, final reaction force, and representative nodal displacements against the archived oracle tolerances from the plan.

5. Port VTU output on top of the real solver path.
   Translate `paraview_vtu_output.f90` into a dedicated output module, emit `mesh_config_XXXX.vtu` and `mesh_config_series.pvd`, and validate them against the archived compression snapshots before claiming AC-12 progress.

6. Port runtime vdW, self-contact, cyclic loading, crease memory, checkpoint/restart, and MPI consistency after the serial solver path is working.
   Translate `vdw_modules.f90`, cyclic load control from `load.f90`, `crease.f90`, `crease_analysis.f90`, and the checkpoint logic exercised in the archived cyclic case.
   Then add `np=1/2/4` consistency tests, checkpoint rank-count compatibility tests, and cyclic-oracle comparisons against `test/cases/graphene_cyclic_crumple/simulator_run`.

7. Finish the documentation and final integration suite last, but before any completion claim.
   Create `AGENT.md`.
   Create `document/translation_notes.md`.
   Expand the integration suite so AC-1 through AC-13 are directly covered by executable checks instead of relying on milestone notes.

## Verification

- Rebuilt: `cmake --build build --target unit_tests integration_tests -j4`
- Re-ran focused Round 13 coverage: `ctest --test-dir build --output-on-failure -R 'ReadGeneral.GrapheneCompression|NewtonInner.AcceptsCommittedCompressionMaterialPayload|NewtonInner.RejectsUnsupportedPotentialCode|Brenner.DefaultMaterialUsesSupportedPotentialCode'`
- Re-ran full suite: `ctest --test-dir build --output-on-failure`
- Result: pass `52/52`

Round 13 remains incomplete.
