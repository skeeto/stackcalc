#include "mpfr_bridge.hpp"
#include "decimal_float.hpp"
#include "fraction.hpp"
#include "coercion.hpp"
#include <cmath>
#include <cstdlib>

namespace sc::mpfr_bridge {

mpfr_prec_t decimal_to_binary_prec(int decimal_prec) {
    // bits = ceil(decimal_prec * log2(10)) + guard bits
    return static_cast<mpfr_prec_t>(std::ceil(decimal_prec * 3.3219281) + 16);
}

static void set_from_decimal_float(mpfr_t out, const DecimalFloat& df, mpfr_prec_t bits) {
    mpfr_set_z(out, df.mantissa.get_mpz_t(), MPFR_RNDN);
    if (df.exponent != 0) {
        mpfr_t scale;
        mpfr_init2(scale, bits);
        if (df.exponent > 0) {
            mpfr_ui_pow_ui(scale, 10, static_cast<unsigned long>(df.exponent), MPFR_RNDN);
            mpfr_mul(out, out, scale, MPFR_RNDN);
        } else {
            mpfr_ui_pow_ui(scale, 10, static_cast<unsigned long>(-df.exponent), MPFR_RNDN);
            mpfr_div(out, out, scale, MPFR_RNDN);
        }
        mpfr_clear(scale);
    }
}

void to_mpfr(mpfr_t out, const ValuePtr& v, int precision) {
    mpfr_prec_t bits = decimal_to_binary_prec(precision);
    mpfr_init2(out, bits);

    if (v->is_integer()) {
        mpfr_set_z(out, v->as_integer().v.get_mpz_t(), MPFR_RNDN);
    } else if (v->is_fraction()) {
        auto& f = v->as_fraction();
        mpfr_t num, den;
        mpfr_init2(num, bits);
        mpfr_init2(den, bits);
        mpfr_set_z(num, f.num.get_mpz_t(), MPFR_RNDN);
        mpfr_set_z(den, f.den.get_mpz_t(), MPFR_RNDN);
        mpfr_div(out, num, den, MPFR_RNDN);
        mpfr_clear(num);
        mpfr_clear(den);
    } else if (v->is_float()) {
        set_from_decimal_float(out, v->as_float(), bits);
    } else {
        // Try to coerce to float first
        auto fv = coerce::to_float(v, precision);
        set_from_decimal_float(out, fv->as_float(), bits);
    }
}

ValuePtr from_mpfr(const mpfr_t x, int precision) {
    if (mpfr_nan_p(x)) {
        return Value::make_infinity(Infinity::NaN);
    }
    if (mpfr_inf_p(x)) {
        return Value::make_infinity(mpfr_sgn(x) > 0 ? Infinity::Pos : Infinity::Neg);
    }
    if (mpfr_zero_p(x)) {
        return Value::make_float(mpz_class(0), 0);
    }

    // Convert to string in decimal, then parse mantissa/exponent
    // mpfr_get_str returns digits + exponent where value = 0.digits * 10^exp
    mpfr_exp_t exp;
    char* str = mpfr_get_str(nullptr, &exp, 10, precision, x, MPFR_RNDN);
    if (!str) {
        return Value::make_float(mpz_class(0), 0);
    }

    // str is like "-12345" or "12345", exp is the exponent such that
    // value = 0.str * 10^exp
    mpz_class mantissa(str);
    mpfr_free_str(str);

    // value = mantissa * 10^(exp - num_digits_in_mantissa)
    int ndigits = decimal_float::num_digits(mantissa);
    int exponent = static_cast<int>(exp) - ndigits;

    return Value::make_float_normalized(std::move(mantissa), exponent, precision);
}

ValuePtr compute_unary(const ValuePtr& a, int precision,
                       int (*mpfr_fn)(mpfr_t, const mpfr_t, mpfr_rnd_t)) {
    mpfr_prec_t bits = decimal_to_binary_prec(precision);
    mpfr_t ma, result;
    to_mpfr(ma, a, precision);
    mpfr_init2(result, bits);
    mpfr_fn(result, ma, MPFR_RNDN);
    auto r = from_mpfr(result, precision);
    mpfr_clear(ma);
    mpfr_clear(result);
    return r;
}

ValuePtr compute_binary(const ValuePtr& a, const ValuePtr& b, int precision,
                        int (*mpfr_fn)(mpfr_t, const mpfr_t, const mpfr_t, mpfr_rnd_t)) {
    mpfr_prec_t bits = decimal_to_binary_prec(precision);
    mpfr_t ma, mb, result;
    to_mpfr(ma, a, precision);
    to_mpfr(mb, b, precision);
    mpfr_init2(result, bits);
    mpfr_fn(result, ma, mb, MPFR_RNDN);
    auto r = from_mpfr(result, precision);
    mpfr_clear(ma);
    mpfr_clear(mb);
    mpfr_clear(result);
    return r;
}

} // namespace sc::mpfr_bridge
