Implemented a production-path fix for the AC-7 hidden-state blocker and replaced the ambiguous local-oracle dependency with committed archived fixtures.

What changed:
- Restored the canonical Fortran `minimize_free` / `minimize` outer-loop stop on `IFLAG=1` when the previous `GNORM < EPS` in [`src/core/lbfgs.cpp`](src/core/lbfgs.cpp). This is the behavior the pinned `7d3f77f` Fortran runtime actually takes on the archived compression case.
- Added explicit bookkeeping in [`include/fce/lbfgs.hpp`](include/fce/lbfgs.hpp) so the solver reports when it exited through that trial-point `GNORM` gate.
- Updated [`src/core/solver.cpp`](src/core/solver.cpp) to preserve the last evaluated assembly/`eta` on that Fortran-style early exit instead of forcing a post-exit reassembly the canonical runtime never performs.
- Added a committed canonical hidden-state fixture at [`test/cases/graphene_compression_simulator/post_minimize_free_coords.dat`](test/cases/graphene_compression_simulator/post_minimize_free_coords.dat), captured from a temporary build of the pinned `7d3f77f` Fortran source with a post-`minimize_free` dump.
- Added `E2ECompression.CrunchItPostMinimizeFreeStateMatchesCanonicalFortranDump` in [`test/integration/test_e2e_compression.cpp`](test/integration/test_e2e_compression.cpp) to compare runtime `mesh_config_0000.vtu` coordinates directly against that dump.
- Reworked `E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace` so it no longer reads mutable ignored `test/cases/graphene_compression_simulator/np1/energy.dat` and `force.dat`; it now checks against committed archived step-1 values.

Key evidence:
- Before the solver fix, the C++ step-0 VTU stayed essentially flat while the canonical Fortran post-free dump had large out-of-plane motion; the max nodal coordinate error was about `1.0468e-1`.
- After the fix, the runtime post-free coordinates match the canonical dump within about `1.06e-7` max absolute error and `7.81e-9` mean absolute error over all nodal coordinates.
- The step-1 executable-path gate is still red even with the hidden state corrected and the archived imperfection trace replayed. Current runtime step 1 is `1.69896783e-04` vs archived `5.7210528e-05`, and the force row remains mismatched.

Validation run this round:
- `./build/integration_tests --gtest_filter=E2ECompression.CrunchItPostMinimizeFreeStateMatchesCanonicalFortranDump` ✅
- `./build/integration_tests --gtest_filter=E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace` ❌
  - energy relative error: `1.9696768923370185`
  - GNORM relative error: `0.07258842380781792`
  - force relative errors: col1 `1.9696561849993883`, col2 `0.7141074833454957`, col3 `0.18183850697794815`

## Goal Tracker Update Request

### Requested Changes:
- Update `task4d` notes to record that the canonical `7d3f77f` Fortran runtime really does exit `minimize_free` through the outer-loop `IFLAG=1` + `GNORM < EPS` gate on the archived compression case, and that reproducing that behavior fixes the post-free coordinate mismatch.
- Add to `task4d` evidence that the new executable-path hidden-state regression now passes against committed `post_minimize_free_coords.dat` with max absolute coordinate error about `1.06e-7`.
- Add to Open Issues that AC-7 is no longer blocked by the hidden post-free coordinate state; the remaining deterministic mismatch is now downstream in the traced step-1 constrained path / reaction output after the corrected free-state handoff.

### Justification:
These tracker updates reflect verified production behavior, not speculation. The main Round 2 blocker has been narrowed from “free-state mismatch” to “post-free handoff fixed, step-1 constrained/runtime semantics still wrong,” which materially changes the next debugging target while keeping AC-7 honestly open.
