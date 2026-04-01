#include "fce/constitutive.hpp"
#include "fce/io.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef ORACLE_DIR
#define ORACLE_DIR "test/cases"
#endif

namespace {

namespace fs = std::filesystem;

using Vec2 = fce::Vec2;
using Vec6 = std::array<double, 6>;
using Mat22 = std::array<std::array<double, 2>, 2>;
using Sym22 = std::array<double, 3>;
using Hessian6 = std::array<Vec6, 6>;

std::vector<std::vector<double>> read_rows(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open fixture: " + path.string());
    }

    std::vector<std::vector<double>> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::vector<double> row;
        double value = 0.0;
        while (iss >> value) {
            row.push_back(value);
        }
        if (!row.empty()) {
            rows.push_back(row);
        }
    }
    return rows;
}

Vec6 row_to_vec6(const std::vector<double>& row) {
    if (row.size() != 6U) {
        throw std::runtime_error("expected 6-column row");
    }
    Vec6 result{};
    std::copy(row.begin(), row.end(), result.begin());
    return result;
}

Vec2 row_to_vec2(const std::vector<double>& row) {
    if (row.size() != 2U) {
        throw std::runtime_error("expected 2-column row");
    }
    return Vec2{row[0], row[1]};
}

Sym22 row_to_sym22(const std::vector<double>& row) {
    if (row.size() != 3U) {
        throw std::runtime_error("expected 3-column row");
    }
    return Sym22{row[0], row[1], row[2]};
}

Mat22 rows_to_mat22(const std::vector<double>& row0, const std::vector<double>& row1) {
    if (row0.size() != 2U || row1.size() != 2U) {
        throw std::runtime_error("expected 2x2 matrix rows");
    }
    return Mat22{{Vec2{row0[0], row0[1]}, Vec2{row1[0], row1[1]}}};
}

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

struct BrennerFixture {
    Vec6 pe{};
    double W{0.0};
    Vec6 dW{};
    Hessian6 ddW{};
};

BrennerFixture read_brenner_fixture(const fs::path& path) {
    const auto rows = read_rows(path);
    if (rows.size() != 9U) {
        throw std::runtime_error("unexpected Brenner fixture row count");
    }

    BrennerFixture fixture;
    fixture.pe = row_to_vec6(rows[0]);
    fixture.W = rows[1].at(0);
    fixture.dW = row_to_vec6(rows[2]);
    for (int i = 0; i < 6; ++i) {
        fixture.ddW[i] = row_to_vec6(rows[static_cast<std::size_t>(3 + i)]);
    }
    return fixture;
}

struct NewtonFixture {
    Sym22 C_elem{};
    Vec2 curvppal{};
    Mat22 vppal{};
    Vec2 eta0{};
    double crit{0.0};
    int max_iter{0};
    int iterations{0};
    int fail_mode{0};
    Vec2 eta{};
    double W{0.0};
    Vec2 dWdeta{};
    Sym22 ddWdeta{};
    Vec6 dW_dpe{};
};

NewtonFixture read_newton_fixture(const fs::path& path) {
    const auto rows = read_rows(path);
    if (rows.size() != 12U) {
        throw std::runtime_error("unexpected Newton fixture row count");
    }

    NewtonFixture fixture;
    fixture.C_elem = row_to_sym22(rows[0]);
    fixture.curvppal = row_to_vec2(rows[1]);
    fixture.vppal = rows_to_mat22(rows[2], rows[3]);
    fixture.eta0 = row_to_vec2(rows[4]);
    fixture.crit = rows[5].at(0);
    fixture.max_iter = static_cast<int>(rows[5].at(1));
    fixture.iterations = static_cast<int>(rows[6].at(0));
    fixture.fail_mode = static_cast<int>(rows[6].at(1));
    fixture.eta = row_to_vec2(rows[7]);
    fixture.W = rows[8].at(0);
    fixture.dWdeta = row_to_vec2(rows[9]);
    fixture.ddWdeta = row_to_sym22(rows[10]);
    fixture.dW_dpe = row_to_vec6(rows[11]);
    return fixture;
}

std::vector<fs::path> sorted_fixture_paths(const fs::path& dir) {
    std::vector<fs::path> result;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".dat") {
            result.push_back(entry.path());
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

double relative_or_absolute_tolerance(double expected,
                                      double abs_tol = 1e-10,
                                      double rel_tol = 1e-8) {
    return std::max(abs_tol, std::abs(expected) * rel_tol);
}

}  // namespace

TEST(Brenner, MatchesCommittedFortranOracleFixtures) {
    const fs::path fixture_dir = fs::path(ORACLE_DIR) / "constitutive_oracle" / "brenner";
    const auto fixtures = sorted_fixture_paths(fixture_dir);
    ASSERT_GE(fixtures.size(), 10U);

    const auto material = oracle_brenner_material();
    for (const auto& path : fixtures) {
        const auto fixture = read_brenner_fixture(path);
        const auto result = fce::evaluate_brenner(material, fixture.pe);

        EXPECT_NEAR(result.W, fixture.W, relative_or_absolute_tolerance(fixture.W)) << path.string();
        for (int i = 0; i < 6; ++i) {
            EXPECT_NEAR(result.dW[i], fixture.dW[i], relative_or_absolute_tolerance(fixture.dW[i]))
                << path.string() << " dW[" << i << "]";
        }
        for (int i = 0; i < 6; ++i) {
            for (int j = 0; j < 6; ++j) {
                EXPECT_NEAR(result.ddW[i][j],
                            fixture.ddW[i][j],
                            relative_or_absolute_tolerance(fixture.ddW[i][j], 1e-10, 1e-7))
                    << path.string() << " ddW[" << i << "][" << j << "]";
            }
        }
    }
}

TEST(Brenner, HessianMatchesFiniteDifference) {
    const auto material = oracle_brenner_material();
    const Vec6 pe{0.143, 0.1425, 0.141, 2.05, 2.11, 2.18};
    const auto base = fce::evaluate_brenner(material, pe);
    constexpr double h = 1e-7;

    for (int j = 0; j < 6; ++j) {
        auto plus = pe;
        auto minus = pe;
        plus[j] += h;
        minus[j] -= h;
        const auto plus_result = fce::evaluate_brenner(material, plus);
        const auto minus_result = fce::evaluate_brenner(material, minus);
        for (int i = 0; i < 6; ++i) {
            const double fd = (plus_result.dW[i] - minus_result.dW[i]) / (2.0 * h);
            EXPECT_NEAR(base.ddW[i][j], fd, std::max(1e-7, std::abs(fd) * 1e-5))
                << "i=" << i << " j=" << j;
        }
    }
}

TEST(Brenner, ReturnsZeroBeyondCutoffRadius) {
    const auto material = oracle_brenner_material();
    const Vec6 pe{0.171, 0.142, 0.142, 2.0943951023931953, 2.0943951023931953, 2.0943951023931953};
    const auto result = fce::evaluate_brenner(material, pe);

    EXPECT_DOUBLE_EQ(result.W, 0.0);
    for (int i = 0; i < 6; ++i) {
        EXPECT_DOUBLE_EQ(result.dW[i], 0.0) << "dW[" << i << "]";
        for (int j = 0; j < 6; ++j) {
            EXPECT_DOUBLE_EQ(result.ddW[i][j], 0.0) << "ddW[" << i << "][" << j << "]";
        }
    }
}

TEST(Brenner, RejectsZeroNormBondLength) {
    const auto material = oracle_brenner_material();
    const Vec6 pe{0.0, 0.142, 0.142, 2.0943951023931953, 2.0943951023931953, 2.0943951023931953};

    EXPECT_THROW((void)fce::evaluate_brenner(material, pe), std::invalid_argument);
}

TEST(Brenner, DefaultMaterialUsesSupportedPotentialCode) {
    EXPECT_EQ(fce::MatData{}.nCode_Pot, 1);
}

TEST(NewtonInner, MatchesCommittedFortranOracleFixtures) {
    const fs::path fixture_dir = fs::path(ORACLE_DIR) / "constitutive_oracle" / "newton_inner";
    const auto fixtures = sorted_fixture_paths(fixture_dir);
    ASSERT_GE(fixtures.size(), 10U);

    const auto material = oracle_brenner_material();
    bool saw_fail_mode_1 = false;
    bool saw_fail_mode_2 = false;
    bool saw_fail_mode_3 = false;
    for (const auto& path : fixtures) {
        const auto fixture = read_newton_fixture(path);
        const auto result = fce::solve_inner_newton(
            fixture.C_elem,
            fixture.curvppal,
            fixture.vppal,
            material,
            fixture.eta0,
            fixture.crit,
            fixture.max_iter);
        const double eta_tol = fixture.fail_mode == 0 ? 1e-10 : 5e-10;
        const double scalar_tol = fixture.fail_mode == 0
                                      ? std::max(1e-10, std::abs(fixture.W) * 1e-8)
                                      : std::max(1e-8, std::abs(fixture.W) * 1e-6);
        const double grad_tol_0 = fixture.fail_mode == 0
                                      ? 1e-10
                                      : std::max(1e-8, std::abs(fixture.dWdeta[0]) * 1e-6);
        const double grad_tol_1 = fixture.fail_mode == 0
                                      ? 1e-10
                                      : std::max(1e-8, std::abs(fixture.dWdeta[1]) * 1e-6);

        EXPECT_EQ(result.iterations, fixture.iterations) << path.string();
        EXPECT_EQ(result.fail_mode, fixture.fail_mode) << path.string();
        EXPECT_NEAR(result.eta[0], fixture.eta[0], eta_tol) << path.string();
        EXPECT_NEAR(result.eta[1], fixture.eta[1], eta_tol) << path.string();
        EXPECT_NEAR(result.W, fixture.W, scalar_tol) << path.string();
        EXPECT_NEAR(result.dWdeta[0], fixture.dWdeta[0], grad_tol_0) << path.string();
        EXPECT_NEAR(result.dWdeta[1], fixture.dWdeta[1], grad_tol_1) << path.string();
        for (int i = 0; i < 3; ++i) {
            const double dd_tol = fixture.fail_mode == 0
                                      ? 2e-10
                                      : std::max(1e-7, std::abs(fixture.ddWdeta[i]) * 1e-6);
            EXPECT_NEAR(result.ddWdeta[i], fixture.ddWdeta[i], dd_tol)
                << path.string() << " ddWdeta[" << i << "]";
        }
        for (int i = 0; i < 6; ++i) {
            const double dW_tol = fixture.fail_mode == 0
                                      ? std::max(1e-10, std::abs(fixture.dW_dpe[i]) * 1e-8)
                                      : std::max(1e-8, std::abs(fixture.dW_dpe[i]) * 1e-6);
            EXPECT_NEAR(result.dW_dpe[i], fixture.dW_dpe[i], dW_tol)
                << path.string() << " dW_dpe[" << i << "]";
        }
        saw_fail_mode_1 = saw_fail_mode_1 || fixture.fail_mode == 1;
        saw_fail_mode_2 = saw_fail_mode_2 || fixture.fail_mode == 2;
        saw_fail_mode_3 = saw_fail_mode_3 || fixture.fail_mode == 3;
    }

    EXPECT_TRUE(saw_fail_mode_1);
    EXPECT_TRUE(saw_fail_mode_2);
    EXPECT_TRUE(saw_fail_mode_3);
}

TEST(NewtonInner, ReportsFailModeOneForSingularHessian) {
    const auto material = oracle_brenner_material();
    const auto result = fce::solve_inner_newton(
        Sym22{0.8, 0.8, 0.0},
        Vec2{-0.2, -0.2},
        Mat22{{Vec2{-1.0, 0.0}, Vec2{-1.0, 0.0}}},
        material,
        Vec2{-0.02, -0.02},
        1e-8,
        5);

    EXPECT_EQ(result.fail_mode, 1);
    EXPECT_EQ(result.iterations, 1);
}

TEST(NewtonInner, ReportsFailModeTwoWhenStepLimitIsExceeded) {
    const auto material = oracle_brenner_material();
    const auto result = fce::solve_inner_newton(
        Sym22{0.7, 0.7, -0.3},
        Vec2{-0.2, -0.2},
        Mat22{{Vec2{1.0, 0.0}, Vec2{0.0, 1.0}}},
        material,
        Vec2{-0.12, -0.12},
        1e-8,
        20);

    EXPECT_EQ(result.fail_mode, 2);
    EXPECT_EQ(result.iterations, 1);
}

TEST(NewtonInner, RejectsUnsupportedPotentialCode) {
    auto material = oracle_brenner_material();
    material.nCode_Pot = 99;

    EXPECT_THROW(
        (void)fce::solve_inner_newton(
            Sym22{1.0, 1.0, 0.0},
            Vec2{0.0, 0.0},
            Mat22{{Vec2{1.0, 0.0}, Vec2{0.0, 1.0}}},
            material,
            Vec2{0.0, 0.0},
            1e-8,
            5),
        std::invalid_argument);
}

TEST(NewtonInner, AcceptsCommittedCompressionMaterialPayload) {
    const auto general =
        fce::io::read_general((fs::path(ORACLE_DIR) / "graphene_compression_prepro" / "nano_general.dat").string());

    const auto result = fce::solve_inner_newton(
        Sym22{1.0, 1.0, 0.0},
        Vec2{0.0, 0.0},
        Mat22{{Vec2{1.0, 0.0}, Vec2{0.0, 1.0}}},
        general.mat,
        Vec2{0.0, 0.0},
        1e-8,
        8);

    EXPECT_EQ(result.fail_mode, 0);
    EXPECT_EQ(result.iterations, 0);
    EXPECT_NEAR(result.W, 0.0, 1e-12);
    EXPECT_NEAR(result.dWdeta[0], 0.0, 1e-12);
    EXPECT_NEAR(result.dWdeta[1], 0.0, 1e-12);
}
