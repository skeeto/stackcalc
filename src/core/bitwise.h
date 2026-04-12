#pragma once

#include "value.h"

namespace sc::bitwise {

// Bitwise operations on integers.
// Word size determines the effective bit width for NOT, shifts, and rotations.
// word_size=0 means infinite precision (NOT not available).

ValuePtr band(const ValuePtr& a, const ValuePtr& b);      // AND
ValuePtr bor(const ValuePtr& a, const ValuePtr& b);       // OR
ValuePtr bxor(const ValuePtr& a, const ValuePtr& b);      // XOR
ValuePtr bnot(const ValuePtr& a, int word_size);           // NOT (complement)
ValuePtr lshift(const ValuePtr& a, int bits, int word_size);   // left shift
ValuePtr rshift(const ValuePtr& a, int bits, int word_size);   // right (arithmetic) shift
ValuePtr lrot(const ValuePtr& a, int bits, int word_size);     // left rotate
ValuePtr rrot(const ValuePtr& a, int bits, int word_size);     // right rotate

// Two's complement conversions
ValuePtr to_twos_complement(const ValuePtr& a, int word_size);
ValuePtr from_twos_complement(const ValuePtr& a, int word_size);

// Clip to word size (mask to low N bits)
ValuePtr clip(const ValuePtr& a, int word_size);

} // namespace sc::bitwise
