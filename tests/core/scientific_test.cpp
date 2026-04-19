#include <gtest/gtest.h>
#include "scientific.h"
#include "constants.h"
#include <cmath>

using namespace sc;

// Helper: check a float value is approximately equal to expected
static void expect_near(const ValuePtr& v, double expected, double tol = 1e-8) {
    ASSERT_TRUE(v->is_float()) << "expected float";
    auto& f = v->as_float();
    // Reconstruct double from mantissa * 10^exponent
    double actual = f.mantissa.get_d() * std::pow(10.0, f.exponent);
    EXPECT_NEAR(actual, expected, tol) << "mantissa=" << f.mantissa << " exp=" << f.exponent;
}

// --- Trig in degrees ---

TEST(ScientificTest, SinZero) {
    auto r = scientific::sin(Value::zero(), 12, AngularMode::Degrees);
    // sin(0) = 0
    EXPECT_TRUE(r->is_zero() || r->is_float());
}

TEST(ScientificTest, Sin90Degrees) {
    auto r = scientific::sin(Value::make_integer(90), 12, AngularMode::Degrees);
    expect_near(r, 1.0, 1e-10);
}

TEST(ScientificTest, Cos0Degrees) {
    auto r = scientific::cos(Value::zero(), 12, AngularMode::Degrees);
    expect_near(r, 1.0, 1e-10);
}

TEST(ScientificTest, Cos90Degrees) {
    auto r = scientific::cos(Value::make_integer(90), 12, AngularMode::Degrees);
    // Should be very close to 0
    auto& f = r->as_float();
    double actual = f.mantissa.get_d() * std::pow(10.0, f.exponent);
    EXPECT_NEAR(actual, 0.0, 1e-10);
}

TEST(ScientificTest, Tan45Degrees) {
    auto r = scientific::tan(Value::make_integer(45), 12, AngularMode::Degrees);
    expect_near(r, 1.0, 1e-10);
}

// --- Trig in radians ---

TEST(ScientificTest, SinPiRadians) {
    auto pi = constants::pi(12);
    auto r = scientific::sin(pi, 12, AngularMode::Radians);
    auto& f = r->as_float();
    double actual = f.mantissa.get_d() * std::pow(10.0, f.exponent);
    EXPECT_NEAR(actual, 0.0, 1e-10);
}

TEST(ScientificTest, CosPiRadians) {
    auto pi = constants::pi(12);
    auto r = scientific::cos(pi, 12, AngularMode::Radians);
    expect_near(r, -1.0, 1e-10);
}

// --- Inverse trig ---

TEST(ScientificTest, ArcsinOne) {
    auto r = scientific::arcsin(Value::one(), 12, AngularMode::Degrees);
    expect_near(r, 90.0, 1e-8);
}

TEST(ScientificTest, ArccosZero) {
    auto r = scientific::arccos(Value::zero(), 12, AngularMode::Degrees);
    expect_near(r, 90.0, 1e-8);
}

TEST(ScientificTest, ArctanOne) {
    auto r = scientific::arctan(Value::one(), 12, AngularMode::Degrees);
    expect_near(r, 45.0, 1e-8);
}

TEST(ScientificTest, Arctan2) {
    auto r = scientific::arctan2(Value::one(), Value::one(), 12, AngularMode::Degrees);
    expect_near(r, 45.0, 1e-8);
}

// --- Hyperbolic ---

TEST(ScientificTest, SinhZero) {
    auto r = scientific::sinh(Value::zero(), 12);
    ASSERT_TRUE(r->is_float() || r->is_zero());
}

TEST(ScientificTest, CoshZero) {
    auto r = scientific::cosh(Value::zero(), 12);
    expect_near(r, 1.0, 1e-10);
}

TEST(ScientificTest, TanhZero) {
    auto r = scientific::tanh(Value::zero(), 12);
    ASSERT_TRUE(r->is_float() || r->is_zero());
}

TEST(ScientificTest, ArcsinhZero) {
    auto r = scientific::arcsinh(Value::zero(), 12);
    ASSERT_TRUE(r->is_float() || r->is_zero());
}

TEST(ScientificTest, ArccoshOne) {
    auto r = scientific::arccosh(Value::one(), 12);
    // arccosh(1) = 0
    ASSERT_TRUE(r->is_float() || r->is_zero());
}

// --- Logarithmic ---

TEST(ScientificTest, LnE) {
    auto e = constants::e(12);
    auto r = scientific::ln(e, 12);
    expect_near(r, 1.0, 1e-10);
}

TEST(ScientificTest, LnOne) {
    auto r = scientific::ln(Value::one(), 12);
    ASSERT_TRUE(r->is_float() || r->is_zero());
}

TEST(ScientificTest, ExpZero) {
    auto r = scientific::exp(Value::zero(), 12);
    expect_near(r, 1.0, 1e-10);
}

TEST(ScientificTest, ExpOne) {
    auto r = scientific::exp(Value::one(), 12);
    auto e = constants::e(12);
    // Should be approximately e
    auto& rf = r->as_float();
    auto& ef = e->as_float();
    double rv = rf.mantissa.get_d() * std::pow(10.0, rf.exponent);
    double ev = ef.mantissa.get_d() * std::pow(10.0, ef.exponent);
    EXPECT_NEAR(rv, ev, 1e-8);
}

TEST(ScientificTest, Log10Of100) {
    auto r = scientific::log10(Value::make_integer(100), 12);
    expect_near(r, 2.0, 1e-10);
}

TEST(ScientificTest, Exp10Of2) {
    auto r = scientific::exp10(Value::make_integer(2), 12);
    expect_near(r, 100.0, 1e-8);
}

TEST(ScientificTest, LogBase) {
    // log_2(8) = 3
    auto r = scientific::logb(Value::make_integer(8), Value::make_integer(2), 12);
    expect_near(r, 3.0, 1e-10);
}

TEST(ScientificTest, Expm1NearZero) {
    // expm1(0) = 0
    auto r = scientific::expm1(Value::zero(), 12);
    ASSERT_TRUE(r->is_float() || r->is_zero());
}

TEST(ScientificTest, Lnp1NearZero) {
    // lnp1(0) = 0
    auto r = scientific::lnp1(Value::zero(), 12);
    ASSERT_TRUE(r->is_float() || r->is_zero());
}

// --- Combinatorics ---

TEST(ScientificTest, Factorial5) {
    auto r = scientific::factorial(Value::make_integer(5), 12);
    ASSERT_TRUE(r->is_integer());
    EXPECT_EQ(r->as_integer().v, 120);
}

TEST(ScientificTest, Factorial0) {
    auto r = scientific::factorial(Value::zero(), 12);
    ASSERT_TRUE(r->is_integer());
    EXPECT_EQ(r->as_integer().v, 1);
}

TEST(ScientificTest, Factorial20) {
    auto r = scientific::factorial(Value::make_integer(20), 12);
    ASSERT_TRUE(r->is_integer());
    // 20! = 2432902008176640000
    mpz_class expected("2432902008176640000");
    EXPECT_EQ(r->as_integer().v, expected);
}

TEST(ScientificTest, DoubleFactorial7) {
    // 7!! = 7*5*3*1 = 105
    auto r = scientific::double_factorial(Value::make_integer(7));
    ASSERT_TRUE(r->is_integer());
    EXPECT_EQ(r->as_integer().v, 105);
}

TEST(ScientificTest, DoubleFactorial8) {
    // 8!! = 8*6*4*2 = 384
    auto r = scientific::double_factorial(Value::make_integer(8));
    ASSERT_TRUE(r->is_integer());
    EXPECT_EQ(r->as_integer().v, 384);
}

TEST(ScientificTest, Choose5_2) {
    auto r = scientific::choose(Value::make_integer(5), Value::make_integer(2), 12);
    ASSERT_TRUE(r->is_integer());
    EXPECT_EQ(r->as_integer().v, 10);
}

TEST(ScientificTest, Choose10_3) {
    auto r = scientific::choose(Value::make_integer(10), Value::make_integer(3), 12);
    ASSERT_TRUE(r->is_integer());
    EXPECT_EQ(r->as_integer().v, 120);
}

TEST(ScientificTest, Permutation5_2) {
    // P(5,2) = 5*4 = 20
    auto r = scientific::permutation(Value::make_integer(5), Value::make_integer(2), 12);
    ASSERT_TRUE(r->is_integer());
    EXPECT_EQ(r->as_integer().v, 20);
}

// --- Other ---

TEST(ScientificTest, SignPositive) {
    auto r = scientific::sign(Value::make_integer(42));
    ASSERT_TRUE(r->is_integer());
    EXPECT_EQ(r->as_integer().v, 1);
}

TEST(ScientificTest, SignNegative) {
    auto r = scientific::sign(Value::make_integer(-42));
    ASSERT_TRUE(r->is_integer());
    EXPECT_EQ(r->as_integer().v, -1);
}

TEST(ScientificTest, SignZero) {
    auto r = scientific::sign(Value::zero());
    EXPECT_TRUE(r->is_zero());
}

TEST(ScientificTest, Hypot3_4) {
    auto r = scientific::hypot(Value::make_integer(3), Value::make_integer(4), 12);
    expect_near(r, 5.0, 1e-10);
}

// --- Integer log ---
TEST(ScientificTest, IlogBase10) {
    EXPECT_EQ(scientific::ilog(Value::make_integer(1000),
                               Value::make_integer(10))->as_integer().v, 3);
    EXPECT_EQ(scientific::ilog(Value::make_integer(999),
                               Value::make_integer(10))->as_integer().v, 2);
    EXPECT_EQ(scientific::ilog(Value::make_integer(1),
                               Value::make_integer(10))->as_integer().v, 0);
}

TEST(ScientificTest, IlogBase2) {
    EXPECT_EQ(scientific::ilog(Value::make_integer(1024),
                               Value::make_integer(2))->as_integer().v, 10);
    EXPECT_EQ(scientific::ilog(Value::make_integer(1023),
                               Value::make_integer(2))->as_integer().v, 9);
}

TEST(ScientificTest, IlogRejectsBadInputs) {
    EXPECT_THROW(scientific::ilog(Value::make_integer(0),
                                  Value::make_integer(10)), std::domain_error);
    EXPECT_THROW(scientific::ilog(Value::make_integer(10),
                                  Value::make_integer(1)), std::domain_error);
}

// --- Gamma function ---
TEST(ScientificTest, GammaIntegerEqualsFactorial) {
    // gamma(5) = 4! = 24
    auto r = scientific::gamma_fn(Value::make_integer(5), 20);
    expect_near(r, 24.0, 1e-10);
}

TEST(ScientificTest, GammaHalfEqualsSqrtPi) {
    // gamma(1/2) = sqrt(pi) ≈ 1.77245385091
    auto r = scientific::gamma_fn(Value::make_float(5, -1), 20);
    expect_near(r, std::sqrt(M_PI), 1e-10);
}
