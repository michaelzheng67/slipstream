#include "server/decision_engine.h"
#include <gtest/gtest.h>

namespace {

constexpr int64_t vwap_px = 1'000'000; // $100.0000
constexpr uint64_t window_ns = 1'000'000'000;

// band 20 bps: BUY if ask <= 998000, SELL if bid >= 1002000
flag_config make_cfg() {
  flag_config cfg;
  cfg.vwap_window_ms = 1000;
  cfg.band_bps = 20;
  cfg.participation_cap_bps = 1000; // 10%
  cfg.max_quantity = 500;
  return cfg;
}

top_of_book make_book(int64_t bid_px, uint32_t bid_qty, int64_t ask_px,
                      uint32_t ask_qty) {
  return top_of_book{{bid_px, bid_qty}, {ask_px, ask_qty}, 1};
}

// first trade at ts 0 with 10'000 market volume, plus enough quotes
decision_engine warmed_up_engine(flag_config cfg = make_cfg()) {
  decision_engine e(cfg);
  for (int i = 0; i < 10; ++i) {
    e.on_quote();
  }
  e.on_trade(0, 10'000);
  return e;
}

} // namespace

TEST(DecisionEngineTest, HaltedWinsOverEverything) {
  auto e = warmed_up_engine();
  e.set_halted(true);
  auto d = e.evaluate(window_ns, make_book(996'000, 200, 997'000, 300), vwap_px);
  EXPECT_EQ(d.reason, reject_reason::halted);
}

TEST(DecisionEngineTest, WarmingUp) {
  const auto book = make_book(996'000, 200, 997'000, 300);

  decision_engine no_trades(make_cfg());
  for (int i = 0; i < 10; ++i) {
    no_trades.on_quote();
  }
  EXPECT_EQ(no_trades.evaluate(window_ns, book, vwap_px).reason,
            reject_reason::warming_up);

  auto e = warmed_up_engine();
  EXPECT_EQ(e.evaluate(window_ns - 1, book, vwap_px).reason,
            reject_reason::warming_up); // window not elapsed
  EXPECT_EQ(e.evaluate(window_ns, book, std::nullopt).reason,
            reject_reason::warming_up); // no vwap
  EXPECT_EQ(e.evaluate(window_ns, top_of_book{}, vwap_px).reason,
            reject_reason::warming_up); // book never quoted

  decision_engine few_quotes(make_cfg());
  few_quotes.on_trade(0, 10'000);
  EXPECT_EQ(few_quotes.evaluate(window_ns, book, vwap_px).reason,
            reject_reason::warming_up);
}

TEST(DecisionEngineTest, NoSignalInsideBand) {
  auto e = warmed_up_engine();
  auto d = e.evaluate(window_ns, make_book(999'000, 200, 1'001'000, 300), vwap_px);
  EXPECT_EQ(d.reason, reject_reason::no_signal);
}

TEST(DecisionEngineTest, BuyHitsAsk) {
  auto e = warmed_up_engine();
  auto d = e.evaluate(window_ns, make_book(996'000, 200, 997'000, 300), vwap_px);
  ASSERT_TRUE(d.is_accepted());
  EXPECT_EQ(d.side, side_t::buy);
  EXPECT_EQ(d.limit_price, 997'000);
  EXPECT_EQ(d.quantity, 300u); // ask qty, below max_quantity
}

TEST(DecisionEngineTest, SellHitsBid) {
  auto e = warmed_up_engine();
  auto d = e.evaluate(window_ns, make_book(1'003'000, 900, 1'004'000, 100), vwap_px);
  ASSERT_TRUE(d.is_accepted());
  EXPECT_EQ(d.side, side_t::sell);
  EXPECT_EQ(d.limit_price, 1'003'000);
  EXPECT_EQ(d.quantity, 500u); // capped by max_quantity
}

TEST(DecisionEngineTest, BandEdgeIsInclusive) {
  auto e = warmed_up_engine();
  auto d = e.evaluate(window_ns, make_book(997'000, 200, 998'000, 300), vwap_px);
  EXPECT_EQ(d.side, side_t::buy);
  d = e.evaluate(window_ns, make_book(998'000, 200, 998'001, 300), vwap_px);
  EXPECT_EQ(d.reason, reject_reason::no_signal);
}

TEST(DecisionEngineTest, ZeroQuantity) {
  auto e = warmed_up_engine();
  auto d = e.evaluate(window_ns, make_book(996'000, 200, 997'000, 0), vwap_px);
  EXPECT_EQ(d.reason, reject_reason::zero_quantity);
}

TEST(DecisionEngineTest, ParticipationCap) {
  // cap = 10% of 10'000 = 1'000; each buy is 300
  auto e = warmed_up_engine();
  const auto book = make_book(996'000, 200, 997'000, 300);
  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(e.evaluate(window_ns, book, vwap_px).is_accepted()); // 900
  }
  EXPECT_EQ(e.evaluate(window_ns, book, vwap_px).reason,
            reject_reason::over_participation_cap); // 1200 > 1000

  // the reject didn't count toward our volume: still 900, so 900 + 300 fits
  e.on_trade(1, 2'000); // cap now 1'200
  EXPECT_TRUE(e.evaluate(window_ns, book, vwap_px).is_accepted());
}
