#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>

enum class overflow_policy { halt, drop_oldest };

struct print {
  uint64_t ts_ns{};
  int64_t px{};
  uint32_t qty{};
};

using i128 = __int128;

template <std::size_t N, overflow_policy Policy> class rolling_vwap {
  static_assert(N > 0 && (N & (N - 1)) == 0, "N must be a power of two");

  std::array<print, N> _buf{};
  std::size_t _head{0}, _count{0};
  i128 _sum_pxqty{0};
  uint64_t _sum_qty{0};
  uint64_t _window_ns;

  void evict_oldest() {
    const auto old_print = _buf[_head];
    _sum_pxqty -= static_cast<i128>(old_print.px) * old_print.qty;
    _sum_qty -= old_print.qty;
    _head = (_head + 1) & (N - 1);
    _count--;
  }

public:
  explicit rolling_vwap(uint64_t window_ns) : _window_ns(window_ns) {}

  void add(uint64_t ts_ns, int64_t px, uint32_t qty) {
    while (_count > 0 && _buf[_head].ts_ns + _window_ns < ts_ns) {
      evict_oldest();
    }

    if (_count == N) {
      if constexpr (Policy == overflow_policy::halt) {
        throw std::runtime_error("rolling vwap overflow");
      } else {
        evict_oldest();
      }
    }

    _buf[(_head + _count) & (N - 1)] = print(ts_ns, px, qty);
    _sum_pxqty += static_cast<i128>(px) * qty;
    _sum_qty += qty;
    _count++;
  }

  [[nodiscard]] std::optional<int64_t> vwap() const {
    if (_sum_qty == 0) {
      return std::nullopt;
    }

    return static_cast<int64_t>(_sum_pxqty / _sum_qty);
  }
};