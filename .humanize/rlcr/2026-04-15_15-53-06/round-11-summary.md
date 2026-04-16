# Round 11 Summary

## Work Completed

- Generalized the root-owned checkpoint restore path so malformed checkpoint reads are now coordinated across ranks:
  - rank 0 catches `read_checkpoint()` exceptions,
  - a restore status code is broadcast before any rank throws,
  - all ranks now fail consistently with `failed to read checkpoint` instead of risking a deadlock on later collectives.
- Added a new executable multi-rank regression:
  - `E2ECyclicRuntime.CrunchItRejectsMalformedCheckpointAcrossRanks`
  - writes a deliberately malformed cyclic checkpoint,
  - runs `mpirun -np 2 crunch_it ...`,
  - asserts nonzero exit and the expected failure message.

## Files Modified

- `src/simulator/main.cpp`
- `test/integration/test_e2e_compression.cpp`
- `.humanize/rlcr/2026-04-15_15-53-06/goal-tracker.md`

## Validation

- `cmake --build build --target crunch_it integration_tests -j4`
- `./build/integration_tests --gtest_filter='E2ECyclicRuntime.CrunchItRejectsCheckpointWrittenWithDifferentRankCount:E2ECyclicRuntime.CrunchItRejectsMalformedCheckpointAcrossRanks'`

## Remaining Items

- The cyclic replay-row gate is still the primary explicit red gate on AC-9/AC-10.
- `task7b` / `task7c` remain open: no translated `update_creases`, no translated `analyse_creases`, no `crease_map.dat`.
- Restart parity coverage is still missing even though the checkpoint failure paths are now MPI-safe.
- Runtime vdW, real-`nvdw=1` VTU coverage, and multi-rank acceptance coverage are still open.

## Goal Tracker Update Request

### Requested Changes:
- Add a Plan Evolution note that Round 11 generalized checkpoint restore failure handling so malformed checkpoint reads are now coordinated across ranks as well as rank-mismatch failures.
- Update `task7d` / `task8b` notes to reflect that both incompatible-rank and malformed-checkpoint multi-rank failure paths are now covered on the real executable path, while uninterrupted-vs-restarted parity is still pending.

### Justification:
- This is concrete AC-10 / AC-11 progress on the runtime failure semantics, not just local parser coverage.
- The repository now has committed executable regressions for both of the currently implemented multi-rank checkpoint failure cases.

## BitLesson Delta

- Action: none
- Lesson ID(s): NONE
- Notes: `bitlesson-selector` remains unavailable in this environment, so no selector-driven lesson update was possible this round.
