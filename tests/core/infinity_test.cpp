#include <gtest/gtest.h>
#include "infinity.hpp"
#include "arithmetic.hpp"

using namespace sc;

TEST(InfinityTest, PosInfPlusFinite) {
    auto r = infinity_ops::add(Infinity{Infinity::Pos}, Value::make_integer(42));
    ASSERT_TRUE(r->is_infinity());
    EXPECT_EQ(r->as_infinity().kind, Infinity::Pos);
}

TEST(InfinityTest, NegInfPlusFinite) {
    auto r = infinity_ops::add(Infinity{Infinity::Neg}, Value::make_integer(42));
    ASSERT_TRUE(r->is_infinity());
    EXPECT_EQ(r->as_infinity().kind, Infinity::Neg);
}

TEST(InfinityTest, InfPlusInfSameSign) {
    auto r = infinity_ops::add(Infinity{Infinity::Pos}, Value::make_infinity(Infinity::Pos));
    ASSERT_TRUE(r->is_infinity());
    EXPECT_EQ(r->as_infinity().kind, Infinity::Pos);
}

TEST(InfinityTest, InfPlusInfOppositeSign) {
    // inf + (-inf) = nan
    auto r = infinity_ops::add(Infinity{Infinity::Pos}, Value::make_infinity(Infinity::Neg));
    ASSERT_TRUE(r->is_infinity());
    EXPECT_EQ(r->as_infinity().kind, Infinity::NaN);
}

TEST(InfinityTest, MulPosInfPositive) {
    auto r = infinity_ops::mul(Infinity{Infinity::Pos}, Value::make_integer(5));
    ASSERT_TRUE(r->is_infinity());
    EXPECT_EQ(r->as_infinity().kind, Infinity::Pos);
}

TEST(InfinityTest, MulPosInfNegative) {
    auto r = infinity_ops::mul(Infinity{Infinity::Pos}, Value::make_integer(-5));
    ASSERT_TRUE(r->is_infinity());
    EXPECT_EQ(r->as_infinity().kind, Infinity::Neg);
}

TEST(InfinityTest, MulInfZero) {
    // inf * 0 = nan
    auto r = infinity_ops::mul(Infinity{Infinity::Pos}, Value::zero());
    ASSERT_TRUE(r->is_infinity());
    EXPECT_EQ(r->as_infinity().kind, Infinity::NaN);
}

TEST(InfinityTest, MulInfInfSameSign) {
    auto r = infinity_ops::mul(Infinity{Infinity::Pos}, Value::make_infinity(Infinity::Pos));
    ASSERT_TRUE(r->is_infinity());
    EXPECT_EQ(r->as_infinity().kind, Infinity::Pos);
}

TEST(InfinityTest, MulInfInfOppositeSign) {
    auto r = infinity_ops::mul(Infinity{Infinity::Pos}, Value::make_infinity(Infinity::Neg));
    ASSERT_TRUE(r->is_infinity());
    EXPECT_EQ(r->as_infinity().kind, Infinity::Neg);
}

TEST(InfinityTest, NegPos) {
    auto r = infinity_ops::neg(Infinity{Infinity::Pos});
    ASSERT_TRUE(r->is_infinity());
    EXPECT_EQ(r->as_infinity().kind, Infinity::Neg);
}

TEST(InfinityTest, NegNeg) {
    auto r = infinity_ops::neg(Infinity{Infinity::Neg});
    ASSERT_TRUE(r->is_infinity());
    EXPECT_EQ(r->as_infinity().kind, Infinity::Pos);
}

TEST(InfinityTest, NegNaN) {
    auto r = infinity_ops::neg(Infinity{Infinity::NaN});
    ASSERT_TRUE(r->is_infinity());
    EXPECT_EQ(r->as_infinity().kind, Infinity::NaN);
}

TEST(InfinityTest, NegUndirected) {
    auto r = infinity_ops::neg(Infinity{Infinity::Undirected});
    ASSERT_TRUE(r->is_infinity());
    EXPECT_EQ(r->as_infinity().kind, Infinity::Undirected);
}

TEST(InfinityTest, ArithDispatch) {
    auto a = Value::make_infinity(Infinity::Pos);
    auto b = Value::make_integer(10);
    auto r = arith::add(a, b, 12);
    ASSERT_TRUE(r->is_infinity());
    EXPECT_EQ(r->as_infinity().kind, Infinity::Pos);
}
