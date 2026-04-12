#include "constants.h"
#include "mpfr_bridge.h"
#include <mutex>

namespace sc::constants {

struct CachedConstant {
    std::mutex mu;
    int cached_prec = 0;
    ValuePtr value;
};

static CachedConstant cache_pi, cache_e, cache_gamma, cache_phi;

static ValuePtr get_or_compute(CachedConstant& cache, int precision,
                                void (*compute)(mpfr_t, mpfr_prec_t)) {
    std::lock_guard lock(cache.mu);
    if (cache.cached_prec >= precision && cache.value) {
        // Re-round to requested precision if cached at higher
        if (cache.cached_prec == precision) return cache.value;
        // Recompute at exact precision for consistency
    }
    mpfr_prec_t bits = mpfr_bridge::decimal_to_binary_prec(precision);
    mpfr_t result;
    mpfr_init2(result, bits);
    compute(result, bits);
    cache.value = mpfr_bridge::from_mpfr(result, precision);
    cache.cached_prec = precision;
    mpfr_clear(result);
    return cache.value;
}

static void compute_pi(mpfr_t out, mpfr_prec_t) {
    mpfr_const_pi(out, MPFR_RNDN);
}

static void compute_e(mpfr_t out, mpfr_prec_t) {
    // e = exp(1)
    mpfr_t one;
    mpfr_init2(one, mpfr_get_prec(out));
    mpfr_set_ui(one, 1, MPFR_RNDN);
    mpfr_exp(out, one, MPFR_RNDN);
    mpfr_clear(one);
}

static void compute_gamma(mpfr_t out, mpfr_prec_t) {
    mpfr_const_euler(out, MPFR_RNDN);
}

static void compute_phi(mpfr_t out, mpfr_prec_t) {
    // phi = (1 + sqrt(5)) / 2
    mpfr_t five, one, two;
    mpfr_prec_t prec = mpfr_get_prec(out);
    mpfr_init2(five, prec);
    mpfr_init2(one, prec);
    mpfr_init2(two, prec);
    mpfr_set_ui(five, 5, MPFR_RNDN);
    mpfr_sqrt(out, five, MPFR_RNDN);
    mpfr_set_ui(one, 1, MPFR_RNDN);
    mpfr_add(out, out, one, MPFR_RNDN);
    mpfr_set_ui(two, 2, MPFR_RNDN);
    mpfr_div(out, out, two, MPFR_RNDN);
    mpfr_clear(five);
    mpfr_clear(one);
    mpfr_clear(two);
}

ValuePtr pi(int precision)    { return get_or_compute(cache_pi, precision, compute_pi); }
ValuePtr e(int precision)     { return get_or_compute(cache_e, precision, compute_e); }
ValuePtr gamma(int precision) { return get_or_compute(cache_gamma, precision, compute_gamma); }
ValuePtr phi(int precision)   { return get_or_compute(cache_phi, precision, compute_phi); }

} // namespace sc::constants
