// Unit tests for nano_*.dat readers — parse oracle files and verify key fields.
// Does NOT require MPI; runs with plain ctest.

#include "fce/io.hpp"
#include "fce/mpi_env.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <string>

#ifndef ORACLE_DIR
#define ORACLE_DIR "test/cases"
#endif

static const std::string kPrepro = std::string(ORACLE_DIR) + "/graphene_compression_prepro/";

// ─── Helpers ──────────────────────────────────────────────────────────────────

static bool near(double a, double b, double tol = 1e-10) {
    return std::abs(a - b) <= tol;
}

// ─── parse_fortran_double ─────────────────────────────────────────────────────

TEST(ParseFortranDouble, BasicD) {
    EXPECT_NEAR(fce::io::parse_fortran_double("0.20000000000000000D+02"), 20.0, 1e-15);
    EXPECT_NEAR(fce::io::parse_fortran_double("0.14199999999999999D+00"), 0.142, 1e-15);
    EXPECT_NEAR(fce::io::parse_fortran_double("-0.86602540378443849D+00"), -0.8660254037844385, 1e-15);
    EXPECT_NEAR(fce::io::parse_fortran_double("0.61232339957367660D-16"), 6.123233995736766e-17, 1e-30);
}

TEST(ParseFortranDouble, StandardE) {
    EXPECT_NEAR(fce::io::parse_fortran_double("1.5E+03"), 1500.0, 1e-10);
    EXPECT_NEAR(fce::io::parse_fortran_double("-2.5e-04"), -0.00025, 1e-15);
}

// ─── nano_dims.dat ────────────────────────────────────────────────────────────

TEST(ReadDims, GrapheneCompression) {
    auto d = fce::io::read_dims(kPrepro + "nano_dims.dat");
    EXPECT_EQ(d.numele,       3200);
    EXPECT_EQ(d.numnods,      1681);
    EXPECT_EQ(d.nedge,         166);
    EXPECT_EQ(d.nelem_ghost,     0);
    EXPECT_EQ(d.nnode_ghost,     0);
    EXPECT_EQ(d.ngauss,          2);
    EXPECT_EQ(d.nnodBC,         82);
    EXPECT_EQ(d.ndofBC,        246);
    EXPECT_EQ(d.ndofOP,       4797);
    EXPECT_EQ(d.nvdw,            0);
}

// ─── nano_general.dat ─────────────────────────────────────────────────────────

TEST(ReadGeneral, GrapheneCompression) {
    auto g = fce::io::read_general(kPrepro + "nano_general.dat");
    EXPECT_NEAR(g.ylength,    20.0, 1e-10);
    EXPECT_NEAR(g.mat.A0,    0.142, 1e-12);
    EXPECT_EQ  (g.mat.nCode_Pot, 1);
    // Bond vector E1 (armchair orientation)
    EXPECT_NEAR(g.mat.E[0][0],  0.8660254037844387, 1e-12);
    EXPECT_NEAR(g.mat.E[0][1],  0.4999999999999999, 1e-12);
    // s0 ≈ 0.05238 nm²
    EXPECT_NEAR(g.mat.s0, 0.052387608725728257, 1e-13);
    EXPECT_FALSE(g.nW_hat);
    EXPECT_NEAR(g.crit_global, 1e-5, 1e-20);
    EXPECT_NEAR(g.crit_local,  1e-8, 1e-20); // from file: 1.0e-7 → check
    EXPECT_TRUE(g.imperfect);
    EXPECT_NEAR(g.fact_imp, 0.01, 1e-15);
}

// ─── nano_zero.dat ────────────────────────────────────────────────────────────

TEST(ReadZero, GrapheneCompression) {
    // nano_zero.dat stores one RefConfig per element (not per gauss point)
    auto rc = fce::io::read_zero(kPrepro + "nano_zero.dat", 3200);
    EXPECT_EQ(static_cast<int>(rc.size()), 3200);
    // First record (element 1): J0=0.25
    EXPECT_NEAR(rc[0].J0, 0.25, 1e-13);
    EXPECT_NEAR(rc[0].F0[0][0],  2.0, 1e-12);
    EXPECT_NEAR(rc[0].F0[0][1],  0.0, 1e-12);
    EXPECT_NEAR(rc[0].F0[1][0],  0.0, 1e-12);
    EXPECT_NEAR(rc[0].F0[1][1],  2.0, 1e-12);
    // Second record (element 2): J0=0.25, F0 sign-flipped
    EXPECT_NEAR(rc[1].J0, 0.25, 1e-13);
    EXPECT_NEAR(rc[1].F0[0][0], -2.0, 1e-12);
}

// ─── nano_config.dat ──────────────────────────────────────────────────────────

TEST(ReadConfig, GrapheneCompression) {
    auto c = fce::io::read_config(kPrepro + "nano_config.dat", 1681, 3200, 2);
    EXPECT_EQ(static_cast<int>(c.coords.size()), 1681);
    EXPECT_EQ(static_cast<int>(c.eta.size()), 3200);
    EXPECT_EQ(static_cast<int>(c.eta[0].size()), 2);
    // First node: (0, 0, 0) for flat graphene
    EXPECT_NEAR(c.coords[0][0], 0.0, 1e-15);
    EXPECT_NEAR(c.coords[0][1], 0.0, 1e-15);
    EXPECT_NEAR(c.coords[0][2], 0.0, 1e-15);
    // Second node: (0.5, 0, 0) — mesh spacing = 20nm/40 = 0.5nm
    EXPECT_NEAR(c.coords[1][0], 0.5, 1e-12);
    // All η initially zero
    EXPECT_NEAR(c.eta[0][0][0], 0.0, 1e-15);
    EXPECT_NEAR(c.eta[0][0][1], 0.0, 1e-15);
}

// ─── nano_BCs.dat ─────────────────────────────────────────────────────────────

TEST(ReadBCs, GrapheneCompression) {
    auto bc = fce::io::read_bcs(kPrepro + "nano_BCs.dat");
    EXPECT_EQ(bc.nloadstep, 50);
    EXPECT_EQ(bc.nCodeLoad, 3);
    EXPECT_EQ(bc.ndofBC,  246);
    EXPECT_EQ(bc.ndofOP, 4797);
    EXPECT_EQ(bc.nnodBC,   82);
    // First constrained DOF: 1-based value 1 → 0-based 0
    EXPECT_EQ(bc.mdofBC[0], 0);
    EXPECT_EQ(bc.mdofBC[1], 1);
    EXPECT_EQ(bc.mdofBC[2], 2);
    // Cyclic params (present but unused for nCodeLoad=3)
    EXPECT_EQ(bc.ncycles,         1);
    EXPECT_EQ(bc.nloadstep_comp, 50);
    EXPECT_EQ(bc.nloadstep_rel,   0);
    EXPECT_NEAR(bc.value, 1.0, 1e-12);
    EXPECT_NEAR(bc.value_comp, 1.0, 1e-12);
}

// ─── nano_Mesh.dat ────────────────────────────────────────────────────────────

TEST(ReadMesh, GrapheneCompression) {
    auto m = fce::io::read_mesh(kPrepro + "nano_Mesh.dat", 2);
    EXPECT_EQ(m.numele, 3200);
    EXPECT_EQ(m.nedge,   166);
    EXPECT_EQ(m.nelem_ghost, 0);
    EXPECT_EQ(m.nnode_ghost, 0);
    // First element: vertices 0, 1, 41 (1-based: 1, 2, 42)
    EXPECT_EQ(m.connect[0].vertices[0], 0);
    EXPECT_EQ(m.connect[0].vertices[1], 1);
    EXPECT_EQ(m.connect[0].vertices[2], 41);
    // num_neigh_elem and num_neigh_vert
    EXPECT_EQ(m.connect[0].num_neigh_elem, 3);
    EXPECT_EQ(m.connect[0].num_neigh_vert, 12);
    // code_bc for first element (boundary element)
    EXPECT_EQ(m.connect[0].code_bc[0], 1);
    EXPECT_EQ(m.connect[0].code_bc[1], 0);
    EXPECT_EQ(m.connect[0].code_bc[2], 1);
    // nghost_tab: first entry
    EXPECT_EQ(static_cast<int>(m.nghost_tab.size()), 166);
}

// ─── nano_tub_loc.dat ─────────────────────────────────────────────────────────

TEST(ReadTubLoc, GrapheneCompression) {
    auto parts = fce::io::read_tub_loc(kPrepro + "nano_tub_loc.dat");
    EXPECT_EQ(static_cast<int>(parts.size()), 1);
    // np=1: single rank covers all elements
    EXPECT_EQ(parts[0].first, 0);
    // The end value from oracle: 150400 = 3200*47? Actually from file: "1\n      150400"
    // This is nelem*ngauss*some_factor... or it's just the ngauss-weighted partition index.
    // From Fortran pre_ener.f90: ielem_end = total_work_units
    // For now just check it's positive and >= 3200
    EXPECT_GT(parts[0].second, 3200);
}

// ─── MPI partition helper ─────────────────────────────────────────────────────

TEST(ElementPartition, Serial) {
    auto p = fce::element_partition(3200, 1, 0);
    EXPECT_EQ(p.first, 0);
    EXPECT_EQ(p.second, 3200);
}

TEST(ElementPartition, TwoRanks) {
    auto p0 = fce::element_partition(3200, 2, 0);
    auto p1 = fce::element_partition(3200, 2, 1);
    EXPECT_EQ(p0.first, 0);
    EXPECT_EQ(p0.second, 1600);
    EXPECT_EQ(p1.first, 1600);
    EXPECT_EQ(p1.second, 3200);
    // No overlap, full coverage
    EXPECT_EQ(p0.second, p1.first);
}

TEST(ElementPartition, OddElements) {
    // 3201 elements, 2 ranks: first gets 1601, second gets 1600
    auto p0 = fce::element_partition(3201, 2, 0);
    auto p1 = fce::element_partition(3201, 2, 1);
    EXPECT_EQ(p0.second - p0.first + p1.second - p1.first, 3201);
}
