#include "fce/element_state.hpp"

#include <cmath>
#include <stdexcept>

namespace fce {
namespace {

Voigt3 subtract_voigt(const Voigt3& a, const Voigt3& b) {
    return Voigt3{a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

std::array<Vec2, 3> compute_normalized_bonds(const MatData& mat,
                                             const Vec2& eta,
                                             std::array<double, 3>* a_norm) {
    std::array<Vec2, 3> Ei{};
    for (int ibond = 0; ibond < 3; ++ibond) {
        Ei[ibond] = Vec2{
            mat.A0 * mat.E[ibond][0] + eta[0],
            mat.A0 * mat.E[ibond][1] + eta[1],
        };
        const double norm =
            std::sqrt(Ei[ibond][0] * Ei[ibond][0] + Ei[ibond][1] * Ei[ibond][1]);
        if (norm <= 0.0) {
            throw std::invalid_argument("bond preparation encountered zero-norm bond vector");
        }
        (*a_norm)[ibond] = norm;
        Ei[ibond][0] /= norm;
        Ei[ibond][1] /= norm;
    }
    return Ei;
}

}  // namespace

ElementState compute_element_state(const NeighborCoords12& xneigh,
                                   const ShapeGradient12& dn,
                                   const ShapeCurvature12& ddn,
                                   const Mat22& f0,
                                   const Voigt3& reference_curvature) {
    ElementState out;
    out.metric = compute_metric(xneigh, dn, f0);
    out.curvature = compute_curvature(xneigh, ddn, f0, out.metric.xnor_elem, out.metric.dnorm);
    out.C_elem = out.metric.C_elem;
    out.curv0_elem = subtract_voigt(out.curvature.curv0_elem, reference_curvature);

    const PrincipalResult principal = compute_principal_curvature(out.C_elem, out.curv0_elem);
    out.curvppal = principal.curvppal;
    out.vppal = principal.vppal;
    out.dcurvppaldC = principal.dcurvppaldC;
    out.dcurvppaldk = principal.dcurvppaldk;
    out.dvppaldC = principal.dvppaldC;
    out.dvppaldk = principal.dvppaldk;
    out.flag_num_diff = principal.flag_num_diff;
    return out;
}

PreparedBondState prepare_bond_state(const ElementState& state,
                                     const MatData& mat,
                                     const Vec2& eta) {
    const ElementState prepared_state = prepare_element_state(state, mat, eta);
    PreparedBondState out;
    out.A_norm = prepared_state.prepared_bonds.A_norm;
    out.Ei = prepared_state.prepared_bonds.Ei;
    out.bonds = prepared_state.prepared_bonds.bonds;
    return out;
}

PreparedBondStateWithDerivatives prepare_bond_state_with_derivatives(const ElementState& state,
                                                                     const MatData& mat,
                                                                     const Vec2& eta) {
    const ElementState prepared_state = prepare_element_state(state, mat, eta);
    PreparedBondStateWithDerivatives out;
    out.A_norm = prepared_state.prepared_bonds.A_norm;
    out.Ei = prepared_state.prepared_bonds.Ei;
    out.bonds = prepared_state.prepared_bonds.bonds_with_derivatives;
    return out;
}

ElementState prepare_element_state(const ElementState& state,
                                   const MatData& mat,
                                   const Vec2& eta) {
    ElementState out = state;
    out.prepared_eta = eta;
    out.prepared_material_A0 = mat.A0;
    out.prepared_material_E = mat.E;
    out.prepared_bonds.Ei = compute_normalized_bonds(mat, eta, &out.prepared_bonds.A_norm);
    out.prepared_bonds.bonds_with_derivatives = compute_deformed_bonds_with_derivatives(
        state.C_elem,
        state.curvppal,
        state.vppal,
        state.dcurvppaldC,
        state.dcurvppaldk,
        state.dvppaldC,
        state.dvppaldk,
        out.prepared_bonds.A_norm,
        out.prepared_bonds.Ei);
    out.prepared_bonds.bonds.pe = out.prepared_bonds.bonds_with_derivatives.pe;
    out.has_prepared_bond_state = true;
    return out;
}

RelaxedElementState solve_inner_newton_for_element(const NeighborCoords12& xneigh,
                                                   const ShapeGradient12& dn,
                                                   const ShapeCurvature12& ddn,
                                                   const Mat22& f0,
                                                   const Voigt3& reference_curvature,
                                                   const MatData& mat,
                                                   const Vec2& eta0,
                                                   const double crit,
                                                   const int max_iter) {
    RelaxedElementState out;
    out.state = compute_element_state(xneigh, dn, ddn, f0, reference_curvature);
    out.inner = solve_inner_newton(out.state, mat, eta0, crit, max_iter);
    return out;
}

}  // namespace fce
