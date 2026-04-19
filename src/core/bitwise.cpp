#include "bitwise.hpp"
#include <stdexcept>

namespace sc::bitwise {

static mpz_class get_int(const ValuePtr& v) {
    if (!v->is_integer()) throw std::invalid_argument("bitwise operation requires integer");
    return v->as_integer().v;
}

static mpz_class mask_bits(int word_size) {
    // (1 << word_size) - 1
    mpz_class m;
    mpz_ui_pow_ui(m.get_mpz_t(), 2, static_cast<unsigned long>(word_size));
    return m - 1;
}

ValuePtr band(const ValuePtr& a, const ValuePtr& b) {
    mpz_class result = get_int(a) & get_int(b);
    return Value::make_integer(std::move(result));
}

ValuePtr bor(const ValuePtr& a, const ValuePtr& b) {
    mpz_class result = get_int(a) | get_int(b);
    return Value::make_integer(std::move(result));
}

ValuePtr bxor(const ValuePtr& a, const ValuePtr& b) {
    mpz_class result = get_int(a) ^ get_int(b);
    return Value::make_integer(std::move(result));
}

ValuePtr bnot(const ValuePtr& a, int word_size) {
    if (word_size <= 0) throw std::invalid_argument("NOT requires a positive word size");
    mpz_class v = get_int(a);
    mpz_class m = mask_bits(word_size);
    // NOT = XOR with all-ones mask
    mpz_class result = v ^ m;
    return Value::make_integer(std::move(result));
}

ValuePtr lshift(const ValuePtr& a, int bits, int word_size) {
    mpz_class v = get_int(a);
    if (bits < 0) return rshift(a, -bits, word_size);
    mpz_class result;
    mpz_mul_2exp(result.get_mpz_t(), v.get_mpz_t(), static_cast<mp_bitcnt_t>(bits));
    if (word_size > 0) {
        result &= mask_bits(word_size);
    }
    return Value::make_integer(std::move(result));
}

ValuePtr rshift(const ValuePtr& a, int bits, int word_size) {
    mpz_class v = get_int(a);
    if (bits < 0) return lshift(a, -bits, word_size);
    mpz_class result;
    mpz_fdiv_q_2exp(result.get_mpz_t(), v.get_mpz_t(), static_cast<mp_bitcnt_t>(bits));
    if (word_size > 0) {
        result &= mask_bits(word_size);
    }
    return Value::make_integer(std::move(result));
}

ValuePtr lrot(const ValuePtr& a, int bits, int word_size) {
    if (word_size <= 0) throw std::invalid_argument("rotate requires a positive word size");
    mpz_class v = get_int(a);
    mpz_class m = mask_bits(word_size);
    v &= m;
    int shift = ((bits % word_size) + word_size) % word_size;
    mpz_class high, low;
    mpz_mul_2exp(high.get_mpz_t(), v.get_mpz_t(), static_cast<mp_bitcnt_t>(shift));
    mpz_fdiv_q_2exp(low.get_mpz_t(), v.get_mpz_t(), static_cast<mp_bitcnt_t>(word_size - shift));
    mpz_class result = (high | low) & m;
    return Value::make_integer(std::move(result));
}

ValuePtr rrot(const ValuePtr& a, int bits, int word_size) {
    return lrot(a, word_size - (bits % word_size), word_size);
}

ValuePtr to_twos_complement(const ValuePtr& a, int word_size) {
    if (word_size <= 0) throw std::invalid_argument("two's complement requires a positive word size");
    mpz_class v = get_int(a);
    mpz_class m = mask_bits(word_size);
    if (v < 0) {
        // Two's complement: 2^word_size + v
        mpz_class pow2;
        mpz_ui_pow_ui(pow2.get_mpz_t(), 2, static_cast<unsigned long>(word_size));
        v = pow2 + v;
    }
    v &= m;
    return Value::make_integer(std::move(v));
}

ValuePtr from_twos_complement(const ValuePtr& a, int word_size) {
    if (word_size <= 0) throw std::invalid_argument("two's complement requires a positive word size");
    mpz_class v = get_int(a);
    mpz_class m = mask_bits(word_size);
    v &= m;
    // Check sign bit
    mpz_class sign_bit;
    mpz_ui_pow_ui(sign_bit.get_mpz_t(), 2, static_cast<unsigned long>(word_size - 1));
    if (v >= sign_bit) {
        // Negative: v - 2^word_size
        mpz_class pow2;
        mpz_ui_pow_ui(pow2.get_mpz_t(), 2, static_cast<unsigned long>(word_size));
        v = v - pow2;
    }
    return Value::make_integer(std::move(v));
}

ValuePtr clip(const ValuePtr& a, int word_size) {
    if (word_size <= 0) return a;
    mpz_class v = get_int(a);
    v &= mask_bits(word_size);
    return Value::make_integer(std::move(v));
}

} // namespace sc::bitwise
