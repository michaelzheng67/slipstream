#include "cli_config.h"
#include "server/slipstream_server.h"
#include <iostream>

int main(int argc, char **argv) {
  try {
    auto cfg = parse_args(argc, argv);
    slipstream_server server(cfg.md_port, cfg.oe_port, cfg.symbol,
                             cfg.vwap_window_ms);
    server.start();
    server.run();
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}