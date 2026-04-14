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

#if !defined(ORACLE_DIR)
#define ORACLE_DIR "test/cases"
#endif

namespace {

namespace fs = std::filesystem;

using Vec2 = fce::Vec2;
using Vec3 = fce::Vec3;
using Voigt3 = fce::Voigt3;
using NeighborCoords12 = fce::NeighborCoords12;
using ShapeGradient12 = fce::ShapeGradient12;
using ShapeCurvature12 = fce::ShapeCurvature12;

const fs::path kCaseDir =
    fs::path(ORACLE_DIR) / "graphene_compression_simulator" / "np1";
const fs::path kExpectedFixture =
    fs::path(ORACLE_DIR) / "first_constrained_step_oracle" / "element83_expected.dat";
const fs::path kStateFixture =
    fs::path(ORACLE_DIR) / "first_constrained_step_oracle" / "element83_state.dat";

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

}  // namespace

TEST(FirstConstrainedStepOracle, Element83UnitFixtureMatchesCommittedFortranOracle) {
    const auto input = fce::load_simulator_input(kCaseDir.string());
    const auto expected = read_expected_fixture(kExpectedFixture);
    const auto state_fixture = read_state_fixture(kStateFixture);

    ASSERT_EQ(state_fixture.element_index, expected.element_index);
    ASSERT_EQ(state_fixture.ngauss, expected.ngauss);
    ASSERT_EQ(input.dims.ngauss, expected.ngauss);

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
        EXPECT_EQ(state.flag_num_diff, expected.flag_num_diff.at(static_cast<std::size_t>(igauss)))
            << "gauss=" << igauss;
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
}
