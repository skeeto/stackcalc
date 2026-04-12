#include <gtest/gtest.h>
#include "scientific.h"

using namespace sc;

// --- GCD / LCM ---

TEST(NumberTheoryTest, Gcd) {
    auto a = Value::make_integer(12);
    auto b = Value::make_integer(8);
    auto r = scientific::gcd(a, b);
    EXPECT_EQ(r->as_integer().v, 4);
}

TEST(NumberTheoryTest, GcdCoprime) {
    auto a = Value::make_integer(7);
    auto b = Value::make_integer(13);
    auto r = scientific::gcd(a, b);
    EXPECT_EQ(r->as_integer().v, 1);
}

TEST(NumberTheoryTest, Lcm) {
    auto a = Value::make_integer(12);
    auto b = Value::make_integer(8);
    auto r = scientific::lcm(a, b);
    EXPECT_EQ(r->as_integer().v, 24);
}

TEST(NumberTheoryTest, GcdNonIntegerThrows) {
    auto a = Value::make_float(15, -1);
    auto b = Value::make_integer(3);
    EXPECT_THROW(scientific::gcd(a, b), std::domain_error);
}

// --- Next/Prev Prime ---

TEST(NumberTheoryTest, NextPrime) {
    auto a = Value::make_integer(10);
    auto r = scientific::next_prime(a);
    EXPECT_EQ(r->as_integer().v, 11);
}

TEST(NumberTheoryTest, NextPrimeFromPrime) {
    auto a = Value::make_integer(13);
    auto r = scientific::next_prime(a);
    EXPECT_EQ(r->as_integer().v, 17);
}

TEST(NumberTheoryTest, NextPrimeFromOne) {
    auto a = Value::make_integer(1);
    auto r = scientific::next_prime(a);
    EXPECT_EQ(r->as_integer().v, 2);
}

TEST(NumberTheoryTest, PrevPrime) {
    auto a = Value::make_integer(10);
    auto r = scientific::prev_prime(a);
    EXPECT_EQ(r->as_integer().v, 7);
}

TEST(NumberTheoryTest, PrevPrimeFromPrime) {
    auto a = Value::make_integer(13);
    auto r = scientific::prev_prime(a);
    EXPECT_EQ(r->as_integer().v, 11);
}

TEST(NumberTheoryTest, PrevPrimeFrom3) {
    auto a = Value::make_integer(3);
    auto r = scientific::prev_prime(a);
    EXPECT_EQ(r->as_integer().v, 2);
}

TEST(NumberTheoryTest, PrevPrimeFrom2Throws) {
    auto a = Value::make_integer(2);
    EXPECT_THROW(scientific::prev_prime(a), std::domain_error);
}

// --- Prime Test ---

TEST(NumberTheoryTest, PrimeTestPrime) {
    auto a = Value::make_integer(17);
    auto r = scientific::prime_test(a);
    EXPECT_GE(r->as_integer().v, 1); // 1 or 2 = prime
}

TEST(NumberTheoryTest, PrimeTestComposite) {
    auto a = Value::make_integer(15);
    auto r = scientific::prime_test(a);
    EXPECT_EQ(r->as_integer().v, 0);
}

TEST(NumberTheoryTest, PrimeTestLargePrime) {
    // Mersenne prime 2^61 - 1
    mpz_class m;
    mpz_ui_pow_ui(m.get_mpz_t(), 2, 61);
    m -= 1;
    auto a = Value::make_integer(std::move(m));
    auto r = scientific::prime_test(a);
    EXPECT_GE(r->as_integer().v, 1);
}

// --- Totient ---

TEST(NumberTheoryTest, TotientPrime) {
    auto a = Value::make_integer(7);
    auto r = scientific::totient(a);
    EXPECT_EQ(r->as_integer().v, 6); // phi(7) = 6
}

TEST(NumberTheoryTest, Totient12) {
    auto a = Value::make_integer(12);
    auto r = scientific::totient(a);
    EXPECT_EQ(r->as_integer().v, 4); // phi(12) = 4
}

TEST(NumberTheoryTest, Totient1) {
    auto a = Value::make_integer(1);
    auto r = scientific::totient(a);
    EXPECT_EQ(r->as_integer().v, 1);
}

TEST(NumberTheoryTest, TotientPrimePower) {
    // phi(8) = phi(2^3) = 4
    auto a = Value::make_integer(8);
    auto r = scientific::totient(a);
    EXPECT_EQ(r->as_integer().v, 4);
}

// --- Prime Factors ---

TEST(NumberTheoryTest, PrimeFactors12) {
    auto a = Value::make_integer(12);
    auto r = scientific::prime_factors(a);
    ASSERT_TRUE(r->is_vector());
    auto& v = r->as_vector();
    ASSERT_EQ(v.elements.size(), 2u); // [[2,2], [3,1]]
    // First factor: [2, 2]
    ASSERT_TRUE(v.elements[0]->is_vector());
    auto& f0 = v.elements[0]->as_vector();
    EXPECT_EQ(f0.elements[0]->as_integer().v, 2);
    EXPECT_EQ(f0.elements[1]->as_integer().v, 2);
    // Second factor: [3, 1]
    auto& f1 = v.elements[1]->as_vector();
    EXPECT_EQ(f1.elements[0]->as_integer().v, 3);
    EXPECT_EQ(f1.elements[1]->as_integer().v, 1);
}

TEST(NumberTheoryTest, PrimeFactorsPrime) {
    auto a = Value::make_integer(17);
    auto r = scientific::prime_factors(a);
    auto& v = r->as_vector();
    ASSERT_EQ(v.elements.size(), 1u); // [[17, 1]]
    EXPECT_EQ(v.elements[0]->as_vector().elements[0]->as_integer().v, 17);
}

TEST(NumberTheoryTest, PrimeFactors1) {
    auto a = Value::make_integer(1);
    auto r = scientific::prime_factors(a);
    auto& v = r->as_vector();
    EXPECT_EQ(v.elements.size(), 0u); // empty
}

// --- Random ---

TEST(NumberTheoryTest, RandomInRange) {
    auto bound = Value::make_integer(100);
    for (int i = 0; i < 20; i++) {
        auto r = scientific::random(bound);
        ASSERT_TRUE(r->is_integer());
        EXPECT_GE(r->as_integer().v, 0);
        EXPECT_LT(r->as_integer().v, 100);
    }
}

TEST(NumberTheoryTest, RandomNonPositiveThrows) {
    auto a = Value::make_integer(0);
    EXPECT_THROW(scientific::random(a), std::domain_error);
}

// --- Extended GCD ---

TEST(NumberTheoryTest, ExtendedGcd) {
    auto a = Value::make_integer(35);
    auto b = Value::make_integer(15);
    auto r = scientific::extended_gcd(a, b);
    ASSERT_TRUE(r->is_vector());
    auto& v = r->as_vector();
    ASSERT_EQ(v.elements.size(), 3u);
    mpz_class g = v.elements[0]->as_integer().v;
    mpz_class s = v.elements[1]->as_integer().v;
    mpz_class t = v.elements[2]->as_integer().v;
    EXPECT_EQ(g, 5);
    // Verify: a*s + b*t == g
    EXPECT_EQ(35 * s + 15 * t, g);
}

// --- Modular Exponentiation ---

TEST(NumberTheoryTest, ModPow) {
    auto base = Value::make_integer(3);
    auto exp = Value::make_integer(10);
    auto m = Value::make_integer(7);
    auto r = scientific::mod_pow(base, exp, m);
    // 3^10 = 59049, 59049 mod 7 = 4
    EXPECT_EQ(r->as_integer().v, 4);
}

TEST(NumberTheoryTest, ModPowLarge) {
    auto base = Value::make_integer(2);
    auto exp = Value::make_integer(100);
    auto m = Value::make_integer(13);
    auto r = scientific::mod_pow(base, exp, m);
    // 2^100 mod 13 = 3
    EXPECT_EQ(r->as_integer().v, 3);
}
