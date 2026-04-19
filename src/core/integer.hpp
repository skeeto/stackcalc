#pragma once

#include "value.hpp"

namespace sc {

// Integer arithmetic — operates on Integer values, returns Integer ValuePtrs.
// These are the low-level building blocks; higher-level arithmetic in calc/
// handles type coercion and compound types.

namespace integer {

ValuePtr add(const Integer& a, const Integer& b);
ValuePtr sub(const Integer& a, const Integer& b);
ValuePtr mul(const Integer& a, const Integer& b);

// Integer division: truncates toward zero (like C++)
ValuePtr div(const Integer& a, const Integer& b);

// Floor division: toward negative infinity (Emacs calc \ key)
ValuePtr fdiv(const Integer& a, const Integer& b);

// Modulo: remainder after floor division (Emacs calc % key)
ValuePtr mod(const Integer& a, const Integer& b);

ValuePtr neg(const Integer& a);
ValuePtr abs(const Integer& a);

ValuePtr pow(const Integer& base, unsigned long exp);

ValuePtr gcd(const Integer& a, const Integer& b);
ValuePtr lcm(const Integer& a, const Integer& b);

ValuePtr factorial(unsigned long n);

// Compare: returns -1, 0, or 1
int cmp(const Integer& a, const Integer& b);

bool is_zero(const Integer& a);
bool is_negative(const Integer& a);
bool is_positive(const Integer& a);
bool is_even(const Integer& a);

} // namespace integer
} // namespace sc
