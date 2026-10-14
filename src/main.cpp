#include "cli/cli_config.h"
#include "server/slipstream_server.h"
#include <iostream>
#include <print>

int main(int argc, char **argv) {
  try {
    auto cfg = parse_args(argc, argv);

    slipstream_server server(cfg);
    std::println("starting server...");
    server.start();
    server.run();
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}