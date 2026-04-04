#pragma once
// Per-element energy and force kernel.
// Direct translation of ener_elem.f90 from the Fortran reference simulator.

#include "fce/geometry.hpp"
#include "fce/quadrature.hpp"
#include "fce/types.hpp"

#include <array>
#include <vector>

namespace fce {

struct ElementEnergyResult {
    double W_elem{0.0};
    std::array<std::array<double, 3>, 12> f_elem{};
    std::vector<Vec2> eta{};   // size ngauss — updated etas (only updated on Newton convergence)
    int inner_fail{0};         // count of Gauss points where Newton did not converge
};

// Compute per-element energy and nodal forces for one element.
//
// xneigh       : 12-node B-spline patch coordinates (nm)
// f0           : reference deformation gradient (2×2)
// reference_curvature : reference curvature subtracted from geometric curvature (Voigt3)
// gauss        : Gauss quadrature data (shapef, weights)
// mat          : material parameters (nCode_Pot selects Morse or Brenner)
// nW_hat       : true = inner Newton relaxation active; false = use eta0 as-is
// crit         : Newton convergence criterion
// max_iter     : Newton max iterations
// eta0         : initial inner displacements per Gauss point (size ngauss)
//
// Returns W_elem, f_elem (12×3), updated eta per Gauss point, and inner_fail count.
ElementEnergyResult compute_element_energy(const NeighborCoords12& xneigh,
                                           const Mat22& f0,
                                           const Voigt3& reference_curvature,
                                           const GaussData& gauss,
                                           const MatData& mat,
                                           bool nW_hat,
                                           double crit,
                                           int max_iter,
                                           const std::vector<Vec2>& eta0);

}  // namespace fce
