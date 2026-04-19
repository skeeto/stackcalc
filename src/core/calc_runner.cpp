#include "calc_runner.hpp"

#include <cassert>
#include <mpfr.h>
#include <utility>

namespace sc {

CalcRunner::CalcRunner() {
    thread_ = std::thread([this]{ worker_loop(); });
}

CalcRunner::~CalcRunner() {
    // If a job is still running when we're destroyed (e.g. app quit
    // during a long calc), request cancel first so the worker doesn't
    // keep burning CPU while we wait to join.
    cancel::request();
    {
        std::lock_guard<std::mutex> lk(mu_);
        stop_ = true;
    }
    cv_.notify_one();
    thread_.join();
    cancel::reset();
}

void CalcRunner::submit(std::function<void()> work, DoneCallback on_done) {
    assert(!busy() && "submit() called while runner is busy");
    busy_.store(true, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lk(mu_);
        work_    = std::move(work);
        on_done_ = std::move(on_done);
    }
    cv_.notify_one();
}

void CalcRunner::request_cancel() {
    // No need to hold mu_; the cancel flag is an atomic consulted by
    // the allocator hook. Safe to call even when !busy() (no-op).
    cancel::request();
}

void CalcRunner::worker_loop() {
    while (true) {
        std::function<void()> work;
        DoneCallback          done;
        {
            std::unique_lock<std::mutex> lk(mu_);
            cv_.wait(lk, [&]{ return stop_ || work_; });
            if (stop_ && !work_) return;
            work = std::move(work_);
            done = std::move(on_done_);
            // std::function's moved-from state is "valid but unspecified"
            // — it's NOT guaranteed to be empty. Without these explicit
            // resets, the next loop iteration's wait predicate (stop_ ||
            // work_) sees a non-empty work_ and the worker re-fires the
            // same job. Clear the slots explicitly so the predicate
            // reflects truth.
            work_    = nullptr;
            on_done_ = nullptr;
        }

        // Fresh cancel state for each job, regardless of how the
        // previous one ended.
        cancel::reset();

        Outcome             outcome = Outcome::Success;
        std::exception_ptr  error;
        {
            cancel::Section s;
            try {
                work();
            } catch (const CancelledException&) {
                outcome = Outcome::Cancelled;
            } catch (...) {
                outcome = Outcome::Error;
                error   = std::current_exception();
            }
            // The controller's own catch(std::exception&) absorbs
            // CancelledException before we see it — its catch sets
            // message_ to "calculation cancelled" and returns normally.
            // Detect that path by consulting the flag directly.
            if (outcome == Outcome::Success && cancel::requested()) {
                outcome = Outcome::Cancelled;
            }
        }

        // After a cancel, MPFR's thread-local caches may be in an
        // inconsistent state (e.g. we threw partway through filling a
        // cached pi). Drop them so the next computation rebuilds from
        // scratch. Harmless if no caches were in use.
        if (outcome == Outcome::Cancelled) {
            mpfr_free_cache();
        }
        cancel::reset();

        busy_.store(false, std::memory_order_release);
        if (done) done(outcome, error);
    }
}

} // namespace sc
