// Gauss quadrature setup — translates gauss() from gauss.f90.

#include "fce/quadrature.hpp"
#include "fce/bspline.hpp"
#include <stdexcept>

namespace fce {

static void fill_gauss_point(GaussData& gd, int ig, double vg, double wg)
{
    std::array<double,12> N;
    std::array<std::array<double,2>,12> DN;
    std::array<std::array<double,3>,12> DDN;

    BSpline(N, vg, wg);
    DBSpline(DN, vg, wg);
    DDBSpline(DDN, vg, wg);

    for (int i = 0; i < 12; ++i) {
        gd.shapef[ig][i][0] = N[i];
        gd.shapef[ig][i][1] = DN[i][0];  // dN/dv
        gd.shapef[ig][i][2] = DN[i][1];  // dN/dw
        gd.shapef[ig][i][3] = DDN[i][0]; // d²N/dvdv
        gd.shapef[ig][i][4] = DDN[i][1]; // d²N/dwdw
        gd.shapef[ig][i][5] = DDN[i][2]; // d²N/dvdw
    }
}

GaussData setup_gauss(int ngauss)
{
    GaussData gd;
    gd.ngauss = ngauss;
    gd.shapef.resize(ngauss);
    gd.weight.resize(ngauss);

    if (ngauss == 1) {
        fill_gauss_point(gd, 0, 1.0/3.0, 1.0/3.0);
        gd.weight[0] = 1.0;
    } else if (ngauss == 2) {
        double pos1 = 1.0 / 6.0;
        double pos2 = 2.0 / 3.0;
        fill_gauss_point(gd, 0, pos1, pos2);
        fill_gauss_point(gd, 1, pos2, pos1);
        gd.weight[0] = 0.5;
        gd.weight[1] = 0.5;
    } else if (ngauss == 3) {
        double pos1 = 1.0 / 6.0;
        double pos2 = 2.0 / 3.0;
        fill_gauss_point(gd, 0, pos1, pos1);
        fill_gauss_point(gd, 1, pos2, pos1);
        fill_gauss_point(gd, 2, pos1, pos2);
        gd.weight[0] = 1.0/3.0;
        gd.weight[1] = 1.0/3.0;
        gd.weight[2] = 1.0/3.0;
    } else {
        throw std::runtime_error("setup_gauss: ngauss not supported: " +
                                 std::to_string(ngauss));
    }
    return gd;
}

} // namespace fce
