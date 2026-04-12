#include "scientific.h"
#include "mpfr_bridge.h"
#include "constants.h"
#include "arithmetic.h"
#include "integer.h"
#include <stdexcept>

namespace sc::scientific {

// --- Angle conversion helpers ---

// Convert from user angular mode to radians
static ValuePtr to_radians(const ValuePtr& a, int precision, AngularMode mode) {
    if (mode == AngularMode::Radians) return a;
    // degrees -> radians: a * pi / 180
    auto pi = constants::pi(precision);
    auto num = arith::mul(a, pi, precision);
    return arith::div(num, Value::make_integer(180), precision, FractionMode::Float);
}

// Convert from radians to user angular mode
static ValuePtr from_radians(const ValuePtr& a, int precision, AngularMode mode) {
    if (mode == AngularMode::Radians) return a;
    // radians -> degrees: a * 180 / pi
    auto pi = constants::pi(precision);
    auto num = arith::mul(a, Value::make_integer(180), precision);
    return arith::div(num, pi, precision, FractionMode::Float);
}

// --- Trigonometric ---

ValuePtr sin(const ValuePtr& a, int precision, AngularMode mode) {
    auto rad = to_radians(a, precision, mode);
    return mpfr_bridge::compute_unary(rad, precision, mpfr_sin);
}

ValuePtr cos(const ValuePtr& a, int precision, AngularMode mode) {
    auto rad = to_radians(a, precision, mode);
    return mpfr_bridge::compute_unary(rad, precision, mpfr_cos);
}

ValuePtr tan(const ValuePtr& a, int precision, AngularMode mode) {
    auto rad = to_radians(a, precision, mode);
    return mpfr_bridge::compute_unary(rad, precision, mpfr_tan);
}

ValuePtr arcsin(const ValuePtr& a, int precision, AngularMode mode) {
    auto rad = mpfr_bridge::compute_unary(a, precision, mpfr_asin);
    return from_radians(rad, precision, mode);
}

ValuePtr arccos(const ValuePtr& a, int precision, AngularMode mode) {
    auto rad = mpfr_bridge::compute_unary(a, precision, mpfr_acos);
    return from_radians(rad, precision, mode);
}

ValuePtr arctan(const ValuePtr& a, int precision, AngularMode mode) {
    auto rad = mpfr_bridge::compute_unary(a, precision, mpfr_atan);
    return from_radians(rad, precision, mode);
}

ValuePtr arctan2(const ValuePtr& y, const ValuePtr& x, int precision, AngularMode mode) {
    auto rad = mpfr_bridge::compute_binary(y, x, precision, mpfr_atan2);
    return from_radians(rad, precision, mode);
}

// --- Hyperbolic ---

ValuePtr sinh(const ValuePtr& a, int precision) {
    return mpfr_bridge::compute_unary(a, precision, mpfr_sinh);
}

ValuePtr cosh(const ValuePtr& a, int precision) {
    return mpfr_bridge::compute_unary(a, precision, mpfr_cosh);
}

ValuePtr tanh(const ValuePtr& a, int precision) {
    return mpfr_bridge::compute_unary(a, precision, mpfr_tanh);
}

ValuePtr arcsinh(const ValuePtr& a, int precision) {
    return mpfr_bridge::compute_unary(a, precision, mpfr_asinh);
}

ValuePtr arccosh(const ValuePtr& a, int precision) {
    return mpfr_bridge::compute_unary(a, precision, mpfr_acosh);
}

ValuePtr arctanh(const ValuePtr& a, int precision) {
    return mpfr_bridge::compute_unary(a, precision, mpfr_atanh);
}

// --- Logarithmic ---

ValuePtr ln(const ValuePtr& a, int precision) {
    return mpfr_bridge::compute_unary(a, precision, mpfr_log);
}

ValuePtr exp(const ValuePtr& a, int precision) {
    return mpfr_bridge::compute_unary(a, precision, mpfr_exp);
}

ValuePtr log10(const ValuePtr& a, int precision) {
    return mpfr_bridge::compute_unary(a, precision, mpfr_log10);
}

ValuePtr exp10(const ValuePtr& a, int precision) {
    return mpfr_bridge::compute_unary(a, precision, mpfr_exp10);
}

ValuePtr logb(const ValuePtr& a, const ValuePtr& base, int precision) {
    // log_base(a) = ln(a) / ln(base)
    auto ln_a = ln(a, precision);
    auto ln_b = ln(base, precision);
    return arith::div(ln_a, ln_b, precision, FractionMode::Float);
}

ValuePtr expm1(const ValuePtr& a, int precision) {
    return mpfr_bridge::compute_unary(a, precision, mpfr_expm1);
}

ValuePtr lnp1(const ValuePtr& a, int precision) {
    return mpfr_bridge::compute_unary(a, precision, mpfr_log1p);
}

// --- Combinatorics ---

ValuePtr factorial(const ValuePtr& a, int precision) {
    if (a->is_integer()) {
        auto& v = a->as_integer().v;
        if (v < 0) throw std::domain_error("factorial of negative integer");
        if (!v.fits_ulong_p()) throw std::overflow_error("factorial argument too large");
        unsigned long n = v.get_ui();
        mpz_class result;
        mpz_fac_ui(result.get_mpz_t(), n);
        return Value::make_integer(std::move(result));
    }
    // Non-integer: use gamma function, n! = gamma(n+1)
    auto n_plus_1 = arith::add(a, Value::one(), precision);
    mpfr_prec_t bits = mpfr_bridge::decimal_to_binary_prec(precision);
    mpfr_t mx, result;
    mpfr_bridge::to_mpfr(mx, n_plus_1, precision);
    mpfr_init2(result, bits);
    mpfr_gamma(result, mx, MPFR_RNDN);
    auto r = mpfr_bridge::from_mpfr(result, precision);
    mpfr_clear(mx);
    mpfr_clear(result);
    return r;
}

ValuePtr double_factorial(const ValuePtr& a) {
    if (!a->is_integer()) throw std::domain_error("double factorial requires integer");
    auto& v = a->as_integer().v;
    if (v < 0) throw std::domain_error("double factorial of negative integer");
    if (!v.fits_ulong_p()) throw std::overflow_error("double factorial argument too large");
    unsigned long n = v.get_ui();
    mpz_class result;
    mpz_2fac_ui(result.get_mpz_t(), n);
    return Value::make_integer(std::move(result));
}

ValuePtr choose(const ValuePtr& n, const ValuePtr& m, int precision) {
    if (n->is_integer() && m->is_integer()) {
        auto& nv = n->as_integer().v;
        auto& mv = m->as_integer().v;
        if (mv < 0 || nv < mv) return Value::zero();
        if (!mv.fits_ulong_p()) throw std::overflow_error("choose argument too large");
        unsigned long k = mv.get_ui();
        mpz_class result;
        mpz_bin_ui(result.get_mpz_t(), nv.get_mpz_t(), k);
        return Value::make_integer(std::move(result));
    }
    // General case: n! / (m! * (n-m)!)
    auto n_fact = factorial(n, precision);
    auto m_fact = factorial(m, precision);
    auto nm = arith::sub(n, m, precision);
    auto nm_fact = factorial(nm, precision);
    auto denom = arith::mul(m_fact, nm_fact, precision);
    return arith::div(n_fact, denom, precision, FractionMode::Float);
}

ValuePtr permutation(const ValuePtr& n, const ValuePtr& m, int precision) {
    if (n->is_integer() && m->is_integer()) {
        auto& nv = n->as_integer().v;
        auto& mv = m->as_integer().v;
        if (mv < 0 || nv < mv) return Value::zero();
        if (!mv.fits_ulong_p()) throw std::overflow_error("permutation argument too large");
        unsigned long k = mv.get_ui();
        // n * (n-1) * ... * (n-k+1)
        mpz_class result(1);
        mpz_class cur = nv;
        for (unsigned long i = 0; i < k; ++i) {
            result *= cur;
            cur -= 1;
        }
        return Value::make_integer(std::move(result));
    }
    // General case: n! / (n-m)!
    auto n_fact = factorial(n, precision);
    auto nm = arith::sub(n, m, precision);
    auto nm_fact = factorial(nm, precision);
    return arith::div(n_fact, nm_fact, precision, FractionMode::Float);
}

// --- Other ---

ValuePtr sign(const ValuePtr& a) {
    if (a->is_zero()) return Value::zero();
    if (a->is_negative()) return Value::make_integer(-1);
    return Value::one();
}

ValuePtr hypot(const ValuePtr& a, const ValuePtr& b, int precision) {
    return mpfr_bridge::compute_binary(a, b, precision, mpfr_hypot);
}

} // namespace sc::scientific
