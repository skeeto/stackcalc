#include "interval.h"
#include "arithmetic.h"
#include <algorithm>

namespace sc::interval {

// Mask bits: bit1 = lo closed, bit0 = hi closed
// Addition of closedness: both closed -> closed
static uint8_t and_mask_lo(uint8_t a, uint8_t b) {
    return ((a & 2) && (b & 2)) ? 2 : 0;
}
static uint8_t and_mask_hi(uint8_t a, uint8_t b) {
    return ((a & 1) && (b & 1)) ? 1 : 0;
}

ValuePtr add(const Interval& a, const Interval& b, int precision) {
    auto lo = arith::add(a.lo, b.lo, precision);
    auto hi = arith::add(a.hi, b.hi, precision);
    uint8_t mask = and_mask_lo(a.mask, b.mask) | and_mask_hi(a.mask, b.mask);
    return Value::make_interval(mask, std::move(lo), std::move(hi));
}

ValuePtr sub(const Interval& a, const Interval& b, int precision) {
    // [a..b] - [c..d] = [a-d .. b-c]
    auto lo = arith::sub(a.lo, b.hi, precision);
    auto hi = arith::sub(a.hi, b.lo, precision);
    // lo closedness: a.lo closed AND b.hi closed
    // hi closedness: a.hi closed AND b.lo closed
    uint8_t lo_closed = ((a.mask & 2) && (b.mask & 1)) ? 2 : 0;
    uint8_t hi_closed = ((a.mask & 1) && (b.mask & 2)) ? 1 : 0;
    return Value::make_interval(lo_closed | hi_closed, std::move(lo), std::move(hi));
}

ValuePtr mul(const Interval& a, const Interval& b, int precision) {
    // Worst-case: compute all four products, take min/max
    auto p1 = arith::mul(a.lo, b.lo, precision);
    auto p2 = arith::mul(a.lo, b.hi, precision);
    auto p3 = arith::mul(a.hi, b.lo, precision);
    auto p4 = arith::mul(a.hi, b.hi, precision);

    // Compare to find min and max
    ValuePtr products[] = {p1, p2, p3, p4};
    ValuePtr lo = products[0], hi = products[0];
    for (int i = 1; i < 4; ++i) {
        if (arith::sub(products[i], lo, precision)->is_negative()) lo = products[i];
        if (arith::sub(products[i], hi, precision)->is_negative()) {} else {
            if (arith::sub(hi, products[i], precision)->is_negative()) hi = products[i];
        }
    }

    // For simplicity, use closed interval for multiplication
    return Value::make_interval(3, std::move(lo), std::move(hi));
}

ValuePtr div(const Interval& a, const Interval& b, int precision) {
    // Check if b contains zero
    bool b_lo_neg = b.lo->is_negative();
    bool b_hi_neg = b.hi->is_negative();
    bool b_lo_zero = b.lo->is_zero();
    bool b_hi_zero = b.hi->is_zero();
    if ((b_lo_neg && !b_hi_neg && !b_hi_zero) ||
        (b_lo_zero && b_hi_zero)) {
        throw std::domain_error("interval division by interval containing zero");
    }

    // Invert b and multiply
    auto inv_lo = arith::inv(b.hi, precision);
    auto inv_hi = arith::inv(b.lo, precision);
    // Swap mask bits
    uint8_t inv_mask = ((b.mask & 1) << 1) | ((b.mask & 2) >> 1);
    Interval b_inv{inv_mask, std::move(inv_lo), std::move(inv_hi)};
    return mul(a, b_inv, precision);
}

ValuePtr neg(const Interval& a) {
    auto lo = arith::neg(a.hi);
    auto hi = arith::neg(a.lo);
    // Swap mask bits
    uint8_t mask = ((a.mask & 1) << 1) | ((a.mask & 2) >> 1);
    return Value::make_interval(mask, std::move(lo), std::move(hi));
}

ValuePtr abs(const Interval& a, int precision) {
    auto abs_lo = arith::abs(a.lo);
    auto abs_hi = arith::abs(a.hi);

    // If interval spans zero, min is 0
    if (a.lo->is_negative() && !a.hi->is_negative()) {
        auto zero = Value::zero();
        // max of abs endpoints
        ValuePtr hi;
        if (arith::sub(abs_lo, abs_hi, precision)->is_negative()) {
            hi = abs_hi;
        } else {
            hi = abs_lo;
        }
        return Value::make_interval(3, std::move(zero), std::move(hi));
    }

    // Both same sign
    if (a.lo->is_negative()) {
        // Both negative: [abs(hi) .. abs(lo)]
        return Value::make_interval(a.mask, std::move(abs_hi), std::move(abs_lo));
    }
    return Value::make_interval(a.mask, std::move(abs_lo), std::move(abs_hi));
}

ValuePtr pow(const Interval& a, long n, int precision) {
    if (n == 0) {
        // a^0 = [1, 1] (using the convention 0^0 = 1).
        return Value::make_interval(3, Value::one(), Value::one());
    }
    bool negative_exp = n < 0;
    long m = negative_exp ? -n : n;

    // Repeated multiplication. The mul() routine already handles
    // sign-flips correctly for intervals straddling zero (computes all
    // four corner products and takes min/max), so we get the right answer
    // even when the base interval includes negative numbers.
    Interval cur = a;
    for (long i = 1; i < m; ++i) {
        auto v = mul(cur, a, precision);
        cur = v->as_interval();
    }
    auto val = Value::make_interval(cur.mask, cur.lo, cur.hi);
    if (!negative_exp) return val;

    // For negative exponent, divide [1,1] by the result.
    Interval one_iv{3, Value::one(), Value::one()};
    return div(one_iv, val->as_interval(), precision);
}

} // namespace sc::interval
