// Reference deformation gradient — translates Def_Grad_Cart_Conv from Fortran.

#include "fce/reference_config.hpp"

namespace fce {

std::vector<RefConfig> compute_ref_config(const Mesh& mesh,
                                          const FlatCoords& x0)
{
    int ne = mesh.numele;
    std::vector<RefConfig> rc(ne);

    for (int ie = 0; ie < ne; ++ie) {
        int v1 = mesh.connect[ie].vertices[0]; // 0-based
        int v2 = mesh.connect[ie].vertices[1];
        int v3 = mesh.connect[ie].vertices[2];

        // Fortran: temp(1,1)=x(3*v2-2)-x(3*v1-2)  x-component edge 1→2
        double t00 = x0[v2*3+0] - x0[v1*3+0];
        double t01 = x0[v3*3+0] - x0[v1*3+0];
        double t10 = x0[v2*3+1] - x0[v1*3+1];
        double t11 = x0[v3*3+1] - x0[v1*3+1];

        double J0 = t00 * t11 - t01 * t10;
        rc[ie].J0 = J0;

        double inv_J0 = 1.0 / J0;
        rc[ie].F0[0][0] =  inv_J0 * t11;
        rc[ie].F0[0][1] = -inv_J0 * t01;
        rc[ie].F0[1][0] = -inv_J0 * t10;
        rc[ie].F0[1][1] =  inv_J0 * t00;
    }
    return rc;
}

} // namespace fce
