#include "server/slipstream_server.h"
#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {

int connect_to(uint16_t port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
  if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
    close(fd);
    return -1;
  }
  return fd;
}

} // namespace

TEST(SlipstreamServer, AcceptsBothClients) {
  slipstream_server server(0, 0, "SYNTH4", 1000);
  server.start();

  std::thread server_thread([&] { server.run(); });

  int md_client = connect_to(server.md_port());
  ASSERT_GE(md_client, 0);

  int oe_client = connect_to(server.oe_port());
  ASSERT_GE(oe_client, 0);

  close(md_client);
  close(oe_client);
  server_thread.join();
}