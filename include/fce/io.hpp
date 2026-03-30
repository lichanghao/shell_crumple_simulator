#pragma once
// nano_*.dat reader/writer declarations.
// All indices converted: 1-based Fortran file ↔ 0-based C++ internal.
// Fortran D-exponent floats are handled transparently.

#include "fce/types.hpp"
#include <string>
#include <vector>

namespace fce {
namespace io {

// ─── D-exponent parsing ───────────────────────────────────────────────────────
// Replace 'D'/'d' with 'E'/'e' in a string before parsing as double.
double parse_fortran_double(const std::string& token);

// ─── nano_dims.dat ────────────────────────────────────────────────────────────

struct DimsData {
    int  numele{0};
    int  numnods{0};
    int  nedge{0};
    int  nelem_ghost{0};
    int  nnode_ghost{0};
    int  ngauss{2};
    int  nnodBC{0};
    int  ndofBC{0};
    int  ndofOP{0};
    int  nvdw{0};
};

DimsData read_dims(const std::string& path);
void     write_dims(const std::string& path, const DimsData& d);

// ─── nano_general.dat ─────────────────────────────────────────────────────────

struct GeneralData {
    double ylength{0};      // sheet width (nm)
    MatData mat;
    bool   nW_hat{false};   // inner relaxation flag
    double crit_global{0};
    double crit_local{0};
    bool   imperfect{false};
    double fact_imp{0};
};

GeneralData read_general(const std::string& path);
void        write_general(const std::string& path, const GeneralData& g);

// ─── nano_zero.dat ────────────────────────────────────────────────────────────
// Returns one RefConfig per element (J0 and F0 are element-level quantities).

std::vector<RefConfig> read_zero(const std::string& path, int numele);
void                   write_zero(const std::string& path,
                                  const std::vector<RefConfig>& rc,
                                  int numele);

// ─── nano_config.dat ──────────────────────────────────────────────────────────

struct ConfigData {
    Coords    coords;   // size: numnods
    EtaField  eta;      // [ielem][igauss], 2D inner displacement
};

ConfigData read_config(const std::string& path, int numnods, int numele, int ngauss);
void       write_config(const std::string& path, const ConfigData& c,
                        int numnods, int numele, int ngauss);

// ─── nano_BCs.dat ─────────────────────────────────────────────────────────────

BCData read_bcs(const std::string& path);
void   write_bcs(const std::string& path, const BCData& bc);

// ─── nano_Mesh.dat ────────────────────────────────────────────────────────────
// Reads connectivity, B-spline ntable, nghost_tab, and code_bc.
// Ghost flag == 0 → real node; nonzero → ghost entry.

Mesh read_mesh(const std::string& path, int ngauss);
void write_mesh(const std::string& path, const Mesh& mesh, int ngauss);

// ─── nano_tub_loc.dat ─────────────────────────────────────────────────────────
// Returns list of (ielem_start, ielem_end) pairs (0-based) per MPI rank.

std::vector<std::pair<int,int>> read_tub_loc(const std::string& path);
void                            write_tub_loc(const std::string& path,
                                              const std::vector<std::pair<int,int>>& parts);

// ─── nano_crease.dat ──────────────────────────────────────────────────────────
// Only present for cyclic cases (ncrease==1).

CreaseData read_crease(const std::string& path, int numnods, int ngauss);
void       write_crease(const std::string& path, const CreaseData& c,
                        int numnods, int ngauss);

} // namespace io
} // namespace fce
