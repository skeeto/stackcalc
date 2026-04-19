#pragma once

#include "stack.hpp"
#include "arithmetic.hpp"

namespace sc::ops {

// Stack-level operations: pop args, apply arith::, push result, record trail/undo.

// Binary arithmetic
void add(Stack& s);
void sub(Stack& s);
void mul(Stack& s);
void div(Stack& s);
void idiv(Stack& s);
void mod(Stack& s);
void power(Stack& s);

// Unary arithmetic
void neg(Stack& s);
void inv(Stack& s);
void sqrt_op(Stack& s);
void abs_op(Stack& s);

// Rounding
void floor_op(Stack& s);
void ceil_op(Stack& s);
void round_op(Stack& s);
void trunc_op(Stack& s);

} // namespace sc::ops
