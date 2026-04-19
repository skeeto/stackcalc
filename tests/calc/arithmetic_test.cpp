#include <gtest/gtest.h>
#include "operations.hpp"
#include <cmath>

using namespace sc;

class ArithmeticTest : public ::testing::Test {
protected:
    Stack s;
    void push_int(long v) { s.push(Value::make_integer(v)); }
    void push_frac(long num, long den) { s.push(Value::make_fraction(num, den)); }
    void push_float(long mant, int exp) { s.push(Value::make_float(mpz_class(mant), exp)); }
};

// --- Addition ---

TEST_F(ArithmeticTest, AddIntegers) {
    push_int(2);
    push_int(3);
    ops::add(s);
    EXPECT_EQ(s.top()->as_integer().v, 5);
    EXPECT_EQ(s.size(), 1u);
}

TEST_F(ArithmeticTest, AddFractions) {
    push_frac(1, 3);
    push_frac(1, 6);
    ops::add(s);
    ASSERT_TRUE(s.top()->is_fraction());
    EXPECT_EQ(s.top()->as_fraction().num, 1);
    EXPECT_EQ(s.top()->as_fraction().den, 2);
}

TEST_F(ArithmeticTest, AddIntAndFraction) {
    push_int(3);
    push_frac(1, 3);
    ops::add(s);
    // Emacs: 3 + 1:3 = (frac 10 3)
    ASSERT_TRUE(s.top()->is_fraction());
    EXPECT_EQ(s.top()->as_fraction().num, 10);
    EXPECT_EQ(s.top()->as_fraction().den, 3);
}

TEST_F(ArithmeticTest, AddIntAndFloat) {
    push_int(3);
    push_float(25, -1);  // 2.5
    ops::add(s);
    // Emacs: 3 + 2.5 = (float 55 -1)
    ASSERT_TRUE(s.top()->is_float());
    EXPECT_EQ(s.top()->as_float().mantissa, 55);
    EXPECT_EQ(s.top()->as_float().exponent, -1);
}

TEST_F(ArithmeticTest, AddFracAndFloat) {
    push_frac(1, 3);
    push_float(25, -1);
    ops::add(s);
    // Emacs: 1:3 + 2.5 = (float 283333333333 -11)
    ASSERT_TRUE(s.top()->is_float());
    EXPECT_EQ(s.top()->as_float().mantissa, mpz_class("283333333333"));
    EXPECT_EQ(s.top()->as_float().exponent, -11);
}

// --- Subtraction ---

TEST_F(ArithmeticTest, SubIntegers) {
    push_int(10);
    push_int(3);
    ops::sub(s);
    EXPECT_EQ(s.top()->as_integer().v, 7);
}

// --- Multiplication ---

TEST_F(ArithmeticTest, MulIntegers) {
    push_int(6);
    push_int(7);
    ops::mul(s);
    EXPECT_EQ(s.top()->as_integer().v, 42);
}

TEST_F(ArithmeticTest, MulFractions) {
    push_frac(2, 3);
    push_frac(3, 4);
    ops::mul(s);
    EXPECT_EQ(s.top()->as_fraction().num, 1);
    EXPECT_EQ(s.top()->as_fraction().den, 2);
}

// --- Division ---

TEST_F(ArithmeticTest, DivIntExact) {
    push_int(10);
    push_int(2);
    ops::div(s);
    // 10/2 = 5 (exact, stays integer)
    ASSERT_TRUE(s.top()->is_integer());
    EXPECT_EQ(s.top()->as_integer().v, 5);
}

TEST_F(ArithmeticTest, DivIntInexactFloat) {
    push_int(10);
    push_int(3);
    ops::div(s);
    // In float mode: 10/3 = float
    ASSERT_TRUE(s.top()->is_float());
}

TEST_F(ArithmeticTest, DivIntInexactFraction) {
    s.state().fraction_mode = FractionMode::Fraction;
    push_int(10);
    push_int(3);
    ops::div(s);
    // In fraction mode: 10/3 = fraction
    ASSERT_TRUE(s.top()->is_fraction());
    EXPECT_EQ(s.top()->as_fraction().num, 10);
    EXPECT_EQ(s.top()->as_fraction().den, 3);
}

TEST_F(ArithmeticTest, DivByZero) {
    push_int(1);
    push_int(0);
    EXPECT_THROW(ops::div(s), std::domain_error);
}

// --- Integer division ---

TEST_F(ArithmeticTest, IntDivPositive) {
    push_int(17);
    push_int(5);
    ops::idiv(s);
    EXPECT_EQ(s.top()->as_integer().v, 3);
}

TEST_F(ArithmeticTest, IntDivNegative) {
    push_int(-17);
    push_int(5);
    ops::idiv(s);
    EXPECT_EQ(s.top()->as_integer().v, -4); // floor division
}

// --- Modulo ---

TEST_F(ArithmeticTest, Mod) {
    push_int(17);
    push_int(5);
    ops::mod(s);
    EXPECT_EQ(s.top()->as_integer().v, 2);
}

// --- Power ---

TEST_F(ArithmeticTest, PowInt) {
    push_int(2);
    push_int(10);
    ops::power(s);
    EXPECT_EQ(s.top()->as_integer().v, 1024);
}

TEST_F(ArithmeticTest, PowNegativeExp) {
    push_int(2);
    push_int(-1);
    ops::power(s);
    ASSERT_TRUE(s.top()->is_fraction());
    EXPECT_EQ(s.top()->as_fraction().num, 1);
    EXPECT_EQ(s.top()->as_fraction().den, 2);
}

TEST_F(ArithmeticTest, PowFraction) {
    push_frac(2, 3);
    push_int(3);
    ops::power(s);
    ASSERT_TRUE(s.top()->is_fraction());
    EXPECT_EQ(s.top()->as_fraction().num, 8);
    EXPECT_EQ(s.top()->as_fraction().den, 27);
}

// Helper: convert top-of-stack DecimalFloat to a double for comparison
static double float_to_double(const ValuePtr& v) {
    auto& f = v->as_float();
    return f.mantissa.get_d() * std::pow(10.0, f.exponent);
}

TEST_F(ArithmeticTest, PowIntBaseFloatExp) {
    push_int(2);
    push_float(5, -1);            // 0.5
    ops::power(s);
    ASSERT_TRUE(s.top()->is_float());
    EXPECT_NEAR(float_to_double(s.top()), std::sqrt(2.0), 1e-10);
}

TEST_F(ArithmeticTest, PowIntBaseFractionExp) {
    push_int(2);
    push_frac(1, 2);              // 1/2
    ops::power(s);
    ASSERT_TRUE(s.top()->is_float());
    EXPECT_NEAR(float_to_double(s.top()), std::sqrt(2.0), 1e-10);
}

TEST_F(ArithmeticTest, PowFloatBaseFloatExp) {
    push_float(25, -1);           // 2.5
    push_float(37, -1);           // 3.7
    ops::power(s);
    ASSERT_TRUE(s.top()->is_float());
    EXPECT_NEAR(float_to_double(s.top()), std::pow(2.5, 3.7), 1e-9);
}

TEST_F(ArithmeticTest, PowIntegerExpFromFloatStaysExact) {
    // 2 ^ 3.0 — the float exponent represents an exact integer (3), so we
    // use the integer fast path and the result stays an exact integer 8
    // rather than an MPFR-rounded float.
    push_int(2);
    push_float(3, 0);             // 3.0
    ops::power(s);
    ASSERT_TRUE(s.top()->is_integer());
    EXPECT_EQ(s.top()->as_integer().v, 8);
}

TEST_F(ArithmeticTest, PowNegativeBaseNonIntegerExpIsNaN) {
    // (-2) ^ 0.5 is complex; without complex support we expect NaN.
    push_int(-2);
    push_float(5, -1);
    ops::power(s);
    ASSERT_TRUE(s.top()->is_infinity());
    EXPECT_EQ(s.top()->as_infinity().kind, Infinity::NaN);
}

// --- Power: cross-platform exponent threshold ---

TEST_F(ArithmeticTest, PowExponentJustBeyondInt64MaxThrows) {
    // INT64_MAX + 1 doesn't fit in int64, so the threshold check
    // throws on every platform. (The previous platform-divergent
    // threshold was tied to mpz_fits_slong_p, which on Windows
    // tripped at 2^31 instead of 2^63.)
    push_int(2);
    s.push(Value::make_integer(mpz_class("9223372036854775808")));
    EXPECT_THROW(ops::power(s), std::overflow_error);
}

TEST_F(ArithmeticTest, PowExponentAtInt64MaxThrowsIsAccepted) {
    // INT64_MAX itself fits in int64, so the threshold check passes.
    // We can't actually compute 1^INT64_MAX without a special-case
    // shortcut, but base=1 short-circuits before the threshold check
    // anyway — this is a smoke test that the call doesn't throw the
    // threshold error.
    push_int(1);
    s.push(Value::make_integer(mpz_class("9223372036854775807")));
    EXPECT_NO_THROW(ops::power(s));
    EXPECT_EQ(s.top()->as_integer().v, 1);
}

// --- Unary ---

TEST_F(ArithmeticTest, Neg) {
    push_int(42);
    ops::neg(s);
    EXPECT_EQ(s.top()->as_integer().v, -42);
}

TEST_F(ArithmeticTest, Inv) {
    push_int(4);
    ops::inv(s);
    ASSERT_TRUE(s.top()->is_fraction());
    EXPECT_EQ(s.top()->as_fraction().num, 1);
    EXPECT_EQ(s.top()->as_fraction().den, 4);
}

TEST_F(ArithmeticTest, Sqrt) {
    push_int(4);
    ops::sqrt_op(s);
    ASSERT_TRUE(s.top()->is_float());
    EXPECT_EQ(s.top()->as_float().mantissa, 2);
    EXPECT_EQ(s.top()->as_float().exponent, 0);
}

TEST_F(ArithmeticTest, AbsNeg) {
    push_int(-42);
    ops::abs_op(s);
    EXPECT_EQ(s.top()->as_integer().v, 42);
}

// --- Rounding ---

TEST_F(ArithmeticTest, Floor) {
    push_float(35, -1); // 3.5
    ops::floor_op(s);
    EXPECT_EQ(s.top()->as_integer().v, 3);
}

TEST_F(ArithmeticTest, Ceil) {
    push_float(35, -1);
    ops::ceil_op(s);
    EXPECT_EQ(s.top()->as_integer().v, 4);
}

TEST_F(ArithmeticTest, Round) {
    push_float(25, -1); // 2.5
    ops::round_op(s);
    EXPECT_EQ(s.top()->as_integer().v, 3);
}

TEST_F(ArithmeticTest, Trunc) {
    push_float(-35, -1);
    ops::trunc_op(s);
    EXPECT_EQ(s.top()->as_integer().v, -3);
}

TEST_F(ArithmeticTest, FloorFraction) {
    push_frac(7, 3); // 2.333...
    ops::floor_op(s);
    EXPECT_EQ(s.top()->as_integer().v, 2);
}

TEST_F(ArithmeticTest, RoundFraction) {
    push_frac(5, 2); // 2.5
    ops::round_op(s);
    EXPECT_EQ(s.top()->as_integer().v, 3); // half away from zero
}

// --- Keep args ---

TEST_F(ArithmeticTest, KeepArgs) {
    push_int(2);
    push_int(3);
    s.state().keep_args_flag = true;
    ops::add(s);
    // Should have: 2, 3, 5
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(s.top(0)->as_integer().v, 5);
    EXPECT_EQ(s.top(1)->as_integer().v, 3);
    EXPECT_EQ(s.top(2)->as_integer().v, 2);
}

// --- Last args ---

TEST_F(ArithmeticTest, LastArgs) {
    push_int(2);
    push_int(3);
    ops::add(s);
    EXPECT_EQ(s.last_args().size(), 2u);
    EXPECT_EQ(s.last_args()[0]->as_integer().v, 2);
    EXPECT_EQ(s.last_args()[1]->as_integer().v, 3);
}

// --- Trail recording ---

TEST_F(ArithmeticTest, TrailRecords) {
    push_int(2);
    push_int(3);
    ops::add(s);
    EXPECT_GE(s.trail().size(), 1u);
    // The last trail entry should be the result
    auto& last = s.trail().at(s.trail().size() - 1);
    EXPECT_EQ(last.tag, "add");
    EXPECT_EQ(last.value->as_integer().v, 5);
}

// --- Undo through arithmetic ---

TEST_F(ArithmeticTest, UndoArithmetic) {
    push_int(2);
    push_int(3);
    ops::add(s);
    EXPECT_EQ(s.size(), 1u);
    EXPECT_EQ(s.top()->as_integer().v, 5);

    s.undo();
    EXPECT_EQ(s.size(), 2u);
    EXPECT_EQ(s.top(0)->as_integer().v, 3);
    EXPECT_EQ(s.top(1)->as_integer().v, 2);
}
