#pragma once

#include <stdint.h>

#pragma pack(push, 1)
struct frame_header {
  uint16_t body_len;
  uint8_t msg_type;
  uint8_t version;
};

struct quote_body {
  char symbol[12];
  uint64_t ts_ns;
  uint32_t bid_qty;
  int64_t bid_px;
  uint32_t ask_qty;
  int64_t ask_px;
};

struct trade_body {
  char symbol[12];
  uint64_t ts_ns;
  uint32_t qty;
  int64_t px;
  char aggressor;
  int64_t id;
};

struct heartbeat_body {
  uint64_t ts_ns;
};

struct sessioncontrol_body {
  uint64_t ts_ns;
  uint8_t state;
};

#pragma pack(pop)