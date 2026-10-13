#pragma once

#include "cli/cli_config.h"
#include "server/top_of_book.h"
#include <algorithm>
#include <cstdint>
#include <optional>

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

inline uint64_t ms_to_ns(uint64_t v) { return v * 1'000'000; }

class decision_engine {
  static constexpr uint32_t k_min_warmup_quotes{10};
  int64_t _band_bps;
  uint32_t _cap_bps;
  uint32_t _max_quantity;
  uint64_t _window_ns;

  std::optional<uint64_t> _first_trade_ts{};
  uint32_t _quote_count{};
  uint64_t _market_volume{};
  uint64_t _our_volume{};
  bool _halted{false};

public:
  explicit decision_engine(const flag_config &cfg)
      : _band_bps(cfg.band_bps), _cap_bps(cfg.participation_cap_bps),
        _max_quantity(cfg.max_quantity),
        _window_ns(ms_to_ns(cfg.vwap_window_ms)) {}

  void on_quote() { ++_quote_count; }

  void on_trade(uint64_t ts, uint32_t qty) {
    if (!_first_trade_ts.has_value()) {
      _first_trade_ts = ts;
    }
    _market_volume += qty;
  }

  void set_halted(bool h) { _halted = h; }

  [[nodiscard]] decision evaluate(uint64_t now, const top_of_book &book,
                                  std::optional<int64_t> vwap) {

    constexpr uint64_t bps_scale = 10'000;
    decision d{};

    if (_halted) {
      d.reason = reject_reason::halted;
      return d;
    }

    if (!vwap || !_first_trade_ts.has_value() ||
        now < *_first_trade_ts + _window_ns ||
        _quote_count < k_min_warmup_quotes || !book.valid()) {
      d.reason = reject_reason::warming_up;
      return d;
    }

    auto v = *vwap;
    int64_t ask_px = book.ask.px;
    int64_t bid_px = book.bid.px;

    if (ask_px * bps_scale <= v * (bps_scale - _band_bps)) {
      d.side = side_t::buy;
    } else if (bid_px * bps_scale >= v * (bps_scale + _band_bps)) {
      d.side = side_t::sell;
    } else {
      d.reason = reject_reason::no_signal;
      return d;
    }

    uint32_t level_qty = d.side == side_t::buy ? book.ask.qty : book.bid.qty;
    uint32_t qty = std::min(_max_quantity, level_qty);

    if (qty == 0) {
      d.reason = reject_reason::zero_quantity;
      return d;
    }

    if ((_our_volume + qty) * bps_scale > _cap_bps * _market_volume) {
      d.reason = reject_reason::over_participation_cap;
      return d;
    }

    d.quantity = qty;
    d.limit_price = d.side == side_t::buy ? book.ask.px : book.bid.px;
    d.reason = reject_reason::accepted;
    _our_volume += qty;
    return d;
  }
};