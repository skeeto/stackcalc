#pragma once

#include "value.h"

namespace sc {
namespace infinity_ops {

// Infinity arithmetic rules:
// a + inf = inf, a - inf = -inf, a * inf = inf (a>0) or -inf (a<0)
// inf + inf = inf, inf - inf = nan
// 0 * inf = nan, inf / inf = nan, 0 / 0 = nan
// a / 0 = inf (in infinite mode)

ValuePtr add(const Infinity& a, const ValuePtr& b);
ValuePtr add(const ValuePtr& a, const Infinity& b);
ValuePtr mul(const Infinity& a, const ValuePtr& b);
ValuePtr neg(const Infinity& a);

} // namespace infinity_ops
} // namespace sc
