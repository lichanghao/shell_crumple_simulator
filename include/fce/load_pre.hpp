#pragma once
// Boundary condition setup.
// Translates load_pre from load.f90.

#include "fce/types.hpp"
#include "fce/mesh_generator.hpp"

namespace fce {

// Set up BCData for the given mesh and loading parameters.
// x0: flat coord array (real nodes only, 3*numnods doubles).
// nrow, ncol: mesh dimensions.
// nborder: number of ghost border rows (0 in oracle case).
// imesh: sheet index (1-based, relevant for some nCodeLoad values).
// angle2: secondary angle/value (only for nCodeLoad=13, 222, etc.).
// BCData fields nnodBC, ndofBC, nCodeLoad, nloadstep, value must be pre-set.
// This function fills: rotation, xc, mnodBC, mdofBC, mdofOP.
void load_pre(const FlatCoords& x0, const Mesh& mesh, BCData& bc,
              double xlength, double ylength,
              int nrow, int ncol, int nborder, int imesh, double angle2);

// Compute ndofOP by excluding mdofBC from all DOFs.
void compute_mdofOP(BCData& bc, int numnods);

} // namespace fce
