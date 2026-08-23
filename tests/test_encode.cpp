#include "codec/decoder.h"
#include "codec/encoder.h"
#include "codec/protocol.h"
#include <gtest/gtest.h>

TEST(Encoder, QuoteRoundTrip) {
  quote_body qb{};
  qb.bid_qty = 100;
  qb.ask_qty = 150;

  frame_header fh{};
  fh.body_len = sizeof(quote_body);
  fh.msg_type = 1;
  fh.version = 1;

  std::byte buf[128];
  auto n = encoder::encoding(fh, qb, buf);
  ASSERT_EQ(n, sizeof(frame_header) + sizeof(quote_body));

  decoder::msg_body decoded = decoder::decoding(buf, n);
  auto *out = std::get_if<quote_body>(&decoded);
  ASSERT_NE(out, nullptr);
  EXPECT_EQ(out->bid_qty, qb.bid_qty);
  EXPECT_EQ(out->ask_qty, qb.ask_qty);
}