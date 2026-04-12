#include <gtest/gtest.h>
#include "modulo_form.h"
#include "arithmetic.h"

using namespace sc;

TEST(ModuloFormTest, Reduce) {
    auto r = modulo_form::reduce(Value::make_integer(17), Value::make_integer(5), 12);
    ASSERT_TRUE(r->is_mod());
    EXPECT_EQ(r->as_mod().n->as_integer().v, 2);
    EXPECT_EQ(r->as_mod().m->as_integer().v, 5);
}

TEST(ModuloFormTest, Add) {
    // 3 mod 7 + 5 mod 7 = 8 mod 7 = 1 mod 7
    ModuloForm a{Value::make_integer(3), Value::make_integer(7)};
    ModuloForm b{Value::make_integer(5), Value::make_integer(7)};
    auto r = modulo_form::add(a, b, 12);
    ASSERT_TRUE(r->is_mod());
    EXPECT_EQ(r->as_mod().n->as_integer().v, 1);
    EXPECT_EQ(r->as_mod().m->as_integer().v, 7);
}

TEST(ModuloFormTest, Sub) {
    // 3 mod 7 - 5 mod 7 = -2 mod 7 = 5 mod 7
    ModuloForm a{Value::make_integer(3), Value::make_integer(7)};
    ModuloForm b{Value::make_integer(5), Value::make_integer(7)};
    auto r = modulo_form::sub(a, b, 12);
    ASSERT_TRUE(r->is_mod());
    EXPECT_EQ(r->as_mod().n->as_integer().v, 5);
    EXPECT_EQ(r->as_mod().m->as_integer().v, 7);
}

TEST(ModuloFormTest, Mul) {
    // 3 mod 7 * 5 mod 7 = 15 mod 7 = 1 mod 7
    ModuloForm a{Value::make_integer(3), Value::make_integer(7)};
    ModuloForm b{Value::make_integer(5), Value::make_integer(7)};
    auto r = modulo_form::mul(a, b, 12);
    ASSERT_TRUE(r->is_mod());
    EXPECT_EQ(r->as_mod().n->as_integer().v, 1);
    EXPECT_EQ(r->as_mod().m->as_integer().v, 7);
}

TEST(ModuloFormTest, Neg) {
    // -(3 mod 7) = -3 mod 7 = 4 mod 7
    ModuloForm a{Value::make_integer(3), Value::make_integer(7)};
    auto r = modulo_form::neg(a, 12);
    ASSERT_TRUE(r->is_mod());
    EXPECT_EQ(r->as_mod().n->as_integer().v, 4);
    EXPECT_EQ(r->as_mod().m->as_integer().v, 7);
}

TEST(ModuloFormTest, Pow) {
    // 2^10 mod 7 = 1024 mod 7 = 2
    ModuloForm a{Value::make_integer(2), Value::make_integer(7)};
    auto r = modulo_form::pow(a, Value::make_integer(10), 12);
    ASSERT_TRUE(r->is_mod());
    EXPECT_EQ(r->as_mod().n->as_integer().v, 2);
    EXPECT_EQ(r->as_mod().m->as_integer().v, 7);
}

TEST(ModuloFormTest, DifferentModuliThrows) {
    ModuloForm a{Value::make_integer(3), Value::make_integer(7)};
    ModuloForm b{Value::make_integer(5), Value::make_integer(11)};
    EXPECT_THROW(modulo_form::add(a, b, 12), std::invalid_argument);
}

TEST(ModuloFormTest, ArithDispatch) {
    auto a = Value::make_mod(Value::make_integer(3), Value::make_integer(7));
    auto b = Value::make_mod(Value::make_integer(5), Value::make_integer(7));
    auto r = arith::add(a, b, 12);
    ASSERT_TRUE(r->is_mod());
    EXPECT_EQ(r->as_mod().n->as_integer().v, 1);
}
