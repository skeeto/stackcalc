#include <gtest/gtest.h>
#include "formatter.h"

using namespace sc;

class FormatterTest : public ::testing::Test {
protected:
    CalcState state;
    void SetUp() override {
        state = CalcState{}; // defaults
    }
};

// --- Integer display ---

TEST_F(FormatterTest, Integer) {
    Formatter fmt(state);
    EXPECT_EQ(fmt.format(Value::make_integer(42)), "42");
    EXPECT_EQ(fmt.format(Value::make_integer(-7)), "-7");
    EXPECT_EQ(fmt.format(Value::zero()), "0");
}

TEST_F(FormatterTest, IntegerHex) {
    state.display_radix = 16;
    Formatter fmt(state);
    EXPECT_EQ(fmt.format(Value::make_integer(255)), "16#FF");
    EXPECT_EQ(fmt.format(Value::make_integer(0)), "16#0");
}

TEST_F(FormatterTest, IntegerBinary) {
    state.display_radix = 2;
    Formatter fmt(state);
    EXPECT_EQ(fmt.format(Value::make_integer(10)), "2#1010");
}

TEST_F(FormatterTest, IntegerOctal) {
    state.display_radix = 8;
    Formatter fmt(state);
    EXPECT_EQ(fmt.format(Value::make_integer(255)), "8#377");
}

TEST_F(FormatterTest, IntegerGrouping) {
    state.group_digits = 3;
    Formatter fmt(state);
    EXPECT_EQ(fmt.format(Value::make_integer(1234567)), "1,234,567");
}

// --- Fraction display ---

TEST_F(FormatterTest, Fraction) {
    Formatter fmt(state);
    EXPECT_EQ(fmt.format(Value::make_fraction(1, 3)), "1:3");
    EXPECT_EQ(fmt.format(Value::make_fraction(-2, 5)), "-2:5");
}

// --- Float display (Normal) ---

TEST_F(FormatterTest, FloatNormal) {
    Formatter fmt(state);
    // 1.5 = mantissa=15, exponent=-1
    EXPECT_EQ(fmt.format(Value::make_float(15, -1)), "1.5");
    // 0.1 = mantissa=1, exponent=-1
    EXPECT_EQ(fmt.format(Value::make_float(1, -1)), "0.1");
    // 100.0 = mantissa=1, exponent=2
    EXPECT_EQ(fmt.format(Value::make_float(1, 2)), "100.");
}

TEST_F(FormatterTest, FloatZero) {
    Formatter fmt(state);
    EXPECT_EQ(fmt.format(Value::make_float(0, 0)), "0.");
}

TEST_F(FormatterTest, FloatNegative) {
    Formatter fmt(state);
    EXPECT_EQ(fmt.format(Value::make_float(-15, -1)), "-1.5");
}

// --- Normal-format scientific-notation thresholds (matches Emacs `d n`) ---

TEST_F(FormatterTest, FloatNormalSmallStaysDecimal) {
    Formatter fmt(state);
    // 0.01 (sci_exp = -2) is shown decimally
    EXPECT_EQ(fmt.format(Value::make_float(1, -2)), "0.01");
}

TEST_F(FormatterTest, FloatNormalVerySmallSwitchesToSci) {
    Formatter fmt(state);
    // 0.001 (sci_exp = -3) is shown in scientific notation
    EXPECT_EQ(fmt.format(Value::make_float(1, -3)), "1e-3");
    // 1.5e-4: mantissa=15, exp=-5, sci_exp=-4
    EXPECT_EQ(fmt.format(Value::make_float(15, -5)), "1.5e-4");
}

TEST_F(FormatterTest, FloatNormalLargeStaysDecimal) {
    Formatter fmt(state);  // precision = 12, threshold > 15 → sci
    // 1e15 (sci_exp = 15) is the largest still shown decimally at prec=12
    EXPECT_EQ(fmt.format(Value::make_float(1, 15)), "1000000000000000.");
}

TEST_F(FormatterTest, FloatNormalVeryLargeSwitchesToSci) {
    Formatter fmt(state);  // precision = 12
    // 1e16 (sci_exp = 16) crosses threshold → scientific
    EXPECT_EQ(fmt.format(Value::make_float(1, 16)), "1e16");
    // 1.234e17 (mantissa=1234, exp=14, sci_exp=17)
    EXPECT_EQ(fmt.format(Value::make_float(1234, 14)), "1.234e17");
}

TEST_F(FormatterTest, FloatNormalThresholdScalesWithPrecision) {
    state.precision = 6;   // threshold becomes sci_exp > 9
    Formatter fmt(state);
    // sci_exp = 9 → still decimal
    EXPECT_EQ(fmt.format(Value::make_float(1, 9)), "1000000000.");
    // sci_exp = 10 → scientific
    EXPECT_EQ(fmt.format(Value::make_float(1, 10)), "1e10");
}

// --- Float display (Scientific) ---

TEST_F(FormatterTest, FloatSci) {
    state.display_format = DisplayFormat::Sci;
    Formatter fmt(state);
    // 1.5 -> 1.5e0
    EXPECT_EQ(fmt.format(Value::make_float(15, -1)), "1.5e0");
    // 150 -> 1.5e2
    EXPECT_EQ(fmt.format(Value::make_float(15, 1)), "1.5e2");
}

TEST_F(FormatterTest, FloatSciWithDigits) {
    state.display_format = DisplayFormat::Sci;
    state.display_digits = 4;
    Formatter fmt(state);
    // pi ~= 3.142 at 4 sig figs
    EXPECT_EQ(fmt.format(Value::make_float(31416, -4)), "3.141e0");
}

// --- Float display (Engineering) ---

TEST_F(FormatterTest, FloatEng) {
    state.display_format = DisplayFormat::Eng;
    Formatter fmt(state);
    // 1500 -> 1.5e3
    EXPECT_EQ(fmt.format(Value::make_float(15, 2)), "1.5e3");
}

// --- Float display (Fixed) ---

TEST_F(FormatterTest, FloatFix) {
    state.display_format = DisplayFormat::Fix;
    state.display_digits = 2;
    Formatter fmt(state);
    EXPECT_EQ(fmt.format(Value::make_float(15, -1)), "1.50");
    EXPECT_EQ(fmt.format(Value::make_float(1, 2)), "100.00");
}

// --- Complex display ---

TEST_F(FormatterTest, ComplexPair) {
    Formatter fmt(state);
    auto c = Value::make_rect_complex(Value::make_integer(3), Value::make_integer(4));
    EXPECT_EQ(fmt.format(c), "(3, 4)");
}

TEST_F(FormatterTest, ComplexINotation) {
    state.complex_format = ComplexFormat::INotation;
    Formatter fmt(state);
    auto c = Value::make_rect_complex(Value::make_integer(3), Value::make_integer(4));
    EXPECT_EQ(fmt.format(c), "3 + 4i");
}

TEST_F(FormatterTest, ComplexINotationNegIm) {
    state.complex_format = ComplexFormat::INotation;
    Formatter fmt(state);
    auto c = Value::make_rect_complex(Value::make_integer(3), Value::make_integer(-4));
    EXPECT_EQ(fmt.format(c), "3 - 4i");
}

TEST_F(FormatterTest, ComplexJNotation) {
    state.complex_format = ComplexFormat::JNotation;
    Formatter fmt(state);
    auto c = Value::make_rect_complex(Value::make_integer(3), Value::make_integer(4));
    EXPECT_EQ(fmt.format(c), "3 + 4j");
}

// --- HMS display ---

TEST_F(FormatterTest, HMS) {
    Formatter fmt(state);
    auto h = Value::make_hms(2, 30, Value::make_integer(15));
    EXPECT_EQ(fmt.format(h), "2@ 30' 15\"");
}

TEST_F(FormatterTest, HMSNeg) {
    Formatter fmt(state);
    auto h = Value::make_hms(-2, 30, Value::make_integer(0));
    EXPECT_EQ(fmt.format(h), "-2@ 30' 0\"");
}

// --- Error form display ---

TEST_F(FormatterTest, ErrorForm) {
    Formatter fmt(state);
    auto e = Value::make_error(Value::make_integer(10), Value::make_integer(2));
    EXPECT_EQ(fmt.format(e), "10 +/- 2");
}

// --- Modulo form display ---

TEST_F(FormatterTest, ModuloForm) {
    Formatter fmt(state);
    auto m = Value::make_mod(Value::make_integer(3), Value::make_integer(7));
    EXPECT_EQ(fmt.format(m), "3 mod 7");
}

// --- Interval display ---

TEST_F(FormatterTest, IntervalClosed) {
    Formatter fmt(state);
    auto iv = Value::make_interval(3, Value::make_integer(1), Value::make_integer(5));
    EXPECT_EQ(fmt.format(iv), "[1 .. 5]");
}

TEST_F(FormatterTest, IntervalOpen) {
    Formatter fmt(state);
    auto iv = Value::make_interval(0, Value::make_integer(1), Value::make_integer(5));
    EXPECT_EQ(fmt.format(iv), "(1 .. 5)");
}

TEST_F(FormatterTest, IntervalHalfOpen) {
    Formatter fmt(state);
    auto iv = Value::make_interval(2, Value::make_integer(1), Value::make_integer(5));
    EXPECT_EQ(fmt.format(iv), "[1 .. 5)");
}

// --- Vector display ---

TEST_F(FormatterTest, Vector) {
    Formatter fmt(state);
    auto v = Value::make_vector({Value::make_integer(1), Value::make_integer(2), Value::make_integer(3)});
    EXPECT_EQ(fmt.format(v), "[1, 2, 3]");
}

TEST_F(FormatterTest, Matrix) {
    Formatter fmt(state);
    auto m = Value::make_vector({
        Value::make_vector({Value::make_integer(1), Value::make_integer(2)}),
        Value::make_vector({Value::make_integer(3), Value::make_integer(4)})
    });
    EXPECT_EQ(fmt.format(m), "[[1, 2], [3, 4]]");
}

// --- Infinity display ---

TEST_F(FormatterTest, Infinity) {
    Formatter fmt(state);
    EXPECT_EQ(fmt.format(Value::make_infinity(Infinity::Pos)), "inf");
    EXPECT_EQ(fmt.format(Value::make_infinity(Infinity::Neg)), "-inf");
    EXPECT_EQ(fmt.format(Value::make_infinity(Infinity::Undirected)), "uinf");
    EXPECT_EQ(fmt.format(Value::make_infinity(Infinity::NaN)), "nan");
}
