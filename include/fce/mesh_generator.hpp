#pragma once
// Mesh generation for square triangulated meshes.
// Translates mesh_gen_square / meshg_gen_square from Fortran.

#include "fce/types.hpp"
#include <vector>

namespace fce {

// Nodal positions flat array (Fortran layout): x0[3*numnods]
// x0[inode*3+0]=x, x0[inode*3+1]=y, x0[inode*3+2]=z
using FlatCoords = std::vector<double>;

// Generate a square (nrow × ncol) triangulated mesh.
// Returns nodal coords in x0 (3*(numnods) doubles) and fills mesh.
// mesh.numnods, mesh.numele are set on return.
void mesh_gen_square(int nrow, int ncol,
                     double xlength, double ylength,
                     FlatCoords& x0, Mesh& mesh);

// Same but for extended ghost mesh (nrowg × ncolg).
void meshg_gen_square(int nrowg, int ncolg,
                      double xlength, double ylength,
                      FlatCoords& xg, Mesh& meshg);

} // namespace fce
