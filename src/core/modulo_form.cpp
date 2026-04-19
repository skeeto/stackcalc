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

ValuePtr div(const ModuloForm& a, const ModuloForm& b, int precision) {
    // a/b mod m = a * (b^-1) mod m. Requires gcd(b, m) == 1.
    check_moduli(a, b);
    if (!a.n->is_integer() || !b.n->is_integer() || !a.m->is_integer())
        throw std::invalid_argument("modular division requires integer values");
    mpz_class inv;
    int ok = mpz_invert(inv.get_mpz_t(),
                        b.n->as_integer().v.get_mpz_t(),
                        a.m->as_integer().v.get_mpz_t());
    if (!ok) throw std::domain_error("no modular inverse exists "
                                     "(divisor not coprime with modulus)");
    auto inv_v = Value::make_integer(std::move(inv));
    auto prod = arith::mul(a.n, inv_v, precision);
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
