#pragma once
// Gauss quadrature setup for triangular elements.
// Translates gauss() from gauss.f90.

#include <array>
#include <vector>

namespace fce {

// shapef[ig][i][k]:
//   ig = Gauss point index (0..ngauss-1)
//   i  = shape function index (0..11)
//   k  = 0: N,  1: dN/dv,  2: dN/dw,  3: d²N/dvdv,  4: d²N/dwdw,  5: d²N/dvdw
using ShapeFData = std::vector<std::array<std::array<double,6>,12>>;

struct GaussData {
    int          ngauss{2};
    ShapeFData   shapef;    // size [ngauss]
    std::vector<double> weight; // size [ngauss]
};

// Fill GaussData for the given ngauss (1, 2, or 3 supported).
GaussData setup_gauss(int ngauss);

} // namespace fce
