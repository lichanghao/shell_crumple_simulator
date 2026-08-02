#include "fce/geometry.hpp"

#if defined(__clang__)
#pragma clang fp contract(off)
#endif

#include <cmath>
#include <stdexcept>

namespace fce {
namespace {

Vec3 operator-(const Vec3& a, const Vec3& b) {
    return Vec3{a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

Vec3 operator*(double s, const Vec3& a) {
    return Vec3{s * a[0], s * a[1], s * a[2]};
}

double dot(const Vec3& a, const Vec3& b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

double norm(const Vec3& a) {
    return std::sqrt(dot(a, a));
}

Voigt3 pull_back(const Voigt3& source, const Mat22& f0) {
    return Voigt3{
        source[0] * f0[0][0] * f0[0][0] + 2.0 * source[2] * f0[0][0] * f0[1][0] +
            source[1] * f0[1][0] * f0[1][0],
        source[0] * f0[0][1] * f0[0][1] + 2.0 * source[2] * f0[0][1] * f0[1][1] +
            source[1] * f0[1][1] * f0[1][1],
        source[0] * f0[0][0] * f0[0][1] +
            source[2] * (f0[0][0] * f0[1][1] + f0[0][1] * f0[1][0]) +
            source[1] * f0[1][0] * f0[1][1],
    };
}

}  // namespace

MetricResult compute_metric(const NeighborCoords12& xneigh, const ShapeGradient12& dn, const Mat22& f0) {
    MetricResult out;
    std::array<Vec3, 2> g_convect{};
    const double f00 = f0[0][0];
    const double f01 = f0[0][1];
    const double f10 = f0[1][0];
    const double f11 = f0[1][1];
    const double shear_coeff = f00 * f11 + f01 * f10;

    for (int idir = 0; idir < 2; ++idir) {
        for (int coord = 0; coord < 3; ++coord) {
            for (int node = 0; node < 12; ++node) {
                g_convect[idir][coord] += dn[node][idir] * xneigh[node][coord];
            }
        }
    }

    const double g11 = g_convect[0][0] * g_convect[0][0] +
                       g_convect[0][1] * g_convect[0][1] +
                       g_convect[0][2] * g_convect[0][2];
    const double g22 = g_convect[1][0] * g_convect[1][0] +
                       g_convect[1][1] * g_convect[1][1] +
                       g_convect[1][2] * g_convect[1][2];
    const double g12 = g_convect[0][0] * g_convect[1][0] +
                       g_convect[0][1] * g_convect[1][1] +
                       g_convect[0][2] * g_convect[1][2];
    const Voigt3 g_elem{g11, g22, g12};

    out.C_elem[0] = g_elem[0] * f00 * f00 +
                    2.0 * g_elem[2] * f00 * f10 +
                    g_elem[1] * f10 * f10;
    out.C_elem[2] = g_elem[0] * f00 * f01 +
                    g_elem[2] * shear_coeff +
                    g_elem[1] * f10 * f11;
    out.C_elem[1] = g_elem[0] * f01 * f01 +
                    2.0 * g_elem[2] * f01 * f11 +
                    g_elem[1] * f11 * f11;

    out.xnor_elem = Vec3{
        g_convect[0][1] * g_convect[1][2] - g_convect[0][2] * g_convect[1][1],
        g_convect[0][2] * g_convect[1][0] - g_convect[0][0] * g_convect[1][2],
        g_convect[0][0] * g_convect[1][1] - g_convect[0][1] * g_convect[1][0],
    };
    const double xnorm = norm(out.xnor_elem);
    if (xnorm <= 1e-16 || !std::isfinite(xnorm)) {
        throw std::invalid_argument("geometry metric encountered degenerate surface normal");
    }
    out.xnor_elem = Vec3{
        out.xnor_elem[0] / xnorm,
        out.xnor_elem[1] / xnorm,
        out.xnor_elem[2] / xnorm,
    };

    for (int node = 0; node < 12; ++node) {
        for (int idof = 0; idof < 3; ++idof) {
            const double dn1 = dn[node][0];
            const double dn2 = dn[node][1];
            const double temp0 = dn2 * g_convect[0][0] - dn1 * g_convect[1][0];
            const double temp1 = dn2 * g_convect[0][1] - dn1 * g_convect[1][1];
            const double temp2 = dn2 * g_convect[0][2] - dn1 * g_convect[1][2];

            const Voigt3 dg{
                2.0 * dn1 * g_convect[0][idof],
                2.0 * dn2 * g_convect[1][idof],
                dn1 * g_convect[1][idof] + dn2 * g_convect[0][idof],
            };
            out.dC[node][idof][0] = dg[0] * f00 * f00 +
                                     2.0 * dg[2] * f00 * f10 +
                                     dg[1] * f10 * f10;
            out.dC[node][idof][2] = dg[0] * f00 * f01 +
                                     dg[2] * shear_coeff +
                                     dg[1] * f10 * f11;
            out.dC[node][idof][1] = dg[0] * f01 * f01 +
                                     2.0 * dg[2] * f01 * f11 +
                                     dg[1] * f11 * f11;

            const double dJ =
                ((dn1 * g_elem[1] - dn2 * g_elem[2]) * g_convect[0][idof] -
                 (dn1 * g_elem[2] - dn2 * g_elem[0]) * g_convect[1][idof]) /
                xnorm;
            Vec3 cross_term{};
            if (idof == 0) {
                cross_term = Vec3{0.0, temp2, -temp1};
            } else if (idof == 1) {
                cross_term = Vec3{-temp2, 0.0, temp0};
            } else {
                cross_term = Vec3{temp1, -temp0, 0.0};
            }
            const Vec3 dnormal = cross_term - dJ * out.xnor_elem;
            out.dnorm[node][idof] = Vec3{
                dnormal[0] / xnorm,
                dnormal[1] / xnorm,
                dnormal[2] / xnorm,
            };
        }
    }

    return out;
}

CurvatureResult compute_curvature(const NeighborCoords12& xneigh,
                                  const ShapeCurvature12& ddn,
                                  const Mat22& f0,
                                  const Vec3& xnor_elem,
                                  const NodeDerivativeVec3& dnorm) {
    CurvatureResult out;
    std::array<Vec3, 3> aux{};
    const double f00 = f0[0][0];
    const double f01 = f0[0][1];
    const double f10 = f0[1][0];
    const double f11 = f0[1][1];
    const double shear_coeff = f00 * f11 + f01 * f10;

    for (int idir = 0; idir < 3; ++idir) {
        for (int coord = 0; coord < 3; ++coord) {
            for (int node = 0; node < 12; ++node) {
                aux[idir][coord] += ddn[node][idir] * xneigh[node][coord];
            }
        }
    }

    const Voigt3 curv0_aux{
        xnor_elem[0] * aux[0][0] + xnor_elem[1] * aux[0][1] + xnor_elem[2] * aux[0][2],
        xnor_elem[0] * aux[1][0] + xnor_elem[1] * aux[1][1] + xnor_elem[2] * aux[1][2],
        xnor_elem[0] * aux[2][0] + xnor_elem[1] * aux[2][1] + xnor_elem[2] * aux[2][2],
    };
    out.curv0_elem[0] = curv0_aux[0] * f00 * f00 +
                        2.0 * curv0_aux[2] * f00 * f10 +
                        curv0_aux[1] * f10 * f10;
    out.curv0_elem[2] = curv0_aux[0] * f00 * f01 +
                        curv0_aux[2] * shear_coeff +
                        curv0_aux[1] * f10 * f11;
    out.curv0_elem[1] = curv0_aux[0] * f01 * f01 +
                        2.0 * curv0_aux[2] * f01 * f11 +
                        curv0_aux[1] * f11 * f11;

    for (int node = 0; node < 12; ++node) {
        for (int idof = 0; idof < 3; ++idof) {
            const Voigt3 dk{
                aux[0][0] * dnorm[node][idof][0] + aux[0][1] * dnorm[node][idof][1] +
                    aux[0][2] * dnorm[node][idof][2] + ddn[node][0] * xnor_elem[idof],
                aux[1][0] * dnorm[node][idof][0] + aux[1][1] * dnorm[node][idof][1] +
                    aux[1][2] * dnorm[node][idof][2] + ddn[node][1] * xnor_elem[idof],
                aux[2][0] * dnorm[node][idof][0] + aux[2][1] * dnorm[node][idof][1] +
                    aux[2][2] * dnorm[node][idof][2] + ddn[node][2] * xnor_elem[idof],
            };
            out.dcurv[node][idof][0] = dk[0] * f00 * f00 +
                                       2.0 * dk[2] * f00 * f10 +
                                       dk[1] * f10 * f10;
            out.dcurv[node][idof][2] = dk[0] * f00 * f01 +
                                       dk[2] * shear_coeff +
                                       dk[1] * f10 * f11;
            out.dcurv[node][idof][1] = dk[0] * f01 * f01 +
                                       2.0 * dk[2] * f01 * f11 +
                                       dk[1] * f11 * f11;
        }
    }

    return out;
}

}  // namespace fce
