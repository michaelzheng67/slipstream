#include "client/harness.h"
#include <iostream>

// usage: slipstream_client <host> <md_port> <oe_port> <csv>
int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "usage: " << argv[0] << " <host> <md_port> <oe_port> <csv>\n";
    return 1;
  }

  try {
    client_harness harness(argv[1], std::stoi(argv[2]), std::stoi(argv[3]),
                           argv[4], argv[4]);
    harness.run();
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }
}
