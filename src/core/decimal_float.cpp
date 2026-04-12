#include "decimal_float.h"
#include <stdexcept>
#include <algorithm>

namespace sc::decimal_float {

int num_digits(const mpz_class& n) {
    if (n == 0) return 0;
    mpz_class a = ::abs(n);
    // mpz_sizeinbase can overestimate by 1 for base 10
    size_t est = mpz_sizeinbase(a.get_mpz_t(), 10);
    // Verify: if 10^(est-1) > a, then est was overestimated
    mpz_class threshold;
    mpz_ui_pow_ui(threshold.get_mpz_t(), 10, est - 1);
    if (a < threshold) return static_cast<int>(est - 1);
    return static_cast<int>(est);
}

// Align two floats to the same exponent (the smaller one), returning
// their aligned mantissas and the common exponent.
static void align(const DecimalFloat& a, const DecimalFloat& b,
                  mpz_class& ma, mpz_class& mb, int& exp) {
    if (a.exponent < b.exponent) {
        exp = a.exponent;
        ma = a.mantissa;
        mpz_class scale;
        mpz_ui_pow_ui(scale.get_mpz_t(), 10, b.exponent - a.exponent);
        mb = b.mantissa * scale;
    } else if (b.exponent < a.exponent) {
        exp = b.exponent;
        mb = b.mantissa;
        mpz_class scale;
        mpz_ui_pow_ui(scale.get_mpz_t(), 10, a.exponent - b.exponent);
        ma = a.mantissa * scale;
    } else {
        exp = a.exponent;
        ma = a.mantissa;
        mb = b.mantissa;
    }
}

ValuePtr add(const DecimalFloat& a, const DecimalFloat& b, int precision) {
    mpz_class ma, mb;
    int exp;
    align(a, b, ma, mb, exp);
    return Value::make_float_normalized(ma + mb, exp, precision);
}

ValuePtr sub(const DecimalFloat& a, const DecimalFloat& b, int precision) {
    mpz_class ma, mb;
    int exp;
    align(a, b, ma, mb, exp);
    return Value::make_float_normalized(ma - mb, exp, precision);
}

ValuePtr mul(const DecimalFloat& a, const DecimalFloat& b, int precision) {
    return Value::make_float_normalized(
        a.mantissa * b.mantissa,
        a.exponent + b.exponent,
        precision
    );
}

ValuePtr div(const DecimalFloat& a, const DecimalFloat& b, int precision) {
    if (b.mantissa == 0) throw std::domain_error("division by zero");

    // We need enough digits: compute a.mantissa * 10^(precision + guard) / b.mantissa
    int guard = 2;
    mpz_class scale;
    mpz_ui_pow_ui(scale.get_mpz_t(), 10, precision + guard);
    mpz_class scaled = a.mantissa * scale;

    // Round half away from zero
    mpz_class abs_scaled = ::abs(scaled);
    mpz_class abs_b = ::abs(b.mantissa);
    mpz_class half_b = abs_b / 2;
    mpz_class numerator = abs_scaled + half_b;
    mpz_class q;
    mpz_tdiv_q(q.get_mpz_t(), numerator.get_mpz_t(), abs_b.get_mpz_t());

    // Fix sign
    bool neg = (scaled < 0) != (b.mantissa < 0);
    if (neg) q = -q;

    int new_exp = a.exponent - b.exponent - precision - guard;
    return Value::make_float_normalized(std::move(q), new_exp, precision);
}

ValuePtr neg(const DecimalFloat& a) {
    return Value::make_float(-a.mantissa, a.exponent);
}

ValuePtr abs(const DecimalFloat& a) {
    return Value::make_float(::abs(a.mantissa), a.exponent);
}

// Helper: compute the full integer mantissa * 10^exponent as an mpz,
// or shift to get integer + fractional info
static mpz_class full_value(const DecimalFloat& a, int extra_digits = 0) {
    if (a.exponent >= 0) {
        mpz_class scale;
        mpz_ui_pow_ui(scale.get_mpz_t(), 10, a.exponent + extra_digits);
        return a.mantissa * scale;
    } else {
        int shift = -a.exponent - extra_digits;
        if (shift <= 0) {
            mpz_class scale;
            mpz_ui_pow_ui(scale.get_mpz_t(), 10, -shift);
            return a.mantissa * scale;
        }
        // Would lose fractional part — caller must handle
        return mpz_class(0); // sentinel
    }
}

ValuePtr floor(const DecimalFloat& a) {
    if (a.mantissa == 0) return Value::make_integer(0);
    if (a.exponent >= 0) {
        // Already integer
        mpz_class scale;
        mpz_ui_pow_ui(scale.get_mpz_t(), 10, a.exponent);
        return Value::make_integer(a.mantissa * scale);
    }
    // Negative exponent: divide mantissa by 10^(-exponent)
    mpz_class scale;
    mpz_ui_pow_ui(scale.get_mpz_t(), 10, -a.exponent);
    mpz_class q;
    mpz_fdiv_q(q.get_mpz_t(), a.mantissa.get_mpz_t(), scale.get_mpz_t());
    return Value::make_integer(std::move(q));
}

ValuePtr ceil(const DecimalFloat& a) {
    if (a.mantissa == 0) return Value::make_integer(0);
    if (a.exponent >= 0) {
        mpz_class scale;
        mpz_ui_pow_ui(scale.get_mpz_t(), 10, a.exponent);
        return Value::make_integer(a.mantissa * scale);
    }
    mpz_class scale;
    mpz_ui_pow_ui(scale.get_mpz_t(), 10, -a.exponent);
    mpz_class q;
    mpz_cdiv_q(q.get_mpz_t(), a.mantissa.get_mpz_t(), scale.get_mpz_t());
    return Value::make_integer(std::move(q));
}

ValuePtr round(const DecimalFloat& a) {
    // Round half away from zero
    if (a.mantissa == 0) return Value::make_integer(0);
    if (a.exponent >= 0) {
        mpz_class scale;
        mpz_ui_pow_ui(scale.get_mpz_t(), 10, a.exponent);
        return Value::make_integer(a.mantissa * scale);
    }
    mpz_class scale;
    mpz_ui_pow_ui(scale.get_mpz_t(), 10, -a.exponent);
    mpz_class half = scale / 2;

    mpz_class q;
    if (a.mantissa >= 0) {
        mpz_class numer = a.mantissa + half;
        mpz_tdiv_q(q.get_mpz_t(), numer.get_mpz_t(), scale.get_mpz_t());
    } else {
        mpz_class numer = a.mantissa - half;
        mpz_tdiv_q(q.get_mpz_t(), numer.get_mpz_t(), scale.get_mpz_t());
    }
    return Value::make_integer(std::move(q));
}

ValuePtr trunc(const DecimalFloat& a) {
    if (a.mantissa == 0) return Value::make_integer(0);
    if (a.exponent >= 0) {
        mpz_class scale;
        mpz_ui_pow_ui(scale.get_mpz_t(), 10, a.exponent);
        return Value::make_integer(a.mantissa * scale);
    }
    mpz_class scale;
    mpz_ui_pow_ui(scale.get_mpz_t(), 10, -a.exponent);
    mpz_class q;
    mpz_tdiv_q(q.get_mpz_t(), a.mantissa.get_mpz_t(), scale.get_mpz_t());
    return Value::make_integer(std::move(q));
}

ValuePtr fdiv(const DecimalFloat& a, const DecimalFloat& b, int precision) {
    // floor(a / b)
    auto quotient = div(a, b, precision);
    return floor(quotient->as_float());
}

ValuePtr mod(const DecimalFloat& a, const DecimalFloat& b, int precision) {
    // a - b * floor(a/b)
    auto q = fdiv(a, b, precision);
    // q is Integer; convert to float, multiply by b, subtract from a
    DecimalFloat qf = from_integer(q->as_integer());
    auto bq = mul(b, qf, precision);
    return sub(a, bq->as_float(), precision);
}

ValuePtr pow_int(const DecimalFloat& base, long exp, int precision) {
    if (exp == 0) return Value::make_float(mpz_class(1), 0);
    if (exp < 0) {
        auto pos = pow_int(base, -exp, precision);
        DecimalFloat one_f{mpz_class(1), 0};
        return div(one_f, pos->as_float(), precision);
    }
    // Binary exponentiation
    DecimalFloat result{mpz_class(1), 0};
    DecimalFloat b = base;
    long e = exp;
    while (e > 0) {
        if (e & 1) {
            auto r = mul(result, b, precision);
            result = r->as_float();
        }
        e >>= 1;
        if (e > 0) {
            auto r = mul(b, b, precision);
            b = r->as_float();
        }
    }
    return Value::make_float_normalized(result.mantissa, result.exponent, precision);
}

ValuePtr sqrt(const DecimalFloat& a, int precision) {
    if (a.mantissa < 0) throw std::domain_error("square root of negative number");
    if (a.mantissa == 0) return Value::make_float(mpz_class(0), 0);

    // Newton's method in decimal:
    // Scale mantissa so we work with 2*precision+2 digits
    int work_digits = 2 * precision + 2;
    mpz_class scaled = ::abs(a.mantissa);
    int scaled_exp = a.exponent;

    // Make exponent even by adjusting
    int total_exp = scaled_exp;
    // We want the mantissa to have work_digits*2 digits, and exponent even
    int cur_digits = num_digits(scaled);
    int need = work_digits * 2 - cur_digits;
    if (need > 0) {
        mpz_class s;
        mpz_ui_pow_ui(s.get_mpz_t(), 10, need);
        scaled *= s;
        total_exp -= need;
    }
    // Make total_exp even
    if (total_exp % 2 != 0) {
        scaled *= 10;
        total_exp -= 1;
    }

    // Integer sqrt
    mpz_class root;
    mpz_sqrt(root.get_mpz_t(), scaled.get_mpz_t());

    return Value::make_float_normalized(std::move(root), total_exp / 2, precision);
}

ValuePtr reciprocal(const DecimalFloat& a, int precision) {
    DecimalFloat one{mpz_class(1), 0};
    return div(one, a, precision);
}

DecimalFloat from_integer(const Integer& i) {
    return DecimalFloat{i.v, 0};
}

DecimalFloat from_fraction(const Fraction& f, int precision) {
    if (f.num == 0) return DecimalFloat{mpz_class(0), 0};

    mpz_class scale;
    mpz_ui_pow_ui(scale.get_mpz_t(), 10, precision);
    mpz_class scaled_num = f.num * scale;

    mpz_class abs_num = ::abs(scaled_num);
    mpz_class half_den = f.den / 2;
    mpz_class numerator = abs_num + half_den;
    mpz_class q;
    mpz_tdiv_q(q.get_mpz_t(), numerator.get_mpz_t(), f.den.get_mpz_t());
    if (scaled_num < 0) q = -q;

    // Strip trailing zeros
    int exp = -precision;
    while (q != 0 && mpz_divisible_ui_p(q.get_mpz_t(), 10)) {
        q /= 10;
        exp++;
    }
    return DecimalFloat{std::move(q), exp};
}

std::optional<mpz_class> to_integer(const DecimalFloat& f) {
    if (f.mantissa == 0) return mpz_class(0);
    if (f.exponent >= 0) {
        mpz_class scale;
        mpz_ui_pow_ui(scale.get_mpz_t(), 10, f.exponent);
        return f.mantissa * scale;
    }
    // Check if all fractional digits are zero
    mpz_class scale;
    mpz_ui_pow_ui(scale.get_mpz_t(), 10, -f.exponent);
    if (mpz_divisible_p(f.mantissa.get_mpz_t(), scale.get_mpz_t())) {
        return f.mantissa / scale;
    }
    return std::nullopt;
}

int cmp(const DecimalFloat& a, const DecimalFloat& b) {
    // Align and compare
    mpz_class ma, mb;
    int exp;
    align(a, b, ma, mb, exp);
    return mpz_cmp(ma.get_mpz_t(), mb.get_mpz_t());
}

bool is_zero(const DecimalFloat& a) { return a.mantissa == 0; }
bool is_negative(const DecimalFloat& a) { return a.mantissa < 0; }

} // namespace sc::decimal_float
