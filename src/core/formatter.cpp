#include "formatter.h"
#include "decimal_float.h"
#include "arithmetic.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace sc {

std::string Formatter::format(const ValuePtr& v) const {
    switch (v->tag()) {
        case Tag::Integer:      return format_integer(v->as_integer());
        case Tag::Fraction:     return format_fraction(v->as_fraction());
        case Tag::DecimalFloat: return format_float(v->as_float());
        case Tag::RectComplex:  return format_complex(v->as_rect_complex());
        case Tag::PolarComplex: return format_polar(v->as_polar_complex());
        case Tag::HMS:          return format_hms(v->as_hms());
        case Tag::ErrorForm:    return format_error(v->as_error());
        case Tag::ModuloForm:   return format_mod(v->as_mod());
        case Tag::Interval:     return format_interval(v->as_interval());
        case Tag::Vector:       return format_vector(v->as_vector());
        case Tag::Infinity:     return format_infinity(v->as_infinity());
        case Tag::DateForm:     return format_date(v->as_date());
    }
    return "?";
}

std::string Formatter::group_digits(const std::string& digits, bool after_point) const {
    if (state_.group_digits <= 0) return digits;
    int g = state_.group_digits;
    std::string result;
    if (after_point) {
        // Group left-to-right after decimal point
        for (size_t i = 0; i < digits.size(); ++i) {
            if (i > 0 && (i % g) == 0) result += state_.group_char;
            result += digits[i];
        }
    } else {
        // Group right-to-left before decimal point
        int n = static_cast<int>(digits.size());
        for (int i = 0; i < n; ++i) {
            if (i > 0 && ((n - i) % g) == 0) result += state_.group_char;
            result += digits[i];
        }
    }
    return result;
}

std::string Formatter::format_integer_str(const mpz_class& v) const {
    if (state_.display_radix == 10) {
        std::string s = v.get_str(10);
        // Separate sign from digits for grouping
        if (s[0] == '-') {
            return "-" + group_digits(s.substr(1));
        }
        return group_digits(s);
    }

    // Non-decimal radix
    bool neg = (v < 0);
    mpz_class abs_v = ::abs(v);
    std::string digits = abs_v.get_str(state_.display_radix);

    // Uppercase for hex
    if (state_.display_radix > 10) {
        for (auto& c : digits) c = static_cast<char>(std::toupper(c));
    }

    // Leading zeros for word size
    if (state_.leading_zeros && state_.word_size > 0) {
        int bits = state_.word_size;
        // Digits needed = ceil(bits / log2(radix))
        double log2r = std::log2(static_cast<double>(state_.display_radix));
        int needed = static_cast<int>(std::ceil(bits / log2r));
        while (static_cast<int>(digits.size()) < needed)
            digits = "0" + digits;
    }

    digits = group_digits(digits);

    std::string prefix = std::to_string(state_.display_radix) + "#";
    return (neg ? "-" : "") + prefix + digits;
}

std::string Formatter::format_integer(const Integer& v) const {
    return format_integer_str(v.v);
}

std::string Formatter::format_fraction(const Fraction& v) const {
    return format_integer_str(v.num) + ":" + format_integer_str(v.den);
}

std::string Formatter::format_float(const DecimalFloat& v) const {
    if (decimal_float::is_zero(v)) {
        switch (state_.display_format) {
            case DisplayFormat::Sci: return "0" + std::string(1, state_.point_char) + "e0";
            case DisplayFormat::Eng: return "0" + std::string(1, state_.point_char) + "e0";
            case DisplayFormat::Fix: {
                int digits = state_.display_digits >= 0 ? state_.display_digits : 0;
                if (digits == 0) return "0" + std::string(1, state_.point_char);
                return "0" + state_.point_char + std::string(digits, '0');
            }
            default: return "0" + std::string(1, state_.point_char);
        }
    }

    // Get the digit string and sign
    bool neg = decimal_float::is_negative(v);
    mpz_class abs_m = ::abs(v.mantissa);
    std::string digits = abs_m.get_str(10);
    int ndigits = static_cast<int>(digits.size());

    // The value is: digits * 10^exponent
    // As a number: d0.d1d2...dn * 10^(ndigits + exponent - 1)
    // scientific exponent = ndigits + exponent - 1
    int sci_exp = ndigits + v.exponent - 1;

    std::string sign = neg ? "-" : "";

    switch (state_.display_format) {
        case DisplayFormat::Sci: {
            // Mantissa: first digit, then point, then rest
            int sig = state_.display_digits >= 0 ? state_.display_digits : ndigits;
            std::string mantissa;
            mantissa += digits[0];
            mantissa += state_.point_char;
            if (ndigits > 1) {
                std::string frac = digits.substr(1);
                if (static_cast<int>(frac.size()) < sig - 1)
                    frac += std::string(sig - 1 - static_cast<int>(frac.size()), '0');
                else if (static_cast<int>(frac.size()) > sig - 1 && sig > 0)
                    frac = frac.substr(0, sig - 1);
                mantissa += frac;
            } else if (sig > 1) {
                mantissa += std::string(sig - 1, '0');
            }
            return sign + mantissa + "e" + std::to_string(sci_exp);
        }
        case DisplayFormat::Eng: {
            // Exponent must be multiple of 3
            int eng_exp = sci_exp - ((sci_exp % 3 + 3) % 3);
            int pre_digits = sci_exp - eng_exp + 1;
            int sig = state_.display_digits >= 0 ? state_.display_digits : ndigits;

            // Pad digits if needed
            std::string d = digits;
            while (static_cast<int>(d.size()) < pre_digits) d += '0';
            while (static_cast<int>(d.size()) < sig) d += '0';

            std::string mantissa = d.substr(0, pre_digits);
            mantissa += state_.point_char;
            std::string frac = d.substr(pre_digits);
            if (static_cast<int>(frac.size()) > sig - pre_digits && sig > pre_digits)
                frac = frac.substr(0, sig - pre_digits);
            mantissa += frac;
            return sign + mantissa + "e" + std::to_string(eng_exp);
        }
        case DisplayFormat::Fix: {
            int fix_digits = state_.display_digits >= 0 ? state_.display_digits : state_.precision;
            // Position of decimal point from the left of digits
            // The number is: digits * 10^exponent
            // Integer part has ndigits + exponent digits (if positive), else starts with 0.
            int int_digits = ndigits + v.exponent;

            if (int_digits <= 0) {
                // Number is 0.000...digits
                std::string result = sign + "0" + state_.point_char;
                int leading_zeros = -int_digits;
                std::string frac = std::string(leading_zeros, '0') + digits;
                if (static_cast<int>(frac.size()) < fix_digits)
                    frac += std::string(fix_digits - static_cast<int>(frac.size()), '0');
                else if (static_cast<int>(frac.size()) > fix_digits)
                    frac = frac.substr(0, fix_digits);
                return result + frac;
            }
            if (int_digits >= ndigits) {
                // No fractional part, just pad
                std::string int_part = digits + std::string(int_digits - ndigits, '0');
                int_part = group_digits(int_part);
                if (fix_digits > 0) {
                    return sign + int_part + state_.point_char + std::string(fix_digits, '0');
                }
                return sign + int_part + state_.point_char;
            }
            // Has both integer and fractional parts
            std::string int_part = group_digits(digits.substr(0, int_digits));
            std::string frac = digits.substr(int_digits);
            if (static_cast<int>(frac.size()) < fix_digits)
                frac += std::string(fix_digits - static_cast<int>(frac.size()), '0');
            else if (static_cast<int>(frac.size()) > fix_digits)
                frac = frac.substr(0, fix_digits);
            return sign + int_part + state_.point_char + frac;
        }
        default: break;
    }

    // Normal format. Switch to scientific notation for very small or very
    // large numbers (matches Emacs M-x calc `d n` behavior). Threshold:
    //   sci_exp <= -3                      → scientific (avoids 0.000…)
    //   sci_exp >  precision + 3           → scientific (avoids long zero tails)
    if (sci_exp <= -3 || sci_exp > state_.precision + 3) {
        // Mantissa: leading digit, decimal point only if more digits.
        // Trailing zeros are already stripped by DecimalFloat normalization.
        std::string mantissa;
        mantissa += digits[0];
        if (ndigits > 1) {
            mantissa += state_.point_char;
            mantissa += digits.substr(1);
        }
        return sign + mantissa + "e" + std::to_string(sci_exp);
    }

    // Decimal layout.
    // Integer part digits = ndigits + exponent
    int int_part_digits = ndigits + v.exponent;

    if (int_part_digits <= 0) {
        // 0.000...digits
        int leading = -int_part_digits;
        return sign + "0" + state_.point_char + std::string(leading, '0') + digits;
    }
    if (int_part_digits >= ndigits) {
        // Pure integer with trailing zeros
        std::string int_str = digits + std::string(int_part_digits - ndigits, '0');
        return sign + group_digits(int_str) + state_.point_char;
    }
    // Mixed: some digits before point, some after
    std::string int_str = group_digits(digits.substr(0, int_part_digits));
    std::string frac_str = digits.substr(int_part_digits);
    return sign + int_str + state_.point_char + frac_str;
}

std::string Formatter::format_real(const ValuePtr& v) const {
    return format(v);
}

std::string Formatter::format_complex(const RectComplex& v) const {
    switch (state_.complex_format) {
        case ComplexFormat::INotation: {
            std::string re = format(v.re);
            std::string im = format(v.im);
            if (v.im->is_negative()) return re + " - " + format(arith::neg(v.im)) + "i";
            return re + " + " + im + "i";
        }
        case ComplexFormat::JNotation: {
            std::string re = format(v.re);
            std::string im = format(v.im);
            if (v.im->is_negative()) return re + " - " + format(arith::neg(v.im)) + "j";
            return re + " + " + im + "j";
        }
        default: // Pair
            return "(" + format(v.re) + ", " + format(v.im) + ")";
    }
}

std::string Formatter::format_polar(const PolarComplex& v) const {
    return "(" + format(v.r) + "; " + format(v.theta) + ")";
}

std::string Formatter::format_hms(const HMS& v) const {
    std::string result;
    if (v.hours < 0) result += "-";
    result += std::to_string(std::abs(v.hours));
    result += "@ ";
    result += std::to_string(v.minutes);
    result += "' ";
    result += format(v.seconds);
    result += "\"";
    return result;
}

std::string Formatter::format_error(const ErrorForm& v) const {
    return format(v.x) + " +/- " + format(v.sigma);
}

std::string Formatter::format_mod(const ModuloForm& v) const {
    return format(v.n) + " mod " + format(v.m);
}

std::string Formatter::format_interval(const Interval& v) const {
    std::string lo_bracket = (v.mask & 2) ? "[" : "(";
    std::string hi_bracket = (v.mask & 1) ? "]" : ")";
    return lo_bracket + format(v.lo) + " .. " + format(v.hi) + hi_bracket;
}

std::string Formatter::format_vector(const Vector& v) const {
    std::string result = "[";
    for (size_t i = 0; i < v.elements.size(); ++i) {
        if (i > 0) result += ", ";
        result += format(v.elements[i]);
    }
    result += "]";
    return result;
}

std::string Formatter::format_infinity(const Infinity& v) const {
    switch (v.kind) {
        case Infinity::Pos: return "inf";
        case Infinity::Neg: return "-inf";
        case Infinity::Undirected: return "uinf";
        case Infinity::NaN: return "nan";
    }
    return "?";
}

std::string Formatter::format_date(const DateForm& v) const {
    return "<date " + format(v.n) + ">";
}

} // namespace sc
