#pragma once

#include "value.h"
#include <mpfr.h>

namespace sc::mpfr_bridge {

// Convert a real ValuePtr to MPFR. Caller must mpfr_clear the result.
void to_mpfr(mpfr_t out, const ValuePtr& v, int precision);

// Convert MPFR back to DecimalFloat at given precision.
ValuePtr from_mpfr(const mpfr_t x, int precision);

// Unary functions: compute f(a) via MPFR at given decimal precision.
ValuePtr compute_unary(const ValuePtr& a, int precision,
                       int (*mpfr_fn)(mpfr_t, const mpfr_t, mpfr_rnd_t));

// Binary functions: compute f(a, b) via MPFR.
ValuePtr compute_binary(const ValuePtr& a, const ValuePtr& b, int precision,
                        int (*mpfr_fn)(mpfr_t, const mpfr_t, const mpfr_t, mpfr_rnd_t));

// Bits of precision needed for n decimal digits (with guard).
mpfr_prec_t decimal_to_binary_prec(int decimal_prec);

} // namespace sc::mpfr_bridge
