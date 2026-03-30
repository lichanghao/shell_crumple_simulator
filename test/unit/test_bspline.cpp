#include "fce/bspline.hpp"
#include "fce/io.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

#ifndef ORACLE_DIR
#define ORACLE_DIR "test/cases"
#endif

struct BSplineOracleFixture {
    double v{0.0};
    double w{0.0};
    std::array<double, 12> shape{};
    std::array<std::array<double, 2>, 12> first{};
    std::array<std::array<double, 3>, 12> second{};
};

std::array<double, 12> eval_shape(double v, double w)
{
    std::array<double, 12> shape{};
    fce::BSpline(shape, v, w);
    return shape;
}

BSplineOracleFixture load_fixture(const std::filesystem::path& path)
{
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open B-spline oracle fixture: " + path.string());
    }

    BSplineOracleFixture fixture;
    std::string line;
    auto read_tokens = [&](int expected_count) {
        while (std::getline(in, line)) {
            if (line.empty()) {
                continue;
            }
            std::istringstream row(line);
            std::vector<std::string> toks;
            std::string tok;
            while (row >> tok) {
                toks.push_back(tok);
            }
            if (toks.empty()) {
                continue;
            }
            if (static_cast<int>(toks.size()) != expected_count) {
                throw std::runtime_error("Unexpected token count in fixture: " + path.string());
            }
            return toks;
        }
        throw std::runtime_error("Unexpected EOF while reading fixture: " + path.string());
    };

    {
        const auto toks = read_tokens(2);
        fixture.v = fce::io::parse_fortran_double(toks[0]);
        fixture.w = fce::io::parse_fortran_double(toks[1]);
    }
    for (int i = 0; i < 12; ++i) {
        const auto toks = read_tokens(1);
        fixture.shape[i] = fce::io::parse_fortran_double(toks[0]);
    }
    for (int i = 0; i < 12; ++i) {
        const auto toks = read_tokens(2);
        fixture.first[i][0] = fce::io::parse_fortran_double(toks[0]);
        fixture.first[i][1] = fce::io::parse_fortran_double(toks[1]);
    }
    for (int i = 0; i < 12; ++i) {
        const auto toks = read_tokens(3);
        fixture.second[i][0] = fce::io::parse_fortran_double(toks[0]);
        fixture.second[i][1] = fce::io::parse_fortran_double(toks[1]);
        fixture.second[i][2] = fce::io::parse_fortran_double(toks[2]);
    }
    return fixture;
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

TEST(BSpline, OutOfDomainCoordinatesAreRejected)
{
    std::array<double, 12> shape{};
    std::array<std::array<double, 2>, 12> first{};
    std::array<std::array<double, 3>, 12> second{};

    EXPECT_THROW(fce::BSpline(shape, -1e-6, 0.2), std::invalid_argument);
    EXPECT_THROW(fce::BSpline(shape, 0.2, -1e-6), std::invalid_argument);
    EXPECT_THROW(fce::BSpline(shape, 0.6, 0.5), std::invalid_argument);

    EXPECT_THROW(fce::DBSpline(first, -1e-6, 0.2), std::invalid_argument);
    EXPECT_THROW(fce::DBSpline(first, 0.2, -1e-6), std::invalid_argument);
    EXPECT_THROW(fce::DBSpline(first, 0.6, 0.5), std::invalid_argument);

    EXPECT_THROW(fce::DDBSpline(second, -1e-6, 0.2), std::invalid_argument);
    EXPECT_THROW(fce::DDBSpline(second, 0.2, -1e-6), std::invalid_argument);
    EXPECT_THROW(fce::DDBSpline(second, 0.6, 0.5), std::invalid_argument);
}

TEST(BSpline, MatchesCommittedFortranOracleFixtures)
{
    const std::filesystem::path base =
        std::filesystem::path(ORACLE_DIR) / "bspline_oracle";
    const std::vector<std::string> fixture_names{
        "interior_01.dat",
        "interior_02.dat",
        "interior_03.dat",
        "interior_04.dat",
        "interior_05.dat",
        "boundary_01.dat",
        "boundary_02.dat",
        "boundary_03.dat",
        "boundary_04.dat",
        "boundary_05.dat",
    };

    for (const auto& name : fixture_names) {
        const auto fixture = load_fixture(base / name);
        std::array<double, 12> shape{};
        std::array<std::array<double, 2>, 12> first{};
        std::array<std::array<double, 3>, 12> second{};

        fce::BSpline(shape, fixture.v, fixture.w);
        fce::DBSpline(first, fixture.v, fixture.w);
        fce::DDBSpline(second, fixture.v, fixture.w);

        for (int i = 0; i < 12; ++i) {
            EXPECT_NEAR(shape[i], fixture.shape[i], 1e-14) << name << " N[" << i << "]";
            EXPECT_NEAR(first[i][0], fixture.first[i][0], 1e-14) << name << " dN/dv[" << i << "]";
            EXPECT_NEAR(first[i][1], fixture.first[i][1], 1e-14) << name << " dN/dw[" << i << "]";
            EXPECT_NEAR(second[i][0], fixture.second[i][0], 1e-14) << name << " d2N/dv2[" << i << "]";
            EXPECT_NEAR(second[i][1], fixture.second[i][1], 1e-14) << name << " d2N/dw2[" << i << "]";
            EXPECT_NEAR(second[i][2], fixture.second[i][2], 1e-14) << name << " d2N/dvdw[" << i << "]";
        }
    }
}
