#include <gtest/gtest.h>
#include "bitwise.hpp"

using namespace sc;

TEST(BitwiseTest, And) {
    auto a = Value::make_integer(0xFF);
    auto b = Value::make_integer(0x0F);
    auto r = bitwise::band(a, b);
    EXPECT_EQ(r->as_integer().v, 0x0F);
}

TEST(BitwiseTest, Or) {
    auto a = Value::make_integer(0xF0);
    auto b = Value::make_integer(0x0F);
    auto r = bitwise::bor(a, b);
    EXPECT_EQ(r->as_integer().v, 0xFF);
}

TEST(BitwiseTest, Xor) {
    auto a = Value::make_integer(0xFF);
    auto b = Value::make_integer(0x0F);
    auto r = bitwise::bxor(a, b);
    EXPECT_EQ(r->as_integer().v, 0xF0);
}

TEST(BitwiseTest, Not8Bit) {
    auto a = Value::make_integer(0x0F);
    auto r = bitwise::bnot(a, 8);
    EXPECT_EQ(r->as_integer().v, 0xF0);
}

TEST(BitwiseTest, Not32Bit) {
    auto a = Value::make_integer(0);
    auto r = bitwise::bnot(a, 32);
    mpz_class expected("4294967295"); // 2^32 - 1
    EXPECT_EQ(r->as_integer().v, expected);
}

TEST(BitwiseTest, LShift) {
    auto a = Value::make_integer(1);
    auto r = bitwise::lshift(a, 4, 0);
    EXPECT_EQ(r->as_integer().v, 16);
}

TEST(BitwiseTest, LShiftWithWordSize) {
    auto a = Value::make_integer(0x80);
    auto r = bitwise::lshift(a, 1, 8);
    EXPECT_EQ(r->as_integer().v, 0); // shifted out
}

TEST(BitwiseTest, RShift) {
    auto a = Value::make_integer(16);
    auto r = bitwise::rshift(a, 2, 0);
    EXPECT_EQ(r->as_integer().v, 4);
}

TEST(BitwiseTest, LRot8Bit) {
    // 0b10000001 rotated left by 1 = 0b00000011
    auto a = Value::make_integer(0x81);
    auto r = bitwise::lrot(a, 1, 8);
    EXPECT_EQ(r->as_integer().v, 0x03);
}

TEST(BitwiseTest, RRot8Bit) {
    // 0b00000011 rotated right by 1 = 0b10000001
    auto a = Value::make_integer(0x03);
    auto r = bitwise::rrot(a, 1, 8);
    EXPECT_EQ(r->as_integer().v, 0x81);
}

TEST(BitwiseTest, TwosComplement) {
    // -1 in 8-bit two's complement = 255
    auto a = Value::make_integer(-1);
    auto r = bitwise::to_twos_complement(a, 8);
    EXPECT_EQ(r->as_integer().v, 255);
}

TEST(BitwiseTest, FromTwosComplement) {
    // 255 in 8-bit two's complement = -1
    auto a = Value::make_integer(255);
    auto r = bitwise::from_twos_complement(a, 8);
    EXPECT_EQ(r->as_integer().v, -1);
}

TEST(BitwiseTest, TwosComplementPositive) {
    auto a = Value::make_integer(42);
    auto r = bitwise::to_twos_complement(a, 8);
    EXPECT_EQ(r->as_integer().v, 42);
    auto back = bitwise::from_twos_complement(r, 8);
    EXPECT_EQ(back->as_integer().v, 42);
}

TEST(BitwiseTest, TwosComplementRoundTrip) {
    auto a = Value::make_integer(-42);
    auto tc = bitwise::to_twos_complement(a, 8);
    auto back = bitwise::from_twos_complement(tc, 8);
    EXPECT_EQ(back->as_integer().v, -42);
}

TEST(BitwiseTest, Clip) {
    auto a = Value::make_integer(0x1FF);
    auto r = bitwise::clip(a, 8);
    EXPECT_EQ(r->as_integer().v, 0xFF);
}

TEST(BitwiseTest, NonIntegerThrows) {
    auto a = Value::make_float(15, -1);
    EXPECT_THROW(bitwise::band(a, Value::make_integer(1)), std::invalid_argument);
}
