#pragma once

#include "value.h"
#include "modes.h"

namespace sc::scientific {

// --- Trigonometric (angular mode aware) ---
ValuePtr sin(const ValuePtr& a, int precision, AngularMode mode);
ValuePtr cos(const ValuePtr& a, int precision, AngularMode mode);
ValuePtr tan(const ValuePtr& a, int precision, AngularMode mode);
ValuePtr arcsin(const ValuePtr& a, int precision, AngularMode mode);
ValuePtr arccos(const ValuePtr& a, int precision, AngularMode mode);
ValuePtr arctan(const ValuePtr& a, int precision, AngularMode mode);
ValuePtr arctan2(const ValuePtr& y, const ValuePtr& x, int precision, AngularMode mode);

// --- Hyperbolic (no angular mode) ---
ValuePtr sinh(const ValuePtr& a, int precision);
ValuePtr cosh(const ValuePtr& a, int precision);
ValuePtr tanh(const ValuePtr& a, int precision);
ValuePtr arcsinh(const ValuePtr& a, int precision);
ValuePtr arccosh(const ValuePtr& a, int precision);
ValuePtr arctanh(const ValuePtr& a, int precision);

// --- Logarithmic ---
ValuePtr ln(const ValuePtr& a, int precision);
ValuePtr exp(const ValuePtr& a, int precision);
ValuePtr log10(const ValuePtr& a, int precision);
ValuePtr exp10(const ValuePtr& a, int precision);  // 10^a
ValuePtr logb(const ValuePtr& a, const ValuePtr& base, int precision); // log_base(a)
ValuePtr expm1(const ValuePtr& a, int precision);  // e^a - 1
ValuePtr lnp1(const ValuePtr& a, int precision);   // ln(1+a)

// --- Combinatorics ---
ValuePtr factorial(const ValuePtr& a, int precision);
ValuePtr double_factorial(const ValuePtr& a);
ValuePtr choose(const ValuePtr& n, const ValuePtr& m, int precision);
ValuePtr permutation(const ValuePtr& n, const ValuePtr& m, int precision);

// --- Other ---
ValuePtr sign(const ValuePtr& a);
ValuePtr hypot(const ValuePtr& a, const ValuePtr& b, int precision);

} // namespace sc::scientific
