#include <gtest/gtest.h>
#include "interval.hpp"
#include "arithmetic.hpp"

using namespace sc;

TEST(IntervalTest, AddClosed) {
    // [1..3] + [2..4] = [3..7]
    Interval a{3, Value::make_integer(1), Value::make_integer(3)};
    Interval b{3, Value::make_integer(2), Value::make_integer(4)};
    auto r = interval::add(a, b, 12);
    ASSERT_TRUE(r->is_interval());
    auto& iv = r->as_interval();
    EXPECT_EQ(iv.mask, 3); // both closed
    EXPECT_EQ(iv.lo->as_integer().v, 3);
    EXPECT_EQ(iv.hi->as_integer().v, 7);
}

TEST(IntervalTest, SubClosed) {
    // [5..10] - [1..3] = [5-3 .. 10-1] = [2..9]
    Interval a{3, Value::make_integer(5), Value::make_integer(10)};
    Interval b{3, Value::make_integer(1), Value::make_integer(3)};
    auto r = interval::sub(a, b, 12);
    ASSERT_TRUE(r->is_interval());
    auto& iv = r->as_interval();
    EXPECT_EQ(iv.mask, 3);
    EXPECT_EQ(iv.lo->as_integer().v, 2);
    EXPECT_EQ(iv.hi->as_integer().v, 9);
}

TEST(IntervalTest, MulPositive) {
    // [2..3] * [4..5] = [8..15]
    Interval a{3, Value::make_integer(2), Value::make_integer(3)};
    Interval b{3, Value::make_integer(4), Value::make_integer(5)};
    auto r = interval::mul(a, b, 12);
    ASSERT_TRUE(r->is_interval());
    auto& iv = r->as_interval();
    EXPECT_EQ(iv.lo->as_integer().v, 8);
    EXPECT_EQ(iv.hi->as_integer().v, 15);
}

TEST(IntervalTest, MulMixed) {
    // [-2..3] * [1..4] = min(-8,-2,3,12)..max(-8,-2,3,12) = [-8..12]
    Interval a{3, Value::make_integer(-2), Value::make_integer(3)};
    Interval b{3, Value::make_integer(1), Value::make_integer(4)};
    auto r = interval::mul(a, b, 12);
    ASSERT_TRUE(r->is_interval());
    auto& iv = r->as_interval();
    EXPECT_EQ(iv.lo->as_integer().v, -8);
    EXPECT_EQ(iv.hi->as_integer().v, 12);
}

TEST(IntervalTest, DivSimple) {
    // [6..12] / [2..3] = [6/3..12/2] = [2..6]
    Interval a{3, Value::make_integer(6), Value::make_integer(12)};
    Interval b{3, Value::make_integer(2), Value::make_integer(3)};
    auto r = interval::div(a, b, 12);
    ASSERT_TRUE(r->is_interval());
    auto& iv = r->as_interval();
    EXPECT_EQ(iv.lo->as_integer().v, 2);
    EXPECT_EQ(iv.hi->as_integer().v, 6);
}

TEST(IntervalTest, DivByZeroThrows) {
    Interval a{3, Value::make_integer(1), Value::make_integer(2)};
    Interval b{3, Value::make_integer(-1), Value::make_integer(1)};
    EXPECT_THROW(interval::div(a, b, 12), std::domain_error);
}

TEST(IntervalTest, Neg) {
    // -[2..5] = [-5..-2], open/closed swap
    Interval a{3, Value::make_integer(2), Value::make_integer(5)};
    auto r = interval::neg(a);
    ASSERT_TRUE(r->is_interval());
    auto& iv = r->as_interval();
    EXPECT_EQ(iv.lo->as_integer().v, -5);
    EXPECT_EQ(iv.hi->as_integer().v, -2);
    EXPECT_EQ(iv.mask, 3); // both still closed (symmetric)
}

TEST(IntervalTest, NegMaskSwap) {
    // -[2..5) should become (-5..2]
    // mask=2 means [lo..hi), after neg: (-hi..-lo] = mask should have lo open, hi closed
    Interval a{2, Value::make_integer(2), Value::make_integer(5)};
    auto r = interval::neg(a);
    ASSERT_TRUE(r->is_interval());
    auto& iv = r->as_interval();
    EXPECT_EQ(iv.lo->as_integer().v, -5);
    EXPECT_EQ(iv.hi->as_integer().v, -2);
    // Original mask=2 (lo closed, hi open). After neg, lo was hi (open) -> bit1=0, hi was lo (closed) -> bit0=1
    EXPECT_EQ(iv.mask, 1); // (lo..hi]
}

TEST(IntervalTest, AbsBothPositive) {
    // |[2..5]| = [2..5]
    Interval a{3, Value::make_integer(2), Value::make_integer(5)};
    auto r = interval::abs(a, 12);
    ASSERT_TRUE(r->is_interval());
    auto& iv = r->as_interval();
    EXPECT_EQ(iv.lo->as_integer().v, 2);
    EXPECT_EQ(iv.hi->as_integer().v, 5);
}

TEST(IntervalTest, AbsBothNegative) {
    // |[-5..-2]| = [2..5]
    Interval a{3, Value::make_integer(-5), Value::make_integer(-2)};
    auto r = interval::abs(a, 12);
    ASSERT_TRUE(r->is_interval());
    auto& iv = r->as_interval();
    EXPECT_EQ(iv.lo->as_integer().v, 2);
    EXPECT_EQ(iv.hi->as_integer().v, 5);
}

TEST(IntervalTest, AbsSpansZero) {
    // |[-3..5]| = [0..5]
    Interval a{3, Value::make_integer(-3), Value::make_integer(5)};
    auto r = interval::abs(a, 12);
    ASSERT_TRUE(r->is_interval());
    auto& iv = r->as_interval();
    EXPECT_TRUE(iv.lo->is_zero());
    EXPECT_EQ(iv.hi->as_integer().v, 5);
}

TEST(IntervalTest, ArithDispatch) {
    auto a = Value::make_interval(3, Value::make_integer(1), Value::make_integer(3));
    auto b = Value::make_interval(3, Value::make_integer(2), Value::make_integer(4));
    auto r = arith::add(a, b, 12);
    ASSERT_TRUE(r->is_interval());
    EXPECT_EQ(r->as_interval().lo->as_integer().v, 3);
    EXPECT_EQ(r->as_interval().hi->as_integer().v, 7);
}

// --- Power ---

TEST(IntervalTest, PowZero) {
    Interval a{3, Value::make_integer(2), Value::make_integer(5)};
    auto r = interval::pow(a, 0, 12);
    ASSERT_TRUE(r->is_interval());
    EXPECT_EQ(r->as_interval().lo->as_integer().v, 1);
    EXPECT_EQ(r->as_interval().hi->as_integer().v, 1);
}

TEST(IntervalTest, PowPositive) {
    // [2..3]^3 = [8..27]
    Interval a{3, Value::make_integer(2), Value::make_integer(3)};
    auto r = interval::pow(a, 3, 12);
    ASSERT_TRUE(r->is_interval());
    auto& iv = r->as_interval();
    EXPECT_EQ(iv.lo->as_integer().v, 8);
    EXPECT_EQ(iv.hi->as_integer().v, 27);
}

TEST(IntervalTest, PowSpansZeroEven) {
    // [-2..3] * [-2..3] worst-case from corner products = [-6..9]
    Interval a{3, Value::make_integer(-2), Value::make_integer(3)};
    auto r = interval::pow(a, 2, 12);
    auto& iv = r->as_interval();
    EXPECT_EQ(iv.lo->as_integer().v, -6);
    EXPECT_EQ(iv.hi->as_integer().v, 9);
}

TEST(IntervalTest, ArithPowerDispatch) {
    auto a = Value::make_interval(3, Value::make_integer(2), Value::make_integer(3));
    auto r = arith::power(a, Value::make_integer(3), 12);
    ASSERT_TRUE(r->is_interval());
    EXPECT_EQ(r->as_interval().lo->as_integer().v, 8);
    EXPECT_EQ(r->as_interval().hi->as_integer().v, 27);
}
