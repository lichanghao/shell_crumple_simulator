#include "fce/element_state.hpp"

namespace fce {
namespace {

Voigt3 subtract_voigt(const Voigt3& a, const Voigt3& b) {
    return Voigt3{a[0] - b[0], a[1] - b[1], a[2] - b[2]};
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
