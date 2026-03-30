#include "fce/bspline.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <vector>

namespace {

std::array<double, 12> eval_shape(double v, double w)
{
    std::array<double, 12> shape{};
    fce::BSpline(shape, v, w);
    return shape;
}

} // namespace

TEST(BSpline, PartitionOfUnityAndDerivativeSumsHoldAtRepresentativePoints)
{
    const std::vector<std::pair<double, double>> sample_points{
        {1.0 / 6.0, 1.0 / 6.0},
        {1.0 / 6.0, 2.0 / 3.0},
        {0.20, 0.30},
        {0.45, 0.10},
    };

    for (const auto& [v, w] : sample_points) {
        std::array<double, 12> shape{};
        std::array<std::array<double, 2>, 12> first{};
        std::array<std::array<double, 3>, 12> second{};
        fce::BSpline(shape, v, w);
        fce::DBSpline(first, v, w);
        fce::DDBSpline(second, v, w);

        double sum_shape = 0.0;
        double sum_dv = 0.0;
        double sum_dw = 0.0;
        double sum_dvv = 0.0;
        double sum_dww = 0.0;
        double sum_dvw = 0.0;
        for (int i = 0; i < 12; ++i) {
            sum_shape += shape[i];
            sum_dv += first[i][0];
            sum_dw += first[i][1];
            sum_dvv += second[i][0];
            sum_dww += second[i][1];
            sum_dvw += second[i][2];
        }

        EXPECT_NEAR(sum_shape, 1.0, 1e-12) << "v=" << v << " w=" << w;
        EXPECT_NEAR(sum_dv, 0.0, 1e-11) << "v=" << v << " w=" << w;
        EXPECT_NEAR(sum_dw, 0.0, 1e-11) << "v=" << v << " w=" << w;
        EXPECT_NEAR(sum_dvv, 0.0, 1e-10) << "v=" << v << " w=" << w;
        EXPECT_NEAR(sum_dww, 0.0, 1e-10) << "v=" << v << " w=" << w;
        EXPECT_NEAR(sum_dvw, 0.0, 1e-10) << "v=" << v << " w=" << w;
    }
}

TEST(BSpline, AnalyticalDerivativesMatchFiniteDifferences)
{
    constexpr double v = 0.31;
    constexpr double w = 0.27;
    constexpr double h = 1e-7;

    std::array<std::array<double, 2>, 12> first{};
    std::array<std::array<double, 3>, 12> second{};
    fce::DBSpline(first, v, w);
    fce::DDBSpline(second, v, w);

    const auto plus_v = eval_shape(v + h, w);
    const auto minus_v = eval_shape(v - h, w);
    const auto plus_w = eval_shape(v, w + h);
    const auto minus_w = eval_shape(v, w - h);

    std::array<std::array<double, 2>, 12> plus_first_v{};
    std::array<std::array<double, 2>, 12> minus_first_v{};
    std::array<std::array<double, 2>, 12> plus_first_w{};
    std::array<std::array<double, 2>, 12> minus_first_w{};
    fce::DBSpline(plus_first_v, v + h, w);
    fce::DBSpline(minus_first_v, v - h, w);
    fce::DBSpline(plus_first_w, v, w + h);
    fce::DBSpline(minus_first_w, v, w - h);

    for (int i = 0; i < 12; ++i) {
        const double fd_dv = (plus_v[i] - minus_v[i]) / (2.0 * h);
        const double fd_dw = (plus_w[i] - minus_w[i]) / (2.0 * h);
        EXPECT_NEAR(first[i][0], fd_dv, 1e-6) << "shape " << i << " dv";
        EXPECT_NEAR(first[i][1], fd_dw, 1e-6) << "shape " << i << " dw";

        const double fd_dvv = (plus_first_v[i][0] - minus_first_v[i][0]) / (2.0 * h);
        const double fd_dww = (plus_first_w[i][1] - minus_first_w[i][1]) / (2.0 * h);
        const double fd_dvw = (plus_first_v[i][1] - minus_first_v[i][1]) / (2.0 * h);
        EXPECT_NEAR(second[i][0], fd_dvv, 1e-4) << "shape " << i << " dvdv";
        EXPECT_NEAR(second[i][1], fd_dww, 1e-4) << "shape " << i << " dwdw";
        EXPECT_NEAR(second[i][2], fd_dvw, 1e-4) << "shape " << i << " dvdw";
    }
}
