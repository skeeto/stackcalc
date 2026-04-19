#include <gtest/gtest.h>
#include "stack.hpp"

using namespace sc;

TEST(UndoTest, BasicUndo) {
    Stack s;
    s.push(Value::make_integer(1));
    s.push(Value::make_integer(2));

    // Perform an operation that records undo
    s.dup();
    EXPECT_EQ(s.size(), 3u);

    s.undo();
    EXPECT_EQ(s.size(), 2u);
    EXPECT_EQ(s.top()->as_integer().v, 2);
}

TEST(UndoTest, UndoRedo) {
    Stack s;
    s.push(Value::make_integer(1));
    s.push(Value::make_integer(2));
    s.swap();
    EXPECT_EQ(s.top(0)->as_integer().v, 1);
    EXPECT_EQ(s.top(1)->as_integer().v, 2);

    s.undo();
    EXPECT_EQ(s.top(0)->as_integer().v, 2);
    EXPECT_EQ(s.top(1)->as_integer().v, 1);

    s.redo();
    EXPECT_EQ(s.top(0)->as_integer().v, 1);
    EXPECT_EQ(s.top(1)->as_integer().v, 2);
}

TEST(UndoTest, MultipleUndos) {
    Stack s;
    s.push(Value::make_integer(1));
    s.dup();  // [1, 1]
    s.dup();  // [1, 1, 1]
    EXPECT_EQ(s.size(), 3u);

    s.undo(); // back to [1, 1]
    EXPECT_EQ(s.size(), 2u);
    s.undo(); // back to [1]
    EXPECT_EQ(s.size(), 1u);
}

TEST(UndoTest, RedoClearedByNewCommand) {
    Stack s;
    s.push(Value::make_integer(1));
    s.dup();  // [1, 1]
    s.undo(); // [1]

    // New command clears redo
    s.dup();  // [1, 1]
    s.undo(); // [1]
    // Can't redo to the original dup
    s.redo(); // back to [1, 1]
    EXPECT_EQ(s.size(), 2u);
}
