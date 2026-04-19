#include "hms.hpp"
#include "arithmetic.hpp"
#include "decimal_float.hpp"

namespace sc::hms {

// Helper: convert HMS to total seconds as a Value
static ValuePtr to_total_seconds(const HMS& h, int precision) {
    // total = hours*3600 + minutes*60 + seconds
    auto h_sec = arith::mul(Value::make_integer(h.hours), Value::make_integer(3600), precision);
    auto m_sec = arith::mul(Value::make_integer(h.minutes), Value::make_integer(60), precision);
    auto sum = arith::add(h_sec, m_sec, precision);
    return arith::add(sum, h.seconds, precision);
}

// Helper: reconstruct HMS from total seconds
static ValuePtr from_total_seconds(const ValuePtr& total_sec, int precision) {
    // hours = floor(total / 3600)
    auto sixty = Value::make_integer(60);
    auto thirty_six_hundred = Value::make_integer(3600);

    auto total_abs = total_sec;
    bool negative = total_sec->is_negative();
    if (negative) total_abs = arith::neg(total_sec);

    auto hours_v = arith::floor(arith::div(total_abs, thirty_six_hundred, precision, FractionMode::Float));
    int hours = static_cast<int>(hours_v->as_integer().v.get_si());

    auto remainder = arith::sub(total_abs,
        arith::mul(hours_v, thirty_six_hundred, precision), precision);

    auto mins_v = arith::floor(arith::div(remainder, sixty, precision, FractionMode::Float));
    int mins = static_cast<int>(mins_v->as_integer().v.get_si());

    auto secs = arith::sub(remainder,
        arith::mul(mins_v, sixty, precision), precision);

    if (negative) {
        hours = -hours;
        // In Emacs calc, sign applies to the whole HMS
    }

    return Value::make_hms(negative ? -hours : hours, mins, std::move(secs));
}

ValuePtr add(const HMS& a, const HMS& b, int precision) {
    auto sa = to_total_seconds(a, precision);
    auto sb = to_total_seconds(b, precision);
    auto total = arith::add(sa, sb, precision);
    return from_total_seconds(total, precision);
}

ValuePtr sub(const HMS& a, const HMS& b, int precision) {
    auto sa = to_total_seconds(a, precision);
    auto sb = to_total_seconds(b, precision);
    auto total = arith::sub(sa, sb, precision);
    return from_total_seconds(total, precision);
}

ValuePtr mul_scalar(const HMS& a, const ValuePtr& scalar, int precision) {
    auto sa = to_total_seconds(a, precision);
    auto total = arith::mul(sa, scalar, precision);
    return from_total_seconds(total, precision);
}

ValuePtr div_scalar(const HMS& a, const ValuePtr& scalar, int precision) {
    auto sa = to_total_seconds(a, precision);
    auto total = arith::div(sa, scalar, precision, FractionMode::Float);
    return from_total_seconds(total, precision);
}

ValuePtr neg(const HMS& a) {
    return Value::make_hms(-a.hours, a.minutes, a.seconds);
}

ValuePtr to_degrees(const HMS& a, int precision) {
    // degrees = hours + minutes/60 + seconds/3600
    auto sixty = Value::make_integer(60);
    auto thirty_six_hundred = Value::make_integer(3600);
    auto h = Value::make_integer(a.hours);
    auto m_frac = arith::div(Value::make_integer(a.minutes), sixty, precision, FractionMode::Float);
    auto s_frac = arith::div(a.seconds, thirty_six_hundred, precision, FractionMode::Float);
    auto sum = arith::add(h, m_frac, precision);
    return arith::add(sum, s_frac, precision);
}

ValuePtr from_degrees(const ValuePtr& deg, int precision) {
    auto thirty_six_hundred = Value::make_integer(3600);
    auto total_sec = arith::mul(deg, thirty_six_hundred, precision);
    return from_total_seconds(total_sec, precision);
}

} // namespace sc::hms
