#include "value.h"

namespace sc {

// --- Compound type equality (delegate to Value equality) ---

bool HMS::operator==(const HMS& o) const {
    return hours == o.hours && minutes == o.minutes &&
           value_equal(seconds, o.seconds);
}

bool RectComplex::operator==(const RectComplex& o) const {
    return value_equal(re, o.re) && value_equal(im, o.im);
}

bool PolarComplex::operator==(const PolarComplex& o) const {
    return value_equal(r, o.r) && value_equal(theta, o.theta);
}

bool DateForm::operator==(const DateForm& o) const {
    return value_equal(n, o.n);
}

bool ModuloForm::operator==(const ModuloForm& o) const {
    return value_equal(n, o.n) && value_equal(m, o.m);
}

bool ErrorForm::operator==(const ErrorForm& o) const {
    return value_equal(x, o.x) && value_equal(sigma, o.sigma);
}

bool Interval::operator==(const Interval& o) const {
    return mask == o.mask && value_equal(lo, o.lo) && value_equal(hi, o.hi);
}

bool Vector::operator==(const Vector& o) const {
    if (elements.size() != o.elements.size()) return false;
    for (size_t i = 0; i < elements.size(); ++i) {
        if (!value_equal(elements[i], o.elements[i])) return false;
    }
    return true;
}

// --- Factory functions ---

ValuePtr Value::make_integer(mpz_class v) {
    auto val = std::make_shared<Value>();
    val->data = Integer{std::move(v)};
    return val;
}

ValuePtr Value::make_fraction(mpz_class num, mpz_class den) {
    if (den == 0) {
        throw std::domain_error("fraction with zero denominator");
    }
    // Normalize sign: denominator always positive
    if (den < 0) {
        num = -num;
        den = -den;
    }
    // Reduce
    mpz_class g;
    mpz_gcd(g.get_mpz_t(), num.get_mpz_t(), den.get_mpz_t());
    if (g > 1) {
        num /= g;
        den /= g;
    }
    // If denominator is 1, return integer
    if (den == 1) {
        return make_integer(std::move(num));
    }
    auto val = std::make_shared<Value>();
    val->data = Fraction{std::move(num), std::move(den)};
    return val;
}

ValuePtr Value::make_float(mpz_class mantissa, int exponent) {
    // Strip trailing zeros from mantissa
    if (mantissa == 0) {
        auto val = std::make_shared<Value>();
        val->data = DecimalFloat{mpz_class(0), 0};
        return val;
    }
    while (mpz_divisible_ui_p(mantissa.get_mpz_t(), 10)) {
        mantissa /= 10;
        exponent++;
    }
    auto val = std::make_shared<Value>();
    val->data = DecimalFloat{std::move(mantissa), exponent};
    return val;
}

ValuePtr Value::make_float_normalized(mpz_class mantissa, int exponent, int precision) {
    if (mantissa == 0) {
        auto val = std::make_shared<Value>();
        val->data = DecimalFloat{mpz_class(0), 0};
        return val;
    }

    // Count digits of |mantissa|
    mpz_class abs_m = abs(mantissa);
    size_t digits = mpz_sizeinbase(abs_m.get_mpz_t(), 10);

    // If too many digits, round
    if (static_cast<int>(digits) > precision) {
        int excess = static_cast<int>(digits) - precision;
        mpz_class divisor;
        mpz_ui_pow_ui(divisor.get_mpz_t(), 10, excess);

        // Round half away from zero
        mpz_class half = divisor / 2;
        if (mantissa > 0) {
            mantissa = (mantissa + half) / divisor;
        } else {
            mantissa = -((-mantissa + half) / divisor);
        }
        exponent += excess;
    }

    // Strip trailing zeros
    return make_float(std::move(mantissa), exponent);
}

ValuePtr Value::make_hms(int h, int m, ValuePtr s) {
    auto val = std::make_shared<Value>();
    val->data = HMS{h, m, std::move(s)};
    return val;
}

ValuePtr Value::make_rect_complex(ValuePtr re, ValuePtr im) {
    // If imaginary part is zero, return just the real part
    if (im->is_zero()) {
        return re;
    }
    auto val = std::make_shared<Value>();
    val->data = RectComplex{std::move(re), std::move(im)};
    return val;
}

ValuePtr Value::make_polar_complex(ValuePtr r, ValuePtr theta) {
    if (theta->is_zero()) {
        return r;
    }
    auto val = std::make_shared<Value>();
    val->data = PolarComplex{std::move(r), std::move(theta)};
    return val;
}

ValuePtr Value::make_error(ValuePtr x, ValuePtr sigma) {
    auto val = std::make_shared<Value>();
    val->data = ErrorForm{std::move(x), std::move(sigma)};
    return val;
}

ValuePtr Value::make_mod(ValuePtr n, ValuePtr m) {
    auto val = std::make_shared<Value>();
    val->data = ModuloForm{std::move(n), std::move(m)};
    return val;
}

ValuePtr Value::make_interval(uint8_t mask, ValuePtr lo, ValuePtr hi) {
    auto val = std::make_shared<Value>();
    val->data = Interval{mask, std::move(lo), std::move(hi)};
    return val;
}

ValuePtr Value::make_vector(std::vector<ValuePtr> elements) {
    auto val = std::make_shared<Value>();
    val->data = Vector{std::move(elements)};
    return val;
}

ValuePtr Value::make_date(ValuePtr n) {
    auto val = std::make_shared<Value>();
    val->data = DateForm{std::move(n)};
    return val;
}

ValuePtr Value::make_infinity(Infinity::Kind k) {
    auto val = std::make_shared<Value>();
    val->data = Infinity{k};
    return val;
}

ValuePtr Value::zero() {
    return make_integer(0);
}

ValuePtr Value::one() {
    return make_integer(1);
}

bool Value::is_zero() const {
    switch (tag()) {
        case Tag::Integer:
            return as_integer().v == 0;
        case Tag::Fraction:
            return as_fraction().num == 0;
        case Tag::DecimalFloat:
            return as_float().mantissa == 0;
        default:
            return false;
    }
}

bool Value::is_negative() const {
    switch (tag()) {
        case Tag::Integer:
            return as_integer().v < 0;
        case Tag::Fraction:
            return as_fraction().num < 0;
        case Tag::DecimalFloat:
            return as_float().mantissa < 0;
        case Tag::Infinity:
            return as_infinity().kind == Infinity::Neg;
        default:
            return false;
    }
}

bool value_equal(const ValuePtr& a, const ValuePtr& b) {
    if (a == b) return true;
    if (!a || !b) return false;
    return *a == *b;
}

} // namespace sc
