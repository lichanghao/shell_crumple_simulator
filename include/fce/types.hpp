#pragma once
// Core data types mirroring Fortran headers.f90
// All indices stored internally are 0-based (Fortran file values are 1-based; subtract 1 on read).

#include <array>
#include <vector>
#include <cstdint>

namespace fce {

// ─── Constants ───────────────────────────────────────────────────────────────

inline constexpr int kMaxNeighElem = 12;
inline constexpr int kMaxNeighVert = 12;

// ─── Tensor / vector helpers (mirror Fortran tensor22, vector2, vector3) ─────

using Mat22 = std::array<std::array<double, 2>, 2>;  // row-major [row][col]
using Vec2  = std::array<double, 2>;
using Vec3  = std::array<double, 3>;

// ─── Element connectivity (mirrors Fortran TYPE tri) ─────────────────────────

struct TriElement {
    std::array<int, 3>              vertices{};        // 0-based node indices
    int                             num_neigh_elem{0};
    int                             num_neigh_vert{0};
    std::array<int, kMaxNeighElem>  neigh_elem{};      // 0-based element indices
    std::array<int, kMaxNeighVert>  neigh_vert{};      // 0-based node indices
    std::array<int, 3>              code_bc{};         // boundary condition codes
};

// ─── Mesh (mirrors Fortran TYPE mesh in data_mesh) ───────────────────────────

struct Mesh {
    int                    numele{0};
    int                    numnods{0};
    int                    nedge{0};
    int                    nelem_ghost{0};
    int                    nnode_ghost{0};
    int                    ngauss{2};          // Gauss points per element (read from dims)

    std::vector<TriElement>   connect;         // size: numele
    std::vector<std::vector<int>> ntable;      // B-spline 12-node patch: ntable[ielem][0..11]
    std::vector<std::vector<int>> nghost_tab;  // ghost table: nghost_tab[ielem][0..11]
    std::vector<int>          elem_ghost;      // size: nelem_ghost
    std::vector<int>          node_ghost;      // size: nnode_ghost
};

// ─── Boundary conditions (mirrors Fortran TYPE BC_data) ──────────────────────

struct BCData {
    int    nloadstep{0};
    int    nCodeLoad{0};
    int    nnodBC{0};
    int    ndofBC{0};
    int    ndofOP{0};

    std::vector<int> mdofBC;       // 0-based constrained DOF indices (size: ndofBC)
    std::vector<int> mdofOP;       // 0-based free DOF indices (size: ndofOP)
    std::vector<std::array<int,2>> mnodBC;  // 0-based node indices (size: nnodBC × 2)

    std::array<std::array<double, 3>, 3> rotation{};  // 3×3 rotation matrix
    Vec3   xc{};     // center point (nm)
    double value{0}; // total loading value (nm or degrees)

    // Cyclic parameters (nCodeLoad=30/31)
    int    ncycles{0};
    int    nloadstep_comp{0};
    int    nloadstep_rel{0};
    double value_comp{0};
    double value_rel{0};

    // Runtime phase tracking (simulator only)
    int    icycle{0};
    int    iphase{1}; // 1=compression, 2=release
};

// ─── Material (mirrors Fortran TYPE material in data_mat) ────────────────────

struct MatData {
    int    nCode_Pot{1};           // 1 = Brenner REBO
    double A0{0.142};              // equilibrium C-C bond length (nm)
    double s0{0};                  // reference unit cell area (nm²)
    double A1{0};                  // (unused in graphene path)
    std::array<Vec2, 3> E{};      // bond vectors E1, E2, E3 (2D, nm)
    Vec3   Vs{};                   // (unused)
    Vec3   Va{};                   // (unused)
};

// ─── Crease memory (mirrors Fortran data_crease MODULE globals) ───────────────

struct CreaseData {
    int    ncrease{0};       // 1 = crease memory active
    double kappa_cr{0};      // effective curvature threshold (1/nm)
    double alpha_lock{0};    // locking fraction per release phase [0,1]

    // K0_ref[igauss][k][ielem], k=0..2 (Voigt curvature components)
    // Stored as [ngauss * 3 * numele] flat or indexed as [ielem][igauss][k]
    std::vector<std::vector<std::array<double,3>>> K0_ref; // K0_ref[ielem][igauss][k]
};

// ─── Van der Waals data (mirrors Fortran TYPE vdw_data — minimal for I/O) ────

struct VdwData {
    int    nvdw{0};            // 0=disabled, 1=enabled
    int    nself_contact{0};   // 1=same-sheet self-contact mode
    // Full vdW state (spatial bins, neighbor lists) added in Milestone 6
};

// ─── Reference config (J0 and F0, one record per element per Gauss point) ────

struct RefConfig {
    double                    J0{0};   // reference Jacobian (nm²)
    Mat22                     F0{};    // 2×2 reference deformation gradient
};

// ─── Nodal positions ─────────────────────────────────────────────────────────

using Coords = std::vector<Vec3>;       // size: numnods, each Vec3 = {x,y,z} nm

// ─── Inner displacement η (one 2D vector per element per Gauss point) ────────

using EtaField = std::vector<std::vector<Vec2>>;  // [ielem][igauss]

} // namespace fce
