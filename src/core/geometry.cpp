#include "fce/geometry.hpp"

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

    for (int idir = 0; idir < 2; ++idir) {
        for (int coord = 0; coord < 3; ++coord) {
            for (int node = 0; node < 12; ++node) {
                g_convect[idir][coord] += dn[node][idir] * xneigh[node][coord];
            }
        }
    }

    const Voigt3 g_elem{
        dot(g_convect[0], g_convect[0]),
        dot(g_convect[1], g_convect[1]),
        dot(g_convect[0], g_convect[1]),
    };
    out.C_elem = pull_back(g_elem, f0);

    out.xnor_elem = Vec3{
        g_convect[0][1] * g_convect[1][2] - g_convect[0][2] * g_convect[1][1],
        g_convect[0][2] * g_convect[1][0] - g_convect[0][0] * g_convect[1][2],
        g_convect[0][0] * g_convect[1][1] - g_convect[0][1] * g_convect[1][0],
    };
    const double xnorm = norm(out.xnor_elem);
    if (xnorm <= 1e-16 || !std::isfinite(xnorm)) {
        throw std::invalid_argument("geometry metric encountered degenerate surface normal");
    }
    out.xnor_elem = (1.0 / xnorm) * out.xnor_elem;

    for (int node = 0; node < 12; ++node) {
        for (int idof = 0; idof < 3; ++idof) {
            const double dn1 = dn[node][0];
            const double dn2 = dn[node][1];
            const Vec3 temp{
                dn2 * g_convect[0][0] - dn1 * g_convect[1][0],
                dn2 * g_convect[0][1] - dn1 * g_convect[1][1],
                dn2 * g_convect[0][2] - dn1 * g_convect[1][2],
            };
            const std::array<Vec3, 3> temp1{
                Vec3{0.0, temp[2], -temp[1]},
                Vec3{-temp[2], 0.0, temp[0]},
                Vec3{temp[1], -temp[0], 0.0},
            };

            const Voigt3 dg{
                2.0 * dn1 * g_convect[0][idof],
                2.0 * dn2 * g_convect[1][idof],
                dn1 * g_convect[1][idof] + dn2 * g_convect[0][idof],
            };
            out.dC[node][idof] = pull_back(dg, f0);

            const double dJ =
                ((dn1 * g_elem[1] - dn2 * g_elem[2]) * g_convect[0][idof] -
                 (dn1 * g_elem[2] - dn2 * g_elem[0]) * g_convect[1][idof]) /
                xnorm;
            out.dnorm[node][idof] = (1.0 / xnorm) * (temp1[idof] - dJ * out.xnor_elem);
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

    for (int idir = 0; idir < 3; ++idir) {
        for (int coord = 0; coord < 3; ++coord) {
            for (int node = 0; node < 12; ++node) {
                aux[idir][coord] += ddn[node][idir] * xneigh[node][coord];
            }
        }
    }

    const Voigt3 curv0_aux{
        dot(xnor_elem, aux[0]),
        dot(xnor_elem, aux[1]),
        dot(xnor_elem, aux[2]),
    };
    out.curv0_elem = pull_back(curv0_aux, f0);

    for (int node = 0; node < 12; ++node) {
        for (int idof = 0; idof < 3; ++idof) {
            const Voigt3 dk{
                dot(aux[0], dnorm[node][idof]) + ddn[node][0] * xnor_elem[idof],
                dot(aux[1], dnorm[node][idof]) + ddn[node][1] * xnor_elem[idof],
                dot(aux[2], dnorm[node][idof]) + ddn[node][2] * xnor_elem[idof],
            };
            out.dcurv[node][idof] = pull_back(dk, f0);
        }
    }

    return out;
}

}  // namespace fce
