#include <gtest/gtest.h>
#include "modes.hpp"
#include "stack.hpp"

using namespace sc;

TEST(ModesTest, DefaultState) {
    CalcState state;
    EXPECT_EQ(state.precision, 12);
    EXPECT_EQ(state.angular_mode, AngularMode::Degrees);
    EXPECT_EQ(state.display_format, DisplayFormat::Normal);
    EXPECT_EQ(state.display_radix, 10);
    EXPECT_EQ(state.fraction_mode, FractionMode::Float);
    EXPECT_FALSE(state.symbolic_mode);
    EXPECT_FALSE(state.infinite_mode);
    EXPECT_FALSE(state.polar_mode);
    EXPECT_EQ(state.word_size, 32);
    EXPECT_EQ(state.complex_format, ComplexFormat::Pair);
}

TEST(ModesTest, TransientFlags) {
    CalcState state;
    state.inverse_flag = true;
    state.hyperbolic_flag = true;
    state.keep_args_flag = true;
    state.clear_transient_flags();
    EXPECT_FALSE(state.inverse_flag);
    EXPECT_FALSE(state.hyperbolic_flag);
    EXPECT_FALSE(state.keep_args_flag);
}

TEST(ModesTest, SetPrecision) {
    CalcState state;
    state.precision = 50;
    EXPECT_EQ(state.precision, 50);
}

TEST(ModesTest, SetAngularMode) {
    CalcState state;
    state.angular_mode = AngularMode::Radians;
    EXPECT_EQ(state.angular_mode, AngularMode::Radians);
}

TEST(ModesTest, SetDisplayFormat) {
    CalcState state;
    state.display_format = DisplayFormat::Sci;
    EXPECT_EQ(state.display_format, DisplayFormat::Sci);
}

TEST(ModesTest, SetRadix) {
    CalcState state;
    state.display_radix = 16;
    EXPECT_EQ(state.display_radix, 16);
}

TEST(ModesTest, StackStateAccess) {
    Stack s;
    s.state().precision = 20;
    s.state().angular_mode = AngularMode::Radians;
    EXPECT_EQ(s.state().precision, 20);
    EXPECT_EQ(s.state().angular_mode, AngularMode::Radians);
}

TEST(ModesTest, FractionMode) {
    CalcState state;
    state.fraction_mode = FractionMode::Fraction;
    EXPECT_EQ(state.fraction_mode, FractionMode::Fraction);
}
