#pragma once

#include "value.hpp"
#include "modes.hpp"

namespace sc::vector_ops {

// --- Construction ---
ValuePtr build(std::vector<ValuePtr> elements);
ValuePtr cvec(const ValuePtr& val, int n);  // n copies of val
ValuePtr index(int n);                      // [1, 2, ..., n]
ValuePtr identity(int n);                   // n x n identity matrix
ValuePtr diagonal(const ValuePtr& v);       // diagonal matrix from vector

// --- Deconstruction ---
ValuePtr head(const Vector& v);
ValuePtr tail(const Vector& v);
ValuePtr cons(const ValuePtr& elem, const Vector& v);   // prepend
ValuePtr rcons(const Vector& v, const ValuePtr& elem);  // append
int length(const Vector& v);
ValuePtr extract(const Vector& v, int index);  // 1-based indexing

// --- Manipulation ---
ValuePtr reverse(const Vector& v);
ValuePtr subvec(const Vector& v, int start, int end);  // 1-based, inclusive start, exclusive end
ValuePtr concat(const Vector& a, const Vector& b);
ValuePtr sort(const Vector& v, int precision);
ValuePtr rsort(const Vector& v, int precision);

// --- Element-wise arithmetic ---
// These apply an operation element-wise; scalar broadcasts.
ValuePtr map_add(const ValuePtr& a, const ValuePtr& b, int precision);
ValuePtr map_sub(const ValuePtr& a, const ValuePtr& b, int precision);
ValuePtr map_mul(const ValuePtr& a, const ValuePtr& b, int precision);
ValuePtr map_div(const ValuePtr& a, const ValuePtr& b, int precision, FractionMode fm = FractionMode::Float);
ValuePtr map_neg(const Vector& v);

// --- Reduce operations ---
ValuePtr reduce_add(const Vector& v, int precision);    // sum
ValuePtr reduce_mul(const Vector& v, int precision);    // product
ValuePtr reduce_max(const Vector& v, int precision);
ValuePtr reduce_min(const Vector& v, int precision);

// --- Linear algebra ---
ValuePtr dot(const Vector& a, const Vector& b, int precision);
ValuePtr cross(const Vector& a, const Vector& b, int precision);  // 3-element only
ValuePtr transpose(const Vector& m);
ValuePtr determinant(const Vector& m, int precision);
ValuePtr inverse(const Vector& m, int precision);
ValuePtr trace(const Vector& m, int precision);

// --- Predicates ---
bool is_matrix(const Vector& v);
int matrix_rows(const Vector& v);
int matrix_cols(const Vector& v);

} // namespace sc::vector_ops
