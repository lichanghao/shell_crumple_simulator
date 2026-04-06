# Round 3 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 6/13 addressed | Forgotten items: 2 | Unjustified deferrals: 0`

## Findings

1. The original plan is still far from complete; Round 3 only tightened Milestone 2 preprocessor parity while Milestones 3-8 remain unimplemented.
Evidence:
- `src/simulator/main.cpp:1-11` is still a stub that only prints `FCE Simulator (C++17) — stub`.
- `rg --files src include` still shows only preprocessor-side modules (`bspline`, `ghost_nodes`, `io`, `load_pre`, `mesh_generator`, `mpi_env`, `preprocessor`, `quadrature`, `reference_config`) and no constitutive, solver, cyclic runtime, checkpoint, vdW kernel, or VTU implementation files.
- `AGENT.md` and `document/translation_notes.md` are still absent.
Impact:
- AC-5 through AC-12 remain unimplemented.
- AC-13 remains partial.
Required action:
- Resume the original plan from Milestone 3 onward and complete it end to end. Do not present the untouched milestones as future work.

2. Round 3 did fix the specific archived cyclic preprocessor parity blocker, but `task2g` and AC-8 are still not implemented.
Evidence:
- The new cyclic oracle test is real: `test/integration/test_prepro_oracle.cpp:46-60` now compares the generated cyclic outputs against `test/cases/graphene_cyclic_crumple/prepro_run/`, and `ctest --test-dir build --output-on-failure -R '^PreprocessorOracle'` passes both oracle tests.
- The code explicitly documents the remaining gap: `src/core/preprocessor.cpp:190-202` hard-codes preserved per-element spans (`47` and `50`) for `nano_tub_loc.dat`, and `src/core/preprocessor.cpp:628-637` still writes that file from those preserved constants rather than from translated vdW preprocessing state.
Impact:
- The archived disabled-vdW cyclic baseline now matches, so the old tracker blocker about missing `nano_crease.dat` / mismatched `nano_tub_loc.dat` was stale.
- AC-2's `nvdw=1` positive path and all of AC-8 remain open because there is still no real neighbor-list, shape-function, or `vdw_previous`-equivalent preprocessing.
Required action:
- Keep `task2g` pending.
- Replace the preserved-span bridge with the actual vdW preprocessing pipeline and add `nvdw=1` oracle coverage before claiming AC-8 or full AC-2 completion.

3. The required AC-2 invalid-input negative case is still missing, and the preprocessor still accepts invalid chirality input.
Evidence:
- `src/core/preprocessor.cpp:217-229` computes `theta` directly from `nchir`, `xn1`, and `xn2` with no validation.
- I re-ran `build/PrePro` on a copy of the archived compression `data.dat` edited to `xn1=xn2=0` and `nchir=0`; it exited `0` and wrote `NAN` bond vectors into `nano_general.dat`.
- `test/integration/test_prepro_oracle.cpp:32-60` still contains only the two positive oracle tests; there is no negative invalid-input regression.
Impact:
- AC-2 remains partial.
- Claude's summary correctly admits this is still pending, so this is an incompleteness issue rather than a false completion claim.
Required action:
- Validate `nchir` / chirality-index input before computing `theta`, raise a clear error on invalid input, and add the required negative integration test.

4. AC-2 negative coverage is still incomplete because there is no regression that deliberately corrupts generated mesh connectivity and proves the comparator rejects it.
Evidence:
- `test/support/oracle_compare.cpp:220-246` is capable of catching connectivity mismatches.
- `test/integration/test_prepro_oracle.cpp:32-60` never mutates generated output or asserts a comparator failure on a corrupted mesh.
Impact:
- One of AC-2's required negative checks is still unimplemented.
- The tracker had not been surfacing this gap explicitly enough.
Required action:
- Add a negative integration test that swaps connectivity in a generated `nano_Mesh.dat` and asserts `compare_preprocessor_outputs(...)` fails with a clear mismatch.

5. AC-3 is still only self-consistency-tested; the required oracle fixtures and out-of-domain rejection are still missing.
Evidence:
- `src/core/bspline.cpp:8-75` evaluates the closed-form polynomials directly for any `(v,w)` and contains no valid-domain guard.
- `test/unit/test_bspline.cpp:20-99` only checks partition-of-unity and finite-difference consistency at representative points; there are no archived Fortran fixture comparisons and no out-of-domain negative case.
Impact:
- AC-3 remains partial.
- The current implementation could silently extrapolate outside the valid parameter domain.
Required action:
- Add the required 5 interior and 5 boundary oracle fixtures, reject invalid `(v,w)` outside the element parameter domain, and add the negative test.

6. AC-4 still lacks the required negative anchor-node regression.
Evidence:
- `test/unit/test_ghost_nodes.cpp:6-33` only exercises the positive parallelogram extrapolation case.
- There is still no test that deliberately swaps the anchor node and asserts the 1×10⁻¹² tolerance check fails.
Impact:
- AC-4 remains partial even though the positive oracle path passes.
- This gap was not being tracked clearly enough.
Required action:
- Add the wrong-anchor negative regression and keep AC-4 marked partial until it exists.

## Goal Alignment Check

- AC-1: complete.
- AC-2: partial. Both archived preprocessor oracle baselines now pass, but invalid-input rejection, the corrupted-mesh negative case, and the real `nvdw=1` preprocessing path are still missing.
- AC-3: partial. Self-consistency checks exist, but oracle fixtures and the out-of-domain negative case are still absent.
- AC-4: partial. Positive ghost-node parity exists, but the negative anchor-node regression is still missing.
- AC-5: not addressed.
- AC-6: not addressed.
- AC-7: not addressed.
- AC-8: not addressed in substance. The Round 3 `nano_tub_loc.dat` change is an archived-oracle bridge, not the vdW preprocessing implementation required by the plan.
- AC-9: not addressed in substance. Preprocessor-side cyclic artifact parity improved, but there is still no cyclic runtime controller, `K0_ref` update logic, or `crease_map.dat` generation.
- AC-10: not addressed.
- AC-11: partial. MPI scaffolding exists, but solver equivalence and checkpoint-rank checks have not started.
- AC-12: not addressed.
- AC-13: partial. CMake/infrastructure exists, but `AGENT.md` and `document/translation_notes.md` are still missing.

Tracker assessment:
- Claude's Round 3 goal-tracker update request is approved in part. I updated `goal-tracker.md` to remove the stale cyclic-parity blocker, record the passing archived cyclic oracle test, and keep `task2g` pending because the fix is still a disabled-vdW compatibility bridge rather than real vdW preprocessing.
- I also added the still-missing AC-2 corrupted-mesh negative and AC-4 wrong-anchor negative gaps to the tracker, because they were not being surfaced clearly enough.
- There are no explicit deferrals in the tracker, but the implementation is still materially incomplete relative to the original plan.

## Verification Performed

- `ctest --test-dir build --output-on-failure -R '^PreprocessorOracle'` → passed both archived preprocessor oracle tests.
- `ctest --test-dir build --output-on-failure` → passed `27/27` tests.
- Manual invalid-chirality probe: edited the archived compression `data.dat` to `xn1=xn2=0`, `nchir=0`; `build/PrePro` exited `0` and emitted `NAN` bond vectors in `nano_general.dat`.

## Directive Implementation Plan

1. Finish Milestone 2 completely before claiming further preprocessor completion.
   - Validate chirality input before `theta` is computed and add the invalid-input negative regression.
   - Add the corrupted-mesh negative integration test that proves the comparator fails on swapped connectivity.
   - Add the AC-3 oracle fixture suite and reject out-of-domain `(v,w)` in the B-spline evaluators.
   - Add the AC-4 wrong-anchor negative regression.
   - Port the actual `vdw_previous` preprocessing pipeline and remove the preserved-span `nano_tub_loc.dat` bridge.
   - Add `nvdw=1` preprocessor oracle coverage so AC-2 and AC-8 can advance on the real path, not only on archived disabled-vdW compatibility.

2. Implement Milestone 3 exactly as written in `document/plan.md`.
   - Add the exponential Cauchy-Born map, deformation-gradient decomposition, Brenner REBO potential, inner Newton solver, element energy/force kernel, and principal-curvature extraction.
   - Back each kernel with oracle or finite-difference tests that satisfy AC-5 and AC-6 numerically.

3. Replace the simulator stub with the Milestone 4 solver pipeline.
   - Implement MPI-aware assembly, the translated `lbfgs.f` algorithm, the loading controller, the `pasapas` loop, and reaction-force computation.
   - Add the serial compression oracle test and verify AC-7 against the archived simulator outputs.

4. Implement Milestone 5 VTU output.
   - Translate the ParaView writer and add the XML-validity and state-consistency tests required for AC-12.

5. Implement Milestone 6 vdW/self-contact runtime behavior.
   - Add the Lennard-Jones kernel, self-contact filtering, spatial binning, and solver integration.
   - Compare vdW energy, forces, and neighbor lists against the archived oracle to satisfy AC-8.

6. Implement Milestone 7 cyclic runtime and checkpoint/restart.
   - Add the cyclic BC controller, phase transitions, L-BFGS history resets, crease-memory updates, crease analysis output, and checkpoint serialization/restoration.
   - Add oracle-backed cyclic and restart tests to satisfy AC-9 and AC-10.

7. Finish Milestone 8 verification and documentation.
   - Add `np=1/2/4` consistency tests and checkpoint rank-compatibility checks for AC-11.
   - Create `AGENT.md` and `document/translation_notes.md`, then keep them updated with verification evidence as the remaining milestones land.
   - Complete the full integration test suite and keep the tracker aligned with actual verified progress.
