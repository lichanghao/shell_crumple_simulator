# Round 33 Contract

## Mainline Objective
Close the remaining AC-7 runtime-semantics gaps on the archived `graphene_compression_simulator/np1` executable path by matching canonical `pasapas` step ordering more closely and by adding direct regression coverage for live solver state.

## Target ACs
- AC-7: End-to-end serial compression runtime matches the archived Fortran oracle on the real executable path.
- AC-9: Preserve the new live-`eta` runtime-state plumbing with direct coverage so cyclic/checkpoint work has a sound base.

## Blocking Issues
- Canonical `pasapas.f90` applies the imperfection update before each constrained minimization when `imperfect = 1`; the current C++ runtime does not.
- Shortened executable probes currently change the physical load increment by overwriting `BCs%nloadstep` instead of stopping early on the canonical 50-step denominator.
- `task4e` is still incomplete: the archived `force.dat` oracle remains red and the current reaction path may still omit `get_reac.f90` details that matter for parity.
- The `RuntimeState` / live-`eta` change still lacks a direct test on a case that actually exercises `nW_hat = 1`.

## Queued / Out Of Scope
- Milestone 5 onward (`task5a`-`task8d`) remains pending in the overall plan, but this round will only touch them if a change is strictly required to unblock the AC-7 runtime path.

## Success Criteria
- `pasapas()` preserves the file-loaded `nloadstep` denominator, uses a separate stop limit for shortened probes, and ports the canonical imperfection step in the correct place.
- The archived executable-path AC-7 regression still runs against the real `crunch_it` binary and shows improved agreement or at least a narrower, source-backed remaining mismatch.
- A dedicated test proves live `eta` updates are persisted and reused across repeated runtime evaluations on an `nW_hat = 1` fixture.
- Round 33 summary includes a Goal Tracker Update Request instead of directly editing the tracker, per the new prompt.
