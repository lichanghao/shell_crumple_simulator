# Round 27 Summary

## Objective

Close Milestone 3 oracle gaps: task3a (exponential-map oracle fixtures), task3f (principal curvature oracle fixtures), and task3c (Brenner-through-element-energy production path validation).

## Work Completed

### task3f + task3a — Principal Curvature and Exponential-Map Oracle Fixtures

Created `test/cases/tools/dump_principal_exponential_oracle.f90`, a standalone Fortran oracle driver that:
- Reads the archived compression state (nano_dims, nano_general, nano_zero, nano_final_config, nano_Mesh)
- Iterates over interior element patches to find 10 cases (elements 83–87, Gauss points 1–2)
- For each case: calls `metric()` → `curv()` → `principal()` (full derivatives), computes bond geometry with eta=0 (A_norm, Ei independent of Newton), calls `def_bonds()` (full derivatives)
- Writes 36-row fixture: header + C_elem + curv0_elem + flag_num_diff + curvppal + vppal(2×2) + dcurvppaldC(2×3) + dcurvppaldk(2×3) + dvppaldC(2×2×3) + dvppaldk(2×2×3) + A_norm(3) + Ei(3×2) + pe(6) + dpedC(6×3) + dpedk(6×3)

Generated 10 fixture files: `test/cases/principal_exponential_oracle/case_01.dat` … `case_10.dat`.

Created `test/cases/principal_exponential_oracle/build_provenance.md` documenting oracle commit, fixture scope, row format, tolerance rationale, and reproduction recipe.

Added oracle tests:
- `test/unit/test_principal.cpp`: `Principal.MatchesArchivedCompressionFortranOracle` — compares all principal outputs (curvppal, vppal, all 4 derivative tensors) at 1e-12 absolute tolerance
- `test/unit/test_exponential.cpp`: `Exponential.MatchesArchivedCompressionFortranOracle` — compares pe[6], dpedC[6][3], dpedk[6][3] at 1e-12 absolute tolerance

Both tests pass over all 10 fixtures.

### task3c — Brenner Material through Element-Energy Production Path

Created `test/cases/tools/dump_element_energy_brenner_oracle.f90`, a Fortran oracle driver that:
- Uses element 83's archived geometry from `nano_final_config.dat`
- Hardcodes Brenner REBO material (nCode_Pot=2, A0=0.142, A1=0.142, Vs=[0.60310500860214233, 26.25, 0.9], Va=[0.75400000810623169, 0.149, 0.25]) — same as `dump_constitutive_oracle.f90`
- Runs the full production path: metric → curv → principal → newton_inner (Inner_Brenner) → def_bonds/def_bonds_ → Brenner (outer) → Stresses → f_elem assembly
- Writes 14-row fixture to `test/cases/element_energy_oracle/brenner_geom_np1/case_01.dat`

Added `ElementEnergy.BrennerMaterialMatchesFortranOracle` test in `test/unit/test_element_energy.cpp`:
- Assembles MatData with nCode_Pot=2 and Brenner parameters
- Loads element 83 geometry from the archived compression state
- Calls `compute_element_energy` and compares W_elem at 1e-6, all 12×3 f_elem components at max(1e-6, |expected|×1e-6)
- Test passes

Updated `test/cases/element_energy_oracle/build_provenance.md` with the new fixture section.

## Files Changed

### New Files
1. `test/cases/tools/dump_principal_exponential_oracle.f90` — Fortran oracle driver for principal + exponential map
2. `test/cases/principal_exponential_oracle/build_provenance.md` — fixture provenance documentation
3. `test/cases/principal_exponential_oracle/case_01.dat` … `case_10.dat` — 10 Fortran-derived fixture files (36 rows each)
4. `test/cases/tools/dump_element_energy_brenner_oracle.f90` — Fortran oracle driver for Brenner element-energy
5. `test/cases/element_energy_oracle/brenner_geom_np1/case_01.dat` — Brenner element-energy fixture (14 rows)

### Modified Files
6. `test/unit/test_principal.cpp` — added `PrincipalExponentialFixture`, `read_pexp_fixture`, `Principal.MatchesArchivedCompressionFortranOracle`
7. `test/unit/test_exponential.cpp` — added `PExpFixture`, `read_pexp_fixture_exp`, `Exponential.MatchesArchivedCompressionFortranOracle`
8. `test/unit/test_element_energy.cpp` — added `ElementEnergy.BrennerMaterialMatchesFortranOracle`
9. `test/cases/element_energy_oracle/build_provenance.md` — added `brenner_geom_np1/case_01.dat` section
10. `.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md` — task3a, task3c, task3f marked completed; plan version 30; Round 27 evolution log entry

## Validation

- **Unit tests**: 63/63 pass
- **Integration tests**: 18/18 pass
- All oracle comparisons pass at stated tolerances (1e-12 for principal/exponential, 1e-6 for Brenner element-energy)

## Acceptance Criteria Addressed

- **AC-5** (principal curvature oracle fixtures and test): CLOSED — task3f done
- **AC-6** (exponential-map oracle fixtures and test): CLOSED — task3a done
- **AC-7** (Brenner material through element-energy production path): CLOSED — task3c done

## Remaining Items

- **task4a** (global energy/force assembly, Milestone 4 Phase A): Deferred to Round 28 per Round 27 contract.
- **task6a-6c** (runtime vdW/self-contact): Not started, blocked side issue.
- **task8c** (AGENT.md + translation_notes.md): Not started.

## BitLesson Delta

Action: none
Lesson ID(s): NONE
Notes: No new non-obvious problems encountered. The `principal_` flag-as-input lesson (BL-20260404-principal-flag-input) was captured in Round 26 and applied correctly here.
