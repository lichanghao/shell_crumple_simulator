#pragma once

#include "fce/mesh_generator.hpp"
#include "fce/types.hpp"

#include <utility>
#include <vector>

namespace fce {

void compute_vdw_cutoff(VdwData& vdw);
void setup_vdw_quadrature(VdwData& vdw);

void initialize_preprocessor_vdw(VdwData& vdw,
                                 const Mesh& mesh,
                                 const FlatCoords& coords,
                                 const MatData& mat,
                                 double sheet_xlength,
                                 double sheet_ylength,
                                 double twist_angle_radians,
                                 bool use_atomic_density);

std::vector<std::pair<int, int>> build_tub_partitions(
    const std::vector<int>& elements_per_sheet,
    int ngauss_vdw);

} // namespace fce
