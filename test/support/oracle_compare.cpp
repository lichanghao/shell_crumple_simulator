#include "oracle_compare.hpp"

#include "fce/io.hpp"

#include <cmath>
#include <filesystem>
#include <sstream>
#include <string>

namespace fce::test_support {
namespace {

class MismatchCollector {
public:
    void add(const std::string& message)
    {
        ++count_;
        if (shown_ < kMaxShown) {
            stream_ << "- " << message << "\n";
            ++shown_;
        }
    }

    bool empty() const { return count_ == 0; }

    std::string str() const
    {
        std::ostringstream out;
        out << count_ << " mismatch(es)\n";
        out << stream_.str();
        if (count_ > kMaxShown) {
            out << "- ... " << (count_ - kMaxShown) << " more mismatch(es)\n";
        }
        return out.str();
    }

private:
    static constexpr int kMaxShown = 25;
    int count_{0};
    int shown_{0};
    std::ostringstream stream_;
};

std::string join(const std::string& dir, const std::string& file)
{
    return (std::filesystem::path(dir) / file).string();
}

bool near_abs(double lhs, double rhs, double tol)
{
    return std::abs(lhs - rhs) <= tol;
}

void expect_equal_int(MismatchCollector& mismatches,
                      const std::string& label,
                      int lhs,
                      int rhs)
{
    if (lhs != rhs) {
        mismatches.add(label + ": expected " + std::to_string(rhs) +
                       ", got " + std::to_string(lhs));
    }
}

void expect_close(MismatchCollector& mismatches,
                  const std::string& label,
                  double lhs,
                  double rhs,
                  double tol)
{
    if (!near_abs(lhs, rhs, tol)) {
        std::ostringstream msg;
        msg << label << ": expected " << rhs << ", got " << lhs
            << " (|diff|=" << std::abs(lhs - rhs) << ", tol=" << tol << ")";
        mismatches.add(msg.str());
    }
}

} // namespace

::testing::AssertionResult compare_preprocessor_outputs(const std::string& actual_dir,
                                                        const std::string& oracle_dir,
                                                        double float_abs_tol)
{
    MismatchCollector mismatches;

    const auto actual_dims = io::read_dims(join(actual_dir, "nano_dims.dat"));
    const auto oracle_dims = io::read_dims(join(oracle_dir, "nano_dims.dat"));
    expect_equal_int(mismatches, "dims.numele", actual_dims.numele, oracle_dims.numele);
    expect_equal_int(mismatches, "dims.numnods", actual_dims.numnods, oracle_dims.numnods);
    expect_equal_int(mismatches, "dims.nedge", actual_dims.nedge, oracle_dims.nedge);
    expect_equal_int(mismatches, "dims.nelem_ghost", actual_dims.nelem_ghost, oracle_dims.nelem_ghost);
    expect_equal_int(mismatches, "dims.nnode_ghost", actual_dims.nnode_ghost, oracle_dims.nnode_ghost);
    expect_equal_int(mismatches, "dims.ngauss", actual_dims.ngauss, oracle_dims.ngauss);
    expect_equal_int(mismatches, "dims.nnodBC", actual_dims.nnodBC, oracle_dims.nnodBC);
    expect_equal_int(mismatches, "dims.ndofBC", actual_dims.ndofBC, oracle_dims.ndofBC);
    expect_equal_int(mismatches, "dims.ndofOP", actual_dims.ndofOP, oracle_dims.ndofOP);
    expect_equal_int(mismatches, "dims.nvdw", actual_dims.nvdw, oracle_dims.nvdw);

    const auto actual_general = io::read_general(join(actual_dir, "nano_general.dat"));
    const auto oracle_general = io::read_general(join(oracle_dir, "nano_general.dat"));
    expect_close(mismatches, "general.ylength", actual_general.ylength, oracle_general.ylength, float_abs_tol);
    expect_close(mismatches, "general.A0", actual_general.mat.A0, oracle_general.mat.A0, float_abs_tol);
    expect_equal_int(mismatches, "general.nCode_Pot", actual_general.mat.nCode_Pot, oracle_general.mat.nCode_Pot);
    expect_close(mismatches, "general.s0", actual_general.mat.s0, oracle_general.mat.s0, float_abs_tol);
    expect_equal_int(mismatches, "general.nW_hat", actual_general.nW_hat ? 1 : 0, oracle_general.nW_hat ? 1 : 0);
    expect_close(mismatches, "general.crit_global", actual_general.crit_global, oracle_general.crit_global, float_abs_tol);
    expect_close(mismatches, "general.crit_local", actual_general.crit_local, oracle_general.crit_local, float_abs_tol);
    expect_equal_int(mismatches, "general.imperfect", actual_general.imperfect ? 1 : 0, oracle_general.imperfect ? 1 : 0);
    expect_close(mismatches, "general.fact_imp", actual_general.fact_imp, oracle_general.fact_imp, float_abs_tol);
    for (int i = 0; i < 3; ++i) {
        expect_close(mismatches, "general.E[" + std::to_string(i) + "][0]",
                     actual_general.mat.E[i][0], oracle_general.mat.E[i][0], float_abs_tol);
        expect_close(mismatches, "general.E[" + std::to_string(i) + "][1]",
                     actual_general.mat.E[i][1], oracle_general.mat.E[i][1], float_abs_tol);
    }

    const auto actual_zero = io::read_zero(join(actual_dir, "nano_zero.dat"), oracle_dims.numele);
    const auto oracle_zero = io::read_zero(join(oracle_dir, "nano_zero.dat"), oracle_dims.numele);
    expect_equal_int(mismatches, "zero.size", static_cast<int>(actual_zero.size()), static_cast<int>(oracle_zero.size()));
    for (int i = 0; i < static_cast<int>(oracle_zero.size()) && i < static_cast<int>(actual_zero.size()); ++i) {
        expect_close(mismatches, "zero[" + std::to_string(i) + "].J0", actual_zero[i].J0, oracle_zero[i].J0, float_abs_tol);
        for (int row = 0; row < 2; ++row) {
            for (int col = 0; col < 2; ++col) {
                expect_close(mismatches,
                             "zero[" + std::to_string(i) + "].F0[" + std::to_string(row) + "][" + std::to_string(col) + "]",
                             actual_zero[i].F0[row][col],
                             oracle_zero[i].F0[row][col],
                             float_abs_tol);
            }
        }
    }

    const auto actual_config = io::read_config(join(actual_dir, "nano_config.dat"),
                                               oracle_dims.numnods,
                                               oracle_dims.numele,
                                               oracle_dims.ngauss);
    const auto oracle_config = io::read_config(join(oracle_dir, "nano_config.dat"),
                                               oracle_dims.numnods,
                                               oracle_dims.numele,
                                               oracle_dims.ngauss);
    for (int node = 0; node < oracle_dims.numnods; ++node) {
        for (int dim = 0; dim < 3; ++dim) {
            expect_close(mismatches,
                         "config.coords[" + std::to_string(node) + "][" + std::to_string(dim) + "]",
                         actual_config.coords[node][dim],
                         oracle_config.coords[node][dim],
                         float_abs_tol);
        }
    }
    for (int elem = 0; elem < oracle_dims.numele; ++elem) {
        for (int gauss = 0; gauss < oracle_dims.ngauss; ++gauss) {
            for (int dim = 0; dim < 2; ++dim) {
                expect_close(mismatches,
                             "config.eta[" + std::to_string(elem) + "][" + std::to_string(gauss) + "][" + std::to_string(dim) + "]",
                             actual_config.eta[elem][gauss][dim],
                             oracle_config.eta[elem][gauss][dim],
                             float_abs_tol);
            }
        }
    }

    const auto actual_bcs = io::read_bcs(join(actual_dir, "nano_BCs.dat"));
    const auto oracle_bcs = io::read_bcs(join(oracle_dir, "nano_BCs.dat"));
    expect_equal_int(mismatches, "bcs.nloadstep", actual_bcs.nloadstep, oracle_bcs.nloadstep);
    expect_equal_int(mismatches, "bcs.nCodeLoad", actual_bcs.nCodeLoad, oracle_bcs.nCodeLoad);
    expect_equal_int(mismatches, "bcs.nnodBC", actual_bcs.nnodBC, oracle_bcs.nnodBC);
    expect_equal_int(mismatches, "bcs.ndofBC", actual_bcs.ndofBC, oracle_bcs.ndofBC);
    expect_equal_int(mismatches, "bcs.ndofOP", actual_bcs.ndofOP, oracle_bcs.ndofOP);
    expect_equal_int(mismatches, "bcs.ncycles", actual_bcs.ncycles, oracle_bcs.ncycles);
    expect_equal_int(mismatches, "bcs.nloadstep_comp", actual_bcs.nloadstep_comp, oracle_bcs.nloadstep_comp);
    expect_equal_int(mismatches, "bcs.nloadstep_rel", actual_bcs.nloadstep_rel, oracle_bcs.nloadstep_rel);
    expect_close(mismatches, "bcs.value", actual_bcs.value, oracle_bcs.value, float_abs_tol);
    expect_close(mismatches, "bcs.value_comp", actual_bcs.value_comp, oracle_bcs.value_comp, float_abs_tol);
    expect_close(mismatches, "bcs.value_rel", actual_bcs.value_rel, oracle_bcs.value_rel, float_abs_tol);
    for (int i = 0; i < static_cast<int>(oracle_bcs.mdofBC.size()) && i < static_cast<int>(actual_bcs.mdofBC.size()); ++i) {
        expect_equal_int(mismatches, "bcs.mdofBC[" + std::to_string(i) + "]", actual_bcs.mdofBC[i], oracle_bcs.mdofBC[i]);
    }
    for (int i = 0; i < static_cast<int>(oracle_bcs.mdofOP.size()) && i < static_cast<int>(actual_bcs.mdofOP.size()); ++i) {
        expect_equal_int(mismatches, "bcs.mdofOP[" + std::to_string(i) + "]", actual_bcs.mdofOP[i], oracle_bcs.mdofOP[i]);
    }
    for (int i = 0; i < static_cast<int>(oracle_bcs.mnodBC.size()) && i < static_cast<int>(actual_bcs.mnodBC.size()); ++i) {
        expect_equal_int(mismatches, "bcs.mnodBC[" + std::to_string(i) + "].node", actual_bcs.mnodBC[i][0], oracle_bcs.mnodBC[i][0]);
        expect_equal_int(mismatches, "bcs.mnodBC[" + std::to_string(i) + "].tag", actual_bcs.mnodBC[i][1], oracle_bcs.mnodBC[i][1]);
    }
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            expect_close(mismatches,
                         "bcs.rotation[" + std::to_string(row) + "][" + std::to_string(col) + "]",
                         actual_bcs.rotation[row][col],
                         oracle_bcs.rotation[row][col],
                         float_abs_tol);
        }
        expect_close(mismatches, "bcs.xc[" + std::to_string(row) + "]", actual_bcs.xc[row], oracle_bcs.xc[row], float_abs_tol);
    }

    const auto actual_mesh = io::read_mesh(join(actual_dir, "nano_Mesh.dat"), oracle_dims.ngauss);
    const auto oracle_mesh = io::read_mesh(join(oracle_dir, "nano_Mesh.dat"), oracle_dims.ngauss);
    expect_equal_int(mismatches, "mesh.numele", actual_mesh.numele, oracle_mesh.numele);
    expect_equal_int(mismatches, "mesh.numnods", actual_mesh.numnods, oracle_mesh.numnods);
    expect_equal_int(mismatches, "mesh.nedge", actual_mesh.nedge, oracle_mesh.nedge);
    for (int elem = 0; elem < oracle_mesh.numele && elem < actual_mesh.numele; ++elem) {
        for (int i = 0; i < 3; ++i) {
            expect_equal_int(mismatches,
                             "mesh.connect[" + std::to_string(elem) + "].vertices[" + std::to_string(i) + "]",
                             actual_mesh.connect[elem].vertices[i],
                             oracle_mesh.connect[elem].vertices[i]);
            expect_equal_int(mismatches,
                             "mesh.connect[" + std::to_string(elem) + "].code_bc[" + std::to_string(i) + "]",
                             actual_mesh.connect[elem].code_bc[i],
                             oracle_mesh.connect[elem].code_bc[i]);
        }
        expect_equal_int(mismatches,
                         "mesh.connect[" + std::to_string(elem) + "].num_neigh_elem",
                         actual_mesh.connect[elem].num_neigh_elem,
                         oracle_mesh.connect[elem].num_neigh_elem);
        expect_equal_int(mismatches,
                         "mesh.connect[" + std::to_string(elem) + "].num_neigh_vert",
                         actual_mesh.connect[elem].num_neigh_vert,
                         oracle_mesh.connect[elem].num_neigh_vert);
        for (int i = 0; i < 12; ++i) {
            expect_equal_int(mismatches,
                             "mesh.connect[" + std::to_string(elem) + "].neigh_elem[" + std::to_string(i) + "]",
                             actual_mesh.connect[elem].neigh_elem[i],
                             oracle_mesh.connect[elem].neigh_elem[i]);
            expect_equal_int(mismatches,
                             "mesh.connect[" + std::to_string(elem) + "].neigh_vert[" + std::to_string(i) + "]",
                             actual_mesh.connect[elem].neigh_vert[i],
                             oracle_mesh.connect[elem].neigh_vert[i]);
        }
    }
    for (int edge = 0; edge < oracle_mesh.nedge && edge < actual_mesh.nedge; ++edge) {
        for (int i = 0; i < 3; ++i) {
            expect_equal_int(mismatches,
                             "mesh.nghost_tab[" + std::to_string(edge) + "][" + std::to_string(i) + "]",
                             actual_mesh.nghost_tab[edge][i],
                             oracle_mesh.nghost_tab[edge][i]);
        }
    }

    const auto actual_tub = io::read_tub_loc(join(actual_dir, "nano_tub_loc.dat"));
    const auto oracle_tub = io::read_tub_loc(join(oracle_dir, "nano_tub_loc.dat"));
    expect_equal_int(mismatches, "tub_loc.size", static_cast<int>(actual_tub.size()), static_cast<int>(oracle_tub.size()));
    for (int i = 0; i < static_cast<int>(oracle_tub.size()) && i < static_cast<int>(actual_tub.size()); ++i) {
        expect_equal_int(mismatches, "tub_loc[" + std::to_string(i) + "].first", actual_tub[i].first, oracle_tub[i].first);
        expect_equal_int(mismatches, "tub_loc[" + std::to_string(i) + "].second", actual_tub[i].second, oracle_tub[i].second);
    }

    const auto actual_crease_path = std::filesystem::path(actual_dir) / "nano_crease.dat";
    const auto oracle_crease_path = std::filesystem::path(oracle_dir) / "nano_crease.dat";
    const bool actual_has_crease = std::filesystem::exists(actual_crease_path);
    const bool oracle_has_crease = std::filesystem::exists(oracle_crease_path);
    expect_equal_int(mismatches,
                     "crease.exists",
                     actual_has_crease ? 1 : 0,
                     oracle_has_crease ? 1 : 0);
    if (actual_has_crease && oracle_has_crease) {
        const auto actual_crease =
            io::read_crease(actual_crease_path.string(), oracle_dims.numnods, oracle_dims.ngauss);
        const auto oracle_crease =
            io::read_crease(oracle_crease_path.string(), oracle_dims.numnods, oracle_dims.ngauss);
        expect_equal_int(mismatches, "crease.ncrease", actual_crease.ncrease, oracle_crease.ncrease);
        expect_close(mismatches,
                     "crease.kappa_cr",
                     actual_crease.kappa_cr,
                     oracle_crease.kappa_cr,
                     float_abs_tol);
        expect_close(mismatches,
                     "crease.alpha_lock",
                     actual_crease.alpha_lock,
                     oracle_crease.alpha_lock,
                     float_abs_tol);
    }

    if (!mismatches.empty()) {
        return ::testing::AssertionFailure() << mismatches.str();
    }
    return ::testing::AssertionSuccess();
}

} // namespace fce::test_support
