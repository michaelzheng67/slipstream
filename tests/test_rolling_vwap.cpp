#include "server/rolling_vwap.h"
#include <gtest/gtest.h>

using vwap_halt = rolling_vwap<4, overflow_policy::halt>;
using vwap_drop = rolling_vwap<4, overflow_policy::drop_oldest>;

TEST(RollingVwapTest, EmptyHasNoVwap) {
  vwap_halt v(1000);
  EXPECT_FALSE(v.vwap().has_value());
}

TEST(RollingVwapTest, WeightsByQty) {
  vwap_halt v(1000);
  v.add(1, 100, 1);
  v.add(2, 200, 3);
  EXPECT_EQ(v.vwap(), 175); // (100*1 + 200*3) / 4
}

TEST(RollingVwapTest, EvictsOnlyPrintsOlderThanWindow) {
  vwap_halt v(1000);
  v.add(0, 100, 1);
  v.add(1000, 200, 1); // first print is exactly window old: kept
  EXPECT_EQ(v.vwap(), 150);

  v.add(1001, 200, 1); // now it's older than window: evicted
  EXPECT_EQ(v.vwap(), 200);
}

TEST(RollingVwapTest, HaltThrowsWhenFull) {
  vwap_halt v(1000);
  for (uint64_t ts = 0; ts < 4; ++ts) {
    v.add(ts, 100, 1);
  }
  EXPECT_THROW(v.add(4, 100, 1), std::runtime_error);
}

TEST(RollingVwapTest, DropOldestWhenFull) {
  vwap_drop v(1000);
  v.add(0, 1000, 1); // dropped on the 5th add
  for (uint64_t ts = 1; ts < 5; ++ts) {
    v.add(ts, 100, 1);
  }
  EXPECT_EQ(v.vwap(), 100);
}

TEST(RollingVwapTest, SumExceedsInt64) {
  vwap_halt v(1000);
  const int64_t px = 1'000'000'000'000; // px * qty ~ 4e21 > INT64_MAX
  v.add(0, px, UINT32_MAX);
  v.add(1, px, UINT32_MAX);
  EXPECT_EQ(v.vwap(), px);
}
