#include <gtest/gtest.h>
#include "vector.hpp"
#include "arithmetic.hpp"
#include <cmath>

using namespace sc;

// --- Construction ---

TEST(VectorTest, Build) {
    auto v = vector_ops::build({Value::make_integer(1), Value::make_integer(2), Value::make_integer(3)});
    ASSERT_TRUE(v->is_vector());
    EXPECT_EQ(v->as_vector().elements.size(), 3u);
}

TEST(VectorTest, Cvec) {
    auto v = vector_ops::cvec(Value::make_integer(7), 4);
    ASSERT_TRUE(v->is_vector());
    auto& elems = v->as_vector().elements;
    EXPECT_EQ(elems.size(), 4u);
    for (auto& e : elems) EXPECT_EQ(e->as_integer().v, 7);
}

TEST(VectorTest, Index) {
    auto v = vector_ops::index(5);
    ASSERT_TRUE(v->is_vector());
    auto& elems = v->as_vector().elements;
    EXPECT_EQ(elems.size(), 5u);
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(elems[i]->as_integer().v, i + 1);
}

TEST(VectorTest, Identity) {
    auto m = vector_ops::identity(3);
    ASSERT_TRUE(m->is_vector());
    ASSERT_TRUE(vector_ops::is_matrix(m->as_vector()));
    // Check diagonal
    for (int i = 0; i < 3; ++i) {
        auto& row = m->as_vector().elements[i]->as_vector().elements;
        for (int j = 0; j < 3; ++j) {
            if (i == j) EXPECT_EQ(row[j]->as_integer().v, 1);
            else EXPECT_TRUE(row[j]->is_zero());
        }
    }
}

TEST(VectorTest, Diagonal) {
    auto v = vector_ops::build({Value::make_integer(2), Value::make_integer(3), Value::make_integer(5)});
    auto m = vector_ops::diagonal(v);
    ASSERT_TRUE(vector_ops::is_matrix(m->as_vector()));
    EXPECT_EQ(m->as_vector().elements[0]->as_vector().elements[0]->as_integer().v, 2);
    EXPECT_EQ(m->as_vector().elements[1]->as_vector().elements[1]->as_integer().v, 3);
    EXPECT_EQ(m->as_vector().elements[2]->as_vector().elements[2]->as_integer().v, 5);
}

// --- Deconstruction ---

TEST(VectorTest, HeadTail) {
    auto v = vector_ops::build({Value::make_integer(10), Value::make_integer(20), Value::make_integer(30)});
    auto h = vector_ops::head(v->as_vector());
    EXPECT_EQ(h->as_integer().v, 10);
    auto t = vector_ops::tail(v->as_vector());
    EXPECT_EQ(t->as_vector().elements.size(), 2u);
    EXPECT_EQ(t->as_vector().elements[0]->as_integer().v, 20);
}

TEST(VectorTest, ConsRcons) {
    auto v = vector_ops::build({Value::make_integer(2), Value::make_integer(3)});
    auto c = vector_ops::cons(Value::make_integer(1), v->as_vector());
    EXPECT_EQ(c->as_vector().elements.size(), 3u);
    EXPECT_EQ(c->as_vector().elements[0]->as_integer().v, 1);

    auto r = vector_ops::rcons(v->as_vector(), Value::make_integer(4));
    EXPECT_EQ(r->as_vector().elements.size(), 3u);
    EXPECT_EQ(r->as_vector().elements[2]->as_integer().v, 4);
}

TEST(VectorTest, Length) {
    auto v = vector_ops::build({Value::make_integer(1), Value::make_integer(2)});
    EXPECT_EQ(vector_ops::length(v->as_vector()), 2);
}

TEST(VectorTest, Extract) {
    auto v = vector_ops::build({Value::make_integer(10), Value::make_integer(20), Value::make_integer(30)});
    EXPECT_EQ(vector_ops::extract(v->as_vector(), 2)->as_integer().v, 20);
}

// --- Manipulation ---

TEST(VectorTest, Reverse) {
    auto v = vector_ops::build({Value::make_integer(1), Value::make_integer(2), Value::make_integer(3)});
    auto r = vector_ops::reverse(v->as_vector());
    EXPECT_EQ(r->as_vector().elements[0]->as_integer().v, 3);
    EXPECT_EQ(r->as_vector().elements[2]->as_integer().v, 1);
}

TEST(VectorTest, Subvec) {
    auto v = vector_ops::build({Value::make_integer(10), Value::make_integer(20),
                                Value::make_integer(30), Value::make_integer(40)});
    auto s = vector_ops::subvec(v->as_vector(), 2, 4);
    EXPECT_EQ(s->as_vector().elements.size(), 2u);
    EXPECT_EQ(s->as_vector().elements[0]->as_integer().v, 20);
    EXPECT_EQ(s->as_vector().elements[1]->as_integer().v, 30);
}

TEST(VectorTest, Concat) {
    auto a = vector_ops::build({Value::make_integer(1), Value::make_integer(2)});
    auto b = vector_ops::build({Value::make_integer(3), Value::make_integer(4)});
    auto c = vector_ops::concat(a->as_vector(), b->as_vector());
    EXPECT_EQ(c->as_vector().elements.size(), 4u);
}

TEST(VectorTest, Sort) {
    auto v = vector_ops::build({Value::make_integer(3), Value::make_integer(1), Value::make_integer(2)});
    auto s = vector_ops::sort(v->as_vector(), 12);
    EXPECT_EQ(s->as_vector().elements[0]->as_integer().v, 1);
    EXPECT_EQ(s->as_vector().elements[1]->as_integer().v, 2);
    EXPECT_EQ(s->as_vector().elements[2]->as_integer().v, 3);
}

TEST(VectorTest, RSort) {
    auto v = vector_ops::build({Value::make_integer(3), Value::make_integer(1), Value::make_integer(2)});
    auto s = vector_ops::rsort(v->as_vector(), 12);
    EXPECT_EQ(s->as_vector().elements[0]->as_integer().v, 3);
    EXPECT_EQ(s->as_vector().elements[1]->as_integer().v, 2);
    EXPECT_EQ(s->as_vector().elements[2]->as_integer().v, 1);
}

// --- Element-wise arithmetic ---

TEST(VectorTest, AddVectors) {
    auto a = vector_ops::build({Value::make_integer(1), Value::make_integer(2)});
    auto b = vector_ops::build({Value::make_integer(3), Value::make_integer(4)});
    auto r = arith::add(a, b, 12);
    ASSERT_TRUE(r->is_vector());
    EXPECT_EQ(r->as_vector().elements[0]->as_integer().v, 4);
    EXPECT_EQ(r->as_vector().elements[1]->as_integer().v, 6);
}

TEST(VectorTest, ScalarMulVector) {
    auto s = Value::make_integer(3);
    auto v = vector_ops::build({Value::make_integer(1), Value::make_integer(2), Value::make_integer(3)});
    auto r = arith::mul(s, v, 12);
    ASSERT_TRUE(r->is_vector());
    EXPECT_EQ(r->as_vector().elements[0]->as_integer().v, 3);
    EXPECT_EQ(r->as_vector().elements[1]->as_integer().v, 6);
    EXPECT_EQ(r->as_vector().elements[2]->as_integer().v, 9);
}

TEST(VectorTest, NegVector) {
    auto v = vector_ops::build({Value::make_integer(1), Value::make_integer(-2)});
    auto r = arith::neg(v);
    ASSERT_TRUE(r->is_vector());
    EXPECT_EQ(r->as_vector().elements[0]->as_integer().v, -1);
    EXPECT_EQ(r->as_vector().elements[1]->as_integer().v, 2);
}

// --- Reduce ---

TEST(VectorTest, ReduceAdd) {
    auto v = vector_ops::build({Value::make_integer(1), Value::make_integer(2), Value::make_integer(3)});
    auto r = vector_ops::reduce_add(v->as_vector(), 12);
    EXPECT_EQ(r->as_integer().v, 6);
}

TEST(VectorTest, ReduceMul) {
    auto v = vector_ops::build({Value::make_integer(2), Value::make_integer(3), Value::make_integer(4)});
    auto r = vector_ops::reduce_mul(v->as_vector(), 12);
    EXPECT_EQ(r->as_integer().v, 24);
}

TEST(VectorTest, ReduceMax) {
    auto v = vector_ops::build({Value::make_integer(5), Value::make_integer(2), Value::make_integer(8)});
    auto r = vector_ops::reduce_max(v->as_vector(), 12);
    EXPECT_EQ(r->as_integer().v, 8);
}

TEST(VectorTest, ReduceMin) {
    auto v = vector_ops::build({Value::make_integer(5), Value::make_integer(2), Value::make_integer(8)});
    auto r = vector_ops::reduce_min(v->as_vector(), 12);
    EXPECT_EQ(r->as_integer().v, 2);
}

// --- Linear algebra ---

TEST(VectorTest, DotProduct) {
    auto a = vector_ops::build({Value::make_integer(1), Value::make_integer(2), Value::make_integer(3)});
    auto b = vector_ops::build({Value::make_integer(4), Value::make_integer(5), Value::make_integer(6)});
    // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    auto r = vector_ops::dot(a->as_vector(), b->as_vector(), 12);
    EXPECT_EQ(r->as_integer().v, 32);
}

TEST(VectorTest, CrossProduct) {
    auto a = vector_ops::build({Value::make_integer(1), Value::make_integer(0), Value::make_integer(0)});
    auto b = vector_ops::build({Value::make_integer(0), Value::make_integer(1), Value::make_integer(0)});
    auto r = vector_ops::cross(a->as_vector(), b->as_vector(), 12);
    ASSERT_TRUE(r->is_vector());
    auto& e = r->as_vector().elements;
    EXPECT_EQ(e[0]->as_integer().v, 0);
    EXPECT_EQ(e[1]->as_integer().v, 0);
    EXPECT_EQ(e[2]->as_integer().v, 1);
}

TEST(VectorTest, Transpose) {
    // [[1,2],[3,4]] -> [[1,3],[2,4]]
    auto m = vector_ops::build({
        vector_ops::build({Value::make_integer(1), Value::make_integer(2)}),
        vector_ops::build({Value::make_integer(3), Value::make_integer(4)})
    });
    auto t = vector_ops::transpose(m->as_vector());
    ASSERT_TRUE(t->is_vector());
    EXPECT_EQ(t->as_vector().elements[0]->as_vector().elements[0]->as_integer().v, 1);
    EXPECT_EQ(t->as_vector().elements[0]->as_vector().elements[1]->as_integer().v, 3);
    EXPECT_EQ(t->as_vector().elements[1]->as_vector().elements[0]->as_integer().v, 2);
    EXPECT_EQ(t->as_vector().elements[1]->as_vector().elements[1]->as_integer().v, 4);
}

TEST(VectorTest, Determinant2x2) {
    // [[1,2],[3,4]], det = 1*4 - 2*3 = -2
    auto m = vector_ops::build({
        vector_ops::build({Value::make_integer(1), Value::make_integer(2)}),
        vector_ops::build({Value::make_integer(3), Value::make_integer(4)})
    });
    auto d = vector_ops::determinant(m->as_vector(), 12);
    // Result may be float due to division during Gaussian elim
    if (d->is_integer()) {
        EXPECT_EQ(d->as_integer().v, -2);
    } else {
        ASSERT_TRUE(d->is_float());
        auto& f = d->as_float();
        double val = f.mantissa.get_d() * std::pow(10.0, f.exponent);
        EXPECT_NEAR(val, -2.0, 1e-8);
    }
}

TEST(VectorTest, Determinant3x3) {
    // [[1,2,3],[4,5,6],[7,8,0]], det = 1*(0-48) - 2*(0-42) + 3*(32-35)
    // = -48 + 84 - 9 = 27
    auto m = vector_ops::build({
        vector_ops::build({Value::make_integer(1), Value::make_integer(2), Value::make_integer(3)}),
        vector_ops::build({Value::make_integer(4), Value::make_integer(5), Value::make_integer(6)}),
        vector_ops::build({Value::make_integer(7), Value::make_integer(8), Value::make_integer(0)})
    });
    auto d = vector_ops::determinant(m->as_vector(), 12);
    if (d->is_integer()) {
        EXPECT_EQ(d->as_integer().v, 27);
    } else {
        ASSERT_TRUE(d->is_float());
        double val = d->as_float().mantissa.get_d() * std::pow(10.0, d->as_float().exponent);
        EXPECT_NEAR(val, 27.0, 1e-8);
    }
}

TEST(VectorTest, Trace) {
    auto m = vector_ops::build({
        vector_ops::build({Value::make_integer(1), Value::make_integer(2)}),
        vector_ops::build({Value::make_integer(3), Value::make_integer(4)})
    });
    auto t = vector_ops::trace(m->as_vector(), 12);
    EXPECT_EQ(t->as_integer().v, 5);
}

TEST(VectorTest, MatrixMultiply) {
    // [[1,2],[3,4]] * [[5,6],[7,8]] = [[19,22],[43,50]]
    auto a = vector_ops::build({
        vector_ops::build({Value::make_integer(1), Value::make_integer(2)}),
        vector_ops::build({Value::make_integer(3), Value::make_integer(4)})
    });
    auto b = vector_ops::build({
        vector_ops::build({Value::make_integer(5), Value::make_integer(6)}),
        vector_ops::build({Value::make_integer(7), Value::make_integer(8)})
    });
    auto r = arith::mul(a, b, 12);
    ASSERT_TRUE(r->is_vector());
    EXPECT_EQ(r->as_vector().elements[0]->as_vector().elements[0]->as_integer().v, 19);
    EXPECT_EQ(r->as_vector().elements[0]->as_vector().elements[1]->as_integer().v, 22);
    EXPECT_EQ(r->as_vector().elements[1]->as_vector().elements[0]->as_integer().v, 43);
    EXPECT_EQ(r->as_vector().elements[1]->as_vector().elements[1]->as_integer().v, 50);
}

TEST(VectorTest, Inverse2x2) {
    // [[1,2],[3,4]]^-1 = [[-2,1],[1.5,-0.5]]
    auto m = vector_ops::build({
        vector_ops::build({Value::make_integer(1), Value::make_integer(2)}),
        vector_ops::build({Value::make_integer(3), Value::make_integer(4)})
    });
    auto inv = vector_ops::inverse(m->as_vector(), 12);
    ASSERT_TRUE(inv->is_vector());
    // Verify M * M^-1 = I
    auto product = arith::mul(m, inv, 12);
    ASSERT_TRUE(product->is_vector());
    auto& rows = product->as_vector().elements;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            auto& elem = rows[i]->as_vector().elements[j];
            double val;
            if (elem->is_integer()) val = elem->as_integer().v.get_d();
            else val = elem->as_float().mantissa.get_d() * std::pow(10.0, elem->as_float().exponent);
            if (i == j) EXPECT_NEAR(val, 1.0, 1e-8);
            else EXPECT_NEAR(val, 0.0, 1e-8);
        }
    }
}

// --- Predicates ---

TEST(VectorTest, IsMatrix) {
    auto m = vector_ops::build({
        vector_ops::build({Value::make_integer(1), Value::make_integer(2)}),
        vector_ops::build({Value::make_integer(3), Value::make_integer(4)})
    });
    EXPECT_TRUE(vector_ops::is_matrix(m->as_vector()));
    EXPECT_EQ(vector_ops::matrix_rows(m->as_vector()), 2);
    EXPECT_EQ(vector_ops::matrix_cols(m->as_vector()), 2);
}

TEST(VectorTest, NotMatrix) {
    auto v = vector_ops::build({Value::make_integer(1), Value::make_integer(2)});
    EXPECT_FALSE(vector_ops::is_matrix(v->as_vector()));
}

TEST(VectorTest, SizeMismatchThrows) {
    auto a = vector_ops::build({Value::make_integer(1), Value::make_integer(2)});
    auto b = vector_ops::build({Value::make_integer(3)});
    EXPECT_THROW(arith::add(a, b, 12), std::invalid_argument);
}
