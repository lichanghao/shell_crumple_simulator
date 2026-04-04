#include "fce/exponential.hpp"
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

namespace fs = std::filesystem;

// ── Minimal fixture reader (mirrors the format in dump_principal_exponential_oracle.f90) ──

struct PExpFixture {
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

static std::vector<std::vector<double>> exp_read_rows(const fs::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open: " + path.string());
    std::vector<std::vector<double>> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::vector<double> row;
        std::string token;
        while (iss >> token) row.push_back(fce::io::parse_fortran_double(token));
        if (!row.empty()) rows.push_back(row);
    }
    return rows;
}

static std::vector<fs::path> exp_sorted_paths(const fs::path& dir) {
    std::vector<fs::path> result;
    for (const auto& e : fs::directory_iterator(dir)) {
        if (e.is_regular_file() && e.path().extension() == ".dat") result.push_back(e.path());
    }
    std::sort(result.begin(), result.end());
    return result;
}

static PExpFixture read_pexp_fixture_exp(const fs::path& path) {
    const auto rows = exp_read_rows(path);
    if (rows.size() != 36U) {
        throw std::runtime_error("unexpected row count " + std::to_string(rows.size()) + " in " + path.string());
    }
    PExpFixture f;
    f.C_elem        = fce::Voigt3{rows[1][0], rows[1][1], rows[1][2]};
    f.curv0_elem    = fce::Voigt3{rows[2][0], rows[2][1], rows[2][2]};
    f.flag_num_diff = (static_cast<int>(rows[3][0]) != 0);
    f.curvppal      = fce::Vec2{rows[4][0], rows[4][1]};
    f.vppal         = fce::Mat22{{fce::Vec2{rows[5][0], rows[5][1]}, fce::Vec2{rows[6][0], rows[6][1]}}};
    f.dcurvppaldC[0] = fce::Voigt3{rows[7][0], rows[7][1], rows[7][2]};
    f.dcurvppaldC[1] = fce::Voigt3{rows[8][0], rows[8][1], rows[8][2]};
    f.dcurvppaldk[0] = fce::Voigt3{rows[9][0], rows[9][1], rows[9][2]};
    f.dcurvppaldk[1] = fce::Voigt3{rows[10][0], rows[10][1], rows[10][2]};
    f.dvppaldC[0][0] = fce::Voigt3{rows[11][0], rows[11][1], rows[11][2]};
    f.dvppaldC[0][1] = fce::Voigt3{rows[12][0], rows[12][1], rows[12][2]};
    f.dvppaldC[1][0] = fce::Voigt3{rows[13][0], rows[13][1], rows[13][2]};
    f.dvppaldC[1][1] = fce::Voigt3{rows[14][0], rows[14][1], rows[14][2]};
    f.dvppaldk[0][0] = fce::Voigt3{rows[15][0], rows[15][1], rows[15][2]};
    f.dvppaldk[0][1] = fce::Voigt3{rows[16][0], rows[16][1], rows[16][2]};
    f.dvppaldk[1][0] = fce::Voigt3{rows[17][0], rows[17][1], rows[17][2]};
    f.dvppaldk[1][1] = fce::Voigt3{rows[18][0], rows[18][1], rows[18][2]};
    f.A_norm = {rows[19][0], rows[19][1], rows[19][2]};
    f.Ei[0] = fce::Vec2{rows[20][0], rows[20][1]};
    f.Ei[1] = fce::Vec2{rows[21][0], rows[21][1]};
    f.Ei[2] = fce::Vec2{rows[22][0], rows[22][1]};
    for (int i = 0; i < 6; ++i) f.pe[i] = rows[23][static_cast<std::size_t>(i)];
    for (int i = 0; i < 6; ++i) f.dpedC[i] = fce::Voigt3{rows[24 + i][0], rows[24 + i][1], rows[24 + i][2]};
    for (int i = 0; i < 6; ++i) f.dpedk[i] = fce::Voigt3{rows[30 + i][0], rows[30 + i][1], rows[30 + i][2]};
    return f;
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

TEST(Exponential, MatchesArchivedCompressionFortranOracle) {
    // Compares compute_deformed_bonds_with_derivatives against Fortran-derived fixtures for
    // elements 83-87 of the archived compression simulator state (10 cases).
    // Bond geometry uses eta=0 (inner displacement zero) — see oracle driver comments.
    // Tolerance 1e-12: analytical formula, identical floating-point operations.
    const fs::path fixture_dir =
        fs::path(ORACLE_DIR) / "principal_exponential_oracle";
    const auto fixture_paths = exp_sorted_paths(fixture_dir);
    ASSERT_GE(fixture_paths.size(), 10U);

    constexpr double tol = 1e-12;

    for (const auto& path : fixture_paths) {
        const auto f = read_pexp_fixture_exp(path);

        const auto result = fce::compute_deformed_bonds_with_derivatives(
            f.C_elem,
            f.curvppal,
            f.vppal,
            f.dcurvppaldC,
            f.dcurvppaldk,
            f.dvppaldC,
            f.dvppaldk,
            f.A_norm,
            f.Ei);

        for (int i = 0; i < 6; ++i) {
            EXPECT_NEAR(result.pe[i], f.pe[i], tol)
                << path.string() << " pe[" << i << "]";
            for (int j = 0; j < 3; ++j) {
                EXPECT_NEAR(result.dpedC[i][j], f.dpedC[i][j], tol)
                    << path.string() << " dpedC[" << i << "][" << j << "]";
                EXPECT_NEAR(result.dpedk[i][j], f.dpedk[i][j], tol)
                    << path.string() << " dpedk[" << i << "][" << j << "]";
            }
        }
    }
}
