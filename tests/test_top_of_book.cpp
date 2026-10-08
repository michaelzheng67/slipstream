#include "server/top_of_book.h"
#include <format>
#include <gtest/gtest.h>

TEST(PriceTest, FormatsFixedPoint) {
  EXPECT_EQ(fmt_px(342000), "34.2000");
  EXPECT_EQ(fmt_px(340500), "34.0500");
  EXPECT_EQ(fmt_px(5), "0.0005");
  EXPECT_EQ(fmt_px(-5), "-0.0005");
}

TEST(TopOfBookTest, InvalidBeforeFirstQuote) {
  top_of_book b;
  EXPECT_FALSE(b.valid());
}

TEST(TopOfBookTest, UpdateSpreadMid) {
  quote_body q{};
  q.ts_ns = 1;
  q.bid_px = 341900;
  q.bid_qty = 100;
  q.ask_px = 342000;
  q.ask_qty = 250;

  top_of_book b;
  b.update(q);

  EXPECT_TRUE(b.valid());
  EXPECT_EQ(b.spread(), 100);
  EXPECT_EQ(b.mid_x2(), 683900); // odd: exact mid is a half tick
  EXPECT_EQ(std::format("{}", b), "34.1900 x 34.2000 (100 / 250)");
}
