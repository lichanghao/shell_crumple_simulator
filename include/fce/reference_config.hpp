#pragma once
// Reference deformation gradient computation.
// Translates Def_Grad_Cart_Conv from Def_Grad.f90.

#include "fce/types.hpp"
#include "fce/mesh_generator.hpp"
#include <vector>

namespace fce {

// Compute F0 and J0 for all elements.
// x0: flat coord array, 3 values per node (0-based indexing: x0[node*3+k])
// Returns vector of RefConfig (one per element).
std::vector<RefConfig> compute_ref_config(const Mesh& mesh,
                                          const FlatCoords& x0);

} // namespace fce
