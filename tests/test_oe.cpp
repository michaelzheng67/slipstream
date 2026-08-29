#include "client/oe.h"
#include <cstring>
#include <functional>
#include <gtest/gtest.h>
#include <vector>

TEST(OrderEntryClientTest, HandlesIncomingExecReport) {
  const std::string path = "./data/test_file.csv";

  {
    std::ofstream file(path);
    file << "Timestamp,Type,Symbol,BidPrice,BidQty,AskPrice,AskQty,Price,Qty\n";
  }

  order_entry_client client(path);

  exec_report report{};
  report.client_order_id = 123;
  report.status = 1;
  report.filled_qty = 50;
  report.avg_px = 341900;
  report.reason_code = 0;

  frame_header header{
      .body_len = sizeof(exec_report),
      .msg_type = 11,
      .version = 1,
  };

  std::vector<std::byte> frame(sizeof(frame_header) + sizeof(exec_report));

  std::memcpy(frame.data(), &header, sizeof(header));

  std::memcpy(frame.data() + sizeof(header), &report, sizeof(report));

  std::vector<exec_report> reports;

  std::function<void(const exec_report &)> callback =
      [&](const exec_report &r) { reports.push_back(r); };

  // Split the incoming message across two recv-like boundaries.
  std::size_t split = frame.size() / 2;

  client.feed(frame.data(), split, callback);

  EXPECT_TRUE(reports.empty());

  client.feed(frame.data() + split, frame.size() - split, callback);

  ASSERT_EQ(reports.size(), 1);

  EXPECT_EQ(reports[0].client_order_id, 123);
  EXPECT_EQ(reports[0].filled_qty, 50);
  EXPECT_EQ(reports[0].avg_px, 341900);
  EXPECT_EQ(reports[0].reason_code, 0);
}