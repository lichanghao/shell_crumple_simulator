#include "fce/exponential.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace {

using Voigt3 = fce::Voigt3;
using Vec2 = fce::Vec2;
using Mat22 = fce::Mat22;

std::array<double, 3> reference_norms() {
    return {0.142, 0.142, 0.142};
}

std::array<Vec2, 3> reference_bonds() {
    return {
        Vec2{std::sqrt(3.0) / 2.0, 0.5},
        Vec2{-std::sqrt(3.0) / 2.0, 0.5},
        Vec2{0.0, -1.0},
    };
}

double derivative_tolerance(double expected) {
    return std::max(1e-9, std::abs(expected) * 1e-6);
}

}  // namespace

TEST(Exponential, FlatReferenceStateRecoversBondLengthsAndAngles) {
    const auto result = fce::compute_deformed_bonds(
        Voigt3{1.0, 1.0, 0.0},
        Vec2{0.0, 0.0},
        Mat22{{Vec2{1.0, 0.0}, Vec2{0.0, 1.0}}},
        reference_norms(),
        reference_bonds());

    EXPECT_NEAR(result.pe[0], 0.142, 1e-12);
    EXPECT_NEAR(result.pe[1], 0.142, 1e-12);
    EXPECT_NEAR(result.pe[2], 0.142, 1e-12);
    const double angle = 2.0 * 3.14159265358979323846 / 3.0;
    EXPECT_NEAR(result.pe[3], angle, 1e-12);
    EXPECT_NEAR(result.pe[4], angle, 1e-12);
    EXPECT_NEAR(result.pe[5], angle, 1e-12);
}

TEST(Exponential, DirectMetricDerivativesMatchFiniteDifferenceWhenPrincipalTermsAreHeldFixed) {
    const Voigt3 C_elem{1.1, 0.95, 0.08};
    const Vec2 curvppal{0.12, -0.04};
    const Mat22 vppal{{Vec2{0.9659258262890683, 0.2588190451025207},
                       Vec2{-0.2588190451025207, 0.9659258262890683}}};
    const auto zero_vec = std::array<Voigt3, 2>{Voigt3{0.0, 0.0, 0.0}, Voigt3{0.0, 0.0, 0.0}};
    const auto zero_mat = std::array<std::array<Voigt3, 2>, 2>{
        std::array<Voigt3, 2>{Voigt3{0.0, 0.0, 0.0}, Voigt3{0.0, 0.0, 0.0}},
        std::array<Voigt3, 2>{Voigt3{0.0, 0.0, 0.0}, Voigt3{0.0, 0.0, 0.0}},
    };
    const auto base = fce::compute_deformed_bonds_with_derivatives(
        C_elem,
        curvppal,
        vppal,
        zero_vec,
        zero_vec,
        zero_mat,
        zero_mat,
        reference_norms(),
        reference_bonds());
    constexpr double h = 1e-7;

    for (int j = 0; j < 3; ++j) {
        auto plus = C_elem;
        auto minus = C_elem;
        plus[j] += h;
        minus[j] -= h;
        const auto plus_result = fce::compute_deformed_bonds(
            plus, curvppal, vppal, reference_norms(), reference_bonds());
        const auto minus_result = fce::compute_deformed_bonds(
            minus, curvppal, vppal, reference_norms(), reference_bonds());
        for (int i = 0; i < 6; ++i) {
            const double fd = (plus_result.pe[i] - minus_result.pe[i]) / (2.0 * h);
            EXPECT_NEAR(base.dpedC[i][j], fd, derivative_tolerance(fd))
                << "bond_component=" << i << " metric_component=" << j;
        }
    }
}
