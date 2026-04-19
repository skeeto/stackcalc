#pragma once

// Pre-flight result-size limit for arbitrary-precision operations.
//
// GMP and MPFR will happily attempt to compute results that don't fit
// in available memory — `2^(2^32)`, `(10^6)!`, etc. all start
// allocating gigabytes before they fail (or before the OS kills the
// process). We get ahead of the explosion by estimating the result
// size from the inputs at the entry point of every primitive that can
// produce arbitrary-precision growth (pow, factorial, combinatorics)
// and refusing if the estimate exceeds a fixed bit-count cap.
//
// The cap is generous: 10 million bits ≈ 1.25 MB of raw output, or
// roughly 3 million decimal digits. That comfortably exceeds any
// plausible interactive result while staying well below the memory
// cliff. Hardcoded for now; promote to a CalcState field if/when we
// want users to tune it.
//
// Estimates are upper bounds — better to refuse a borderline-feasible
// computation than to OOM the app. Saturating arithmetic protects
// against uint64 overflow in the multiplication.

#include <cstdint>
#include <stdexcept>
#include <string>
#include <gmpxx.h>

namespace sc {

inline constexpr std::uint64_t kMaxResultBits = 10'000'000;

[[noreturn]] inline void throw_result_too_large(const char* op,
                                                 std::uint64_t est_bits) {
    throw std::overflow_error(
        std::string("result too large for ") + op
        + " (estimated " + std::to_string(est_bits)
        + " bits, limit " + std::to_string(kMaxResultBits) + ")");
}

inline void check_result_bits(std::uint64_t est_bits, const char* op) {
    if (est_bits > kMaxResultBits) throw_result_too_large(op, est_bits);
}

// Bit length of |z|. mpz_sizeinbase ignores sign; returns 1 for z == 0,
// which we patch to 0 since it's nicer for size arithmetic.
inline std::uint64_t mpz_bits(const mpz_class& z) {
    if (z == 0) return 0;
    return static_cast<std::uint64_t>(mpz_sizeinbase(z.get_mpz_t(), 2));
}

// Saturating a*b for the size estimators — never wraps, returns
// UINT64_MAX on overflow which the cap check then rejects.
inline std::uint64_t sat_mul_u64(std::uint64_t a, std::uint64_t b) {
    if (a == 0 || b == 0) return 0;
    if (a > UINT64_MAX / b) return UINT64_MAX;
    return a * b;
}

} // namespace sc
