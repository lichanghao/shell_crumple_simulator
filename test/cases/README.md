# Oracle Reference Cases

These directories contain archived reference outputs from the canonical Fortran oracle.

## Oracle Baseline

- **Repository**: `../finite_crystal_elasticity/` (Fortran 90 source)
- **Frozen commit**: `7d3f77f` ("Document Modules 1–3 in build_and_run_notes.md")
- **Compiler**: gfortran 15.2.0 (Homebrew GCC) + mpif90 (OpenMPI)
- **Platform**: macOS Darwin 24.6.0

## Case Inventory

### graphene_compression_prepro/
Preprocessor reference outputs for the standard graphene compression case.
- Input: `data.dat` — 40×40 mesh, 20nm×20nm, nCodeLoad=3 (compression), nloadstep=50
- Oracle outputs: `nano_BCs.dat`, `nano_config.dat`, `nano_dims.dat`, `nano_general.dat`,
  `nano_Mesh.dat`, `nano_tub_loc.dat`, `nano_zero.dat`, `ghost_coords.dat`, `meshini.msh`
- Build log: `prepro.log`

### graphene_compression_simulator/np1/
Simulator reference outputs for the standard graphene compression case — serial run (np=1).
- Input: nano_*.dat files from graphene_compression_prepro/; run command: `mpirun -np 1 crunch_it`
- Deterministic replay fixture: `graphene_compression_simulator/imperfection_trace_fortran.dat`
  is a frozen per-step imperfection scalar sequence captured from an instrumented Fortran
  `pasapas.f90` runtime loop against the same archived `nano_*.dat` input. It is used by the
  C++ integration harness when it copies this archived case to a temp directory; it is not part
  of the original frozen output set in `np1/`.
- Deterministic replay monitor fixture: `graphene_compression_simulator/replay_step1_monitor.dat`
  records the first constrained-step monitor rows from a same-trace O3 Fortran replay using the
  committed `imperfection_trace_fortran.dat`. It is the replay-specific oracle for trace-driven
  step-1 monitor comparisons and is intentionally separate from the frozen `np1/` archived output
  set.
- Deterministic replay stdout excerpt: `graphene_compression_simulator/replay_step1_stdout.txt`
  is the matching executable-path runtime monitor excerpt captured from `crunch_it` for the same
  replay lane. Tests use it to validate the runtime monitor parser without rerunning the expensive
  monitored executable path.
- Deterministic replay eval fixture: `graphene_compression_simulator/replay_step1_eval_sequence.dat`
  records the corresponding early constrained-evaluation energy sequence from the same replay and
  is the replay-specific oracle for deterministic step-1 evaluation-prefix comparisons.
- Deterministic replay output rows: `graphene_compression_simulator/replay_step1_energy.dat` and
  `graphene_compression_simulator/replay_step1_force.dat` capture the first positive-load runtime
  rows from the same replay and serve as the executable-path replay oracle for `energy.dat` and
  `force.dat`.
- Provenance note: `graphene_compression_simulator/build_provenance.md`
- Confirmed single-rank: `simulator.log` line 1: "Numero de procesadores: 1"
- Oracle outputs: `energy.dat`, `force.dat`, `nano_final_config.dat`, `output.dat`,
  `simulator.log`, `mesh_config_0000.vtu` … `mesh_config_0050.vtu` (51 VTU snapshots),
  `mesh_config_series.pvd`
- Important caveat: for step 1, the archived numeric oracle is `energy.dat`, `force.dat`, and
  `mesh_config_0001.vtu`. The frozen `np1/simulator.log` step-one equilibrium line does not match
  those archived numeric artifacts, so it should be treated as historical context rather than the
  authoritative step-one energy oracle.
- Final energy: 1.3427137479171509E-003 (load step 50, IFLAG=0)

### graphene_cyclic_crumple/
Preprocessor and simulator reference outputs for the cyclic crumpling case.
- Input: `prepro_run/data.dat` — 40×40 mesh, 20nm×20nm, nCodeLoad=31 (biaxial cyclic),
  5 cycles, nloadstep_comp=20, nloadstep_rel=20, ncrease=1
- Preprocessor outputs: `prepro_run/nano_*.dat`, `prepro_run/ghost_coords.dat`, `prepro_run/prepro.log`
- Simulator outputs: `energy.dat`, `force.dat`, `crease_map.dat`, `nano_final_config.dat`,
  `output.dat`, `nano_checkpoint.dat`

### graphene_self_contact/
Preprocessor reference outputs for the single-sheet self-contact cyclic case.
- Input: `prepro_run/data.dat` — 10×10 mesh, 5nm×5nm, nCodeLoad=30, nvdw=1, nself_contact=1,
  4 cycles, nloadstep_comp=20, nloadstep_rel=20, ncrease=1
- Oracle outputs: `prepro_run/nano_*.dat`, `prepro_run/ghost_coords.dat`, `prepro_run/prepro.log`

### graphene_bilayer_twist_vdw_1000/
Preprocessor reference outputs for the bilayer twist local-density case.
- Input: `prepro_run/data.dat` — two 20×20 sheets, 1nm×1nm, nCodeLoad=1000, twist=30 degrees,
  inter-layer separation=0.5nm, nvdw=1, `alpha_sharp=200`, input `nborder=0` with Fortran
  override to `nborder=2`
- Oracle outputs: `prepro_run/nano_*.dat`, `prepro_run/ghost_coords.dat`, `prepro_run/prepro.log`

### bspline_oracle/
Committed Fortran oracle fixtures for `BSpline`, `DBSpline`, and `DDBSpline`.
- Fixtures: `interior_01.dat` … `interior_05.dat`, `boundary_01.dat` … `boundary_05.dat`
- Reproduction helper: `tools/dump_bspline_oracle.f90`
- Source of truth: `../finite_crystal_elasticity/grapheneCompressionOriginPrePro/BSpline.f90`

### constitutive_oracle/
Committed Fortran oracle fixtures for the Brenner constitutive kernel and inner Newton relaxation.
- Fixtures: `brenner/case_01.dat` … `brenner/case_10.dat`,
  `newton_inner/case_01.dat` … `newton_inner/case_10.dat`,
  `archived_compression_np1/case_01.dat` … `archived_compression_np1/case_10.dat`
- Reproduction helpers: `tools/dump_constitutive_oracle.f90`,
  `tools/dump_archived_constitutive_oracle.f90`
- Source of truth: `../finite_crystal_elasticity/grapheneCompressionOriginVersion/brenner.f90`,
  `../finite_crystal_elasticity/grapheneCompressionOriginVersion/Hyper_pot_inner_alg.f90`,
  `../finite_crystal_elasticity/grapheneCompressionOriginVersion/newton_inner.f90`,
  `../finite_crystal_elasticity/grapheneCompressionOriginVersion/geometry.f90`,
  `../finite_crystal_elasticity/grapheneCompressionOriginVersion/principal.f90`,
  `../finite_crystal_elasticity/grapheneCompressionOriginVersion/exponential.f90`

## nCodeLoad Reference

The standard graphene compression case uses **nCodeLoad=3** (not nCodeLoad=1 as originally
written in the plan). The plan's AC-7 and task4c have been updated to reflect this.

nCodeLoad values in the Fortran simulator (load.f90):
- 1, 2: rotation/twist modes
- 3: uniaxial compression (one edge fixed, one edge compressed in x)
- 10, 11, 13: other loading modes
- 30: uniaxial cyclic compression (compress-release cycles)
- 31: biaxial cyclic compression (corner-loaded)
