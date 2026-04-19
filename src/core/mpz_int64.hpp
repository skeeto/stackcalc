#pragma once

// Width-stable integer extraction from mpz_class.
//
// GMP exposes mpz_get_si / mpz_get_ui / mpz_fits_slong_p, all in terms of
// `signed long` and `unsigned long`. On LLP64 platforms (notably Windows
// MSVC and MinGW) `long` is 32 bits, so those calls silently truncate at
// 2^31 / 2^32 and the calculator's "exponent too large" thresholds drift
// between platforms. These helpers always read/check against int64_t /
// uint64_t, regardless of the target's `long` width.
//
// On LP64 (macOS, Linux x86_64) `long` is already 64-bit, so the helpers
// fall through to a single mpz_get_si / mpz_get_ui call. On LLP64 they
// peel off the value 32 bits at a time via mpz_class shift/mask
// arithmetic.

#include <cstdint>
#include <climits>
#include <gmpxx.h>

namespace sc {

// Returns true iff `z` is in [INT64_MIN, INT64_MAX].
inline bool mpz_fits_sint64(const mpz_class& z) {
    if (mpz_fits_slong_p(z.get_mpz_t())) return true;  // LP64 fast path
    static const mpz_class kMin("-9223372036854775808");  // INT64_MIN
    static const mpz_class kMax( "9223372036854775807");  // INT64_MAX
    return z >= kMin && z <= kMax;
}

// Returns true iff `z` is in [0, UINT64_MAX].
inline bool mpz_fits_uint64(const mpz_class& z) {
    if (z < 0) return false;
    if (mpz_fits_ulong_p(z.get_mpz_t())) return true;  // LP64 fast path
    static const mpz_class kMax("18446744073709551615");  // UINT64_MAX
    return z <= kMax;
}

// Extracts `z` as uint64_t. Precondition: mpz_fits_uint64(z).
inline uint64_t mpz_get_uint64(const mpz_class& z) {
    if (mpz_fits_ulong_p(z.get_mpz_t())) {
        return static_cast<uint64_t>(z.get_ui());  // LP64 fast path
    }
    // LLP64 fallback: peel off two 32-bit halves.
    mpz_class hi = z >> 32;
    mpz_class lo = z - (hi << 32);
    return (static_cast<uint64_t>(hi.get_ui()) << 32)
         |  static_cast<uint64_t>(lo.get_ui());
}

// Extracts `z` as int64_t. Precondition: mpz_fits_sint64(z).
inline int64_t mpz_get_sint64(const mpz_class& z) {
    if (mpz_fits_slong_p(z.get_mpz_t())) {
        return static_cast<int64_t>(z.get_si());  // LP64 fast path
    }
    if (z >= 0) {
        // Magnitude fits in [INT64_MAX < UINT64_MAX], safe.
        return static_cast<int64_t>(mpz_get_uint64(z));
    }
    // Negative branch: handle INT64_MIN explicitly since -INT64_MIN
    // overflows int64.
    mpz_class abs_z = -z;
    uint64_t mag = mpz_get_uint64(abs_z);
    if (mag == 0x8000000000000000ULL) return INT64_MIN;
    return -static_cast<int64_t>(mag);
}

} // namespace sc
