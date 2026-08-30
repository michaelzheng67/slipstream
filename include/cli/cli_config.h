#pragma once

#include <cstdint>
#include <string>

struct flag_config {
  uint16_t md_port = 0;
  uint16_t oe_port = 0;
  std::string symbol;
  uint64_t vwap_window_ms = 0;
};

flag_config parse_args(int argc, char **argv);