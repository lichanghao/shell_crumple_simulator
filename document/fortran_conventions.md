# Fortran Codebase Conventions

Reference document for translating the Fortran graphene FEM simulator to C++.

**Oracle commit**: `7d3f77f` of `finite_crystal_elasticity/`

---

## 1. Array Indexing Convention

| Fortran | C++ |
|---------|-----|
| 1-based (all arrays, all files) | 0-based throughout |

**Critical rule**: Every index stored in `nano_*.dat` files is **1-based** Fortran. When reading into C++, subtract 1 from every node index, DOF index, element index, and neighbor index.

Examples:
- `nano_BCs.dat` `mdofBC` list: a value of `1` means C++ DOF index `0`.
- `nano_Mesh.dat` connectivity: vertex nodes `1, 2, 42` → C++ `0, 1, 41`.
- `nano_Mesh.dat` B-spline 12-node patch: all indices are 1-based; ghost node marker `0` means "no valid neighbor" (not a node index).

---

## 2. Unit System

| Quantity | Unit |
|----------|------|
| Lengths, coordinates | nm (nanometres) |
| Bond length (A0) | nm (e.g., `0.142 nm` for Brenner C-C) |
| Energies | eV |
| Forces | eV/nm |
| Curvatures (κ) | 1/nm |
| Deformation gradients F0 | dimensionless |
| Jacobians J0 | nm² (area element in reference config) |
| Convergence criteria | dimensionless residual |

---

## 3. Floating-Point Format

All floating-point values in `nano_*.dat` files use **Fortran `D` exponent notation** (double precision, 64-bit IEEE 754). When reading in C++, replace `D` with `E` before parsing, or use `sscanf` with `%lf` after substitution.

Example: `0.20000000000000000D+02` = 20.0 nm.

Typical precision: 17 significant decimal digits (full double precision).

---

## 4. `nano_*.dat` File Formats

### 4.1 `nano_dims.dat`

Mesh dimension summary. Read first to allocate data structures.

```
 Dims data
 ---------
 mesh0%numele          → number of triangular elements (integer)
 mesh0%numnods         → total node count including ghost nodes (integer)
 mesh0%nedge           → number of boundary edge nodes (integer)
 mesh0%nelem_ghost     → number of ghost elements (integer)
 mesh0%nnode_ghost     → number of ghost nodes (integer)
 ngauss                → Gauss points per element (integer, typically 2)
 BCs%nnodBC            → number of BC nodes (integer)
 BCs%ndofBC            → number of constrained DOFs (integer)
 BCs%ndofOP            → number of free (optimized) DOFs (integer)
 vdw1%nvdw             → vdW flag (0=disabled, 1=enabled) (integer)
```

For the standard 40×40 graphene case: `numele=3200`, `numnods=1681`, `nedge=166`, `ngauss=2`.

### 4.2 `nano_general.dat`

Material properties, potential parameters, and solver settings.

```
 General data
 ------------
 ylength              → sheet circumferential/width length (real, nm)
 mat1%A0              → equilibrium C-C bond length (real, nm, e.g. 0.142)
 mat1%nCode_Pot       → potential code (integer; 1=Brenner REBO)
 [4 Brenner params]   → 4 reals: C0, C1, C2, C3 (potential fitting parameters)
 mat1%E               → 3 bond vectors E1, E2, E3 (3 rows × 2 columns, reals)
 mat1%s0              → reference unit cell area (real, nm²)
 nW_hat               → inner relaxation flag (integer; 1=yes, 0=no)
 crit(1)              → global convergence tolerance (real)
 crit(2)              → local (inner) convergence tolerance (real)
 imperfect            → imperfection flag (integer; 1=yes, 0=no)
 fact_imp             → imperfection amplitude (real)
```

Bond vectors `E_i` are 2D vectors in the material frame (x,y components only, 2D reference lattice).

### 4.3 `nano_zero.dat`

Reference configuration Jacobians and deformation gradients, per element per Gauss point.

Structure (repeat `numele × ngauss` times):
```
 J0            → reference Jacobian (scalar real, nm²)
 F0[0][0]  F0[0][1]   → row 1 of 2×2 deformation gradient (2 reals)
 F0[1][0]  F0[1][1]   → row 2 of 2×2 deformation gradient (2 reals)
```

Ordering: element 1 gauss-1, element 1 gauss-2, element 2 gauss-1, element 2 gauss-2, ...

For the 40×40 mesh: 3200 elements × 2 Gauss points = 6400 records of 5 values each.

### 4.4 `nano_config.dat`

Initial nodal positions and inner displacement η.

```
 Config Data
 -----------
 Nodal positions
 [x y z]  × numnods     → 3D coordinates for each node (3 reals per line, nm)
 [inner displacement]   → η values per Gauss point per element (initially all 0.0)
                          ordering: element 1 gauss-1, element 1 gauss-2, ...
```

For flat graphene: all z-coordinates are 0. x spans [0, xlength], y spans [0, ylength].

### 4.5 `nano_BCs.dat`

Boundary condition and loading parameters.

```
 BCs data
 --------
 BCs%nloadstep        → number of load steps (integer)
 BCs%nCodeLoad        → loading type code (integer; see nCodeLoad table)
 BCs%mdofBC           → constrained DOF indices, one per line (integer, 1-based, ndofBC values)
 BCs%mdofOP           → free DOF indices, one per line (integer, 1-based, ndofOP values)
 BCs%mnodBC           → BC node indices (integer, 1-based, nnodBC values)
 BCs%rotation(3,3)    → 3×3 rotation matrix (reals, row-major)
 BCs%xc(3)            → center point for rotation (3 reals, nm)
 BCs%value            → total loading value (real, nm or degrees)
 [cyclic params]      → ncycles, nloadstep_comp, value_comp, nloadstep_rel, value_rel
                         (only present when nCodeLoad=30 or 31)
```

For nCodeLoad=3: `value` = total compression distance in x (nm).

### 4.6 `nano_Mesh.dat`

Element connectivity and B-spline 12-node patch neighbor tables.

```
 Mesh data
 ---------
 Connect
 New element        → marker (appears before each element record)
 v1  v2  v3         → 3 vertex node indices (integer, 1-based)
 ngauss             → always 2
 [12 B-spline patch nodes, 2 values per node: (ghost_flag, node_index)]
    ghost_flag: 0=real node, nonzero=ghost
    node_index: 1-based node number, or ghost offset when ghost_flag≠0
 code_bc(3)         → boundary condition code for each of 3 edges (integer)
```

The 12-node B-spline patch covers the element plus its neighbors. Ghost entries (ghost_flag≠0) indicate nodes extrapolated beyond the mesh boundary via the parallelogram rule.

### 4.7 `nano_tub_loc.dat`

Element-to-tube partition table for MPI element distribution.

```
 [ielem_start, ielem_end] per MPI rank
```

For single-rank (np=1) runs: one entry covering all elements.

### 4.8 `nano_crease.dat` (cyclic cases only)

Reference curvature tensor `K0_ref` for crease memory, written/read during cyclic runs.

---

## 5. Sign Conventions for Boundary Conditions

- **nCodeLoad=3 (compression)**: The positive `value` means the right edge (maximum x) is compressed inward (moved in the −x direction). The left edge (x=0) is held fixed.
- **nCodeLoad=30 (uniaxial cyclic)**: `value_comp` = compression distance per compression phase; `value_rel` = release distance per release phase. Sign flips between phases in `load.f90`.
- **nCodeLoad=31 (biaxial cyclic)**: Corner-loaded; corners B, C, D receive x, y, xy compression respectively.
- The rotation matrix in `nano_BCs.dat` is used for twist-mode loading (not relevant for nCodeLoad=3).
- DOF ordering: each node contributes 3 DOFs in order (x, y, z), so DOF `3*(inode-1)+1` = x-DOF of node `inode` (1-based Fortran).

---

## 6. MPI Conventions

- **I/O**: Only MPI rank 0 reads and writes files. All broadcast/scatter operations follow.
- **Element distribution**: Elements are partitioned contiguously per rank using `pre_ener.f90`. The partition table is stored in `nano_tub_loc.dat`.
- **Global assembly**: Each rank computes energy and forces for its assigned elements. MPI_ALLREDUCE (SUM) gathers global energy and force vector.
- **Ghost elements**: Not needed for MPI; ghost *nodes* are used only for B-spline boundary patches (different concept).
- **Checkpointing**: Only rank 0 writes `nano_checkpoint.dat`; all ranks receive checkpoint data via broadcast on restart.

---

## 7. nCodeLoad Reference Table

| nCodeLoad | Description | Where set |
|-----------|-------------|-----------|
| 0 | No loading / identity | Prepro + Simulator |
| 1, 2 | Rotation modes | Simulator |
| 3 | Uniaxial compression (x-direction) | Prepro code 3 + Simulator |
| 10, 11, 13 | Other loading modes | Simulator |
| 30 | Uniaxial cyclic compression (compress-release) | Prepro code 30 + Simulator |
| 31 | Biaxial corner-loaded cyclic compression | Prepro code 31 + Simulator |
| 222, 1000 | Internal test/identity modes | Simulator |

**Note**: In the prepro (`data.dat`), the "Code" field sets the `nCodeLoad` value that will be written to `nano_BCs.dat`. The simulator reads `nCodeLoad` from `nano_BCs.dat`.

---

## 8. Active Source Files

### Preprocessor (`grapheneCompressionOriginPrePro/`)

| File | Role |
|------|------|
| `headers.f90` | Data type definitions (data_tri, data_mesh, data_BC, data_vdw, data_mat) |
| `Prepro.f90` | Main program |
| `read_data.f90` | Input parser for `data.dat` |
| `mesh_gen.f90` | `mesh_gen_square` — rectangular mesh generation |
| `connect_mesh.f90` | Element connectivity, neighbor tables, ghost node generation |
| `Def_Grad.f90` | Initial deformation gradient F0 and Jacobian J0 |
| `BSpline.f90` | B-spline basis functions N_i and derivatives |
| `gauss.f90` | Gauss quadrature setup |
| `load.f90` | BC setup and load parameter generation |
| `vdw_previous.f90` | vdW preprocessing (neighbor list, shape functions) |
| `gmsh_out.f90` | gmsh .msh output |

**Exclude from oracle**: `*.f90_mod2`, `*.f90_good`, `*.f90A` — these are backup variants, not canonical.

### Simulator (`grapheneCompressionOriginVersion/`)

| File | Role |
|------|------|
| `headers.f90` | Data types (extended with data_crease, cyclic params) |
| `Optim.f90` | Main program, MPI init, checkpoint detection |
| `read.f90` | Input readers for all nano_*.dat; checkpoint read/write |
| `pasapas.f90` | Load-stepping loop, cycle controller, crease updates |
| `load.f90` | Load increment application for all nCodeLoad modes |
| `minimize.f90` | L-BFGS wrapper (MPI-aware) |
| `minimize_free.f90` | L-BFGS wrapper for unconstrained optimization |
| `lbfgs.f` | **Nocedal L-BFGS implementation** (legacy Fortran 77; uses COMMON blocks; must be translated directly, not replaced) |
| `energy.f90` | Global energy/force assembly, MPI reduction |
| `pre_ener.f90` | MPI element partitioning |
| `ener_elem.f90` | Per-element energy/force/stiffness kernel |
| `geometry.f90` | Metric tensor, curvature tensor computation |
| `principal.f90` | Principal curvature extraction |
| `exponential.f90` | Exponential map for Cauchy-Born rule |
| `newton_inner.f90` | Newton solver for inner shift vector η |
| `Hyper_pot_inner_alg.f90` | Hyperelastic potential for inner Newton |
| `brenner.f90` | Brenner REBO potential W, dW/dpe, d²W/dpe² |
| `brenner2.f90` | Alternative Brenner implementation |
| `BSpline.f90` | B-spline basis functions and derivatives |
| `gauss.f90` | Gauss quadrature |
| `ghost_nodes.f90` | Ghost node coordinate computation |
| `get_reac.f90` | Reaction force and torque computation |
| `vdw_modules.f90` | Van der Waals interactions, self-contact, neighbor list |
| `crease.f90` | Crease memory: init_creases, update_creases, write_crease_stats |
| `crease_analysis.f90` | Crease detection and facet analysis |
| `paraview_vtu_output.f90` | VTU/ParaView output |
| `dyna_relax.f90` | Dynamic relaxation (alternative minimizer) |
| `Taylor.f90` | Taylor expansion utilities |
| `mm3.f90` | MM3 force field (alternative potential) |
| `morse.f90` | Morse potential (alternative potential) |
| `ensight.f90` | Ensight Gold output (not required for C++ v1) |
| `fem_ensight_wrap.f90` | Ensight wrapper (not required for C++ v1) |
| `gmsh_out.f90` | gmsh output |
| `netcdf_mesh_output.f90` | NetCDF output (not required for C++ v1) |

**Exclude from oracle**: `brenner.f90_good`, `brenner.f90_mod2`, `brenner.f90_mod3`, `brenner2.f90_*`, `pasapas.f90A`, `pasapas.f90N`, `read.f90A`, `Optim.f90A` — backup variants only.

---

## 9. Key Implementation Notes for C++ Translation

1. **Indexing**: Convert all 1-based indices to 0-based when reading from files and in all internal arrays. Add an index-conversion test: for every file read, verify that the index range `[1, N]` maps correctly to `[0, N-1]`.

2. **D-exponent**: When reading `nano_*.dat` files in C++, scan for `D` or `d` and replace with `E`/`e` before calling `strtod` or `sscanf("%lf")`.

3. **COMMON blocks in lbfgs.f**: The Fortran L-BFGS uses `COMMON /LB3/` and `COMMON /LB2/` for state. In C++, these become private member variables in an `LbfgsSolver` class.

4. **MPI rank 0 I/O**: Wrap all file I/O in `if (rank == 0)` guards, then broadcast data to all ranks.

5. **Ghost node flag**: In `nano_Mesh.dat`, the ghost flag is 0 for a real node and nonzero for a ghost. A ghost flag value `k` combined with the node index encodes the ghost table entry. Read `connect_mesh.f90` carefully to reconstruct the exact encoding.

6. **nano_config.dat inner displacement**: After the nodal positions (numnods × 3 values), there are `ngauss × numele` scalar η values (one per Gauss point per element). Initially all zero in the reference state.
