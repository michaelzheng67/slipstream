#pragma once

#include <cstdint>
#include <format>
#include <string>

// prices on the wire and in memory are fixed point x10,000
inline constexpr int64_t px_scale = 10'000;

// 342000 -> "34.2000". display only: allocates, keep it off the hot path
inline std::string fmt_px(int64_t px) {
  // split the sign off first: / and % round toward zero, so -5 % 10000 is -5
  const char *sign = px < 0 ? "-" : "";
  uint64_t abs_px = px < 0 ? -static_cast<uint64_t>(px) : px;
  return std::format("{}{}.{:04}", sign, abs_px / px_scale, abs_px % px_scale);
}
