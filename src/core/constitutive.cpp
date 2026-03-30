#include "fce/constitutive.hpp"

#include "fce/taylor.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace fce {
namespace {

using Vec3 = std::array<double, 3>;

constexpr std::array<std::array<int, 2>, 3> kBondPermutations{{
    {{1, 2}},
    {{2, 0}},
    {{0, 1}},
}};

double dot(const Vec2& a, const Vec2& b) {
    return a[0] * b[0] + a[1] * b[1];
}

double dot(const Vec3& a, const Vec3& b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

double norm(const Vec2& a) {
    return std::sqrt(dot(a, a));
}

double norm(const Vec3& a) {
    return std::sqrt(dot(a, a));
}

Vec2 operator+(const Vec2& a, const Vec2& b) {
    return Vec2{a[0] + b[0], a[1] + b[1]};
}

Vec2 operator-(const Vec2& a, const Vec2& b) {
    return Vec2{a[0] - b[0], a[1] - b[1]};
}

Vec2 operator*(const double s, const Vec2& a) {
    return Vec2{s * a[0], s * a[1]};
}

Vec2 operator*(const Vec2& a, const double s) {
    return s * a;
}

Vec2 operator/(const Vec2& a, const double s) {
    return Vec2{a[0] / s, a[1] / s};
}

Voigt3 operator+(const Voigt3& a, const Voigt3& b) {
    return Voigt3{a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

Voigt3 operator-(const Voigt3& a, const Voigt3& b) {
    return Voigt3{a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

Voigt3 operator*(const double s, const Voigt3& a) {
    return Voigt3{s * a[0], s * a[1], s * a[2]};
}

Voigt3 operator/(const Voigt3& a, const double s) {
    return Voigt3{a[0] / s, a[1] / s, a[2] / s};
}

Vec2 c_vec(const Voigt3& c, const Vec2& x) {
    return Vec2{
        c[0] * x[0] + c[2] * x[1],
        c[2] * x[0] + c[1] * x[1],
    };
}

Voigt3 symmetric_half(const Vec2& a, const Vec2& b) {
    return Voigt3{
        a[0] * b[0],
        a[1] * b[1],
        0.5 * (a[0] * b[1] + a[1] * b[0]),
    };
}

void validate_brenner_material(const MatData& mat) {
    if (mat.s0 == 0.0) {
        throw std::invalid_argument("Brenner material requires nonzero s0");
    }
    if (mat.Vs[2] <= 0.0 || mat.Va[2] <= 0.0) {
        throw std::invalid_argument("Brenner material requires positive Vs(3) and Va(3)");
    }
}

std::array<Vec3, 3> gang_bis(const Vec3& theta, const MatData& mat) {
    std::array<Vec3, 3> ga{};
    for (int i = 0; i < 3; ++i) {
        const double aux0 = std::cos(theta[i]);
        const double aux1 = 1.0 + aux0;
        const double aux2 = mat.Va[2] + aux1 * aux1;
        ga[0][i] = 1.0 + (mat.Va[1] / mat.Va[2]) - (mat.Va[1] / aux2);
        ga[1][i] = -2.0 / (aux2 * aux2) * mat.Va[1] * aux1 * std::sin(theta[i]);
        ga[2][i] =
            2.0 / (aux2 * aux2 * aux2) * mat.Va[1] *
            (((1.0 - aux0 - 2.0 * aux0 * aux0) * aux2) -
             4.0 * std::pow(aux1 * std::sin(theta[i]), 2.0));
    }
    for (auto& row : ga) {
        for (double& value : row) {
            value *= mat.Va[0];
        }
    }
    return ga;
}

Vec3 vrep_bis(const double a, const MatData& mat) {
    const double aux = std::sqrt(2.0 * mat.Vs[2]) * mat.Vs[1];
    const double ex = std::exp(-aux * (a - mat.A1));
    const double v0 = mat.Vs[0] / (mat.Vs[2] - 1.0) * ex;
    return Vec3{v0, -v0 * aux, v0 * aux * aux};
}

Vec3 vatt_bis(const double a, const MatData& mat) {
    const double aux = std::sqrt(2.0 / mat.Vs[2]) * mat.Vs[1];
    const double ex = std::exp(-aux * (a - mat.A1));
    const double v0 = mat.Vs[0] * mat.Vs[2] / (mat.Vs[2] - 1.0) * ex;
    return Vec3{v0, -v0 * aux, v0 * aux * aux};
}

struct InnerBrennerEtaOutput {
    double W{0.0};
    Vec2 dWdeta{};
    Voigt3 ddWdeta{};
    Vec6 dW{};
    Mat66 ddW{};
};

InnerBrennerEtaOutput evaluate_inner_brenner(const MatData& mat,
                                             const Vec6& pe,
                                             const std::array<Vec2, 6>& dpedeta,
                                             const std::array<Voigt3, 6>& ddpedeta) {
    const BrennerOutput brenner = evaluate_brenner(mat, pe);

    std::array<Vec2, 6> aux{};
    for (int i = 0; i < 6; ++i) {
        aux[i] = Vec2{0.0, 0.0};
        for (int j = 0; j < 6; ++j) {
            aux[i] = aux[i] + brenner.ddW[i][j] * dpedeta[j];
        }
    }

    InnerBrennerEtaOutput out;
    out.W = brenner.W;
    out.dW = brenner.dW;
    out.ddW = brenner.ddW;
    out.dWdeta = Vec2{0.0, 0.0};
    out.ddWdeta = Voigt3{0.0, 0.0, 0.0};

    for (int i = 0; i < 6; ++i) {
        out.dWdeta = out.dWdeta + brenner.dW[i] * dpedeta[i];
        out.ddWdeta = out.ddWdeta + symmetric_half(aux[i], dpedeta[i]) +
                      brenner.dW[i] * ddpedeta[i];
    }

    return out;
}

}  // namespace

BrennerOutput evaluate_brenner(const MatData& mat, const Vec6& pe) {
    validate_brenner_material(mat);
    for (int i = 0; i < 3; ++i) {
        if (pe[i] <= 0.0) {
            throw std::invalid_argument("Brenner bond lengths must be positive");
        }
    }

    BrennerOutput out;
    const Vec3 theta{pe[3], pe[4], pe[5]};
    const auto ga = gang_bis(theta, mat);

    for (int ibond = 0; ibond < 3; ++ibond) {
        const int ip1 = kBondPermutations[ibond][0];
        const int ip2 = kBondPermutations[ibond][1];
        const double fang =
            1.0 / std::sqrt(1.0 + ga[0][ip1] + ga[0][ip2]);
        const Vec3 vr = vrep_bis(pe[ibond], mat);
        const Vec3 va = vatt_bis(pe[ibond], mat);

        out.W += vr[0] - fang * va[0];
        out.dW[ibond] = vr[1] - fang * va[1];
        out.dW[3 + ip1] += va[0] * std::pow(fang, 3.0) * ga[1][ip1] / 2.0;
        out.dW[3 + ip2] += va[0] * std::pow(fang, 3.0) * ga[1][ip2] / 2.0;

        out.ddW[ibond][ibond] = vr[2] - fang * va[2];
        out.ddW[3 + ip1][3 + ip1] +=
            ga[2][ip1] / 2.0 * va[0] * std::pow(fang, 3.0) -
            0.75 * ga[1][ip1] * ga[1][ip1] * va[0] * std::pow(fang, 5.0);
        out.ddW[3 + ip2][3 + ip2] +=
            ga[2][ip2] / 2.0 * va[0] * std::pow(fang, 3.0) -
            0.75 * ga[1][ip2] * ga[1][ip2] * va[0] * std::pow(fang, 5.0);
        out.ddW[ibond][3 + ip1] = va[1] * std::pow(fang, 3.0) * ga[1][ip1] / 2.0;
        out.ddW[ibond][3 + ip2] = va[1] * std::pow(fang, 3.0) * ga[1][ip2] / 2.0;
        out.ddW[3 + ip1][3 + ip2] =
            -0.75 * va[0] * std::pow(fang, 5.0) * ga[1][ip1] * ga[1][ip2];
    }

    for (int i = 0; i < 6; ++i) {
        for (int j = i + 1; j < 6; ++j) {
            if (out.ddW[i][j] == 0.0 && out.ddW[j][i] != 0.0) {
                out.ddW[i][j] = out.ddW[j][i];
            } else {
                out.ddW[j][i] = out.ddW[i][j];
            }
        }
    }

    out.W /= mat.s0;
    for (double& value : out.dW) {
        value /= mat.s0;
    }
    for (auto& row : out.ddW) {
        for (double& value : row) {
            value /= mat.s0;
        }
    }
    return out;
}

InnerPotentialOutput evaluate_inner_potential(const Voigt3& C_elem,
                                              const Vec2& curvppal,
                                              const Mat22& vppal,
                                              const MatData& mat,
                                              const Vec2& eta) {
    if (mat.nCode_Pot != 2) {
        throw std::invalid_argument("Only Brenner nCode_Pot=2 is supported");
    }

    std::array<Vec2, 3> Ei{};
    Vec3 A_norm{};
    for (int ibond = 0; ibond < 3; ++ibond) {
        Ei[ibond] = mat.A0 * mat.E[ibond] + eta;
        A_norm[ibond] = norm(Ei[ibond]);
        if (A_norm[ibond] <= 0.0) {
            throw std::invalid_argument("Inner potential encountered zero-norm bond vector");
        }
        Ei[ibond] = Ei[ibond] / A_norm[ibond];
    }

    const Vec2 ttemp1{
        C_elem[0] * vppal[0][0] + C_elem[2] * vppal[0][1],
        C_elem[2] * vppal[0][0] + C_elem[1] * vppal[0][1],
    };
    const Vec2 ttemp2{
        C_elem[0] * vppal[1][0] + C_elem[2] * vppal[1][1],
        C_elem[2] * vppal[1][0] + C_elem[1] * vppal[1][1],
    };
    const Vec2 dpdeta = ttemp1;
    const Vec2 dqdeta = ttemp2;
    const Voigt3 dpdeta2{dpdeta[0] * dpdeta[0], dpdeta[1] * dpdeta[1], dpdeta[0] * dpdeta[1]};
    const Voigt3 dqdeta2{dqdeta[0] * dqdeta[0], dqdeta[1] * dqdeta[1], dqdeta[0] * dqdeta[1]};

    std::array<Vec3, 3> a_def{};
    std::array<std::array<Vec2, 3>, 3> dadeta{};
    std::array<std::array<Voigt3, 3>, 3> ddadeta{};
    std::array<Vec2, 6> dpedeta_all{};
    std::array<Voigt3, 6> ddpedeta{};
    Vec6 pe{};

    for (int i = 0; i < 3; ++i) {
        const Vec2 ttemp = c_vec(C_elem, Ei[i]);
        const double temp3 = dot(vppal[0], ttemp);
        const double temp4 = dot(vppal[1], ttemp);
        const double p = A_norm[i] * temp3;
        const double q = A_norm[i] * temp4;

        const double f1 = sinxx(curvppal[0] * p);
        const double f2 = sinxx(curvppal[1] * q);
        const double f12 = sinxx(curvppal[0] * p / 2.0);
        const double f22 = sinxx(curvppal[1] * q / 2.0);

        a_def[i][0] = p * f1;
        a_def[i][1] = q * f2;
        a_def[i][2] = curvppal[0] * p * p / 2.0 * f12 * f12 +
                      curvppal[1] * q * q / 2.0 * f22 * f22;

        const double g1 = dsinxx(curvppal[0] * p);
        const double g2 = dsinxx(curvppal[1] * q);
        const double g12 = dsinxx(curvppal[0] * p / 2.0);
        const double g22 = dsinxx(curvppal[1] * q / 2.0);

        dadeta[i][0] = (f1 + curvppal[0] * p * g1) * dpdeta;
        dadeta[i][1] = (f2 + curvppal[1] * q * g2) * dqdeta;

        const double xx1 = f12 + curvppal[0] * p / 2.0 * g12;
        const double xx2 = f22 + curvppal[1] * q / 2.0 * g22;
        const double yy1 = curvppal[0] * p * f12;
        const double yy2 = curvppal[1] * q * f22;
        dadeta[i][2] = yy1 * xx1 * dpdeta + yy2 * xx2 * dqdeta;

        const double h1 = ddsinxx(curvppal[0] * p);
        const double h2 = ddsinxx(curvppal[1] * q);
        const double h12 = ddsinxx(curvppal[0] * p / 2.0);
        const double h22 = ddsinxx(curvppal[1] * q / 2.0);

        ddadeta[i][0] = curvppal[0] * (2.0 * g1 + curvppal[0] * p * h1) * dpdeta2;
        ddadeta[i][1] = curvppal[1] * (2.0 * g2 + curvppal[1] * q * h2) * dqdeta2;
        ddadeta[i][2] =
            curvppal[0] * (xx1 * xx1 + yy1 * (g12 + curvppal[0] * p / 4.0 * h12)) * dpdeta2 +
            curvppal[1] * (xx2 * xx2 + yy2 * (g22 + curvppal[1] * q / 4.0 * h22)) * dqdeta2;

        pe[i] = norm(a_def[i]);
        if (pe[i] <= 0.0) {
            throw std::invalid_argument("Inner potential encountered zero bond deformation norm");
        }
        dpedeta_all[i] =
            (a_def[i][0] * dadeta[i][0] + a_def[i][1] * dadeta[i][1] + a_def[i][2] * dadeta[i][2]) / pe[i];

        Voigt3 aux1{
            dadeta[i][0][0] * dadeta[i][0][0] + dadeta[i][1][0] * dadeta[i][1][0] +
                dadeta[i][2][0] * dadeta[i][2][0],
            dadeta[i][0][1] * dadeta[i][0][1] + dadeta[i][1][1] * dadeta[i][1][1] +
                dadeta[i][2][1] * dadeta[i][2][1],
            dadeta[i][0][0] * dadeta[i][0][1] + dadeta[i][1][0] * dadeta[i][1][1] +
                dadeta[i][2][0] * dadeta[i][2][1],
        };
        aux1 = aux1 + a_def[i][0] * ddadeta[i][0] + a_def[i][1] * ddadeta[i][1] + a_def[i][2] * ddadeta[i][2];
        const Voigt3 aux2{
            dpedeta_all[i][0] * dpedeta_all[i][0],
            dpedeta_all[i][1] * dpedeta_all[i][1],
            dpedeta_all[i][0] * dpedeta_all[i][1],
        };
        ddpedeta[i] = (aux1 - aux2) / pe[i];
    }

    for (int k = 0; k < 3; ++k) {
        const int i = kBondPermutations[k][0];
        const int j = kBondPermutations[k][1];
        const double temp6 = dot(a_def[i], a_def[j]);
        const double temp7 = std::clamp(temp6 / (pe[i] * pe[j]), -1.0, 1.0);
        pe[3 + k] = std::acos(temp7);
        const double sin_theta = std::sin(pe[3 + k]);
        if (std::abs(sin_theta) < 1e-15) {
            throw std::invalid_argument("Inner potential encountered singular bond angle derivative");
        }

        Vec2 ttemp = a_def[j][0] * dadeta[i][0] + a_def[j][1] * dadeta[i][1] + a_def[j][2] * dadeta[i][2] +
                     a_def[i][0] * dadeta[j][0] + a_def[i][1] * dadeta[j][1] + a_def[i][2] * dadeta[j][2];
        ttemp = (ttemp - temp6 * (dpedeta_all[i] / pe[i] + dpedeta_all[j] / pe[j])) / (pe[i] * pe[j]);
        dpedeta_all[3 + k] = (-1.0 / sin_theta) * ttemp;

        Voigt3 aux1{0.0, 0.0, 0.0};
        for (int icomp = 0; icomp < 3; ++icomp) {
            aux1 = aux1 + Voigt3{
                               2.0 * dadeta[i][icomp][0] * dadeta[j][icomp][0],
                               2.0 * dadeta[i][icomp][1] * dadeta[j][icomp][1],
                               dadeta[i][icomp][0] * dadeta[j][icomp][1] +
                                   dadeta[i][icomp][1] * dadeta[j][icomp][0],
                           };
        }
        aux1 = aux1 + a_def[j][0] * ddadeta[i][0] + a_def[j][1] * ddadeta[i][1] + a_def[j][2] * ddadeta[i][2] +
               a_def[i][0] * ddadeta[j][0] + a_def[i][1] * ddadeta[j][1] + a_def[i][2] * ddadeta[j][2];

        const Vec2 xaux = pe[j] * dpedeta_all[i] + pe[i] * dpedeta_all[j];
        Voigt3 aux2{
            2.0 * ttemp[0] * xaux[0],
            2.0 * ttemp[1] * xaux[1],
            ttemp[0] * xaux[1] + ttemp[1] * xaux[0],
        };
        aux2 = (aux1 - aux2 -
                temp7 * (pe[j] * ddpedeta[i] + pe[i] * ddpedeta[j] +
                         Voigt3{
                             2.0 * dpedeta_all[i][0] * dpedeta_all[j][0],
                             2.0 * dpedeta_all[i][1] * dpedeta_all[j][1],
                             dpedeta_all[i][0] * dpedeta_all[j][1] +
                                 dpedeta_all[i][1] * dpedeta_all[j][0],
                         })) /
               (pe[i] * pe[j]);
        const Voigt3 aux3{
            dpedeta_all[3 + k][0] * dpedeta_all[3 + k][0],
            dpedeta_all[3 + k][1] * dpedeta_all[3 + k][1],
            dpedeta_all[3 + k][0] * dpedeta_all[3 + k][1],
        };
        ddpedeta[3 + k] = (-1.0 / sin_theta) * (temp7 * aux3 + aux2);
    }

    const InnerBrennerEtaOutput inner = evaluate_inner_brenner(mat, pe, dpedeta_all, ddpedeta);

    InnerPotentialOutput out;
    out.W = inner.W;
    out.dWdeta = inner.dWdeta;
    out.ddWdeta = inner.ddWdeta;
    out.dW_dpe = inner.dW;
    return out;
}

NewtonInnerOutput solve_inner_newton(const Voigt3& C_elem,
                                     const Vec2& curvppal,
                                     const Mat22& vppal,
                                     const MatData& mat,
                                     const Vec2& eta0,
                                     const double crit,
                                     const int max_iter) {
    if (mat.nCode_Pot != 2) {
        throw std::invalid_argument("Only Brenner nCode_Pot=2 is supported");
    }

    NewtonInnerOutput out;
    out.eta = eta0;
    out.iterations = 0;
    out.fail_mode = 0;

    InnerPotentialOutput current =
        evaluate_inner_potential(C_elem, curvppal, vppal, mat, out.eta);
    double test = crit * (1.0 + std::abs(current.W));
    double gnorm = norm(current.dWdeta);

    if (gnorm <= test) {
        out.W = current.W;
        out.dWdeta = current.dWdeta;
        out.ddWdeta = current.ddWdeta;
        out.dW_dpe = current.dW_dpe;
        return out;
    }

    while (gnorm > test && out.iterations < max_iter) {
        ++out.iterations;
        const double det = current.ddWdeta[0] * current.ddWdeta[1] -
                           current.ddWdeta[2] * current.ddWdeta[2];

        if (std::abs(det) < 1e-15) {
            if (gnorm <= test * 10.0) {
                break;
            }
            out.fail_mode = 1;
            break;
        }

        Vec2 dx{
            (current.dWdeta[1] * current.ddWdeta[2] -
             current.dWdeta[0] * current.ddWdeta[1]) /
                det,
            (current.dWdeta[0] * current.ddWdeta[2] -
             current.dWdeta[1] * current.ddWdeta[0]) /
                det,
        };

        const double step_len = norm(dx);
        if (step_len > 0.1 * mat.A0) {
            dx = dx * (0.1 * mat.A0 / step_len);
        }

        out.eta = out.eta + dx;
        if (norm(out.eta) > 0.5 * mat.A0) {
            out.fail_mode = 2;
            break;
        }

        current = evaluate_inner_potential(C_elem, curvppal, vppal, mat, out.eta);
        test = crit * (1.0 + std::abs(current.W));
        gnorm = norm(current.dWdeta);
    }

    if (gnorm > test && out.fail_mode == 0) {
        out.fail_mode = 3;
    }

    current = evaluate_inner_potential(C_elem, curvppal, vppal, mat, out.eta);
    out.W = current.W;
    out.dWdeta = current.dWdeta;
    out.ddWdeta = current.ddWdeta;
    out.dW_dpe = current.dW_dpe;
    return out;
}

}  // namespace fce
