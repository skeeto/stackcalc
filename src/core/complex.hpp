#pragma once

#include <cstdint>
#include "value.hpp"

namespace sc {
namespace complex {

// Rectangular complex arithmetic
ValuePtr add(const RectComplex& a, const RectComplex& b, int precision);
ValuePtr sub(const RectComplex& a, const RectComplex& b, int precision);
ValuePtr mul(const RectComplex& a, const RectComplex& b, int precision);
ValuePtr div(const RectComplex& a, const RectComplex& b, int precision);
ValuePtr neg(const RectComplex& a);
ValuePtr conj(const RectComplex& a);

// Build complex from two reals
ValuePtr make_rect(ValuePtr re, ValuePtr im);

// Convert real to complex (with im=0, returns the real itself)
ValuePtr from_real(ValuePtr re);

// Magnitude |a+bi| = sqrt(a^2 + b^2)
ValuePtr magnitude(const RectComplex& a, int precision);

// Argument angle
ValuePtr argument(const RectComplex& a, int precision);

// Reciprocal: conj(a) / |a|^2
ValuePtr inv(const RectComplex& a, int precision);

// Principal square root (closed-form, no trig)
ValuePtr sqrt(const RectComplex& a, int precision);

// Integer power via binary exponentiation
ValuePtr pow(const RectComplex& a, std::int64_t n, int precision);

} // namespace complex
} // namespace sc
