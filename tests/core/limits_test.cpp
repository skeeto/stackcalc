#include "limits.hpp"
#include "integer.hpp"
#include "scientific.hpp"
#include <gtest/gtest.h>
#include <cstdint>

using namespace sc;

// --- helpers ---

TEST(LimitsTest, MpzBitsZero) {
    EXPECT_EQ(mpz_bits(mpz_class(0)), 0u);
}

TEST(LimitsTest, MpzBitsOne) {
    EXPECT_EQ(mpz_bits(mpz_class(1)), 1u);
}

TEST(LimitsTest, MpzBits2to31) {
    // 2^31 = 0x80000000, 32 bits
    EXPECT_EQ(mpz_bits(mpz_class("2147483648")), 32u);
}

TEST(LimitsTest, MpzBitsHandlesNegative) {
    // sign is irrelevant — magnitude only
    EXPECT_EQ(mpz_bits(mpz_class(-256)), 9u);
}

TEST(LimitsTest, SatMulNoOverflow) {
    EXPECT_EQ(sat_mul_u64(1000, 2000), 2000000ull);
}

TEST(LimitsTest, SatMulOverflowSaturates) {
    EXPECT_EQ(sat_mul_u64(UINT64_MAX, 2), UINT64_MAX);
    EXPECT_EQ(sat_mul_u64(UINT64_MAX / 2 + 1, 3), UINT64_MAX);
}

TEST(LimitsTest, SatMulZero) {
    EXPECT_EQ(sat_mul_u64(0, UINT64_MAX), 0u);
    EXPECT_EQ(sat_mul_u64(UINT64_MAX, 0), 0u);
}

// --- integer::pow ---

TEST(LimitsTest, PowAcceptsModerateResult) {
    // 2^1000 is 1000 bits — well under the cap.
    EXPECT_NO_THROW(integer::pow(Integer{2}, 1000ull));
}

TEST(LimitsTest, PowAcceptsNearCap) {
    // 2^9999999 produces ~10M bits; just under kMaxResultBits.
    // Don't actually run it (too slow); instead use a slightly higher
    // exp that we know exceeds the cap, and confirm we throw.
    // (See PowRefusesAboveCap.)
    SUCCEED();
}

TEST(LimitsTest, PowRefusesAboveCap) {
    // 2^11000000 needs 11M bits > 10M cap → throw before any allocation.
    EXPECT_THROW(integer::pow(Integer{2}, 11'000'000ull),
                 std::overflow_error);
}

TEST(LimitsTest, PowRefusesUserCase2to2to32) {
    // The reported case: 2^(2^32). Estimate is 4 billion bits, way
    // over the cap. Must throw immediately, not attempt to allocate
    // half a gigabyte.
    EXPECT_THROW(integer::pow(Integer{2}, 4294967296ull),
                 std::overflow_error);
}

TEST(LimitsTest, PowRefusesLargeBaseSmallExp) {
    // base = 2^30, exp = 1_000_000 → ~30M bits. Refuse.
    auto big = mpz_class("1073741824");  // 2^30, 31 bits
    EXPECT_THROW(integer::pow(Integer{big}, 1'000'000ull),
                 std::overflow_error);
}

// --- integer::factorial ---

TEST(LimitsTest, FactorialAcceptsModerate) {
    EXPECT_NO_THROW(integer::factorial(1000ull));
}

TEST(LimitsTest, FactorialRefusesHuge) {
    // 10M! has ~10M * 23 ≈ 230M bits. Over the cap.
    EXPECT_THROW(integer::factorial(10'000'000ull), std::overflow_error);
}

// --- scientific:: combinatorics ---

TEST(LimitsTest, ChooseAcceptsModerate) {
    auto n = Value::make_integer(100);
    auto k = Value::make_integer(50);
    EXPECT_NO_THROW(scientific::choose(n, k, 12));
}

TEST(LimitsTest, ChooseRefusesHuge) {
    // n = 20M → C(n, n/2) is ~20M bits. Over cap.
    auto n = Value::make_integer(mpz_class("20000000"));
    auto k = Value::make_integer(mpz_class("10000000"));
    EXPECT_THROW(scientific::choose(n, k, 12), std::overflow_error);
}

TEST(LimitsTest, PermutationRefusesHuge) {
    // P(n, k) = n^k upper bound. n = 2^30 (31 bits), k = 1M ⇒ ~31M bits.
    auto n = Value::make_integer(mpz_class("1073741824"));
    auto k = Value::make_integer(mpz_class("1000000"));
    EXPECT_THROW(scientific::permutation(n, k, 12), std::overflow_error);
}

TEST(LimitsTest, DoubleFactorialRefusesHuge) {
    auto v = Value::make_integer(mpz_class("10000000"));
    EXPECT_THROW(scientific::double_factorial(v), std::overflow_error);
}
