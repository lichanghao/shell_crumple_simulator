# Element Energy Oracle Provenance

## Oracle Repository

- **Path**: `../finite_crystal_elasticity/`
- **Frozen commit**: `7d3f77fab2378d675d14ebeac3d8e65d94221a4f`
- **Commit message**: "Document Modules 1–3 in build_and_run_notes.md"

## Fixture Scope

### `archived_compression_np1/case_01.dat`

Canonical Fortran reference for `ElementEnergy.FElemMatchesFortranOracle`.

- **Element**: 83 (1-based), the first interior element in the compressed graphene mesh
- **ngauss**: 2
- **Path**: analytical stresses (`flag_num_diff=false`): `def_bonds` + `Stresses` +
  `Hyper_Pot` (inline Morse wrapper, nCode_Pot=1)
- **Initial condition**: `eta=0` per Gauss point
- **Source**: `nano_final_config.dat` (deformed state)
- **Format**: header (ielem ngauss) + W_elem + 12 f_elem rows (14 rows total)

**flag_num_diff note**: Element 83 has non-trivial curvature (`|curv0_elem[0]| ≈ 0.043`),
so principal curvatures are distinct and `flag_num_diff=false`. The oracle uses the
analytical path.

### `flat_geom_np1/case_01.dat`

Canonical Fortran reference for `ElementEnergy.FlagNumDiffStressesMatchFortranOracle`.
Verifies the Round-25 S_m fix: S_m must perturb `C_elem` (not `curv0_elem`), so
S_n == S_m in the `flag_num_diff=true` branch.

- **Element**: 83 (1-based), same connectivity as archived case
- **ngauss**: 2
- **Geometry**: element 83 x,y from `nano_final_config.dat` but z=0 (flat)
- **Path**: numerical differentiation (`flag_num_diff=true`): `curv0_elem=0` →
  `k1=k2=0` → both S_n and S_m loops perturb `C_elem_`
- **Initial condition**: `eta=0` per Gauss point
- **Source**: `nano_final_config.dat` (deformed state), z zeroed
- **Format**: header + W_elem + 12 f_elem rows + per-Gauss stresses (20 rows total):
  ```
  Row 0:     ielem  ngauss
  Row 1:     W_elem
  Rows 2-13: f_elem(inode, 0:2)
  For each Gauss point (3 rows each):
    Row 14+ig*3: flag_num_diff (1 or 0)
    Row 15+ig*3: S_n[3]
    Row 16+ig*3: S_m[3]
  ```

**S_m note**: In the canonical `ener_elem.f90` (lines 76-84), the bending-stress S_m loop
is identical to the membrane-stress S_n loop: both perturb `C_elem_`. This is a probable
copy-paste defect in the Fortran (for bending stress, perturbing `curv0_elem_` is more
physical), but for oracle fidelity the C++ implementation matches this exactly. The
fixture confirms S_n == S_m for both Gauss points of the flat element.

**principal_ note**: The Fortran `principal_` subroutine takes `flag_num_diff` as an
INPUT (not output) to decide which vppal branch to use. In the numerical-diff loop, the
canonical ener_elem.f90 passes the same `flag_num_diff` variable (set by the earlier
`principal(...)` call). The oracle does the same. If `flag_dummy` (uninitialized) is
passed instead, `principal_` may take the wrong branch and produce NaN vppal.

**Tolerance note**: The `FlagNumDiffStressesMatchFortranOracle` test uses 1e-6 absolute
tolerance for S_n/S_m and 1e-6-relative for f_elem. The inherent truncation and
rounding error of one-sided FD with h=1e-8 causes gfortran/g++ to produce values that
differ by up to ~3e-7. The exact equality `S_n[i] == S_m[i]` (CPU registers, same
code path) is still asserted.

## Reproduction

```bash
mkdir -p /tmp/elem_energy_mods
cd /tmp && gfortran -std=legacy -O0 -J /tmp/elem_energy_mods -c \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/headers.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/Taylor.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/BSpline.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/gauss.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/geometry.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/principal.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/exponential.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/brenner.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/brenner2.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/morse.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/mm3.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/Hyper_pot_inner_alg.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/newton_inner.f90 \
  ../finite_crystal_elasticity_Cpp/test/cases/tools/dump_element_energy_oracle.f90
gfortran -std=legacy -O0 -J /tmp/elem_energy_mods \
  -o /tmp/dump_element_energy_oracle /tmp/*.o
# Write both fixtures (oracle-dir is the parent of archived_compression_np1 and flat_geom_np1)
/tmp/dump_element_energy_oracle \
  ../finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1 \
  ../finite_crystal_elasticity_Cpp/test/cases/element_energy_oracle
```

Note: `energy.f90` (which contains `Hyper_Pot` and `Stresses` but also an MPI include) is
intentionally excluded. The oracle implements `My_Hyper_Pot` (Morse wrapper) and
`My_Stresses` as contained subroutines, matching the canonical `Hyper_Pot` and `Stresses`
semantics exactly.

### `brenner_geom_np1/case_01.dat`

Canonical Fortran reference for `ElementEnergy.BrennerMaterialMatchesFortranOracle` (task3c).
Validates the production `compute_element_energy` path for Brenner REBO (`nCode_Pot=2`).

- **Element**: 83 (1-based), same connectivity as archived cases
- **ngauss**: 2
- **Geometry**: element 83 x,y,z from `nano_final_config.dat` (same as archived_compression_np1)
- **Material**: Brenner REBO — same parameters as `dump_constitutive_oracle.f90`
  (`A0=0.142`, `A1=0.142`, `Vs=[0.603105, 26.25, 0.9]`, `Va=[0.754000, 0.149, 0.25]`)
- **Path**: analytical (`flag_num_diff=false` for element 83 deformed geometry)
- **Format**: same as archived_compression_np1/case_01.dat (14 rows: header + W_elem + 12 f_elem)
- **Tolerance**: 1e-6 absolute (Brenner values O(100); gfortran/g++ rounding ~1e-6)

**Reproduction**:

```bash
mkdir -p /tmp/brenner_ee_mods /tmp/brenner_ee_obj
cd /path/to/parent && gfortran -std=legacy -O0 -J /tmp/brenner_ee_mods -c \
  finite_crystal_elasticity/grapheneCompressionOriginVersion/headers.f90 \
  finite_crystal_elasticity/grapheneCompressionOriginVersion/Taylor.f90 \
  finite_crystal_elasticity/grapheneCompressionOriginVersion/BSpline.f90 \
  finite_crystal_elasticity/grapheneCompressionOriginVersion/gauss.f90 \
  finite_crystal_elasticity/grapheneCompressionOriginVersion/geometry.f90 \
  finite_crystal_elasticity/grapheneCompressionOriginVersion/principal.f90 \
  finite_crystal_elasticity/grapheneCompressionOriginVersion/exponential.f90 \
  finite_crystal_elasticity/grapheneCompressionOriginVersion/brenner.f90 \
  finite_crystal_elasticity/grapheneCompressionOriginVersion/brenner2.f90 \
  finite_crystal_elasticity/grapheneCompressionOriginVersion/morse.f90 \
  finite_crystal_elasticity/grapheneCompressionOriginVersion/mm3.f90 \
  finite_crystal_elasticity/grapheneCompressionOriginVersion/Hyper_pot_inner_alg.f90 \
  finite_crystal_elasticity/grapheneCompressionOriginVersion/newton_inner.f90 \
  finite_crystal_elasticity_Cpp/test/cases/tools/dump_element_energy_brenner_oracle.f90
mv *.o /tmp/brenner_ee_obj/
gfortran -std=legacy -O0 -J /tmp/brenner_ee_mods \
  -o /tmp/dump_element_energy_brenner_oracle /tmp/brenner_ee_obj/*.o
/tmp/dump_element_energy_brenner_oracle \
  finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1 \
  finite_crystal_elasticity_Cpp/test/cases/element_energy_oracle
```
