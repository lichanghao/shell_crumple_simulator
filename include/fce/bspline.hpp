#pragma once
// B-spline basis functions for 12-node triangular patch.
// Translates BSpline.f90.

#include <array>

namespace fce {

// Compute 12 B-spline shape function values at (v,w).
// N[0..11] correspond to Fortran N(1..12).
void BSpline(std::array<double,12>& N, double v, double w);

// Compute first derivatives dN/dv (col 0) and dN/dw (col 1).
// DN[i][0] = dN_i/dv,  DN[i][1] = dN_i/dw
void DBSpline(std::array<std::array<double,2>,12>& DN, double v, double w);

// Compute second derivatives.
// DDN[i][0] = d²N_i/dvdv,  [1] = d²N_i/dwdw,  [2] = d²N_i/dvdw
void DDBSpline(std::array<std::array<double,3>,12>& DDN, double v, double w);

} // namespace fce
