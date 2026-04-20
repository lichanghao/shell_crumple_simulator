#include "fce/constitutive.hpp"

#include "fce/element_state.hpp"
#include "fce/exponential.hpp"
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
constexpr double kBrennerCutoffRadius = 0.17;

double dot(const Vec2& a, const Vec2& b) {
    return a[0] * b[0] + a[1] * b[1];
}

double dot(const Vec3& a, const Vec3& b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

double norm(const Vec2& a) {
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

ElementState make_element_state_view(const Voigt3& C_elem,
                                     const Vec2& curvppal,
                                     const Mat22& vppal) {
    ElementState state;
    state.C_elem = C_elem;
    state.curvppal = curvppal;
    state.vppal = vppal;
    return state;
}

bool has_matching_prepared_bond_state(const ElementState& state,
                                      const MatData& mat,
                                      const Vec2& eta) {
    return state.has_prepared_bond_state &&
           state.prepared_eta == eta &&
           state.prepared_material_A0 == mat.A0 &&
           state.prepared_material_E == mat.E;
}

ElementState ensure_prepared_element_state(const ElementState& state,
                                           const MatData& mat,
                                           const Vec2& eta) {
    if (has_matching_prepared_bond_state(state, mat, eta)) {
        return state;
    }
    return prepare_element_state(state, mat, eta);
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

void validate_morse_material(const MatData& mat) {
    if (mat.s0 == 0.0 || mat.A0 == 0.0) {
        throw std::invalid_argument("Morse material requires nonzero A0 and s0");
    }
    if (mat.Vs[0] <= 0.0 || mat.Vs[1] <= 0.0 || mat.Va[0] <= 0.0) {
        throw std::invalid_argument("Morse material requires positive Vs(1:2) and Va(1)");
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

Vec3 vstretch_bis(const double a, const MatData& mat) {
    const double ex = std::exp(-mat.Vs[1] * (a - mat.A0));
    const double stretch = 1.0 - ex;
    return Vec3{
        mat.Vs[0] * stretch * stretch,
        mat.Vs[0] * 2.0 * mat.Vs[1] * ex * stretch,
        mat.Vs[0] * 2.0 * mat.Vs[1] * mat.Vs[1] * (ex * ex - ex * stretch),
    };
}

Vec3 vangle_bis(const double ang, const MatData& mat) {
    constexpr double kAngle0 = 2.0 * 3.14159265358979323846 / 3.0;
    const double t1 = ang - kAngle0;
    const double t2 = t1 * t1;
    const double t4 = t2 * t2;
    return Vec3{
        mat.Va[0] * 0.5 * t2 * (1.0 + mat.Va[1] * t4),
        mat.Va[0] * t1 * (1.0 + 3.0 * mat.Va[1] * t4),
        mat.Va[0] * (1.0 + 15.0 * mat.Va[1] * t4),
    };
}

struct InnerBrennerEtaOutput {
    double W{0.0};
    Vec2 dWdeta{};
    Voigt3 ddWdeta{};
    Vec6 dW{};
    Mat66 ddW{};
};

struct InnerMorseEtaOutput {
    double W{0.0};
    Vec2 dWdeta{};
    Voigt3 ddWdeta{};
    Vec6 dW{};
};

InnerMorseEtaOutput evaluate_inner_morse(const MatData& mat,
                                         const Vec6& pe,
                                         const std::array<Vec2, 6>& dpedeta,
                                         const std::array<Voigt3, 6>& ddpedeta) {
    validate_morse_material(mat);

    InnerMorseEtaOutput out;
    for (int i = 0; i < 3; ++i) {
        const Vec3 vs = vstretch_bis(pe[i], mat);
        out.W += vs[0];
        out.dW[i] = vs[1] / mat.s0;
        out.dWdeta = out.dWdeta + out.dW[i] * dpedeta[i];
        out.ddWdeta = out.ddWdeta +
                      (vs[2] / mat.s0) * Voigt3{
                                              dpedeta[i][0] * dpedeta[i][0],
                                              dpedeta[i][1] * dpedeta[i][1],
                                              dpedeta[i][0] * dpedeta[i][1],
                                          } +
                      out.dW[i] * ddpedeta[i];

        const Vec3 va = vangle_bis(pe[3 + i], mat);
        out.W += 2.0 * va[0];
        out.dW[3 + i] = 2.0 * va[1] / mat.s0;
        out.dWdeta = out.dWdeta + out.dW[3 + i] * dpedeta[3 + i];
        out.ddWdeta = out.ddWdeta +
                      (2.0 * va[2] / mat.s0) * Voigt3{
                                                  dpedeta[3 + i][0] * dpedeta[3 + i][0],
                                                  dpedeta[3 + i][1] * dpedeta[3 + i][1],
                                                  dpedeta[3 + i][0] * dpedeta[3 + i][1],
                                              } +
                      out.dW[3 + i] * ddpedeta[3 + i];
    }
    out.W /= mat.s0;
    return out;
}

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

InnerPotentialOutput evaluate_inner_potential_from_prepared_state(const ElementState& state,
                                                                  const MatData& mat) {
    if (!state.has_prepared_bond_state) {
        throw std::invalid_argument("ElementState must own a prepared bond stage before evaluation");
    }

    const auto& prepared = state.prepared_bonds;
    const auto& A_norm = prepared.A_norm;
    const auto& Ei = prepared.Ei;
    const BondState& bond_state = prepared.bonds;

    const Vec2 ttemp1{
        state.C_elem[0] * state.vppal[0][0] + state.C_elem[2] * state.vppal[0][1],
        state.C_elem[2] * state.vppal[0][0] + state.C_elem[1] * state.vppal[0][1],
    };
    const Vec2 ttemp2{
        state.C_elem[0] * state.vppal[1][0] + state.C_elem[2] * state.vppal[1][1],
        state.C_elem[2] * state.vppal[1][0] + state.C_elem[1] * state.vppal[1][1],
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
        const Vec2 ttemp = c_vec(state.C_elem, Ei[i]);
        const double temp3 = dot(state.vppal[0], ttemp);
        const double temp4 = dot(state.vppal[1], ttemp);
        const double p = A_norm[i] * temp3;
        const double q = A_norm[i] * temp4;

        const double f1 = sinxx(state.curvppal[0] * p);
        const double f2 = sinxx(state.curvppal[1] * q);
        const double f12 = sinxx(state.curvppal[0] * p / 2.0);
        const double f22 = sinxx(state.curvppal[1] * q / 2.0);

        a_def[i][0] = p * f1;
        a_def[i][1] = q * f2;
        a_def[i][2] = state.curvppal[0] * p * p / 2.0 * f12 * f12 +
                      state.curvppal[1] * q * q / 2.0 * f22 * f22;

        const double g1 = dsinxx(state.curvppal[0] * p);
        const double g2 = dsinxx(state.curvppal[1] * q);
        const double g12 = dsinxx(state.curvppal[0] * p / 2.0);
        const double g22 = dsinxx(state.curvppal[1] * q / 2.0);

        dadeta[i][0] = (f1 + state.curvppal[0] * p * g1) * dpdeta;
        dadeta[i][1] = (f2 + state.curvppal[1] * q * g2) * dqdeta;

        const double xx1 = f12 + state.curvppal[0] * p / 2.0 * g12;
        const double xx2 = f22 + state.curvppal[1] * q / 2.0 * g22;
        const double yy1 = state.curvppal[0] * p * f12;
        const double yy2 = state.curvppal[1] * q * f22;
        dadeta[i][2] = yy1 * xx1 * dpdeta + yy2 * xx2 * dqdeta;

        const double h1 = ddsinxx(state.curvppal[0] * p);
        const double h2 = ddsinxx(state.curvppal[1] * q);
        const double h12 = ddsinxx(state.curvppal[0] * p / 2.0);
        const double h22 = ddsinxx(state.curvppal[1] * q / 2.0);

        ddadeta[i][0] = state.curvppal[0] * (2.0 * g1 + state.curvppal[0] * p * h1) * dpdeta2;
        ddadeta[i][1] = state.curvppal[1] * (2.0 * g2 + state.curvppal[1] * q * h2) * dqdeta2;
        ddadeta[i][2] =
            state.curvppal[0] * (xx1 * xx1 + yy1 * (g12 + state.curvppal[0] * p / 4.0 * h12)) * dpdeta2 +
            state.curvppal[1] * (xx2 * xx2 + yy2 * (g22 + state.curvppal[1] * q / 4.0 * h22)) * dqdeta2;

        pe[i] = bond_state.pe[i];
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
        pe[3 + k] = bond_state.pe[3 + k];
        const double temp7 = std::cos(pe[3 + k]);
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

    InnerPotentialOutput out;
    if (mat.nCode_Pot == 1) {
        const InnerMorseEtaOutput inner = evaluate_inner_morse(mat, pe, dpedeta_all, ddpedeta);
        out.W = inner.W;
        out.dWdeta = inner.dWdeta;
        out.ddWdeta = inner.ddWdeta;
        out.dW_dpe = inner.dW;
    } else {
        const InnerBrennerEtaOutput inner = evaluate_inner_brenner(mat, pe, dpedeta_all, ddpedeta);
        out.W = inner.W;
        out.dWdeta = inner.dWdeta;
        out.ddWdeta = inner.ddWdeta;
        out.dW_dpe = inner.dW;
    }
    return out;
}

template <typename EvaluateFn>
NewtonInnerOutput solve_inner_newton_impl(const MatData& mat,
                                          const Vec2& eta0,
                                          const double crit,
                                          const int max_iter,
                                          EvaluateFn&& evaluate) {
    if (mat.nCode_Pot != 1 && mat.nCode_Pot != 2) {
        throw std::invalid_argument("Only Morse nCode_Pot=1 and Brenner nCode_Pot=2 are supported");
    }

    NewtonInnerOutput out;
    out.eta = eta0;
    out.iterations = 0;
    out.fail_mode = 0;

    InnerPotentialOutput current;
    try {
        current = evaluate(out.eta);
    } catch (const std::invalid_argument& ex) {
        if (std::string(ex.what()).find("singular bond angle derivative") == std::string::npos) {
            throw;
        }
        out.fail_mode = 1;
        out.W = 0.0;
        out.dWdeta = Vec2{0.0, 0.0};
        out.ddWdeta = Voigt3{0.0, 0.0, 0.0};
        out.dW_dpe = Vec6{};
        return out;
    }
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

        const Vec2 eta_prev = out.eta;
        out.eta = out.eta + dx;
        if (norm(out.eta) > 0.5 * mat.A0) {
            out.fail_mode = 2;
            try {
                current = evaluate(out.eta);
            } catch (const std::invalid_argument& ex) {
                if (std::string(ex.what()).find("singular bond angle derivative") == std::string::npos) {
                    throw;
                }
                out.fail_mode = 1;
                out.eta = eta_prev;
            }
            break;
        }

        try {
            current = evaluate(out.eta);
        } catch (const std::invalid_argument& ex) {
            if (std::string(ex.what()).find("singular bond angle derivative") == std::string::npos) {
                throw;
            }
            out.fail_mode = 1;
            out.eta = eta_prev;
            break;
        }
        test = crit * (1.0 + std::abs(current.W));
        gnorm = norm(current.dWdeta);
    }

    if (gnorm > test && out.fail_mode == 0) {
        out.fail_mode = 3;
    }

    if (out.fail_mode == 0) {
        current = evaluate(out.eta);
    }
    out.W = current.W;
    out.dWdeta = current.dWdeta;
    out.ddWdeta = current.ddWdeta;
    out.dW_dpe = current.dW_dpe;
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
    if (std::any_of(pe.begin(), pe.begin() + 3, [](double a) {
            return a >= kBrennerCutoffRadius;
        })) {
        return out;
    }

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
    if (mat.nCode_Pot != 1 && mat.nCode_Pot != 2) {
        throw std::invalid_argument("Only Morse nCode_Pot=1 and Brenner nCode_Pot=2 are supported");
    }
    const ElementState prepared_state =
        prepare_element_state(make_element_state_view(C_elem, curvppal, vppal), mat, eta);
    return evaluate_inner_potential_from_prepared_state(prepared_state, mat);
}

InnerPotentialOutput evaluate_inner_potential(const ElementState& state,
                                              const MatData& mat,
                                              const Vec2& eta) {
    const ElementState prepared_state = ensure_prepared_element_state(state, mat, eta);
    return evaluate_inner_potential_from_prepared_state(prepared_state, mat);
}

NewtonInnerOutput solve_inner_newton(const Voigt3& C_elem,
                                     const Vec2& curvppal,
                                     const Mat22& vppal,
                                     const MatData& mat,
                                     const Vec2& eta0,
                                     const double crit,
                                     const int max_iter) {
    return solve_inner_newton_impl(
        mat,
        eta0,
        crit,
        max_iter,
        [&](const Vec2& eta) { return evaluate_inner_potential(C_elem, curvppal, vppal, mat, eta); });
}

NewtonInnerOutput solve_inner_newton(const ElementState& state,
                                     const MatData& mat,
                                     const Vec2& eta0,
                                     const double crit,
                                     const int max_iter) {
    ElementState prepared_state = state;
    return solve_inner_newton_impl(
        mat,
        eta0,
        crit,
        max_iter,
        [&](const Vec2& eta) {
            prepared_state = ensure_prepared_element_state(prepared_state, mat, eta);
            return evaluate_inner_potential_from_prepared_state(prepared_state, mat);
        });
}

OuterPotentialOutput evaluate_outer_potential(const MatData& mat, const Vec6& pe) {
    if (mat.nCode_Pot == 1) {
        validate_morse_material(mat);
        OuterPotentialOutput out;
        for (int i = 0; i < 3; ++i) {
            const Vec3 vs = vstretch_bis(pe[i], mat);
            out.W += vs[0];
            out.dW[i] = vs[1];
            const Vec3 va = vangle_bis(pe[3 + i], mat);
            out.W += 2.0 * va[0];
            out.dW[3 + i] = 2.0 * va[1];
        }
        out.W /= mat.s0;
        for (double& d : out.dW) {
            d /= mat.s0;
        }
        return out;
    }
    if (mat.nCode_Pot == 2) {
        const BrennerOutput brenner = evaluate_brenner(mat, pe);
        OuterPotentialOutput out;
        out.W = brenner.W;
        out.dW = brenner.dW;
        return out;
    }
    throw std::invalid_argument("Only Morse nCode_Pot=1 and Brenner nCode_Pot=2 are supported for outer potential");
}

}  // namespace fce
