#include "protocol.h"
#include <cstddef>
#include <cstring>
#include <stdint.h>
#include <variant>

namespace decoder {

using bytes_written = std::size_t;
using msg_body = std::variant<quote_body, trade_body, heartbeat_body,
                              sessioncontrol_body, new_order, exec_report>;

inline msg_body decoding(const std::byte *buf, std::size_t len) {
  frame_header fh;
  std::memcpy(&fh, buf, sizeof(frame_header));
  auto body_ptr = buf + sizeof(frame_header);

  switch (fh.msg_type) {
  case 1: {
    quote_body qb;
    std::memcpy(&qb, body_ptr, sizeof(quote_body));
    return qb;
  }
  case 2: {
    trade_body tb;
    std::memcpy(&tb, body_ptr, sizeof(trade_body));
    return tb;
  }
  case 3: {
    heartbeat_body hb;
    std::memcpy(&hb, body_ptr, sizeof(heartbeat_body));
    return hb;
  }
  case 4: {
    sessioncontrol_body scb;
    std::memcpy(&scb, body_ptr, sizeof(sessioncontrol_body));
    return scb;
  }
  case 10: {
    new_order no;
    std::memcpy(&no, body_ptr, sizeof(new_order));
    return no;
  }
  case 11: {
    exec_report er;
    std::memcpy(&er, body_ptr, sizeof(exec_report));
    return er;
  }
  }
}

}; // namespace decoder