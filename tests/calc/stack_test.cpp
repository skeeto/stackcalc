#include <gtest/gtest.h>
#include "stack.hpp"

using namespace sc;

TEST(StackTest, PushPop) {
    Stack s;
    s.push(Value::make_integer(42));
    EXPECT_EQ(s.size(), 1u);
    auto v = s.pop();
    EXPECT_EQ(v->as_integer().v, 42);
    EXPECT_TRUE(s.empty());
}

TEST(StackTest, PopEmpty) {
    Stack s;
    EXPECT_THROW(s.pop(), StackError);
}

TEST(StackTest, TopDepth) {
    Stack s;
    s.push(Value::make_integer(1));
    s.push(Value::make_integer(2));
    s.push(Value::make_integer(3));
    EXPECT_EQ(s.top(0)->as_integer().v, 3);  // top
    EXPECT_EQ(s.top(1)->as_integer().v, 2);  // second
    EXPECT_EQ(s.top(2)->as_integer().v, 1);  // third
}

TEST(StackTest, TopN) {
    Stack s;
    s.push(Value::make_integer(1));
    s.push(Value::make_integer(2));
    s.push(Value::make_integer(3));
    auto vals = s.top_n(2);
    EXPECT_EQ(vals.size(), 2u);
    EXPECT_EQ(vals[0]->as_integer().v, 2);
    EXPECT_EQ(vals[1]->as_integer().v, 3);
}

TEST(StackTest, PopN) {
    Stack s;
    s.push(Value::make_integer(1));
    s.push(Value::make_integer(2));
    s.push(Value::make_integer(3));
    auto vals = s.pop_n(2);
    EXPECT_EQ(vals.size(), 2u);
    EXPECT_EQ(vals[0]->as_integer().v, 2);
    EXPECT_EQ(vals[1]->as_integer().v, 3);
    EXPECT_EQ(s.size(), 1u);
    EXPECT_EQ(s.top()->as_integer().v, 1);
}

TEST(StackTest, Dup) {
    Stack s;
    s.push(Value::make_integer(42));
    s.dup();
    EXPECT_EQ(s.size(), 2u);
    EXPECT_EQ(s.top(0)->as_integer().v, 42);
    EXPECT_EQ(s.top(1)->as_integer().v, 42);
}

TEST(StackTest, DupDepth) {
    Stack s;
    s.push(Value::make_integer(1));
    s.push(Value::make_integer(2));
    s.push(Value::make_integer(3));
    s.dup(2); // duplicate element at depth 2 (= 1)
    EXPECT_EQ(s.size(), 4u);
    EXPECT_EQ(s.top()->as_integer().v, 1);
}

TEST(StackTest, Drop) {
    Stack s;
    s.push(Value::make_integer(1));
    s.push(Value::make_integer(2));
    s.drop();
    EXPECT_EQ(s.size(), 1u);
    EXPECT_EQ(s.top()->as_integer().v, 1);
}

TEST(StackTest, Swap) {
    Stack s;
    s.push(Value::make_integer(1));
    s.push(Value::make_integer(2));
    s.swap();
    EXPECT_EQ(s.top(0)->as_integer().v, 1);
    EXPECT_EQ(s.top(1)->as_integer().v, 2);
}

TEST(StackTest, RollDown3) {
    Stack s;
    s.push(Value::make_integer(1));
    s.push(Value::make_integer(2));
    s.push(Value::make_integer(3));
    // Roll down 3: top goes to bottom of group
    // [1, 2, 3] -> [1, 3, 2] wait no...
    // Roll down n: top element moves down n positions
    // [1, 2, 3] with roll_down(3) -> [3, 1, 2]
    s.roll_down(3);
    EXPECT_EQ(s.top(0)->as_integer().v, 2);
    EXPECT_EQ(s.top(1)->as_integer().v, 1);
    EXPECT_EQ(s.top(2)->as_integer().v, 3);
}

TEST(StackTest, RollUp3) {
    Stack s;
    s.push(Value::make_integer(1));
    s.push(Value::make_integer(2));
    s.push(Value::make_integer(3));
    // Roll up 3: bottom-of-group moves to top
    // [1, 2, 3] -> [2, 3, 1]
    s.roll_up(3);
    EXPECT_EQ(s.top(0)->as_integer().v, 1);
    EXPECT_EQ(s.top(1)->as_integer().v, 3);
    EXPECT_EQ(s.top(2)->as_integer().v, 2);
}

TEST(StackTest, SwapTooFew) {
    Stack s;
    s.push(Value::make_integer(1));
    EXPECT_THROW(s.swap(), StackError);
}
