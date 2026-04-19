#pragma once

#include "value.hpp"

namespace sc {
namespace error_form {

// Gaussian error propagation for basic arithmetic.
// (x +/- dx) op (y +/- dy)

ValuePtr add(const ErrorForm& a, const ErrorForm& b, int precision);
ValuePtr sub(const ErrorForm& a, const ErrorForm& b, int precision);
ValuePtr mul(const ErrorForm& a, const ErrorForm& b, int precision);
ValuePtr div(const ErrorForm& a, const ErrorForm& b, int precision);
ValuePtr neg(const ErrorForm& a);
ValuePtr abs(const ErrorForm& a, int precision);

// Wrap a plain value as an error form with zero error
ValuePtr from_value(ValuePtr v);

} // namespace error_form
} // namespace sc
