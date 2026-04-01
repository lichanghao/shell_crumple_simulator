#include "fce/principal.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace {

using Voigt3 = fce::Voigt3;
using Vec2 = fce::Vec2;
using Mat22 = fce::Mat22;

double curvature_tolerance(double expected) {
    return std::max(1e-10, std::abs(expected) * 1e-7);
}

}  // namespace

TEST(Principal, DistinctCurvaturesReturnExpectedDirections) {
    const auto result = fce::compute_principal_curvature(Voigt3{1.0, 1.0, 0.0}, Voigt3{0.2, 0.1, 0.0});

    EXPECT_FALSE(result.flag_num_diff);
    EXPECT_NEAR(result.curvppal[0], 0.2, 1e-12);
    EXPECT_NEAR(result.curvppal[1], 0.1, 1e-12);
    EXPECT_NEAR(result.vppal[0][0], 1.0, 1e-12);
    EXPECT_NEAR(result.vppal[0][1], 0.0, 1e-12);
    EXPECT_NEAR(result.vppal[1][0], 0.0, 1e-12);
    EXPECT_NEAR(result.vppal[1][1], 1.0, 1e-12);
}

TEST(Principal, RepeatedCurvaturesTriggerNumericalFallbackFlag) {
    const auto result = fce::compute_principal_curvature(Voigt3{1.0, 1.0, 0.0}, Voigt3{0.15, 0.15, 0.0});

    EXPECT_TRUE(result.flag_num_diff);
    EXPECT_NEAR(result.curvppal[0], 0.15, 1e-12);
    EXPECT_NEAR(result.curvppal[1], 0.15, 1e-12);
    EXPECT_NEAR(result.vppal[0][0], 1.0, 1e-12);
    EXPECT_NEAR(result.vppal[0][1], 0.0, 1e-12);
    EXPECT_NEAR(result.vppal[1][0], 0.0, 1e-12);
    EXPECT_NEAR(result.vppal[1][1], 1.0, 1e-12);
}

TEST(Principal, CurvatureDerivativesMatchFiniteDifference) {
    const Voigt3 C_elem{1.1, 0.9, 0.1};
    const Voigt3 curv0_elem{0.23, 0.11, 0.04};
    const auto base = fce::compute_principal_curvature(C_elem, curv0_elem);
    ASSERT_FALSE(base.flag_num_diff);
    constexpr double h = 1e-7;

    for (int j = 0; j < 3; ++j) {
        auto plus = C_elem;
        auto minus = C_elem;
        plus[j] += h;
        minus[j] -= h;
        const auto plus_result = fce::compute_principal_curvature(plus, curv0_elem);
        const auto minus_result = fce::compute_principal_curvature(minus, curv0_elem);
        for (int i = 0; i < 2; ++i) {
            const double fd = (plus_result.curvppal[i] - minus_result.curvppal[i]) / (2.0 * h);
            EXPECT_NEAR(base.dcurvppaldC[i][j], fd, curvature_tolerance(fd))
                << "curvature=" << i << " component=" << j;
        }
    }

    for (int j = 0; j < 3; ++j) {
        auto plus = curv0_elem;
        auto minus = curv0_elem;
        plus[j] += h;
        minus[j] -= h;
        const auto plus_result = fce::compute_principal_curvature(C_elem, plus);
        const auto minus_result = fce::compute_principal_curvature(C_elem, minus);
        for (int i = 0; i < 2; ++i) {
            const double fd = (plus_result.curvppal[i] - minus_result.curvppal[i]) / (2.0 * h);
            EXPECT_NEAR(base.dcurvppaldk[i][j], fd, curvature_tolerance(fd))
                << "curvature=" << i << " component=" << j;
        }
    }
}
