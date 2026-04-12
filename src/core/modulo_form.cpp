#include "modulo_form.h"
#include "arithmetic.h"
#include <stdexcept>

namespace sc::modulo_form {

static void check_moduli(const ModuloForm& a, const ModuloForm& b) {
    if (!value_equal(a.m, b.m)) {
        throw std::invalid_argument("modulo forms have different moduli");
    }
}

ValuePtr reduce(ValuePtr n, ValuePtr m, int precision) {
    auto r = arith::mod(n, m, precision);
    return Value::make_mod(std::move(r), std::move(m));
}

ValuePtr add(const ModuloForm& a, const ModuloForm& b, int precision) {
    check_moduli(a, b);
    auto sum = arith::add(a.n, b.n, precision);
    return reduce(std::move(sum), a.m, precision);
}

ValuePtr sub(const ModuloForm& a, const ModuloForm& b, int precision) {
    check_moduli(a, b);
    auto diff = arith::sub(a.n, b.n, precision);
    return reduce(std::move(diff), a.m, precision);
}

ValuePtr mul(const ModuloForm& a, const ModuloForm& b, int precision) {
    check_moduli(a, b);
    auto prod = arith::mul(a.n, b.n, precision);
    return reduce(std::move(prod), a.m, precision);
}

ValuePtr neg(const ModuloForm& a, int precision) {
    auto n = arith::neg(a.n);
    return reduce(std::move(n), a.m, precision);
}

ValuePtr pow(const ModuloForm& a, const ValuePtr& exp, int precision) {
    // Modular exponentiation
    if (!exp->is_integer()) {
        throw std::invalid_argument("modular power requires integer exponent");
    }
    if (!a.n->is_integer() || !a.m->is_integer()) {
        throw std::invalid_argument("modular power requires integer base and modulus");
    }

    mpz_class result;
    mpz_powm(result.get_mpz_t(),
             a.n->as_integer().v.get_mpz_t(),
             exp->as_integer().v.get_mpz_t(),
             a.m->as_integer().v.get_mpz_t());
    return Value::make_mod(Value::make_integer(std::move(result)), a.m);
}

} // namespace sc::modulo_form
