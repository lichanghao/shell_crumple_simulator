#include "fce/element_energy.hpp"
#include "fce/element_state.hpp"
#include "fce/io.hpp"
#include "fce/simulator.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdlib>
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

#if defined(CRUNCH_IT_BIN)
constexpr const char* kCrunchItBinPath = CRUNCH_IT_BIN;
#else
constexpr const char* kCrunchItBinPath = "build/crunch_it";
#endif

using Vec2 = fce::Vec2;
using Vec3 = fce::Vec3;
using Voigt3 = fce::Voigt3;
using NeighborCoords12 = fce::NeighborCoords12;
using ShapeGradient12 = fce::ShapeGradient12;
using ShapeCurvature12 = fce::ShapeCurvature12;

const fs::path kCaseDir =
    fs::path(kOracleDir) / "graphene_compression_simulator" / "np1";
const fs::path kTraceFixture =
    fs::path(kOracleDir) / "graphene_compression_simulator" / "imperfection_trace_fortran.dat";
const fs::path kExpectedFixture =
    fs::path(kOracleDir) / "first_constrained_step_oracle" / "element83_expected.dat";
const fs::path kCrunchItBin = fs::path(kCrunchItBinPath);

struct FirstStepExpected {
    int element_index{0};
    int ngauss{0};
    double W_elem{0.0};
    std::vector<Vec2> eta;
    std::vector<bool> flag_num_diff;
};

std::string shell_quote(const fs::path& path) {
    std::string s = path.string();
    std::string out = "'";
    for (const char c : s) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out += c;
        }
    }
    out += "'";
    return out;
}

std::string shell_quote(const std::string& s) {
    std::string out = "'";
    for (const char c : s) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out += c;
        }
    }
    out += "'";
    return out;
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

FirstStepExpected read_expected_fixture(const fs::path& path) {
    const auto rows = read_rows(path);
    if (rows.size() != 5U) {
        throw std::runtime_error("unexpected first constrained-step fixture row count");
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

std::vector<Vec3> read_coord_dump(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open coord dump: " + path.string());
    }

    std::vector<Vec3> coords;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        int node = 0;
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        if (!(iss >> node >> x >> y >> z)) {
            continue;
        }
        (void)node;
        coords.push_back(Vec3{x, y, z});
    }
    return coords;
}

fs::path make_temp_dir() {
    std::string templ = (fs::temp_directory_path() / "fce-first-step-XXXXXX").string();
    std::vector<char> buffer(templ.begin(), templ.end());
    buffer.push_back('\0');
    char* raw = ::mkdtemp(buffer.data());
    if (raw == nullptr) {
        throw std::runtime_error("mkdtemp failed");
    }
    return fs::path(raw);
}

int run_crunch_it(const fs::path& case_dir,
                  const fs::path& dump_dir) {
    const std::string command =
        "FCE_TRACE_COORD_DUMPS=" + shell_quote(dump_dir.string()) + " " +
        shell_quote(kCrunchItBin) + " " + shell_quote(case_dir) + " 1 >/dev/null 2>&1";
    return std::system(command.c_str());
}

NeighborCoords12 build_neighbor_patch(const fce::SimulatorInput& input,
                                      const std::vector<Vec3>& coords,
                                      const int element_index) {
    NeighborCoords12 xneigh{};
    const auto& element = input.mesh.connect.at(static_cast<std::size_t>(element_index));
    for (int inode = 0; inode < 12; ++inode) {
        const int node_index = element.neigh_vert[inode];
        xneigh[inode] = coords.at(static_cast<std::size_t>(node_index));
    }
    return xneigh;
}

}  // namespace

TEST(FirstConstrainedStepOracle, Element83ReplayMatchesCommittedFortranOracle) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kTraceFixture)) << "Missing replay trace fixture at " << kTraceFixture;
    ASSERT_TRUE(fs::exists(kExpectedFixture)) << "Missing first-step oracle fixture at " << kExpectedFixture;

    const auto expected = read_expected_fixture(kExpectedFixture);

    const fs::path temp_root = make_temp_dir();
    const fs::path case_dir = temp_root / "case";
    const fs::path dump_dir = temp_root / "dumps";
    fs::create_directories(case_dir);
    fs::create_directories(dump_dir);
    fs::copy(kCaseDir, case_dir,
             fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    fs::copy_file(kTraceFixture, case_dir / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);

    ASSERT_EQ(run_crunch_it(case_dir, dump_dir), 0);

    const auto input = fce::load_simulator_input(case_dir.string());
    auto after_increment = read_coord_dump(dump_dir / "step1_after_increment.dat");
    auto after_imperfection = read_coord_dump(dump_dir / "step1_after_imperfection.dat");
    ASSERT_EQ(after_increment.size(), static_cast<std::size_t>(input.dims.numnods));
    ASSERT_EQ(after_imperfection.size(), static_cast<std::size_t>(input.dims.numnods));

    for (const int dof : input.bcs.mdofBC) {
        const int node = dof / 3;
        const int axis = dof % 3;
        after_imperfection.at(static_cast<std::size_t>(node))[axis] =
            after_increment.at(static_cast<std::size_t>(node))[axis];
    }

    const auto xneigh = build_neighbor_patch(input, after_imperfection, expected.element_index);
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

        const auto state = fce::compute_element_state(xneigh, dn, ddn, f0, reference_curvature);
        EXPECT_EQ(state.flag_num_diff, expected.flag_num_diff.at(static_cast<std::size_t>(igauss)))
            << "gauss=" << igauss;
    }

    const std::vector<Vec2> eta0(static_cast<std::size_t>(expected.ngauss), Vec2{0.0, 0.0});
    const auto result = fce::compute_element_energy(
        xneigh,
        f0,
        reference_curvature,
        input.gauss,
        input.general.mat,
        /*nW_hat=*/true,
        input.general.crit_local,
        100,
        eta0);

    ASSERT_EQ(result.inner_fail, 0);
    ASSERT_EQ(result.eta.size(), static_cast<std::size_t>(expected.ngauss));
    for (int igauss = 0; igauss < expected.ngauss; ++igauss) {
        for (int axis = 0; axis < 2; ++axis) {
            const double tol = 1e-8;
            EXPECT_NEAR(result.eta.at(static_cast<std::size_t>(igauss))[axis],
                        expected.eta.at(static_cast<std::size_t>(igauss))[axis],
                        tol)
                << "gauss=" << igauss << " axis=" << axis;
        }
    }
    const double w_tol = 1e-7;
    EXPECT_NEAR(result.W_elem, expected.W_elem, w_tol);

    fs::remove_all(temp_root);
}
