#include <gtest/gtest.h>
#include "complex.hpp"
#include "arithmetic.hpp"
#include <cmath>

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

// --- Inv / sqrt / power ---

// Helper: |x| as a double, where x is real
static double real_to_double(const ValuePtr& v) {
    if (v->is_integer()) return v->as_integer().v.get_d();
    if (v->is_fraction()) {
        auto& f = v->as_fraction();
        return f.num.get_d() / f.den.get_d();
    }
    if (v->is_float()) {
        auto& f = v->as_float();
        return f.mantissa.get_d() * std::pow(10.0, f.exponent);
    }
    return std::nan("");
}

TEST(ComplexTest, InvOfI) {
    // 1/i = -i
    auto i = Value::make_rect_complex(Value::zero(), Value::make_integer(1));
    auto r = arith::inv(i, 20);
    ASSERT_TRUE(r->is_complex());
    auto& rc = r->as_rect_complex();
    EXPECT_NEAR(real_to_double(rc.re), 0.0, 1e-15);
    EXPECT_NEAR(real_to_double(rc.im), -1.0, 1e-15);
}

TEST(ComplexTest, InvRoundTripsToOne) {
    // (1 + 2i) * (1/(1 + 2i)) ≈ 1
    auto z = Value::make_rect_complex(Value::make_integer(1), Value::make_integer(2));
    auto inv = arith::inv(z, 20);
    auto product = arith::mul(z, inv, 20);
    EXPECT_NEAR(real_to_double(product), 1.0, 1e-15);
}

TEST(ComplexTest, SqrtOfNegativeOneIsI) {
    auto neg_one = Value::make_integer(-1);
    auto r = arith::sqrt(neg_one, 20);
    ASSERT_TRUE(r->is_complex());
    auto& rc = r->as_rect_complex();
    EXPECT_NEAR(real_to_double(rc.re), 0.0, 1e-15);
    EXPECT_NEAR(real_to_double(rc.im), 1.0, 1e-15);
}

TEST(ComplexTest, SqrtOfComplexSquared) {
    // sqrt(3+4i)^2 == 3+4i
    auto z = Value::make_rect_complex(Value::make_integer(3), Value::make_integer(4));
    auto root = arith::sqrt(z, 20);
    auto squared = arith::mul(root, root, 20);
    ASSERT_TRUE(squared->is_complex());
    auto& sq = squared->as_rect_complex();
    EXPECT_NEAR(real_to_double(sq.re), 3.0, 1e-15);
    EXPECT_NEAR(real_to_double(sq.im), 4.0, 1e-15);
}

TEST(ComplexTest, IntegerPowerOfI) {
    // i^2 = -1
    auto i = Value::make_rect_complex(Value::zero(), Value::make_integer(1));
    auto r = arith::power(i, Value::make_integer(2), 20);
    EXPECT_NEAR(real_to_double(r), -1.0, 1e-15);
}

TEST(ComplexTest, IntegerPowerComplex) {
    // (1+i)^4 = -4
    auto z = Value::make_rect_complex(Value::make_integer(1), Value::make_integer(1));
    auto r = arith::power(z, Value::make_integer(4), 20);
    EXPECT_NEAR(real_to_double(r), -4.0, 1e-15);
}

TEST(ComplexTest, AbsOfComplex) {
    // |3+4i| = 5
    auto z = Value::make_rect_complex(Value::make_integer(3), Value::make_integer(4));
    auto r = arith::abs(z);
    EXPECT_NEAR(real_to_double(r), 5.0, 1e-15);
}
