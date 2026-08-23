#pragma once

#include <stdint.h>

#pragma pack(push, 1)

// shared header
struct frame_header {
  uint16_t body_len;
  uint8_t msg_type;
  uint8_t version;
};

// GCMD
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

// GCOE
struct new_order {
  uint64_t client_order_id;
  char symbol[12];
  char status;
  uint64_t ts_ns;
  int64_t trace_id;
  char side;
  uint32_t qty;
  int64_t limit_px;
};

struct exec_report {
  uint64_t client_order_id;
  uint64_t symbol[12];
  uint8_t status;
  uint32_t filled_qty;
  int64_t avg_px;
  uint8_t reason_code;
};

#pragma pack(pop)