#include <gtest/gtest.h>
#include "input_state.h"
#include <cmath>

using namespace sc;

TEST(InputStateTest, NotActiveByDefault) {
    InputState is;
    EXPECT_FALSE(is.active());
    EXPECT_TRUE(is.text().empty());
}

TEST(InputStateTest, DigitsActivate) {
    InputState is;
    CalcState state;
    EXPECT_TRUE(is.feed('3', state));
    EXPECT_TRUE(is.active());
    EXPECT_EQ(is.text(), "3");
    EXPECT_TRUE(is.feed('7', state));
    EXPECT_EQ(is.text(), "37");
}

TEST(InputStateTest, FinalizeInteger) {
    InputState is;
    CalcState state;
    is.feed('4', state);
    is.feed('2', state);
    auto v = is.finalize(state);
    ASSERT_TRUE(v);
    EXPECT_TRUE(v->is_integer());
    EXPECT_EQ(v->as_integer().v, 42);
    EXPECT_FALSE(is.active());
}

TEST(InputStateTest, DecimalFloat) {
    InputState is;
    CalcState state;
    is.feed('3', state);
    is.feed('.', state);
    is.feed('1', state);
    is.feed('4', state);
    auto v = is.finalize(state);
    ASSERT_TRUE(v);
    EXPECT_TRUE(v->is_float());
    EXPECT_EQ(v->as_float().mantissa, 314);
    EXPECT_EQ(v->as_float().exponent, -2);
}

TEST(InputStateTest, NegativeNumber) {
    InputState is;
    CalcState state;
    is.feed('_', state);
    is.feed('5', state);
    auto v = is.finalize(state);
    ASSERT_TRUE(v);
    EXPECT_TRUE(v->is_integer());
    EXPECT_EQ(v->as_integer().v, -5);
}

TEST(InputStateTest, ScientificNotation) {
    InputState is;
    CalcState state;
    is.feed('1', state);
    is.feed('.', state);
    is.feed('5', state);
    is.feed('e', state);
    is.feed('3', state);
    auto v = is.finalize(state);
    ASSERT_TRUE(v);
    EXPECT_TRUE(v->is_float());
    // 1.5e3 = 1500
    auto& f = v->as_float();
    double val = f.mantissa.get_d() * std::pow(10.0, f.exponent);
    EXPECT_NEAR(val, 1500.0, 1e-8);
}

TEST(InputStateTest, Fraction) {
    InputState is;
    CalcState state;
    is.feed('1', state);
    is.feed(':', state);
    is.feed('3', state);
    auto v = is.finalize(state);
    ASSERT_TRUE(v);
    EXPECT_TRUE(v->is_fraction());
    EXPECT_EQ(v->as_fraction().num, 1);
    EXPECT_EQ(v->as_fraction().den, 3);
}

TEST(InputStateTest, HexInput) {
    InputState is;
    CalcState state;
    state.display_radix = 16;
    is.feed('F', state);
    is.feed('F', state);
    auto v = is.finalize(state);
    ASSERT_TRUE(v);
    EXPECT_TRUE(v->is_integer());
    EXPECT_EQ(v->as_integer().v, 255);
}

TEST(InputStateTest, Cancel) {
    InputState is;
    CalcState state;
    is.feed('5', state);
    is.cancel();
    EXPECT_FALSE(is.active());
    EXPECT_TRUE(is.text().empty());
}

TEST(InputStateTest, BackspaceLastDigit) {
    InputState is;
    CalcState state;
    is.feed('5', state);
    is.feed('\b', state);
    EXPECT_FALSE(is.active());
}

TEST(InputStateTest, BackspacePartial) {
    InputState is;
    CalcState state;
    is.feed('1', state);
    is.feed('2', state);
    is.feed('\b', state);
    EXPECT_TRUE(is.active());
    EXPECT_EQ(is.text(), "1");
}

TEST(InputStateTest, FinalizeEmpty) {
    InputState is;
    CalcState state;
    EXPECT_EQ(is.finalize(state), nullptr);
}

TEST(InputStateTest, RadixPrefix) {
    InputState is;
    CalcState state;
    is.feed('1', state);
    is.feed('6', state);
    is.feed('#', state);
    is.feed('F', state);
    is.feed('F', state);
    auto v = is.finalize(state);
    ASSERT_TRUE(v);
    EXPECT_TRUE(v->is_integer());
    EXPECT_EQ(v->as_integer().v, 255);
}

TEST(InputStateTest, DotAlone) {
    InputState is;
    CalcState state;
    is.feed('.', state);
    EXPECT_TRUE(is.active());
    EXPECT_EQ(is.text(), "0.");
}
