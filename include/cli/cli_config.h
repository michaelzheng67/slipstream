#pragma once

#include <cstdint>
#include <string>
#include <string_view>

struct flag_config {
  uint16_t md_port = 0;
  uint16_t oe_port = 0;
  std::string symbol;
  uint64_t vwap_window_ms = 0;
  uint32_t band_bps = 0;
  uint32_t participation_cap_bps = 0;
  uint32_t max_quantity = 0;
  bool quiet = false;
};

flag_config parse_args(int argc, char **argv);
uint32_t parse_bps(std::string_view s);