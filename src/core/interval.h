#pragma once

#include "value.h"

namespace sc {
namespace interval {

// Interval arithmetic: compute worst-case bounds
ValuePtr add(const Interval& a, const Interval& b, int precision);
ValuePtr sub(const Interval& a, const Interval& b, int precision);
ValuePtr mul(const Interval& a, const Interval& b, int precision);
ValuePtr div(const Interval& a, const Interval& b, int precision);
ValuePtr neg(const Interval& a);
ValuePtr abs(const Interval& a, int precision);

} // namespace interval
} // namespace sc
