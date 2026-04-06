# Round 34 Contract

## Mainline Objective
Eliminate the deterministic-surrogate imperfection path from the AC-7 runtime solver, rerun the real archived-oracle executable regression, and capture the first still-failing runtime step with source-equivalent step ordering.

## Target ACs
- AC-7: End-to-end serial compression runtime matches the archived Fortran oracle on the real executable path.

## Blocking Issues
- The current C++ imperfection logic is reproducible but not source-equivalent to canonical `pasapas.f90`.
- Full archived-oracle force/final-config parity has not been rerun since the reaction-side mapping fix.
- The tracker and summary need fresh post-change evidence, not another inference from stale runs.

## Success Criteria
- The deterministic `minstd_rand` imperfection surrogate is removed or replaced with a closer source-equivalent runtime behavior.
- A fresh full executable-path archived-oracle regression is run after that change.
- The round records the earliest still-failing energy/force/final-config evidence after the imperfection-path correction.
- Round 34 summary requests only tracker updates that are directly supported by repository evidence.
