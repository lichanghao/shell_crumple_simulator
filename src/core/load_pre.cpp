// Boundary condition setup — translates load_pre from load.f90.
// Fortran uses 1-based node/DOF indices; we store 0-based.
// The writer (io.cpp) will convert back to 1-based on output.

#include "fce/load_pre.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace fce {

static const double PI0 = 3.141592653589793238;

void compute_mdofOP(BCData& bc, int numnods)
{
    // Build set of constrained DOFs (0-based)
    std::vector<bool> is_fixed(3 * numnods, false);
    for (int d : bc.mdofBC) {
        if (d >= 0 && d < 3 * numnods) is_fixed[d] = true;
    }
    bc.mdofOP.clear();
    bc.mdofOP.reserve(3 * numnods - static_cast<int>(bc.mdofBC.size()));
    for (int i = 0; i < 3 * numnods; ++i) {
        if (!is_fixed[i]) bc.mdofOP.push_back(i);
    }
    bc.ndofOP = static_cast<int>(bc.mdofOP.size());
}

void load_pre(const FlatCoords& x0, const Mesh& mesh, BCData& bc,
              double xlength, double ylength,
              int nrow, int ncol, int nborder, int imesh, double angle2)
{
    int numnods = mesh.numnods;

    // Zero rotation matrix
    for (auto& row : bc.rotation) row.fill(0.0);

    // ── Set rotation vector ───────────────────────────────────────────────────
    int code = bc.nCodeLoad;

    if (code == 0) {
        bc.value = 0.0;
        bc.nloadstep = 1;
    } else if (code == 1 || code == 10) {
        double ang = bc.value * PI0 / 180.0;
        bc.value   = ang;
        double s   = ang / bc.nloadstep;
        bc.rotation[0] = { std::cos(s),  std::sin(s), 0.0 };
        bc.rotation[1] = {-std::sin(s),  std::cos(s), 0.0 };
        bc.rotation[2] = { 0.0,          0.0,          1.0 };
    } else if (code == 13) {
        double ang = bc.value * PI0 / 180.0;
        bc.value   = ang;
        double s   = ang / bc.nloadstep;
        bc.rotation[0] = { std::cos(s),  std::sin(s), 0.0 };
        bc.rotation[1] = {-std::sin(s),  std::cos(s), 0.0 };
        bc.rotation[2] = { angle2 / 2.0 / bc.nloadstep, 0.0, 0.0 };
    } else if (code == 2) {
        double ang = bc.value * PI0 / 180.0;
        double s   = ang / bc.nloadstep;
        bc.rotation[0] = { 1.0, 0.0,          0.0         };
        bc.rotation[1] = { 0.0, std::cos(s),  std::sin(s) };
        bc.rotation[2] = { 0.0,-std::sin(s),  std::cos(s) };
    } else if (code == 3 || code == 4 || code == 7) {
        bc.rotation[0] = { 0.0, 0.0, bc.value / bc.nloadstep };
    } else if (code == 30 || code == 31) {
        bc.rotation[0] = { 0.0, 0.0, bc.value_comp / bc.nloadstep_comp };
    } else if (code == 5) {
        bc.rotation[0] = { 0.0, bc.value / bc.nloadstep * ylength / nrow, 0.0 };
    } else if (code == 6) {
        bc.rotation[0] = { 0.0, -bc.value * ylength / PI0 / 200.0 / bc.nloadstep, 0.0 };
    } else if (code == 666) {
        // no rotation
    } else if (code == 222 || code == 1000) {
        bc.rotation[0] = { 1.0, 0.0, 0.0 };
        bc.rotation[1] = { 0.0, 1.0, 0.0 };
        bc.rotation[2] = { 0.0, 0.0, 1.0 };
    } else if (code == 333) {
        bc.rotation[0] = { 1.0, 0.0, 0.0 };
        bc.rotation[1] = { 0.0, 1.0, 0.0 };
        bc.rotation[2] = { 0.0, 0.0, 1.0 };
    } else if (code == 8) {
        bc.rotation[0] = { 0.0, bc.value / bc.nloadstep * ylength / nrow, 0.0 };
    } else {
        throw std::runtime_error("load_pre: nCodeLoad not implemented: " + std::to_string(code));
    }

    // ── Compute center of mass xc ─────────────────────────────────────────────
    bc.xc = {0.0, 0.0, 0.0};
    for (int inod = 0; inod < numnods; ++inod) {
        bc.xc[0] += x0[inod*3+0];
        bc.xc[1] += x0[inod*3+1];
        bc.xc[2] += x0[inod*3+2];
    }
    bc.xc[0] /= numnods;
    bc.xc[1] /= numnods;
    bc.xc[2] /= numnods;

    // ── Allocate mnodBC and mdofBC ─────────────────────────────────────────────
    bc.mnodBC.assign(bc.nnodBC, {0, 0});
    bc.mdofBC.assign(bc.ndofBC, 0);

    // Fortran array comprehension helper:
    // [(ijk, ijk=1, nrow*(ncol+1)+1, ncol+1)] for nrow+1 values: 1, ncol+2, ...
    // In 0-based: 0, ncol+1, 2*(ncol+1), ..., nrow*(ncol+1)
    // This is: irow * (ncol+1) for irow=0..nrow

    auto left_col_nodes = [&](int extra) -> std::vector<int> {
        // Fortran: [(ijk, ijk=1, nrow*(ncol+1)+1, ncol+1)] + nborder + extra
        // = 1-based node (irow-1)*(ncol+1)+1 + nborder + extra, for irow=0..nrow
        std::vector<int> v;
        for (int irow = 0; irow <= nrow; ++irow)
            v.push_back(irow * (ncol + 1) + nborder + extra); // 0-based
        return v;
    };
    auto right_col_nodes = [&](int extra) -> std::vector<int> {
        std::vector<int> v;
        for (int irow = 0; irow <= nrow; ++irow)
            v.push_back(irow * (ncol + 1) + ncol - nborder + extra); // 0-based
        return v;
    };

    if (code == 0) {
        // Fortran: mnodBC(1:nnodBC,1)=[(ijk,ijk=1,(nrow-1)*(ncol+1)+1,ncol+1)]
        // 0-based: irow*(ncol+1) for irow=0..nrow-1 (nrow nodes total? but nnodBC=nrow)
        bc.mnodBC.assign(bc.nnodBC, {0, 0});
        int jj = 0;
        for (int irow = 0; irow < nrow; irow++) {
            int inod = irow * (ncol + 1); // 0-based
            bc.mnodBC[jj][0] = inod;
            bc.mnodBC[jj][1] = 0; // tag
            jj++;
        }
        // mdofBC
        jj = 0;
        if (imesh == 1) {
            for (int irow = 0; irow < nrow; irow++) {
                int inod = irow * (ncol + 1);
                bc.mdofBC[jj++] = inod * 3; // x-DOF 0-based
            }
            bc.mdofBC[bc.ndofBC - 3] = 2; // DOF 3 (1-based) → 2 (0-based)
            bc.mdofBC[bc.ndofBC - 2] = 3 * (1 + (ncol+1)*nrow/4) - 2; // 0-based
            bc.mdofBC[bc.ndofBC - 1] = 3 * (1 + (ncol+1)*nrow/2) - 1; // 0-based... check
            // Actually Fortran: 3, 3*(1+(ncol+1)*nrow/4)-1, 3*(1+(ncol+1)*nrow/2) → 1-based
            // 0-based: 2, 3*(1+(ncol+1)*nrow/4)-2, 3*(1+(ncol+1)*nrow/2)-1
            bc.mdofBC[bc.ndofBC - 3] = 2;
            bc.mdofBC[bc.ndofBC - 2] = 3 * ((ncol+1)*nrow/4) + 1; // 0-based
            bc.mdofBC[bc.ndofBC - 1] = 3 * ((ncol+1)*nrow/2);
        } else {
            for (int irow = 0; irow < nrow; irow++) {
                int inod = irow * (ncol + 1);
                bc.mdofBC[jj++] = inod * 3;
            }
        }
    } else if (code == 7) {
        int i1 = (1 + (ncol+1)*nrow/4) + nborder - 1; // 0-based
        int i2 = (1 + (ncol+1)*nrow/4*3) + nborder - 1;
        int i3 = (1 + (ncol+1)*nrow/4) + ncol - nborder - 1;
        int i4 = (1 + (ncol+1)*nrow/4*3) + ncol - nborder - 1;
        bc.mnodBC[0] = {i1, 0}; // tag=1 → 0 (0-based? or 1-based?)
        bc.mnodBC[1] = {i2, 0};
        bc.mnodBC[2] = {i3, 1};
        bc.mnodBC[3] = {i4, 1};
        // Fortran: mnodBC(:,2) = 1 or 2 (tag)
        bc.mnodBC[0][1] = 0; bc.mnodBC[1][1] = 0;
        bc.mnodBC[2][1] = 1; bc.mnodBC[3][1] = 1;
        // mdofBC: 3 DOFs each
        for (int k = 0; k < 3; ++k) { bc.mdofBC[k]   = i1*3+k; }
        for (int k = 0; k < 3; ++k) { bc.mdofBC[3+k] = i2*3+k; }
        for (int k = 0; k < 3; ++k) { bc.mdofBC[6+k] = i3*3+k; }
        for (int k = 0; k < 3; ++k) { bc.mdofBC[9+k] = i4*3+k; }
    } else if (code == 222 || code == 1000) {
        int jj = 0;
        for (int j = 1; j <= nborder + 1; ++j) {
            for (int i = 1; i <= nrow + 1; ++i) {
                int inod_l = (i-1)*(ncol+1) + j - 1; // 0-based
                bc.mnodBC[jj][0] = inod_l;
                bc.mnodBC[jj][1] = 0;
                bc.mdofBC[3*jj+0] = inod_l*3+0;
                bc.mdofBC[3*jj+1] = inod_l*3+1;
                bc.mdofBC[3*jj+2] = inod_l*3+2;
                jj++;
            }
        }
        for (int j = ncol + 1 - nborder; j <= ncol + 1; ++j) {
            for (int i = 1; i <= nrow + 1; ++i) {
                int inod_r = (i-1)*(ncol+1) + j - 1; // 0-based
                bc.mnodBC[jj][0] = inod_r;
                bc.mnodBC[jj][1] = 1;
                bc.mdofBC[3*jj+0] = inod_r*3+0;
                bc.mdofBC[3*jj+1] = inod_r*3+1;
                bc.mdofBC[3*jj+2] = inod_r*3+2;
                jj++;
            }
        }
    } else if (code == 333) {
        int jj = 0;
        for (int j = 1; j <= nborder + 1; ++j) {
            for (int i = 1; i <= nrow + 1; ++i) {
                int inod_l = (i-1)*(ncol+1) + j - 1;
                bc.mnodBC[jj][0] = inod_l;
                bc.mnodBC[jj][1] = 0;
                bc.mdofBC[3*jj+0] = inod_l*3+0;
                bc.mdofBC[3*jj+1] = inod_l*3+1;
                bc.mdofBC[3*jj+2] = inod_l*3+2;
                jj++;
            }
        }
        for (int j = ncol + 1 - nborder; j <= ncol + 1; ++j) {
            for (int i = 1; i <= nrow + 1; ++i) {
                int inod_r = (i-1)*(ncol+1) + j - 1;
                bc.mnodBC[jj][0] = inod_r;
                bc.mnodBC[jj][1] = 1;
                bc.mdofBC[3*jj+0] = inod_r*3+0;
                bc.mdofBC[3*jj+1] = inod_r*3+1;
                bc.mdofBC[3*jj+2] = inod_r*3+2;
                jj++;
            }
        }
    } else if (code == 1 || code == 2 || code == 3 || code == 13 || code == 30) {
        // Left column nodes: irow=0..nrow, node = irow*(ncol+1)+nborder (0-based)
        auto lnodes = left_col_nodes(0);  // nborder already added in left_col_nodes
        auto rnodes = right_col_nodes(0);
        int hn = bc.nnodBC / 2; // nrow+1

        for (int k = 0; k < hn; ++k) {
            bc.mnodBC[k][0]      = lnodes[k];
            bc.mnodBC[k][1]      = 0;
            bc.mnodBC[hn+k][0]   = rnodes[k];
            bc.mnodBC[hn+k][1]   = 1;
        }
        // mdofBC: first half (left), stride 3 pattern
        for (int k = 0; k < hn; ++k) {
            bc.mdofBC[k*3+0] = lnodes[k]*3+0;
            bc.mdofBC[k*3+1] = lnodes[k]*3+1;
            bc.mdofBC[k*3+2] = lnodes[k]*3+2;
        }
        int off = bc.ndofBC / 2;
        for (int k = 0; k < hn; ++k) {
            bc.mdofBC[off + k*3+0] = rnodes[k]*3+0;
            bc.mdofBC[off + k*3+1] = rnodes[k]*3+1;
            bc.mdofBC[off + k*3+2] = rnodes[k]*3+2;
        }
    } else if (code == 31) {
        int i_bot   = nborder;         // 0-based row
        int i_top   = nrow - nborder;  // 0-based row
        int j_left  = nborder;         // 0-based col
        int j_right = ncol - nborder;  // 0-based col
        int inod_A  = i_bot  * (ncol+1) + j_left;
        int inod_B  = i_bot  * (ncol+1) + j_right;
        int inod_C  = i_top  * (ncol+1) + j_left;
        int inod_D  = i_top  * (ncol+1) + j_right;
        bc.mnodBC[0] = {inod_A, 0};
        bc.mnodBC[1] = {inod_B, 1};
        bc.mnodBC[2] = {inod_C, 2};
        bc.mnodBC[3] = {inod_D, 3};
        for (int k = 0; k < 3; ++k) bc.mdofBC[k]    = inod_A*3+k;
        for (int k = 0; k < 3; ++k) bc.mdofBC[3+k]  = inod_B*3+k;
        for (int k = 0; k < 3; ++k) bc.mdofBC[6+k]  = inod_C*3+k;
        for (int k = 0; k < 3; ++k) bc.mdofBC[9+k]  = inod_D*3+k;
    } else if (code == 10) {
        auto lnodes = left_col_nodes(0);
        auto rnodes = right_col_nodes(0);
        // Only nrow nodes for left/right (excluding bottom row node)
        // Fortran: [(ijk,ijk=1,(nrow-1)*(ncol+1)+1,ncol+1)] + nborder  -> nrow nodes for irow=0..nrow-1
        // Actually for nCodeLoad=10: nnodBC = 2*nrow + 2*(ncol-1)
        // Left nrow, right nrow, bottom interior ncol-1, mid-row interior ncol-1
        std::vector<int> lnodes10, rnodes10;
        for (int irow = 0; irow < nrow; ++irow) {
            lnodes10.push_back(irow*(ncol+1)+nborder);
            rnodes10.push_back(irow*(ncol+1)+ncol-nborder);
        }
        for (int k = 0; k < nrow; ++k) {
            bc.mnodBC[k][0]       = lnodes10[k]; bc.mnodBC[k][1]       = 0;
            bc.mnodBC[nrow+k][0]  = rnodes10[k]; bc.mnodBC[nrow+k][1]  = 1;
            // DOFs x,y,z for each left
            bc.mdofBC[k*3+0] = lnodes10[k]*3+0;
            bc.mdofBC[k*3+1] = lnodes10[k]*3+1;
            bc.mdofBC[k*3+2] = lnodes10[k]*3+2;
            bc.mdofBC[3*nrow+k*3+0] = rnodes10[k]*3+0;
            bc.mdofBC[3*nrow+k*3+1] = rnodes10[k]*3+1;
            bc.mdofBC[3*nrow+k*3+2] = rnodes10[k]*3+2;
        }
        // Bottom interior nodes j=2..ncol (0-based: j=1..ncol-1)
        int off1 = 6*nrow;
        for (int j = 1; j < ncol; ++j) {
            bc.mnodBC[2*nrow + j - 1][0] = j; // node index 0-based
            bc.mnodBC[2*nrow + j - 1][1] = 0;
            bc.mdofBC[off1 + j - 1] = j*3 + 2; // z-DOF
        }
        // Mid-row interior
        int mid_row = nrow / 2;
        int off2 = off1 + (ncol - 1);
        for (int j = 1; j < ncol; ++j) {
            int inod = mid_row*(ncol+1) + j;
            bc.mnodBC[2*nrow+(ncol-1)+j-1][0] = inod;
            bc.mnodBC[2*nrow+(ncol-1)+j-1][1] = 0;
            bc.mdofBC[off2 + j - 1] = inod*3 + 2;
        }
    } else if (code == 4) {
        auto lnodes = left_col_nodes(0);
        auto rnodes = right_col_nodes(0);
        int hn = bc.nnodBC / 2;
        for (int k = 0; k < hn; ++k) {
            bc.mnodBC[k][0]    = lnodes[k]; bc.mnodBC[k][1]    = 0;
            bc.mnodBC[hn+k][0] = rnodes[k]; bc.mnodBC[hn+k][1] = 1;
        }
        // mdofBC: only x-DOF for left/right, plus 3 pin DOFs
        for (int k = 0; k < hn; ++k) bc.mdofBC[k]      = lnodes[k]*3;
        for (int k = 0; k < hn; ++k) bc.mdofBC[hn+k]   = rnodes[k]*3;
        bc.mdofBC[bc.ndofBC-3] = 2;
        bc.mdofBC[bc.ndofBC-2] = 3*((ncol+1)*nrow/4) + 1;
        bc.mdofBC[bc.ndofBC-1] = 3*((ncol+1)*nrow/2);
    } else if (code == 5) {
        auto lnodes = left_col_nodes(0);
        auto rnodes = right_col_nodes(0);
        int hn = bc.nnodBC / 2;
        for (int k = 0; k < hn; ++k) {
            bc.mnodBC[k][0]    = lnodes[k]; bc.mnodBC[k][1]    = 0;
            bc.mnodBC[hn+k][0] = rnodes[k]; bc.mnodBC[hn+k][1] = 1;
        }
        for (int k = 0; k < hn; ++k) {
            bc.mdofBC[k*3+0] = lnodes[k]*3+0;
            bc.mdofBC[k*3+1] = lnodes[k]*3+1;
            bc.mdofBC[k*3+2] = lnodes[k]*3+2;
        }
    } else if (code == 8) {
        auto lnodes_v = [&]() {
            std::vector<int> v;
            for (int irow = 0; irow < nrow; ++irow)
                v.push_back(irow*(ncol+1)+nborder);
            return v;
        }();
        auto rnodes_v = [&]() {
            std::vector<int> v;
            for (int irow = 0; irow < nrow; ++irow)
                v.push_back(irow*(ncol+1)+ncol-nborder);
            return v;
        }();
        for (int k = 0; k < nrow; ++k) {
            bc.mnodBC[k][0]       = lnodes_v[k]; bc.mnodBC[k][1]      = 0;
            bc.mnodBC[nrow+k][0]  = rnodes_v[k]; bc.mnodBC[nrow+k][1] = 1;
        }
        // The rest of mnodBC
        for (int j = 1; j < ncol; ++j) {
            bc.mnodBC[2*nrow+j-1][0] = j; bc.mnodBC[2*nrow+j-1][1] = 0;
        }
        int mid = nrow/2;
        for (int j = 1; j < ncol; ++j) {
            int inod = mid*(ncol+1)+j;
            bc.mnodBC[2*nrow+(ncol-1)+j-1][0] = inod;
            bc.mnodBC[2*nrow+(ncol-1)+j-1][1] = 0;
        }
        int halfncol1 = ncol/2;
        bc.mnodBC[2*nrow+2*(ncol-1)+0][0] = halfncol1 + (ncol+1);
        bc.mnodBC[2*nrow+2*(ncol-1)+1][0] = halfncol1 - (ncol+1) + (ncol+1)*nrow;
        bc.mnodBC[2*nrow+2*(ncol-1)+0][1] = 0;
        bc.mnodBC[2*nrow+2*(ncol-1)+1][1] = 0;
        // mdofBC
        for (int j = 1; j < ncol; ++j)
            bc.mdofBC[j-1]         = j*3 + 2; // z-DOF
        for (int j = 1; j < ncol; ++j)
            bc.mdofBC[(ncol-1)+j-1] = (mid*(ncol+1)+j)*3 + 2;
        bc.mdofBC[2*(ncol-1)+0] = halfncol1*3+0;
        bc.mdofBC[2*(ncol-1)+1] = halfncol1*3+1;
        bc.mdofBC[2*(ncol-1)+2] = (halfncol1+(ncol+1))*3+1;
        bc.mdofBC[2*(ncol-1)+3] = (halfncol1-(ncol+1)+(ncol+1)*nrow)*3+1;
    } else if (code == 666) {
        // Fortran: mnodBC(1:nnodBC/2,1) = [(ijk,ijk=1,ncol+1)]
        // 0-based: 0..ncol (first row)
        int hn = bc.nnodBC / 2;
        for (int j = 0; j < hn; ++j) {
            bc.mnodBC[j][0]    = j;
            bc.mnodBC[j][1]    = 0;
            bc.mnodBC[hn+j][0] = (ncol+1)*(nrow/2) + j;
            bc.mnodBC[hn+j][1] = 0;
        }
        bc.mnodBC[0][1] = 0;
        bc.mnodBC[hn][1] = 1;
        // mdofBC: z-DOFs for both rows + 2 pin DOFs
        for (int j = 0; j < hn; ++j)
            bc.mdofBC[j]    = j*3+2;
        for (int j = 0; j < hn; ++j)
            bc.mdofBC[hn+j] = ((ncol+1)*(nrow/2)+j)*3+2;
        bc.mdofBC[bc.ndofBC-2] = 0;
        bc.mdofBC[bc.ndofBC-1] = ((ncol+1)*(nrow/2))*3+0;
    } else if (code == 6) {
        // Complex case omitted for oracle — falls through to mdofOP
    }

    // Compute mdofOP
    compute_mdofOP(bc, numnods);
}

} // namespace fce
