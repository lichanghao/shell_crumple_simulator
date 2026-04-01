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

double coupled_derivative_tolerance(double expected) {
    return std::max(5e-9, std::abs(expected) * 2e-6);
}

double fourth_order_central_difference(double minus2, double minus1, double plus1, double plus2, double h) {
    return (minus2 - 8.0 * minus1 + 8.0 * plus1 - plus2) / (12.0 * h);
}

fce::PrincipalResult reference_principal_state() {
    return fce::compute_principal_curvature(Voigt3{1.1, 0.9, 0.1}, Voigt3{0.23, 0.11, 0.04});
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

TEST(Exponential, CoupledMetricAndCurvatureDerivativesMatchFiniteDifference) {
    const Voigt3 C_elem{1.1, 0.9, 0.1};
    const Voigt3 curv0_elem{0.23, 0.11, 0.04};
    const auto principal = reference_principal_state();
    ASSERT_FALSE(principal.flag_num_diff);

    const auto base = fce::compute_deformed_bonds_with_derivatives(
        C_elem,
        principal.curvppal,
        principal.vppal,
        principal.dcurvppaldC,
        principal.dcurvppaldk,
        principal.dvppaldC,
        principal.dvppaldk,
        reference_norms(),
        reference_bonds());
    constexpr double h = 1e-7;

    for (int j = 0; j < 3; ++j) {
        auto plus2 = C_elem;
        auto plus = C_elem;
        auto minus = C_elem;
        auto minus2 = C_elem;
        plus2[j] += 2.0 * h;
        plus[j] += h;
        minus[j] -= h;
        minus2[j] -= 2.0 * h;
        const auto plus2_principal = fce::compute_principal_curvature(plus2, curv0_elem);
        const auto plus_principal = fce::compute_principal_curvature(plus, curv0_elem);
        const auto minus_principal = fce::compute_principal_curvature(minus, curv0_elem);
        const auto minus2_principal = fce::compute_principal_curvature(minus2, curv0_elem);
        ASSERT_FALSE(plus2_principal.flag_num_diff);
        ASSERT_FALSE(plus_principal.flag_num_diff);
        ASSERT_FALSE(minus_principal.flag_num_diff);
        ASSERT_FALSE(minus2_principal.flag_num_diff);
        const auto plus2_result = fce::compute_deformed_bonds(
            plus2, plus2_principal.curvppal, plus2_principal.vppal, reference_norms(), reference_bonds());
        const auto plus_result = fce::compute_deformed_bonds(
            plus, plus_principal.curvppal, plus_principal.vppal, reference_norms(), reference_bonds());
        const auto minus_result = fce::compute_deformed_bonds(
            minus, minus_principal.curvppal, minus_principal.vppal, reference_norms(), reference_bonds());
        const auto minus2_result = fce::compute_deformed_bonds(
            minus2, minus2_principal.curvppal, minus2_principal.vppal, reference_norms(), reference_bonds());
        for (int i = 0; i < 6; ++i) {
            const double fd =
                fourth_order_central_difference(minus2_result.pe[i], minus_result.pe[i], plus_result.pe[i], plus2_result.pe[i], h);
            EXPECT_NEAR(base.dpedC[i][j], fd, coupled_derivative_tolerance(fd))
                << "coupled metric derivative bond_component=" << i << " metric_component=" << j;
        }
    }

    for (int j = 0; j < 3; ++j) {
        auto plus2 = curv0_elem;
        auto plus = curv0_elem;
        auto minus = curv0_elem;
        auto minus2 = curv0_elem;
        plus2[j] += 2.0 * h;
        plus[j] += h;
        minus[j] -= h;
        minus2[j] -= 2.0 * h;
        const auto plus2_principal = fce::compute_principal_curvature(C_elem, plus2);
        const auto plus_principal = fce::compute_principal_curvature(C_elem, plus);
        const auto minus_principal = fce::compute_principal_curvature(C_elem, minus);
        const auto minus2_principal = fce::compute_principal_curvature(C_elem, minus2);
        ASSERT_FALSE(plus2_principal.flag_num_diff);
        ASSERT_FALSE(plus_principal.flag_num_diff);
        ASSERT_FALSE(minus_principal.flag_num_diff);
        ASSERT_FALSE(minus2_principal.flag_num_diff);
        const auto plus2_result = fce::compute_deformed_bonds(
            C_elem, plus2_principal.curvppal, plus2_principal.vppal, reference_norms(), reference_bonds());
        const auto plus_result = fce::compute_deformed_bonds(
            C_elem, plus_principal.curvppal, plus_principal.vppal, reference_norms(), reference_bonds());
        const auto minus_result = fce::compute_deformed_bonds(
            C_elem, minus_principal.curvppal, minus_principal.vppal, reference_norms(), reference_bonds());
        const auto minus2_result = fce::compute_deformed_bonds(
            C_elem, minus2_principal.curvppal, minus2_principal.vppal, reference_norms(), reference_bonds());
        for (int i = 0; i < 6; ++i) {
            const double fd =
                fourth_order_central_difference(minus2_result.pe[i], minus_result.pe[i], plus_result.pe[i], plus2_result.pe[i], h);
            EXPECT_NEAR(base.dpedk[i][j], fd, coupled_derivative_tolerance(fd))
                << "coupled curvature derivative bond_component=" << i << " curvature_component=" << j;
        }
    }
}
