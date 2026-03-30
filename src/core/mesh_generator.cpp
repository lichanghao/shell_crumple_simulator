// Mesh generation — translates mesh_gen_square / meshg_gen_square from Fortran.
// All stored indices are 0-based (Fortran 1-based → subtract 1).

#include "fce/mesh_generator.hpp"
#include <stdexcept>

namespace fce {

static void gen_square_impl(int nrow, int ncol,
                             double xlength, double ylength,
                             FlatCoords& x0, Mesh& mesh)
{
    int numnods = (nrow + 1) * (ncol + 1);
    int numele  = 2 * nrow * ncol;

    mesh.numnods = numnods;
    mesh.numele  = numele;
    mesh.connect.assign(numele, TriElement{});

    double xstep = xlength / static_cast<double>(ncol);
    double ystep = ylength / static_cast<double>(nrow);

    // Nodal coordinates (flat, 3 per node: x, y, z)
    x0.assign(3 * numnods, 0.0);
    for (int i = 1; i <= nrow + 1; ++i) {
        for (int j = 1; j <= ncol + 1; ++j) {
            int inode = (i - 1) * (ncol + 1) + j; // 1-based
            // 0-based index = inode-1
            x0[(inode - 1) * 3 + 0] = (j - 1) * xstep;
            x0[(inode - 1) * 3 + 1] = (i - 1) * ystep;
            x0[(inode - 1) * 3 + 2] = 0.0;
        }
    }

    // Connectivity
    for (int i = 1; i <= nrow; ++i) {
        for (int j = 1; j <= ncol; ++j) {
            int ielem = ((i - 1) * ncol + j) * 2 - 1; // 1-based odd element
            int in1 = (i - 1) * (ncol + 1) + j;       // 1-based nodes
            int in2 = in1 + 1;
            int in3 = in1 + (ncol + 1);
            int in4 = in3 + 1;

            // Type 1 element (odd): vertices [in1, in2, in3]
            mesh.connect[ielem - 1].vertices = { in1 - 1, in2 - 1, in3 - 1 };
            // Type 2 element (even): vertices [in4, in3, in2]
            mesh.connect[ielem    ].vertices = { in4 - 1, in3 - 1, in2 - 1 };
        }
    }

    // Boundary tags (code_bc):
    //   code_bc[0] = 1: bottom (i=1) or top (i=nrow) row exposed edge
    //   code_bc[2] = 1: left (j=1) or right (j=ncol) col exposed edge

    // Bottom row (i=1): Type1 elements, code_bc[0]=1
    for (int icol = 1; icol <= ncol; ++icol) {
        int ielem = icol * 2 - 1; // 1-based
        mesh.connect[ielem - 1].code_bc[0] = 1;
    }

    // Top row (i=nrow): Type2 elements, code_bc[0]=1
    for (int icol = 1; icol <= ncol; ++icol) {
        int ielem = 2 * ncol * (nrow - 1) + icol * 2; // 1-based
        mesh.connect[ielem - 1].code_bc[0] = 1;
    }

    // Left col (j=1): Type1 elements, code_bc[2]=1
    for (int irow = 1; irow <= nrow; ++irow) {
        int ielem = (irow - 1) * ncol * 2 + 1; // 1-based
        mesh.connect[ielem - 1].code_bc[2] = 1;
    }

    // Right col (j=ncol): Type2 elements, code_bc[2]=1
    for (int irow = 1; irow <= nrow; ++irow) {
        int ielem = (irow - 1) * ncol * 2 + ncol * 2; // 1-based
        mesh.connect[ielem - 1].code_bc[2] = 1;
    }
}

void mesh_gen_square(int nrow, int ncol,
                     double xlength, double ylength,
                     FlatCoords& x0, Mesh& mesh)
{
    gen_square_impl(nrow, ncol, xlength, ylength, x0, mesh);
}

void meshg_gen_square(int nrowg, int ncolg,
                      double xlength, double ylength,
                      FlatCoords& xg, Mesh& meshg)
{
    gen_square_impl(nrowg, ncolg, xlength, ylength, xg, meshg);
}

} // namespace fce
