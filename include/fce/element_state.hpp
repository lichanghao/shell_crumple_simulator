#pragma once

#include "fce/constitutive.hpp"
#include "fce/geometry.hpp"
#include "fce/principal.hpp"

namespace fce {

struct ElementState {
    MetricResult metric{};
    CurvatureResult curvature{};
    Voigt3 C_elem{};
    Voigt3 curv0_elem{};
    Vec2 curvppal{};
    Mat22 vppal{};
    PrincipalDerivativeVector dcurvppaldC{};
    PrincipalDerivativeVector dcurvppaldk{};
    PrincipalDerivativeMatrix dvppaldC{};
    PrincipalDerivativeMatrix dvppaldk{};
    bool flag_num_diff{false};
};

struct RelaxedElementState {
    ElementState state{};
    NewtonInnerOutput inner{};
};

ElementState compute_element_state(const NeighborCoords12& xneigh,
                                   const ShapeGradient12& dn,
                                   const ShapeCurvature12& ddn,
                                   const Mat22& f0,
                                   const Voigt3& reference_curvature = {});

RelaxedElementState solve_inner_newton_for_element(const NeighborCoords12& xneigh,
                                                   const ShapeGradient12& dn,
                                                   const ShapeCurvature12& ddn,
                                                   const Mat22& f0,
                                                   const Voigt3& reference_curvature,
                                                   const MatData& mat,
                                                   const Vec2& eta0,
                                                   double crit,
                                                   int max_iter);

}  // namespace fce
