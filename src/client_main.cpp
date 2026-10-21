#include "client/harness.h"
#include <csignal>
#include <iostream>
#include <string_view>
// usage: slipstream_client <host> <md_port> <oe_port> <csv>
int main(int argc, char **argv) {
  std::signal(SIGPIPE, SIG_IGN);

  if (argc != 5 && argc != 6) {
    std::cerr << "usage: " << argv[0]
              << " <host> <md_port> <oe_port> <csv> [--no-sleep]\n";
    return 1;
  }

  bool realtime{true};
  if (argc == 6) {
    if (std::string_view(argv[5]) != "--no-sleep") {
      std::cerr << "Unknown option: " << argv[5] << '\n';
      return 1;
    }

    realtime = false;
  }

  try {
    client_harness harness(argv[1], std::stoi(argv[2]), std::stoi(argv[3]),
                           argv[4], argv[4], realtime);
    harness.run();
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }
}
