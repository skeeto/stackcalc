#include "fraction.hpp"
#include <stdexcept>

namespace sc::fraction {

ValuePtr add(const Fraction& a, const Fraction& b) {
    // a.num/a.den + b.num/b.den = (a.num*b.den + b.num*a.den) / (a.den*b.den)
    return Value::make_fraction(
        a.num * b.den + b.num * a.den,
        a.den * b.den
    );
}

ValuePtr sub(const Fraction& a, const Fraction& b) {
    return Value::make_fraction(
        a.num * b.den - b.num * a.den,
        a.den * b.den
    );
}

ValuePtr mul(const Fraction& a, const Fraction& b) {
    return Value::make_fraction(a.num * b.num, a.den * b.den);
}

ValuePtr div(const Fraction& a, const Fraction& b) {
    if (b.num == 0) throw std::domain_error("division by zero");
    return Value::make_fraction(a.num * b.den, a.den * b.num);
}

ValuePtr neg(const Fraction& a) {
    return Value::make_fraction(-a.num, a.den);
}

ValuePtr abs(const Fraction& a) {
    return Value::make_fraction(::abs(a.num), a.den);
}

ValuePtr reciprocal(const Fraction& a) {
    if (a.num == 0) throw std::domain_error("reciprocal of zero");
    return Value::make_fraction(a.den, a.num);
}

ValuePtr fdiv(const Fraction& a, const Fraction& b) {
    if (b.num == 0) throw std::domain_error("division by zero");
    // (a.num/a.den) / (b.num/b.den) = (a.num*b.den) / (a.den*b.num)
    mpz_class numer = a.num * b.den;
    mpz_class denom = a.den * b.num;
    // Floor division
    if (denom < 0) { numer = -numer; denom = -denom; }
    mpz_class q;
    mpz_fdiv_q(q.get_mpz_t(), numer.get_mpz_t(), denom.get_mpz_t());
    return Value::make_integer(std::move(q));
}

ValuePtr mod(const Fraction& a, const Fraction& b) {
    // a mod b = a - b * floor(a/b)
    if (b.num == 0) throw std::domain_error("division by zero");
    mpz_class numer = a.num * b.den;
    mpz_class denom = a.den * b.num;
    if (denom < 0) { numer = -numer; denom = -denom; }
    mpz_class q;
    mpz_fdiv_q(q.get_mpz_t(), numer.get_mpz_t(), denom.get_mpz_t());
    // result = a - b*q
    return Value::make_fraction(
        a.num * b.den - q * b.num * a.den,
        a.den * b.den
    );
}

Fraction from_integer(const Integer& i) {
    return Fraction{i.v, mpz_class(1)};
}

ValuePtr to_float(const Fraction& f, int precision) {
    if (f.num == 0) {
        return Value::make_float(mpz_class(0), 0);
    }
    // Compute mantissa = num * 10^precision / den, then normalize
    mpz_class scale;
    mpz_ui_pow_ui(scale.get_mpz_t(), 10, precision);
    mpz_class scaled_num = f.num * scale;

    // Round half away from zero
    mpz_class abs_num = ::abs(scaled_num);
    mpz_class half_den = f.den / 2;
    mpz_class numerator = abs_num + half_den;
    mpz_class q;
    mpz_tdiv_q(q.get_mpz_t(), numerator.get_mpz_t(), f.den.get_mpz_t());
    if (scaled_num < 0) q = -q;

    return Value::make_float_normalized(std::move(q), -precision, precision);
}

int cmp(const Fraction& a, const Fraction& b) {
    mpz_class lhs = a.num * b.den;
    mpz_class rhs = b.num * a.den;
    return mpz_cmp(lhs.get_mpz_t(), rhs.get_mpz_t());
}

bool is_zero(const Fraction& a) { return a.num == 0; }
bool is_negative(const Fraction& a) { return a.num < 0; }

} // namespace sc::fraction
