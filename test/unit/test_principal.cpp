#include "fce/exponential.hpp"
#include "fce/io.hpp"
#include "fce/principal.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef ORACLE_DIR
#define ORACLE_DIR "test/cases"
#endif

namespace {

using Voigt3 = fce::Voigt3;
using Vec2 = fce::Vec2;
using Mat22 = fce::Mat22;

double curvature_tolerance(double expected) {
    return std::max(1e-10, std::abs(expected) * 1e-7);
}

Vec2 align_direction(const Vec2& reference, Vec2 candidate) {
    const double dot = reference[0] * candidate[0] + reference[1] * candidate[1];
    if (dot < 0.0) {
        candidate[0] = -candidate[0];
        candidate[1] = -candidate[1];
    }
    return candidate;
}

namespace fs = std::filesystem;

// ── Fixture reader for principal_exponential_oracle ──────────────────────────

struct PrincipalExponentialFixture {
    int element_index{0};  // 0-based
    int gauss_index{0};    // 0-based
    fce::Voigt3 C_elem{};
    fce::Voigt3 curv0_elem{};
    bool flag_num_diff{false};
    fce::Vec2 curvppal{};
    fce::Mat22 vppal{};
    fce::PrincipalDerivativeVector dcurvppaldC{};
    fce::PrincipalDerivativeVector dcurvppaldk{};
    fce::PrincipalDerivativeMatrix dvppaldC{};
    fce::PrincipalDerivativeMatrix dvppaldk{};
    std::array<double, 3> A_norm{};
    std::array<fce::Vec2, 3> Ei{};
    fce::Vec6 pe{};
    std::array<fce::Voigt3, 6> dpedC{};
    std::array<fce::Voigt3, 6> dpedk{};
};

static std::vector<std::vector<double>> pexp_read_rows(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open: " + path.string());
    }
    std::vector<std::vector<double>> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream iss(line);
        std::vector<double> row;
        std::string token;
        while (iss >> token) {
            row.push_back(fce::io::parse_fortran_double(token));
        }
        if (!row.empty()) {
            rows.push_back(row);
        }
    }
    return rows;
}

static std::vector<fs::path> pexp_sorted_fixture_paths(const fs::path& dir) {
    std::vector<fs::path> result;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".dat") {
            result.push_back(entry.path());
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

static fce::Voigt3 pexp_row3(const std::vector<double>& row) {
    if (row.size() != 3U) throw std::runtime_error("expected 3-column row");
    return fce::Voigt3{row[0], row[1], row[2]};
}

static fce::Vec2 pexp_row2(const std::vector<double>& row) {
    if (row.size() != 2U) throw std::runtime_error("expected 2-column row");
    return fce::Vec2{row[0], row[1]};
}

static PrincipalExponentialFixture read_pexp_fixture(const fs::path& path) {
    const auto rows = pexp_read_rows(path);
    if (rows.size() != 36U) {
        throw std::runtime_error("unexpected principal/exponential fixture row count: " +
                                 std::to_string(rows.size()) + " in " + path.string());
    }

    PrincipalExponentialFixture f;
    f.element_index = static_cast<int>(rows[0].at(0)) - 1;
    f.gauss_index   = static_cast<int>(rows[0].at(1)) - 1;
    f.C_elem        = pexp_row3(rows[1]);
    f.curv0_elem    = pexp_row3(rows[2]);
    f.flag_num_diff = (static_cast<int>(rows[3].at(0)) != 0);
    f.curvppal      = pexp_row2(rows[4]);
    f.vppal         = fce::Mat22{{pexp_row2(rows[5]), pexp_row2(rows[6])}};
    // Principal derivatives: rows 7-18
    f.dcurvppaldC[0] = pexp_row3(rows[7]);
    f.dcurvppaldC[1] = pexp_row3(rows[8]);
    f.dcurvppaldk[0] = pexp_row3(rows[9]);
    f.dcurvppaldk[1] = pexp_row3(rows[10]);
    // dvppaldC(ipp, jpp) → dvppaldC[ipp][jpp_component]
    f.dvppaldC[0][0] = pexp_row3(rows[11]);  // dvppaldC(1,1)
    f.dvppaldC[0][1] = pexp_row3(rows[12]);  // dvppaldC(1,2)
    f.dvppaldC[1][0] = pexp_row3(rows[13]);  // dvppaldC(2,1)
    f.dvppaldC[1][1] = pexp_row3(rows[14]);  // dvppaldC(2,2)
    f.dvppaldk[0][0] = pexp_row3(rows[15]);  // dvppaldk(1,1)
    f.dvppaldk[0][1] = pexp_row3(rows[16]);  // dvppaldk(1,2)
    f.dvppaldk[1][0] = pexp_row3(rows[17]);  // dvppaldk(2,1)
    f.dvppaldk[1][1] = pexp_row3(rows[18]);  // dvppaldk(2,2)
    // Bond geometry: rows 19-22
    f.A_norm = {rows[19].at(0), rows[19].at(1), rows[19].at(2)};
    f.Ei[0]  = pexp_row2(rows[20]);
    f.Ei[1]  = pexp_row2(rows[21]);
    f.Ei[2]  = pexp_row2(rows[22]);
    // Exponential outputs: rows 23-35
    if (rows[23].size() != 6U) throw std::runtime_error("expected 6-column pe row");
    for (int i = 0; i < 6; ++i) f.pe[i] = rows[23].at(static_cast<std::size_t>(i));
    for (int i = 0; i < 6; ++i) f.dpedC[i] = pexp_row3(rows[24 + i]);
    for (int i = 0; i < 6; ++i) f.dpedk[i] = pexp_row3(rows[30 + i]);
    return f;
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

TEST(Principal, NearlyRepeatedCurvaturesUseNumericalFallback) {
    const auto result =
        fce::compute_principal_curvature(Voigt3{1.0, 1.0, 0.0}, Voigt3{0.15 + 1e-12, 0.15 - 1e-12, 0.0});

    EXPECT_TRUE(result.flag_num_diff);
    EXPECT_TRUE(std::isfinite(result.curvppal[0]));
    EXPECT_TRUE(std::isfinite(result.curvppal[1]));
}

TEST(Principal, NonFiniteDiscriminantThrows) {
    const double huge = std::numeric_limits<double>::max();
    EXPECT_THROW(
        (void)fce::compute_principal_curvature(Voigt3{1.0, 1.0, 0.0}, Voigt3{huge, huge, 0.0}),
        std::invalid_argument);
}

TEST(Principal, MatchesArchivedCompressionFortranOracle) {
    // Compares compute_principal_curvature against Fortran-derived fixtures for
    // elements 83-87 of the archived compression simulator state (10 cases).
    // All cases have flag_num_diff=false (distinct curvatures, analytical path).
    // Tolerance 1e-12: both paths are analytical with identical floating-point operations.
    const fs::path fixture_dir =
        fs::path(ORACLE_DIR) / "principal_exponential_oracle";
    const auto fixtures = pexp_sorted_fixture_paths(fixture_dir);
    ASSERT_GE(fixtures.size(), 10U);

    constexpr double tol = 1e-12;

    for (const auto& path : fixtures) {
        const auto f = read_pexp_fixture(path);

        // Non-triviality guard: archived geometry must be genuinely deformed.
        ASSERT_GT(std::abs(f.curv0_elem[0]), 1e-4)
            << path.string() << ": fixture has near-zero curv0_elem[0] — may be undeformed";

        const auto result = fce::compute_principal_curvature(f.C_elem, f.curv0_elem);

        EXPECT_EQ(result.flag_num_diff, f.flag_num_diff) << path.string();

        for (int i = 0; i < 2; ++i) {
            EXPECT_NEAR(result.curvppal[i], f.curvppal[i], tol)
                << path.string() << " curvppal[" << i << "]";
            for (int comp = 0; comp < 2; ++comp) {
                EXPECT_NEAR(result.vppal[i][comp], f.vppal[i][comp], tol)
                    << path.string() << " vppal[" << i << "][" << comp << "]";
            }
            for (int j = 0; j < 3; ++j) {
                EXPECT_NEAR(result.dcurvppaldC[i][j], f.dcurvppaldC[i][j], tol)
                    << path.string() << " dcurvppaldC[" << i << "][" << j << "]";
                EXPECT_NEAR(result.dcurvppaldk[i][j], f.dcurvppaldk[i][j], tol)
                    << path.string() << " dcurvppaldk[" << i << "][" << j << "]";
                for (int comp = 0; comp < 2; ++comp) {
                    EXPECT_NEAR(result.dvppaldC[i][comp][j], f.dvppaldC[i][comp][j], tol)
                        << path.string() << " dvppaldC[" << i << "][" << comp << "][" << j << "]";
                    EXPECT_NEAR(result.dvppaldk[i][comp][j], f.dvppaldk[i][comp][j], tol)
                        << path.string() << " dvppaldk[" << i << "][" << comp << "][" << j << "]";
                }
            }
        }
    }
}

TEST(Principal, DirectionDerivativesMatchFiniteDifference) {
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
        auto plus_result = fce::compute_principal_curvature(plus, curv0_elem);
        auto minus_result = fce::compute_principal_curvature(minus, curv0_elem);
        ASSERT_FALSE(plus_result.flag_num_diff);
        ASSERT_FALSE(minus_result.flag_num_diff);
        for (int i = 0; i < 2; ++i) {
            plus_result.vppal[i] = align_direction(base.vppal[i], plus_result.vppal[i]);
            minus_result.vppal[i] = align_direction(base.vppal[i], minus_result.vppal[i]);
            for (int component = 0; component < 2; ++component) {
                const double fd =
                    (plus_result.vppal[i][component] - minus_result.vppal[i][component]) / (2.0 * h);
                EXPECT_NEAR(base.dvppaldC[i][component][j], fd, curvature_tolerance(fd))
                    << "metric derivative direction=" << i << " component=" << component << " variable=" << j;
            }
        }
    }

    for (int j = 0; j < 3; ++j) {
        auto plus = curv0_elem;
        auto minus = curv0_elem;
        plus[j] += h;
        minus[j] -= h;
        auto plus_result = fce::compute_principal_curvature(C_elem, plus);
        auto minus_result = fce::compute_principal_curvature(C_elem, minus);
        ASSERT_FALSE(plus_result.flag_num_diff);
        ASSERT_FALSE(minus_result.flag_num_diff);
        for (int i = 0; i < 2; ++i) {
            plus_result.vppal[i] = align_direction(base.vppal[i], plus_result.vppal[i]);
            minus_result.vppal[i] = align_direction(base.vppal[i], minus_result.vppal[i]);
            for (int component = 0; component < 2; ++component) {
                const double fd =
                    (plus_result.vppal[i][component] - minus_result.vppal[i][component]) / (2.0 * h);
                EXPECT_NEAR(base.dvppaldk[i][component][j], fd, curvature_tolerance(fd))
                    << "curvature derivative direction=" << i << " component=" << component << " variable=" << j;
            }
        }
    }
}
