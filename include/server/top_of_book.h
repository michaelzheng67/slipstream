#pragma once

#include "codec/protocol.h"
#include "common/price.h"
#include <cstdint>
#include <format>

struct level {
  int64_t px{};
  uint32_t qty{};
};

struct top_of_book {
  level bid{};
  level ask{};
  uint64_t ts_ns{};

  void update(const quote_body &qb) {
    bid = {qb.bid_px, qb.bid_qty};
    ask = {qb.ask_px, qb.ask_qty};
    ts_ns = qb.ts_ns;
  }

  [[nodiscard]] int64_t spread() const { return ask.px - bid.px; }

  [[nodiscard]] int64_t mid_x2() const { return bid.px + ask.px; }

  [[nodiscard]] bool valid() const { return ts_ns != 0; }
};

// lets std::format / std::println print a book directly: "34.1900 x 34.2000 (100 / 250)"
template <> struct std::formatter<top_of_book> {
  constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

  auto format(const top_of_book &b, std::format_context &ctx) const {
    return std::format_to(ctx.out(), "{} x {} ({} / {})", fmt_px(b.bid.px),
                          fmt_px(b.ask.px), b.bid.qty, b.ask.qty);
  }
};
