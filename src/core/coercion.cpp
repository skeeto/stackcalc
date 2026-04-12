#include "coercion.h"
#include "fraction.h"
#include "decimal_float.h"

namespace sc::coerce {

ValuePtr to_float(const ValuePtr& v, int precision) {
    switch (v->tag()) {
        case Tag::Integer: {
            auto& i = v->as_integer();
            auto df = decimal_float::from_integer(i);
            return Value::make_float(std::move(df.mantissa), df.exponent);
        }
        case Tag::Fraction: {
            auto& f = v->as_fraction();
            return fraction::to_float(f, precision);
        }
        case Tag::DecimalFloat:
            return v; // already a float
        default:
            throw std::invalid_argument("cannot convert to float");
    }
}

ValuePtr to_fraction(const ValuePtr& v) {
    switch (v->tag()) {
        case Tag::Integer:
            return Value::make_fraction(v->as_integer().v, mpz_class(1));
        case Tag::Fraction:
            return v;
        default:
            throw std::invalid_argument("cannot convert to fraction");
    }
}

CoercedPair coerce_pair(const ValuePtr& a, const ValuePtr& b, int precision) {
    Tag ta = a->tag();
    Tag tb = b->tag();

    // If either is float, promote both to float
    if (ta == Tag::DecimalFloat || tb == Tag::DecimalFloat) {
        return {
            to_float(a, precision),
            to_float(b, precision),
            Tag::DecimalFloat
        };
    }

    // If either is fraction, promote both to fraction
    if (ta == Tag::Fraction || tb == Tag::Fraction) {
        return {
            to_fraction(a),
            to_fraction(b),
            Tag::Fraction
        };
    }

    // Both integers
    if (ta == Tag::Integer && tb == Tag::Integer) {
        return {a, b, Tag::Integer};
    }

    throw std::invalid_argument("cannot coerce types for arithmetic");
}

} // namespace sc::coerce
