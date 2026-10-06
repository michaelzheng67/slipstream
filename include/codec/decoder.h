#pragma once

#include "protocol.h"
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <stdint.h>
#include <type_traits>
#include <variant>

namespace decoder {

using bytes_written = std::size_t;
using msg_body = std::variant<quote_body, trade_body, heartbeat_body,
                              sessioncontrol_body, new_order, exec_report>;

template <typename T>
T read_body(const std::byte *body, std::size_t body_len) {
  static_assert(std::is_trivially_copyable_v<T>);

  if (body_len != sizeof(T)) {
    throw std::runtime_error("wrong packet length");
  }

  T ret;
  std::memcpy(&ret, body, sizeof(T));
  return ret;
}

inline msg_body decoding(const std::byte *buf, std::size_t len) {
  if (len < sizeof(frame_header)) {
    throw std::runtime_error("frame shorter than header");
  }

  frame_header fh;
  std::memcpy(&fh, buf, sizeof(frame_header));
  auto body_ptr = buf + sizeof(frame_header);
  auto body_len = len - sizeof(frame_header);

  switch (fh.msg_type) {
  case 1: {
    return read_body<quote_body>(body_ptr, body_len);
  }
  case 2: {
    return read_body<trade_body>(body_ptr, body_len);
  }
  case 3: {
    return read_body<heartbeat_body>(body_ptr, body_len);
  }
  case 4: {
    return read_body<sessioncontrol_body>(body_ptr, body_len);
  }
  case 10: {
    return read_body<new_order>(body_ptr, body_len);
  }
  case 11: {
    return read_body<exec_report>(body_ptr, body_len);
  }
  }

  throw std::runtime_error("unexpected message type");
}

}; // namespace decoder