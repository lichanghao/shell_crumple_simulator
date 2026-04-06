# Round 32 Summary

## Work Completed
- Re-read the plan, goal tracker, Round 31 summary/review, and wrote `round-32-contract.md` with the AC-7 executable-path objective.
- Reworked the runtime solver state so live `eta` travels with coordinates instead of being reread from `input.initial_config.eta` on every assembly:
  - added `fce::RuntimeState { coords, eta }`
  - added `make_runtime_state(input)`
  - threaded stateful overloads through `assemble_energy_forces()`, `minimize_free()`, `minimize_constrained()`, and `pasapas()`
  - propagated the converged per-element `eta` returned by `compute_element_energy()` back into the live runtime state and into `nano_final_config.dat`
- Fixed the executable contract in `src/simulator/main.cpp`:
  - positional second argument is now optional `nloadstep`
  - archived-VTU single-step assembly stays behind explicit `--single-step <step>`
  - `crunch_it <case_dir> 50` now runs the real pasapas solver path
- Extended runtime artifact generation in `pasapas()`:
  - truncates/regenerates fresh `energy.dat` and `force.dat`
  - writes `output.dat`
  - writes `nano_final_config.dat` at the end of the solve
  - supports `iload_stop` so executable-path tests can request an exact step count
- Replaced the Round 31 self-oracle AC-7 test with an archived Fortran oracle regression:
  - creates a writable temporary copy of `test/cases/graphene_compression_simulator/np1`
  - deletes copied runtime outputs first
  - executes the real `crunch_it` binary via `CRUNCH_IT_BIN`
  - requires `energy.dat`, `force.dat`, `output.dat`, and `nano_final_config.dat`
  - compares energy, reaction-force, and final-config coordinate/eta outputs against the archived `np1` oracle
- Restored the L-BFGS stale-gradient fix in `src/core/lbfgs.cpp` after noticing an in-tree diff had regressed the old premature-exit behavior.
- Investigated the remaining solver mismatch against the local canonical Fortran source under `../finite_crystal_elasticity/grapheneCompressionOriginVersion/`:
  - confirmed `crunch_it <case_dir> 50` should go through full `pasapas`
  - confirmed canonical `write_config` writes eta into `nano_final_config.dat`
  - confirmed the archived `np1` case has `nW_hat = 0`, so it is not a direct live-eta oracle by itself
- Tested and then reverted a deterministic imperfection-port experiment after it made the archived-oracle mismatch materially worse.

## Files Changed
- `CMakeLists.txt`
- `include/fce/simulator.hpp`
- `include/fce/solver.hpp`
- `src/core/lbfgs.cpp`
- `src/core/simulator.cpp`
- `src/core/solver.cpp`
- `src/simulator/main.cpp`
- `test/integration/test_e2e_compression.cpp`
- `.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md`
- `.humanize/rlcr/2026-03-30_08-11-11/round-32-summary.md`

## Validation
- `cmake --build build --target unit_tests integration_tests crunch_it -j4`
  - PASS
- `./build/unit_tests --gtest_filter='Lbfgs*'`
  - PASS (`4/4`)
- `./build/integration_tests --gtest_filter='E2ECompression.CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts'`
  - FAIL after `1169160 ms` (~19.5 min), but now fails for the correct reason on the real executable path:
    - runtime artifacts are generated
    - archived-oracle comparison runs end-to-end
    - energy mismatch starts immediately after load step 1
    - example mismatches from the test output:
      - step 1 energy: actual `5.79788888e-05` vs oracle `5.72105277e-05`
      - step 2 energy: actual `5.91932754e-05` vs oracle `1.03739788e-04`
      - widespread final-config coordinate errors exceed the `1e-3` relative tolerance
- Manual short-run probe:
  - `./build/crunch_it <temp_copy>/np1 3`
  - confirmed fresh runtime files are created and step-0 energy is again near zero (`7.68087041e-25`)

## Remaining Items
- `task4d` remains open: the solver now carries live `eta`, but the load-step trajectory still diverges from the archived Fortran runtime after the first increment.
- `task4e` remains open: torque parity from `get_reac.f90` is still untranslated, and the reaction outputs are not yet oracle-clean because the larger runtime mismatch remains.
- `task4f` remains open: the executable-path regression is now the correct archived-oracle test, but it is still red.
- Milestone 5 onward (`task5a`-`task8d`) remains out of scope for this round and still pending.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable project lesson was added this round. The main result was converting the AC-7 harness from a false-green self-oracle into an honest archived-oracle executable regression.
