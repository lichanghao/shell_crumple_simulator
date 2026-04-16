# Round 8 Summary

## Work Completed

- Added focused solver coverage for the `stop_on_first_trial` branch:
  - `LbfgsStopOnFirstTrial.OnlyTerminatesWhenEnabled`
  - This distinguishes “first trial reached” from “first trial permitted to terminate” at the direct `LbfgsSolver` API level.
- Updated `document/translation_notes.md` so AC-13 matches the current cyclic state:
  - the cyclic post-free oracle is now green,
  - the remaining cyclic blocker is the first constrained replay row rather than the post-`minimize_free` handoff.

## Validation

- `cmake --build build --target unit_tests -j4`
- `./build/unit_tests --gtest_filter='LbfgsStopOnFirstTrial.OnlyTerminatesWhenEnabled'`

## Remaining Items

- The primary cyclic blocker is still `E2ECyclicRuntime.CrunchItReplaysCommittedCyclicStepOneTraceDeterministically`.
- `task7b` / `task7c` remain open: no translated `update_creases`, no translated `analyse_creases`, no `crease_map.dat`.
- `task7d` remains partial: no MPI restore broadcast and no restart-parity proof yet.
- Runtime vdW, real-`nvdw=1` VTU coverage, and multi-rank acceptance coverage are still open.

## Goal Tracker Update Request

### Requested Changes:
- Update `task8c` / documentation notes only if Codex thinks any residual AC-13 mismatch remains; otherwise no tracker content change is required from this round.

### Justification:
- This round is mainly a coverage-and-alignment cleanup around the already landed Round 6 solver change, not a new functional milestone.

## BitLesson Delta

- Action: none
- Lesson ID(s): NONE
- Notes: `bitlesson-selector` remains unavailable in this environment, so no selector-driven lesson update was possible this round.
