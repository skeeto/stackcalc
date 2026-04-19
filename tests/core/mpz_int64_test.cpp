#include "mpz_int64.hpp"
#include <gtest/gtest.h>
#include <cstdint>

using namespace sc;

// fits_sint64

TEST(MpzInt64Test, FitsSint64Zero) {
    EXPECT_TRUE(mpz_fits_sint64(mpz_class(0)));
}

TEST(MpzInt64Test, FitsSint64SmallPositive) {
    EXPECT_TRUE(mpz_fits_sint64(mpz_class(42)));
}

TEST(MpzInt64Test, FitsSint64SmallNegative) {
    EXPECT_TRUE(mpz_fits_sint64(mpz_class(-42)));
}

TEST(MpzInt64Test, FitsSint64Boundary2to31) {
    // Values around the 32-bit `long` boundary (Windows would silently
    // truncate via mpz_get_si on values past 2^31). Both should fit
    // happily in int64 on every platform.
    EXPECT_TRUE(mpz_fits_sint64(mpz_class("2147483648")));   // 2^31
    EXPECT_TRUE(mpz_fits_sint64(mpz_class("-2147483649")));  // -(2^31+1)
}

TEST(MpzInt64Test, FitsSint64Boundary2to32) {
    // The user's exact case: 2^32 = 4294967296. Must fit in int64.
    EXPECT_TRUE(mpz_fits_sint64(mpz_class("4294967296")));
}

TEST(MpzInt64Test, FitsSint64Int64Max) {
    EXPECT_TRUE(mpz_fits_sint64(mpz_class("9223372036854775807")));
}

TEST(MpzInt64Test, FitsSint64Int64Min) {
    EXPECT_TRUE(mpz_fits_sint64(mpz_class("-9223372036854775808")));
}

TEST(MpzInt64Test, FitsSint64JustAboveInt64Max) {
    EXPECT_FALSE(mpz_fits_sint64(mpz_class("9223372036854775808")));
}

TEST(MpzInt64Test, FitsSint64JustBelowInt64Min) {
    EXPECT_FALSE(mpz_fits_sint64(mpz_class("-9223372036854775809")));
}

TEST(MpzInt64Test, FitsSint64FarOutside) {
    EXPECT_FALSE(mpz_fits_sint64(mpz_class("100000000000000000000")));
    EXPECT_FALSE(mpz_fits_sint64(mpz_class("-100000000000000000000")));
}

// fits_uint64

TEST(MpzInt64Test, FitsUint64Zero) {
    EXPECT_TRUE(mpz_fits_uint64(mpz_class(0)));
}

TEST(MpzInt64Test, FitsUint64Negative) {
    EXPECT_FALSE(mpz_fits_uint64(mpz_class(-1)));
}

TEST(MpzInt64Test, FitsUint64Above2to32) {
    EXPECT_TRUE(mpz_fits_uint64(mpz_class("4294967296")));
}

TEST(MpzInt64Test, FitsUint64Uint64Max) {
    EXPECT_TRUE(mpz_fits_uint64(mpz_class("18446744073709551615")));
}

TEST(MpzInt64Test, FitsUint64JustAboveUint64Max) {
    EXPECT_FALSE(mpz_fits_uint64(mpz_class("18446744073709551616")));
}

// get_sint64

TEST(MpzInt64Test, GetSint64Zero) {
    EXPECT_EQ(mpz_get_sint64(mpz_class(0)), 0);
}

TEST(MpzInt64Test, GetSint64SmallPositive) {
    EXPECT_EQ(mpz_get_sint64(mpz_class(12345)), 12345);
}

TEST(MpzInt64Test, GetSint64SmallNegative) {
    EXPECT_EQ(mpz_get_sint64(mpz_class(-12345)), -12345);
}

TEST(MpzInt64Test, GetSint64TwoTo32) {
    EXPECT_EQ(mpz_get_sint64(mpz_class("4294967296")),
              static_cast<std::int64_t>(4294967296LL));
}

TEST(MpzInt64Test, GetSint64NegativeTwoTo32) {
    EXPECT_EQ(mpz_get_sint64(mpz_class("-4294967296")),
              static_cast<std::int64_t>(-4294967296LL));
}

TEST(MpzInt64Test, GetSint64Int64Max) {
    EXPECT_EQ(mpz_get_sint64(mpz_class("9223372036854775807")), INT64_MAX);
}

TEST(MpzInt64Test, GetSint64Int64Min) {
    EXPECT_EQ(mpz_get_sint64(mpz_class("-9223372036854775808")), INT64_MIN);
}

// get_uint64

TEST(MpzInt64Test, GetUint64Zero) {
    EXPECT_EQ(mpz_get_uint64(mpz_class(0)), 0u);
}

TEST(MpzInt64Test, GetUint64Small) {
    EXPECT_EQ(mpz_get_uint64(mpz_class(99999)), 99999u);
}

TEST(MpzInt64Test, GetUint64TwoTo32) {
    EXPECT_EQ(mpz_get_uint64(mpz_class("4294967296")), 4294967296ull);
}

TEST(MpzInt64Test, GetUint64Uint64Max) {
    EXPECT_EQ(mpz_get_uint64(mpz_class("18446744073709551615")), UINT64_MAX);
}

// Round-trips

TEST(MpzInt64Test, RoundTripPositiveInt64Boundary) {
    for (std::int64_t v : { (std::int64_t)0,
                            (std::int64_t)1,
                            (std::int64_t)-1,
                            (std::int64_t)INT32_MAX,
                            (std::int64_t)INT32_MAX + 1,
                            (std::int64_t)INT64_MAX - 1,
                            INT64_MAX,
                            INT64_MIN }) {
        mpz_class z;
        if (v >= 0) mpz_set_ui(z.get_mpz_t(), static_cast<unsigned long>(v & 0xFFFFFFFFu));
        // Just construct from string for portability with INT64_MIN/etc.
        std::string s = std::to_string(v);
        mpz_class z2(s);
        ASSERT_TRUE(mpz_fits_sint64(z2)) << "fits failed for " << s;
        EXPECT_EQ(mpz_get_sint64(z2), v) << "round-trip failed for " << s;
    }
}
