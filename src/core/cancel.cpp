#include "cancel.hpp"

#include <atomic>
#include <cstdlib>
#include <gmp.h>

namespace sc::cancel {

namespace {

std::atomic<bool> g_requested{false};
thread_local int  t_section_depth = 0;

inline void check() {
    if (t_section_depth > 0 &&
        g_requested.load(std::memory_order_relaxed)) {
        throw CancelledException{};
    }
}

// GMP's allocator contract:
//   alloc(size)            -> non-NULL pointer or abort
//   realloc(p, old, new)   -> non-NULL pointer or abort
//   free(p, size)          -> never fails
// Returning NULL is unsafe — GMP doesn't check, dereferences anyway,
// segfault. Default GMP allocator aborts on OOM; we keep that.
void* alloc_hook(size_t size) {
    check();
    void* p = std::malloc(size);
    if (!p) std::abort();
    return p;
}

void* realloc_hook(void* old, size_t /*old_size*/, size_t new_size) {
    check();
    void* p = std::realloc(old, new_size);
    if (!p) std::abort();
    return p;
}

void free_hook(void* p, size_t /*size*/) noexcept {
    // Never throw from free — this is invoked from GMP destructors and
    // mpz_class destructors, which run during stack unwinding.
    std::free(p);
}

// Static initialiser: install our hooks before main() runs, so the
// very first GMP/MPFR allocation in the program goes through our
// allocator. (The thread_local section depth defaults to 0 and the
// atomic flag defaults to false, so this is a no-op functionally
// until someone enters a Section and calls request().)
struct Installer {
    Installer() { install_allocator(); }
};
const Installer g_installer;

} // anon namespace

void install_allocator() {
    mp_set_memory_functions(alloc_hook, realloc_hook, free_hook);
}

void request() { g_requested.store(true,  std::memory_order_relaxed); }
void reset()   { g_requested.store(false, std::memory_order_relaxed); }
bool requested() { return g_requested.load(std::memory_order_relaxed); }

Section::Section()  { ++t_section_depth; }
Section::~Section() { --t_section_depth; }

} // namespace sc::cancel
