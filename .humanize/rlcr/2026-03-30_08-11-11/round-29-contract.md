# Round 29 Contract

## Mainline Objective

Upgrade the new `task4a` assembly slice from a proof-of-concept test to file-backed oracle verification against the archived simulator outputs.

This round will keep the production assembly code focused on Milestone 4 Phase A and close the most actionable Round 28 review gap by:
- comparing assembled energy against the archived `energy.dat` file rather than a hard-coded literal,
- validating the archived VTU trajectory over all available compression steps,
- and adding the required corrupted-mesh negative coverage against `nano_Mesh.dat`.

## Target Acceptance Criteria

- **AC-7**: strengthen the archived compression oracle coverage for the existing assembly slice.
- **AC-11**: keep the split-range partitioned assembly checks green while broadening the oracle surface.

## Blocking Issues

- None block this round's verification objective. The missing runtime solver modules and the `task3f`
  repeated-curvature principal fixture remain real gaps, but they do not prevent stronger oracle
  validation of the already-landed `task4a` assembly path.

## Queued / Explicitly Out Of Scope

- `task3f` repeated-curvature `flag_num_diff=true` Fortran-backed principal fixture work.
- Milestone 4 Phases B-F (`task4b` through `task4f`): L-BFGS, runtime load controller, pasapas,
  reaction force, and the full 50-step solver driver.
- VTU writer, runtime vdW/self-contact, cyclic/crease/checkpoint, MPI rank-consistency, and
  documentation milestones.

## Success Criteria

1. Add failing tests first for file-backed `energy.dat` trajectory checks and corrupted `nano_Mesh.dat` rejection.
2. Replace the hard-coded energy literal in simulator assembly tests with parsed oracle data.
3. Verify the archived VTU sweep against the archived `energy.dat` rows within AC-7 tolerance.
4. Add a negative test that corrupts `nano_Mesh.dat` and fails before returning an assembled result.
5. Keep the simulator assembly slice, full unit suite, and full integration suite green after the changes.
