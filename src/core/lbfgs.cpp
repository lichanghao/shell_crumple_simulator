// L-BFGS solver — faithful C++ translation of lbfgs.f (Nocedal 1980 / More-Thuente line search).
//
// Fortran W array layout (1-based, total size N*(2M+1)+2M):
//   W(1..N)               : scratch (-H*g), then current gradient
//   W(N+1..N+M)           : rho scalars (indexed by CP = 0..M-1, W[N+CP] 0-based)
//   W(N+M+1..N+2M)        : alpha scalars (indexed by CP, W[N+M+CP] 0-based)
//   W(N+2M+1..N+2M+NM)    : s vectors  ISPT=N+2M, s[j] at W[ISPT + j*N .. +N-1]
//   W(N+2M+NM+1..N+2M+2NM): y vectors  IYPT=ISPT+N*M, y[j] at W[IYPT + j*N .. +N-1]
//
// In 0-based C++: same offsets but array indices shift by -1 for the rho/alpha slots.
// Specifically Fortran W(N+CP+1) = 0-based W[N+CP] where CP is 0..M-1.

#include "fce/lbfgs.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fce {

namespace {

bool lbfgs_deriv_trace_enabled() {
    const char* raw = std::getenv("FCE_LBFGS_DERIV_TRACE");
    if (raw == nullptr) {
        return false;
    }
    std::string value(raw);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return !(value.empty() || value == "0" || value == "false" ||
             value == "no" || value == "off");
}

bool lbfgs_state_trace_enabled() {
    const char* raw = std::getenv("FCE_LBFGS_STATE_TRACE");
    if (raw == nullptr) {
        return false;
    }
    std::string value(raw);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return !(value.empty() || value == "0" || value == "false" ||
             value == "no" || value == "off");
}

double vec_dot(int n, const double* a, const double* b) {
    double sum = 0.0;
    if (n <= 0) {
        return sum;
    }

    const int m = n % 5;
    for (int i = 0; i < m; ++i) {
        sum += a[i] * b[i];
    }
    if (n < 5) {
        return sum;
    }

    for (int i = m; i < n; i += 5) {
        sum += a[i] * b[i]
             + a[i + 1] * b[i + 1]
             + a[i + 2] * b[i + 2]
             + a[i + 3] * b[i + 3]
             + a[i + 4] * b[i + 4];
    }
    return sum;
}

void vec_axpy(int n, double alpha, const double* x, double* y) {
    if (n <= 0 || alpha == 0.0) {
        return;
    }

    const int m = n % 4;
    for (int i = 0; i < m; ++i) {
        y[i] += alpha * x[i];
    }
    if (n < 4) {
        return;
    }

    for (int i = m; i < n; i += 4) {
        y[i] += alpha * x[i];
        y[i + 1] += alpha * x[i + 1];
        y[i + 2] += alpha * x[i + 2];
        y[i + 3] += alpha * x[i + 3];
    }
}

}  // namespace

// ─── LbfgsSolver ─────────────────────────────────────────────────────────────

LbfgsSolver::LbfgsSolver(int m, double eps, double xtol, int max_eval, bool monitor)
    : m_(m), eps_(eps), xtol_(xtol), max_eval_(max_eval), monitor_(monitor) {}

void LbfgsSolver::reset() {
    lbfgs_initialized_ = false;
    mcsrch_initialized_ = false;
    iter_ = 0;
    nfun_ = 0;
    point_ = 0;
    finish_ = false;
    raw_gnorm_ = 1.0;
    crit_conv_ = 1.0;
    stp_ = 1.0;
    nfev_ls_ = 0;
    infoc_ = 1;
    deriv_trace_eval_ = 0;
    stopped_on_trial_gnorm_gate_ = false;
}

void LbfgsSolver::print_monitor_initial(const int n, const double f, const double critc) const {
    if (!monitor_) return;

    auto old_flags = std::cout.flags();
    auto old_precision = std::cout.precision();

    std::cout << "*************************************************\n";
    std::cout << "  N=" << std::setw(5) << n
              << "   NUMBER OF CORRECTIONS=" << m_ << "\n";
    std::cout << "       INITIAL VALUES\n";
    std::cout << std::uppercase << std::scientific << std::setprecision(3)
              << " F= " << std::setw(10) << f
              << "   CRITC= " << std::setw(10) << critc << "\n";
    std::cout << "*************************************************\n\n";
    std::cout << "   I   NFN    FUNC        GNORM       STEPLENGTH\n\n";

    std::cout.flags(old_flags);
    std::cout.precision(old_precision);
}

void LbfgsSolver::print_monitor_iteration(const int iter,
                                          const int nfun,
                                          const double f,
                                          const double critc,
                                          const double stp,
                                          const bool finish) const {
    if (!monitor_) return;

    auto old_flags = std::cout.flags();
    auto old_precision = std::cout.precision();

    std::cout << std::uppercase << std::scientific << std::setprecision(3)
              << std::setw(4) << iter
              << std::setw(5) << nfun
              << "   " << std::setw(10) << f
              << "  " << std::setw(10) << critc
              << "  " << std::setw(10) << stp << "\n";
    if (finish) {
        std::cout << "\n THE MINIMIZATION TERMINATED WITHOUT DETECTING ERRORS.\n";
        std::cout << " IFLAG = 0, NUMITER: " << std::setw(6) << iter << "\n";
    }

    std::cout.flags(old_flags);
    std::cout.precision(old_precision);
}

// ─── minimize ────────────────────────────────────────────────────────────────

int LbfgsSolver::minimize(std::vector<double>& x,
                           double xnorm0,
                           bool stop_on_first_trial,
                           std::function<std::pair<double, std::vector<double>>(
                               const std::vector<double>&)> callback) {
    const int n = static_cast<int>(x.size());
    if (n <= 0) return -3;

    const int nwork = n * (2 * m_ + 1) + 2 * m_;
    std::vector<double> w(static_cast<std::size_t>(nwork), 0.0);
    std::vector<double> diag(static_cast<std::size_t>(n), 1.0);

    reset();

    double f = 0.0;
    std::vector<double> g(static_cast<std::size_t>(n), 0.0);
    int icall = 0;

    // Fortran: on first entry IFLAG=0; then inner while loop calls LBFGS repeatedly
    // with IFLAG alternating between 1 (need f/g) and 0/negative (done).
    // We model this as: call callback to get f/g, then call internal step.

    while (true) {
        // Get f and g at current x.
        auto fg = callback(x);
        f  = fg.first;
        g  = fg.second;

        // NaN check.
        bool nan = (f != f);
        for (int i = 0; i < n && !nan; ++i) nan = (g[i] != g[i]) || (x[i] != x[i]);
        if (nan) return -4;

        double ddx = 0.0;
        int iflag = lbfgs_step(n, x, f, g, diag, w, xnorm0, ddx);

        if (iflag < 0) {
            if (monitor_) {
                std::cout << " LBFGS terminated with IFLAG=" << iflag
                          << " after " << nfun_ << " function evaluations"
                          << " (line-search info=" << infoc_
                          << ", nfev_ls=" << nfev_ls_
                          << ", stp=" << stp_
                          << ", dginit=" << dginit_ << ")\n";
            }
            // Line search failure — Fortran continues (goto 50) if f is not NaN.
            return iflag;
        }
        if (iflag == 0) {
            return 0;  // converged
        }

        // iflag == 1: x was updated by MCSRCH, need new f/g evaluation.
        // The caller-side reverse-communication gate in minimize.f90 and
        // minimize_free.f90 compares the stale GNORM argument returned by
        // LBFGS, which is actually CRIT_CONV, not the raw ||g||.
        if (stop_on_first_trial) {
            stopped_on_trial_gnorm_gate_ = true;
            return 0;
        }
        icall++;
        if (crit_conv_ < eps_) {
            stopped_on_trial_gnorm_gate_ = true;
            return 0;
        }
        if (icall > max_eval_) {
            return 1;
        }
    }
}

// ─── lbfgs_step ──────────────────────────────────────────────────────────────
// Models the Fortran LBFGS subroutine as a state machine.
// State variable: lbfgs_initialized_ tracks whether we're past the first iteration.
// The mcsrch_initialized_ tracks whether MCSRCH is mid-evaluation.
//
// Returns: 1 = caller should evaluate f/g at new x and call again
//          0 = converged
//         <0 = error

int LbfgsSolver::lbfgs_step(int n,
                              std::vector<double>& x,
                              double  f,
                              const std::vector<double>& g,
                              std::vector<double>& diag,
                              std::vector<double>& w,
                              double  xnorm0,
                              double& ddx_out) {
    const int ispt  = n + 2 * m_;   // 0-based offset for s-vectors in w
    const int iypt  = ispt + n * m_; // 0-based offset for y-vectors in w

    const double ftol   = 1.0e-6;
    const int    maxfev = 100;

    double stp = stp_;
    double ddx = 0.0;

    // ── FIRST ENTRY (IFLAG==0 in Fortran) ────────────────────────────────────
    if (!lbfgs_initialized_) {
        // GTOL correction.
        if (gtol_ <= 1.0e-4) gtol_ = 0.9;

        iter_   = 0;
        nfun_   = 1;
        point_  = 0;
        finish_ = false;

        // DIAGCO=false: DIAG = 1.
        for (int i = 0; i < n; ++i) diag[i] = 1.0;

        // Initial search direction: W[ISPT + i] = -G[i]*DIAG[i]
        for (int i = 0; i < n; ++i) {
            w[static_cast<std::size_t>(ispt + i)] = -g[i] * diag[i];
        }

        raw_gnorm_ = std::sqrt(vec_dot(n, g.data(), g.data()));
        stp1_  = 1.0 / raw_gnorm_;
        crit_conv_ = raw_gnorm_;

        // Fortran FTOL/MAXFEV already set as constants above.

        // Save current gradient in W[0..N-1] (for gradient difference later).
        for (int i = 0; i < n; ++i) w[static_cast<std::size_t>(i)] = g[i];

        print_monitor_initial(n, f, crit_conv_);

        // Initial step (STP=STP1 at iter==1).
        stp = stp1_;
        mcsrch_initialized_ = false;

        lbfgs_initialized_ = true;
    } else {
        // ── RE-ENTRY (IFLAG==1): f/g just evaluated at new x ─────────────────
        // Check if MCSRCH needs another function evaluation.
        int nfev_local = 0;
        // Get the current search direction from W[ISPT + point_*N .. ].
        const std::size_t s_off = static_cast<std::size_t>(ispt + point_ * n);
        std::vector<double> s_dir(w.begin() + static_cast<std::ptrdiff_t>(s_off),
                                   w.begin() + static_cast<std::ptrdiff_t>(s_off) + n);

        int info = mcsrch(n, x, f, const_cast<std::vector<double>&>(g),
                          s_dir, stp, ftol, xtol_, maxfev, nfev_local, diag, ddx);

        if (info == -1) {
            // MCSRCH needs another f/g — x updated.
            // Do NOT overwrite W[0..N-1]: it holds the gradient at the start of this
            // line search iteration (set before the first mcsrch call) and is needed
            // for the gradient-difference W[IYPT+NPT+I] = G[I] - W[I] later.
            stp_ = stp;
            ddx_out = ddx;
            return 1;
        }
        if (info != 1) {
            stp_ = stp;
            return -1;
        }

        // Line search succeeded.
        nfun_ += nfev_local;

        // Update s and y vectors.
        const int npt = point_ * n;
        for (int i = 0; i < n; ++i) {
            w[static_cast<std::size_t>(ispt + npt + i)] *= stp;
            w[static_cast<std::size_t>(iypt + npt + i)] = g[i] - w[static_cast<std::size_t>(i)];
        }
        point_++;
        if (point_ == m_) point_ = 0;

        // Termination test.
        raw_gnorm_ = std::sqrt(vec_dot(n, g.data(), g.data()));
        crit_conv_ = raw_gnorm_ / 100.0 + std::abs(ddx) / xnorm0;
        stp_ = stp;
        ddx_out = ddx;
        print_monitor_iteration(iter_, nfun_, f, crit_conv_, stp, crit_conv_ <= eps_);
        if (crit_conv_ <= eps_) {
            finish_ = true;
            return 0;
        }
    }

    // ── ADVANCE TO NEXT ITERATION ─────────────────────────────────────────────
    iter_++;

    // Compute -H*g using the two-loop recursion (Nocedal 1980).
    int bound = (iter_ - 1 < m_) ? iter_ - 1 : m_;

    // YS and YY from the most recently stored pair (before point_ was incremented).
    // The pair index is (point_ - 1 + m_) % m_ (the pair just stored).
    int prev = (point_ == 0) ? m_ - 1 : point_ - 1;
    const int npt_prev = prev * n;

    if (iter_ > 1) {
        double ys = vec_dot(n,
                            w.data() + iypt + npt_prev,
                            w.data() + ispt + npt_prev);
        double yy = vec_dot(n,
                            w.data() + iypt + npt_prev,
                            w.data() + iypt + npt_prev);
        for (int i = 0; i < n; ++i) diag[i] = ys / yy;

        // Store rho for the new pair: W[N + prev] = 1/YS.
        // Fortran: CP=POINT; W(N+CP)=1/YS. At this point CP=POINT after increment.
        // After increment point_ is already the *next* slot.
        // Fortran's CP = POINT at the start of label-80 block (after NPT/s,y update + POINT++).
        // Fortran W(N+CP) means W[N + point_] in 0-based? No: Fortran CP=POINT where POINT is 0..M-1
        // and W(N+CP) 1-based = W[N + CP - 1] 0-based.  But CP goes 0..M-1 in Fortran too.
        // Reading Fortran lines 330-332:
        //   CP=POINT; W(N+CP)=ONE/YS
        //   DO 112 I=1,N; W(I)=-G(I); END DO
        //   CP=POINT
        // W(N+CP) where CP is 0..M-1 and indexing is 1-based: slot = N+CP (1-based) = N+CP-1 (0-based).
        // CP=POINT which after increment can be 0..M-1. When CP=0, W(N+0)=W(N) → 0-based W[N-1].
        // That seems wrong. Let me re-check Fortran lines 330-336 carefully:
        //
        //   NPT=POINT*N
        //   DO 175 I=1,N
        //     W(ISPT+NPT+I)=STP*W(ISPT+NPT+I)
        //     W(IYPT+NPT+I)=G(I)-W(I)
        //   175 END DO
        //   POINT=POINT+1
        //   IF(POINT.EQ.M) POINT=0
        //
        //   GNORM = ...
        //   CRIT_CONV = ...
        //   IF(FINISH) RETURN
        //   GO TO 80
        //
        // 80: ITER=ITER+1; INFO=0; BOUND=ITER-1; IF(ITER.EQ.1) GO TO 165; IF(ITER>M) BOUND=M
        //
        //   YS = DDOT(N, W(IYPT+NPT+1), 1, W(ISPT+NPT+1), 1)
        //   → uses NPT which was POINT*N BEFORE the increment.
        //   But at label 80, NPT is NOT set. It was set at 391 as NPT=POINT*N before increment.
        //   The Fortran variable NPT persists (SAVE). So at label 80, NPT = old_POINT*N (before last increment).
        //   Actually in Fortran, variables are saved across GOTO by default with SAVE.
        //   NPT was set to POINT*N at line 391 (before increment), so it still holds that value.
        //
        //   CP=POINT (after increment, mod M, so 0..M-1)
        //   W(N+CP)=ONE/YS  → W[N+CP-1] in 0-based? But W(N+0)=W(N) → 0-based W[N-1]!
        //   That indexing would stomp on the scratch region. This doesn't match.
        //   Actually looking at the COMMON data: W(N*(2M+1)+2M). The rho array is W(N+1..N+M),
        //   i.e., W[N..N+M-1] in 0-based. CP goes 0..M-1.
        //   Fortran W(N+CP) with CP=0 gives W(N) which is 0-based index N-1... no, 1-based W(N)=0-based W[N-1].
        //   If CP=0: W(N+0)=W(N)=W[N-1] in 0-based. That's the last element of scratch!
        //   If CP=1: W(N+1) = W[N] in 0-based. That's rho[0].
        //   So CP effectively ranges 1..M not 0..M-1 for the rho storage?
        //
        // Re-reading: in Fortran POINT goes 0..M-1 and cycles. At label 330 CP=POINT.
        // But then at the backward loop (125): CP=CP-1; IF(CP.EQ.-1)CP=M-1.
        // The rho slot for pair j (0-based j) is W(N+CP+1) where CP = j (0-based). So W(N+j+1)=W[N+j] 0-based.
        // W(N+CP) in Fortran (1-based) at line 332 = W[N+CP-1] 0-based. With CP=point_ (0..M-1):
        //   point_=0 → W[N-1] ← collides with scratch!
        //   point_=1 → W[N]   ← rho slot for j=0
        // That is inconsistent. Let me re-read the actual Fortran at line 332:
        //
        //   CP= POINT
        //   IF (POINT.EQ.0) CP=M          ← *** This adjustment makes CP go 1..M ***
        //   W(N+CP)= ONE/YS
        //
        // Ah yes! Line 331: "IF (POINT.EQ.0) CP=M". So CP is in range 1..M.
        // W(N+CP) 1-based = W[N+CP-1] 0-based. With CP 1..M: W[N..N+M-1]. Correct!
        int cp_fort = (point_ == 0) ? m_ : point_;  // Fortran CP: range 1..M
        w[static_cast<std::size_t>(n + cp_fort - 1)] = 1.0 / ys;
    }

    // W[0..N-1] = -G (start of two-loop).
    for (int i = 0; i < n; ++i) w[static_cast<std::size_t>(i)] = -g[i];

    // Backward loop (Fortran labels 125, CP starts at POINT).
    {
        int cp = point_;  // 0-based 0..M-1
        for (int i = 1; i <= bound; ++i) {
            cp--;
            if (cp == -1) cp = m_ - 1;
            double sq = vec_dot(n, w.data() + ispt + cp * n, w.data());
            // INMC = N+M+CP (Fortran 1-based N+M+CP+1 → 0-based N+M+CP).
            // Fortran: INMC=N+M+CP+1 where CP is 0-based (0..M-1) → 0-based index = N+M+CP.
            int inmc = n + m_ + cp;
            // Fortran: W(N+CP+1) = rho for pair cp → 0-based: w[N+cp].
            double alpha_val = w[static_cast<std::size_t>(n + cp)] * sq;
            w[static_cast<std::size_t>(inmc)] = alpha_val;
            vec_axpy(n, -alpha_val, w.data() + iypt + cp * n, w.data());
        }

        // Scale by diagonal.
        for (int i = 0; i < n; ++i) w[static_cast<std::size_t>(i)] *= diag[i];

        // Forward loop (Fortran label 145).
        for (int i = 1; i <= bound; ++i) {
            double yr   = vec_dot(n, w.data() + iypt + cp * n, w.data());
            int    inmc = n + m_ + cp;
            double beta_val = w[static_cast<std::size_t>(n + cp)] * yr;
            beta_val = w[static_cast<std::size_t>(inmc)] - beta_val;
            vec_axpy(n, beta_val, w.data() + ispt + cp * n, w.data());
            cp++;
            if (cp == m_) cp = 0;
        }
    }

    // Store new search direction.
    for (int i = 0; i < n; ++i) {
        w[static_cast<std::size_t>(ispt + point_ * n + i)] = w[static_cast<std::size_t>(i)];
    }

    // ── PREPARE LINE SEARCH ───────────────────────────────────────────────────
    // Save current gradient for gradient-difference computation.
    for (int i = 0; i < n; ++i) w[static_cast<std::size_t>(i)] = g[i];

    mcsrch_initialized_ = false;
    stp = (iter_ == 1) ? stp1_ : 1.0;

    // Call MCSRCH with new search direction.
    {
        const std::size_t s_off = static_cast<std::size_t>(ispt + point_ * n);
        std::vector<double> s_dir(w.begin() + static_cast<std::ptrdiff_t>(s_off),
                                   w.begin() + static_cast<std::ptrdiff_t>(s_off) + n);

        int nfev_local = 0;
        int info = mcsrch(n, x, f, const_cast<std::vector<double>&>(g),
                          s_dir, stp, ftol, xtol_, maxfev, nfev_local, diag, ddx);

        if (info == -1) {
            for (int i = 0; i < n; ++i) w[static_cast<std::size_t>(i)] = g[i];
            stp_ = stp;
            ddx_out = ddx;
            return 1;
        }
        if (info != 1) {
            stp_ = stp;
            return -1;
        }

        // Immediate success (unusual but handled).
        nfun_ += nfev_local;
        const int npt = point_ * n;
        for (int i = 0; i < n; ++i) {
            w[static_cast<std::size_t>(ispt + npt + i)] *= stp;
            w[static_cast<std::size_t>(iypt + npt + i)] = g[i] - w[static_cast<std::size_t>(i)];
        }
        point_++;
        if (point_ == m_) point_ = 0;
        raw_gnorm_ = std::sqrt(vec_dot(n, g.data(), g.data()));
        crit_conv_ = raw_gnorm_ / 100.0 + std::abs(ddx) / xnorm0;
        stp_ = stp;
        ddx_out = ddx;
        print_monitor_iteration(iter_, nfun_, f, crit_conv_, stp, crit_conv_ <= eps_);
        return (crit_conv_ <= eps_) ? 0 : 1;
    }
}

// ─── mcsrch ──────────────────────────────────────────────────────────────────

int LbfgsSolver::mcsrch(int n,
                          std::vector<double>& x,
                          double& f,
                          std::vector<double>& g,
                          const std::vector<double>& s,
                          double& stp,
                          double  ftol,
                          double  xtol_arg,
                          int     maxfev,
                          int&    nfev,
                          std::vector<double>& wa,
                          double& ddx) {
    const double p5     = 0.5;
    const double p66    = 0.66;
    const double xtrapf = 4.0;
    const double zero   = 0.0;

    if (!mcsrch_initialized_) {
        // First entry: validate and initialise.
        infoc_ = 1;
        if (n <= 0 || stp <= zero || ftol < zero || gtol_ < zero ||
            xtol_arg < zero || stpmin_ < zero || stpmax_ < stpmin_ || maxfev <= 0) {
            return 0;
        }

        dginit_ = zero;
        for (int j = 0; j < n; ++j) dginit_ += g[j] * s[j];

        if (dginit_ >= zero) return 0;

        brackt_  = false;
        stage1_  = true;
        nfev_ls_ = 0;
        nfev     = nfev_ls_;
        finit_   = f;
        dgtest_  = ftol * dginit_;
        width_   = stpmax_ - stpmin_;
        width1_  = width_ / p5;

        for (int j = 0; j < n; ++j) wa[static_cast<std::size_t>(j)] = x[j];

        stx_ = zero;  fx_ = finit_;  dgx_ = dginit_;
        sty_ = zero;  fy_ = finit_;  dgy_ = dginit_;

        mcsrch_initialized_ = true;
    } else {
        // Re-entry: process the new f/g.
        nfev_ls_++;
        nfev = nfev_ls_;
        deriv_trace_eval_++;
        double dg = zero;
        for (int j = 0; j < n; ++j) dg += g[j] * s[j];
        if (lbfgs_deriv_trace_enabled()) {
            const double gnorm = std::sqrt(vec_dot(n, g.data(), g.data()));
            std::cout << "DERIV " << deriv_trace_eval_ << " " << nfev_ls_ << " "
                      << std::uppercase << std::scientific << std::setprecision(16)
                      << f << " " << dg << " " << gnorm << " " << stp << "\n";
        }

        double ftest1 = finit_ + stp * dgtest_;
        int info = 0;
        if (lbfgs_state_trace_enabled()) {
            std::cout << "STATE_PRE " << deriv_trace_eval_ << " " << nfev_ls_ << " "
                      << stage1_ << " " << brackt_ << " "
                      << std::uppercase << std::scientific << std::setprecision(16)
                      << stx_ << " " << sty_ << " "
                      << fx_ << " " << fy_ << " "
                      << dgx_ << " " << dgy_ << " "
                      << width_ << " " << width1_ << " "
                      << stp << " " << ftest1 << "\n";
        }

        if ((brackt_ && (stp <= std::min(stx_, sty_) || stp >= std::max(stx_, sty_))) ||
            infoc_ == 0) {
            info = 6;
        }
        if (stp == stpmax_ && f <= ftest1 && dg <= dgtest_) info = 5;
        if (stp == stpmin_ && (f > ftest1 || dg >= dgtest_))  info = 4;
        if (nfev >= maxfev)                                     info = 3;
        if (brackt_) {
            double lo = std::min(stx_, sty_), hi = std::max(stx_, sty_);
            if (hi - lo <= xtol_arg * hi) info = 2;
        }
        if (f <= ftest1 && std::abs(dg) <= gtol_ * (-dginit_)) info = 1;

        if (info != 0) {
            mcsrch_initialized_ = false;
            return info;
        }

        if (stage1_ && f <= ftest1 && dg >= std::min(ftol, gtol_) * dginit_) {
            stage1_ = false;
        }

        double stmin_loc, stmax_loc;
        if (brackt_) {
            stmin_loc = std::min(stx_, sty_);
            stmax_loc = std::max(stx_, sty_);
        } else {
            stmin_loc = stx_;
            stmax_loc = stp + xtrapf * (stp - stx_);
        }

        if (stage1_ && f <= fx_ && f > ftest1) {
            double fm   = f   - stp  * dgtest_;
            double fxm  = fx_ - stx_ * dgtest_;
            double fym  = fy_ - sty_ * dgtest_;
            double dgm  = dg  - dgtest_;
            double dgxm = dgx_ - dgtest_;
            double dgym = dgy_ - dgtest_;
            mcstep(stx_, fxm, dgxm, sty_, fym, dgym, stp, fm, dgm,
                   brackt_, stmin_loc, stmax_loc, infoc_);
            fx_  = fxm + stx_ * dgtest_;
            fy_  = fym + sty_ * dgtest_;
            dgx_ = dgxm + dgtest_;
            dgy_ = dgym + dgtest_;
        } else {
            mcstep(stx_, fx_, dgx_, sty_, fy_, dgy_, stp, f, dg,
                   brackt_, stmin_loc, stmax_loc, infoc_);
        }

        if (brackt_) {
            if (std::abs(sty_ - stx_) >= p66 * width1_) {
                stp = stx_ + p5 * (sty_ - stx_);
            }
            width1_ = width_;
            width_  = std::abs(sty_ - stx_);
        }
        if (lbfgs_state_trace_enabled()) {
            std::cout << "STATE_POST " << deriv_trace_eval_ << " " << nfev_ls_ << " "
                      << stage1_ << " " << brackt_ << " "
                      << std::uppercase << std::scientific << std::setprecision(16)
                      << stx_ << " " << sty_ << " "
                      << fx_ << " " << fy_ << " "
                      << dgx_ << " " << dgy_ << " "
                      << width_ << " " << width1_ << " "
                      << stp << "\n";
        }
    }

    double stmin_eval, stmax_eval;
    if (brackt_) {
        stmin_eval = std::min(stx_, sty_);
        stmax_eval = std::max(stx_, sty_);
    } else {
        stmin_eval = stx_;
        stmax_eval = stp + xtrapf * (stp - stx_);
    }
    // Mirror the Fortran label-30 safeguards before requesting another f/g.
    stp = std::max(stp, stpmin_);
    stp = std::min(stp, stpmax_);
    if ((brackt_ && (stp <= stmin_eval || stp >= stmax_eval)) ||
        nfev_ls_ >= maxfev - 1 ||
        infoc_ == 0 ||
        (brackt_ && stmax_eval - stmin_eval <= xtol_arg * stmax_eval)) {
        stp = stx_;
    }

    // Compute new trial x.
    double slen = 0.0;
    for (int j = 0; j < n; ++j) {
        x[j] = wa[static_cast<std::size_t>(j)] + stp * s[j];
        slen += s[j] * s[j];
    }
    ddx = stp * std::sqrt(slen);
    nfev = nfev_ls_;
    return -1;  // needs f/g evaluation
}

// ─── mcstep ──────────────────────────────────────────────────────────────────

void LbfgsSolver::mcstep(double& stx, double& fx, double& dx,
                           double& sty, double& fy, double& dy,
                           double& stp, double  fp, double  dp,
                           bool&   brackt,
                           double  stpmin, double stpmax,
                           int&    info) {
    info = 0;

    if ((brackt && (stp <= std::min(stx, sty) || stp >= std::max(stx, sty))) ||
        dx * (stp - stx) >= 0.0 || stpmax < stpmin) {
        return;
    }

    double sgnd = dp * (dx / std::abs(dx));
    double stpf;
    double theta, s, gamma, p, q, r;
    double stpc, stpq;
    bool   bound;

    if (fp > fx) {
        // Case 1.
        info = 1; bound = true;
        theta = 3.0*(fx - fp)/(stp - stx) + dx + dp;
        s = std::max({std::abs(theta), std::abs(dx), std::abs(dp)});
        gamma = s * std::sqrt((theta/s)*(theta/s) - (dx/s)*(dp/s));
        if (stp < stx) gamma = -gamma;
        p = (gamma - dx) + theta;
        q = ((gamma - dx) + gamma) + dp;
        r = p/q;
        stpc = stx + r*(stp - stx);
        stpq = stx + ((dx/((fx-fp)/(stp-stx)+dx))/2.0)*(stp - stx);
        stpf = (std::abs(stpc-stx) < std::abs(stpq-stx)) ? stpc : stpc + (stpq-stpc)/2.0;
        brackt = true;

    } else if (sgnd < 0.0) {
        // Case 2.
        info = 2; bound = false;
        theta = 3.0*(fx - fp)/(stp - stx) + dx + dp;
        s = std::max({std::abs(theta), std::abs(dx), std::abs(dp)});
        gamma = s * std::sqrt((theta/s)*(theta/s) - (dx/s)*(dp/s));
        if (stp > stx) gamma = -gamma;
        p = (gamma - dp) + theta;
        q = ((gamma - dp) + gamma) + dx;
        r = p/q;
        stpc = stp + r*(stx - stp);
        stpq = stp + (dp/(dp - dx))*(stx - stp);
        stpf = (std::abs(stpc-stp) > std::abs(stpq-stp)) ? stpc : stpq;
        brackt = true;

    } else if (std::abs(dp) < std::abs(dx)) {
        // Case 3.
        info = 3; bound = true;
        theta = 3.0*(fx - fp)/(stp - stx) + dx + dp;
        s = std::max({std::abs(theta), std::abs(dx), std::abs(dp)});
        gamma = s * std::sqrt(std::max(0.0, (theta/s)*(theta/s) - (dx/s)*(dp/s)));
        if (stp > stx) gamma = -gamma;
        p = (gamma - dp) + theta;
        q = (gamma + (dx - dp)) + gamma;
        r = p/q;
        if (r < 0.0 && gamma != 0.0) {
            stpc = stp + r*(stx - stp);
        } else if (stp > stx) {
            stpc = stpmax;
        } else {
            stpc = stpmin;
        }
        stpq = stp + (dp/(dp - dx))*(stx - stp);
        if (brackt) {
            stpf = (std::abs(stp-stpc) < std::abs(stp-stpq)) ? stpc : stpq;
        } else {
            stpf = (std::abs(stp-stpc) > std::abs(stp-stpq)) ? stpc : stpq;
        }

    } else {
        // Case 4.
        info = 4; bound = false;
        if (brackt) {
            theta = 3.0*(fp - fy)/(sty - stp) + dy + dp;
            s = std::max({std::abs(theta), std::abs(dy), std::abs(dp)});
            gamma = s * std::sqrt((theta/s)*(theta/s) - (dy/s)*(dp/s));
            if (stp > sty) gamma = -gamma;
            p = (gamma - dp) + theta;
            q = ((gamma - dp) + gamma) + dy;
            r = p/q;
            stpc = stp + r*(sty - stp);
            stpf = stpc;
        } else if (stp > stx) {
            stpf = stpmax;
        } else {
            stpf = stpmin;
        }
    }

    // Update interval.
    if (fp > fx) {
        sty = stp; fy = fp; dy = dp;
    } else {
        if (sgnd < 0.0) { sty = stx; fy = fx; dy = dx; }
        stx = stp; fx = fp; dx = dp;
    }

    // Safeguard.
    stpf = std::min(stpmax, stpf);
    stpf = std::max(stpmin, stpf);
    stp  = stpf;
    if (brackt && bound) {
        if (sty > stx) stp = std::min(stx + 0.66*(sty - stx), stp);
        else           stp = std::max(stx + 0.66*(sty - stx), stp);
    }
}

}  // namespace fce
