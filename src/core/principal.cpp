#include "fce/principal.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace fce {
namespace {

Voigt3 operator*(double s, const Voigt3& a) {
    return Voigt3{s * a[0], s * a[1], s * a[2]};
}

Voigt3 operator+(const Voigt3& a, const Voigt3& b) {
    return Voigt3{a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

Voigt3 operator-(const Voigt3& a, const Voigt3& b) {
    return Voigt3{a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

double metric_norm(const Voigt3& C, const Vec2& v) {
    return std::sqrt(C[0] * v[0] * v[0] + 2.0 * C[2] * v[0] * v[1] + C[1] * v[1] * v[1]);
}

}  // namespace

PrincipalResult compute_principal_curvature(const Voigt3& C_elem, const Voigt3& curv0_elem) {
    PrincipalResult out;

    const double detC = C_elem[0] * C_elem[1] - C_elem[2] * C_elem[2];
    if (detC <= 0.0 || std::isnan(detC)) {
        throw std::invalid_argument("principal requires positive-definite metric tensor");
    }
    const double detk0 = curv0_elem[0] * curv0_elem[1] - curv0_elem[2] * curv0_elem[2];
    const double alpha = C_elem[0] * curv0_elem[1] + C_elem[1] * curv0_elem[0] - 2.0 * C_elem[2] * curv0_elem[2];
    const double xmean = alpha / (2.0 * detC);
    const double gauss = detk0 / detC;
    const double beta_sq = xmean * xmean - gauss;
    const double beta = std::sqrt(std::max(0.0, beta_sq));

    out.curvppal = Vec2{xmean + beta, xmean - beta};

    if (std::abs(beta) < 1e-6) {
        out.flag_num_diff = true;
        out.vppal[0] = Vec2{C_elem[0], C_elem[2]};
        out.vppal[1] = Vec2{-C_elem[2] * (C_elem[0] + C_elem[1]), C_elem[0] * C_elem[0] + C_elem[2] * C_elem[2]};
        for (auto& v : out.vppal) {
            const double fkk = metric_norm(C_elem, v);
            v[0] /= fkk;
            v[1] /= fkk;
        }
        return out;
    }

    const std::array<int, 2> iperm{1, 0};
    std::array<Vec2, 2> C1vppal{};
    std::array<Vec2, 2> C2vppal{};

    C1vppal[0] = Vec2{-curv0_elem[2] + out.curvppal[0] * C_elem[2], curv0_elem[0] - out.curvppal[0] * C_elem[0]};
    C2vppal[0] = Vec2{-curv0_elem[1] + out.curvppal[0] * C_elem[1], curv0_elem[2] - out.curvppal[0] * C_elem[2]};
    const double test1 = std::max(std::abs(C1vppal[0][0]), std::abs(C1vppal[0][1]));
    const double test2 = std::max(std::abs(C2vppal[0][0]), std::abs(C2vppal[0][1]));
    out.vppal[0] = test1 > test2 ? C1vppal[0] : C2vppal[0];

    C1vppal[1] = Vec2{-curv0_elem[2] + out.curvppal[1] * C_elem[2], curv0_elem[0] - out.curvppal[1] * C_elem[0]};
    C2vppal[1] = Vec2{-curv0_elem[1] + out.curvppal[1] * C_elem[1], curv0_elem[2] - out.curvppal[1] * C_elem[2]};
    const double test3 = std::max(std::abs(C1vppal[1][0]), std::abs(C1vppal[1][1]));
    const double test4 = std::max(std::abs(C2vppal[1][0]), std::abs(C2vppal[1][1]));
    out.vppal[1] = test3 > test4 ? C1vppal[1] : C2vppal[1];

    if (std::max(test1, test2) > std::max(test3, test4)) {
        out.vppal[1] = Vec2{
            -C_elem[2] * out.vppal[0][0] - C_elem[1] * out.vppal[0][1],
            C_elem[0] * out.vppal[0][0] + C_elem[2] * out.vppal[0][1],
        };
    } else {
        out.vppal[0] = Vec2{
            -C_elem[2] * out.vppal[1][0] - C_elem[1] * out.vppal[1][1],
            C_elem[0] * out.vppal[1][0] + C_elem[2] * out.vppal[1][1],
        };
    }

    for (auto& v : out.vppal) {
        const double fkk = metric_norm(C_elem, v);
        v[0] /= fkk;
        v[1] /= fkk;
    }

    const Voigt3 temp1{
        out.vppal[0][0] * out.vppal[1][0],
        out.vppal[0][1] * out.vppal[1][1],
        out.vppal[0][0] * out.vppal[1][1] + out.vppal[1][0] * out.vppal[0][1],
    };

    for (int ipp = 0; ipp < 2; ++ipp) {
        const Voigt3 temp2{
            out.vppal[ipp][0] * out.vppal[ipp][0],
            out.vppal[ipp][1] * out.vppal[ipp][1],
            2.0 * out.vppal[ipp][0] * out.vppal[ipp][1],
        };
        out.dcurvppaldk[ipp] = temp2;
        out.dcurvppaldC[ipp] = (-out.curvppal[ipp]) * temp2;

        const int jpp = iperm[ipp];
        out.dvppaldk[ipp][0] =
            (out.vppal[jpp][0] / (out.curvppal[ipp] - out.curvppal[jpp])) * temp1;
        out.dvppaldk[ipp][1] =
            (out.vppal[jpp][1] / (out.curvppal[ipp] - out.curvppal[jpp])) * temp1;

        out.dvppaldC[ipp][0] =
            (-out.curvppal[ipp]) * out.dvppaldk[ipp][0] - 0.5 * out.vppal[ipp][0] * temp2;
        out.dvppaldC[ipp][1] =
            (-out.curvppal[ipp]) * out.dvppaldk[ipp][1] - 0.5 * out.vppal[ipp][1] * temp2;
    }

    return out;
}

}  // namespace fce
