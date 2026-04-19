#pragma once

#include "value.hpp"

namespace sc {
namespace interval {

// Interval arithmetic: compute worst-case bounds
ValuePtr add(const Interval& a, const Interval& b, int precision);
ValuePtr sub(const Interval& a, const Interval& b, int precision);
ValuePtr mul(const Interval& a, const Interval& b, int precision);
ValuePtr div(const Interval& a, const Interval& b, int precision);
ValuePtr neg(const Interval& a);
ValuePtr abs(const Interval& a, int precision);

// Integer exponent only. Naive repeated multiplication via mul (so the
// resulting interval correctly accounts for sign-flips when the base
// interval crosses zero).
ValuePtr pow(const Interval& a, long n, int precision);

} // namespace interval
} // namespace sc
