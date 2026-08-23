#include <gtest/gtest.h>

#include "client/md.h"
#include <chrono>
#include <fstream>
#include <functional>
#include <vector>

TEST(MarketDataClientTest, ReplaysQuotesWithTiming) {
  const std::string path = "./data/Quotes_and_Trades.csv";

  // Create tiny test CSV
  {
    std::ofstream file(path);

    file << "# test data\n";
    file << "Timestamp,Type,Symbol,BidPrice,BidQty,AskPrice,AskQty,Price,Qty\n";
    file << "09:30:00.000,Q,SYNTH4,34.18,100,34.19,200,,\n";
    file << "09:30:00.010,T,SYNTH4,,,,,34.19,50\n";
    file << "09:30:00.020,Q,SYNTH4,34.20,300,34.21,400,,\n";
  }

  market_data_client client(path);

  std::vector<row> rows;
  std::vector<std::chrono::steady_clock::time_point> times;

  std::function<void(const row &)> callback = [&](const row &r) {
    rows.push_back(r);
    times.push_back(std::chrono::steady_clock::now());
  };

  client.replay(callback);

  // Trade should have been ignored.
  ASSERT_EQ(rows.size(), 2);

  EXPECT_EQ(rows[0].symbol, "SYNTH4");
  EXPECT_DOUBLE_EQ(rows[0].bid_price.value(), 34.18);
  EXPECT_EQ(rows[0].bid_qty.value(), 100);

  EXPECT_EQ(rows[1].symbol, "SYNTH4");
  EXPECT_DOUBLE_EQ(rows[1].bid_price.value(), 34.20);
  EXPECT_EQ(rows[1].bid_qty.value(), 300);

  // CSV quotes were 20 ms apart.
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      times[1] - times[0]);

  EXPECT_GE(elapsed.count(), 15);
}