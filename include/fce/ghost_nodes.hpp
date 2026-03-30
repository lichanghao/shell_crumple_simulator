#pragma once
// Ghost node connectivity and position computation.
// Translates connect_mesh, connect_orig_mesh, ghost_nodes from connect_mesh.f90.

#include "fce/types.hpp"
#include "fce/mesh_generator.hpp"

namespace fce {

// Build full B-spline 12-node connectivity for each element in meshh.
// Operates on the EXTENDED ghost mesh.
// Fills meshh.connect[*].neigh_elem, neigh_vert, num_neigh_elem, num_neigh_vert.
void connect_mesh(Mesh& meshh);

// Map ghost mesh connectivity back to original mesh.
// mesh0: original (nrow×ncol) mesh, already has vertices set.
// meshg: extended ((nrow+2)×(ncol+2)) mesh, already connected.
// Fills mesh0.connect[*].neigh_elem, neigh_vert + mesh0.nghost_tab.
void connect_orig_mesh(Mesh& mesh0, const Mesh& meshg, int ncol, int nrow);

// Compute ghost node positions using parallelogram rule.
// x0 must be sized 3*(mesh0.numnods + mesh0.nedge); ghost positions are appended.
// All indices in nghost_tab are 0-based.
void ghost_nodes(const Mesh& meshh, FlatCoords& x0);

} // namespace fce
