#pragma once

#include "value.hpp"

namespace sc {
namespace hms {

// HMS + HMS
ValuePtr add(const HMS& a, const HMS& b, int precision);
// HMS - HMS
ValuePtr sub(const HMS& a, const HMS& b, int precision);
// HMS * real scalar
ValuePtr mul_scalar(const HMS& a, const ValuePtr& scalar, int precision);
// HMS / real scalar
ValuePtr div_scalar(const HMS& a, const ValuePtr& scalar, int precision);
// Negate HMS
ValuePtr neg(const HMS& a);

// Convert HMS to decimal degrees
ValuePtr to_degrees(const HMS& a, int precision);
// Convert decimal degrees to HMS
ValuePtr from_degrees(const ValuePtr& deg, int precision);

} // namespace hms
} // namespace sc
