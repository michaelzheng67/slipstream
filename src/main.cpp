#include "cli/cli_config.h"
#include "server/slipstream_server.h"
#include <iostream>
#include <print>

int main(int argc, char **argv) {
  try {
    auto cfg = parse_args(argc, argv);

    std::thread server_thread([&]() {
      slipstream_server server(cfg.md_port, cfg.oe_port, cfg.symbol,
                               cfg.vwap_window_ms);
      std::println("starting server...");
      server.start();
      server.run();
    });

    server_thread.join();
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}