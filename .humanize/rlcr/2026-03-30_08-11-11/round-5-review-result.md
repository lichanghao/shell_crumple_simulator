# Round 5 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 4/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. Round 5 did not close AC-4. The new ghost-coordinate block in [test/support/oracle_compare.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/support/oracle_compare.cpp#L147) reads only the real-node `nano_config.dat` payload, because [src/core/io.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/io.cpp#L240) loads exactly `numnods` coordinates and no archived ghost coordinates. It then calls the same C++ `ghost_nodes()` implementation on both the generated data and the archived oracle data in [test/support/oracle_compare.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/support/oracle_compare.cpp#L254). That is not a direct archived ghost-coordinate comparison; it is a self-consistency check over already-matched real-node inputs. The tracker request to remove the AC-4 blocker is rejected.

2. The original plan is still incomplete, and Claude explicitly tried to defer required work instead of completing it. `task2g` remains pending: [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L190) still hardcodes the archived span constants for `nano_tub_loc.dat`, [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L656) still writes that preserved bridge instead of translated vdW preprocessing output, and [include/fce/types.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/types.hpp#L105) still carries only a placeholder `VdwData`. AC-2 and AC-8 therefore remain incomplete.

3. AC-3 is still incomplete. [test/unit/test_bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_bspline.cpp#L20) contains partition-of-unity, finite-difference, and out-of-domain checks, but there are still no committed 5 interior plus 5 boundary Fortran oracle fixtures for basis values and first derivatives. Claude’s summary correctly admits this remains open; it is incomplete work and must be finished, not deferred.

4. Milestones 3 through 8 remain largely unimplemented. The simulator entry point is still a stub in [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1), and the repository still lacks the constitutive-model, solver, runtime vdW, cyclic controller, checkpoint/restart, VTU-output, and final documentation files required by the plan. AC-5 through AC-12 are therefore still not met, and AC-13 remains partial because `AGENT.md` and `document/translation_notes.md` are still absent.

## Goal Tracker Update Request Assessment

Partially approved.

- Approved: remove the queued fixed-temp-dir side issue. Commit `eef0ab9` does harden `PreprocessorOracle` temp-dir creation, and this round verified `cmake --build build --target integration_tests -j4 && ctest --test-dir build --output-on-failure -R '^PreprocessorOracle'` passes `5/5`.
- Approved: update the tracker’s Round 5 evidence to reflect the current `32/32` full-suite pass and the temp-dir harness hardening.
- Rejected: remove the AC-4 blocker or rewrite AC-4 evidence as “direct archived ghost-coordinate comparison.” The comparator still regenerates the oracle-side ghost coordinates in C++, so the evidence remains indirect.
- Rejected: any implication that Milestone 2 is fully closed. `task2g` and the AC-3 fixture gap remain open, and Milestones 3 through 8 remain active implementation work.

I updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md) accordingly.

## Goal Alignment Check

- AC-1 remains the only fully met acceptance criterion.
- AC-2, AC-3, and AC-4 have partial progress, but each still has an open blocker that prevents closure.
- AC-5 through AC-12 and AC-13 received no substantive implementation progress in Round 5.
- No plan task IDs are missing from Active/Completed/Deferred, so there are no forgotten tracker items.
- There are no formally deferred items in the tracker, but Claude’s summary still attempts to push required work into future phases. Treat those as incomplete tasks, not acceptable deferrals.

## Required Implementation Plan

Execute the remaining original plan in this order. Do not spend another round claiming milestone closure until these deliverables exist in code and tests.

1. Finish Milestone 2 completely.
   Create `include/fce/vdw_preprocess.hpp` and `src/core/vdw_preprocess.cpp` to translate the preprocessor-side `vdw_previous.f90` pipeline. Extend [include/fce/types.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/types.hpp#L103) so `VdwData` carries the real preprocessing state: `ngauss_vdw`, per-sheet spans, neighbor-pair storage, shape-function payload, and any lookup/bin data needed later by the runtime vdW kernel. Replace the preserved-span bridge in [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L190) and [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L656) so `nano_tub_loc.dat` and all vdW-related preprocessor outputs come from translated logic, not constants.

2. Add real `nvdw=1` oracle coverage for the preprocessor.
   Archive at least one Fortran `nvdw=1` preprocessor case under `test/cases/` with the corresponding `nano_*.dat` outputs needed for comparison. Extend [test/support/oracle_compare.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/support/oracle_compare.cpp#L86) and [test/integration/test_prepro_oracle.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_prepro_oracle.cpp#L59) so the integration suite compares the translated neighbor-list output, shape-function data, and `nano_tub_loc.dat` against that archived oracle. AC-2 and AC-8 stay open until this exists.

3. Close AC-3 with committed Fortran fixtures.
   Add fixture files under `test/cases/bspline_oracle/` for 5 interior and 5 boundary 12-node patch configurations, including ghost-node cases. Extend [test/unit/test_bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_bspline.cpp#L20) or add a dedicated integration test so `BSpline`, `DBSpline`, and any required derivative helpers are checked against those fixtures at absolute tolerance `1e-14`. Keep the current partition-of-unity and out-of-domain negative tests, but do not claim AC-3 until the Fortran fixtures are in place.

4. Close AC-4 with archived ghost-coordinate artifacts.
   Materialize archived Fortran ghost coordinates into a committed artifact, for example `test/cases/graphene_compression_prepro/ghost_coords.dat` and the cyclic counterpart if needed. Change [test/support/oracle_compare.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/support/oracle_compare.cpp#L254) so it reads those archived ghost coordinates directly and compares the generated ghost positions against them. Do not compute the oracle side with C++ `ghost_nodes()`. Add an end-to-end negative regression that perturbs one generated ghost coordinate or forces the wrong anchor through the preprocessor path so the archive-based comparison fails.

5. Implement the constitutive-model kernels from Milestone 3 before touching more tracker bookkeeping.
   Add `include/fce/exponential.hpp` and `src/core/exponential.cpp`, `include/fce/geometry.hpp` and `src/core/geometry.cpp`, `include/fce/brenner.hpp` and `src/core/brenner.cpp`, `include/fce/inner_newton.hpp` and `src/core/inner_newton.cpp`, `include/fce/element_energy.hpp` and `src/core/element_energy.cpp`, and `include/fce/principal_curvature.hpp` plus `src/core/principal_curvature.cpp`. Use the Fortran modules named in [document/plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L216) and [document/fortran_conventions.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/fortran_conventions.md#L226) as the exact algorithmic source. Add unit and oracle tests for AC-5 and AC-6 before integrating these kernels into the solver.

6. Implement the FEM solver core from Milestone 4.
   Add `include/fce/assembly.hpp` and `src/core/assembly.cpp`, `include/fce/lbfgs.hpp` and `src/core/lbfgs.cpp`, `include/fce/load_runtime.hpp` and `src/core/load_runtime.cpp`, `include/fce/pasapas.hpp` and `src/core/pasapas.cpp`, and `include/fce/reaction.hpp` plus `src/core/reaction.cpp`. Replace the stub in [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) with a real `crunch_it` pipeline that reads `nano_*.dat`, runs the translated load stepping, and writes `energy.dat`, `force.dat`, `output.dat`, and `nano_final_config.dat`. Add end-to-end serial oracle tests against `test/cases/graphene_compression_simulator/np1` for energy trajectory, final reaction force, and intermediate displacements.

7. Implement Milestones 5 through 7 in the same translation-first style.
   Add `include/fce/vtu_writer.hpp` and `src/core/vtu_writer.cpp` for VTU/PVD output, `include/fce/vdw_runtime.hpp` and `src/core/vdw_runtime.cpp` for runtime vdW and self-contact, `include/fce/crease.hpp` and `src/core/crease.cpp` for crease memory and analysis, and `include/fce/checkpoint.hpp` plus `src/core/checkpoint.cpp` for checkpoint/restart. Verify VTU output against the archived `.vtu`/`.pvd` corpus, verify vdW/self-contact against `nvdw=1` oracle fixtures, and verify cyclic/checkpoint behavior against `test/cases/graphene_cyclic_crumple/simulator_run`.

8. Finish Milestone 8 and the missing documentation.
   Add MPI consistency tests for `np=1`, `np=2`, and `np=4`; add restart compatibility tests across rank counts; then create root-level `AGENT.md` and `document/translation_notes.md` with the project structure, build/run instructions, design decisions, milestone evidence, and translation notes required by AC-13. Only after those tests and documents exist should the tracker move any of AC-5 through AC-13 out of the incomplete state.
