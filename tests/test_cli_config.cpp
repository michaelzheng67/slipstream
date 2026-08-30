// cli_config_test.cpp
#include "cli/cli_config.h"
#include <gtest/gtest.h>
#include <vector>

namespace {

// helper: build a char*[] from a list of std::strings so we can call
// parse_args like it's really invoked from the command line
class argv_builder {
  std::vector<std::string> _storage;
  std::vector<char *> _argv;

public:
  explicit argv_builder(std::initializer_list<std::string> args) {
    _storage.emplace_back("./slipstream"); // argv[0]
    _storage.insert(_storage.end(), args);
    for (auto &s : _storage) {
      _argv.push_back(s.data());
    }
  }

  int argc() const { return static_cast<int>(_argv.size()); }
  char **argv() { return _argv.data(); }
};

} // namespace

TEST(ParseArgs, ParsesAllFlags) {
  argv_builder args({"--md-port", "9000", "--oe-port", "9001", "--symbol",
                     "SYNTH4", "--vwap-window-ms", "1000"});

  flag_config cfg = parse_args(args.argc(), args.argv());

  EXPECT_EQ(cfg.md_port, 9000);
  EXPECT_EQ(cfg.oe_port, 9001);
  EXPECT_EQ(cfg.symbol, "SYNTH4");
  EXPECT_EQ(cfg.vwap_window_ms, 1000u);
}

TEST(ParseArgs, FlagsCanAppearInAnyOrder) {
  argv_builder args({"--symbol", "SYNTH4", "--vwap-window-ms", "500",
                     "--oe-port", "9001", "--md-port", "9000"});

  flag_config cfg = parse_args(args.argc(), args.argv());

  EXPECT_EQ(cfg.md_port, 9000);
  EXPECT_EQ(cfg.oe_port, 9001);
  EXPECT_EQ(cfg.symbol, "SYNTH4");
  EXPECT_EQ(cfg.vwap_window_ms, 500u);
}

TEST(ParseArgs, MissingSymbolThrows) {
  argv_builder args({"--md-port", "9000", "--oe-port", "9001"});

  EXPECT_THROW(parse_args(args.argc(), args.argv()), std::runtime_error);
}

TEST(ParseArgs, UnknownFlagThrows) {
  argv_builder args({"--bogus-flag", "123"});

  EXPECT_THROW(parse_args(args.argc(), args.argv()), std::runtime_error);
}

TEST(ParseArgs, FlagMissingValueThrows) {
  argv_builder args({"--md-port"}); // no value follows

  EXPECT_THROW(parse_args(args.argc(), args.argv()), std::runtime_error);
}

TEST(ParseArgs, NonNumericPortThrows) {
  argv_builder args({"--md-port", "not-a-number", "--symbol", "SYNTH4"});

  EXPECT_THROW(parse_args(args.argc(), args.argv()), std::runtime_error);
}

TEST(ParseArgs, PortOutOfRangeThrows) {
  // uint16_t max is 65535 — 70000 overflows it
  argv_builder args({"--md-port", "70000", "--symbol", "SYNTH4"});

  EXPECT_THROW(parse_args(args.argc(), args.argv()), std::runtime_error);
}

TEST(ParseArgs, NonNumericVwapWindowThrows) {
  argv_builder args({"--symbol", "SYNTH4", "--vwap-window-ms", "abc"});

  EXPECT_THROW(parse_args(args.argc(), args.argv()), std::runtime_error);
}

TEST(ParseArgs, NoArgsThrowsMissingSymbol) {
  argv_builder args({});

  EXPECT_THROW(parse_args(args.argc(), args.argv()), std::runtime_error);
}