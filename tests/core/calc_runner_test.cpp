#include "calc_runner.hpp"
#include "cancel.hpp"
#include "integer.hpp"
#include "constants.hpp"

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <future>
#include <thread>

using namespace sc;
using namespace std::chrono_literals;

namespace {
// Block the calling thread until `pred` is true or `timeout` elapses.
// Returns true if pred succeeded.
template <class P>
bool wait_for(P pred, std::chrono::milliseconds timeout = 2s) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (pred()) return true;
        std::this_thread::sleep_for(1ms);
    }
    return false;
}
} // namespace

class CalcRunnerTest : public ::testing::Test {
protected:
    void SetUp() override { cancel::install_allocator(); cancel::reset(); }
    void TearDown() override { cancel::reset(); }
};

TEST_F(CalcRunnerTest, ConstructDestruct) {
    // Spawning + joining the worker with no submissions must work.
    CalcRunner r;
    EXPECT_FALSE(r.busy());
}

TEST_F(CalcRunnerTest, RunsSimpleWork) {
    CalcRunner r;
    std::promise<CalcRunner::Outcome> p;
    auto fut = p.get_future();

    bool ran = false;
    r.submit(
        [&ran]{ ran = true; },
        [&p](auto outcome, auto){ p.set_value(outcome); });

    EXPECT_EQ(fut.get(), CalcRunner::Outcome::Success);
    EXPECT_TRUE(ran);
    EXPECT_TRUE(wait_for([&]{ return !r.busy(); }));
}

TEST_F(CalcRunnerTest, BusyFlagReflectsState) {
    CalcRunner r;
    EXPECT_FALSE(r.busy());

    std::promise<void> released;
    auto block = released.get_future();
    std::promise<void> done_p;
    auto done_fut = done_p.get_future();

    r.submit(
        [&block]{ block.wait(); },
        [&done_p](auto, auto){ done_p.set_value(); });

    EXPECT_TRUE(wait_for([&]{ return r.busy(); }));
    released.set_value();
    done_fut.wait();
    EXPECT_TRUE(wait_for([&]{ return !r.busy(); }));
}

TEST_F(CalcRunnerTest, CancelInterruptsLongComputation) {
    CalcRunner r;
    std::promise<CalcRunner::Outcome> p;
    auto fut = p.get_future();

    // Submit a workload guaranteed to allocate on every iteration —
    // multiplying mpz_class by itself produces fresh limbs each time
    // (constants::pi would cache after the first call). The cancel
    // hook will trip on the first allocation after request_cancel.
    std::atomic<bool> started{false};
    r.submit(
        [&started]{
            mpz_class x("12345678901234567890");
            started.store(true);
            for (;;) {
                mpz_class y = x * x;
                (void)y;
            }
        },
        [&p](auto outcome, auto){ p.set_value(outcome); });

    EXPECT_TRUE(wait_for([&]{ return started.load(); }));
    r.request_cancel();
    EXPECT_EQ(fut.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(fut.get(), CalcRunner::Outcome::Cancelled);
}

TEST_F(CalcRunnerTest, CancelOfFastJobNoOp) {
    // request_cancel() called when nothing is running just sets the
    // flag. The next job clears it before running, so it completes
    // normally.
    CalcRunner r;
    r.request_cancel();

    std::promise<CalcRunner::Outcome> p;
    auto fut = p.get_future();
    r.submit(
        []{ /* noop */ },
        [&p](auto outcome, auto){ p.set_value(outcome); });

    EXPECT_EQ(fut.get(), CalcRunner::Outcome::Success);
}

TEST_F(CalcRunnerTest, ErrorOutcomeCarriesException) {
    CalcRunner r;
    std::promise<std::pair<CalcRunner::Outcome, std::exception_ptr>> p;
    auto fut = p.get_future();
    r.submit(
        []{ throw std::runtime_error("boom"); },
        [&p](auto outcome, auto err){
            p.set_value({outcome, err});
        });

    auto [outcome, err] = fut.get();
    EXPECT_EQ(outcome, CalcRunner::Outcome::Error);
    ASSERT_TRUE(static_cast<bool>(err));
    try { std::rethrow_exception(err); }
    catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "boom");
    } catch (...) { FAIL() << "wrong exception type"; }
}

TEST_F(CalcRunnerTest, SequentialJobsShareNoState) {
    CalcRunner r;
    for (int i = 0; i < 5; ++i) {
        std::promise<CalcRunner::Outcome> p;
        auto fut = p.get_future();
        r.submit(
            []{ auto v = integer::pow(Integer{2}, 100ull); (void)v; },
            [&p](auto outcome, auto){ p.set_value(outcome); });
        EXPECT_EQ(fut.get(), CalcRunner::Outcome::Success);
    }
}

TEST_F(CalcRunnerTest, DestructorCancelsInFlight) {
    // Construct, submit a long job, then destruct without explicitly
    // cancelling. Dtor should request cancel, drain, and join. If it
    // doesn't, this test hangs.
    std::promise<CalcRunner::Outcome> p;
    auto fut = p.get_future();
    {
        CalcRunner r;
        std::atomic<bool> started{false};
        r.submit(
            [&started]{
                mpz_class x("99999999999999999999");
                started.store(true);
                for (;;) {
                    mpz_class y = x * x;
                    (void)y;
                }
            },
            [&p](auto outcome, auto){ p.set_value(outcome); });
        EXPECT_TRUE(wait_for([&]{ return started.load(); }));
    } // dtor here
    EXPECT_EQ(fut.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(fut.get(), CalcRunner::Outcome::Cancelled);
}
