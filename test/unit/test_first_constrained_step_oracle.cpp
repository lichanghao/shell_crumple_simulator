#include "fce/element_energy.hpp"
#include "fce/element_state.hpp"
#include "fce/io.hpp"
#include "fce/simulator.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

#if defined(ORACLE_DIR)
constexpr const char* kOracleDir = ORACLE_DIR;
#else
constexpr const char* kOracleDir = "test/cases";
#endif

using Vec2 = fce::Vec2;
using Vec3 = fce::Vec3;
using Voigt3 = fce::Voigt3;
using NeighborCoords12 = fce::NeighborCoords12;
using ShapeGradient12 = fce::ShapeGradient12;
using ShapeCurvature12 = fce::ShapeCurvature12;

const fs::path kCaseDir =
    fs::path(kOracleDir) / "graphene_compression_simulator" / "np1";
const fs::path kExpectedFixture =
    fs::path(kOracleDir) / "first_constrained_step_oracle" / "element83_expected.dat";
const fs::path kStateFixture =
    fs::path(kOracleDir) / "first_constrained_step_oracle" / "element83_state.dat";
const fs::path kFullOracleFixture =
    fs::path(kOracleDir) / "first_constrained_step_oracle" / "element83_full_oracle.dat";

double tolerance(double expected) {
    return std::max(1e-10, std::abs(expected) * 1e-8);
}

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

struct FirstStepExpected {
    int element_index{0};
    int ngauss{0};
    double W_elem{0.0};
    std::vector<Vec2> eta;
    std::vector<bool> flag_num_diff;
};

struct GaussOracle {
    Voigt3 C_elem{};
    Voigt3 curv0_elem{};
    Vec2 curvppal{};
    fce::Mat22 vppal{};
    bool flag_num_diff{false};
    std::array<double, 6> pe{};
    int iterations{0};
    int fail_mode{0};
    Vec2 eta{};
    double W{0.0};
    Voigt3 ddWdeta{};
};

struct FirstStepFullOracle {
    int element_index{0};
    int ngauss{0};
    std::vector<GaussOracle> gauss;
    double W_elem{0.0};
    std::array<Vec3, 12> f_elem{};
};

FirstStepExpected read_expected_fixture(const fs::path& path) {
    const auto rows = read_rows(path);
    if (rows.size() != 5U) {
        throw std::runtime_error("unexpected first constrained-step expected row count");
    }

    FirstStepExpected out;
    out.element_index = static_cast<int>(rows[0].at(0)) - 1;
    out.ngauss = static_cast<int>(rows[0].at(1));
    out.W_elem = rows[1].at(0);
    out.eta.push_back(Vec2{rows[2].at(0), rows[2].at(1)});
    out.eta.push_back(Vec2{rows[3].at(0), rows[3].at(1)});
    out.flag_num_diff.push_back(static_cast<int>(rows[4].at(0)) != 0);
    out.flag_num_diff.push_back(static_cast<int>(rows[4].at(1)) != 0);
    return out;
}

struct FirstStepState {
    int element_index{0};
    int ngauss{0};
    NeighborCoords12 xneigh{};
};

FirstStepState read_state_fixture(const fs::path& path) {
    const auto rows = read_rows(path);
    if (rows.size() != 13U) {
        throw std::runtime_error("unexpected first constrained-step state row count");
    }

    FirstStepState out;
    out.element_index = static_cast<int>(rows[0].at(0)) - 1;
    out.ngauss = static_cast<int>(rows[0].at(1));
    for (int inode = 0; inode < 12; ++inode) {
        out.xneigh[inode] = Vec3{
            rows[static_cast<std::size_t>(1 + inode)].at(0),
            rows[static_cast<std::size_t>(1 + inode)].at(1),
            rows[static_cast<std::size_t>(1 + inode)].at(2),
        };
    }
    return out;
}

FirstStepFullOracle read_full_oracle_fixture(const fs::path& path) {
    const auto rows = read_rows(path);
    if (rows.size() != 36U) {
        throw std::runtime_error("unexpected first constrained-step full oracle row count");
    }

    FirstStepFullOracle out;
    out.element_index = static_cast<int>(rows[0].at(0)) - 1;
    out.ngauss = static_cast<int>(rows[0].at(1));
    out.gauss.resize(static_cast<std::size_t>(out.ngauss));

    std::size_t row = 1;
    for (int igauss = 0; igauss < out.ngauss; ++igauss) {
        auto& g = out.gauss[static_cast<std::size_t>(igauss)];
        g.C_elem = Voigt3{rows[row][0], rows[row][1], rows[row][2]}; ++row;
        g.curv0_elem = Voigt3{rows[row][0], rows[row][1], rows[row][2]}; ++row;
        g.curvppal = Vec2{rows[row][0], rows[row][1]}; ++row;
        g.vppal[0] = Vec2{rows[row][0], rows[row][1]}; ++row;
        g.vppal[1] = Vec2{rows[row][0], rows[row][1]}; ++row;
        g.flag_num_diff = static_cast<int>(rows[row][0]) != 0; ++row;
        for (int i = 0; i < 6; ++i) {
            g.pe[i] = rows[row][static_cast<std::size_t>(i)];
        }
        ++row;
        g.iterations = static_cast<int>(rows[row][0]);
        g.fail_mode = static_cast<int>(rows[row][1]);
        ++row;
        g.eta = Vec2{rows[row][0], rows[row][1]}; ++row;
        g.W = rows[row][0]; ++row;
        g.ddWdeta = Voigt3{rows[row][0], rows[row][1], rows[row][2]}; ++row;
    }

    out.W_elem = rows[row][0];
    ++row;
    for (int inode = 0; inode < 12; ++inode, ++row) {
        out.f_elem[static_cast<std::size_t>(inode)] =
            Vec3{rows[row][0], rows[row][1], rows[row][2]};
    }

    return out;
}

}  // namespace

TEST(FirstConstrainedStepOracle, Element83UnitFixtureMatchesCommittedFortranOracle) {
    const auto input = fce::load_simulator_input(kCaseDir.string());
    const auto expected = read_expected_fixture(kExpectedFixture);
    const auto state_fixture = read_state_fixture(kStateFixture);
    const auto full_oracle = read_full_oracle_fixture(kFullOracleFixture);

    ASSERT_EQ(state_fixture.element_index, expected.element_index);
    ASSERT_EQ(state_fixture.ngauss, expected.ngauss);
    ASSERT_EQ(input.dims.ngauss, expected.ngauss);
    ASSERT_EQ(full_oracle.element_index, expected.element_index);
    ASSERT_EQ(full_oracle.ngauss, expected.ngauss);

    const auto& f0 = input.ref_config.at(static_cast<std::size_t>(expected.element_index)).F0;
    const Voigt3 reference_curvature{0.0, 0.0, 0.0};

    for (int igauss = 0; igauss < expected.ngauss; ++igauss) {
        const auto& sf = input.gauss.shapef.at(static_cast<std::size_t>(igauss));
        ShapeGradient12 dn{};
        ShapeCurvature12 ddn{};
        for (int inode = 0; inode < 12; ++inode) {
            dn[inode] = Vec2{sf[inode][1], sf[inode][2]};
            ddn[inode] = Voigt3{sf[inode][3], sf[inode][4], sf[inode][5]};
        }

        const auto state =
            fce::compute_element_state(state_fixture.xneigh, dn, ddn, f0, reference_curvature);
        const auto& oracle = full_oracle.gauss.at(static_cast<std::size_t>(igauss));

        EXPECT_EQ(state.flag_num_diff, expected.flag_num_diff.at(static_cast<std::size_t>(igauss)))
            << "gauss=" << igauss;
        EXPECT_EQ(state.flag_num_diff, oracle.flag_num_diff) << "gauss=" << igauss;
        for (int i = 0; i < 3; ++i) {
            EXPECT_NEAR(state.C_elem[i], oracle.C_elem[i], tolerance(oracle.C_elem[i]))
                << "gauss=" << igauss << " C_elem[" << i << "]";
            EXPECT_NEAR(state.curv0_elem[i], oracle.curv0_elem[i], tolerance(oracle.curv0_elem[i]))
                << "gauss=" << igauss << " curv0_elem[" << i << "]";
        }
        for (int i = 0; i < 2; ++i) {
            EXPECT_NEAR(state.curvppal[i], oracle.curvppal[i], tolerance(oracle.curvppal[i]))
                << "gauss=" << igauss << " curvppal[" << i << "]";
            for (int j = 0; j < 2; ++j) {
                EXPECT_NEAR(state.vppal[i][j], oracle.vppal[i][j], tolerance(oracle.vppal[i][j]))
                    << "gauss=" << igauss << " vppal[" << i << "][" << j << "]";
            }
        }

        const auto inner = fce::solve_inner_newton(
            state, input.general.mat, Vec2{0.0, 0.0}, input.general.crit_local, 100);
        EXPECT_EQ(inner.iterations, oracle.iterations) << "gauss=" << igauss;
        EXPECT_EQ(inner.fail_mode, oracle.fail_mode) << "gauss=" << igauss;
        for (int i = 0; i < 2; ++i) {
            EXPECT_NEAR(inner.eta[i], oracle.eta[i], tolerance(oracle.eta[i]))
                << "gauss=" << igauss << " eta[" << i << "]";
        }
        EXPECT_NEAR(inner.W, oracle.W, tolerance(oracle.W)) << "gauss=" << igauss << " W";
        for (int i = 0; i < 3; ++i) {
            EXPECT_NEAR(inner.ddWdeta[i], oracle.ddWdeta[i], tolerance(oracle.ddWdeta[i]))
                << "gauss=" << igauss << " ddWdeta[" << i << "]";
        }

        const auto prepared = fce::prepare_element_state(state, input.general.mat, inner.eta);
        for (int i = 0; i < 6; ++i) {
            EXPECT_NEAR(prepared.prepared_bonds.bonds.pe[i], oracle.pe[i], tolerance(oracle.pe[i]))
                << "gauss=" << igauss << " pe[" << i << "]";
        }
    }

    const std::vector<Vec2> eta0(static_cast<std::size_t>(expected.ngauss), Vec2{0.0, 0.0});
    const auto result = fce::compute_element_energy(
        state_fixture.xneigh,
        f0,
        reference_curvature,
        input.gauss,
        input.general.mat,
        /*nW_hat=*/true,
        input.general.crit_local,
        100,
        eta0);

    ASSERT_EQ(result.inner_fail, 0);
    for (int igauss = 0; igauss < expected.ngauss; ++igauss) {
        for (int axis = 0; axis < 2; ++axis) {
            EXPECT_NEAR(result.eta.at(static_cast<std::size_t>(igauss))[axis],
                        expected.eta.at(static_cast<std::size_t>(igauss))[axis],
                        tolerance(expected.eta.at(static_cast<std::size_t>(igauss))[axis]))
                << "gauss=" << igauss << " axis=" << axis;
        }
    }
    EXPECT_NEAR(result.W_elem, expected.W_elem, tolerance(expected.W_elem));
    EXPECT_NEAR(result.W_elem, full_oracle.W_elem, tolerance(full_oracle.W_elem));
    for (int inode = 0; inode < 12; ++inode) {
        for (int axis = 0; axis < 3; ++axis) {
            EXPECT_NEAR(result.f_elem[static_cast<std::size_t>(inode)][axis],
                        full_oracle.f_elem[static_cast<std::size_t>(inode)][axis],
                        tolerance(full_oracle.f_elem[static_cast<std::size_t>(inode)][axis]))
                << "f_elem[" << inode << "][" << axis << "]";
        }
    }
}
