#include "fce/geometry.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace {

using Vec2 = fce::Vec2;
using Vec3 = fce::Vec3;
using Voigt3 = fce::Voigt3;
using Mat22 = fce::Mat22;
using NeighborCoords12 = fce::NeighborCoords12;
using ShapeGradient12 = fce::ShapeGradient12;
using ShapeCurvature12 = fce::ShapeCurvature12;

double tolerance(double expected) {
    return std::max(1e-9, std::abs(expected) * 1e-6);
}

NeighborCoords12 flat_patch() {
    NeighborCoords12 xneigh{};
    xneigh[0] = Vec3{1.0, 0.0, 0.0};
    xneigh[1] = Vec3{0.0, 1.0, 0.0};
    return xneigh;
}

ShapeGradient12 flat_dn() {
    ShapeGradient12 dn{};
    dn[0] = Vec2{1.0, 0.0};
    dn[1] = Vec2{0.0, 1.0};
    return dn;
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

}  // namespace

TEST(Geometry, MetricFlatReferencePatchProducesIdentityMetricAndUpwardNormal) {
    const auto result = fce::compute_metric(flat_patch(), flat_dn(), Mat22{{Vec2{1.0, 0.0}, Vec2{0.0, 1.0}}});

    EXPECT_NEAR(result.C_elem[0], 1.0, 1e-12);
    EXPECT_NEAR(result.C_elem[1], 1.0, 1e-12);
    EXPECT_NEAR(result.C_elem[2], 0.0, 1e-12);
    EXPECT_NEAR(result.xnor_elem[0], 0.0, 1e-12);
    EXPECT_NEAR(result.xnor_elem[1], 0.0, 1e-12);
    EXPECT_NEAR(result.xnor_elem[2], 1.0, 1e-12);
}

TEST(Geometry, MetricDerivativesMatchFiniteDifference) {
    const auto xneigh = curved_patch();
    const auto dn = curved_dn();
    const Mat22 f0{{Vec2{0.9, 0.1}, Vec2{-0.2, 1.1}}};
    const auto base = fce::compute_metric(xneigh, dn, f0);
    constexpr double h = 1e-7;

    for (int inode = 0; inode < 4; ++inode) {
        for (int idof = 0; idof < 3; ++idof) {
            auto plus = xneigh;
            auto minus = xneigh;
            plus[inode][idof] += h;
            minus[inode][idof] -= h;
            const auto plus_result = fce::compute_metric(plus, dn, f0);
            const auto minus_result = fce::compute_metric(minus, dn, f0);
            for (int component = 0; component < 3; ++component) {
                const double fd = (plus_result.C_elem[component] - minus_result.C_elem[component]) / (2.0 * h);
                EXPECT_NEAR(base.dC[inode][idof][component], fd, tolerance(fd))
                    << "inode=" << inode << " idof=" << idof << " metric_component=" << component;
            }
            for (int component = 0; component < 3; ++component) {
                const double fd = (plus_result.xnor_elem[component] - minus_result.xnor_elem[component]) / (2.0 * h);
                EXPECT_NEAR(base.dnorm[inode][idof][component], fd, tolerance(fd))
                    << "inode=" << inode << " idof=" << idof << " normal_component=" << component;
            }
        }
    }
}

TEST(Geometry, CurvatureDerivativesMatchFiniteDifference) {
    const auto xneigh = curved_patch();
    const auto dn = curved_dn();
    const auto ddn = curved_ddn();
    const Mat22 f0{{Vec2{0.9, 0.1}, Vec2{-0.2, 1.1}}};
    const auto metric = fce::compute_metric(xneigh, dn, f0);
    const auto base = fce::compute_curvature(xneigh, ddn, f0, metric.xnor_elem, metric.dnorm);
    constexpr double h = 1e-7;

    for (int inode = 0; inode < 4; ++inode) {
        for (int idof = 0; idof < 3; ++idof) {
            auto plus = xneigh;
            auto minus = xneigh;
            plus[inode][idof] += h;
            minus[inode][idof] -= h;
            const auto plus_metric = fce::compute_metric(plus, dn, f0);
            const auto minus_metric = fce::compute_metric(minus, dn, f0);
            const auto plus_result = fce::compute_curvature(plus, ddn, f0, plus_metric.xnor_elem, plus_metric.dnorm);
            const auto minus_result = fce::compute_curvature(minus, ddn, f0, minus_metric.xnor_elem, minus_metric.dnorm);
            for (int component = 0; component < 3; ++component) {
                const double fd = (plus_result.curv0_elem[component] - minus_result.curv0_elem[component]) / (2.0 * h);
                EXPECT_NEAR(base.dcurv[inode][idof][component], fd, tolerance(fd))
                    << "inode=" << inode << " idof=" << idof << " curvature_component=" << component;
            }
        }
    }
}
