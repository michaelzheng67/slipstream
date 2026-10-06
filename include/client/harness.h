#pragma once

#include "codec/encoder.h"
#include "md.h"
#include "oe.h"
#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cmath>
#include <cstring>
#include <functional>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

inline int connect_to_server(const std::string &host, int port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    throw std::runtime_error("socket failed");
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(port));
  if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
    close(fd);
    throw std::runtime_error("invalid host: " + host);
  }

  if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
    close(fd);
    throw std::runtime_error("connect failed");
  }

  return fd;
}

inline void send_all(int fd, const std::byte *data, std::size_t len) {
  std::size_t sent = 0;
  while (sent < len) {
    ssize_t n = send(fd, data + sent, len - sent, 0);
    if (n <= 0)
      throw std::runtime_error("send failed");
    sent += static_cast<std::size_t>(n);
  }
}

// decimal price -> fixed point x10,000. llround so 101.23 doesn't truncate to
// 1012299 when the double comes out as 1012299.9999...
inline int64_t to_fixed_px(double px) { return std::llround(px * 10'000); }

// symbol fields are fixed 12 bytes, zero padded, not null terminated
inline void copy_symbol(char (&dst)[12], const std::string &src) {
  std::memcpy(dst, src.data(), std::min(src.size(), sizeof(dst)));
}

using quote_frame = std::array<std::byte, sizeof(frame_header) + sizeof(quote_body)>;
using trade_frame = std::array<std::byte, sizeof(frame_header) + sizeof(trade_body)>;

inline quote_frame serialize_quote(const row &r) {
  if (!r.bid_price || !r.bid_qty || !r.ask_price || !r.ask_qty) {
    throw std::runtime_error("quote row missing bid/ask fields");
  }

  frame_header fh{
      .body_len = sizeof(quote_body),
      .msg_type = 1,
      .version = 1,
  };

  quote_body qb{};
  copy_symbol(qb.symbol, r.symbol);
  qb.ts_ns = csv_row::parse_timestamp_ns(r.timestamp);
  qb.bid_qty = *r.bid_qty;
  qb.bid_px = to_fixed_px(*r.bid_price);
  qb.ask_qty = *r.ask_qty;
  qb.ask_px = to_fixed_px(*r.ask_price);

  quote_frame out;
  encoder::encoding(fh, qb, out.data());
  return out;
}

// the CSV has no aggressor side or trade id, so aggressor is 'U' (unknown) and
// id is a per-client sequence number supplied by the caller
inline trade_frame serialize_trade(const row &r, int64_t id) {
  if (!r.price || !r.qty) {
    throw std::runtime_error("trade row missing price/qty fields");
  }

  frame_header fh{
      .body_len = sizeof(trade_body),
      .msg_type = 2,
      .version = 1,
  };

  trade_body tb{};
  copy_symbol(tb.symbol, r.symbol);
  tb.ts_ns = csv_row::parse_timestamp_ns(r.timestamp);
  tb.qty = *r.qty;
  tb.px = to_fixed_px(*r.price);
  tb.aggressor = 'U';
  tb.id = id;

  trade_frame out;
  encoder::encoding(fh, tb, out.data());
  return out;
}

class client_harness {
  std::string _host;
  int _md_port;
  int _oe_port;
  market_data_client _md_client;
  order_entry_client _oe_client;
  int _md_fd{-1};
  int _oe_fd{-1};

public:
  client_harness(std::string host, int md_port, int oe_port,
                 std::string md_file, std::string oe_file)
      : _host(std::move(host)), _md_port(md_port), _oe_port(oe_port),
        _md_client(md_file), _oe_client(oe_file) {}

  ~client_harness() {
    if (_md_fd >= 0)
      close(_md_fd);
    if (_oe_fd >= 0)
      close(_oe_fd);
  }

  void run() {
    // Connect MD before OE — matches server's accept() order.
    _md_fd = connect_to_server(_host, _md_port);
    _oe_fd = connect_to_server(_host, _oe_port);

    std::thread md_thread([this] {
      std::function<void(const row &)> send_quote = [this](const row &r) {
        auto bytes = serialize_quote(r);
        send_all(_md_fd, bytes.data(), bytes.size());
      };
      _md_client.replay(send_quote);
    });

    std::thread oe_thread([this] {
      int64_t next_id = 1;
      std::function<void(const row &)> send_trade = [&](const row &r) {
        auto bytes = serialize_trade(r, next_id++);
        send_all(_oe_fd, bytes.data(), bytes.size());
      };
      _oe_client.replay(send_trade);
    });

    md_thread.join();
    oe_thread.join();

    // Half-close write sides so the server's recv() loops see EOF and exit.
    ::shutdown(_md_fd, SHUT_WR);
    ::shutdown(_oe_fd, SHUT_WR);
  }
};