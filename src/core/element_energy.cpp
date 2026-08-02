#include "fce/element_energy.hpp"

#if defined(__clang__)
#pragma clang fp contract(off)
#pragma clang fp reassociate(off)
#endif

#include "fce/constitutive.hpp"
#include "fce/element_state.hpp"
#include "fce/exponential.hpp"
#include "fce/principal.hpp"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <string>
#include <stdexcept>

namespace fce {

ElementEnergyResult compute_element_energy(const NeighborCoords12& xneigh,
                                           const Mat22& f0,
                                           const std::vector<Voigt3>& reference_curvature,
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
    if (static_cast<int>(reference_curvature.size()) != ngauss) {
        throw std::invalid_argument("reference_curvature size must equal ngauss");
    }

    ElementEnergyResult result;
    static int debug_element_call = 0;
    const int debug_element_index = debug_element_call++;
    result.eta = eta0;  // copy initial etas; updated per Gauss point on Newton convergence
    // The reference ener_elem.f90 accumulates W_elem in REAL(8).  Keeping
    // this as double is intentional: the optimizer's line search observes
    // the exact element-energy rounding, so a wider accumulator changes the
    // accepted trajectory even though the result is algebraically identical.
    double energy_accum = 0.0;
    const char* extended_force_env = std::getenv("FCE_LONG_DOUBLE_FORCE_ACCUM");
    const bool extended_force_accumulation =
        extended_force_env != nullptr && *extended_force_env != '\0' &&
        std::string(extended_force_env) != "0" &&
        std::string(extended_force_env) != "false" &&
        std::string(extended_force_env) != "no" &&
        std::string(extended_force_env) != "off";
    std::array<std::array<long double, 3>, 12> force_accum_extended{};

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
        const ElementState state =
            compute_element_state(xneigh,
                                  dn,
                                  ddn,
                                  f0,
                                  reference_curvature.at(static_cast<std::size_t>(igauss)));

        // Inner Newton relaxation (if nW_hat)
        Vec2 eta_gauss = result.eta[igauss];
        double W = 0.0;
        Vec6 dW{};
        Vec6 debug_pe{};
        std::array<Voigt3, 6> debug_dpedC{};
        std::array<Voigt3, 6> debug_dpedk{};

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
        std::array<double, 3> debug_fd_W_n{};
        std::array<double, 3> debug_fd_W_m{};

        if (state.flag_num_diff) {
            // Stresses by numerical differentiation (k1 == k2 degenerate case)
            constexpr double h = 1e-8;
            const BondState bonds_base =
                compute_deformed_bonds(state.C_elem, state.curvppal, state.vppal, A_norm, Ei);
            const OuterPotentialOutput outer_base = evaluate_outer_potential(mat, bonds_base.pe);
            W = outer_base.W;
            debug_pe = bonds_base.pe;
            dW = outer_base.dW;

            for (int i = 0; i < 3; ++i) {
                // S_n: finite difference in C_elem direction i
                Voigt3 C_p = state.C_elem;
                C_p[i] += h;
                const PrincipalResult pp_C =
                    compute_principal_curvature(C_p, state.curv0_elem, state.flag_num_diff);
                const BondState bonds_C =
                    compute_deformed_bonds(C_p, pp_C.curvppal, pp_C.vppal, A_norm, Ei);
                debug_fd_W_n[i] = evaluate_outer_potential(mat, bonds_C.pe).W;
                S_n[i] = (debug_fd_W_n[i] - W) / h;

                // S_m: finite difference in C_elem direction i
                // Matches canonical ener_elem.f90 lines 76-84: the bending-stress loop
                // is identical to the membrane-stress loop (both perturb C_elem).
                Voigt3 C_pm = state.C_elem;
                C_pm[i] += h;
                const PrincipalResult pp_m =
                    compute_principal_curvature(C_pm, state.curv0_elem, state.flag_num_diff);
                const BondState bonds_m =
                    compute_deformed_bonds(C_pm, pp_m.curvppal, pp_m.vppal, A_norm, Ei);
                debug_fd_W_m[i] = evaluate_outer_potential(mat, bonds_m.pe).W;
                S_m[i] = (debug_fd_W_m[i] - W) / h;
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

            // The canonical ener_elem.f90 path always recomputes Hyper_Pot on the
            // final relaxed bond state before assembling analytical stresses.
            const OuterPotentialOutput outer = evaluate_outer_potential(mat, bonds.pe);
            W  = outer.W;
            dW = outer.dW;
            debug_pe = bonds.pe;
            debug_dpedC = bonds.dpedC;
            debug_dpedk = bonds.dpedk;

            for (int i = 0; i < 6; ++i) {
                for (int j = 0; j < 3; ++j) {
                    S_n[j] += dW[i] * bonds.dpedC[i][j];
                    S_m[j] += dW[i] * bonds.dpedk[i][j];
                }
            }
        }

        const char* debug_element_path = std::getenv("FCE_TRACE_ELEMENT_STATE");
        const char* debug_element_number = std::getenv("FCE_TRACE_ELEMENT_INDEX");
        if (debug_element_path != nullptr && *debug_element_path != '\0' &&
            debug_element_number != nullptr &&
            debug_element_index == std::stoi(debug_element_number)) {
            std::ofstream debug(debug_element_path, std::ios::out | std::ios::app);
            debug << std::uppercase << std::scientific << std::setprecision(17)
                  << "gauss " << igauss << " flag " << state.flag_num_diff
                  << " W " << W
                  << " C " << state.C_elem[0] << " " << state.C_elem[1] << " " << state.C_elem[2]
                  << " K " << state.curv0_elem[0] << " " << state.curv0_elem[1] << " " << state.curv0_elem[2]
                  << " KP " << state.curvppal[0] << " " << state.curvppal[1]
                  << " V " << state.vppal[0][0] << " " << state.vppal[0][1]
                  << " " << state.vppal[1][0] << " " << state.vppal[1][1]
                  << " SN " << S_n[0] << " " << S_n[1] << " " << S_n[2]
                  << " SM " << S_m[0] << " " << S_m[1] << " " << S_m[2] << '\n';
            debug << "pe ";
            for (const double value : debug_pe) debug << value << " ";
            debug << "A_norm ";
            for (const double value : A_norm) debug << value << " ";
            debug << "Ei ";
            for (const auto& value : Ei) debug << value[0] << " " << value[1] << " ";
            debug << "dW ";
            for (const double value : dW) debug << value << " ";
            debug << "fdW_n ";
            for (const double value : debug_fd_W_n) debug << value << " ";
            debug << "fdW_m ";
            for (const double value : debug_fd_W_m) debug << value << " ";
            debug << "dpedC ";
            for (const auto& value : debug_dpedC) {
                debug << value[0] << " " << value[1] << " " << value[2] << " ";
            }
            debug << "dpedk ";
            for (const auto& value : debug_dpedk) {
                debug << value[0] << " " << value[1] << " " << value[2] << " ";
            }
            debug << '\n';
            for (int inode = 0; inode < 12; ++inode) {
                for (int axis = 0; axis < 3; ++axis) {
                    debug << "dC " << inode + 1 << " " << axis + 1 << " "
                          << state.metric.dC[inode][axis][0] << " "
                          << state.metric.dC[inode][axis][1] << " "
                          << state.metric.dC[inode][axis][2] << '\n';
                    debug << "dK " << inode + 1 << " " << axis + 1 << " "
                          << state.curvature.dcurv[inode][axis][0] << " "
                          << state.curvature.dcurv[inode][axis][1] << " "
                          << state.curvature.dcurv[inode][axis][2] << '\n';
                }
            }
        }

        // Force and energy accumulation
        const double weight = gauss.weight.at(static_cast<std::size_t>(igauss));
        for (int ij = 0; ij < 3; ++ij) {
            for (int inode = 0; inode < 12; ++inode) {
                for (int k = 0; k < 3; ++k) {
                    const double contribution =
                        (S_n[ij] * state.metric.dC[inode][k][ij] +
                         S_m[ij] * state.curvature.dcurv[inode][k][ij]) *
                        weight;
                    if (extended_force_accumulation) {
                        force_accum_extended[inode][k] +=
                            static_cast<long double>(contribution);
                    } else {
                        result.f_elem[inode][k] += contribution;
                    }
                }
            }
        }
        if (extended_force_accumulation) {
            for (int inode = 0; inode < 12; ++inode) {
                for (int k = 0; k < 3; ++k) {
                    result.f_elem[inode][k] =
                        static_cast<double>(force_accum_extended[inode][k]);
                }
            }
        }
        energy_accum += W * weight;
    }

    result.W_elem = energy_accum;

    const char* element_energy_trace = std::getenv("FCE_TRACE_ELEMENT_W");
    if (element_energy_trace != nullptr && *element_energy_trace != '\0') {
        std::ofstream trace(element_energy_trace, std::ios::out | std::ios::app);
        if (!trace) {
            throw std::runtime_error("cannot open element-energy trace: " +
                                     std::string(element_energy_trace));
        }
        trace << std::setprecision(17) << debug_element_index << " " << result.W_elem << '\n';
    }
    const char* element_force_trace = std::getenv("FCE_TRACE_ELEMENT_FORCE_ALL");
    if (element_force_trace != nullptr && *element_force_trace != '\0') {
        std::ofstream trace(element_force_trace, std::ios::out | std::ios::app);
        if (!trace) {
            throw std::runtime_error("cannot open element-force trace: " +
                                     std::string(element_force_trace));
        }
        trace << std::setprecision(17) << debug_element_index;
        for (int inode = 0; inode < 12; ++inode) {
            for (int axis = 0; axis < 3; ++axis) {
                trace << " " << result.f_elem[inode][axis];
            }
        }
        trace << " " << result.W_elem << '\n';
    }

    return result;
}

ElementEnergyResult compute_element_energy(const NeighborCoords12& xneigh,
                                           const Mat22& f0,
                                           const Voigt3& reference_curvature,
                                           const GaussData& gauss,
                                           const MatData& mat,
                                           const bool nW_hat,
                                           const double crit,
                                           const int max_iter,
                                           const std::vector<Vec2>& eta0) {
    return compute_element_energy(xneigh,
                                  f0,
                                  std::vector<Voigt3>(static_cast<std::size_t>(gauss.ngauss),
                                                      reference_curvature),
                                  gauss,
                                  mat,
                                  nW_hat,
                                  crit,
                                  max_iter,
                                  eta0);
}

}  // namespace fce
