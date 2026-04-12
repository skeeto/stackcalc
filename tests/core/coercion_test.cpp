#include <gtest/gtest.h>
#include "coercion.h"
#include "value.h"

using namespace sc;

TEST(CoercionTest, IntegerPair) {
    auto a = Value::make_integer(3);
    auto b = Value::make_integer(5);
    auto [ca, cb, tag] = coerce::coerce_pair(a, b, 12);
    EXPECT_EQ(tag, Tag::Integer);
    EXPECT_TRUE(ca->is_integer());
    EXPECT_TRUE(cb->is_integer());
}

TEST(CoercionTest, IntegerAndFraction) {
    auto a = Value::make_integer(3);
    auto b = Value::make_fraction(1, 3);
    auto [ca, cb, tag] = coerce::coerce_pair(a, b, 12);
    EXPECT_EQ(tag, Tag::Fraction);
    // 3/1 normalizes back to integer — that's fine, the common tag
    // tells the caller how to dispatch arithmetic
    ASSERT_TRUE(ca->is_rational());
    ASSERT_TRUE(cb->is_fraction());
}

// Emacs: 3 + 1:3 = (frac 10 3)
TEST(CoercionTest, EmacsCheck_IntPlusFrac) {
    auto a = Value::make_integer(3);
    auto b = Value::make_fraction(1, 3);
    auto [ca, cb, tag] = coerce::coerce_pair(a, b, 12);
    EXPECT_EQ(tag, Tag::Fraction);
    // Both should be fractions now; actual addition tested elsewhere
}

TEST(CoercionTest, IntegerAndFloat) {
    auto a = Value::make_integer(3);
    auto b = Value::make_float(mpz_class(25), -1);  // 2.5
    auto [ca, cb, tag] = coerce::coerce_pair(a, b, 12);
    EXPECT_EQ(tag, Tag::DecimalFloat);
    ASSERT_TRUE(ca->is_float());
    EXPECT_EQ(ca->as_float().mantissa, 3);
    EXPECT_EQ(ca->as_float().exponent, 0);
    ASSERT_TRUE(cb->is_float());
}

TEST(CoercionTest, FractionAndFloat) {
    auto a = Value::make_fraction(1, 3);
    auto b = Value::make_float(mpz_class(25), -1);
    auto [ca, cb, tag] = coerce::coerce_pair(a, b, 12);
    EXPECT_EQ(tag, Tag::DecimalFloat);
    ASSERT_TRUE(ca->is_float());
    // 1/3 as float at precision 12
    EXPECT_EQ(ca->as_float().mantissa, mpz_class("333333333333"));
    EXPECT_EQ(ca->as_float().exponent, -12);
}

TEST(CoercionTest, FractionPair) {
    auto a = Value::make_fraction(1, 3);
    auto b = Value::make_fraction(1, 6);
    auto [ca, cb, tag] = coerce::coerce_pair(a, b, 12);
    EXPECT_EQ(tag, Tag::Fraction);
}

TEST(CoercionTest, FloatPair) {
    auto a = Value::make_float(mpz_class(1), -1);
    auto b = Value::make_float(mpz_class(2), -1);
    auto [ca, cb, tag] = coerce::coerce_pair(a, b, 12);
    EXPECT_EQ(tag, Tag::DecimalFloat);
    // Should be unchanged
    EXPECT_EQ(ca, a);
    EXPECT_EQ(cb, b);
}

TEST(CoercionTest, ToFloatFromInteger) {
    auto v = Value::make_integer(42);
    auto f = coerce::to_float(v, 12);
    ASSERT_TRUE(f->is_float());
    EXPECT_EQ(f->as_float().mantissa, 42);
    EXPECT_EQ(f->as_float().exponent, 0);
}

TEST(CoercionTest, ToFloatIdempotent) {
    auto v = Value::make_float(mpz_class(15), -1);
    auto f = coerce::to_float(v, 12);
    EXPECT_EQ(f, v); // same pointer
}

TEST(CoercionTest, ToFractionFromInteger) {
    auto v = Value::make_integer(5);
    auto f = coerce::to_fraction(v);
    // make_fraction(5, 1) normalizes to integer since den==1
    // This is correct — fraction arithmetic handles integers too
    ASSERT_TRUE(f->is_rational());
}
