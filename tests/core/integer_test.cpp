#include <gtest/gtest.h>
#include "integer.hpp"

using namespace sc;

TEST(IntegerTest, Add) {
    auto r = integer::add(Integer{42}, Integer{13});
    EXPECT_EQ(r->as_integer().v, 55);
}

TEST(IntegerTest, AddNegative) {
    auto r = integer::add(Integer{-10}, Integer{3});
    EXPECT_EQ(r->as_integer().v, -7);
}

TEST(IntegerTest, AddLarge) {
    mpz_class a("999999999999999999999999999999");
    mpz_class b("1");
    auto r = integer::add(Integer{a}, Integer{b});
    EXPECT_EQ(r->as_integer().v, mpz_class("1000000000000000000000000000000"));
}

TEST(IntegerTest, Sub) {
    auto r = integer::sub(Integer{42}, Integer{13});
    EXPECT_EQ(r->as_integer().v, 29);
}

TEST(IntegerTest, Mul) {
    auto r = integer::mul(Integer{6}, Integer{7});
    EXPECT_EQ(r->as_integer().v, 42);
}

TEST(IntegerTest, MulLarge) {
    mpz_class a("123456789012345678901234567890");
    auto r = integer::mul(Integer{a}, Integer{2});
    EXPECT_EQ(r->as_integer().v, mpz_class("246913578024691357802469135780"));
}

TEST(IntegerTest, Div) {
    auto r = integer::div(Integer{17}, Integer{5});
    EXPECT_EQ(r->as_integer().v, 3); // truncate toward zero
}

TEST(IntegerTest, DivNegative) {
    auto r = integer::div(Integer{-17}, Integer{5});
    EXPECT_EQ(r->as_integer().v, -3); // truncate toward zero
}

TEST(IntegerTest, DivByZero) {
    EXPECT_THROW(integer::div(Integer{1}, Integer{0}), std::domain_error);
}

TEST(IntegerTest, FloorDiv) {
    auto r = integer::fdiv(Integer{-17}, Integer{5});
    EXPECT_EQ(r->as_integer().v, -4); // floor toward -inf
}

TEST(IntegerTest, FloorDivPositive) {
    auto r = integer::fdiv(Integer{17}, Integer{5});
    EXPECT_EQ(r->as_integer().v, 3);
}

TEST(IntegerTest, Mod) {
    auto r = integer::mod(Integer{17}, Integer{5});
    EXPECT_EQ(r->as_integer().v, 2);
}

TEST(IntegerTest, ModNegative) {
    // -17 mod 5 = 3 (remainder after floor division: -17 = 5*(-4) + 3)
    auto r = integer::mod(Integer{-17}, Integer{5});
    EXPECT_EQ(r->as_integer().v, 3);
}

TEST(IntegerTest, Neg) {
    auto r = integer::neg(Integer{42});
    EXPECT_EQ(r->as_integer().v, -42);

    auto r2 = integer::neg(Integer{-42});
    EXPECT_EQ(r2->as_integer().v, 42);
}

TEST(IntegerTest, Abs) {
    auto r = integer::abs(Integer{-42});
    EXPECT_EQ(r->as_integer().v, 42);

    auto r2 = integer::abs(Integer{42});
    EXPECT_EQ(r2->as_integer().v, 42);
}

TEST(IntegerTest, Pow) {
    auto r = integer::pow(Integer{2}, 10);
    EXPECT_EQ(r->as_integer().v, 1024);
}

TEST(IntegerTest, PowZero) {
    auto r = integer::pow(Integer{42}, 0);
    EXPECT_EQ(r->as_integer().v, 1);
}

TEST(IntegerTest, Gcd) {
    auto r = integer::gcd(Integer{12}, Integer{18});
    EXPECT_EQ(r->as_integer().v, 6);
}

TEST(IntegerTest, GcdCoprime) {
    auto r = integer::gcd(Integer{7}, Integer{13});
    EXPECT_EQ(r->as_integer().v, 1);
}

TEST(IntegerTest, Lcm) {
    auto r = integer::lcm(Integer{12}, Integer{18});
    EXPECT_EQ(r->as_integer().v, 36);
}

TEST(IntegerTest, Factorial) {
    auto r = integer::factorial(10);
    EXPECT_EQ(r->as_integer().v, 3628800);
}

TEST(IntegerTest, FactorialLarge) {
    auto r = integer::factorial(20);
    EXPECT_EQ(r->as_integer().v, mpz_class("2432902008176640000"));
}

TEST(IntegerTest, Cmp) {
    EXPECT_LT(integer::cmp(Integer{1}, Integer{2}), 0);
    EXPECT_EQ(integer::cmp(Integer{5}, Integer{5}), 0);
    EXPECT_GT(integer::cmp(Integer{10}, Integer{3}), 0);
}

TEST(IntegerTest, Predicates) {
    EXPECT_TRUE(integer::is_zero(Integer{0}));
    EXPECT_FALSE(integer::is_zero(Integer{1}));
    EXPECT_TRUE(integer::is_negative(Integer{-1}));
    EXPECT_FALSE(integer::is_negative(Integer{0}));
    EXPECT_TRUE(integer::is_positive(Integer{1}));
    EXPECT_FALSE(integer::is_positive(Integer{0}));
    EXPECT_TRUE(integer::is_even(Integer{4}));
    EXPECT_FALSE(integer::is_even(Integer{3}));
}
