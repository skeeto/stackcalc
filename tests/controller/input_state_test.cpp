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

TEST(InputStateTest, NegativeExponentViaMinus) {
    // '-' immediately after 'e' is unambiguous: must be the exponent sign,
    // can't be the subtract operator. So "1e-3" should Just Work.
    InputState is;
    CalcState state;
    EXPECT_TRUE(is.feed('1', state));
    EXPECT_TRUE(is.feed('e', state));
    EXPECT_TRUE(is.feed('-', state));
    EXPECT_TRUE(is.feed('3', state));
    EXPECT_EQ(is.text(), "1e-3");
    auto v = is.finalize(state);
    ASSERT_TRUE(v);
    auto& f = v->as_float();
    double val = f.mantissa.get_d() * std::pow(10.0, f.exponent);
    EXPECT_NEAR(val, 0.001, 1e-15);
}

TEST(InputStateTest, PositiveExponentViaPlus) {
    InputState is;
    CalcState state;
    EXPECT_TRUE(is.feed('2', state));
    EXPECT_TRUE(is.feed('e', state));
    EXPECT_TRUE(is.feed('+', state));
    EXPECT_TRUE(is.feed('5', state));
    EXPECT_EQ(is.text(), "2e+5");
    auto v = is.finalize(state);
    ASSERT_TRUE(v);
    auto& f = v->as_float();
    double val = f.mantissa.get_d() * std::pow(10.0, f.exponent);
    EXPECT_NEAR(val, 200000.0, 1e-8);
}

TEST(InputStateTest, MinusOutsideExponentNotConsumed) {
    // After a digit (not after 'e'), '-' is the subtract operator and
    // must not be consumed as part of the entry.
    InputState is;
    CalcState state;
    is.feed('1', state);
    is.feed('2', state);
    EXPECT_FALSE(is.feed('-', state));
    EXPECT_EQ(is.text(), "12");
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

TEST(InputStateTest, BareHexLettersInHexDisplayDoNotEnterAsDigits) {
    // Number entry is always decimal regardless of display_radix. To enter
    // a hex literal the user must use the radix-prefix form (see
    // RadixPrefix below). Bare 'F' here is not a digit and not consumed.
    InputState is;
    CalcState state;
    state.display_radix = 16;
    EXPECT_FALSE(is.feed('F', state));
    EXPECT_FALSE(is.active());
    EXPECT_TRUE(is.text().empty());
}

TEST(InputStateTest, DecimalInputInHexDisplayMode) {
    // "10" in hex display mode still parses as decimal ten — display radix
    // does not affect input.
    InputState is;
    CalcState state;
    state.display_radix = 16;
    EXPECT_TRUE(is.feed('1', state));
    EXPECT_TRUE(is.feed('0', state));
    auto v = is.finalize(state);
    ASSERT_TRUE(v);
    EXPECT_TRUE(v->is_integer());
    EXPECT_EQ(v->as_integer().v, 10);
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
