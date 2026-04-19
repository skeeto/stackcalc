#pragma once

#include "value.hpp"
#include "modes.hpp"

namespace sc::arith {

// Value-level arithmetic with type dispatch and coercion.
// These are the core arithmetic building blocks used by compound types
// and by the stack operations layer.

ValuePtr add(const ValuePtr& a, const ValuePtr& b, int precision);
ValuePtr sub(const ValuePtr& a, const ValuePtr& b, int precision);
ValuePtr mul(const ValuePtr& a, const ValuePtr& b, int precision);
ValuePtr div(const ValuePtr& a, const ValuePtr& b, int precision,
             FractionMode frac_mode = FractionMode::Float);
ValuePtr idiv(const ValuePtr& a, const ValuePtr& b, int precision);
ValuePtr mod(const ValuePtr& a, const ValuePtr& b, int precision);
ValuePtr power(const ValuePtr& a, const ValuePtr& b, int precision);

ValuePtr neg(const ValuePtr& a);
ValuePtr inv(const ValuePtr& a, int precision);
ValuePtr sqrt(const ValuePtr& a, int precision);
ValuePtr abs(const ValuePtr& a);

ValuePtr floor(const ValuePtr& a);
ValuePtr ceil(const ValuePtr& a);
ValuePtr round(const ValuePtr& a);
ValuePtr trunc(const ValuePtr& a);

} // namespace sc::arith
