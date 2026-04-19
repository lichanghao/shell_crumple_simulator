#pragma once
// L-BFGS solver translated from lbfgs.f (Nocedal 1980 / More-Thuente line search).
// Faithfully mirrors the Fortran LBFGS + MCSRCH + MCSTEP subroutines.
//
// Key parameters (matching Fortran constants):
//   M (MSAVE)  = 10 corrections
//   FTOL       = 1e-6  (line search sufficient decrease)
//   MAXFEV     = 100   (max function evaluations per line search)
//   XTOL       = 1e-12 (machine precision for line search)
//   GTOL       = 0.9   (curvature condition for line search)
//   STPMIN     = 1e-20
//   STPMAX     = 1e+20
//
// Convergence criterion (from Fortran):
//   CRIT_CONV = GNORM/100 + |ddX|/XNORM0 <= EPS
//
// Usage:
//   LbfgsSolver solver;
//   int flag = solver.minimize(x, xnorm0, callback);

#include <functional>
#include <utility>
#include <vector>

namespace fce {

class LbfgsSolver {
public:
    // m       : number of corrections stored (MSAVE, default 10)
    // eps     : convergence tolerance (default 1e-8)
    // xtol    : machine precision for line search (default 1e-12)
    // max_eval: max gradient evaluations before forced exit (default 20000)
    // monitor : when true, print Fortran-style iteration diagnostics on stdout
    explicit LbfgsSolver(int m = 10, double eps = 1.0e-8, double xtol = 1.0e-12,
                         int max_eval = 20000,
                         bool monitor = false);

    // Minimize f(x) starting from x (modified in-place to the minimizer).
    // xnorm0   : precomputed bbox diagonal of initial configuration (see minimize.f90)
    // callback : given x, returns {f, gradient}
    //
    // Returns:
    //   0  = converged normally (CRIT_CONV <= eps or GNORM < eps)
    //   <0 = line search failure (IFLAG < 0 in Fortran)
    //   1  = max_eval reached without convergence
    int minimize(std::vector<double>& x,
                 double xnorm0,
                 bool stop_on_first_trial,
                 std::function<std::pair<double, std::vector<double>>(
                     const std::vector<double>&)> callback);

    // Reset history so the next minimize() call starts fresh (called between load steps).
    void reset();

    double gnorm() const { return crit_conv_; }
    double raw_gnorm() const { return raw_gnorm_; }
    bool stopped_on_trial_gnorm_gate() const { return stopped_on_trial_gnorm_gate_; }
    void set_accepted_step_observer(
        std::function<void(int, int, double, double, double,
                           const std::vector<double>&,
                           const std::vector<double>&)> observer) {
        accepted_step_observer_ = std::move(observer);
    }

private:
    // ── configuration ────────────────────────────────────────────────────────
    int    m_;
    double eps_;
    double xtol_;
    int    max_eval_;

    // ── COMMON /LB3/ equivalents ─────────────────────────────────────────────
    double gtol_{0.9};
    double stpmin_{1.0e-20};
    double stpmax_{1.0e+20};

    // ── iteration state ───────────────────────────────────────────────────────
    double raw_gnorm_{1.0};
    double crit_conv_{1.0};
    bool   monitor_{false};
    bool   stopped_on_trial_gnorm_gate_{false};

    // ── L-BFGS core: mirrors the Fortran LBFGS subroutine ───────────────────
    // Returns IFLAG: 1 = needs f/g evaluation, 0 = converged, <0 = error.
    int lbfgs_step(int n,
                   std::vector<double>& x,
                   double  f,
                   const std::vector<double>& g,
                   std::vector<double>& diag,
                   std::vector<double>& w,
                   double  xnorm0,
                   double& ddx_out);

    // ── Line search: mirrors Fortran MCSRCH ──────────────────────────────────
    // Returns INFO: 1 = success, -1 = needs f/g evaluation, other = error/done.
    int mcsrch(int n,
               std::vector<double>& x,
               double& f,
               std::vector<double>& g,
               const std::vector<double>& s,
               double& stp,
               double  ftol,
               double  xtol,
               int     maxfev,
               int&    nfev,
               std::vector<double>& wa,
               double& ddx);

    // ── MCSTEP ───────────────────────────────────────────────────────────────
    static void mcstep(double& stx, double& fx, double& dx,
                       double& sty, double& fy, double& dy,
                       double& stp, double  fp, double  dp,
                       bool&   brackt,
                       double  stpmin, double stpmax,
                       int&    info);

    void print_monitor_initial(int n, double f, double critc) const;
    void print_monitor_iteration(int iter, int nfun, double f, double critc,
                                 double stp, bool finish) const;

    // ── persistent LBFGS state (mirrors Fortran SAVE variables) ─────────────
    bool   lbfgs_initialized_{false};
    int    iter_{0};
    int    nfun_{0};
    int    point_{0};
    bool   finish_{false};
    double stp_{1.0};
    double stp1_{1.0};

    // ── MCSRCH persistent state ───────────────────────────────────────────────
    bool   mcsrch_initialized_{false};
    bool   brackt_{false};
    bool   stage1_{true};
    int    nfev_ls_{0};
    int    infoc_{1};
    double dginit_{0.0};
    double dgtest_{0.0};
    double finit_{0.0};
    double width_{0.0};
    double width1_{0.0};
    double stx_{0.0};
    double fx_{0.0};
    double dgx_{0.0};
    double sty_{0.0};
    double fy_{0.0};
    double dgy_{0.0};
    int    deriv_trace_eval_{0};
    std::function<void(int, int, double, double, double,
                       const std::vector<double>&,
                       const std::vector<double>&)>
        accepted_step_observer_{};
};

}  // namespace fce
