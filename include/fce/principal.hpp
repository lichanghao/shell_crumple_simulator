#pragma once

#include "fce/types.hpp"

#include <array>

namespace fce {

using Voigt3 = std::array<double, 3>;
using PrincipalDerivativeVector = std::array<Voigt3, 2>;
using PrincipalDerivativeMatrix = std::array<std::array<Voigt3, 2>, 2>;

struct PrincipalResult {
    Vec2 curvppal{};
    Mat22 vppal{};
    PrincipalDerivativeVector dcurvppaldC{};
    PrincipalDerivativeVector dcurvppaldk{};
    PrincipalDerivativeMatrix dvppaldC{};
    PrincipalDerivativeMatrix dvppaldk{};
    bool flag_num_diff{false};
};

PrincipalResult compute_principal_curvature(const Voigt3& C_elem, const Voigt3& curv0_elem);

}  // namespace fce
