#include <gtest/gtest.h>
#include "trail.h"

using namespace sc;

TEST(TrailTest, RecordAndSize) {
    Trail t;
    EXPECT_TRUE(t.empty());
    t.record("add", Value::make_integer(5));
    EXPECT_EQ(t.size(), 1u);
    EXPECT_EQ(t.at(0).tag, "add");
    EXPECT_EQ(t.at(0).value->as_integer().v, 5);
    EXPECT_EQ(t.at(0).index, 1u);
}

TEST(TrailTest, PointerMovement) {
    Trail t;
    t.record("a", Value::make_integer(1));
    t.record("b", Value::make_integer(2));
    t.record("c", Value::make_integer(3));
    EXPECT_EQ(t.pointer(), 2u); // points to last entry

    t.move_pointer(-1);
    EXPECT_EQ(t.pointer(), 1u);
    t.move_pointer(-5); // clamp to 0
    EXPECT_EQ(t.pointer(), 0u);
    t.move_pointer(100); // clamp to last
    EXPECT_EQ(t.pointer(), 2u);
}

TEST(TrailTest, Yank) {
    Trail t;
    t.record("a", Value::make_integer(10));
    t.record("b", Value::make_integer(20));
    t.record("c", Value::make_integer(30));

    auto v = t.yank();
    EXPECT_EQ(v->as_integer().v, 30);

    t.move_pointer(-1);
    v = t.yank();
    EXPECT_EQ(v->as_integer().v, 20);

    v = t.yank(-1); // relative offset
    EXPECT_EQ(v->as_integer().v, 10);
}

TEST(TrailTest, YankEmpty) {
    Trail t;
    EXPECT_EQ(t.yank(), nullptr);
}

TEST(TrailTest, SetPointer) {
    Trail t;
    t.record("a", Value::make_integer(1));
    t.record("b", Value::make_integer(2));
    t.set_pointer(0);
    EXPECT_EQ(t.pointer(), 0u);
    t.set_pointer(100);
    EXPECT_EQ(t.pointer(), 1u); // clamped
}
