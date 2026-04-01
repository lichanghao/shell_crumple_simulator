#pragma once

#include "fce/principal.hpp"

#include <array>

namespace fce {

using NeighborCoords12 = std::array<Vec3, 12>;
using ShapeGradient12 = std::array<Vec2, 12>;
using ShapeCurvature12 = std::array<Voigt3, 12>;
using NodeDerivativeVoigt = std::array<std::array<Voigt3, 3>, 12>;
using NodeDerivativeVec3 = std::array<std::array<Vec3, 3>, 12>;

struct MetricResult {
    Voigt3 C_elem{};
    NodeDerivativeVoigt dC{};
    Vec3 xnor_elem{};
    NodeDerivativeVec3 dnorm{};
};

struct CurvatureResult {
    Voigt3 curv0_elem{};
    NodeDerivativeVoigt dcurv{};
};

MetricResult compute_metric(const NeighborCoords12& xneigh, const ShapeGradient12& dn, const Mat22& f0);

CurvatureResult compute_curvature(const NeighborCoords12& xneigh,
                                  const ShapeCurvature12& ddn,
                                  const Mat22& f0,
                                  const Vec3& xnor_elem,
                                  const NodeDerivativeVec3& dnorm);

}  // namespace fce
