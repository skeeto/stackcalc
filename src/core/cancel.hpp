#pragma once

// Cooperative cancellation for arbitrary-precision computations.
//
// GMP and MPFR are not interruptible by design — there's no callback
// to ask "should I stop?" inside a long mpz_pow_ui. The standard trick
// (used by SageMath and others) is to install a custom GMP allocator
// via mp_set_memory_functions that throws a C++ exception when a
// cancel flag is set. The exception unwinds through GMP's C frames
// (the GMP/MPFR libraries are built with -fexceptions / /EHa for
// this; see cmake/FetchGMP.cmake) and is caught at the calculator's
// command boundary.
//
// Cancellation is opt-in per call site via cancel::Section. Outside an
// active Section, the allocator never throws — so incidental GMP use
// in the UI thread (display formatting, etc.) keeps working even
// while the cancel flag is set.
//
// Lifecycle:
//   1. install_allocator() runs once at static-init time.
//   2. Worker thread enters a Section, runs the calculation.
//   3. UI thread calls request() to ask the worker to stop.
//   4. Worker's next GMP allocation throws CancelledException.
//   5. Worker catches it at the command boundary, restores stack
//      state, calls reset() to clear the flag.

#include <atomic>
#include <exception>

namespace sc {

class CancelledException : public std::exception {
public:
    const char* what() const noexcept override { return "calculation cancelled"; }
};

namespace cancel {

// Set the cancel flag. The next GMP allocation made on a thread
// inside an active Section will throw. Safe to call from any thread.
void request();

// Clear the cancel flag. Call after a Section has caught
// CancelledException and finished cleaning up, so subsequent
// computations don't immediately re-throw.
void reset();

// True if request() has been called and reset() hasn't.
bool requested();

// RAII: marks the current thread as eligible for cancel-throws in the
// allocator. Reentrant via depth counter (nested Sections are OK).
// Without an active Section the allocator never throws even when the
// flag is set.
class Section {
public:
    Section();
    ~Section();
    Section(const Section&) = delete;
    Section& operator=(const Section&) = delete;
};

// Install our cancellation-aware allocator into GMP via
// mp_set_memory_functions. Idempotent. Called automatically at
// static-init time; exposed for tests that want to be explicit.
void install_allocator();

} // namespace cancel
} // namespace sc
