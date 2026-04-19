#include "complex.hpp"
#include "arithmetic.hpp"

namespace sc::complex {

ValuePtr add(const RectComplex& a, const RectComplex& b, int precision) {
    auto re = arith::add(a.re, b.re, precision);
    auto im = arith::add(a.im, b.im, precision);
    return Value::make_rect_complex(std::move(re), std::move(im));
}

ValuePtr sub(const RectComplex& a, const RectComplex& b, int precision) {
    auto re = arith::sub(a.re, b.re, precision);
    auto im = arith::sub(a.im, b.im, precision);
    return Value::make_rect_complex(std::move(re), std::move(im));
}

ValuePtr mul(const RectComplex& a, const RectComplex& b, int precision) {
    // (a+bi)(c+di) = (ac-bd) + (ad+bc)i
    auto ac = arith::mul(a.re, b.re, precision);
    auto bd = arith::mul(a.im, b.im, precision);
    auto ad = arith::mul(a.re, b.im, precision);
    auto bc = arith::mul(a.im, b.re, precision);
    auto re = arith::sub(ac, bd, precision);
    auto im = arith::add(ad, bc, precision);
    return Value::make_rect_complex(std::move(re), std::move(im));
}

ValuePtr div(const RectComplex& a, const RectComplex& b, int precision) {
    // (a+bi)/(c+di) = ((ac+bd) + (bc-ad)i) / (c^2+d^2)
    auto ac = arith::mul(a.re, b.re, precision);
    auto bd = arith::mul(a.im, b.im, precision);
    auto bc = arith::mul(a.im, b.re, precision);
    auto ad = arith::mul(a.re, b.im, precision);

    auto c2 = arith::mul(b.re, b.re, precision);
    auto d2 = arith::mul(b.im, b.im, precision);
    auto denom = arith::add(c2, d2, precision);

    auto re_num = arith::add(ac, bd, precision);
    auto im_num = arith::sub(bc, ad, precision);

    auto re = arith::div(re_num, denom, precision, FractionMode::Float);
    auto im = arith::div(im_num, denom, precision, FractionMode::Float);
    return Value::make_rect_complex(std::move(re), std::move(im));
}

ValuePtr neg(const RectComplex& a) {
    return Value::make_rect_complex(arith::neg(a.re), arith::neg(a.im));
}

ValuePtr conj(const RectComplex& a) {
    return Value::make_rect_complex(a.re, arith::neg(a.im));
}

ValuePtr make_rect(ValuePtr re, ValuePtr im) {
    return Value::make_rect_complex(std::move(re), std::move(im));
}

ValuePtr from_real(ValuePtr re) {
    return re; // A real is already a valid value; complex with im=0 normalizes to real
}

ValuePtr magnitude(const RectComplex& a, int precision) {
    auto a2 = arith::mul(a.re, a.re, precision);
    auto b2 = arith::mul(a.im, a.im, precision);
    auto sum = arith::add(a2, b2, precision);
    return arith::sqrt(sum, precision);
}

ValuePtr argument(const RectComplex& a, int precision) {
    // For now, just compute atan2(im, re) — placeholder until Phase 4 trig
    (void)a; (void)precision;
    throw std::runtime_error("argument requires atan2 (Phase 4)");
}

ValuePtr inv(const RectComplex& a, int precision) {
    // 1 / (re + im*i) = (re - im*i) / (re^2 + im^2)
    if (a.re->is_zero() && a.im->is_zero())
        throw std::domain_error("reciprocal of zero");
    auto re2 = arith::mul(a.re, a.re, precision);
    auto im2 = arith::mul(a.im, a.im, precision);
    auto denom = arith::add(re2, im2, precision);
    auto re = arith::div(a.re, denom, precision, FractionMode::Float);
    auto im = arith::div(arith::neg(a.im), denom, precision, FractionMode::Float);
    return Value::make_rect_complex(std::move(re), std::move(im));
}

ValuePtr sqrt(const RectComplex& a, int precision) {
    // Closed form (no trig):
    //   sqrt(re + im*i) = sqrt((|z|+re)/2) + sign(im) * i * sqrt((|z|-re)/2)
    // where sign(0) = +1.
    auto mag = magnitude(a, precision);
    auto two = Value::make_integer(2);
    auto re_inner = arith::div(arith::add(mag, a.re, precision), two,
                               precision, FractionMode::Float);
    auto im_inner = arith::div(arith::sub(mag, a.re, precision), two,
                               precision, FractionMode::Float);
    auto re = arith::sqrt(re_inner, precision);
    auto im = arith::sqrt(im_inner, precision);
    if (a.im->is_negative()) im = arith::neg(im);
    return Value::make_rect_complex(std::move(re), std::move(im));
}

ValuePtr pow(const RectComplex& a, long n, int precision) {
    if (n == 0) return Value::one();
    if (n == 1) return Value::make_rect_complex(a.re, a.im);
    if (n < 0) {
        auto recip = inv(a, precision);
        // recip might collapse to a real if a.im was zero; let arith::power
        // handle the recursion either way.
        return arith::power(recip, Value::make_integer(-n), precision);
    }
    // Binary exponentiation through arith::mul (which dispatches on the
    // result type at each step — products of complexes can collapse to real).
    auto half = pow(a, n / 2, precision);
    auto half_sq = arith::mul(half, half, precision);
    if (n % 2 == 0) return half_sq;
    return arith::mul(half_sq, Value::make_rect_complex(a.re, a.im), precision);
}

} // namespace sc::complex
