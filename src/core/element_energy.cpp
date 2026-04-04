#include "fce/element_energy.hpp"

#include "fce/constitutive.hpp"
#include "fce/element_state.hpp"
#include "fce/exponential.hpp"
#include "fce/principal.hpp"

#include <cmath>
#include <stdexcept>

namespace fce {

ElementEnergyResult compute_element_energy(const NeighborCoords12& xneigh,
                                           const Mat22& f0,
                                           const Voigt3& reference_curvature,
                                           const GaussData& gauss,
                                           const MatData& mat,
                                           const bool nW_hat,
                                           const double crit,
                                           const int max_iter,
                                           const std::vector<Vec2>& eta0) {
    const int ngauss = gauss.ngauss;
    if (static_cast<int>(eta0.size()) != ngauss) {
        throw std::invalid_argument("eta0 size must equal ngauss");
    }

    ElementEnergyResult result;
    result.eta = eta0;  // copy initial etas; updated per Gauss point on Newton convergence

    for (int igauss = 0; igauss < ngauss; ++igauss) {
        // Extract per-Gauss-point shape gradients and curvatures from shapef
        const auto& sf = gauss.shapef.at(static_cast<std::size_t>(igauss));
        ShapeGradient12 dn{};
        ShapeCurvature12 ddn{};
        for (int inode = 0; inode < 12; ++inode) {
            dn[inode]  = Vec2{sf[inode][1], sf[inode][2]};
            ddn[inode] = Voigt3{sf[inode][3], sf[inode][4], sf[inode][5]};
        }

        // Geometry: metric → curvature → principal curvatures and eigenvectors
        const ElementState state = compute_element_state(xneigh, dn, ddn, f0, reference_curvature);

        // Inner Newton relaxation (if nW_hat)
        Vec2 eta_gauss = result.eta[igauss];
        double W = 0.0;
        Vec6 dW{};

        if (nW_hat) {
            const NewtonInnerOutput inner = solve_inner_newton(state, mat, eta_gauss, crit, max_iter);
            const bool converged = (inner.fail_mode == 0 && inner.iterations < max_iter);
            if (!converged) {
                result.inner_fail++;
            } else {
                result.eta[igauss] = inner.eta;
                eta_gauss = inner.eta;
            }
            // W and dW_dpe are always valid (solve_inner_newton does a final evaluation)
            W  = inner.W;
            dW = inner.dW_dpe;
        }

        // Bond vectors from current (possibly updated) eta
        std::array<double, 3> A_norm{};
        std::array<Vec2, 3> Ei{};
        for (int ibond = 0; ibond < 3; ++ibond) {
            Ei[ibond] = Vec2{
                mat.A0 * mat.E[ibond][0] + eta_gauss[0],
                mat.A0 * mat.E[ibond][1] + eta_gauss[1],
            };
            const double n = std::sqrt(Ei[ibond][0] * Ei[ibond][0] + Ei[ibond][1] * Ei[ibond][1]);
            A_norm[ibond] = n;
            Ei[ibond][0] /= n;
            Ei[ibond][1] /= n;
        }

        Voigt3 S_n{};
        Voigt3 S_m{};

        if (state.flag_num_diff) {
            // Stresses by numerical differentiation (k1 == k2 degenerate case)
            constexpr double h = 1e-8;
            const BondState bonds_base =
                compute_deformed_bonds(state.C_elem, state.curvppal, state.vppal, A_norm, Ei);
            const OuterPotentialOutput outer_base = evaluate_outer_potential(mat, bonds_base.pe);
            W = outer_base.W;

            for (int i = 0; i < 3; ++i) {
                // S_n: finite difference in C_elem direction i
                Voigt3 C_p = state.C_elem;
                C_p[i] += h;
                const PrincipalResult pp_C = compute_principal_curvature(C_p, state.curv0_elem);
                const BondState bonds_C =
                    compute_deformed_bonds(C_p, pp_C.curvppal, pp_C.vppal, A_norm, Ei);
                S_n[i] = (evaluate_outer_potential(mat, bonds_C.pe).W - W) / h;

                // S_m: finite difference in curv0_elem direction i
                Voigt3 k_p = state.curv0_elem;
                k_p[i] += h;
                const PrincipalResult pp_k = compute_principal_curvature(state.C_elem, k_p);
                const BondState bonds_k =
                    compute_deformed_bonds(state.C_elem, pp_k.curvppal, pp_k.vppal, A_norm, Ei);
                S_m[i] = (evaluate_outer_potential(mat, bonds_k.pe).W - W) / h;
            }
        } else {
            // Stresses analytically (Stresses subroutine: S_n = sum dW[i]*dpedC[i])
            const BondStateWithDerivatives bonds = compute_deformed_bonds_with_derivatives(
                state.C_elem,
                state.curvppal,
                state.vppal,
                state.dcurvppaldC,
                state.dcurvppaldk,
                state.dvppaldC,
                state.dvppaldk,
                A_norm,
                Ei);

            if (!nW_hat) {
                // No inner relaxation: compute W and dW from outer potential
                const OuterPotentialOutput outer = evaluate_outer_potential(mat, bonds.pe);
                W  = outer.W;
                dW = outer.dW;
            }
            // If nW_hat: W and dW already set from solve_inner_newton output above

            for (int i = 0; i < 6; ++i) {
                for (int j = 0; j < 3; ++j) {
                    S_n[j] += dW[i] * bonds.dpedC[i][j];
                    S_m[j] += dW[i] * bonds.dpedk[i][j];
                }
            }
        }

        // Force and energy accumulation
        const double weight = gauss.weight.at(static_cast<std::size_t>(igauss));
        for (int ij = 0; ij < 3; ++ij) {
            for (int inode = 0; inode < 12; ++inode) {
                for (int k = 0; k < 3; ++k) {
                    result.f_elem[inode][k] +=
                        (S_n[ij] * state.metric.dC[inode][k][ij] +
                         S_m[ij] * state.curvature.dcurv[inode][k][ij]) *
                        weight;
                }
            }
        }
        result.W_elem += W * weight;
    }

    return result;
}

}  // namespace fce
