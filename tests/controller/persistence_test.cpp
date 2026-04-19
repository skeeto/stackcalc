#include <gtest/gtest.h>
#include "persistence.hpp"
#include "controller.hpp"
#include <sstream>

using namespace sc;

namespace {

// Round-trip a single ValuePtr through write_value / read_value.
ValuePtr round_trip(const ValuePtr& v) {
    std::ostringstream out;
    persistence::write_value(out, v);
    std::istringstream in(out.str());
    return persistence::read_value(in);
}

bool same_value(const ValuePtr& a, const ValuePtr& b) {
    return value_equal(a, b);
}

void feed_chars(Controller& c, const std::string& s) {
    for (char ch : s) c.process_key(KeyEvent::character(ch));
}
void feed_special(Controller& c, const std::string& n) {
    c.process_key(KeyEvent::special(n));
}

} // anon namespace

// --- Per-value-type round-trips ---

TEST(PersistenceTest, IntegerRoundTrip) {
    auto v = Value::make_integer(mpz_class("12345678901234567890"));
    EXPECT_TRUE(same_value(round_trip(v), v));
}

TEST(PersistenceTest, FractionRoundTrip) {
    auto v = Value::make_fraction(mpz_class(355), mpz_class(113));
    EXPECT_TRUE(same_value(round_trip(v), v));
}

TEST(PersistenceTest, FloatRoundTrip) {
    auto v = Value::make_float(mpz_class("314159265359"), -11);
    EXPECT_TRUE(same_value(round_trip(v), v));
}

TEST(PersistenceTest, NegativeFloatRoundTrip) {
    auto v = Value::make_float(mpz_class("-25"), -1);
    EXPECT_TRUE(same_value(round_trip(v), v));
}

TEST(PersistenceTest, RectComplexRoundTrip) {
    auto v = Value::make_rect_complex(Value::make_integer(3),
                                      Value::make_integer(4));
    EXPECT_TRUE(same_value(round_trip(v), v));
}

TEST(PersistenceTest, PolarComplexRoundTrip) {
    auto v = Value::make_polar_complex(Value::make_integer(5),
                                       Value::make_float(mpz_class(7853), -4));
    EXPECT_TRUE(same_value(round_trip(v), v));
}

TEST(PersistenceTest, HMSRoundTrip) {
    auto v = Value::make_hms(2, 30, Value::make_float(mpz_class(155), -1));
    EXPECT_TRUE(same_value(round_trip(v), v));
}

TEST(PersistenceTest, ErrorFormRoundTrip) {
    auto v = Value::make_error(Value::make_integer(5),
                               Value::make_float(mpz_class(2), -1));
    EXPECT_TRUE(same_value(round_trip(v), v));
}

TEST(PersistenceTest, ModuloFormRoundTrip) {
    auto v = Value::make_mod(Value::make_integer(3),
                             Value::make_integer(7));
    EXPECT_TRUE(same_value(round_trip(v), v));
}

TEST(PersistenceTest, IntervalRoundTrip) {
    auto v = Value::make_interval(3, Value::make_integer(1),
                                  Value::make_integer(5));
    EXPECT_TRUE(same_value(round_trip(v), v));
}

TEST(PersistenceTest, VectorRoundTrip) {
    auto v = Value::make_vector({Value::make_integer(1),
                                 Value::make_integer(2),
                                 Value::make_integer(3)});
    EXPECT_TRUE(same_value(round_trip(v), v));
}

TEST(PersistenceTest, NestedVectorRoundTrip) {
    // Matrix-like: vector of vectors.
    auto row1 = Value::make_vector({Value::make_integer(1), Value::make_integer(2)});
    auto row2 = Value::make_vector({Value::make_integer(3), Value::make_integer(4)});
    auto m = Value::make_vector({row1, row2});
    EXPECT_TRUE(same_value(round_trip(m), m));
}

TEST(PersistenceTest, InfinityRoundTrip) {
    EXPECT_TRUE(same_value(round_trip(Value::make_infinity(Infinity::Pos)),
                           Value::make_infinity(Infinity::Pos)));
    EXPECT_TRUE(same_value(round_trip(Value::make_infinity(Infinity::Neg)),
                           Value::make_infinity(Infinity::Neg)));
    EXPECT_TRUE(same_value(round_trip(Value::make_infinity(Infinity::NaN)),
                           Value::make_infinity(Infinity::NaN)));
    EXPECT_TRUE(same_value(round_trip(Value::make_infinity(Infinity::Undirected)),
                           Value::make_infinity(Infinity::Undirected)));
}

TEST(PersistenceTest, DateRoundTrip) {
    auto v = Value::make_date(Value::make_integer(730000));
    EXPECT_TRUE(same_value(round_trip(v), v));
}

// --- Full controller state round-trip ---

TEST(PersistenceTest, FullControllerRoundTrip) {
    Controller src;
    feed_chars(src, "5"); feed_special(src, "RET");
    feed_chars(src, "7"); feed_special(src, "RET");
    feed_chars(src, "+");                              // [12], trail entries
    feed_chars(src, "3"); feed_special(src, "RET");    // [12, 3]
    feed_chars(src, "ssa");                            // a := 3
    feed_chars(src, "t5");                             // q5 := 3
    feed_chars(src, "p");                              // pop 3 -> precision = 3
    feed_chars(src, "d6");                             // radix 16
    feed_chars(src, "mr");                             // radians

    std::ostringstream out;
    persistence::save(out, src);

    Controller dst;
    std::istringstream in(out.str());
    ASSERT_TRUE(persistence::load(in, dst));

    auto sds = src.display();
    auto dds = dst.display();

    // Stack: same values in same order.
    EXPECT_EQ(sds.stack_depth, dds.stack_depth);
    for (size_t i = 0; i < sds.stack_entries.size(); ++i) {
        EXPECT_EQ(sds.stack_entries[i], dds.stack_entries[i]);
    }

    // Trail
    EXPECT_EQ(sds.trail_entries.size(), dds.trail_entries.size());
    for (size_t i = 0; i < sds.trail_entries.size(); ++i) {
        EXPECT_EQ(sds.trail_entries[i], dds.trail_entries[i]);
    }
    EXPECT_EQ(sds.trail_pointer, dds.trail_pointer);

    // Modes
    EXPECT_EQ(src.stack().state().precision,        dst.stack().state().precision);
    EXPECT_EQ(src.stack().state().display_radix,    dst.stack().state().display_radix);
    EXPECT_EQ(src.stack().state().angular_mode,     dst.stack().state().angular_mode);
    EXPECT_EQ(src.stack().state().group_digits,     dst.stack().state().group_digits);

    // Variables
    EXPECT_TRUE(dst.variables().exists("a"));
    EXPECT_TRUE(dst.variables().recall_quick(5).has_value());
}

TEST(PersistenceTest, UndoHistorySurvivesRoundTrip) {
    Controller src;
    feed_chars(src, "5"); feed_special(src, "RET");
    feed_chars(src, "7"); feed_special(src, "RET");
    feed_chars(src, "+");                              // [12]; one undo frame
    feed_chars(src, "U");                              // [5, 7]; one redo frame

    std::ostringstream out;
    persistence::save(out, src);

    Controller dst;
    std::istringstream in(out.str());
    ASSERT_TRUE(persistence::load(in, dst));

    // After redo, dst should reach the same [12] state src had before undo.
    feed_chars(dst, "D");
    EXPECT_EQ(dst.display().stack_depth, 1);
    EXPECT_EQ(dst.display().stack_entries.back(), "12");
}

TEST(PersistenceTest, MissingHeaderFails) {
    Controller dst;
    feed_chars(dst, "9"); feed_special(dst, "RET");  // pre-load state
    std::istringstream in("(stack (int 5))\n");
    EXPECT_FALSE(persistence::load(in, dst));
    // Failed load resets to pristine.
    EXPECT_EQ(dst.display().stack_depth, 0);
}

TEST(PersistenceTest, WrongVersionFails) {
    Controller dst;
    std::istringstream in("(stackcalc-state 999)\n(stack (int 5))\n");
    EXPECT_FALSE(persistence::load(in, dst));
    EXPECT_EQ(dst.display().stack_depth, 0);
}

TEST(PersistenceTest, UnknownFormsAreIgnored) {
    // Forward compatibility: unknown forms (e.g., a future GUI's
    // (window ...) record) shouldn't break load.
    Controller dst;
    std::istringstream in(
        "(stackcalc-state 1)\n"
        "(stack (int 42))\n"
        "(window 960 640 480)\n"
        "(unknown_thing (with (nested) data))\n"
    );
    ASSERT_TRUE(persistence::load(in, dst));
    EXPECT_EQ(dst.display().stack_depth, 1);
    EXPECT_EQ(dst.display().stack_entries.back(), "42");
}
