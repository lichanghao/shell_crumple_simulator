#pragma once

#include "fce/principal.hpp"

#include <array>

namespace fce {

using Vec6 = std::array<double, 6>;

struct BondState {
    Vec6 pe{};
};

struct BondStateWithDerivatives {
    Vec6 pe{};
    std::array<Voigt3, 6> dpedC{};
    std::array<Voigt3, 6> dpedk{};
};

BondState compute_deformed_bonds(const Voigt3& C_elem,
                                 const Vec2& curvppal,
                                 const Mat22& vppal,
                                 const std::array<double, 3>& A_norm,
                                 const std::array<Vec2, 3>& Ei);

BondStateWithDerivatives compute_deformed_bonds_with_derivatives(
    const Voigt3& C_elem,
    const Vec2& curvppal,
    const Mat22& vppal,
    const PrincipalDerivativeVector& dcurvppaldC,
    const PrincipalDerivativeVector& dcurvppaldk,
    const PrincipalDerivativeMatrix& dvppaldC,
    const PrincipalDerivativeMatrix& dvppaldk,
    const std::array<double, 3>& A_norm,
    const std::array<Vec2, 3>& Ei);

}  // namespace fce
