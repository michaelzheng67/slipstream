#include "server/slipstream_server.h"

int main() {
  slipstream_server server(9000, 90001);
  server.run();
}