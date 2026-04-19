#include "cancel.hpp"
#include "integer.hpp"
#include "scientific.hpp"
#include "constants.hpp"

#include <gtest/gtest.h>
#include <gmpxx.h>
#include <mpfr.h>

using namespace sc;

class CancelTest : public ::testing::Test {
protected:
    void SetUp() override {
        // The static initialiser already installed our allocator at
        // program start; calling it again is a deliberate no-op-by-
        // overwrite for explicitness in tests.
        cancel::install_allocator();
        cancel::reset();
    }
    void TearDown() override {
        cancel::reset();
    }
};

// --- Section semantics ---

TEST_F(CancelTest, NoSectionFlagDoesNothing) {
    // Outside an active Section, allocator must NOT throw — even when
    // the cancel flag is set. This is what protects the UI thread from
    // spuriously throwing during display formatting while a calc-thread
    // cancellation is in flight.
    cancel::request();
    EXPECT_NO_THROW({
        mpz_class a("123456789012345678901234567890");
        mpz_class b = a * a;
        (void)b;
    });
}

TEST_F(CancelTest, SectionWithoutFlagDoesNothing) {
    cancel::Section s;
    EXPECT_NO_THROW({
        mpz_class a = 12345;
        mpz_class b = a * a;
        (void)b;
    });
}

// --- Throw-from-allocator across GMP ---

TEST_F(CancelTest, GmpMultiplyThrowsAfterRequest) {
    cancel::Section s;
    mpz_class a("123456789012345678901234567890");  // pre-allocated
    cancel::request();
    EXPECT_THROW({
        // a * a allocates the result limbs → our hook throws.
        mpz_class b = a * a;
        (void)b;
    }, CancelledException);
}

TEST_F(CancelTest, IntegerPowThrowsMidComputation) {
    // 7^1000 is well under our pre-flight result-bits cap (~2800 bits)
    // so it gets to the actual mpz_pow_ui call — request cancel before
    // entering the section but AFTER any pre-allocation, then verify
    // the call throws.
    cancel::Section s;
    cancel::request();
    EXPECT_THROW({
        auto v = integer::pow(Integer{7}, 1000ull);
        (void)v;
    }, CancelledException);
}

// --- Throw-from-allocator across MPFR ---

TEST_F(CancelTest, MpfrComputeThrowsAfterRequest) {
    // MPFR queries the GMP allocator on every alloc (mpfr-gmp.c), so
    // throws originate in our hook but unwind through MPFR's C frames
    // before reaching us. Needs FetchMPFR.cmake's -fexceptions/EHa.
    cancel::Section s;
    cancel::request();
    EXPECT_THROW({
        // Compute pi to ~3000 digits — plenty of MPFR allocations,
        // first one trips the hook.
        auto v = constants::pi(3000);
        (void)v;
    }, CancelledException);
}

// --- Reset semantics ---

TEST_F(CancelTest, ResetClearsFlag) {
    cancel::request();
    EXPECT_TRUE(cancel::requested());
    cancel::reset();
    EXPECT_FALSE(cancel::requested());

    // After reset, computation inside a Section works again.
    cancel::Section s;
    EXPECT_NO_THROW({
        mpz_class a("99999999999999999999");
        mpz_class b = a * a;
        (void)b;
    });
}

// --- Section nesting ---

TEST_F(CancelTest, NestedSectionsBothCancellable) {
    cancel::Section outer;
    {
        cancel::Section inner;
        cancel::request();
        EXPECT_THROW({
            mpz_class a("1" + std::string(50, '0'));
            mpz_class b = a * a;
            (void)b;
        }, CancelledException);
    }
    // After outer scope ends, depth drops to 0; outside any Section
    // the allocator stops checking the flag (which is still set).
    cancel::reset();
}

// --- Free hook is noexcept (regression test) ---

TEST_F(CancelTest, FreeNeverThrows) {
    // Construct a heap-backed mpz_class while flag is set + Section
    // active; let it destruct (which calls free). Free must not throw
    // — destructors during normal scope exit don't tolerate exceptions
    // and we'd terminate.
    cancel::Section s;
    {
        mpz_class big("123456789012345678901234567890");
        cancel::request();
        // big goes out of scope here → free is called. Must not throw.
    }
    cancel::reset();
    SUCCEED();
}
