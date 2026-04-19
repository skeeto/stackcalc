#include "error_form.hpp"
#include "arithmetic.hpp"

namespace sc::error_form {

// Helper: combine errors as sqrt(da^2 + db^2)
static ValuePtr combine_errors(const ValuePtr& da, const ValuePtr& db, int precision) {
    auto da2 = arith::mul(da, da, precision);
    auto db2 = arith::mul(db, db, precision);
    auto sum = arith::add(da2, db2, precision);
    return arith::sqrt(sum, precision);
}

ValuePtr add(const ErrorForm& a, const ErrorForm& b, int precision) {
    auto x = arith::add(a.x, b.x, precision);
    auto sigma = combine_errors(a.sigma, b.sigma, precision);
    return Value::make_error(std::move(x), std::move(sigma));
}

ValuePtr sub(const ErrorForm& a, const ErrorForm& b, int precision) {
    auto x = arith::sub(a.x, b.x, precision);
    auto sigma = combine_errors(a.sigma, b.sigma, precision);
    return Value::make_error(std::move(x), std::move(sigma));
}

ValuePtr mul(const ErrorForm& a, const ErrorForm& b, int precision) {
    // (x +/- dx) * (y +/- dy) = x*y +/- |x*y| * sqrt((dx/x)^2 + (dy/y)^2)
    auto xy = arith::mul(a.x, b.x, precision);

    // Relative errors
    auto rel_a = arith::div(a.sigma, arith::abs(a.x), precision, FractionMode::Float);
    auto rel_b = arith::div(b.sigma, arith::abs(b.x), precision, FractionMode::Float);
    auto combined_rel = combine_errors(rel_a, rel_b, precision);
    auto sigma = arith::mul(arith::abs(xy), combined_rel, precision);

    return Value::make_error(std::move(xy), std::move(sigma));
}

ValuePtr div(const ErrorForm& a, const ErrorForm& b, int precision) {
    auto xy = arith::div(a.x, b.x, precision, FractionMode::Float);

    auto rel_a = arith::div(a.sigma, arith::abs(a.x), precision, FractionMode::Float);
    auto rel_b = arith::div(b.sigma, arith::abs(b.x), precision, FractionMode::Float);
    auto combined_rel = combine_errors(rel_a, rel_b, precision);
    auto sigma = arith::mul(arith::abs(xy), combined_rel, precision);

    return Value::make_error(std::move(xy), std::move(sigma));
}

ValuePtr neg(const ErrorForm& a) {
    return Value::make_error(arith::neg(a.x), a.sigma);
}

ValuePtr abs(const ErrorForm& a, int precision) {
    (void)precision;
    return Value::make_error(arith::abs(a.x), a.sigma);
}

ValuePtr from_value(ValuePtr v) {
    return Value::make_error(std::move(v), Value::zero());
}

} // namespace sc::error_form
