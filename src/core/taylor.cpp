#include "fce/taylor.hpp"

#if defined(__clang__)
#pragma clang fp contract(off)
#pragma clang fp reassociate(off)
#endif

namespace fce {

double sinxx(const double x) {
    const double t2 = x * x;
    const double t4 = t2 * t2;
    const double t8 = t4 * t4;
    return 1.0 - t2 / 6.0 + t4 / 120.0 - t4 * t2 / 5040.0 + t8 / 362880.0 -
           t8 * t2 / 39916800.0;
}

double dsinxx(const double x) {
    const double t4 = x * x;
    const double t5 = t4 * x;
    const double t7 = t4 * t4;
    const double t12 = t7 * t7;
    return -x / 3.0 + t5 / 30.0 - t7 * x / 840.0 + t7 * t5 / 45360.0 -
           t12 * x / 3991680.0;
}

double ddsinxx(const double x) {
    const double t4 = x * x;
    const double t6 = t4 * t4;
    const double t10 = t6 * t6;
    return -1.0 / 3.0 + t4 / 10.0 - t6 / 168.0 + t6 * t4 / 6480.0 -
           t10 / 443520.0 + t10 * t4 / 47174400.0;
}

}  // namespace fce
