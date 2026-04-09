# Round 1 Summary

## What I changed

- Replaced the old synthetic replay-fixture contract with a Fortran-backed one in the integration harness:
  - `test/integration/test_e2e_compression.cpp`
  - mainline archived-oracle tests now install `test/cases/graphene_compression_simulator/imperfection_trace_fortran.dat`
  - added `CompressionCaseFiles.ArchivedFortranImperfectionTraceFixtureIsNonSynthetic` so the fixture cannot silently revert to the old constant-placeholder shape
- Added the frozen Fortran-backed imperfection trace fixture:
  - `test/cases/graphene_compression_simulator/imperfection_trace_fortran.dat`
- Added provenance notes for that fixture:
  - `test/cases/graphene_compression_simulator/build_provenance.md`
  - `test/cases/README.md`
  - `document/translation_notes.md`

## Fortran trace capture evidence

- Confirmed from the canonical Fortran source that `pasapas.f90` does:
  - `call random_seed()`
  - `call random_number(a)`
  - then applies the same scalar `a` to all real-node coordinates for that load step
- Built an instrumented temporary copy of `grapheneCompressionOriginVersion/pasapas.f90` in `/tmp` and appended `(iload, a)` to `imperfection_trace.dat` on rank 0
- Ran the temporary serial Fortran runtime against the archived `np1/nano_*.dat` inputs until all 50 load-step scalars were emitted
- This immediately proved the old helper was invalid:
  - old fixture: `imperfection_trace_cpp.dat` was 50 lines of `1.0`
  - real Fortran runtime draw for step 1 from the instrumented probe: `0.7562743988536986E+00`

## Verification

- `cmake --build build --target integration_tests`
  - PASS
- `ctest --test-dir build --output-on-failure -R '^CompressionCaseFiles\.ArchivedFortranImperfectionTraceFixtureIsNonSynthetic$'`
  - PASS
- `ctest --test-dir build --output-on-failure -R '^E2ECompression\.CrunchItWritesRuntimeVtuSeriesAndValidatesFullDataArrays$'`
  - PASS in `43.87 sec`
- `ctest --test-dir build --output-on-failure -R '^E2ECompression\.CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts$'`
  - FAIL: timeout at `180.01 sec`

## Runtime-parity probe after the trace swap

- Ran a direct executable probe with the new Fortran-backed trace:
  - `build/crunch_it <temp_case> 3`
- First three positive-load energy rows became:
  - `2.00000e-02 4.45188260e-05`
  - `4.00000e-02 6.62847584e-05`
  - `6.00000e-02 8.21258156e-05`
- Archived oracle rows are still:
  - `2.00000e-02 5.7210528e-05`
  - `4.00000e-02 1.0373979e-04`
  - `6.00000e-02 5.9198200e-05`
- So the solver trajectory changed materially once the synthetic helper was removed, but it still does not follow the archived Fortran oracle and the full 50-step executable-path test remains red.

## BitLesson Delta

- Action: none
- Lesson ID(s): NONE
- Notes: re-read `.humanize/bitlesson.md`; the first selector call had bad arguments, the corrected selector call timed out after 120 seconds, so there was no usable lesson selection for this round.

## Goal Tracker Update Request

### Requested Changes:
- Mark the blocking issue about synthetic executable-path replay as resolved or superseded:
  - replace “Deterministic executable-path replay still depends on a synthetic C++ imperfection trace fixture” with a note that the harness now uses a frozen Fortran-backed trace fixture captured from the `pasapas.f90` imperfection slot
- Add to the AC-7 blocker note that swapping in the Fortran-backed trace changed the early runtime energy trajectory but did not close parity:
  - direct 3-step probe rows are `4.45188260e-05`, `6.62847584e-05`, `8.21258156e-05`
  - archived oracle rows remain `5.7210528e-05`, `1.0373979e-04`, `5.9198200e-05`
- Keep `task4d`-`task4f` and `task8d` pending

### Justification:

This round closed the specific provenance defect called out by Codex review: the archived executable-path harness no longer relies on an obviously synthetic all-ones C++ helper. The mainline AC-7 red test is still red, so runtime load-step semantics, minimization trajectory, and/or reaction/output parity still need deeper work; the tracker should reflect that the randomness blocker is removed while the solver-trajectory blocker remains.
