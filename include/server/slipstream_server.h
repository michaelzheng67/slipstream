#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <print>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "codec/parser.h"
#include "server/top_of_book.h"

class slipstream_server {

  int _md_fd{-1};
  int _oe_fd{-1};
  uint16_t _md_port{0};
  uint16_t _oe_port{0};

  parser _md_parser{32768};
  parser _oe_parser{32768};

  uint64_t _md_dropped{0};
  uint64_t _oe_dropped{0};

  std::string _symbol;
  uint64_t _vwap_window_ns;
  top_of_book _book;

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

    quote_body body;
    try {
      body = std::get<0>(message); // quote_body
    } catch (const std::bad_variant_access &ex) {
      return;
    }

    std::string_view sym(body.symbol, strnlen(body.symbol, 12));
    if (sym != _symbol) {
      ++_md_dropped;
      return;
    }

    _book.update(body);
    std::println("{} book: {}", sym, _book);
  }

  void handle_oe_message(decoder::msg_body &message) {

    trade_body body;
    try {
      body = std::get<1>(message); // trade_body
    } catch (const std::bad_variant_access &ex) {
      return;
    }

    std::string_view sym(body.symbol, strnlen(body.symbol, 12));
    if (sym != _symbol) {
      ++_oe_dropped;
      return;
    }

    std::println("{} oe trade", sym);
  }

  void handle_md(int client_fd) {
    std::byte buf[4096];

    try {
      while (true) {
        ssize_t n = recv(client_fd, buf, sizeof(buf), 0);

        if (n <= 0) {
          break;
        }

        auto messages = _md_parser.feed(buf, static_cast<size_t>(n));
        for (auto &message : messages) {
          handle_md_message(message);
        }
      }
    } catch (const std::exception &e) {
      std::println(stderr, "md connection dropped: {}", e.what());
    }

    std::println("md dropped {} messages", _md_dropped);
    close(client_fd);
  }

  void handle_oe(int client_fd) {
    std::byte buf[4096];

    try {
      while (true) {
        ssize_t n = recv(client_fd, buf, sizeof(buf), 0);

        if (n <= 0) {
          break;
        }

        auto messages = _oe_parser.feed(buf, static_cast<size_t>(n));
        for (auto &message : messages) {
          handle_oe_message(message);
        }
      }
    } catch (const std::exception &e) {
      std::println(stderr, "oe connection dropped: {}", e.what());
    }

    std::println("oe dropped {} messages", _oe_dropped);
    close(client_fd);
  }

public:
  slipstream_server(uint16_t md, uint16_t oe, std::string symbol,
                    uint64_t vwap_window_ms)
      : _md_port(md), _oe_port(oe), _symbol(symbol),
        _vwap_window_ns(vwap_window_ms * 1'000'000ULL) {}

  ~slipstream_server() {
    if (_md_fd >= 0) {
      close(_md_fd);
    }

    if (_oe_fd >= 0) {
      close(_oe_fd);
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
    int oe_client =
        accept(_oe_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);

    if (oe_client < 0) {
      close(md_client);
      throw std::runtime_error("OE accept failed");
    }

    std::println("clients running!");
    std::thread md_thread([&, md_client] { handle_md(md_client); });
    std::thread oe_thread([&, oe_client] { handle_oe(oe_client); });

    md_thread.join();
    oe_thread.join();
  }
};