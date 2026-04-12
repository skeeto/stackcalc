#include "complex.h"
#include "arithmetic.h"

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

} // namespace sc::complex
