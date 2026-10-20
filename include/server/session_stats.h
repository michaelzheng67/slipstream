#pragma once

#include "server/decision_engine.h"
#include <array>
#include <cstdint>
#include <print>

struct session_stats {
  uint64_t orders_sent{};
  uint64_t accepted_qty{};
  uint64_t market_qty{};
  std::array<uint64_t, static_cast<std::size_t>(reject_reason::count)>
      by_reason{};

  void on_trade(uint32_t qty) { market_qty += qty; }
  void on_decision(const decision &d) {
    ++orders_sent;
    ++by_reason[static_cast<std::size_t>(d.reason)];

    if (d.is_accepted()) {
      accepted_qty += d.quantity;
    }
  }
  void print() const {
    std::println("=== execution report ===");
    std::println("{:<16}{}", "orders sent:", orders_sent);
    std::println("{:<16}{}", "accepted qty:", accepted_qty);
    std::println("{:<16}{}", "market qty:", market_qty);

    // cold path, printed once: double is fine here
    if (market_qty > 0) {
      std::println("{:<16}{:.2f}%", "participation:",
                   100.0 * static_cast<double>(accepted_qty) /
                       static_cast<double>(market_qty));
    } else {
      std::println("{:<16}n/a", "participation:");
    }

    for (std::size_t i = 0; i < by_reason.size(); ++i) {
      if (by_reason[i] > 0) {
        std::println("  {:<24}{}", to_string(static_cast<reject_reason>(i)),
                     by_reason[i]);
      }
    }
  }
};