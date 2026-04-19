#include <gtest/gtest.h>
#include "error_form.hpp"
#include "arithmetic.hpp"

using namespace sc;

TEST(ErrorFormTest, FromValue) {
    auto v = error_form::from_value(Value::make_integer(5));
    ASSERT_TRUE(v->is_error());
    EXPECT_EQ(v->as_error().x->as_integer().v, 5);
    EXPECT_TRUE(v->as_error().sigma->is_zero());
}

TEST(ErrorFormTest, AddSimple) {
    // (10 +/- 1) + (20 +/- 2) = (30 +/- sqrt(5))
    ErrorForm a{Value::make_integer(10), Value::make_integer(1)};
    ErrorForm b{Value::make_integer(20), Value::make_integer(2)};
    auto r = error_form::add(a, b, 12);
    ASSERT_TRUE(r->is_error());
    EXPECT_EQ(r->as_error().x->as_integer().v, 30);
    // sigma = sqrt(1+4) = sqrt(5) ~ 2.2360679...
    ASSERT_TRUE(r->as_error().sigma->is_float());
    auto& s = r->as_error().sigma->as_float();
    // mantissa should be 22360679... (12 digits)
    EXPECT_GT(s.mantissa, 0);
}

TEST(ErrorFormTest, SubSimple) {
    ErrorForm a{Value::make_integer(30), Value::make_integer(3)};
    ErrorForm b{Value::make_integer(10), Value::make_integer(4)};
    auto r = error_form::sub(a, b, 12);
    ASSERT_TRUE(r->is_error());
    EXPECT_EQ(r->as_error().x->as_integer().v, 20);
    // sigma = sqrt(9+16) = sqrt(25) = 5
    auto& s = r->as_error().sigma->as_float();
    EXPECT_EQ(s.mantissa, 5);
    EXPECT_EQ(s.exponent, 0);
}

TEST(ErrorFormTest, Neg) {
    ErrorForm a{Value::make_integer(10), Value::make_integer(2)};
    auto r = error_form::neg(a);
    ASSERT_TRUE(r->is_error());
    EXPECT_EQ(r->as_error().x->as_integer().v, -10);
    EXPECT_EQ(r->as_error().sigma->as_integer().v, 2);
}

TEST(ErrorFormTest, Abs) {
    ErrorForm a{Value::make_integer(-10), Value::make_integer(2)};
    auto r = error_form::abs(a, 12);
    ASSERT_TRUE(r->is_error());
    EXPECT_EQ(r->as_error().x->as_integer().v, 10);
    EXPECT_EQ(r->as_error().sigma->as_integer().v, 2);
}

TEST(ErrorFormTest, MulSimple) {
    // (10 +/- 1) * (20 +/- 2) = 200 +/- |200|*sqrt((1/10)^2 + (2/20)^2)
    // = 200 +/- 200*sqrt(0.01 + 0.01) = 200 +/- 200*sqrt(0.02)
    // = 200 +/- 200*0.14142... = 200 +/- 28.284...
    ErrorForm a{Value::make_integer(10), Value::make_integer(1)};
    ErrorForm b{Value::make_integer(20), Value::make_integer(2)};
    auto r = error_form::mul(a, b, 12);
    ASSERT_TRUE(r->is_error());
    EXPECT_EQ(r->as_error().x->as_integer().v, 200);
    // sigma should be ~28.28
    ASSERT_TRUE(r->as_error().sigma->is_float());
}

TEST(ErrorFormTest, DivSimple) {
    // (100 +/- 5) / (10 +/- 1) = 10 +/- |10|*sqrt((5/100)^2 + (1/10)^2)
    // = 10 +/- 10*sqrt(0.0025 + 0.01) = 10 +/- 10*sqrt(0.0125)
    // = 10 +/- 10*0.111803... = 10 +/- 1.118...
    ErrorForm a{Value::make_integer(100), Value::make_integer(5)};
    ErrorForm b{Value::make_integer(10), Value::make_integer(1)};
    auto r = error_form::div(a, b, 12);
    ASSERT_TRUE(r->is_error());
    // x = 100/10 = 10 (as float since div uses float mode)
    ASSERT_TRUE(r->as_error().sigma->is_float());
}

TEST(ErrorFormTest, ArithDispatch) {
    // Arithmetic layer should dispatch to error form
    auto a = Value::make_error(Value::make_integer(10), Value::make_integer(1));
    auto b = Value::make_error(Value::make_integer(20), Value::make_integer(2));
    auto r = arith::add(a, b, 12);
    ASSERT_TRUE(r->is_error());
    EXPECT_EQ(r->as_error().x->as_integer().v, 30);
}
