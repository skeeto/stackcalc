#include "vector.hpp"
#include "arithmetic.hpp"
#include <stdexcept>
#include <algorithm>

namespace sc::vector_ops {

// --- Construction ---

ValuePtr build(std::vector<ValuePtr> elements) {
    return Value::make_vector(std::move(elements));
}

ValuePtr cvec(const ValuePtr& val, int n) {
    std::vector<ValuePtr> elems(n, val);
    return Value::make_vector(std::move(elems));
}

ValuePtr index(int n) {
    std::vector<ValuePtr> elems;
    elems.reserve(n);
    for (int i = 1; i <= n; ++i)
        elems.push_back(Value::make_integer(i));
    return Value::make_vector(std::move(elems));
}

ValuePtr identity(int n) {
    std::vector<ValuePtr> rows;
    rows.reserve(n);
    for (int i = 0; i < n; ++i) {
        std::vector<ValuePtr> row(n, Value::zero());
        row[i] = Value::one();
        rows.push_back(Value::make_vector(std::move(row)));
    }
    return Value::make_vector(std::move(rows));
}

ValuePtr diagonal(const ValuePtr& v) {
    if (!v->is_vector()) throw std::invalid_argument("diagonal requires a vector");
    auto& elems = v->as_vector().elements;
    int n = static_cast<int>(elems.size());
    std::vector<ValuePtr> rows;
    rows.reserve(n);
    for (int i = 0; i < n; ++i) {
        std::vector<ValuePtr> row(n, Value::zero());
        row[i] = elems[i];
        rows.push_back(Value::make_vector(std::move(row)));
    }
    return Value::make_vector(std::move(rows));
}

// --- Deconstruction ---

ValuePtr head(const Vector& v) {
    if (v.elements.empty()) throw std::out_of_range("head of empty vector");
    return v.elements.front();
}

ValuePtr tail(const Vector& v) {
    if (v.elements.empty()) throw std::out_of_range("tail of empty vector");
    std::vector<ValuePtr> rest(v.elements.begin() + 1, v.elements.end());
    return Value::make_vector(std::move(rest));
}

ValuePtr cons(const ValuePtr& elem, const Vector& v) {
    std::vector<ValuePtr> elems;
    elems.reserve(v.elements.size() + 1);
    elems.push_back(elem);
    elems.insert(elems.end(), v.elements.begin(), v.elements.end());
    return Value::make_vector(std::move(elems));
}

ValuePtr rcons(const Vector& v, const ValuePtr& elem) {
    std::vector<ValuePtr> elems = v.elements;
    elems.push_back(elem);
    return Value::make_vector(std::move(elems));
}

int length(const Vector& v) {
    return static_cast<int>(v.elements.size());
}

ValuePtr extract(const Vector& v, int index) {
    if (index < 1 || index > static_cast<int>(v.elements.size()))
        throw std::out_of_range("vector index out of range");
    return v.elements[index - 1];
}

// --- Manipulation ---

ValuePtr reverse(const Vector& v) {
    std::vector<ValuePtr> elems(v.elements.rbegin(), v.elements.rend());
    return Value::make_vector(std::move(elems));
}

ValuePtr subvec(const Vector& v, int start, int end) {
    // 1-based, [start, end)
    if (start < 1) start = 1;
    if (end > static_cast<int>(v.elements.size()) + 1)
        end = static_cast<int>(v.elements.size()) + 1;
    std::vector<ValuePtr> elems(v.elements.begin() + start - 1, v.elements.begin() + end - 1);
    return Value::make_vector(std::move(elems));
}

ValuePtr concat(const Vector& a, const Vector& b) {
    std::vector<ValuePtr> elems;
    elems.reserve(a.elements.size() + b.elements.size());
    elems.insert(elems.end(), a.elements.begin(), a.elements.end());
    elems.insert(elems.end(), b.elements.begin(), b.elements.end());
    return Value::make_vector(std::move(elems));
}

ValuePtr sort(const Vector& v, int precision) {
    std::vector<ValuePtr> elems = v.elements;
    std::sort(elems.begin(), elems.end(), [precision](const ValuePtr& a, const ValuePtr& b) {
        auto diff = arith::sub(a, b, precision);
        return diff->is_negative();
    });
    return Value::make_vector(std::move(elems));
}

ValuePtr rsort(const Vector& v, int precision) {
    std::vector<ValuePtr> elems = v.elements;
    std::sort(elems.begin(), elems.end(), [precision](const ValuePtr& a, const ValuePtr& b) {
        auto diff = arith::sub(b, a, precision);
        return diff->is_negative();
    });
    return Value::make_vector(std::move(elems));
}

// --- Element-wise arithmetic ---

static ValuePtr map_binary(const ValuePtr& a, const ValuePtr& b, int precision,
                           ValuePtr (*op)(const ValuePtr&, const ValuePtr&, int)) {
    bool a_vec = a->is_vector();
    bool b_vec = b->is_vector();

    if (a_vec && b_vec) {
        auto& av = a->as_vector().elements;
        auto& bv = b->as_vector().elements;
        if (av.size() != bv.size())
            throw std::invalid_argument("vector size mismatch");
        std::vector<ValuePtr> result;
        result.reserve(av.size());
        for (size_t i = 0; i < av.size(); ++i)
            result.push_back(op(av[i], bv[i], precision));
        return Value::make_vector(std::move(result));
    }
    if (a_vec) {
        // scalar b broadcast
        auto& av = a->as_vector().elements;
        std::vector<ValuePtr> result;
        result.reserve(av.size());
        for (auto& elem : av)
            result.push_back(op(elem, b, precision));
        return Value::make_vector(std::move(result));
    }
    if (b_vec) {
        // scalar a broadcast
        auto& bv = b->as_vector().elements;
        std::vector<ValuePtr> result;
        result.reserve(bv.size());
        for (auto& elem : bv)
            result.push_back(op(a, elem, precision));
        return Value::make_vector(std::move(result));
    }
    // Both scalars — shouldn't happen, but handle gracefully
    return op(a, b, precision);
}

ValuePtr map_add(const ValuePtr& a, const ValuePtr& b, int precision) {
    return map_binary(a, b, precision, arith::add);
}

ValuePtr map_sub(const ValuePtr& a, const ValuePtr& b, int precision) {
    return map_binary(a, b, precision, arith::sub);
}

// For mul/div we need wrappers since their signatures differ
static ValuePtr mul_wrapper(const ValuePtr& a, const ValuePtr& b, int p) {
    return arith::mul(a, b, p);
}

ValuePtr map_mul(const ValuePtr& a, const ValuePtr& b, int precision) {
    // Matrix multiplication: if both are matrices, do proper matrix mul
    if (a->is_vector() && b->is_vector() && is_matrix(a->as_vector()) && is_matrix(b->as_vector())) {
        auto& am = a->as_vector();
        auto& bm = b->as_vector();
        int ar = matrix_rows(am);
        int ac = matrix_cols(am);
        int br = matrix_rows(bm);
        int bc = matrix_cols(bm);
        if (ac == br) {
            // Matrix multiplication
            std::vector<ValuePtr> rows;
            rows.reserve(ar);
            for (int i = 0; i < ar; ++i) {
                std::vector<ValuePtr> row;
                row.reserve(bc);
                for (int j = 0; j < bc; ++j) {
                    ValuePtr sum = Value::zero();
                    for (int k = 0; k < ac; ++k) {
                        auto aik = am.elements[i]->as_vector().elements[k];
                        auto bkj = bm.elements[k]->as_vector().elements[j];
                        sum = arith::add(sum, arith::mul(aik, bkj, precision), precision);
                    }
                    row.push_back(sum);
                }
                rows.push_back(Value::make_vector(std::move(row)));
            }
            return Value::make_vector(std::move(rows));
        }
    }
    return map_binary(a, b, precision, mul_wrapper);
}

ValuePtr map_div(const ValuePtr& a, const ValuePtr& b, int precision, FractionMode fm) {
    auto div_fn = [fm](const ValuePtr& x, const ValuePtr& y, int p) {
        return arith::div(x, y, p, fm);
    };

    bool a_vec = a->is_vector();
    bool b_vec = b->is_vector();

    if (a_vec && b_vec) {
        auto& av = a->as_vector().elements;
        auto& bv = b->as_vector().elements;
        if (av.size() != bv.size())
            throw std::invalid_argument("vector size mismatch");
        std::vector<ValuePtr> result;
        result.reserve(av.size());
        for (size_t i = 0; i < av.size(); ++i)
            result.push_back(div_fn(av[i], bv[i], precision));
        return Value::make_vector(std::move(result));
    }
    if (a_vec) {
        auto& av = a->as_vector().elements;
        std::vector<ValuePtr> result;
        result.reserve(av.size());
        for (auto& elem : av)
            result.push_back(div_fn(elem, b, precision));
        return Value::make_vector(std::move(result));
    }
    if (b_vec) {
        auto& bv = b->as_vector().elements;
        std::vector<ValuePtr> result;
        result.reserve(bv.size());
        for (auto& elem : bv)
            result.push_back(div_fn(a, elem, precision));
        return Value::make_vector(std::move(result));
    }
    return div_fn(a, b, precision);
}

ValuePtr map_neg(const Vector& v) {
    std::vector<ValuePtr> result;
    result.reserve(v.elements.size());
    for (auto& elem : v.elements)
        result.push_back(arith::neg(elem));
    return Value::make_vector(std::move(result));
}

// --- Reduce operations ---

ValuePtr reduce_add(const Vector& v, int precision) {
    if (v.elements.empty()) return Value::zero();
    ValuePtr acc = v.elements[0];
    for (size_t i = 1; i < v.elements.size(); ++i)
        acc = arith::add(acc, v.elements[i], precision);
    return acc;
}

ValuePtr reduce_mul(const Vector& v, int precision) {
    if (v.elements.empty()) return Value::one();
    ValuePtr acc = v.elements[0];
    for (size_t i = 1; i < v.elements.size(); ++i)
        acc = arith::mul(acc, v.elements[i], precision);
    return acc;
}

ValuePtr reduce_max(const Vector& v, int precision) {
    if (v.elements.empty()) throw std::invalid_argument("max of empty vector");
    ValuePtr best = v.elements[0];
    for (size_t i = 1; i < v.elements.size(); ++i) {
        if (arith::sub(v.elements[i], best, precision)->is_negative()) continue;
        best = v.elements[i];
    }
    return best;
}

ValuePtr reduce_min(const Vector& v, int precision) {
    if (v.elements.empty()) throw std::invalid_argument("min of empty vector");
    ValuePtr best = v.elements[0];
    for (size_t i = 1; i < v.elements.size(); ++i) {
        if (arith::sub(v.elements[i], best, precision)->is_negative())
            best = v.elements[i];
    }
    return best;
}

// --- Linear algebra ---

ValuePtr dot(const Vector& a, const Vector& b, int precision) {
    if (a.elements.size() != b.elements.size())
        throw std::invalid_argument("dot product: vector size mismatch");
    ValuePtr sum = Value::zero();
    for (size_t i = 0; i < a.elements.size(); ++i)
        sum = arith::add(sum, arith::mul(a.elements[i], b.elements[i], precision), precision);
    return sum;
}

ValuePtr cross(const Vector& a, const Vector& b, int precision) {
    if (a.elements.size() != 3 || b.elements.size() != 3)
        throw std::invalid_argument("cross product requires 3-element vectors");
    auto& ae = a.elements;
    auto& be = b.elements;
    // c = [a1*b2 - a2*b1, a2*b0 - a0*b2, a0*b1 - a1*b0]
    std::vector<ValuePtr> result;
    result.push_back(arith::sub(arith::mul(ae[1], be[2], precision),
                                arith::mul(ae[2], be[1], precision), precision));
    result.push_back(arith::sub(arith::mul(ae[2], be[0], precision),
                                arith::mul(ae[0], be[2], precision), precision));
    result.push_back(arith::sub(arith::mul(ae[0], be[1], precision),
                                arith::mul(ae[1], be[0], precision), precision));
    return Value::make_vector(std::move(result));
}

ValuePtr transpose(const Vector& m) {
    if (!is_matrix(m)) throw std::invalid_argument("transpose requires a matrix");
    int rows = matrix_rows(m);
    int cols = matrix_cols(m);
    std::vector<ValuePtr> result_rows;
    result_rows.reserve(cols);
    for (int j = 0; j < cols; ++j) {
        std::vector<ValuePtr> row;
        row.reserve(rows);
        for (int i = 0; i < rows; ++i)
            row.push_back(m.elements[i]->as_vector().elements[j]);
        result_rows.push_back(Value::make_vector(std::move(row)));
    }
    return Value::make_vector(std::move(result_rows));
}

// LU decomposition helper for determinant and inverse
// Returns determinant; modifies aug in-place (Gaussian elimination)
static ValuePtr gauss_det(std::vector<std::vector<ValuePtr>>& mat, int n, int precision) {
    ValuePtr det = Value::one();
    for (int col = 0; col < n; ++col) {
        // Partial pivoting
        int pivot = -1;
        for (int row = col; row < n; ++row) {
            if (!mat[row][col]->is_zero()) { pivot = row; break; }
        }
        if (pivot == -1) return Value::zero(); // singular

        if (pivot != col) {
            std::swap(mat[pivot], mat[col]);
            det = arith::neg(det);
        }

        det = arith::mul(det, mat[col][col], precision);

        // Eliminate below
        for (int row = col + 1; row < n; ++row) {
            if (mat[row][col]->is_zero()) continue;
            auto factor = arith::div(mat[row][col], mat[col][col], precision, FractionMode::Float);
            for (int j = col; j < static_cast<int>(mat[row].size()); ++j) {
                mat[row][j] = arith::sub(mat[row][j],
                                         arith::mul(factor, mat[col][j], precision), precision);
            }
        }
    }
    return det;
}

ValuePtr determinant(const Vector& m, int precision) {
    if (!is_matrix(m)) throw std::invalid_argument("determinant requires a square matrix");
    int n = matrix_rows(m);
    if (n != matrix_cols(m)) throw std::invalid_argument("determinant requires a square matrix");

    // Copy matrix
    std::vector<std::vector<ValuePtr>> mat(n);
    for (int i = 0; i < n; ++i)
        mat[i] = m.elements[i]->as_vector().elements;

    return gauss_det(mat, n, precision);
}

ValuePtr inverse(const Vector& m, int precision) {
    if (!is_matrix(m)) throw std::invalid_argument("inverse requires a square matrix");
    int n = matrix_rows(m);
    if (n != matrix_cols(m)) throw std::invalid_argument("inverse requires a square matrix");

    // Augmented matrix [A | I]
    std::vector<std::vector<ValuePtr>> aug(n);
    for (int i = 0; i < n; ++i) {
        aug[i].reserve(2 * n);
        for (int j = 0; j < n; ++j)
            aug[i].push_back(m.elements[i]->as_vector().elements[j]);
        for (int j = 0; j < n; ++j)
            aug[i].push_back(i == j ? Value::one() : Value::zero());
    }

    // Forward elimination with partial pivoting
    for (int col = 0; col < n; ++col) {
        int pivot = -1;
        for (int row = col; row < n; ++row) {
            if (!aug[row][col]->is_zero()) { pivot = row; break; }
        }
        if (pivot == -1) throw std::domain_error("matrix is singular");
        if (pivot != col) std::swap(aug[pivot], aug[col]);

        auto diag = aug[col][col];
        for (int j = 0; j < 2 * n; ++j)
            aug[col][j] = arith::div(aug[col][j], diag, precision, FractionMode::Float);

        for (int row = 0; row < n; ++row) {
            if (row == col || aug[row][col]->is_zero()) continue;
            auto factor = aug[row][col];
            for (int j = 0; j < 2 * n; ++j)
                aug[row][j] = arith::sub(aug[row][j],
                                         arith::mul(factor, aug[col][j], precision), precision);
        }
    }

    // Extract right half
    std::vector<ValuePtr> rows;
    rows.reserve(n);
    for (int i = 0; i < n; ++i) {
        std::vector<ValuePtr> row(aug[i].begin() + n, aug[i].end());
        rows.push_back(Value::make_vector(std::move(row)));
    }
    return Value::make_vector(std::move(rows));
}

ValuePtr trace(const Vector& m, int precision) {
    if (!is_matrix(m)) throw std::invalid_argument("trace requires a square matrix");
    int n = matrix_rows(m);
    if (n != matrix_cols(m)) throw std::invalid_argument("trace requires a square matrix");
    ValuePtr sum = Value::zero();
    for (int i = 0; i < n; ++i)
        sum = arith::add(sum, m.elements[i]->as_vector().elements[i], precision);
    return sum;
}

// --- Predicates ---

bool is_matrix(const Vector& v) {
    if (v.elements.empty()) return false;
    if (!v.elements[0]->is_vector()) return false;
    int cols = static_cast<int>(v.elements[0]->as_vector().elements.size());
    for (size_t i = 1; i < v.elements.size(); ++i) {
        if (!v.elements[i]->is_vector()) return false;
        if (static_cast<int>(v.elements[i]->as_vector().elements.size()) != cols) return false;
    }
    return true;
}

int matrix_rows(const Vector& v) {
    return static_cast<int>(v.elements.size());
}

int matrix_cols(const Vector& v) {
    if (v.elements.empty() || !v.elements[0]->is_vector()) return 0;
    return static_cast<int>(v.elements[0]->as_vector().elements.size());
}

} // namespace sc::vector_ops
