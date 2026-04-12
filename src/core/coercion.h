#pragma once

#include "value.h"

namespace sc {

// Type coercion for binary arithmetic operations.
// Promotion chain: Integer -> Fraction -> DecimalFloat
// If either operand is a float, both become floats.
// If either is a fraction (and neither is float), both become fractions.

namespace coerce {

// Convert an integer or fraction to a DecimalFloat
ValuePtr to_float(const ValuePtr& v, int precision);

// Convert an integer to a fraction
ValuePtr to_fraction(const ValuePtr& v);

// Promote two real values to a common numeric type.
// Returns the common tag and the two promoted values.
struct CoercedPair {
    ValuePtr a;
    ValuePtr b;
    Tag common;
};

CoercedPair coerce_pair(const ValuePtr& a, const ValuePtr& b, int precision);

} // namespace coerce
} // namespace sc
