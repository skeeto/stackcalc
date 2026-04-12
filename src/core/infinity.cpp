#include "infinity.h"

namespace sc::infinity_ops {

ValuePtr add(const Infinity& a, const ValuePtr& b) {
    if (b->is_infinity()) {
        auto& bi = b->as_infinity();
        if (a.kind == Infinity::Pos && bi.kind == Infinity::Neg) return Value::make_infinity(Infinity::NaN);
        if (a.kind == Infinity::Neg && bi.kind == Infinity::Pos) return Value::make_infinity(Infinity::NaN);
        if (a.kind == bi.kind) return Value::make_infinity(a.kind);
        return Value::make_infinity(Infinity::NaN);
    }
    // a + finite = a (inf + anything finite = inf)
    if (a.kind == Infinity::NaN || a.kind == Infinity::Undirected) return Value::make_infinity(a.kind);
    return Value::make_infinity(a.kind);
}

ValuePtr add(const ValuePtr& a, const Infinity& b) {
    return add(b, a);
}

ValuePtr mul(const Infinity& a, const ValuePtr& b) {
    if (b->is_zero()) return Value::make_infinity(Infinity::NaN);
    if (b->is_infinity()) {
        auto& bi = b->as_infinity();
        if (a.kind == Infinity::NaN || bi.kind == Infinity::NaN) return Value::make_infinity(Infinity::NaN);
        if (a.kind == Infinity::Undirected || bi.kind == Infinity::Undirected) return Value::make_infinity(Infinity::Undirected);
        if (a.kind == bi.kind) return Value::make_infinity(Infinity::Pos);
        return Value::make_infinity(Infinity::Neg);
    }
    if (a.kind == Infinity::NaN) return Value::make_infinity(Infinity::NaN);
    if (a.kind == Infinity::Undirected) return Value::make_infinity(Infinity::Undirected);
    if (b->is_negative()) {
        return Value::make_infinity(a.kind == Infinity::Pos ? Infinity::Neg : Infinity::Pos);
    }
    return Value::make_infinity(a.kind);
}

ValuePtr neg(const Infinity& a) {
    switch (a.kind) {
        case Infinity::Pos: return Value::make_infinity(Infinity::Neg);
        case Infinity::Neg: return Value::make_infinity(Infinity::Pos);
        default: return Value::make_infinity(a.kind);
    }
}

} // namespace sc::infinity_ops
