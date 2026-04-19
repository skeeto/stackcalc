#pragma once

#include "value.hpp"

namespace sc {
namespace fraction {

ValuePtr add(const Fraction& a, const Fraction& b);
ValuePtr sub(const Fraction& a, const Fraction& b);
ValuePtr mul(const Fraction& a, const Fraction& b);
ValuePtr div(const Fraction& a, const Fraction& b);

ValuePtr neg(const Fraction& a);
ValuePtr abs(const Fraction& a);
ValuePtr reciprocal(const Fraction& a);

// Floor division and modulo for fractions
ValuePtr fdiv(const Fraction& a, const Fraction& b);
ValuePtr mod(const Fraction& a, const Fraction& b);

// Convert integer to fraction
Fraction from_integer(const Integer& i);

// Convert fraction to decimal float at given precision
ValuePtr to_float(const Fraction& f, int precision);

int cmp(const Fraction& a, const Fraction& b);
bool is_zero(const Fraction& a);
bool is_negative(const Fraction& a);

} // namespace fraction
} // namespace sc
