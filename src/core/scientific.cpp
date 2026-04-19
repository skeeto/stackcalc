#include "scientific.h"
#include "mpfr_bridge.h"
#include "constants.h"
#include "arithmetic.h"
#include "integer.h"
#include <chrono>
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

// --- Integer logarithm ---

ValuePtr ilog(const ValuePtr& a, const ValuePtr& base) {
    // Emacs calc semantics: floor(log_base(a)) for positive integers.
    if (!a->is_integer() || !base->is_integer())
        throw std::domain_error("ilog requires integer arguments");
    auto& av = a->as_integer().v;
    auto& bv = base->as_integer().v;
    if (av <= 0) throw std::domain_error("ilog of non-positive value");
    if (bv < 2) throw std::domain_error("ilog base must be >= 2");
    // Repeated division: count how many times we can divide a by base before
    // it drops below 1.
    mpz_class x = av, count = 0;
    while (x >= bv) { x /= bv; count += 1; }
    return Value::make_integer(std::move(count));
}

// --- Gamma function ---

ValuePtr gamma_fn(const ValuePtr& a, int precision) {
    return mpfr_bridge::compute_unary(a, precision, mpfr_gamma);
}

// --- Number theory ---

static const mpz_class& require_positive_int(const ValuePtr& v, const char* name) {
    if (!v->is_integer()) throw std::domain_error(std::string(name) + " requires integer");
    auto& val = v->as_integer().v;
    if (val <= 0) throw std::domain_error(std::string(name) + " requires positive integer");
    return val;
}

ValuePtr gcd(const ValuePtr& a, const ValuePtr& b) {
    if (!a->is_integer() || !b->is_integer())
        throw std::domain_error("gcd requires integers");
    return integer::gcd(a->as_integer(), b->as_integer());
}

ValuePtr lcm(const ValuePtr& a, const ValuePtr& b) {
    if (!a->is_integer() || !b->is_integer())
        throw std::domain_error("lcm requires integers");
    return integer::lcm(a->as_integer(), b->as_integer());
}

ValuePtr next_prime(const ValuePtr& a) {
    if (!a->is_integer()) throw std::domain_error("next-prime requires integer");
    mpz_class result;
    mpz_nextprime(result.get_mpz_t(), a->as_integer().v.get_mpz_t());
    return Value::make_integer(std::move(result));
}

ValuePtr prev_prime(const ValuePtr& a) {
    if (!a->is_integer()) throw std::domain_error("prev-prime requires integer");
    auto& v = a->as_integer().v;
    if (v <= 2) throw std::domain_error("no prime less than 2");
    // Search backward from v-1
    mpz_class candidate = v - 1;
    while (mpz_probab_prime_p(candidate.get_mpz_t(), 25) == 0) {
        candidate -= 1;
        if (candidate < 2) throw std::domain_error("no prime found");
    }
    return Value::make_integer(std::move(candidate));
}

ValuePtr prime_test(const ValuePtr& a) {
    if (!a->is_integer()) throw std::domain_error("prime-test requires integer");
    int result = mpz_probab_prime_p(a->as_integer().v.get_mpz_t(), 25);
    return Value::make_integer(mpz_class(result));
}

ValuePtr totient(const ValuePtr& a) {
    auto& n = require_positive_int(a, "totient");
    if (n == 1) return Value::one();
    // Factor n, then phi(n) = n * prod(1 - 1/p) for each prime factor p
    mpz_class result = n;
    mpz_class temp = n;
    mpz_class p = 2;
    while (p * p <= temp) {
        if (mpz_divisible_p(temp.get_mpz_t(), p.get_mpz_t())) {
            while (mpz_divisible_p(temp.get_mpz_t(), p.get_mpz_t()))
                mpz_divexact(temp.get_mpz_t(), temp.get_mpz_t(), p.get_mpz_t());
            result -= result / p;
        }
        p += 1;
    }
    if (temp > 1) {
        result -= result / temp;
    }
    return Value::make_integer(std::move(result));
}

ValuePtr prime_factors(const ValuePtr& a) {
    auto& n = require_positive_int(a, "prime-factors");
    if (n == 1) return Value::make_vector({});

    std::vector<ValuePtr> pairs;
    mpz_class temp = n;
    mpz_class p = 2;
    while (p * p <= temp) {
        if (mpz_divisible_p(temp.get_mpz_t(), p.get_mpz_t())) {
            int count = 0;
            while (mpz_divisible_p(temp.get_mpz_t(), p.get_mpz_t())) {
                mpz_divexact(temp.get_mpz_t(), temp.get_mpz_t(), p.get_mpz_t());
                count++;
            }
            pairs.push_back(Value::make_vector({
                Value::make_integer(mpz_class(p)),
                Value::make_integer(mpz_class(count))
            }));
        }
        p += 1;
    }
    if (temp > 1) {
        pairs.push_back(Value::make_vector({
            Value::make_integer(mpz_class(temp)),
            Value::one()
        }));
    }
    return Value::make_vector(std::move(pairs));
}

ValuePtr random(const ValuePtr& n) {
    if (!n->is_integer()) throw std::domain_error("random requires integer");
    auto& v = n->as_integer().v;
    if (v <= 0) throw std::domain_error("random requires positive bound");
    // Thread-local GMP random state
    static thread_local gmp_randstate_t state;
    static thread_local bool initialized = false;
    if (!initialized) {
        gmp_randinit_mt(state);
        gmp_randseed_ui(state, static_cast<unsigned long>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        initialized = true;
    }
    mpz_class result;
    mpz_urandomm(result.get_mpz_t(), state, v.get_mpz_t());
    return Value::make_integer(std::move(result));
}

ValuePtr extended_gcd(const ValuePtr& a, const ValuePtr& b) {
    if (!a->is_integer() || !b->is_integer())
        throw std::domain_error("extended-gcd requires integers");
    mpz_class g, s, t;
    mpz_gcdext(g.get_mpz_t(), s.get_mpz_t(), t.get_mpz_t(),
               a->as_integer().v.get_mpz_t(), b->as_integer().v.get_mpz_t());
    return Value::make_vector({
        Value::make_integer(std::move(g)),
        Value::make_integer(std::move(s)),
        Value::make_integer(std::move(t))
    });
}

ValuePtr mod_pow(const ValuePtr& base, const ValuePtr& exp, const ValuePtr& m) {
    if (!base->is_integer() || !exp->is_integer() || !m->is_integer())
        throw std::domain_error("mod-pow requires integers");
    auto& mv = m->as_integer().v;
    if (mv <= 0) throw std::domain_error("mod-pow requires positive modulus");
    mpz_class result;
    mpz_powm(result.get_mpz_t(),
             base->as_integer().v.get_mpz_t(),
             exp->as_integer().v.get_mpz_t(),
             mv.get_mpz_t());
    return Value::make_integer(std::move(result));
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
