# Round 36 Summary

## Work Completed
- Re-read the plan, goal tracker, and Round 36 review, then narrowed the mainline to the provenance issue on AC-7 replay.
- Removed the synthetic imperfection helper from the frozen archived compression case contract:
  - deleted `test/cases/graphene_compression_simulator/np1/imperfection_trace.dat`
  - added `test/cases/graphene_compression_simulator/imperfection_trace_cpp.dat` as an explicit C++ replay fixture outside the archived `np1/` case directory
  - updated `test/integration/test_e2e_compression.cpp` so the archived-oracle test now copies that fixture into the temp case directory explicitly before invoking `crunch_it`
  - updated `test/cases/README.md` so the fixture provenance is documented correctly
- Restored the accidentally overwritten archived `np1` runtime outputs after discovering that one of my earlier direct `crunch_it` probes had dirtied the frozen oracle files. The cleaned harness now touches only temp copies again.
- Re-ran the deterministic replay checks and the full 50-step archived-oracle comparison under the cleaned harness.

## Files Changed
- `test/integration/test_e2e_compression.cpp`
- `test/cases/README.md`
- `test/cases/graphene_compression_simulator/imperfection_trace_cpp.dat`
- `.humanize/rlcr/2026-03-30_08-11-11/round-36-summary.md`

## Validation
- `cmake --build build --target integration_tests -j4`
  - PASS
- `./build/integration_tests --gtest_filter='E2ECompression.CrunchItReusesRecordedImperfectionTraceDeterministically:E2ECompression.CrunchItRejectsShortImperfectionTrace'`
  - PASS (`2/2`)
- `./build/integration_tests --gtest_filter='E2ECompression.CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts'`
  - FAIL after `632533 ms` (~10.5 min), but now through the cleaned harness that injects the replay trace only into the temp copy
  - deterministic early energy trajectory remains unchanged from Round 35:
    - step 1 actual `5.74298201e-05` vs oracle `5.72105277e-05` (relative error `3.8330724722555058e-03`)
    - step 2 actual `7.47995924e-05` vs oracle `1.03739788e-04` (relative error `2.7896911686441628e-01`)
    - step 3 actual `1.44786817e-04` vs oracle `5.91982000e-05` (relative error `1.4457976256034815`)
  - full trajectory still diverges heavily; step 50 actual remains `7.77787e-04`
  - final configuration still fails badly, especially in tail z-coordinates

## Remaining Items
- The archived-case provenance issue from Round 35 is fixed, but the replay is still source-independent: the harness uses a C++ replay trace fixture rather than a Fortran-captured sequence or an exact port of Fortran RNG semantics.
- AC-7 remains open on the deterministic runtime path. The next debugging slice is still `pasapas` / minimization / reaction semantics, not replay reproducibility.
- AC-12 and Milestones 6-8 remain untouched original-plan scope.

## Goal Tracker Update Request

### Requested Changes:
- Add a Round 36 plan-evolution entry noting that the synthetic imperfection replay helper was removed from the archived `np1/` case contract and relocated to a harness-side fixture `test/cases/graphene_compression_simulator/imperfection_trace_cpp.dat`.
- Update `task4d` / `task4f` notes to record that the deterministic AC-7 failure is unchanged after the provenance cleanup:
  - the cleaned harness still produces the same early deterministic trajectory (`5.74298201e-05`, `7.47995924e-05`, `1.44786817e-04`)
  - the archived-oracle failure is therefore still a real runtime trajectory mismatch rather than an artifact of where the helper trace lives
- Keep the open blocking issue that deterministic replay still depends on a synthetic C++ trace artifact until the sequence is captured from Fortran or the exact Fortran RNG/seed behavior is ported.

### Justification:
- The tracker should distinguish between two separate concerns:
  - the archived-case contract must stay clean
  - the deterministic runtime mismatch remains the real AC-7 blocker
- Recording that the harness-only fixture produces the same failing trajectory preserves the current evidence while fixing the provenance flaw called out in review.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new project lesson was added this round. The work was a provenance cleanup of the deterministic replay harness, not a new reusable technical pattern beyond the Round 35 trace-contract lesson.
