#pragma once

#include "value.h"

namespace sc::constants {

// Compute constants at the given decimal precision.
// Results are cached and recomputed only if precision increases.
ValuePtr pi(int precision);
ValuePtr e(int precision);
ValuePtr gamma(int precision);  // Euler-Mascheroni constant
ValuePtr phi(int precision);    // Golden ratio (1+sqrt(5))/2

} // namespace sc::constants
