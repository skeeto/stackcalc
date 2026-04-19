#include <gtest/gtest.h>
#include "variables.hpp"
#include <algorithm>

using namespace sc;

TEST(VariablesTest, StoreRecall) {
    Variables vars;
    vars.store("x", Value::make_integer(42));
    auto v = vars.recall("x");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ((*v)->as_integer().v, 42);
}

TEST(VariablesTest, RecallMissing) {
    Variables vars;
    EXPECT_FALSE(vars.recall("x").has_value());
}

TEST(VariablesTest, Unstore) {
    Variables vars;
    vars.store("x", Value::make_integer(42));
    vars.unstore("x");
    EXPECT_FALSE(vars.recall("x").has_value());
}

TEST(VariablesTest, Overwrite) {
    Variables vars;
    vars.store("x", Value::make_integer(1));
    vars.store("x", Value::make_integer(2));
    EXPECT_EQ((*vars.recall("x"))->as_integer().v, 2);
}

TEST(VariablesTest, QuickRegisters) {
    Variables vars;
    vars.store_quick(0, Value::make_integer(100));
    vars.store_quick(9, Value::make_integer(999));
    EXPECT_EQ((*vars.recall_quick(0))->as_integer().v, 100);
    EXPECT_EQ((*vars.recall_quick(9))->as_integer().v, 999);
    EXPECT_FALSE(vars.recall_quick(5).has_value());
}

TEST(VariablesTest, StoreAdd) {
    Variables vars;
    vars.store("x", Value::make_integer(10));
    vars.store_add("x", Value::make_integer(5), 12);
    EXPECT_EQ((*vars.recall("x"))->as_integer().v, 15);
}

TEST(VariablesTest, StoreSub) {
    Variables vars;
    vars.store("x", Value::make_integer(10));
    vars.store_sub("x", Value::make_integer(3), 12);
    EXPECT_EQ((*vars.recall("x"))->as_integer().v, 7);
}

TEST(VariablesTest, StoreMul) {
    Variables vars;
    vars.store("x", Value::make_integer(10));
    vars.store_mul("x", Value::make_integer(3), 12);
    EXPECT_EQ((*vars.recall("x"))->as_integer().v, 30);
}

TEST(VariablesTest, StoreDiv) {
    Variables vars;
    vars.store("x", Value::make_integer(10));
    vars.store_div("x", Value::make_integer(2), 12);
    EXPECT_EQ((*vars.recall("x"))->as_integer().v, 5);
}

TEST(VariablesTest, Exchange) {
    Variables vars;
    vars.store("x", Value::make_integer(10));
    auto old = vars.exchange("x", Value::make_integer(20));
    EXPECT_EQ(old->as_integer().v, 10);
    EXPECT_EQ((*vars.recall("x"))->as_integer().v, 20);
}

TEST(VariablesTest, Exists) {
    Variables vars;
    EXPECT_FALSE(vars.exists("x"));
    vars.store("x", Value::make_integer(1));
    EXPECT_TRUE(vars.exists("x"));
}

TEST(VariablesTest, Names) {
    Variables vars;
    vars.store("a", Value::make_integer(1));
    vars.store("b", Value::make_integer(2));
    auto names = vars.names();
    EXPECT_EQ(names.size(), 2u);
    // Contains both names (order unspecified)
    std::sort(names.begin(), names.end());
    EXPECT_EQ(names[0], "a");
    EXPECT_EQ(names[1], "b");
}

TEST(VariablesTest, StoreAddMissingThrows) {
    Variables vars;
    EXPECT_THROW(vars.store_add("x", Value::make_integer(1), 12), std::invalid_argument);
}
