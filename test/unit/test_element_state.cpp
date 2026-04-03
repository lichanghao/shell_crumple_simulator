#include "fce/element_state.hpp"
#include "fce/constitutive.hpp"
#include "fce/exponential.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace {

using Vec2 = fce::Vec2;
using Vec3 = fce::Vec3;
using Voigt3 = fce::Voigt3;
using Mat22 = fce::Mat22;
using NeighborCoords12 = fce::NeighborCoords12;
using ShapeGradient12 = fce::ShapeGradient12;
using ShapeCurvature12 = fce::ShapeCurvature12;

fce::MatData oracle_brenner_material() {
    fce::MatData mat;
    mat.nCode_Pot = 2;
    mat.A0 = 0.142;
    mat.A1 = 0.142;
    mat.s0 = 3.0 * std::sqrt(3.0) * mat.A0 * mat.A0 / 2.0;
    mat.E[0] = {std::sqrt(3.0) / 2.0, 0.5};
    mat.E[1] = {-std::sqrt(3.0) / 2.0, 0.5};
    mat.E[2] = {0.0, -1.0};
    mat.Vs = {0.60310500860214233, 26.25, 0.9};
    mat.Va = {0.75400000810623169, 0.149, 0.25};
    return mat;
}

NeighborCoords12 curved_patch() {
    NeighborCoords12 xneigh{};
    xneigh[0] = Vec3{1.2, -0.1, 0.3};
    xneigh[1] = Vec3{-0.2, 0.9, -0.4};
    xneigh[2] = Vec3{0.5, 0.4, 0.7};
    xneigh[3] = Vec3{-0.3, 0.2, 0.1};
    return xneigh;
}

ShapeGradient12 curved_dn() {
    ShapeGradient12 dn{};
    dn[0] = Vec2{0.8, 0.1};
    dn[1] = Vec2{-0.2, 0.9};
    dn[2] = Vec2{0.4, -0.3};
    dn[3] = Vec2{-0.1, 0.2};
    return dn;
}

ShapeCurvature12 curved_ddn() {
    ShapeCurvature12 ddn{};
    ddn[0] = Voigt3{0.25, -0.15, 0.05};
    ddn[1] = Voigt3{-0.1, 0.2, 0.08};
    ddn[2] = Voigt3{0.05, 0.04, -0.12};
    ddn[3] = Voigt3{-0.02, 0.03, 0.07};
    return ddn;
}

double tolerance(double expected) {
    return std::max(1e-10, std::abs(expected) * 1e-8);
}

Voigt3 subtract_voigt(const Voigt3& a, const Voigt3& b) {
    return Voigt3{a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

}  // namespace

TEST(ElementState, CanonicalPipelineMatchesManualGeometryPrincipalComposition) {
    const auto xneigh = curved_patch();
    const auto dn = curved_dn();
    const auto ddn = curved_ddn();
    const Mat22 f0{{Vec2{0.9, 0.1}, Vec2{-0.2, 1.1}}};
    const Voigt3 reference_curvature{0.01, -0.02, 0.005};

    const auto state = fce::compute_element_state(xneigh, dn, ddn, f0, reference_curvature);
    const auto metric = fce::compute_metric(xneigh, dn, f0);
    const auto curvature = fce::compute_curvature(xneigh, ddn, f0, metric.xnor_elem, metric.dnorm);
    const auto principal =
        fce::compute_principal_curvature(metric.C_elem, subtract_voigt(curvature.curv0_elem, reference_curvature));

    for (int i = 0; i < 3; ++i) {
        EXPECT_NEAR(state.C_elem[i], metric.C_elem[i], tolerance(metric.C_elem[i])) << i;
        EXPECT_NEAR(state.curv0_elem[i], curvature.curv0_elem[i] - reference_curvature[i], tolerance(curvature.curv0_elem[i]))
            << i;
    }
    for (int i = 0; i < 2; ++i) {
        EXPECT_NEAR(state.curvppal[i], principal.curvppal[i], tolerance(principal.curvppal[i])) << i;
        for (int j = 0; j < 2; ++j) {
            EXPECT_NEAR(state.vppal[i][j], principal.vppal[i][j], tolerance(principal.vppal[i][j]))
                << i << ", " << j;
        }
    }
    EXPECT_EQ(state.flag_num_diff, principal.flag_num_diff);
}

TEST(ElementState, RelaxedPipelineMatchesManualNewtonSolve) {
    const auto xneigh = curved_patch();
    const auto dn = curved_dn();
    const auto ddn = curved_ddn();
    const Mat22 f0{{Vec2{0.9, 0.1}, Vec2{-0.2, 1.1}}};
    const Voigt3 reference_curvature{0.01, -0.02, 0.005};
    const Vec2 eta0{0.003, -0.002};

    const auto state = fce::compute_element_state(xneigh, dn, ddn, f0, reference_curvature);
    const auto expected =
        fce::solve_inner_newton(state.C_elem, state.curvppal, state.vppal, oracle_brenner_material(), eta0, 1e-8, 100);
    const auto actual = fce::solve_inner_newton_for_element(
        xneigh, dn, ddn, f0, reference_curvature, oracle_brenner_material(), eta0, 1e-8, 100);

    for (int i = 0; i < 2; ++i) {
        EXPECT_NEAR(actual.state.curvppal[i], state.curvppal[i], tolerance(state.curvppal[i])) << i;
        EXPECT_NEAR(actual.inner.eta[i], expected.eta[i], tolerance(expected.eta[i])) << i;
        EXPECT_NEAR(actual.inner.dWdeta[i], expected.dWdeta[i], tolerance(expected.dWdeta[i])) << i;
    }
    for (int i = 0; i < 3; ++i) {
        EXPECT_NEAR(actual.inner.ddWdeta[i], expected.ddWdeta[i], tolerance(expected.ddWdeta[i])) << i;
    }
    for (int i = 0; i < 6; ++i) {
        EXPECT_NEAR(actual.inner.dW_dpe[i], expected.dW_dpe[i], tolerance(expected.dW_dpe[i])) << i;
    }
    EXPECT_NEAR(actual.inner.W, expected.W, tolerance(expected.W));
    EXPECT_EQ(actual.inner.iterations, expected.iterations);
    EXPECT_EQ(actual.inner.fail_mode, expected.fail_mode);
}

TEST(ElementState, BondPreparationMatchesManualComposition) {
    const auto xneigh = curved_patch();
    const auto dn = curved_dn();
    const auto ddn = curved_ddn();
    const Mat22 f0{{Vec2{0.9, 0.1}, Vec2{-0.2, 1.1}}};
    const Vec2 eta{0.003, -0.002};
    const auto material = oracle_brenner_material();
    const auto state = fce::compute_element_state(xneigh, dn, ddn, f0);

    const auto prepared = fce::prepare_bond_state(state, material, eta);
    const auto prepared_with_derivatives = fce::prepare_bond_state_with_derivatives(state, material, eta);

    std::array<double, 3> expected_a_norm{};
    std::array<Vec2, 3> expected_ei{};
    for (int ibond = 0; ibond < 3; ++ibond) {
        expected_ei[ibond] = Vec2{
            material.A0 * material.E[ibond][0] + eta[0],
            material.A0 * material.E[ibond][1] + eta[1],
        };
        expected_a_norm[ibond] =
            std::sqrt(expected_ei[ibond][0] * expected_ei[ibond][0] + expected_ei[ibond][1] * expected_ei[ibond][1]);
        expected_ei[ibond][0] /= expected_a_norm[ibond];
        expected_ei[ibond][1] /= expected_a_norm[ibond];
    }

    const auto expected_bonds =
        fce::compute_deformed_bonds(state.C_elem, state.curvppal, state.vppal, expected_a_norm, expected_ei);
    const auto expected_bonds_with_derivatives = fce::compute_deformed_bonds_with_derivatives(
        state.C_elem,
        state.curvppal,
        state.vppal,
        state.dcurvppaldC,
        state.dcurvppaldk,
        state.dvppaldC,
        state.dvppaldk,
        expected_a_norm,
        expected_ei);

    for (int ibond = 0; ibond < 3; ++ibond) {
        EXPECT_NEAR(prepared.A_norm[ibond], expected_a_norm[ibond], tolerance(expected_a_norm[ibond])) << ibond;
        EXPECT_NEAR(prepared_with_derivatives.A_norm[ibond], expected_a_norm[ibond], tolerance(expected_a_norm[ibond]))
            << ibond;
        for (int idim = 0; idim < 2; ++idim) {
            EXPECT_NEAR(prepared.Ei[ibond][idim], expected_ei[ibond][idim], tolerance(expected_ei[ibond][idim]))
                << ibond << ", " << idim;
            EXPECT_NEAR(prepared_with_derivatives.Ei[ibond][idim],
                        expected_ei[ibond][idim],
                        tolerance(expected_ei[ibond][idim]))
                << ibond << ", " << idim;
        }
    }

    for (int i = 0; i < 6; ++i) {
        EXPECT_NEAR(prepared.bonds.pe[i], expected_bonds.pe[i], tolerance(expected_bonds.pe[i])) << i;
        EXPECT_NEAR(prepared_with_derivatives.bonds.pe[i],
                    expected_bonds_with_derivatives.pe[i],
                    tolerance(expected_bonds_with_derivatives.pe[i]))
            << i;
        for (int j = 0; j < 3; ++j) {
            EXPECT_NEAR(prepared_with_derivatives.bonds.dpedC[i][j],
                        expected_bonds_with_derivatives.dpedC[i][j],
                        tolerance(expected_bonds_with_derivatives.dpedC[i][j]))
                << i << ", dC, " << j;
            EXPECT_NEAR(prepared_with_derivatives.bonds.dpedk[i][j],
                        expected_bonds_with_derivatives.dpedk[i][j],
                        tolerance(expected_bonds_with_derivatives.dpedk[i][j]))
                << i << ", dk, " << j;
        }
    }
}

TEST(ElementState, CanonicalStateOwnsPreparedBondStage) {
    const auto xneigh = curved_patch();
    const auto dn = curved_dn();
    const auto ddn = curved_ddn();
    const Mat22 f0{{Vec2{0.9, 0.1}, Vec2{-0.2, 1.1}}};
    const Vec2 eta{0.003, -0.002};
    const auto material = oracle_brenner_material();
    const auto state = fce::compute_element_state(xneigh, dn, ddn, f0);

    const auto prepared_state = fce::prepare_element_state(state, material, eta);
    const auto expected = fce::prepare_bond_state_with_derivatives(state, material, eta);

    EXPECT_TRUE(prepared_state.has_prepared_bond_state);
    for (int i = 0; i < 2; ++i) {
        EXPECT_NEAR(prepared_state.prepared_eta[i], eta[i], tolerance(eta[i])) << i;
    }
    for (int ibond = 0; ibond < 3; ++ibond) {
        EXPECT_NEAR(prepared_state.prepared_bonds.A_norm[ibond],
                    expected.A_norm[ibond],
                    tolerance(expected.A_norm[ibond]))
            << ibond;
        for (int idim = 0; idim < 2; ++idim) {
            EXPECT_NEAR(prepared_state.prepared_bonds.Ei[ibond][idim],
                        expected.Ei[ibond][idim],
                        tolerance(expected.Ei[ibond][idim]))
                << ibond << ", " << idim;
        }
    }

    for (int i = 0; i < 6; ++i) {
        EXPECT_NEAR(prepared_state.prepared_bonds.bonds.pe[i],
                    expected.bonds.pe[i],
                    tolerance(expected.bonds.pe[i]))
            << i;
        for (int j = 0; j < 3; ++j) {
            EXPECT_NEAR(prepared_state.prepared_bonds.bonds_with_derivatives.dpedC[i][j],
                        expected.bonds.dpedC[i][j],
                        tolerance(expected.bonds.dpedC[i][j]))
                << i << ", dC, " << j;
            EXPECT_NEAR(prepared_state.prepared_bonds.bonds_with_derivatives.dpedk[i][j],
                        expected.bonds.dpedk[i][j],
                        tolerance(expected.bonds.dpedk[i][j]))
                << i << ", dk, " << j;
        }
    }
}
