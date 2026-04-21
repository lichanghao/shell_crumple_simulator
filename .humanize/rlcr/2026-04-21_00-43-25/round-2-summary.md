## Round 2 Summary

### What changed
- Added [test/cases/tools/dump_cyclic_replay_element_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_cyclic_replay_element_oracle.f90), a dedicated Fortran-side oracle dumper for the cyclic replay accepted-state-2 top-right-corner probe (`replay_step1_accepted_2.dat` / `_eta.dat`, element `3200`).
- The new tool reads the cyclic case material/F0 data, reads the accepted-state coordinate and eta dumps, reconstructs ghost coordinates with the canonical Fortran `ghost_nodes`, and emits per-Gauss constitutive state (`C_elem`, `curv0_elem`, `curvppal`, `vppal`, `eta`, `W`, `pe`, `S_n`, `S_m`) plus assembled `f_elem` / `W_elem`.
- I deliberately did **not** freeze the generated cyclic fixture into the tree yet because the current dump still disagrees materially with the C++ accepted-state-2 constitutive state, which means the oracle extraction path itself still needs one more correction before its output is trustworthy as a committed contract.

### Validation
- Rebuilt `unit_tests` successfully:
  `cmake --build build --target unit_tests -j4`
- Re-ran the existing stable element-energy oracle tests and kept them green:
  `./build/unit_tests --gtest_filter='ElementEnergy.FElemMatchesFortranOracle:ElementEnergy.FlagNumDiffStressesMatchFortranOracle'`
- Rebuilt the new Fortran cyclic oracle tool successfully in an isolated scratch directory by compiling it against the canonical Fortran simulator sources (`headers.f90`, `BSpline.f90`, `gauss.f90`, `geometry.f90`, `ghost_nodes.f90`, `principal.f90`, `exponential.f90`, `Taylor.f90`, `morse.f90`, `mm3.f90`, `brenner*.f90`, `Hyper_pot_inner_alg.f90`, `newton_inner.f90`).

### What I learned
- The accepted-state-2 cyclic mismatch is now backed by a concrete source-backed extraction path instead of inference-only C++ probes.
- The first version of the cyclic oracle dump exposed a real bug immediately: the target cyclic element patch references ghost nodes, so a real-node-only dump path is invalid.
- After fixing ghost reconstruction in the Fortran dumper, the extracted cyclic constitutive state still does **not** match the C++ accepted-state-2 element-3200 state. That means the next blocker is no longer “we lack a Fortran extraction path”; it is now “the cyclic oracle extraction path or its mesh/state reader is still wrong, or the live cyclic C++ path is materially diverging earlier than the current probe assumed.”

### Remaining blocker
- `GT-AC1` is still open. The main cyclic executable-path replay lane is still red, and the new oracle dumper is not yet trustworthy enough to freeze as a committed regression fixture.

## BitLesson Delta

Action: none
Lesson ID(s): NONE
Notes: Re-read `.humanize/bitlesson.md` before the work. `bitlesson-selector` was not available in the current shell environment, so there was no selector output or new BitLesson entry for this round.

## Goal Tracker Update Request

### Requested Changes:
- Add to Open Issues: “The new Fortran-side cyclic accepted-state-2 element-3200 oracle extractor still emits a constitutive state that is incompatible with the C++ accepted-state-2 replay state even after canonical ghost-node reconstruction, so the cyclic replay investigation now needs a second pass on the oracle extraction/reader path before freezing a new fixture.”

### Justification:
- This is a new, concrete blocker discovered while turning the cyclic accepted-state-2 top-right-corner probe into a source-backed oracle surface. It narrows the next round away from generic solver speculation and toward either the Fortran extraction path or an earlier-than-expected cyclic state divergence.
