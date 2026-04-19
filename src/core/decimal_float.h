#pragma once

#include "value.h"
#include <optional>

namespace sc {
namespace decimal_float {

// Count the number of decimal digits in |n|. Returns 0 for n==0.
int num_digits(const mpz_class& n);

// Arithmetic at given precision. Results are normalized (trailing zeros stripped,
// at most `precision` digits in mantissa).
ValuePtr add(const DecimalFloat& a, const DecimalFloat& b, int precision);
ValuePtr sub(const DecimalFloat& a, const DecimalFloat& b, int precision);
ValuePtr mul(const DecimalFloat& a, const DecimalFloat& b, int precision);
ValuePtr div(const DecimalFloat& a, const DecimalFloat& b, int precision);

ValuePtr neg(const DecimalFloat& a);
ValuePtr abs(const DecimalFloat& a);

// Floor, ceiling, round, truncate — returns Integer
ValuePtr floor(const DecimalFloat& a);
ValuePtr ceil(const DecimalFloat& a);
ValuePtr round(const DecimalFloat& a);  // half away from zero
ValuePtr trunc(const DecimalFloat& a);

// Integer division and modulo
ValuePtr fdiv(const DecimalFloat& a, const DecimalFloat& b, int precision);
ValuePtr mod(const DecimalFloat& a, const DecimalFloat& b, int precision);

// Power (integer exponent)
ValuePtr pow_int(const DecimalFloat& base, long exp, int precision);

// Square root
ValuePtr sqrt(const DecimalFloat& a, int precision);

// Reciprocal
ValuePtr reciprocal(const DecimalFloat& a, int precision);

// Convert from integer
DecimalFloat from_integer(const Integer& i);

// Convert from fraction
DecimalFloat from_fraction(const Fraction& f, int precision);

// Reconstruct the full integer value (mantissa * 10^exponent) if exponent >= 0
// Returns nullopt if the float has a fractional part
std::optional<mpz_class> to_integer(const DecimalFloat& f);

int cmp(const DecimalFloat& a, const DecimalFloat& b);
bool is_zero(const DecimalFloat& a);
bool is_negative(const DecimalFloat& a);

} // namespace decimal_float
} // namespace sc
