#include "codec/parser.h"
#include "codec/protocol.h"
#include <cstring>
#include <gtest/gtest.h>
#include <vector>

TEST(ParserTest, HandlesSplitFrame) {
  parser p(1024);

  quote_body quote{};
  std::memcpy(quote.symbol, "SYNTH4", 6);
  quote.ts_ns = 123;
  quote.bid_qty = 10;
  quote.bid_px = 1000000;
  quote.ask_qty = 20;
  quote.ask_px = 1001000;

  frame_header header{
      .body_len = sizeof(quote_body),
      .msg_type = 1,
      .version = 1,
  };

  std::vector<std::byte> frame(sizeof(frame_header) + sizeof(quote_body));
  std::memcpy(frame.data(), &header, sizeof(header));
  std::memcpy(frame.data() + sizeof(header), &quote, sizeof(quote));

  size_t split = frame.size() / 2;

  auto first = p.feed(frame.data(), split);

  EXPECT_TRUE(first.empty());

  auto second = p.feed(frame.data() + split, frame.size() - split);

  ASSERT_EQ(second.size(), 1);

  auto decoded = std::get<quote_body>(second[0]);

  EXPECT_EQ(decoded.ts_ns, 123);
  EXPECT_EQ(decoded.bid_qty, 10);
  EXPECT_EQ(decoded.bid_px, 1000000);
  EXPECT_EQ(decoded.ask_qty, 20);
  EXPECT_EQ(decoded.ask_px, 1001000);

  EXPECT_EQ(std::string(decoded.symbol, 6), "SYNTH4");
}