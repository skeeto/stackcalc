#include <gtest/gtest.h>
#include "complex.h"
#include "arithmetic.h"

using namespace sc;

TEST(ComplexTest, MakeRectCollapsesToReal) {
    auto c = Value::make_rect_complex(Value::make_integer(3), Value::zero());
    // im=0 should collapse to just the real part
    EXPECT_TRUE(c->is_integer());
    EXPECT_EQ(c->as_integer().v, 3);
}

TEST(ComplexTest, MakeRectComplex) {
    auto c = Value::make_rect_complex(Value::make_integer(3), Value::make_integer(4));
    ASSERT_TRUE(c->is_complex());
    EXPECT_EQ(c->as_rect_complex().re->as_integer().v, 3);
    EXPECT_EQ(c->as_rect_complex().im->as_integer().v, 4);
}

TEST(ComplexTest, Add) {
    RectComplex a{Value::make_integer(1), Value::make_integer(2)};
    RectComplex b{Value::make_integer(3), Value::make_integer(4)};
    auto r = complex::add(a, b, 12);
    ASSERT_TRUE(r->is_complex());
    EXPECT_EQ(r->as_rect_complex().re->as_integer().v, 4);
    EXPECT_EQ(r->as_rect_complex().im->as_integer().v, 6);
}

TEST(ComplexTest, Mul) {
    // (1+2i)(3+4i) = (3-8) + (4+6)i = -5 + 10i
    RectComplex a{Value::make_integer(1), Value::make_integer(2)};
    RectComplex b{Value::make_integer(3), Value::make_integer(4)};
    auto r = complex::mul(a, b, 12);
    ASSERT_TRUE(r->is_complex());
    EXPECT_EQ(r->as_rect_complex().re->as_integer().v, -5);
    EXPECT_EQ(r->as_rect_complex().im->as_integer().v, 10);
}

TEST(ComplexTest, Neg) {
    RectComplex a{Value::make_integer(3), Value::make_integer(4)};
    auto r = complex::neg(a);
    ASSERT_TRUE(r->is_complex());
    EXPECT_EQ(r->as_rect_complex().re->as_integer().v, -3);
    EXPECT_EQ(r->as_rect_complex().im->as_integer().v, -4);
}

TEST(ComplexTest, Conj) {
    RectComplex a{Value::make_integer(3), Value::make_integer(4)};
    auto r = complex::conj(a);
    ASSERT_TRUE(r->is_complex());
    EXPECT_EQ(r->as_rect_complex().re->as_integer().v, 3);
    EXPECT_EQ(r->as_rect_complex().im->as_integer().v, -4);
}

TEST(ComplexTest, Magnitude) {
    // |3+4i| = 5
    RectComplex a{Value::make_integer(3), Value::make_integer(4)};
    auto r = complex::magnitude(a, 12);
    ASSERT_TRUE(r->is_float());
    EXPECT_EQ(r->as_float().mantissa, 5);
    EXPECT_EQ(r->as_float().exponent, 0);
}

TEST(ComplexTest, ArithAddRealAndComplex) {
    // 3 + (1+2i) = (4+2i)
    auto a = Value::make_integer(3);
    auto b = Value::make_rect_complex(Value::make_integer(1), Value::make_integer(2));
    auto r = arith::add(a, b, 12);
    ASSERT_TRUE(r->is_complex());
    EXPECT_EQ(r->as_rect_complex().re->as_integer().v, 4);
    EXPECT_EQ(r->as_rect_complex().im->as_integer().v, 2);
}

TEST(ComplexTest, ArithMulComplex) {
    auto a = Value::make_rect_complex(Value::make_integer(1), Value::make_integer(2));
    auto b = Value::make_rect_complex(Value::make_integer(3), Value::make_integer(4));
    auto r = arith::mul(a, b, 12);
    ASSERT_TRUE(r->is_complex());
    EXPECT_EQ(r->as_rect_complex().re->as_integer().v, -5);
    EXPECT_EQ(r->as_rect_complex().im->as_integer().v, 10);
}
