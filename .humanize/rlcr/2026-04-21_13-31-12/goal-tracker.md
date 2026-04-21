# Goal Tracker

<!--
This file tracks the ultimate goal, acceptance criteria, and plan evolution.
It prevents goal drift by maintaining a persistent anchor across all rounds.

RULES:
- IMMUTABLE SECTION: Do not modify after initialization
- MUTABLE SECTION: Update each round, but document all changes
- Every task must be in one of: Active, Completed, or Deferred
- Deferred items require explicit justification
-->

## IMMUTABLE SECTION
<!-- Do not modify after initialization -->

### Ultimate Goal
Finish the C++17 translation of the graphene finite crystal elasticity preprocessor and simulator so the remaining executable-path physics features match the frozen Fortran oracle at commit `7d3f77f`, with deterministic regression evidence for cyclic loading, runtime vdW/self-contact, checkpoint/restart, MPI parity, and VTU outputs.

### Acceptance Criteria
<!-- Each criterion must be independently verifiable -->
<!-- Claude must extract or define these in Round 0 -->
- `GT-AC1`: The cyclic `nCodeLoad=31` executable path matches the committed same-trace Fortran replay through the first constrained step, including accepted-state progression, `GNORM`, reaction output, and emitted `energy.dat`/`force.dat` rows within existing test tolerances.
- `GT-AC2`: Runtime vdW/self-contact translation is connected to the simulator so executable-path `nvdw=1` oracle cases produce Fortran-matching energy, force, neighbor-list, and emitted field data within committed test tolerances.
- `GT-AC3`: Checkpoint/restart for `x0`, `eta`, and `K0_ref` round-trips correctly across interrupted cyclic runs and detects incompatible restart conditions instead of silently corrupting state.
- `GT-AC4`: MPI runtime parity is verified for `np=1`, `np=2`, and `np=4`, with consistent energy/reaction output and correct element partitioning across supported oracle cases.
- `GT-AC5`: VTU/PVD runtime output remains parser-valid and matches executable-path oracle data, including real `nvdw=1` field coverage for `atomic_density` and `W_density`.
- `GT-AC6`: Project documentation and final verification evidence stay synchronized with the translated code, including `document/translation_notes.md`, `document/fortran_conventions.md`, `AGENT.md`, and end-to-end acceptance coverage.

---

## MUTABLE SECTION
<!-- Update each round with justification for changes -->

### Plan Version: 1 (Updated: Round 0)

#### Plan Evolution Log
<!-- Document any changes to the plan with justification -->
| Round | Change | Reason | Impact on AC |
|-------|--------|--------|--------------|
| 0 | Initial tracker seeded from the verified state at the end of the prior RLCR session | The previous loop stopped via circuit breaker, so the restarted session needs the same anchored runtime blockers, completed checkpoint rejection surface, and accepted-state-2 element oracle progress rather than a blank tracker | Keeps the restarted loop aligned with `GT-AC1` through `GT-AC6` without reopening already verified sub-surfaces |

#### Active Tasks
<!-- Map each task to its target Acceptance Criterion and routing tag -->
| Task | Target AC | Status | Tag | Owner | Notes |
|------|-----------|--------|-----|-------|-------|
| Resolve the cyclic constrained-step divergence beginning at accepted state 3 and restore same-trace replay parity for `nCodeLoad=31` | GT-AC1 | pending | coding | claude | Current blocker is the free-gradient / force path after accepted state 2 according to `document/translation_notes.md` |
| Reconcile cyclic executable-path outputs (`GNORM`, reaction columns, `energy.dat`, `force.dat`, and VTU snapshots) with committed replay fixtures | GT-AC1, GT-AC5 | pending | coding | claude | Depends on the constrained-step fix but should remain tracked separately |
| Translate simulator-side vdW/self-contact assembly from `vdw_modules.f90` and connect it to runtime energy/force assembly | GT-AC2 | pending | coding | claude | Preprocessor-side `nvdw=1` support already exists |
| Add executable-path `nvdw=1` oracle coverage for emitted VTU fields and runtime regression tests | GT-AC2, GT-AC5 | pending | coding | claude | Must validate nonzero `atomic_density` and `W_density` against real oracle artifacts |
| Implement and verify cyclic checkpoint/restart for `x0`, `eta`, and `K0_ref`, including incompatible restart detection | GT-AC3 | pending | coding | claude | Covers the remaining restart contract from the plan |
| Implement irreversible crease memory initialization/update/output (`task7b`) and verify `K0_ref` lifecycle on cyclic runs | GT-AC1, GT-AC3 | pending | coding | claude | Original-plan crease-memory work is partially covered but not yet complete under the archived cyclic case |
| Implement crease detection / analysis artifacts (`task7c`), including `crease_map.dat` parity for cyclic cases | GT-AC1, GT-AC6 | pending | coding | claude | Archived `crease_map.dat` parity is covered, but the primary cyclic replay lane remains open |
| Finish multi-rank runtime parity checks for `np=1/2/4`, including partition validation and reduction consistency | GT-AC4 | pending | coding | claude | MPI wrapper exists; acceptance coverage is still partial |
| Refresh project documentation and final integration evidence once runtime parity milestones are green | GT-AC6 | pending | analyze | codex | Includes final acceptance sweep and document synchronization from the plan's closing tasks |

### Completed and Verified
<!-- Only move tasks here after Codex verification -->
| AC | Task | Completed Round | Verified Round | Evidence |
|----|------|-----------------|----------------|----------|
| GT-AC3 | Shared checkpoint resume logic now deterministically rejects malformed checkpoints and rank-count mismatches without depending on the MPI launcher path | prior loop | prior loop | `./build/checkpoint_integration_tests --gtest_filter='CheckpointRejectionRuntime.CrunchItRejectsCheckpointWrittenWithDifferentRankCount:CheckpointRejectionRuntime.CrunchItRejectsMalformedCheckpointAcrossRanks' --gtest_brief=1`; `./checkpoint_integration_tests --gtest_filter='CheckpointRejectionRuntime.CrunchItRejectsCheckpointWrittenWithDifferentRankCount:CheckpointRejectionRuntime.CrunchItRejectsMalformedCheckpointAcrossRanks' --gtest_brief=1`; `ctest --test-dir build --output-on-failure -R '^CheckpointRejectionRuntime\\.(CrunchItRejectsCheckpointWrittenWithDifferentRankCount|CrunchItRejectsMalformedCheckpointAcrossRanks)$'` |

### Explicitly Deferred
<!-- Items here require strong justification -->
| Task | Original AC | Deferred Since | Justification | When to Reconsider |
|------|-------------|----------------|---------------|-------------------|

### Open Issues
<!-- Issues discovered during implementation -->
| Issue | Discovered Round | Blocking AC | Resolution Path |
|-------|-----------------|-------------|-----------------|
| Cyclic replay currently diverges during the constrained solve after accepted state 2 even though earlier checkpoints and `eta` still match the same-trace Fortran replay | prior loop | GT-AC1 | Instrument the free-gradient / force path around accepted states 2-3 and compare against committed replay fixtures |
| Untraced cyclic step-one replay still stalls before step-1 output files are written, even though the traced accepted-state probes complete and still localize the mismatch to the top-right free-node force path | prior loop | GT-AC1 | Fix the constrained solve/update path so the untraced replay reaches the first output row, then reconcile it against the committed replay fixtures |
| The hardened Fortran-side cyclic accepted-state-2 element-3200 oracle extractor is reproducible, its committed artifact is exercised by a passing C++ regression, and that narrows the remaining `GT-AC1` gap to the live cyclic constrained-step replay/output path | prior loop | GT-AC1 | Use the now-consumed element-3200 oracle together with the accepted-state-2/3 replay fixtures to fix the executable-path force / `GNORM` / reaction mismatch |
| Simulator-side vdW/self-contact translation is still missing, so runtime `nvdw=1` oracle evidence and emitted field parity cannot yet pass | prior loop | GT-AC2, GT-AC5 | Port `vdw_modules.f90`, wire it into runtime assembly, then add executable-path regression coverage |
| Checkpoint/restart and multi-rank runtime acceptance coverage are incomplete despite existing serial/MPI scaffolding | prior loop | GT-AC3, GT-AC4 | Finish the remaining runtime implementation and add rank-count parity / restart tests before final verification |
