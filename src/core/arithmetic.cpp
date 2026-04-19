#include "arithmetic.h"
#include "integer.h"
#include "fraction.h"
#include "decimal_float.h"
#include "coercion.h"
#include "complex.h"
#include "hms.h"
#include "error_form.h"
#include "modulo_form.h"
#include "interval.h"
#include "infinity.h"
#include "vector.h"
#include "mpfr_bridge.h"
#include <mpfr.h>

namespace sc::arith {

// Helper: get Fraction from a rational Value
static Fraction to_frac(const ValuePtr& v) {
    if (v->is_integer()) return fraction::from_integer(v->as_integer());
    return v->as_fraction();
}

// --- Real arithmetic (int/frac/float) ---

static ValuePtr real_add(const ValuePtr& a, const ValuePtr& b, int precision) {
    auto [ca, cb, tag] = coerce::coerce_pair(a, b, precision);
    switch (tag) {
        case Tag::Integer:
            return integer::add(ca->as_integer(), cb->as_integer());
        case Tag::Fraction:
            return fraction::add(to_frac(ca), to_frac(cb));
        case Tag::DecimalFloat:
            return decimal_float::add(ca->as_float(), cb->as_float(), precision);
        default:
            throw std::invalid_argument("unsupported types for addition");
    }
}

static ValuePtr real_sub(const ValuePtr& a, const ValuePtr& b, int precision) {
    auto [ca, cb, tag] = coerce::coerce_pair(a, b, precision);
    switch (tag) {
        case Tag::Integer:
            return integer::sub(ca->as_integer(), cb->as_integer());
        case Tag::Fraction:
            return fraction::sub(to_frac(ca), to_frac(cb));
        case Tag::DecimalFloat:
            return decimal_float::sub(ca->as_float(), cb->as_float(), precision);
        default:
            throw std::invalid_argument("unsupported types for subtraction");
    }
}

static ValuePtr real_mul(const ValuePtr& a, const ValuePtr& b, int precision) {
    auto [ca, cb, tag] = coerce::coerce_pair(a, b, precision);
    switch (tag) {
        case Tag::Integer:
            return integer::mul(ca->as_integer(), cb->as_integer());
        case Tag::Fraction:
            return fraction::mul(to_frac(ca), to_frac(cb));
        case Tag::DecimalFloat:
            return decimal_float::mul(ca->as_float(), cb->as_float(), precision);
        default:
            throw std::invalid_argument("unsupported types for multiplication");
    }
}

static ValuePtr real_div(const ValuePtr& a, const ValuePtr& b, int precision,
                         FractionMode frac_mode) {
    if (b->is_zero()) throw std::domain_error("division by zero");

    if (a->is_integer() && b->is_integer()) {
        auto& ai = a->as_integer();
        auto& bi = b->as_integer();
        if (frac_mode == FractionMode::Fraction) {
            return Value::make_fraction(ai.v, bi.v);
        }
        mpz_class r;
        mpz_tdiv_r(r.get_mpz_t(), ai.v.get_mpz_t(), bi.v.get_mpz_t());
        if (r == 0) return Value::make_integer(ai.v / bi.v);
        auto fa = decimal_float::from_integer(ai);
        auto fb = decimal_float::from_integer(bi);
        return decimal_float::div(fa, fb, precision);
    }

    auto [ca, cb, tag] = coerce::coerce_pair(a, b, precision);
    switch (tag) {
        case Tag::Fraction:
            return fraction::div(to_frac(ca), to_frac(cb));
        case Tag::DecimalFloat:
            return decimal_float::div(ca->as_float(), cb->as_float(), precision);
        default:
            throw std::invalid_argument("unsupported types for division");
    }
}

// --- Full dispatch including compound types ---

ValuePtr add(const ValuePtr& a, const ValuePtr& b, int precision) {
    // Vectors
    if (a->is_vector() || b->is_vector()) return vector_ops::map_add(a, b, precision);

    // Infinity
    if (a->is_infinity()) return infinity_ops::add(a->as_infinity(), b);
    if (b->is_infinity()) return infinity_ops::add(a, b->as_infinity());

    // Complex
    if (a->is_complex() || b->is_complex()) {
        auto make_rc = [](const ValuePtr& v) -> RectComplex {
            if (v->tag() == Tag::RectComplex) return v->as_rect_complex();
            return RectComplex{v, Value::zero()};
        };
        return complex::add(make_rc(a), make_rc(b), precision);
    }

    // Error forms
    if (a->is_error() && b->is_error()) return error_form::add(a->as_error(), b->as_error(), precision);
    if (a->is_error()) return error_form::add(a->as_error(), ErrorForm{b, Value::zero()}, precision);
    if (b->is_error()) return error_form::add(ErrorForm{a, Value::zero()}, b->as_error(), precision);

    // Modulo forms
    if (a->is_mod() && b->is_mod()) return modulo_form::add(a->as_mod(), b->as_mod(), precision);

    // Intervals
    if (a->is_interval() && b->is_interval()) return interval::add(a->as_interval(), b->as_interval(), precision);

    // HMS
    if (a->is_hms() && b->is_hms()) return hms::add(a->as_hms(), b->as_hms(), precision);

    // Real arithmetic
    return real_add(a, b, precision);
}

ValuePtr sub(const ValuePtr& a, const ValuePtr& b, int precision) {
    if (a->is_vector() || b->is_vector()) return vector_ops::map_sub(a, b, precision);
    if (a->is_infinity() || b->is_infinity()) return add(a, neg(b), precision);

    if (a->is_complex() || b->is_complex()) {
        auto make_rc = [](const ValuePtr& v) -> RectComplex {
            if (v->tag() == Tag::RectComplex) return v->as_rect_complex();
            return RectComplex{v, Value::zero()};
        };
        return complex::sub(make_rc(a), make_rc(b), precision);
    }

    if (a->is_error() && b->is_error()) return error_form::sub(a->as_error(), b->as_error(), precision);
    if (a->is_error()) return error_form::sub(a->as_error(), ErrorForm{b, Value::zero()}, precision);
    if (b->is_error()) return error_form::sub(ErrorForm{a, Value::zero()}, b->as_error(), precision);

    if (a->is_mod() && b->is_mod()) return modulo_form::sub(a->as_mod(), b->as_mod(), precision);
    if (a->is_interval() && b->is_interval()) return interval::sub(a->as_interval(), b->as_interval(), precision);
    if (a->is_hms() && b->is_hms()) return hms::sub(a->as_hms(), b->as_hms(), precision);

    return real_sub(a, b, precision);
}

ValuePtr mul(const ValuePtr& a, const ValuePtr& b, int precision) {
    if (a->is_vector() || b->is_vector()) return vector_ops::map_mul(a, b, precision);
    if (a->is_infinity()) return infinity_ops::mul(a->as_infinity(), b);
    if (b->is_infinity()) return infinity_ops::mul(b->as_infinity(), a);

    if (a->is_complex() || b->is_complex()) {
        auto make_rc = [](const ValuePtr& v) -> RectComplex {
            if (v->tag() == Tag::RectComplex) return v->as_rect_complex();
            return RectComplex{v, Value::zero()};
        };
        return complex::mul(make_rc(a), make_rc(b), precision);
    }

    if (a->is_error() && b->is_error()) return error_form::mul(a->as_error(), b->as_error(), precision);
    if (a->is_error()) return error_form::mul(a->as_error(), ErrorForm{b, Value::zero()}, precision);
    if (b->is_error()) return error_form::mul(ErrorForm{a, Value::zero()}, b->as_error(), precision);

    if (a->is_mod() && b->is_mod()) return modulo_form::mul(a->as_mod(), b->as_mod(), precision);
    if (a->is_interval() && b->is_interval()) return interval::mul(a->as_interval(), b->as_interval(), precision);

    // HMS * scalar
    if (a->is_hms() && b->is_real()) return hms::mul_scalar(a->as_hms(), b, precision);
    if (b->is_hms() && a->is_real()) return hms::mul_scalar(b->as_hms(), a, precision);

    return real_mul(a, b, precision);
}

ValuePtr div(const ValuePtr& a, const ValuePtr& b, int precision, FractionMode frac_mode) {
    if (a->is_vector() || b->is_vector()) return vector_ops::map_div(a, b, precision, frac_mode);
    if (a->is_complex() || b->is_complex()) {
        auto make_rc = [](const ValuePtr& v) -> RectComplex {
            if (v->tag() == Tag::RectComplex) return v->as_rect_complex();
            return RectComplex{v, Value::zero()};
        };
        return complex::div(make_rc(a), make_rc(b), precision);
    }

    if (a->is_error() && b->is_error()) return error_form::div(a->as_error(), b->as_error(), precision);
    if (a->is_error()) return error_form::div(a->as_error(), ErrorForm{b, Value::zero()}, precision);
    if (b->is_error()) return error_form::div(ErrorForm{a, Value::zero()}, b->as_error(), precision);

    if (a->is_interval() && b->is_interval()) return interval::div(a->as_interval(), b->as_interval(), precision);

    // HMS / scalar
    if (a->is_hms() && b->is_real()) return hms::div_scalar(a->as_hms(), b, precision);

    return real_div(a, b, precision, frac_mode);
}

ValuePtr idiv(const ValuePtr& a, const ValuePtr& b, int precision) {
    if (b->is_zero()) throw std::domain_error("division by zero");
    if (a->is_integer() && b->is_integer()) return integer::fdiv(a->as_integer(), b->as_integer());
    auto [ca, cb, tag] = coerce::coerce_pair(a, b, precision);
    switch (tag) {
        case Tag::Fraction: return fraction::fdiv(to_frac(ca), to_frac(cb));
        case Tag::DecimalFloat: return decimal_float::fdiv(ca->as_float(), cb->as_float(), precision);
        default: throw std::invalid_argument("unsupported types for integer division");
    }
}

ValuePtr mod(const ValuePtr& a, const ValuePtr& b, int precision) {
    if (b->is_zero()) throw std::domain_error("division by zero");
    if (a->is_integer() && b->is_integer()) return integer::mod(a->as_integer(), b->as_integer());
    auto [ca, cb, tag] = coerce::coerce_pair(a, b, precision);
    switch (tag) {
        case Tag::Fraction: return fraction::mod(to_frac(ca), to_frac(cb));
        case Tag::DecimalFloat: return decimal_float::mod(ca->as_float(), cb->as_float(), precision);
        default: throw std::invalid_argument("unsupported types for modulo");
    }
}

ValuePtr power(const ValuePtr& a, const ValuePtr& b, int precision) {
    if (a->is_mod()) return modulo_form::pow(a->as_mod(), b, precision);

    // Coerce a float exponent to an integer if it represents one exactly.
    // Otherwise (true non-integer real exponent), fall back to MPFR's mpfr_pow,
    // which handles fraction/float exponents and signals NaN for things like
    // negative-base ^ non-integer (which would be complex).
    const mpz_class* exp_mpz = nullptr;
    mpz_class exp_storage;
    if (b->is_integer()) {
        exp_mpz = &b->as_integer().v;
    } else if (b->is_float()) {
        if (auto i = decimal_float::to_integer(b->as_float())) {
            exp_storage = std::move(*i);
            exp_mpz = &exp_storage;
        }
    }
    if (!exp_mpz) {
        // Non-integer real exponent. Requires both base and exponent to be
        // convertible to a real number (mpfr_bridge::to_mpfr will throw for
        // complex/HMS/etc. via coerce::to_float).
        return mpfr_bridge::compute_binary(a, b, precision, mpfr_pow);
    }

    // Special cases that have well-defined results regardless of exponent
    // magnitude — avoid the get_si() overflow trap for them.
    if (a->is_integer()) {
        const auto& base = a->as_integer().v;
        if (base == 0) {
            if (*exp_mpz < 0) throw std::domain_error("0 raised to negative power");
            if (*exp_mpz == 0) return Value::one();
            return Value::zero();
        }
        if (base == 1) return Value::one();
        if (base == -1) {
            return mpz_even_p(exp_mpz->get_mpz_t())
                ? Value::one()
                : Value::make_integer(mpz_class(-1));
        }
    }

    // Detect exponent overflow before truncating to a signed long. mpz's
    // get_si() silently wraps modulo 2^64 — without this check, e.g.
    // 16^(2^64) would compute 16^0 = 1 because get_si(2^64) returns 0.
    if (!exp_mpz->fits_slong_p()) {
        throw std::overflow_error("exponent too large to compute");
    }
    long e = exp_mpz->get_si();

    if (a->is_integer()) {
        if (e >= 0) return integer::pow(a->as_integer(), static_cast<unsigned long>(e));
        auto pos = integer::pow(a->as_integer(), static_cast<unsigned long>(-e));
        return Value::make_fraction(mpz_class(1), pos->as_integer().v);
    }
    if (a->is_fraction()) {
        auto& f = a->as_fraction();
        if (e >= 0) {
            mpz_class rn, rd;
            mpz_pow_ui(rn.get_mpz_t(), f.num.get_mpz_t(), static_cast<unsigned long>(e));
            mpz_pow_ui(rd.get_mpz_t(), f.den.get_mpz_t(), static_cast<unsigned long>(e));
            return Value::make_fraction(std::move(rn), std::move(rd));
        }
        mpz_class rn, rd;
        mpz_pow_ui(rn.get_mpz_t(), f.den.get_mpz_t(), static_cast<unsigned long>(-e));
        mpz_pow_ui(rd.get_mpz_t(), f.num.get_mpz_t(), static_cast<unsigned long>(-e));
        return Value::make_fraction(std::move(rn), std::move(rd));
    }
    if (a->is_float()) return decimal_float::pow_int(a->as_float(), e, precision);
    throw std::invalid_argument("unsupported type for power");
}

ValuePtr neg(const ValuePtr& a) {
    switch (a->tag()) {
        case Tag::Integer: return integer::neg(a->as_integer());
        case Tag::Fraction: return fraction::neg(a->as_fraction());
        case Tag::DecimalFloat: return decimal_float::neg(a->as_float());
        case Tag::RectComplex: return complex::neg(a->as_rect_complex());
        case Tag::HMS: return hms::neg(a->as_hms());
        case Tag::ErrorForm: return error_form::neg(a->as_error());
        case Tag::ModuloForm: return modulo_form::neg(a->as_mod(), 12); // precision doesn't matter for neg
        case Tag::Interval: return interval::neg(a->as_interval());
        case Tag::Infinity: return infinity_ops::neg(a->as_infinity());
        case Tag::Vector: return vector_ops::map_neg(a->as_vector());
        default: throw std::invalid_argument("unsupported type for negation");
    }
}

ValuePtr inv(const ValuePtr& a, int precision) {
    if (a->is_zero()) throw std::domain_error("reciprocal of zero");
    switch (a->tag()) {
        case Tag::Integer: return Value::make_fraction(mpz_class(1), a->as_integer().v);
        case Tag::Fraction: return fraction::reciprocal(a->as_fraction());
        case Tag::DecimalFloat: return decimal_float::reciprocal(a->as_float(), precision);
        default: throw std::invalid_argument("unsupported type for reciprocal");
    }
}

ValuePtr sqrt(const ValuePtr& a, int precision) {
    auto f = coerce::to_float(a, precision);
    return decimal_float::sqrt(f->as_float(), precision);
}

ValuePtr abs(const ValuePtr& a) {
    switch (a->tag()) {
        case Tag::Integer: return integer::abs(a->as_integer());
        case Tag::Fraction: return fraction::abs(a->as_fraction());
        case Tag::DecimalFloat: return decimal_float::abs(a->as_float());
        case Tag::ErrorForm: return error_form::abs(a->as_error(), 12);
        default: throw std::invalid_argument("unsupported type for absolute value");
    }
}

ValuePtr floor(const ValuePtr& a) {
    switch (a->tag()) {
        case Tag::Integer: return a;
        case Tag::Fraction: {
            auto& f = a->as_fraction();
            mpz_class q;
            mpz_fdiv_q(q.get_mpz_t(), f.num.get_mpz_t(), f.den.get_mpz_t());
            return Value::make_integer(std::move(q));
        }
        case Tag::DecimalFloat: return decimal_float::floor(a->as_float());
        default: throw std::invalid_argument("unsupported type for floor");
    }
}

ValuePtr ceil(const ValuePtr& a) {
    switch (a->tag()) {
        case Tag::Integer: return a;
        case Tag::Fraction: {
            auto& f = a->as_fraction();
            mpz_class q;
            mpz_cdiv_q(q.get_mpz_t(), f.num.get_mpz_t(), f.den.get_mpz_t());
            return Value::make_integer(std::move(q));
        }
        case Tag::DecimalFloat: return decimal_float::ceil(a->as_float());
        default: throw std::invalid_argument("unsupported type for ceiling");
    }
}

ValuePtr round(const ValuePtr& a) {
    switch (a->tag()) {
        case Tag::Integer: return a;
        case Tag::Fraction: {
            auto& f = a->as_fraction();
            mpz_class abs_num = ::abs(f.num);
            mpz_class doubled = abs_num * 2 + f.den;
            mpz_class denom2 = f.den * 2;
            mpz_class q;
            mpz_tdiv_q(q.get_mpz_t(), doubled.get_mpz_t(), denom2.get_mpz_t());
            if (f.num < 0) q = -q;
            return Value::make_integer(std::move(q));
        }
        case Tag::DecimalFloat: return decimal_float::round(a->as_float());
        default: throw std::invalid_argument("unsupported type for round");
    }
}

ValuePtr trunc(const ValuePtr& a) {
    switch (a->tag()) {
        case Tag::Integer: return a;
        case Tag::Fraction: {
            auto& f = a->as_fraction();
            mpz_class q;
            mpz_tdiv_q(q.get_mpz_t(), f.num.get_mpz_t(), f.den.get_mpz_t());
            return Value::make_integer(std::move(q));
        }
        case Tag::DecimalFloat: return decimal_float::trunc(a->as_float());
        default: throw std::invalid_argument("unsupported type for truncate");
    }
}

} // namespace sc::arith
