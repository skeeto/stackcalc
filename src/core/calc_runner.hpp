#pragma once

// Single-threaded worker for running calculations off the UI thread.
//
// Why: GMP/MPFR have no internal cancellation hook, so we run their
// calls on a dedicated worker thread and use the cancel.hpp allocator
// hook to throw out of any in-flight allocation when the user asks to
// stop. Keeping the work off the UI thread also means the UI keeps
// pumping events (paint, resize, the cancel keystroke) regardless of
// how long the calc takes.
//
// Lifecycle:
//   1. Construct CalcRunner — spawns a worker thread that idles on a
//      condition variable.
//   2. submit(work, on_done) — UI thread enqueues a single job. Asserts
//      not busy. Worker wakes up, enters cancel::Section, runs work().
//   3. Worker calls on_done(outcome, error) when finished. on_done runs
//      ON THE WORKER THREAD — the caller is responsible for marshalling
//      back to the UI thread (e.g. via wxCallAfter).
//   4. request_cancel() — sets the global cancel flag. The worker's
//      next GMP allocation throws CancelledException; outcome is
//      reported as Cancelled. After cancel, the worker clears MPFR's
//      thread-local caches in case any partial constant-fill left them
//      inconsistent.
//   5. Destructor — requests cancel, signals stop, joins the thread.
//
// CalcRunner does not depend on the Controller; it just runs whatever
// std::function the caller hands it. This makes it testable without
// the GUI and reusable for any background-cancellable work.

#include "cancel.hpp"

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>

namespace sc {

class CalcRunner {
public:
    enum class Outcome { Success, Cancelled, Error };

    // Invoked from the worker thread when the submitted job finishes.
    // For Outcome::Error, error is non-null and holds the exception.
    using DoneCallback = std::function<void(Outcome, std::exception_ptr)>;

    CalcRunner();
    ~CalcRunner();

    CalcRunner(const CalcRunner&) = delete;
    CalcRunner& operator=(const CalcRunner&) = delete;

    // Enqueue a single job. Caller must check busy() == false first;
    // submitting while busy aborts (one-job-at-a-time invariant).
    void submit(std::function<void()> work, DoneCallback on_done);

    // True iff the worker is currently running (or about to run) a job.
    // Cleared after on_done returns.
    bool busy() const { return busy_.load(std::memory_order_acquire); }

    // Ask the in-flight job to stop. The worker reports Cancelled on
    // its next allocation. Safe to call from any thread; no-op if not
    // busy.
    void request_cancel();

private:
    void worker_loop();

    std::thread             thread_;
    std::mutex              mu_;
    std::condition_variable cv_;
    std::function<void()>   work_;
    DoneCallback            on_done_;
    std::atomic<bool>       busy_{false};
    bool                    stop_ = false;  // protected by mu_
};

} // namespace sc
