#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <gmpxx.h>

namespace sc {

class Value;
using ValuePtr = std::shared_ptr<const Value>;

// --- Leaf types ---

struct Integer {
    mpz_class v;

    bool operator==(const Integer&) const = default;
};

struct Fraction {
    mpz_class num;
    mpz_class den;  // > 0, gcd(num,den) == 1

    bool operator==(const Fraction&) const = default;
};

// Decimal float: value = mantissa * 10^exponent
// Mantissa has at most `precision` digits, trailing zeros stripped.
// Zero is represented as {0, 0}.
struct DecimalFloat {
    mpz_class mantissa;  // signed
    int exponent;

    bool operator==(const DecimalFloat&) const = default;
};

struct HMS {
    int hours;
    int minutes;     // [0, 60)
    ValuePtr seconds; // real in [0, 60)

    bool operator==(const HMS& o) const;
};

struct RectComplex {
    ValuePtr re;
    ValuePtr im;  // nonzero

    bool operator==(const RectComplex& o) const;
};

struct PolarComplex {
    ValuePtr r;     // positive real
    ValuePtr theta; // angle

    bool operator==(const PolarComplex& o) const;
};

struct DateForm {
    ValuePtr n;  // days since Jan 1, 1 AD

    bool operator==(const DateForm& o) const;
};

struct ModuloForm {
    ValuePtr n;  // value in [0, m)
    ValuePtr m;  // positive modulus

    bool operator==(const ModuloForm& o) const;
};

struct ErrorForm {
    ValuePtr x;     // mean
    ValuePtr sigma; // std dev >= 0

    bool operator==(const ErrorForm& o) const;
};

struct Interval {
    // bit 0 = hi closed, bit 1 = lo closed
    // 0=(a..b), 1=(a..b], 2=[a..b), 3=[a..b]
    uint8_t mask;
    ValuePtr lo;
    ValuePtr hi;

    bool operator==(const Interval& o) const;
};

struct Vector {
    std::vector<ValuePtr> elements;

    bool operator==(const Vector& o) const;
};

struct Infinity {
    enum Kind : uint8_t { Pos, Neg, Undirected, NaN };
    Kind kind;

    bool operator==(const Infinity&) const = default;
};

using ValueData = std::variant<
    Integer,
    Fraction,
    DecimalFloat,
    HMS,
    RectComplex,
    PolarComplex,
    DateForm,
    ModuloForm,
    ErrorForm,
    Interval,
    Vector,
    Infinity
>;

enum class Tag : uint8_t {
    Integer, Fraction, DecimalFloat, HMS,
    RectComplex, PolarComplex,
    DateForm, ModuloForm, ErrorForm, Interval,
    Vector, Infinity
};

class Value {
public:
    ValueData data;

    Tag tag() const { return static_cast<Tag>(data.index()); }

    bool operator==(const Value& o) const { return data == o.data; }

    // --- Type predicates ---
    bool is_integer() const { return tag() == Tag::Integer; }
    bool is_fraction() const { return tag() == Tag::Fraction; }
    bool is_float() const { return tag() == Tag::DecimalFloat; }
    bool is_rational() const { return is_integer() || is_fraction(); }
    bool is_real() const { return is_rational() || is_float(); }
    bool is_complex() const { return tag() == Tag::RectComplex || tag() == Tag::PolarComplex; }
    bool is_number() const { return is_real() || is_complex(); }
    bool is_hms() const { return tag() == Tag::HMS; }
    bool is_vector() const { return tag() == Tag::Vector; }
    bool is_infinity() const { return tag() == Tag::Infinity; }
    bool is_error() const { return tag() == Tag::ErrorForm; }
    bool is_mod() const { return tag() == Tag::ModuloForm; }
    bool is_interval() const { return tag() == Tag::Interval; }
    bool is_date() const { return tag() == Tag::DateForm; }

    // Accessors (unchecked — caller must verify tag)
    const Integer& as_integer() const { return std::get<Integer>(data); }
    const Fraction& as_fraction() const { return std::get<Fraction>(data); }
    const DecimalFloat& as_float() const { return std::get<DecimalFloat>(data); }
    const HMS& as_hms() const { return std::get<HMS>(data); }
    const RectComplex& as_rect_complex() const { return std::get<RectComplex>(data); }
    const PolarComplex& as_polar_complex() const { return std::get<PolarComplex>(data); }
    const DateForm& as_date() const { return std::get<DateForm>(data); }
    const ModuloForm& as_mod() const { return std::get<ModuloForm>(data); }
    const ErrorForm& as_error() const { return std::get<ErrorForm>(data); }
    const Interval& as_interval() const { return std::get<Interval>(data); }
    const Vector& as_vector() const { return std::get<Vector>(data); }
    const Infinity& as_infinity() const { return std::get<Infinity>(data); }

    // --- Factory functions (normalize on construction) ---
    static ValuePtr make_integer(mpz_class v);
    static ValuePtr make_fraction(mpz_class num, mpz_class den);
    static ValuePtr make_float(mpz_class mantissa, int exponent);
    static ValuePtr make_float_normalized(mpz_class mantissa, int exponent, int precision);
    static ValuePtr make_hms(int h, int m, ValuePtr s);
    static ValuePtr make_rect_complex(ValuePtr re, ValuePtr im);
    static ValuePtr make_polar_complex(ValuePtr r, ValuePtr theta);
    static ValuePtr make_error(ValuePtr x, ValuePtr sigma);
    static ValuePtr make_mod(ValuePtr n, ValuePtr m);
    static ValuePtr make_interval(uint8_t mask, ValuePtr lo, ValuePtr hi);
    static ValuePtr make_vector(std::vector<ValuePtr> elements);
    static ValuePtr make_date(ValuePtr n);
    static ValuePtr make_infinity(Infinity::Kind k);

    // Convenience
    static ValuePtr zero();
    static ValuePtr one();

    // Check if value is zero
    bool is_zero() const;
    // Check if value is negative
    bool is_negative() const;
};

// Compare two ValuePtrs by value
bool value_equal(const ValuePtr& a, const ValuePtr& b);

} // namespace sc
