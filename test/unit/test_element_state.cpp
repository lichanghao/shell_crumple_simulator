#include "fce/element_state.hpp"
#include "fce/constitutive.hpp"
#include "fce/exponential.hpp"
#include "fce/ghost_nodes.hpp"
#include "fce/io.hpp"
#include "fce/quadrature.hpp"

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

Vec2 row_to_vec2(const std::vector<double>& row) {
    if (row.size() != 2U) {
        throw std::runtime_error("expected 2-column row");
    }
    return Vec2{row[0], row[1]};
}

Voigt3 row_to_voigt3(const std::vector<double>& row) {
    if (row.size() != 3U) {
        throw std::runtime_error("expected 3-column row");
    }
    return Voigt3{row[0], row[1], row[2]};
}

std::array<double, 6> row_to_vec6(const std::vector<double>& row) {
    if (row.size() != 6U) {
        throw std::runtime_error("expected 6-column row");
    }
    std::array<double, 6> out{};
    std::copy(row.begin(), row.end(), out.begin());
    return out;
}

Mat22 rows_to_mat22(const std::vector<double>& row0, const std::vector<double>& row1) {
    if (row0.size() != 2U || row1.size() != 2U) {
        throw std::runtime_error("expected 2x2 matrix rows");
    }
    return Mat22{{Vec2{row0[0], row0[1]}, Vec2{row1[0], row1[1]}}};
}

struct ArchivedElementFixture {
    int element_index{0};  // 0-based
    int gauss_index{0};    // 0-based
    Voigt3 C_elem{};
    Voigt3 curv0_elem{};
    Vec2 curvppal{};
    Mat22 vppal{};
    Vec2 archived_eta{};
    std::array<double, 3> A_norm{};
    std::array<Vec2, 3> Ei{};
    std::array<double, 6> pe{};
    Vec2 eta0{};
    double crit{0.0};
    int max_iter{0};
    int iterations{0};
    int fail_mode{0};
    Vec2 eta{};
    double W{0.0};
    Vec2 dWdeta{};
    Voigt3 ddWdeta{};
    std::array<double, 6> dW_dpe{};
};

ArchivedElementFixture read_archived_fixture(const fs::path& path) {
    const auto rows = read_rows(path);
    if (rows.size() != 20U) {
        throw std::runtime_error("unexpected archived element fixture row count");
    }

    ArchivedElementFixture fixture;
    fixture.element_index = static_cast<int>(rows[0].at(0)) - 1;
    fixture.gauss_index = static_cast<int>(rows[0].at(1)) - 1;
    fixture.C_elem = row_to_voigt3(rows[1]);
    fixture.curv0_elem = row_to_voigt3(rows[2]);
    fixture.curvppal = row_to_vec2(rows[3]);
    fixture.vppal = rows_to_mat22(rows[4], rows[5]);
    fixture.archived_eta = row_to_vec2(rows[6]);
    fixture.A_norm = row_to_voigt3(rows[7]);
    fixture.Ei[0] = row_to_vec2(rows[8]);
    fixture.Ei[1] = row_to_vec2(rows[9]);
    fixture.Ei[2] = row_to_vec2(rows[10]);
    fixture.pe = row_to_vec6(rows[11]);
    fixture.eta0 = row_to_vec2(rows[12]);
    fixture.crit = rows[13].at(0);
    fixture.max_iter = static_cast<int>(rows[13].at(1));
    fixture.iterations = static_cast<int>(rows[14].at(0));
    fixture.fail_mode = static_cast<int>(rows[14].at(1));
    fixture.eta = row_to_vec2(rows[15]);
    fixture.W = rows[16].at(0);
    fixture.dWdeta = row_to_vec2(rows[17]);
    fixture.ddWdeta = row_to_voigt3(rows[18]);
    fixture.dW_dpe = row_to_vec6(rows[19]);
    return fixture;
}

fce::FlatCoords flatten_coords(const fce::io::ConfigData& config) {
    fce::FlatCoords flat;
    flat.reserve(config.coords.size() * 3);
    for (const auto& xyz : config.coords) {
        flat.push_back(xyz[0]);
        flat.push_back(xyz[1]);
        flat.push_back(xyz[2]);
    }
    return flat;
}

struct ArchivedCompressionState {
    fce::io::DimsData dims;
    fce::io::GeneralData general;
    fce::Mesh mesh;
    std::vector<fce::RefConfig> ref_config;
    fce::io::ConfigData config;
    fce::GaussData gauss;
    fce::FlatCoords coords_with_ghosts;
};

const ArchivedCompressionState& archived_compression_state() {
    static const ArchivedCompressionState state = []() {
        ArchivedCompressionState out;
        const fs::path case_dir = fs::path(ORACLE_DIR) / "graphene_compression_simulator" / "np1";
        out.dims = fce::io::read_dims((case_dir / "nano_dims.dat").string());
        out.general = fce::io::read_general((case_dir / "nano_general.dat").string());
        out.mesh = fce::io::read_mesh((case_dir / "nano_Mesh.dat").string(), out.dims.ngauss);
        out.ref_config = fce::io::read_zero((case_dir / "nano_zero.dat").string(), out.dims.numele);
        out.config = fce::io::read_config((case_dir / "nano_config.dat").string(),
                                          out.dims.numnods,
                                          out.dims.numele,
                                          out.dims.ngauss);
        out.gauss = fce::setup_gauss(out.dims.ngauss);
        out.coords_with_ghosts = flatten_coords(out.config);
        out.coords_with_ghosts.resize(static_cast<std::size_t>(3 * (out.mesh.numnods + out.mesh.nedge)));
        fce::ghost_nodes(out.mesh, out.coords_with_ghosts);
        return out;
    }();
    return state;
}

NeighborCoords12 neighbor_patch_from_archive(const ArchivedCompressionState& archive, int element_index) {
    NeighborCoords12 xneigh{};
    const auto& element = archive.mesh.connect.at(static_cast<std::size_t>(element_index));
    for (int inode = 0; inode < 12; ++inode) {
        const int node_index = element.neigh_vert[inode];
        const std::size_t base = static_cast<std::size_t>(3 * node_index);
        xneigh[inode] = Vec3{
            archive.coords_with_ghosts.at(base),
            archive.coords_with_ghosts.at(base + 1),
            archive.coords_with_ghosts.at(base + 2),
        };
    }
    return xneigh;
}

ShapeGradient12 shape_gradient_from_archive(const ArchivedCompressionState& archive, int gauss_index) {
    ShapeGradient12 dn{};
    const auto& shapef = archive.gauss.shapef.at(static_cast<std::size_t>(gauss_index));
    for (int inode = 0; inode < 12; ++inode) {
        dn[inode] = Vec2{shapef[inode][1], shapef[inode][2]};
    }
    return dn;
}

ShapeCurvature12 shape_curvature_from_archive(const ArchivedCompressionState& archive, int gauss_index) {
    ShapeCurvature12 ddn{};
    const auto& shapef = archive.gauss.shapef.at(static_cast<std::size_t>(gauss_index));
    for (int inode = 0; inode < 12; ++inode) {
        ddn[inode] = Voigt3{shapef[inode][3], shapef[inode][4], shapef[inode][5]};
    }
    return ddn;
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

TEST(ElementState, RelaxedPipelineReturnsConvergedPreparedBondStage) {
    const auto xneigh = curved_patch();
    const auto dn = curved_dn();
    const auto ddn = curved_ddn();
    const Mat22 f0{{Vec2{0.9, 0.1}, Vec2{-0.2, 1.1}}};
    const Voigt3 reference_curvature{0.01, -0.02, 0.005};
    const Vec2 eta0{0.003, -0.002};
    const auto material = oracle_brenner_material();

    const auto base_state = fce::compute_element_state(xneigh, dn, ddn, f0, reference_curvature);
    const auto expected_inner = fce::solve_inner_newton(base_state, material, eta0, 1e-8, 100);
    const auto expected_state = fce::prepare_element_state(base_state, material, expected_inner.eta);
    const auto actual = fce::solve_inner_newton_for_element(
        xneigh, dn, ddn, f0, reference_curvature, material, eta0, 1e-8, 100);

    EXPECT_TRUE(actual.state.has_prepared_bond_state);
    for (int i = 0; i < 2; ++i) {
        EXPECT_NEAR(actual.state.prepared_eta[i], expected_inner.eta[i], tolerance(expected_inner.eta[i])) << i;
    }
    for (int ibond = 0; ibond < 3; ++ibond) {
        EXPECT_NEAR(actual.state.prepared_bonds.A_norm[ibond],
                    expected_state.prepared_bonds.A_norm[ibond],
                    tolerance(expected_state.prepared_bonds.A_norm[ibond]))
            << ibond;
        for (int idim = 0; idim < 2; ++idim) {
            EXPECT_NEAR(actual.state.prepared_bonds.Ei[ibond][idim],
                        expected_state.prepared_bonds.Ei[ibond][idim],
                        tolerance(expected_state.prepared_bonds.Ei[ibond][idim]))
                << ibond << ", " << idim;
        }
    }
    for (int i = 0; i < 6; ++i) {
        EXPECT_NEAR(actual.state.prepared_bonds.bonds.pe[i],
                    expected_state.prepared_bonds.bonds.pe[i],
                    tolerance(expected_state.prepared_bonds.bonds.pe[i]))
            << i;
    }
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

TEST(ElementState, MatchesArchivedCompressionSimulatorOracleFixtures) {
    const fs::path fixture_dir =
        fs::path(ORACLE_DIR) / "constitutive_oracle" / "archived_compression_np1";
    const auto fixtures = sorted_fixture_paths(fixture_dir);
    ASSERT_GE(fixtures.size(), 10U);

    const auto& archive = archived_compression_state();
    const auto& material = archive.general.mat;
    const Voigt3 reference_curvature{0.0, 0.0, 0.0};

    for (const auto& path : fixtures) {
        const auto fixture = read_archived_fixture(path);
        const auto xneigh = neighbor_patch_from_archive(archive, fixture.element_index);
        const auto dn = shape_gradient_from_archive(archive, fixture.gauss_index);
        const auto ddn = shape_curvature_from_archive(archive, fixture.gauss_index);
        const auto& f0 = archive.ref_config.at(static_cast<std::size_t>(fixture.element_index)).F0;

        const auto state = fce::compute_element_state(xneigh, dn, ddn, f0, reference_curvature);
        const auto prepared_state = fce::prepare_element_state(state, material, fixture.archived_eta);
        const auto inner =
            fce::solve_inner_newton(state, material, fixture.eta0, fixture.crit, fixture.max_iter);

        for (int i = 0; i < 3; ++i) {
            EXPECT_NEAR(state.C_elem[i], fixture.C_elem[i], tolerance(fixture.C_elem[i]))
                << path.string() << " C_elem[" << i << "]";
            EXPECT_NEAR(state.curv0_elem[i], fixture.curv0_elem[i], tolerance(fixture.curv0_elem[i]))
                << path.string() << " curv0_elem[" << i << "]";
            EXPECT_NEAR(prepared_state.prepared_bonds.A_norm[i], fixture.A_norm[i], tolerance(fixture.A_norm[i]))
                << path.string() << " A_norm[" << i << "]";
            EXPECT_NEAR(inner.ddWdeta[i], fixture.ddWdeta[i], tolerance(fixture.ddWdeta[i]))
                << path.string() << " ddWdeta[" << i << "]";
        }
        for (int i = 0; i < 2; ++i) {
            EXPECT_NEAR(state.curvppal[i], fixture.curvppal[i], tolerance(fixture.curvppal[i]))
                << path.string() << " curvppal[" << i << "]";
            EXPECT_NEAR(prepared_state.prepared_eta[i], fixture.archived_eta[i], tolerance(fixture.archived_eta[i]))
                << path.string() << " archived_eta[" << i << "]";
            EXPECT_NEAR(inner.eta[i], fixture.eta[i], tolerance(fixture.eta[i]))
                << path.string() << " eta[" << i << "]";
            EXPECT_NEAR(inner.dWdeta[i], fixture.dWdeta[i], tolerance(fixture.dWdeta[i]))
                << path.string() << " dWdeta[" << i << "]";
            for (int j = 0; j < 2; ++j) {
                EXPECT_NEAR(state.vppal[i][j], fixture.vppal[i][j], tolerance(fixture.vppal[i][j]))
                    << path.string() << " vppal[" << i << "][" << j << "]";
                EXPECT_NEAR(prepared_state.prepared_bonds.Ei[i][j],
                            fixture.Ei[i][j],
                            tolerance(fixture.Ei[i][j]))
                    << path.string() << " Ei[" << i << "][" << j << "]";
            }
        }
        for (int i = 0; i < 6; ++i) {
            EXPECT_NEAR(prepared_state.prepared_bonds.bonds.pe[i], fixture.pe[i], tolerance(fixture.pe[i]))
                << path.string() << " pe[" << i << "]";
            EXPECT_NEAR(inner.dW_dpe[i], fixture.dW_dpe[i], tolerance(fixture.dW_dpe[i]))
                << path.string() << " dW_dpe[" << i << "]";
        }
        EXPECT_NEAR(inner.W, fixture.W, tolerance(fixture.W)) << path.string() << " W";
        EXPECT_EQ(inner.iterations, fixture.iterations) << path.string();
        EXPECT_EQ(inner.fail_mode, fixture.fail_mode) << path.string();
    }
}
