#include "fce/exponential.hpp"

#if defined(__clang__)
#pragma clang fp contract(off)
#endif

#include "fce/taylor.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace fce {
namespace {

constexpr std::array<std::array<int, 2>, 3> kBondPermutations{{
    {{1, 2}},
    {{2, 0}},
    {{0, 1}},
}};

Voigt3 operator*(double s, const Voigt3& a) {
    return Voigt3{s * a[0], s * a[1], s * a[2]};
}

Voigt3 operator*(const Voigt3& a, double s) {
    return s * a;
}

Voigt3 operator/(const Voigt3& a, double s) {
    return Voigt3{a[0] / s, a[1] / s, a[2] / s};
}

Voigt3 operator+(const Voigt3& a, const Voigt3& b) {
    return Voigt3{a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

Voigt3 operator-(const Voigt3& a, const Voigt3& b) {
    return Voigt3{a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

Vec2 c_vec(const Voigt3& c, const Vec2& x) {
    return Vec2{
        c[0] * x[0] + c[2] * x[1],
        c[2] * x[0] + c[1] * x[1],
    };
}

double dot(const Vec2& a, const Vec2& b) {
    return a[0] * b[0] + a[1] * b[1];
}

double dot3(const std::array<double, 3>& a, const std::array<double, 3>& b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

double norm3(const std::array<double, 3>& a) {
    return std::sqrt(dot3(a, a));
}

}  // namespace

BondState compute_deformed_bonds(const Voigt3& C_elem,
                                 const Vec2& curvppal,
                                 const Mat22& vppal,
                                 const std::array<double, 3>& A_norm,
                                 const std::array<Vec2, 3>& Ei) {
    BondState out;
    std::array<std::array<double, 3>, 3> a_def{};

    for (int i = 0; i < 3; ++i) {
        const Vec2 ttemp = c_vec(C_elem, Ei[i]);
        const double temp3 = vppal[0][0] * ttemp[0] + vppal[0][1] * ttemp[1];
        const double temp4 = vppal[1][0] * ttemp[0] + vppal[1][1] * ttemp[1];
        const double p = A_norm[i] * temp3;
        const double q = A_norm[i] * temp4;

        const double f1 = sinxx(curvppal[0] * p);
        const double f2 = sinxx(curvppal[1] * q);
        const double f12 = sinxx(curvppal[0] * p / 2.0);
        const double f22 = sinxx(curvppal[1] * q / 2.0);

        a_def[i] = {
            p * f1,
            q * f2,
            curvppal[0] * p * p / 2.0 * f12 * f12 +
                curvppal[1] * q * q / 2.0 * f22 * f22,
        };
        out.pe[i] = std::sqrt(a_def[i][0] * a_def[i][0] +
                              a_def[i][1] * a_def[i][1] +
                              a_def[i][2] * a_def[i][2]);
    }

    for (int k = 0; k < 3; ++k) {
        const int i = kBondPermutations[k][0];
        const int j = kBondPermutations[k][1];
        const double temp6 = a_def[i][0] * a_def[j][0] +
                             a_def[i][1] * a_def[j][1] +
                             a_def[i][2] * a_def[j][2];
        // def_bonds_.f90 evaluates the two divisions sequentially.
        out.pe[3 + k] = temp6 / out.pe[i] / out.pe[j];
        out.pe[3 + k] = std::acos(out.pe[3 + k]);
    }

    return out;
}

BondStateWithDerivatives compute_deformed_bonds_with_derivatives(
    const Voigt3& C_elem,
    const Vec2& curvppal,
    const Mat22& vppal,
    const PrincipalDerivativeVector& dcurvppaldC,
    const PrincipalDerivativeVector& dcurvppaldk,
    const PrincipalDerivativeMatrix& dvppaldC,
    const PrincipalDerivativeMatrix& dvppaldk,
    const std::array<double, 3>& A_norm,
    const std::array<Vec2, 3>& Ei) {
    BondStateWithDerivatives out;
    std::array<std::array<double, 3>, 3> a_def{};
    std::array<std::array<Voigt3, 3>, 3> dadC{};
    std::array<std::array<Voigt3, 3>, 3> dadk{};

    for (int i = 0; i < 3; ++i) {
        const Vec2 ttemp = c_vec(C_elem, Ei[i]);
        const double temp3 = dot(vppal[0], ttemp);
        const double temp4 = dot(vppal[1], ttemp);
        const double p = A_norm[i] * temp3;
        const double q = A_norm[i] * temp4;

        const Voigt3 dtemp3 =
            ttemp[0] * dvppaldC[0][0] + ttemp[1] * dvppaldC[0][1] +
            Voigt3{vppal[0][0] * Ei[i][0], vppal[0][1] * Ei[i][1], vppal[0][0] * Ei[i][1] + vppal[0][1] * Ei[i][0]};
        const Voigt3 dtemp4 =
            ttemp[0] * dvppaldC[1][0] + ttemp[1] * dvppaldC[1][1] +
            Voigt3{vppal[1][0] * Ei[i][0], vppal[1][1] * Ei[i][1], vppal[1][0] * Ei[i][1] + vppal[1][1] * Ei[i][0]};
        const Voigt3 dpdC = A_norm[i] * dtemp3;
        const Voigt3 dqdC = A_norm[i] * dtemp4;

        const Voigt3 dpdk = A_norm[i] * (ttemp[0] * dvppaldk[0][0] + ttemp[1] * dvppaldk[0][1]);
        const Voigt3 dqdk = A_norm[i] * (ttemp[0] * dvppaldk[1][0] + ttemp[1] * dvppaldk[1][1]);

        const double f1 = sinxx(curvppal[0] * p);
        const double f2 = sinxx(curvppal[1] * q);
        const double f12 = sinxx(curvppal[0] * p / 2.0);
        const double f22 = sinxx(curvppal[1] * q / 2.0);

        a_def[i] = {
            p * f1,
            q * f2,
            curvppal[0] * p * p / 2.0 * f12 * f12 + curvppal[1] * q * q / 2.0 * f22 * f22,
        };

        const Voigt3 dtemp11 = p * dcurvppaldC[0] + curvppal[0] * dpdC;
        const Voigt3 dtemp22 = q * dcurvppaldC[1] + curvppal[1] * dqdC;
        const Voigt3 dtemp31 = p * (p * dcurvppaldC[0] + 2.0 * curvppal[0] * dpdC);
        const Voigt3 dtemp41 = q * (q * dcurvppaldC[1] + 2.0 * curvppal[1] * dqdC);

        const double g1 = dsinxx(curvppal[0] * p);
        const double g2 = dsinxx(curvppal[1] * q);
        const double g12 = dsinxx(curvppal[0] * p / 2.0);
        const double g22 = dsinxx(curvppal[1] * q / 2.0);

        dadC[i][0] = dpdC * f1 + dtemp11 * (p * g1);
        dadC[i][1] = dqdC * f2 + dtemp22 * (q * g2);
        // Keep the left-associated products and final division used by
        // exponential.f90.  In particular, Fortran evaluates
        // dtemp3*f12*f12, rather than dtemp3*(f12*f12), and divides the
        // completed sum by 2.  These are algebraically identical but the
        // last bits feed the cancellation-sensitive free gradient.
        for (int component = 0; component < 3; ++component) {
            double term1 = dtemp31[component] * f12;
            term1 *= f12;
            double term2 = curvppal[0] * p;
            term2 *= p;
            term2 *= f12;
            term2 *= g12;
            term2 *= dtemp11[component];
            double term3 = dtemp41[component] * f22;
            term3 *= f22;
            double term4 = curvppal[1] * q;
            term4 *= q;
            term4 *= f22;
            term4 *= g22;
            term4 *= dtemp22[component];
            dadC[i][2][component] = (term1 + term2 + term3 + term4) / 2.0;
        }

        const Voigt3 dtemp12 = p * dcurvppaldk[0] + curvppal[0] * dpdk;
        const Voigt3 dtemp23 = q * dcurvppaldk[1] + curvppal[1] * dqdk;
        const Voigt3 dtemp32 = p * (p * dcurvppaldk[0] + 2.0 * curvppal[0] * dpdk);
        const Voigt3 dtemp42 = q * (q * dcurvppaldk[1] + 2.0 * curvppal[1] * dqdk);

        dadk[i][0] = dpdk * f1 + dtemp12 * (p * g1);
        dadk[i][1] = dqdk * f2 + dtemp23 * (q * g2);
        for (int component = 0; component < 3; ++component) {
            double term1 = dtemp32[component] * f12;
            term1 *= f12;
            double term2 = curvppal[0] * p;
            term2 *= p;
            term2 *= f12;
            term2 *= g12;
            term2 *= dtemp12[component];
            double term3 = dtemp42[component] * f22;
            term3 *= f22;
            double term4 = curvppal[1] * q;
            term4 *= q;
            term4 *= f22;
            term4 *= g22;
            term4 *= dtemp23[component];
            dadk[i][2][component] = (term1 + term2 + term3 + term4) / 2.0;
        }

        out.pe[i] = norm3(a_def[i]);
        if (out.pe[i] <= 0.0) {
            throw std::invalid_argument("compute_deformed_bonds encountered zero bond deformation norm");
        }

        out.dpedC[i] =
            (a_def[i][0] * dadC[i][0] + a_def[i][1] * dadC[i][1] + a_def[i][2] * dadC[i][2]) / out.pe[i];
        out.dpedk[i] =
            (a_def[i][0] * dadk[i][0] + a_def[i][1] * dadk[i][1] + a_def[i][2] * dadk[i][2]) / out.pe[i];
    }

    for (int k = 0; k < 3; ++k) {
        const int i = kBondPermutations[k][0];
        const int j = kBondPermutations[k][1];
        const double temp6 = dot3(a_def[i], a_def[j]);
        // Match def_bonds.f90's sequential divisions.  Multiplying the two
        // norms first changes the angle and its derivative by a last-bit
        // amount on the flat cyclic seed.
        const double cosine = temp6 / out.pe[i] / out.pe[j];
        out.pe[3 + k] = std::acos(cosine);
        const double fact = -1.0 / std::sin(out.pe[3 + k]) / out.pe[i] / out.pe[j];

        const Voigt3 dtemp11 =
            a_def[j][0] * dadC[i][0] + a_def[j][1] * dadC[i][1] + a_def[j][2] * dadC[i][2] +
            a_def[i][0] * dadC[j][0] + a_def[i][1] * dadC[j][1] + a_def[i][2] * dadC[j][2];
        out.dpedC[3 + k] =
            fact * (dtemp11 - temp6 * (out.dpedC[i] / out.pe[i] + out.dpedC[j] / out.pe[j]));

        const Voigt3 dtemp12 =
            a_def[j][0] * dadk[i][0] + a_def[j][1] * dadk[i][1] + a_def[j][2] * dadk[i][2] +
            a_def[i][0] * dadk[j][0] + a_def[i][1] * dadk[j][1] + a_def[i][2] * dadk[j][2];
        out.dpedk[3 + k] =
            fact * (dtemp12 - temp6 * (out.dpedk[i] / out.pe[i] + out.dpedk[j] / out.pe[j]));
    }

    return out;
}

}  // namespace fce
