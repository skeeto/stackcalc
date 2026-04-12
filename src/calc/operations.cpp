#include "operations.h"

namespace sc::ops {

// Helper: binary op through the stack
static void binary_op(Stack& s, const char* tag,
                      ValuePtr(*fn)(const ValuePtr&, const ValuePtr&, int)) {
    s.begin_command();
    auto b = s.pop();
    auto a = s.pop();
    s.set_last_args({a, b});
    auto result = fn(a, b, s.state().precision);
    if (s.state().keep_args_flag) { s.push(a); s.push(b); }
    s.push(result);
    s.end_command(tag, {result});
}

static void unary_op(Stack& s, const char* tag, ValuePtr(*fn)(const ValuePtr&)) {
    s.begin_command();
    auto a = s.pop();
    s.set_last_args({a});
    auto result = fn(a);
    if (s.state().keep_args_flag) s.push(a);
    s.push(result);
    s.end_command(tag, {result});
}

void add(Stack& s) { binary_op(s, "add", arith::add); }
void sub(Stack& s) { binary_op(s, "sub", arith::sub); }
void mul(Stack& s) { binary_op(s, "mul", arith::mul); }

void div(Stack& s) {
    s.begin_command();
    auto b = s.pop();
    auto a = s.pop();
    s.set_last_args({a, b});
    auto result = arith::div(a, b, s.state().precision, s.state().fraction_mode);
    if (s.state().keep_args_flag) { s.push(a); s.push(b); }
    s.push(result);
    s.end_command("div", {result});
}

void idiv(Stack& s) { binary_op(s, "idiv", arith::idiv); }
void mod(Stack& s) { binary_op(s, "mod", arith::mod); }
void power(Stack& s) { binary_op(s, "pow", arith::power); }

void neg(Stack& s) { unary_op(s, "neg", arith::neg); }
void abs_op(Stack& s) { unary_op(s, "abs", arith::abs); }

void inv(Stack& s) {
    s.begin_command();
    auto a = s.pop();
    s.set_last_args({a});
    auto result = arith::inv(a, s.state().precision);
    if (s.state().keep_args_flag) s.push(a);
    s.push(result);
    s.end_command("inv", {result});
}

void sqrt_op(Stack& s) {
    s.begin_command();
    auto a = s.pop();
    s.set_last_args({a});
    auto result = arith::sqrt(a, s.state().precision);
    if (s.state().keep_args_flag) s.push(a);
    s.push(result);
    s.end_command("sqrt", {result});
}

void floor_op(Stack& s) { unary_op(s, "flor", arith::floor); }
void ceil_op(Stack& s) { unary_op(s, "ceil", arith::ceil); }
void round_op(Stack& s) { unary_op(s, "rnd", arith::round); }
void trunc_op(Stack& s) { unary_op(s, "trnc", arith::trunc); }

} // namespace sc::ops
