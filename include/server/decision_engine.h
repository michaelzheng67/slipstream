#pragma once

#include <cstdint>

enum class side_t {
  not_set,
  buy,
  sell,
};

enum class reject_reason {
  not_set,
  accepted,
  warming_up,
  no_signal,
  over_participation_cap,
  zero_quantity,
  halted,
};

struct decision {
  side_t side{};
  uint32_t quantity{};
  int64_t limit_price{};
  reject_reason reason{};

  [[nodiscard]] bool is_accepted() const {
    return reason == reject_reason::accepted;
  }
};