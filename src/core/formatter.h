#pragma once

#include "value.h"
#include "modes.h"
#include <string>

namespace sc {

class Formatter {
public:
    explicit Formatter(const CalcState& state) : state_(state) {}

    // Format any value to its display string.
    std::string format(const ValuePtr& v) const;

private:
    const CalcState& state_;

    std::string format_integer(const Integer& v) const;
    std::string format_fraction(const Fraction& v) const;
    std::string format_float(const DecimalFloat& v) const;
    std::string format_complex(const RectComplex& v) const;
    std::string format_polar(const PolarComplex& v) const;
    std::string format_hms(const HMS& v) const;
    std::string format_error(const ErrorForm& v) const;
    std::string format_mod(const ModuloForm& v) const;
    std::string format_interval(const Interval& v) const;
    std::string format_vector(const Vector& v) const;
    std::string format_infinity(const Infinity& v) const;
    std::string format_date(const DateForm& v) const;

    // Integer formatting in given radix with optional grouping
    std::string format_integer_str(const mpz_class& v) const;
    // Apply digit grouping to a digit string
    std::string group_digits(const std::string& digits, bool after_point = false) const;
    // Format a real number in the current display format
    std::string format_real(const ValuePtr& v) const;
};

} // namespace sc
