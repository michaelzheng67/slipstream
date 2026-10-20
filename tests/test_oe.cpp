#include "client/oe.h"
#include <cstring>
#include <functional>
#include <gtest/gtest.h>
#include <vector>

TEST(OrderEntryClientTest, HandlesIncomingNewOrder) {
  const std::string path = "./data/test_file.csv";

  {
    std::ofstream file(path);
    file << "Timestamp,Type,Symbol,BidPrice,BidQty,AskPrice,AskQty,Price,Qty\n";
  }

  order_entry_client client(path);

  new_order order{};
  order.client_order_id = 123;
  order.status = 'A';
  order.side = 'B';
  order.qty = 50;
  order.limit_px = 341900;
  order.trace_id = 7;

  frame_header header{
      .body_len = sizeof(new_order),
      .msg_type = 10,
      .version = 1,
  };

  std::vector<std::byte> frame(sizeof(frame_header) + sizeof(new_order));
  std::memcpy(frame.data(), &header, sizeof(header));
  std::memcpy(frame.data() + sizeof(header), &order, sizeof(order));

  std::vector<new_order> orders;
  std::function<void(const new_order &)> callback =
      [&](const new_order &o) { orders.push_back(o); };
  std::function<void(const heartbeat_body &)> on_heartbeat =
      [](const heartbeat_body &) {};

  // Split the incoming message across two recv-like boundaries.
  std::size_t split = frame.size() / 2;

  client.feed(frame.data(), split, callback, on_heartbeat);
  EXPECT_TRUE(orders.empty());

  client.feed(frame.data() + split, frame.size() - split, callback,
              on_heartbeat);
  ASSERT_EQ(orders.size(), 1);

  EXPECT_EQ(orders[0].client_order_id, 123u);
  EXPECT_EQ(orders[0].status, 'A');
  EXPECT_EQ(orders[0].side, 'B');
  EXPECT_EQ(orders[0].qty, 50u);
  EXPECT_EQ(orders[0].limit_px, 341900);
  EXPECT_EQ(orders[0].trace_id, 7);
}
