#include "cli/cli_config.h"
#include <charconv>
#include <stdexcept>

flag_config parse_args(int argc, char **argv) {
  flag_config cfg;

  for (int i = 1; i < argc; i++) {
    std::string_view arg = argv[i];

    auto next_value = [&](std::string_view flag) -> std::string_view {
      if (i + 1 >= argc) {
        throw std::runtime_error("missing value for " + std::string(flag));
      }
      return argv[++i];
    };

    if (arg == "--md-port") {
      auto v = next_value(arg);
      auto res = std::from_chars(v.data(), v.data() + v.size(), cfg.md_port);
      if (res.ec != std::errc{}) {
        throw std::runtime_error("invalid --md-port value: " + std::string(v));
      }
    } else if (arg == "--oe-port") {
      auto v = next_value(arg);
      auto res = std::from_chars(v.data(), v.data() + v.size(), cfg.oe_port);
      if (res.ec != std::errc{}) {
        throw std::runtime_error("invalid --oe-port value: " + std::string(v));
      }
    } else if (arg == "--symbol") {
      cfg.symbol = std::string(next_value(arg));
    } else if (arg == "--vwap-window-ms") {
      auto v = next_value(arg);
      auto res =
          std::from_chars(v.data(), v.data() + v.size(), cfg.vwap_window_ms);
      if (res.ec != std::errc{}) {
        throw std::runtime_error("invalid --vwap-window-ms value: " +
                                 std::string(v));
      }
    } else {
      throw std::runtime_error("unknown argument: " + std::string(arg));
    }
  }

  if (cfg.symbol.empty()) {
    throw std::runtime_error("--symbol is required");
  }

  return cfg;
}