#include "integer.hpp"
#include <climits>
#include <cstdint>
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

ValuePtr pow(const Integer& base, std::uint64_t exp) {
    // Fast path: when exp fits in unsigned long, defer to GMP's optimised
    // mpz_pow_ui. This is always taken on LP64 (long is 64-bit) and on
    // LLP64 (Windows) for exp <= 2^32-1. Above that on LLP64 we fall
    // back to a manual exponentiation-by-squaring loop using only mpz
    // multiplications, which take any width of exponent.
    if (exp <= static_cast<std::uint64_t>(ULONG_MAX)) {
        mpz_class result;
        mpz_pow_ui(result.get_mpz_t(), base.v.get_mpz_t(),
                   static_cast<unsigned long>(exp));
        return Value::make_integer(std::move(result));
    }
    mpz_class result(1);
    mpz_class b = base.v;
    while (exp > 0) {
        if (exp & 1) result *= b;
        exp >>= 1;
        if (exp > 0) b *= b;
    }
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

ValuePtr factorial(std::uint64_t n) {
    if (n <= static_cast<std::uint64_t>(ULONG_MAX)) {
        mpz_class result;
        mpz_fac_ui(result.get_mpz_t(), static_cast<unsigned long>(n));
        return Value::make_integer(std::move(result));
    }
    // Manual loop for n > ULONG_MAX (only reachable on LLP64). The
    // result is astronomical and the caller will likely OOM long
    // before this finishes — but the behaviour matches LP64 instead
    // of erroring out at a different boundary per platform.
    mpz_class result(1);
    for (std::uint64_t i = 2; i <= n; ++i) {
        // Multiply by i in ULONG-sized chunks (only the high bits ever
        // exceed ULONG_MAX on LLP64; here i is the loop counter so it's
        // always <= n, but on LLP64 i may itself exceed ULONG_MAX).
        if (i <= static_cast<std::uint64_t>(ULONG_MAX)) {
            result *= mpz_class(static_cast<unsigned long>(i));
        } else {
            mpz_class big;
            mpz_set_ui(big.get_mpz_t(),
                       static_cast<unsigned long>(i >> 32));
            mpz_mul_2exp(big.get_mpz_t(), big.get_mpz_t(), 32);
            mpz_add_ui(big.get_mpz_t(), big.get_mpz_t(),
                       static_cast<unsigned long>(i & 0xFFFFFFFFu));
            result *= big;
        }
    }
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
