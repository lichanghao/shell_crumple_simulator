// Full preprocessor pipeline — translates Prepro.f90.

#include "fce/preprocessor.hpp"
#include "fce/types.hpp"
#include "fce/io.hpp"
#include "fce/mesh_generator.hpp"
#include "fce/bspline.hpp"
#include "fce/quadrature.hpp"
#include "fce/reference_config.hpp"
#include "fce/vdw_preprocessor.hpp"
#include "fce/ghost_nodes.hpp"
#include "fce/load_pre.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cmath>
#include <algorithm>
#include <vector>
#include <iostream>

namespace fce {

// ─── data.dat reader ─────────────────────────────────────────────────────────

struct DataDat {
    int    nsheets{1};
    int    ngauss{2};
    std::vector<int>    nrow, ncol;
    double xlength{20.0}, ylength{20.0};
    double stretch_ini{1.0};
    double A0{0.142};
    std::vector<double> xn1, xn2; // chirality indices per sheet
    int    nchir{1};               // 1=explicit chirality, 0=from indices
    double xorient{30.0};         // explicit chirality angle (degrees)
    int    nCode_Pot{1};
    int    nCodeLoad{3};
    double angle{1.0};            // main loading value
    double angle2{0.0};           // secondary value (biaxial, twist)
    int    nloadstep{50};
    int    ncycles{1};
    int    nloadstep_rel{0};
    int    nW_hat{0};
    double crit_global{1e-5};
    double crit_local{1e-8};
    int    imperfect{1};
    double fact_imp{0.01};
    int    nborder{0};
    int    nvdw{0};
    int    ngauss_vdw{0};
    double r_cut{0.0};
    double r_bond{0.0};
    double a{0.0};
    double sig{0.0};
    double y0{0.0};
    int    meval{0};
    double alpha_sharp{1.0};
    int    nself_contact{0};
    int    ncrease{0};
    double kappa_cr{0.0};
    double alpha_lock{0.0};
};

static DataDat read_data_dat(const std::string& path)
{
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open data.dat: " + path);

    auto skip_label = [&]() {
        std::string line;
        std::getline(f, line); // skip label line
    };
    auto read_line = [&]() -> std::string {
        std::string line;
        while (std::getline(f, line)) {
            // trim
            auto b = line.find_first_not_of(" \t\r\n");
            if (b != std::string::npos) return line.substr(b);
        }
        return "";
    };
    auto read_double = [&]() -> double {
        return io::parse_fortran_double(read_line());
    };

    DataDat d;

    // First pass: read nsheets
    skip_label(); // *numberofsheets
    d.nsheets = std::stoi(read_line());

    skip_label(); // ngauss
    d.ngauss = std::stoi(read_line());

    skip_label(); // nrow, ncol
    d.nrow.resize(d.nsheets);
    d.ncol.resize(d.nsheets);
    d.xn1.resize(d.nsheets);
    d.xn2.resize(d.nsheets);
    for (int i = 0; i < d.nsheets; ++i) {
        std::istringstream iss(read_line());
        iss >> d.nrow[i] >> d.ncol[i];
    }

    skip_label(); // xlength ylength
    {
        std::istringstream iss(read_line());
        std::string x_token;
        std::string y_token;
        iss >> x_token >> y_token;
        d.xlength = io::parse_fortran_double(x_token);
        d.ylength = io::parse_fortran_double(y_token);
    }

    skip_label(); // lambda_ini
    d.stretch_ini = read_double();

    skip_label(); // A0
    d.A0 = read_double();

    skip_label(); // nanotube numbers
    for (int i = 0; i < d.nsheets; ++i) {
        std::istringstream iss(read_line());
        iss >> d.xn1[i] >> d.xn2[i];
    }

    skip_label(); // chirality flag + angle
    d.nchir = std::stoi(read_line());
    d.xorient = read_double();

    skip_label(); // potential code label
    d.nCode_Pot = std::stoi(read_line());

    skip_label(); // nCodeLoad label
    d.nCodeLoad = std::stoi(read_line());

    skip_label(); // angle label
    d.angle = read_double();

    // angle2 only for certain nCodeLoad
    if (d.nCodeLoad == 13 || d.nCodeLoad == 222 || d.nCodeLoad == 333 ||
        d.nCodeLoad == 1000 || d.nCodeLoad == 30 || d.nCodeLoad == 31) {
        skip_label();
        d.angle2 = read_double();
    }

    skip_label(); // nloadstep label
    d.nloadstep = std::stoi(read_line());

    // Extra cyclic params
    d.ncycles = 1;
    d.nloadstep_rel = 0;
    if (d.nCodeLoad == 30 || d.nCodeLoad == 31) {
        skip_label();
        d.nloadstep_rel = std::stoi(read_line());
        skip_label();
        d.ncycles = std::stoi(read_line());
    }

    skip_label(); // nW_hat
    d.nW_hat = std::stoi(read_line());

    skip_label(); // critglobal
    d.crit_global = read_double();

    skip_label(); // critlocal
    d.crit_local = read_double();

    skip_label(); // imperfect
    d.imperfect = std::stoi(read_line());

    skip_label(); // fact_imp
    d.fact_imp = read_double();

    skip_label(); // nborder
    d.nborder = std::stoi(read_line());

    skip_label(); // nvdw
    d.nvdw = std::stoi(read_line());
    if (d.nvdw == 1) {
        skip_label(); // ngauss_vdw
        d.ngauss_vdw = std::stoi(read_line());
        skip_label(); // r_cut
        d.r_cut = read_double();
        skip_label(); // r_bond
        d.r_bond = read_double();
        skip_label(); // a
        d.a = read_double();
        skip_label(); // sig
        d.sig = read_double();
        skip_label(); // y0
        d.y0 = read_double();
        skip_label(); // meval
        d.meval = std::stoi(read_line());
        if (d.nCodeLoad == 1000) {
            skip_label(); // alpha_sharp
            d.alpha_sharp = read_double();
        }
        if (d.nCodeLoad == 30 || d.nCodeLoad == 31) {
            skip_label(); // nself_contact
            d.nself_contact = std::stoi(read_line());
        }
    }

    // Crease memory
    d.ncrease = 0;
    d.kappa_cr = 0.0;
    d.alpha_lock = 0.0;
    if (d.nCodeLoad == 30 || d.nCodeLoad == 31) {
        skip_label();
        d.ncrease = std::stoi(read_line());
        if (d.ncrease == 1) {
            skip_label();
            d.kappa_cr = read_double();
            skip_label();
            d.alpha_lock = read_double();
        }
    }

    return d;
}
static void validate_chirality_input(const DataDat& d)
{
    if (d.nchir != 0 && d.nchir != 1) {
        throw std::runtime_error("Invalid data.dat chirality flag: nchir must be 0 or 1");
    }
    if (d.nchir == 1) {
        return;
    }

    if (d.xn1.empty() || d.xn2.empty()) {
        throw std::runtime_error("Invalid data.dat chirality indices: missing xn1/xn2 values");
    }

    constexpr double kTol = 1e-12;
    for (int i = 0; i < d.nsheets; ++i) {
        const double xn1 = d.xn1[i];
        const double xn2 = d.xn2[i];
        if (xn1 < 0.0 || xn2 < 0.0) {
            throw std::runtime_error("Invalid data.dat chirality indices: xn1 and xn2 must be nonnegative");
        }
        if (std::abs(xn1) < kTol && std::abs(xn2) < kTol) {
            throw std::runtime_error("Invalid data.dat chirality indices: xn1 and xn2 cannot both be zero");
        }
        if (std::abs(2.0 * xn1 + xn2) < kTol) {
            throw std::runtime_error("Invalid data.dat chirality indices: 2*xn1 + xn2 must be nonzero");
        }
    }
}

// ─── Main pipeline ────────────────────────────────────────────────────────────

void run_preprocessor(const std::string& work_dir)
{
    const std::string sep = work_dir.empty() ? "" : work_dir + "/";
    std::cout << "FCE Preprocessor: reading " << sep << "data.dat\n";

    DataDat d = read_data_dat(sep + "data.dat");
    validate_chirality_input(d);

    const double PI = std::acos(-1.0);

    // ── Material: bond vectors and s0 ──────────────────────────────────────────
    double theta;
    if (d.nchir == 1) {
        theta = d.xorient * PI / 180.0;
    } else {
        theta = std::atan(-std::sqrt(3.0) * d.xn2[0] / (2.0 * d.xn1[0] + d.xn2[0]));
    }

    MatData mat;
    mat.A0       = d.A0;
    mat.nCode_Pot = d.nCode_Pot;
    mat.E[0]     = { std::cos(theta),             std::sin(theta)             };
    mat.E[1]     = { std::cos(theta + 2.0*PI/3.0), std::sin(theta + 2.0*PI/3.0) };
    mat.E[2]     = { std::cos(theta - 2.0*PI/3.0), std::sin(theta - 2.0*PI/3.0) };
    mat.s0       = 3.0 * std::sqrt(3.0) / 2.0 * d.A0 * d.A0;

    VdwData vdwT;
    if (d.nvdw == 1) {
        vdwT.nvdw = 1;
        vdwT.ngauss_vdw = d.ngauss_vdw;
        vdwT.r_cut = d.r_cut;
        vdwT.r_bond = d.r_bond;
        vdwT.a = d.a;
        vdwT.sig = d.sig;
        vdwT.y0 = d.y0;
        vdwT.meval = d.meval;
        vdwT.alpha_sharp = d.alpha_sharp;
        vdwT.nself_contact = d.nself_contact;
        compute_vdw_cutoff(vdwT);
    }

    // ── Handle nborder adjustment and xlength scaling ─────────────────────────
    int nborder = d.nborder;
    double xlength_orig = d.xlength;
    // Fortran: xlength = xlength*(ncol+2*nborder)/ncol (for all sheets)
    // Here nsheets=1, ncol=d.ncol[0]
    double xlength_scaled = xlength_orig * (d.ncol[0] + 2 * nborder) / static_cast<double>(d.ncol[0]);

    // Expand nrow, ncol by nborder
    std::vector<int> nrow_eff(d.nsheets), ncol_eff(d.nsheets);
    std::vector<int> numel_arr(d.nsheets), numno_arr(d.nsheets), numed_arr(d.nsheets);
    for (int i = 0; i < d.nsheets; ++i) {
        nrow_eff[i] = d.nrow[i] + 2 * nborder;
        ncol_eff[i] = d.ncol[i] + 2 * nborder;
        numel_arr[i] = nrow_eff[i] * ncol_eff[i] * 2;
        numno_arr[i] = (nrow_eff[i] + 1) * (ncol_eff[i] + 1);
        numed_arr[i] = 2 * (nrow_eff[i] + ncol_eff[i] + 3);
    }

    // ylength is same as input (not scaled by nborder in Fortran for ylength)
    double ylength = d.ylength;

    // ── BCsT totals ───────────────────────────────────────────────────────────
    BCData bcsT;
    bcsT.nCodeLoad = d.nCodeLoad;
    if (d.nCodeLoad == 30 || d.nCodeLoad == 31) {
        bcsT.nloadstep     = d.ncycles * (d.nloadstep + d.nloadstep_rel);
        bcsT.ncycles       = d.ncycles;
        bcsT.nloadstep_comp = d.nloadstep;
        bcsT.nloadstep_rel  = d.nloadstep_rel;
        bcsT.value_comp     = d.angle;
        bcsT.value_rel      = d.angle2;
    } else {
        bcsT.nloadstep      = d.nloadstep;
        bcsT.ncycles        = 1;
        bcsT.nloadstep_comp = d.nloadstep;
        bcsT.nloadstep_rel  = 0;
        bcsT.value_comp     = d.angle;
        bcsT.value_rel      = 0.0;
    }

    // ── Total mesh accumulators ───────────────────────────────────────────────
    Mesh meshT;
    meshT.ngauss = d.ngauss;
    double ylengthT = 0.0;

    int total_numel = 0, total_numno = 0, total_numed = 0;
    for (int i = 0; i < d.nsheets; ++i) {
        total_numel += numel_arr[i];
        total_numno += numno_arr[i];
        total_numed += numed_arr[i];
    }
    meshT.connect.resize(total_numel);
    meshT.nghost_tab.resize(total_numed, {0, 0, 0});

    FlatCoords x0T(3 * (total_numno + total_numed), 0.0);
    std::vector<RefConfig> rc_total(total_numel);

    // nelem_ghost / nnode_ghost totals
    int total_nelem_ghost = 0, total_nnode_ghost = 0;
    std::vector<std::vector<int>> all_elem_ghost(d.nsheets), all_node_ghost(d.nsheets);

    BCData bcsT_acc; // accumulator for BCsT fields
    bcsT_acc.nCodeLoad   = bcsT.nCodeLoad;
    bcsT_acc.nloadstep   = bcsT.nloadstep;
    bcsT_acc.ncycles     = bcsT.ncycles;
    bcsT_acc.nloadstep_comp = bcsT.nloadstep_comp;
    bcsT_acc.nloadstep_rel  = bcsT.nloadstep_rel;
    bcsT_acc.value_comp  = bcsT.value_comp;
    bcsT_acc.value_rel   = bcsT.value_rel;

    int nodT_acc = 0, nelT_acc = 0, nedT_acc = 0;
    BCData first_sheet_bc;
    bool first_sheet_bc_captured = false;

    for (int imesh = 0; imesh < d.nsheets; ++imesh) {
        int nrow = nrow_eff[imesh];
        int ncol = ncol_eff[imesh];
        int numel  = numel_arr[imesh];
        int numno  = numno_arr[imesh];
        int numed  = numed_arr[imesh];

        double xl = xlength_scaled;
        double yl = ylength;

        // ── Generate mesh ──────────────────────────────────────────────────────
        Mesh mesh0;
        FlatCoords x0;
        mesh_gen_square(nrow, ncol, xl, yl, x0, mesh0);

        // ── Compute F0, J0 ────────────────────────────────────────────────────
        auto rc = compute_ref_config(mesh0, x0);

        // ── Ghost/border elements ─────────────────────────────────────────────
        if (nborder > 0) {
            int ng = 2 * nborder * nrow;
            int ne = nborder * 4 * nrow;
            all_elem_ghost[imesh].resize(ne);
            all_node_ghost[imesh].resize(ng);
            int ii = 0, jj = 0;
            for (int irow = 1; irow <= nrow; ++irow) {
                for (int icol = 1; icol <= nborder; ++icol) {
                    int ielem = ((irow-1)*ncol + icol)*2 - 1; // 1-based
                    all_elem_ghost[imesh][ii++] = ielem - 1;
                    all_elem_ghost[imesh][ii++] = ielem;
                    int inod = (irow-1)*(ncol+1)+icol; // 1-based
                    all_node_ghost[imesh][jj++] = inod - 1;
                }
                for (int icol = ncol - nborder + 1; icol <= ncol; ++icol) {
                    int ielem = ((irow-1)*ncol + icol)*2 - 1;
                    all_elem_ghost[imesh][ii++] = ielem - 1;
                    all_elem_ghost[imesh][ii++] = ielem;
                    int inod = (irow-1)*(ncol+1)+icol+1;
                    all_node_ghost[imesh][jj++] = inod - 1;
                }
            }
            mesh0.nelem_ghost = ne;
            mesh0.nnode_ghost = ng;
        } else {
            mesh0.nelem_ghost = 0;
            mesh0.nnode_ghost = 0;
        }

        // ── Ghost mesh connectivity ───────────────────────────────────────────
        int nrowg = nrow + 2;
        int ncolg = ncol + 2;
        double xlg = xl / ncol * (ncol + 1);  // wait - Fortran: xlengthg = xlength/ncol*(ncol+1)
        // But actually Fortran uses xlength[imesh]/ncol[imesh]*(ncol[imesh]+1)
        // Here ncol = ncol_eff[imesh], xl = xlength_scaled
        // But the ghost mesh should use the same per-cell spacing, extended by 1 on each side
        // xlengthg = xl * (ncolg) / ncol = xl + xl/ncol (= xl*(1+1/ncol))
        // Wait Fortran: xlengthg = xlength(imesh)/ncol(imesh)*(ncolg)
        // ncolg = ncol+2, so xlengthg = xl/ncol*(ncol+2)
        // But the Fortran ncolg = ncol(imesh)+2 and nrowg = nrow(imesh)+2
        // where these are already the expanded (with nborder) values
        xlg = xl / ncol * ncolg;
        double ylg = yl / nrow * (nrow + 2); // Fortran: ylengthg = ylength/ncol*(nrow+1)?
        // Actually Fortran: ylengthg = ylength(imesh)/ncol(imesh)*(nrow(imesh)+1)
        // That would be: ylg = yl / ncol * (nrow + 1)
        // Wait, re-reading: ylengthg = ylength(imesh)/ncol(imesh)*(nrow(imesh)+1)
        // This mixes ncol and nrow. Let me use ncol for both (since step sizes are equal
        // for a square mesh xl/ncol = yl/nrow)
        ylg = yl / nrow * (nrow + 2); // nrowg = nrow+2

        Mesh meshg;
        FlatCoords xg;
        meshg_gen_square(nrowg, ncolg, xlg, ylg, xg, meshg);

        connect_mesh(meshg);
        connect_orig_mesh(mesh0, meshg, ncol, nrow);

        // ── Ghost node positions ──────────────────────────────────────────────
        // x0 currently has real nodes only; extend for ghost nodes
        x0.resize(3 * (numno + numed), 0.0);
        ghost_nodes(mesh0, x0);

        if (d.nvdw == 1) {
            VdwData sheet_vdw = vdwT;
            const double twist_angle_radians =
                (d.nCodeLoad == 1000 && imesh == 1) ? d.angle * PI / 180.0 : 0.0;
            initialize_preprocessor_vdw(sheet_vdw,
                                        mesh0,
                                        x0,
                                        mat,
                                        xl,
                                        yl,
                                        twist_angle_radians,
                                        d.nCodeLoad == 1000);
            if (vdwT.shapef.empty()) {
                vdwT.shapef = sheet_vdw.shapef;
                vdwT.weight = sheet_vdw.weight;
                vdwT.xc0 = sheet_vdw.xc0;
                vdwT.yc0 = sheet_vdw.yc0;
                vdwT.nneigh = sheet_vdw.nneigh;
            } else if (vdwT.nneigh != sheet_vdw.nneigh) {
                throw std::runtime_error("run_preprocessor: mixed vdW exclusion modes are not yet supported");
            }
            vdwT.ng_tot += sheet_vdw.ng_tot;
            vdwT.ninrange = std::max(vdwT.ninrange, sheet_vdw.ninrange);
            vdwT.rho.insert(vdwT.rho.end(), sheet_vdw.rho.begin(), sheet_vdw.rho.end());
        }

        // ── BC setup ──────────────────────────────────────────────────────────
        // Determine ndofBC, nnodBC
        BCData bc;
        bc.nCodeLoad   = d.nCodeLoad;
        bc.nloadstep   = d.nloadstep;
        bc.value       = d.angle;
        bc.ncycles     = d.ncycles;
        bc.nloadstep_comp = d.nloadstep;
        bc.nloadstep_rel  = d.nloadstep_rel;
        bc.value_comp  = d.angle;
        bc.value_rel   = d.angle2;

        int code = d.nCodeLoad;
        if (code == 0) {
            bc.ndofBC = (imesh == 0) ? (nrow + 3) : nrow;
            bc.nnodBC = nrow;
        } else if (code==1||code==2||code==3||code==13||code==30) {
            bc.ndofBC = 2 * 3 * (nrow + 1);
            bc.nnodBC = 2 * (nrow + 1);
        } else if (code == 31) {
            bc.nnodBC = 4;
            bc.ndofBC = 12;
        } else if (code == 10) {
            bc.ndofBC = 2*3*nrow + 2*(ncol-1);
            bc.nnodBC = 2*nrow + 2*(ncol-1);
        } else if (code == 4) {
            bc.ndofBC = 2*nrow + 3;
            bc.nnodBC = 2*nrow;
        } else if (code == 5) {
            bc.ndofBC = 3*nrow;
            bc.nnodBC = 2*nrow;
        } else if (code == 6) {
            if (imesh == 0) {
                bc.ndofBC = 4*(ncol+1) + nrow;
                bc.nnodBC = 2*(ncol+1) + nrow - 2;
            } else {
                bc.ndofBC = 2*(ncol+1) + nrow;
                bc.nnodBC = 2*(ncol+1) + nrow - 2;
            }
        } else if (code == 666) {
            bc.ndofBC = 2*(ncol+1) + 2;
            bc.nnodBC = 2*(ncol+1);
        } else if (code == 222 || code == 1000) {
            bc.nnodBC = 2*(nborder+1)*(nrow+1);
            bc.ndofBC = 3*bc.nnodBC;
        } else if (code == 333) {
            bc.nnodBC = 2*(nborder+1)*(nrow+1);
            bc.ndofBC = 3*bc.nnodBC;
        } else if (code == 7) {
            bc.ndofBC = 12;
            bc.nnodBC = 4;
        } else if (code == 8) {
            bc.ndofBC = 2*(ncol-1) + 2 + 2;
            bc.nnodBC = 2*nrow + 2*(ncol-1) + 2;
        } else {
            throw std::runtime_error("run_preprocessor: nCodeLoad not implemented: " +
                                     std::to_string(code));
        }
        bc.ndofOP = 3 * numno;
        bc.mdofOP.resize(bc.ndofOP);

        // Call load_pre with real nodes only (x0[0..3*numno-1])
        FlatCoords x0_real(x0.begin(), x0.begin() + 3 * numno);
        load_pre(x0_real, mesh0, bc, xl, yl, nrow, ncol, nborder, imesh + 1, d.angle2);
        if (!first_sheet_bc_captured) {
            first_sheet_bc = bc;
            first_sheet_bc_captured = true;
        }

        // ── Merge into total ──────────────────────────────────────────────────
        // Accumulate BCsT
        bcsT_acc.nnodBC += bc.nnodBC;
        bcsT_acc.ndofBC += bc.ndofBC;
        bcsT_acc.ndofOP += bc.ndofOP;

        // Copy x0 real nodes into x0T
        int istart_nod = nodT_acc;
        for (int i = 0; i < 3 * numno; ++i)
            x0T[3 * istart_nod + i] = x0[i];

        // Copy rc into rc_total
        for (int ie = 0; ie < numel; ++ie)
            rc_total[nelT_acc + ie] = rc[ie];

        // Merge meshT.connect
        for (int ie = 0; ie < numel; ++ie) {
            int ielT = nelT_acc + ie;
            meshT.connect[ielT] = mesh0.connect[ie];
            // Remap neigh_vert: ghost nodes need to shift
            for (int k = 0; k < 12; ++k) {
                int nv = meshT.connect[ielT].neigh_vert[k];
                if (nv >= numno) {
                    // Ghost node: nv = numno + ghost_idx (0-based)
                    int ghost_idx = nv - numno;
                    meshT.connect[ielT].neigh_vert[k] = total_numno + nedT_acc + ghost_idx;
                } else if (nv >= 0) {
                    meshT.connect[ielT].neigh_vert[k] = nv + nodT_acc;
                }
                // neigh_elem: 1-based element indices, shift by nelT_acc
                if (meshT.connect[ielT].neigh_elem[k] != 0)
                    meshT.connect[ielT].neigh_elem[k] += nelT_acc;
            }
            // vertices
            for (int k = 0; k < 3; ++k)
                meshT.connect[ielT].vertices[k] += nodT_acc;
        }

        // Merge nghost_tab
        for (int ied = 0; ied < numed; ++ied) {
            int iedT = nedT_acc + ied;
            meshT.nghost_tab[iedT] = mesh0.nghost_tab[ied];
            for (int k = 0; k < 3; ++k)
                meshT.nghost_tab[iedT][k] += nodT_acc;
        }

        // Update BCsT mnodBC, mdofBC, mdofOP
        for (auto& p : bc.mnodBC) {
            bcsT_acc.mnodBC.push_back({p[0] + nodT_acc, p[1]});
        }
        for (int d0 : bc.mdofBC)
            bcsT_acc.mdofBC.push_back(d0 + 3 * nodT_acc);
        for (int d0 : bc.mdofOP)
            bcsT_acc.mdofOP.push_back(d0 + 3 * nodT_acc);

        // ghost/border
        all_elem_ghost[imesh].resize(mesh0.nelem_ghost);
        all_node_ghost[imesh].resize(mesh0.nnode_ghost);
        total_nelem_ghost += mesh0.nelem_ghost;
        total_nnode_ghost += mesh0.nnode_ghost;

        nodT_acc += numno;
        nelT_acc += numel;
        nedT_acc += numed;
        ylengthT += yl;
    }

    // Fill meshT dimensions
    meshT.numele  = total_numel;
    meshT.numnods = total_numno;
    meshT.nedge   = total_numed;
    meshT.nelem_ghost = total_nelem_ghost;
    meshT.nnode_ghost = total_nnode_ghost;

    // Build elem_ghost / node_ghost for meshT
    if (nborder > 0) {
        meshT.elem_ghost.clear();
        meshT.node_ghost.clear();
        int n3 = 0, n4 = 0;
        for (int imesh = 0; imesh < d.nsheets; ++imesh) {
            for (int v : all_elem_ghost[imesh]) meshT.elem_ghost.push_back(v + n3);
            for (int v : all_node_ghost[imesh]) meshT.node_ghost.push_back(v + n4);
            n3 += numel_arr[imesh];
            n4 += numno_arr[imesh];
        }
    } else {
        meshT.nelem_ghost = 0;
        meshT.nnode_ghost = 0;
        meshT.elem_ghost = {-10};
        meshT.node_ghost = {-10};
    }

    // Finalize BCsT
    bcsT_acc.value   = d.angle;
    if (first_sheet_bc_captured) {
        bcsT_acc.rotation = first_sheet_bc.rotation;
        bcsT_acc.xc = first_sheet_bc.xc;
        bcsT_acc.value = first_sheet_bc.value;
    }

    // ── Write output files ────────────────────────────────────────────────────
    // nano_dims.dat
    {
        io::DimsData dd;
        dd.numele      = meshT.numele;
        dd.numnods     = meshT.numnods;
        dd.nedge       = meshT.nedge;
        dd.nelem_ghost = meshT.nelem_ghost;
        dd.nnode_ghost = meshT.nnode_ghost;
        dd.ngauss      = d.ngauss;
        dd.nnodBC      = bcsT_acc.nnodBC;
        dd.ndofBC      = bcsT_acc.ndofBC;
        dd.ndofOP      = bcsT_acc.ndofOP;
        dd.nvdw        = d.nvdw;
        dd.ngauss_vdw  = vdwT.ngauss_vdw;
        dd.ng_tot      = vdwT.ng_tot;
        dd.nneigh      = vdwT.nneigh;
        dd.ninrange    = vdwT.ninrange;
        io::write_dims(sep + "nano_dims.dat", dd);
        std::cout << "Wrote nano_dims.dat\n";
    }

    // nano_general.dat
    {
        io::GeneralData gg;
        gg.ylength   = ylengthT;
        gg.mat       = mat;
        gg.nW_hat    = (d.nW_hat != 0);
        gg.crit_global = d.crit_global;
        gg.crit_local  = d.crit_local;
        gg.imperfect = (d.imperfect != 0);
        gg.fact_imp  = d.fact_imp;
        io::write_general(sep + "nano_general.dat", gg);
        std::cout << "Wrote nano_general.dat\n";
    }

    // nano_zero.dat
    {
        io::write_zero(sep + "nano_zero.dat", rc_total, meshT.numele);
        std::cout << "Wrote nano_zero.dat\n";
    }

    // nano_config.dat
    {
        io::ConfigData cfg;
        cfg.coords.resize(meshT.numnods);
        for (int i = 0; i < meshT.numnods; ++i) {
            cfg.coords[i] = { x0T[i*3+0], x0T[i*3+1], x0T[i*3+2] };
        }
        cfg.eta.assign(meshT.numele, std::vector<Vec2>(d.ngauss, {0.0, 0.0}));
        io::write_config(sep + "nano_config.dat", cfg, meshT.numnods, meshT.numele, d.ngauss);
        std::cout << "Wrote nano_config.dat\n";
    }

    // nano_BCs.dat
    {
        io::write_bcs(sep + "nano_BCs.dat", bcsT_acc);
        std::cout << "Wrote nano_BCs.dat\n";
    }

    // nano_Mesh.dat
    {
        Mesh mesh_out = meshT;
        for (auto& element : mesh_out.connect) {
            for (int k = 0; k < 12; ++k) {
                if (element.neigh_elem[k] != 0 && element.neigh_vert[k] >= 0) {
                    element.neigh_vert[k] += 1;
                }
            }
        }
        io::write_mesh(sep + "nano_Mesh.dat", mesh_out, d.ngauss);
        std::cout << "Wrote nano_Mesh.dat\n";
    }

    // nano_tub_loc.dat
    {
        std::vector<std::pair<int,int>> parts;
        if (d.nvdw == 1) {
            parts = build_tub_partitions(numel_arr, vdwT.ngauss_vdw);
        } else {
            int kk9 = 0;
            const int archived_ngauss_vdw =
                ((d.nCodeLoad == 30 || d.nCodeLoad == 31) && d.ncrease == 1) ? 50 : 47;
            for (int i = 0; i < d.nsheets; ++i) {
                kk9 += numel_arr[i] * archived_ngauss_vdw;
                parts.emplace_back(kk9, kk9);
            }
        }
        io::write_tub_loc(sep + "nano_tub_loc.dat", parts);
        std::cout << "Wrote nano_tub_loc.dat\n";
    }

    if (d.nvdw == 1) {
        io::write_vdw(sep + "nano_vdw.dat", vdwT);
        std::cout << "Wrote nano_vdw.dat\n";
    }

    if ((d.nCodeLoad == 30 || d.nCodeLoad == 31) && d.ncrease == 1) {
        CreaseData crease;
        crease.ncrease = d.ncrease;
        crease.kappa_cr = d.kappa_cr;
        crease.alpha_lock = d.alpha_lock;
        io::write_crease(sep + "nano_crease.dat", crease, meshT.numnods, d.ngauss);
        std::cout << "Wrote nano_crease.dat\n";
    }

    std::cout << "Preprocessor done.\n";
    std::cout << "  numele=" << meshT.numele
              << " numnods=" << meshT.numnods
              << " nedge=" << meshT.nedge << "\n";
    std::cout << "  nnodBC=" << bcsT_acc.nnodBC
              << " ndofBC=" << bcsT_acc.ndofBC
              << " ndofOP=" << bcsT_acc.ndofOP << "\n";
}

} // namespace fce
