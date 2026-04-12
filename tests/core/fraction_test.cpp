#include <gtest/gtest.h>
#include "fraction.h"
#include "value.h"

using namespace sc;

TEST(FractionTest, MakeReduced) {
    auto v = Value::make_fraction(4, 6);
    ASSERT_TRUE(v->is_fraction());
    EXPECT_EQ(v->as_fraction().num, 2);
    EXPECT_EQ(v->as_fraction().den, 3);
}

TEST(FractionTest, MakeNormalizesSign) {
    auto v = Value::make_fraction(3, -5);
    ASSERT_TRUE(v->is_fraction());
    EXPECT_EQ(v->as_fraction().num, -3);
    EXPECT_EQ(v->as_fraction().den, 5);
}

TEST(FractionTest, MakeBecomesInteger) {
    auto v = Value::make_fraction(6, 3);
    ASSERT_TRUE(v->is_integer());
    EXPECT_EQ(v->as_integer().v, 2);
}

TEST(FractionTest, MakeZero) {
    auto v = Value::make_fraction(0, 5);
    ASSERT_TRUE(v->is_integer());
    EXPECT_EQ(v->as_integer().v, 0);
}

TEST(FractionTest, MakeZeroDenThrows) {
    EXPECT_THROW(Value::make_fraction(1, 0), std::domain_error);
}

TEST(FractionTest, Add) {
    Fraction a{mpz_class(1), mpz_class(3)};
    Fraction b{mpz_class(1), mpz_class(6)};
    auto r = fraction::add(a, b);
    ASSERT_TRUE(r->is_fraction());
    EXPECT_EQ(r->as_fraction().num, 1);
    EXPECT_EQ(r->as_fraction().den, 2);
}

TEST(FractionTest, AddToInteger) {
    // 2/3 + 1/3 = 1 (integer)
    Fraction a{mpz_class(2), mpz_class(3)};
    Fraction b{mpz_class(1), mpz_class(3)};
    auto r = fraction::add(a, b);
    ASSERT_TRUE(r->is_integer());
    EXPECT_EQ(r->as_integer().v, 1);
}

TEST(FractionTest, Sub) {
    Fraction a{mpz_class(5), mpz_class(6)};
    Fraction b{mpz_class(1), mpz_class(3)};
    auto r = fraction::sub(a, b);
    ASSERT_TRUE(r->is_fraction());
    EXPECT_EQ(r->as_fraction().num, 1);
    EXPECT_EQ(r->as_fraction().den, 2);
}

TEST(FractionTest, Mul) {
    Fraction a{mpz_class(2), mpz_class(3)};
    Fraction b{mpz_class(3), mpz_class(4)};
    auto r = fraction::mul(a, b);
    ASSERT_TRUE(r->is_fraction());
    EXPECT_EQ(r->as_fraction().num, 1);
    EXPECT_EQ(r->as_fraction().den, 2);
}

TEST(FractionTest, Div) {
    Fraction a{mpz_class(2), mpz_class(3)};
    Fraction b{mpz_class(4), mpz_class(5)};
    auto r = fraction::div(a, b);
    ASSERT_TRUE(r->is_fraction());
    EXPECT_EQ(r->as_fraction().num, 5);
    EXPECT_EQ(r->as_fraction().den, 6);
}

TEST(FractionTest, DivByZero) {
    Fraction a{mpz_class(1), mpz_class(2)};
    Fraction b{mpz_class(0), mpz_class(1)};
    EXPECT_THROW(fraction::div(a, b), std::domain_error);
}

TEST(FractionTest, Neg) {
    Fraction a{mpz_class(3), mpz_class(4)};
    auto r = fraction::neg(a);
    ASSERT_TRUE(r->is_fraction());
    EXPECT_EQ(r->as_fraction().num, -3);
    EXPECT_EQ(r->as_fraction().den, 4);
}

TEST(FractionTest, Reciprocal) {
    Fraction a{mpz_class(3), mpz_class(4)};
    auto r = fraction::reciprocal(a);
    ASSERT_TRUE(r->is_fraction());
    EXPECT_EQ(r->as_fraction().num, 4);
    EXPECT_EQ(r->as_fraction().den, 3);
}

TEST(FractionTest, ToFloat) {
    Fraction a{mpz_class(1), mpz_class(3)};
    auto r = fraction::to_float(a, 12);
    ASSERT_TRUE(r->is_float());
    // 1/3 at precision 12 = (float 333333333333 -12)
    EXPECT_EQ(r->as_float().mantissa, mpz_class("333333333333"));
    EXPECT_EQ(r->as_float().exponent, -12);
}

TEST(FractionTest, ToFloatExact) {
    Fraction a{mpz_class(1), mpz_class(4)};
    auto r = fraction::to_float(a, 12);
    ASSERT_TRUE(r->is_float());
    // 1/4 = 0.25 = (float 25 -2)
    EXPECT_EQ(r->as_float().mantissa, mpz_class("25"));
    EXPECT_EQ(r->as_float().exponent, -2);
}

TEST(FractionTest, Cmp) {
    Fraction a{mpz_class(1), mpz_class(3)};
    Fraction b{mpz_class(1), mpz_class(2)};
    EXPECT_LT(fraction::cmp(a, b), 0);
    EXPECT_GT(fraction::cmp(b, a), 0);
    EXPECT_EQ(fraction::cmp(a, a), 0);
}

TEST(FractionTest, FloorDiv) {
    Fraction a{mpz_class(7), mpz_class(2)};  // 3.5
    Fraction b{mpz_class(3), mpz_class(2)};  // 1.5
    auto r = fraction::fdiv(a, b);
    ASSERT_TRUE(r->is_integer());
    EXPECT_EQ(r->as_integer().v, 2); // floor(3.5 / 1.5) = floor(2.333) = 2
}

// Cross-check with Emacs: 2/3 + 1/6 = 5/6
TEST(FractionTest, EmacsCheck_AddFractions) {
    Fraction a{mpz_class(2), mpz_class(3)};
    Fraction b{mpz_class(1), mpz_class(6)};
    auto r = fraction::add(a, b);
    ASSERT_TRUE(r->is_fraction());
    EXPECT_EQ(r->as_fraction().num, 5);
    EXPECT_EQ(r->as_fraction().den, 6);
}
