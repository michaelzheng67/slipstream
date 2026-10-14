#pragma once

#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <poll.h>
#include <print>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

#include "codec/encoder.h"
#include "codec/parser.h"
#include "common/net.h"
#include "server/decision_engine.h"
#include "server/rolling_vwap.h"
#include "server/top_of_book.h"

class slipstream_server {

  int _md_fd{-1};
  int _oe_fd{-1};   // listening socket: only accepts
  int _oe_conn{-1}; // accepted connection: orders are sent here
  uint16_t _md_port{0};
  uint16_t _oe_port{0};

  parser _md_parser{32768};
  parser _oe_parser{32768};

  uint64_t _md_dropped{0};
  uint64_t _oe_dropped{0};

  uint64_t _next_order_id{1};

  std::string _symbol;
  top_of_book _book;
  rolling_vwap<4096, overflow_policy::halt> _vwap;
  decision_engine _engine;

  static int create_fd(uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
      throw std::runtime_error("socket failed");
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
      close(fd);
      throw std::runtime_error("bind failed");
    }

    if (listen(fd, 10) < 0) {
      close(fd);
      throw std::runtime_error("listen failed");
    }

    return fd;
  }

  // in case we pass '0' into the server, we need to retrieve knowledge of
  // bounded port from OS
  static uint16_t bound_port(int fd) {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &len) < 0) {
      throw std::runtime_error("getsockname failed");
    }
    return ntohs(addr.sin_port);
  }

  void handle_md_message(decoder::msg_body &message) {

    auto *body = std::get_if<quote_body>(&message);
    if (!body) {
      return;
    }

    std::string_view sym(body->symbol, strnlen(body->symbol, 12));
    if (sym != _symbol) {
      ++_md_dropped;
      return;
    }

    _book.update(*body);
    _engine.on_quote();
    std::println("{} book: {}", sym, _book);
  }

  void handle_oe_message(decoder::msg_body &message) {

    auto *body = std::get_if<trade_body>(&message);
    if (!body) {
      return;
    }

    std::string_view sym(body->symbol, strnlen(body->symbol, 12));
    if (sym != _symbol) {
      ++_oe_dropped;
      return;
    }

    _vwap.add(body->ts_ns, body->px, body->qty);
    _engine.on_trade(body->ts_ns, body->qty);
    auto d = _engine.evaluate(body->ts_ns, _book, _vwap.vwap());
    send_order(d, *body);
    if (auto v = _vwap.vwap()) {
      std::println("{} trade {} x {} | vwap {}", sym, fmt_px(body->px),
                   body->qty, fmt_px(*v));
    }

    if (d.is_accepted()) {
      std::println("{} decision {} {} @ {}", sym, to_string(d.side), d.quantity,
                   fmt_px(d.limit_price));
    } else {
      std::println("{} decision rejected: {}", sym, to_string(d.reason));
    }
  }

  template <typename OnMessage>
  bool read_one(std::string_view name, int fd, parser &p,
                OnMessage on_message) {
    std::byte buf[4096];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    if (n <= 0) {
      return false;
    }

    try {
      auto messages = p.feed(buf, static_cast<std::size_t>(n));
      for (auto &message : messages) {
        on_message(message);
      }
      return true;
    } catch (const std::exception &e) {
      std::println(stderr, "{} connection dropped: {}", name, e.what());
      return false;
    }
  }

  void send_order(const decision &d, const trade_body &trigger) {
    new_order o{};
    o.client_order_id = _next_order_id++;
    std::memcpy(&o.symbol, &trigger.symbol, sizeof(o.symbol));
    o.status = d.is_accepted() ? 'A' : 'R';
    o.ts_ns = trigger.ts_ns;
    o.trace_id = trigger.id;
    o.side = side_t_to_char(d.side);
    o.qty = d.quantity;
    o.limit_px = d.limit_price;

    frame_header fh{sizeof(new_order), 10, 1};
    std::array<std::byte, sizeof(frame_header) + sizeof(new_order)> buf;
    encoder::encoding(fh, o, buf.data());

    send_all(_oe_conn, buf.data(), buf.size());
  }

public:
  explicit slipstream_server(const flag_config &cfg)
      : _md_port(cfg.md_port), _oe_port(cfg.oe_port), _symbol(cfg.symbol),
        _vwap(ms_to_ns(cfg.vwap_window_ms)), _engine(cfg) {}

  ~slipstream_server() {
    if (_md_fd >= 0) {
      close(_md_fd);
    }

    if (_oe_fd >= 0) {
      close(_oe_fd);
    }

    if (_oe_conn >= 0) {
      close(_oe_conn);
    }
  }

  uint16_t md_port() const { return _md_port; }
  uint16_t oe_port() const { return _oe_port; }

  void start() {
    _md_fd = create_fd(_md_port);
    _oe_fd = create_fd(_oe_port);
    _md_port = bound_port(_md_fd);
    _oe_port = bound_port(_oe_fd);
  }

  void run() {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    int md_client =
        accept(_md_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);

    if (md_client < 0) {
      throw std::runtime_error("MD accept failed");
    }

    client_len = sizeof(client_addr);
    _oe_conn =
        accept(_oe_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);

    if (_oe_conn < 0) {
      close(md_client);
      throw std::runtime_error("OE accept failed");
    }

    std::println("clients running!");
    pollfd fds[2] = {
        {.fd = md_client, .events = POLLIN, .revents = 0},
        {.fd = _oe_conn, .events = POLLIN, .revents = 0},
    };

    while (fds[0].fd >= 0 || fds[1].fd >= 0) {
      if (poll(fds, 2, -1) < 0) {
        if (errno == EINTR)
          continue;
        throw std::runtime_error("poll() failed");
      }

      if (fds[0].revents) {
        auto res = read_one("md", fds[0].fd, _md_parser,
                            [this](auto &m) { handle_md_message(m); });
        if (!res) {
          std::println("md dropped {} messages", _md_dropped);
          close(fds[0].fd);
          fds[0].fd = -1;
        }
      }

      if (fds[1].revents) {
        auto res = read_one("oe", fds[1].fd, _oe_parser,
                            [this](auto &m) { handle_oe_message(m); });
        if (!res) {
          std::println("oe dropped {} messages", _oe_dropped);
          close(fds[1].fd);
          fds[1].fd = -1;
          _oe_conn = -1;
        }
      }
    }
  }
};