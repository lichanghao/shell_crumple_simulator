# Round 9 Summary

## Work Completed

- Extended the checkpoint format with rank metadata:
  - `fce::io::CheckpointData` now carries `nprocs`
  - `write_checkpoint(...)` writes `checkpoint_nprocs`
  - `read_checkpoint(...)` remains backward-compatible with older checkpoint files that do not carry this field
- Updated cyclic checkpoint writing to record the writing MPI size.
- Updated `crunch_it` cyclic restart handling so:
  - rank 0 owns checkpoint file reads
  - restored `coords`, `eta`, and `K0_ref` are broadcast to all ranks
  - incompatible rank-count restarts are rejected explicitly when the checkpoint carries `nprocs`
- Added unit coverage for the new checkpoint metadata while preserving compatibility with the archived checkpoint oracle.

## Files Modified

- `include/fce/io.hpp`
- `src/core/io.cpp`
- `src/core/solver.cpp`
- `src/simulator/main.cpp`
- `test/unit/test_io.cpp`

## Validation

- `cmake --build build --target unit_tests crunch_it -j4`
- `./build/unit_tests --gtest_filter='ReadCheckpoint.GrapheneCyclicCrumple:ReadWriteCheckpoint.PreservesRankMetadata'`

## Remaining Items

- The cyclic replay-row gate is still the primary explicit red gate on AC-9/AC-10.
- `task7b` / `task7c` remain open: no translated `update_creases`, no translated `analyse_creases`, no `crease_map.dat`.
- Restart parity coverage is still missing even though root-owned restore/broadcast scaffolding now exists.
- Runtime vdW, real-`nvdw=1` VTU coverage, and multi-rank acceptance coverage are still open.

## Goal Tracker Update Request

### Requested Changes:
- Add a Plan Evolution note that Round 9 extended checkpoint semantics with writing-rank metadata and moved cyclic restore from local-process-only state to root-read plus MPI broadcast.
- Update `task7d` notes to reflect that checkpoint restart now validates stored rank metadata when present and broadcasts restored state to all ranks, while uninterrupted-vs-restarted parity tests are still missing.

### Justification:
- This is real production progress on AC-10 rather than another pure diagnostic round.
- The repository now distinguishes old checkpoints from new rank-aware checkpoints and restores cyclic state consistently across ranks.

## BitLesson Delta

- Action: none
- Lesson ID(s): NONE
- Notes: `bitlesson-selector` remains unavailable in this environment, so no selector-driven lesson update was possible this round.
