# Cyclic Replay Oracle Extraction Notes

This directory now contains a dedicated Fortran-side extractor for the cyclic accepted-state replay probes:

- Tool: `test/cases/tools/dump_cyclic_replay_element_oracle.f90`
- Current target: accepted-state-2, element `3200` (1-based) from
  `replay_step1_accepted_2.dat` and `replay_step1_accepted_2_eta.dat`

## Reproduction Command

The extractor is compiled out-of-tree against the canonical Fortran simulator sources in
`../finite_crystal_elasticity/grapheneCompressionOriginVersion/`.

Example scratch build command used in Round 3:

```bash
mkdir -p /tmp/fce_cyclic_oracle_build
cd /tmp/fce_cyclic_oracle_build
cp /Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_cyclic_replay_element_oracle.f90 .
cp /Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/{headers.f90,BSpline.f90,gauss.f90,geometry.f90,ghost_nodes.f90,principal.f90,exponential.f90,Taylor.f90,morse.f90,mm3.f90,brenner.f90,brenner2.f90,Hyper_pot_inner_alg.f90,newton_inner.f90} .
gfortran -c headers.f90 BSpline.f90 Taylor.f90 gauss.f90 geometry.f90 ghost_nodes.f90 principal.f90 exponential.f90 morse.f90 mm3.f90 brenner.f90 brenner2.f90 Hyper_pot_inner_alg.f90 newton_inner.f90 dump_cyclic_replay_element_oracle.f90
gfortran -o dump_cyclic_replay_element_oracle headers.o BSpline.o Taylor.o gauss.o geometry.o ghost_nodes.o principal.o exponential.o morse.o mm3.o brenner.o brenner2.o Hyper_pot_inner_alg.o newton_inner.o dump_cyclic_replay_element_oracle.o
./dump_cyclic_replay_element_oracle \
  /Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/prepro_run \
  /Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/replay_step1_accepted_2.dat \
  /Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/replay_step1_accepted_2_eta.dat \
  /tmp/replay_step1_accepted_2_element3200_full_oracle.dat
```

## Current Status

- The extractor now fails hard on two conditions that previously produced untrustworthy output:
  - coordinate-row index mismatch in the accepted-state coordinate dump
  - nonzero `newton_inner` `fail_mode` when inner relaxation is actually enabled
- The extractor also now honors the archived runtime contract from `nano_general.dat`:
  - if `nW_hat=0`, it keeps `eta` fixed and does not call `newton_inner`
  - if `nW_hat=1`, it performs the same guarded inner-relaxation path described above
- The committed artifact `replay_step1_accepted_2_element3200_full_oracle.dat` was regenerated after this fix and now represents the archived cyclic accepted-state-2 runtime contract instead of an unconditional inner-relaxation helper path.
