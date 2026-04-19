#include <gtest/gtest.h>
#include "constants.hpp"

using namespace sc;

TEST(ConstantsTest, Pi12Digits) {
    auto pi = constants::pi(12);
    ASSERT_TRUE(pi->is_float());
    auto& f = pi->as_float();
    // pi = 3.14159265359 at 12 digits
    // mantissa should be 314159265359, exponent = -11
    EXPECT_EQ(f.exponent, -11);
    // Check first few digits
    std::string s = f.mantissa.get_str();
    EXPECT_TRUE(s.substr(0, 6) == "314159") << "got: " << s;
}

TEST(ConstantsTest, E12Digits) {
    auto e = constants::e(12);
    ASSERT_TRUE(e->is_float());
    auto& f = e->as_float();
    std::string s = f.mantissa.get_str();
    EXPECT_TRUE(s.substr(0, 6) == "271828") << "got: " << s;
}

TEST(ConstantsTest, Gamma12Digits) {
    auto g = constants::gamma(12);
    ASSERT_TRUE(g->is_float());
    auto& f = g->as_float();
    std::string s = f.mantissa.get_str();
    // Euler-Mascheroni = 0.577215664902...
    EXPECT_TRUE(s.substr(0, 6) == "577215") << "got: " << s;
}

TEST(ConstantsTest, Phi12Digits) {
    auto phi = constants::phi(12);
    ASSERT_TRUE(phi->is_float());
    auto& f = phi->as_float();
    std::string s = f.mantissa.get_str();
    // phi = 1.61803398875...
    EXPECT_TRUE(s.substr(0, 6) == "161803") << "got: " << s;
}

TEST(ConstantsTest, PiHighPrecision) {
    auto pi = constants::pi(50);
    ASSERT_TRUE(pi->is_float());
    auto& f = pi->as_float();
    std::string s = f.mantissa.get_str();
    // Check more digits
    EXPECT_TRUE(s.substr(0, 15) == "314159265358979") << "got: " << s;
}
