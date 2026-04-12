#pragma once

#include "value.h"

namespace sc {
namespace modulo_form {

ValuePtr add(const ModuloForm& a, const ModuloForm& b, int precision);
ValuePtr sub(const ModuloForm& a, const ModuloForm& b, int precision);
ValuePtr mul(const ModuloForm& a, const ModuloForm& b, int precision);
ValuePtr neg(const ModuloForm& a, int precision);
ValuePtr pow(const ModuloForm& a, const ValuePtr& exp, int precision);

// Reduce value modulo m
ValuePtr reduce(ValuePtr n, ValuePtr m, int precision);

} // namespace modulo_form
} // namespace sc
