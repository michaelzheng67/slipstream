#include "codec/decoder.h"
#include <cstring>
#include <gtest/gtest.h>
#include <vector>

static std::vector<std::byte> make_frame(uint8_t msg_type,
                                         std::size_t body_len) {
  frame_header header{
      .body_len = static_cast<uint16_t>(body_len),
      .msg_type = msg_type,
      .version = 1,
  };
  std::vector<std::byte> frame(sizeof(header) + body_len);
  std::memcpy(frame.data(), &header, sizeof(header));
  return frame;
}

TEST(DecoderTest, ThrowsOnUnknownMsgType) {
  auto frame = make_frame(99, sizeof(quote_body));
  EXPECT_THROW(decoder::decoding(frame.data(), frame.size()),
               std::runtime_error);
}

TEST(DecoderTest, ThrowsOnShortBody) {
  auto frame = make_frame(1, sizeof(quote_body) - 1);
  EXPECT_THROW(decoder::decoding(frame.data(), frame.size()),
               std::runtime_error);
}

TEST(DecoderTest, ThrowsOnFrameShorterThanHeader) {
  std::vector<std::byte> frame(sizeof(frame_header) - 1);
  EXPECT_THROW(decoder::decoding(frame.data(), frame.size()),
               std::runtime_error);
}
