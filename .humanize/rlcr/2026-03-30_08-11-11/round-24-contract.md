# Round 24 Contract

## Mainline Objective

**Fully close task3e**: fix the write_mesh regression, add f_elem force-consistency
coverage, verify the flag_num_diff branch is correctly translated, and add a test
exercising the flag_num_diff=true path.

## Target ACs

- **AC-7** (partial closure): `compute_element_energy` now has force oracle coverage
  via finite-difference consistency and a flag_num_diff regression; `write_mesh` round-trip
  restored (AC-2 regression fixed).
- **AC-2**: `RoundTrip.Mesh` passes again after correcting `write_mesh()` to mirror the
  updated `read_mesh()` semantics.

## Blocking Issues (must fix before mainline target is reached)

1. **write_mesh regression** — `write_mesh()` line 584 uses the obsolete ghost-flag rule
   `(gflag == 0 && ni >= 0)` to decide whether to add back the +1. Since `gflag`
   (`neigh_elem`) is always nonzero for interior elements, the 0-based index is written
   instead of the 1-based value. Fix: always write `ni + 1` for `ni >= 0`, and `0` for
   `ni == -1`.

2. **task3e f_elem has no coverage** — the existing tests only check `eta` and `W_elem`.
   Fix: add a finite-difference self-consistency test that perturbs each of the 12
   neighbor-node coordinates by h=1e-6 and asserts
   `f_elem[inode][k] ≈ -(W(x+h) - W(x)) / h` to 1e-4 relative (finite-difference
   truncation floor).

3. **flag_num_diff=true path untested** — no test forces `flag_num_diff`. Fix: add a
   synthetic flat-geometry test (all z=0, equal principal curvatures → flag_num_diff=true)
   and verify `compute_element_energy` produces finite W_elem and f_elem consistent with
   the finite-difference test. **Note on Codex's flag_num_diff claim**: the review
   asserted that the C++ S_m loop perturbs `curv0_elem` while "the canonical Fortran
   perturbs C_elem_". This is incorrect — ener_elem.f90 line 72 perturbs `curv0_elem_` in
   the S_m loop; line 76 is merely the assignment `S_m(i)=(W_-W)/h`. The C++ translation
   is faithful. The plan-evolution entry below documents this finding.

## Queued Issues (out of scope this round)

- task3f: Fortran-derived principal-curvature oracle fixtures
- task4a onward: global assembly, L-BFGS, pasapas, reaction force, etc.

## Plan Evolution

The flag_num_diff S_m branch perturbs `curv0_elem` (not `C_elem`) — this matches the
canonical Fortran at ener_elem.f90 line 72. Codex's review mistakenly read line 76
(the assignment S_m(i)=(W_-W)/h) as the perturbation site. No change to the C++
implementation is needed for the S_m branch; the plan-evolution log is updated to
document this clarification.

## Concrete Success Criteria

1. `RoundTrip.Mesh` passes (write_mesh uses 1-based for all valid neigh_vert).
2. `ElementEnergy.ForcesAreConsistentWithEnergyByFiniteDifference` passes for element 83
   (12 nodes × 3 components checked, tolerance 1e-4 relative).
3. A `flag_num_diff=true` path test is added and passes.
4. All 56 existing unit tests still pass (no regressions).
5. `RoundTrip.Mesh` integration test passes.
