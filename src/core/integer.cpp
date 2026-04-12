#include "integer.h"
#include <stdexcept>

namespace sc::integer {

ValuePtr add(const Integer& a, const Integer& b) {
    return Value::make_integer(a.v + b.v);
}

ValuePtr sub(const Integer& a, const Integer& b) {
    return Value::make_integer(a.v - b.v);
}

ValuePtr mul(const Integer& a, const Integer& b) {
    return Value::make_integer(a.v * b.v);
}

ValuePtr div(const Integer& a, const Integer& b) {
    if (b.v == 0) throw std::domain_error("division by zero");
    // Truncate toward zero (C++ semantics)
    mpz_class q;
    mpz_tdiv_q(q.get_mpz_t(), a.v.get_mpz_t(), b.v.get_mpz_t());
    return Value::make_integer(std::move(q));
}

ValuePtr fdiv(const Integer& a, const Integer& b) {
    if (b.v == 0) throw std::domain_error("division by zero");
    // Floor division (toward -inf)
    mpz_class q;
    mpz_fdiv_q(q.get_mpz_t(), a.v.get_mpz_t(), b.v.get_mpz_t());
    return Value::make_integer(std::move(q));
}

ValuePtr mod(const Integer& a, const Integer& b) {
    if (b.v == 0) throw std::domain_error("division by zero");
    // Remainder after floor division (always same sign as divisor)
    mpz_class r;
    mpz_fdiv_r(r.get_mpz_t(), a.v.get_mpz_t(), b.v.get_mpz_t());
    return Value::make_integer(std::move(r));
}

ValuePtr neg(const Integer& a) {
    return Value::make_integer(-a.v);
}

ValuePtr abs(const Integer& a) {
    return Value::make_integer(::abs(a.v));
}

ValuePtr pow(const Integer& base, unsigned long exp) {
    mpz_class result;
    mpz_pow_ui(result.get_mpz_t(), base.v.get_mpz_t(), exp);
    return Value::make_integer(std::move(result));
}

ValuePtr gcd(const Integer& a, const Integer& b) {
    mpz_class result;
    mpz_gcd(result.get_mpz_t(), a.v.get_mpz_t(), b.v.get_mpz_t());
    return Value::make_integer(std::move(result));
}

ValuePtr lcm(const Integer& a, const Integer& b) {
    mpz_class result;
    mpz_lcm(result.get_mpz_t(), a.v.get_mpz_t(), b.v.get_mpz_t());
    return Value::make_integer(std::move(result));
}

ValuePtr factorial(unsigned long n) {
    mpz_class result;
    mpz_fac_ui(result.get_mpz_t(), n);
    return Value::make_integer(std::move(result));
}

int cmp(const Integer& a, const Integer& b) {
    return mpz_cmp(a.v.get_mpz_t(), b.v.get_mpz_t());
}

bool is_zero(const Integer& a) { return a.v == 0; }
bool is_negative(const Integer& a) { return a.v < 0; }
bool is_positive(const Integer& a) { return a.v > 0; }
bool is_even(const Integer& a) { return mpz_even_p(a.v.get_mpz_t()); }

} // namespace sc::integer
