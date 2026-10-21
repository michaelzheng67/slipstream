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
    } else if (arg == "--band") {
      cfg.band_bps = parse_bps(next_value(arg));
    } else if (arg == "--participation-cap") {
      cfg.participation_cap_bps = parse_bps(next_value(arg));
    } else if (arg == "--max-quantity") {
      auto v = next_value(arg);
      auto res =
          std::from_chars(v.data(), v.data() + v.size(), cfg.max_quantity);
      if (res.ec != std::errc{}) {
        throw std::runtime_error("invalid --max-quantity value: " +
                                 std::string(v));
      }
    } else if (arg == "--quiet") {
      cfg.quiet = true;
    } else {
      throw std::runtime_error("unknown argument: " + std::string(arg));
    }
  }

  if (cfg.symbol.empty()) {
    throw std::runtime_error("--symbol is required");
  }

  return cfg;
}

// "0.002" -> 20. decimal string to basis points without going through double
uint32_t parse_bps(std::string_view s) {
  auto dot = s.find('.');
  std::string_view whole_str = s.substr(0, dot);
  std::string_view frac_str =
      dot == std::string_view::npos ? "" : s.substr(dot + 1);

  if (whole_str.empty()) {
    throw std::runtime_error("invalid decimal: " + std::string(s));
  }

  uint64_t whole = 0;
  auto [end, ec] = std::from_chars(whole_str.data(),
                                   whole_str.data() + whole_str.size(), whole);
  if (ec != std::errc{} || end != whole_str.data() + whole_str.size()) {
    throw std::runtime_error("invalid decimal: " + std::string(s));
  }

  // each place after the dot is worth 1000, 100, 10, 1 bps
  uint64_t frac = 0;
  uint64_t worth = 1000;
  for (char c : frac_str) {
    if (c < '0' || c > '9') {
      throw std::runtime_error("invalid decimal: " + std::string(s));
    }
    if (worth == 0) {
      throw std::runtime_error("more precision than 1 bp: " + std::string(s));
    }
    frac += static_cast<uint64_t>(c - '0') * worth;
    worth /= 10;
  }

  uint64_t bps = whole * 10'000 + frac;
  if (whole > UINT32_MAX / 10'000 || bps > UINT32_MAX) {
    throw std::runtime_error("decimal out of range: " + std::string(s));
  }
  return static_cast<uint32_t>(bps);
}
