#include <gtest/gtest.h>
#include "decimal_float.h"
#include "value.h"

using namespace sc;

// --- Normalization tests ---

TEST(DecimalFloatTest, MakeStripTrailingZeros) {
    auto v = Value::make_float(mpz_class(1500), -2);
    ASSERT_TRUE(v->is_float());
    // 1500 * 10^-2 = 15.00 -> mantissa=15, exp=0
    EXPECT_EQ(v->as_float().mantissa, 15);
    EXPECT_EQ(v->as_float().exponent, 0);
}

TEST(DecimalFloatTest, MakeZero) {
    auto v = Value::make_float(mpz_class(0), 5);
    ASSERT_TRUE(v->is_float());
    EXPECT_EQ(v->as_float().mantissa, 0);
    EXPECT_EQ(v->as_float().exponent, 0);
}

TEST(DecimalFloatTest, MakeNoTrailingZeros) {
    auto v = Value::make_float(mpz_class(123), -2);
    ASSERT_TRUE(v->is_float());
    EXPECT_EQ(v->as_float().mantissa, 123);
    EXPECT_EQ(v->as_float().exponent, -2);
}

// Emacs: 0.1 = (float 1 -1)
TEST(DecimalFloatTest, EmacsCheck_PointOne) {
    auto v = Value::make_float(mpz_class(1), -1);
    EXPECT_EQ(v->as_float().mantissa, 1);
    EXPECT_EQ(v->as_float().exponent, -1);
}

// Emacs: 1.5 = (float 15 -1)
TEST(DecimalFloatTest, EmacsCheck_OnePointFive) {
    auto v = Value::make_float(mpz_class(15), -1);
    EXPECT_EQ(v->as_float().mantissa, 15);
    EXPECT_EQ(v->as_float().exponent, -1);
}

// Emacs: 1.0 = (float 1 0)
TEST(DecimalFloatTest, EmacsCheck_OnePointZero) {
    auto v = Value::make_float(mpz_class(10), -1);
    EXPECT_EQ(v->as_float().mantissa, 1);
    EXPECT_EQ(v->as_float().exponent, 0);
}

// Emacs: 100.0 = (float 1 2)
TEST(DecimalFloatTest, EmacsCheck_Hundred) {
    auto v = Value::make_float(mpz_class(100), 0);
    EXPECT_EQ(v->as_float().mantissa, 1);
    EXPECT_EQ(v->as_float().exponent, 2);
}

// --- Normalized construction ---

TEST(DecimalFloatTest, NormalizedTruncates) {
    // 12345 at precision 3 should round to 123 * 10^2 -> stripped: 123e2
    auto v = Value::make_float_normalized(mpz_class(12345), 0, 3);
    EXPECT_EQ(v->as_float().mantissa, 123); // 12345 -> rounds to 12300 -> 123e2
    EXPECT_EQ(v->as_float().exponent, 2);
}

TEST(DecimalFloatTest, NormalizedRoundsUp) {
    // 12350 at precision 3: 12350 + 50 = 12400, /100 = 124 -> (124, 2) -> strip: (124, 2)
    auto v = Value::make_float_normalized(mpz_class(12350), 0, 3);
    EXPECT_EQ(v->as_float().mantissa, 124); // rounds half away from zero
    EXPECT_EQ(v->as_float().exponent, 2);
}

TEST(DecimalFloatTest, NormalizedNegativeRoundsAwayFromZero) {
    // -12350 at precision 3 should round to -124 * 10^2
    auto v = Value::make_float_normalized(mpz_class(-12350), 0, 3);
    EXPECT_EQ(v->as_float().mantissa, -124);
    EXPECT_EQ(v->as_float().exponent, 2);
}

// --- Arithmetic ---

// Emacs: 1.5 + 2.3 = (float 38 -1)
TEST(DecimalFloatTest, EmacsCheck_Add) {
    DecimalFloat a{mpz_class(15), -1};  // 1.5
    DecimalFloat b{mpz_class(23), -1};  // 2.3
    auto r = decimal_float::add(a, b, 12);
    EXPECT_EQ(r->as_float().mantissa, 38);
    EXPECT_EQ(r->as_float().exponent, -1);
}

// Emacs: 0.1 + 0.2 = (float 3 -1)
TEST(DecimalFloatTest, EmacsCheck_PointOnePointTwo) {
    DecimalFloat a{mpz_class(1), -1};  // 0.1
    DecimalFloat b{mpz_class(2), -1};  // 0.2
    auto r = decimal_float::add(a, b, 12);
    EXPECT_EQ(r->as_float().mantissa, 3);
    EXPECT_EQ(r->as_float().exponent, -1);
}

TEST(DecimalFloatTest, Sub) {
    DecimalFloat a{mpz_class(38), -1};
    DecimalFloat b{mpz_class(15), -1};
    auto r = decimal_float::sub(a, b, 12);
    EXPECT_EQ(r->as_float().mantissa, 23);
    EXPECT_EQ(r->as_float().exponent, -1);
}

TEST(DecimalFloatTest, Mul) {
    DecimalFloat a{mpz_class(15), -1};  // 1.5
    DecimalFloat b{mpz_class(2), 0};    // 2.0
    auto r = decimal_float::mul(a, b, 12);
    EXPECT_EQ(r->as_float().mantissa, 3);
    EXPECT_EQ(r->as_float().exponent, 0);
}

// Emacs: 1.0 / 3.0 = (float 333333333333 -12)
TEST(DecimalFloatTest, EmacsCheck_DivOneThird) {
    DecimalFloat a{mpz_class(1), 0};
    DecimalFloat b{mpz_class(3), 0};
    auto r = decimal_float::div(a, b, 12);
    EXPECT_EQ(r->as_float().mantissa, mpz_class("333333333333"));
    EXPECT_EQ(r->as_float().exponent, -12);
}

// Emacs: 2.0 / 3.0 = (float 666666666667 -12)
TEST(DecimalFloatTest, EmacsCheck_DivTwoThirds) {
    DecimalFloat a{mpz_class(2), 0};
    DecimalFloat b{mpz_class(3), 0};
    auto r = decimal_float::div(a, b, 12);
    EXPECT_EQ(r->as_float().mantissa, mpz_class("666666666667"));
    EXPECT_EQ(r->as_float().exponent, -12);
}

TEST(DecimalFloatTest, DivByZero) {
    DecimalFloat a{mpz_class(1), 0};
    DecimalFloat b{mpz_class(0), 0};
    EXPECT_THROW(decimal_float::div(a, b, 12), std::domain_error);
}

TEST(DecimalFloatTest, Neg) {
    DecimalFloat a{mpz_class(15), -1};
    auto r = decimal_float::neg(a);
    EXPECT_EQ(r->as_float().mantissa, -15);
    EXPECT_EQ(r->as_float().exponent, -1);
}

TEST(DecimalFloatTest, Abs) {
    DecimalFloat a{mpz_class(-15), -1};
    auto r = decimal_float::abs(a);
    EXPECT_EQ(r->as_float().mantissa, 15);
    EXPECT_EQ(r->as_float().exponent, -1);
}

// --- Rounding ---

// Emacs: 2.5 rounded = 3
TEST(DecimalFloatTest, EmacsCheck_RoundHalfUp) {
    DecimalFloat a{mpz_class(25), -1};
    auto r = decimal_float::round(a);
    ASSERT_TRUE(r->is_integer());
    EXPECT_EQ(r->as_integer().v, 3);
}

// Emacs: 3.5 rounded = 4
TEST(DecimalFloatTest, EmacsCheck_Round3_5) {
    DecimalFloat a{mpz_class(35), -1};
    auto r = decimal_float::round(a);
    EXPECT_EQ(r->as_integer().v, 4);
}

// Emacs: -2.5 rounded = -3
TEST(DecimalFloatTest, EmacsCheck_RoundNegHalf) {
    DecimalFloat a{mpz_class(-25), -1};
    auto r = decimal_float::round(a);
    EXPECT_EQ(r->as_integer().v, -3);
}

TEST(DecimalFloatTest, Floor) {
    DecimalFloat a{mpz_class(35), -1};  // 3.5
    auto r = decimal_float::floor(a);
    EXPECT_EQ(r->as_integer().v, 3);
}

TEST(DecimalFloatTest, FloorNegative) {
    DecimalFloat a{mpz_class(-35), -1};  // -3.5
    auto r = decimal_float::floor(a);
    EXPECT_EQ(r->as_integer().v, -4);
}

TEST(DecimalFloatTest, Ceil) {
    DecimalFloat a{mpz_class(35), -1};
    auto r = decimal_float::ceil(a);
    EXPECT_EQ(r->as_integer().v, 4);
}

TEST(DecimalFloatTest, CeilNegative) {
    DecimalFloat a{mpz_class(-35), -1};
    auto r = decimal_float::ceil(a);
    EXPECT_EQ(r->as_integer().v, -3);
}

TEST(DecimalFloatTest, Trunc) {
    DecimalFloat a{mpz_class(35), -1};
    auto r = decimal_float::trunc(a);
    EXPECT_EQ(r->as_integer().v, 3);
}

TEST(DecimalFloatTest, TruncNegative) {
    DecimalFloat a{mpz_class(-35), -1};
    auto r = decimal_float::trunc(a);
    EXPECT_EQ(r->as_integer().v, -3);
}

TEST(DecimalFloatTest, FloorInteger) {
    DecimalFloat a{mpz_class(5), 0};
    auto r = decimal_float::floor(a);
    EXPECT_EQ(r->as_integer().v, 5);
}

// --- Power ---

TEST(DecimalFloatTest, PowInt) {
    DecimalFloat a{mpz_class(2), 0};
    auto r = decimal_float::pow_int(a, 10, 12);
    EXPECT_EQ(r->as_float().mantissa, 1024);
}

TEST(DecimalFloatTest, PowZero) {
    DecimalFloat a{mpz_class(42), 0};
    auto r = decimal_float::pow_int(a, 0, 12);
    EXPECT_EQ(r->as_float().mantissa, 1);
    EXPECT_EQ(r->as_float().exponent, 0);
}

// --- Square root ---

TEST(DecimalFloatTest, Sqrt) {
    DecimalFloat a{mpz_class(4), 0};
    auto r = decimal_float::sqrt(a, 12);
    EXPECT_EQ(r->as_float().mantissa, 2);
    EXPECT_EQ(r->as_float().exponent, 0);
}

TEST(DecimalFloatTest, SqrtTwo) {
    DecimalFloat a{mpz_class(2), 0};
    auto r = decimal_float::sqrt(a, 12);
    // sqrt(2) ≈ 1.41421356237
    EXPECT_EQ(r->as_float().mantissa, mpz_class("141421356237"));
    EXPECT_EQ(r->as_float().exponent, -11);
}

TEST(DecimalFloatTest, SqrtNegative) {
    DecimalFloat a{mpz_class(-1), 0};
    EXPECT_THROW(decimal_float::sqrt(a, 12), std::domain_error);
}

// --- Comparison ---

TEST(DecimalFloatTest, Cmp) {
    DecimalFloat a{mpz_class(15), -1};  // 1.5
    DecimalFloat b{mpz_class(2), 0};    // 2.0
    EXPECT_LT(decimal_float::cmp(a, b), 0);
    EXPECT_GT(decimal_float::cmp(b, a), 0);
    EXPECT_EQ(decimal_float::cmp(a, a), 0);
}

TEST(DecimalFloatTest, CmpDifferentExponents) {
    DecimalFloat a{mpz_class(1), 2};     // 100
    DecimalFloat b{mpz_class(999), -1};  // 99.9
    EXPECT_GT(decimal_float::cmp(a, b), 0);
}

// --- Conversion ---

TEST(DecimalFloatTest, FromInteger) {
    Integer i{mpz_class(42)};
    auto df = decimal_float::from_integer(i);
    EXPECT_EQ(df.mantissa, 42);
    EXPECT_EQ(df.exponent, 0);
}

TEST(DecimalFloatTest, ToInteger) {
    DecimalFloat f{mpz_class(42), 0};
    auto i = decimal_float::to_integer(f);
    ASSERT_TRUE(i.has_value());
    EXPECT_EQ(*i, 42);
}

TEST(DecimalFloatTest, ToIntegerWithExponent) {
    DecimalFloat f{mpz_class(5), 2};  // 500
    auto i = decimal_float::to_integer(f);
    ASSERT_TRUE(i.has_value());
    EXPECT_EQ(*i, 500);
}

TEST(DecimalFloatTest, ToIntegerFractional) {
    DecimalFloat f{mpz_class(15), -1};  // 1.5
    auto i = decimal_float::to_integer(f);
    EXPECT_FALSE(i.has_value());
}

TEST(DecimalFloatTest, NumDigits) {
    EXPECT_EQ(decimal_float::num_digits(mpz_class(0)), 0);
    EXPECT_EQ(decimal_float::num_digits(mpz_class(1)), 1);
    EXPECT_EQ(decimal_float::num_digits(mpz_class(9)), 1);
    EXPECT_EQ(decimal_float::num_digits(mpz_class(10)), 2);
    EXPECT_EQ(decimal_float::num_digits(mpz_class(99)), 2);
    EXPECT_EQ(decimal_float::num_digits(mpz_class(100)), 3);
    EXPECT_EQ(decimal_float::num_digits(mpz_class(999)), 3);
    EXPECT_EQ(decimal_float::num_digits(mpz_class(-42)), 2);
}
