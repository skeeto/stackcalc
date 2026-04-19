#include <gtest/gtest.h>
#include "hms.hpp"

using namespace sc;

TEST(HMSTest, AddSimple) {
    auto a = Value::make_hms(1, 30, Value::make_integer(0));
    auto b = Value::make_hms(2, 45, Value::make_integer(0));
    auto r = hms::add(a->as_hms(), b->as_hms(), 12);
    ASSERT_TRUE(r->is_hms());
    EXPECT_EQ(r->as_hms().hours, 4);
    EXPECT_EQ(r->as_hms().minutes, 15);
    EXPECT_TRUE(r->as_hms().seconds->is_zero());
}

TEST(HMSTest, AddWithCarry) {
    auto a = Value::make_hms(1, 50, Value::make_integer(30));
    auto b = Value::make_hms(0, 20, Value::make_integer(45));
    auto r = hms::add(a->as_hms(), b->as_hms(), 12);
    ASSERT_TRUE(r->is_hms());
    EXPECT_EQ(r->as_hms().hours, 2);
    EXPECT_EQ(r->as_hms().minutes, 11);
    EXPECT_EQ(r->as_hms().seconds->as_integer().v, 15);
}

TEST(HMSTest, Sub) {
    auto a = Value::make_hms(2, 30, Value::make_integer(0));
    auto b = Value::make_hms(1, 45, Value::make_integer(0));
    auto r = hms::sub(a->as_hms(), b->as_hms(), 12);
    ASSERT_TRUE(r->is_hms());
    EXPECT_EQ(r->as_hms().hours, 0);
    EXPECT_EQ(r->as_hms().minutes, 45);
}

TEST(HMSTest, MulScalar) {
    auto a = Value::make_hms(1, 30, Value::make_integer(0));
    auto r = hms::mul_scalar(a->as_hms(), Value::make_integer(2), 12);
    ASSERT_TRUE(r->is_hms());
    EXPECT_EQ(r->as_hms().hours, 3);
    EXPECT_EQ(r->as_hms().minutes, 0);
}

TEST(HMSTest, ToDegrees) {
    // 2@ 30' 0" = 2.5 degrees
    auto a = Value::make_hms(2, 30, Value::make_integer(0));
    auto r = hms::to_degrees(a->as_hms(), 12);
    ASSERT_TRUE(r->is_float());
    EXPECT_EQ(r->as_float().mantissa, 25);
    EXPECT_EQ(r->as_float().exponent, -1);
}

TEST(HMSTest, Neg) {
    auto a = Value::make_hms(2, 30, Value::make_integer(15));
    auto r = hms::neg(a->as_hms());
    ASSERT_TRUE(r->is_hms());
    EXPECT_EQ(r->as_hms().hours, -2);
    EXPECT_EQ(r->as_hms().minutes, 30);
}
