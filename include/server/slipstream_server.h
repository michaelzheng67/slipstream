#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

class slipstream_server {

  int _md_fd{-1};
  int _oe_fd{-1};
  uint16_t _md_port{0};
  uint16_t _oe_port{0};

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

public:
  slipstream_server(uint16_t md, uint16_t oe) : _md_port(md), _oe_port(oe) {}

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

    int oe_client =
        accept(_oe_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_len);

    if (oe_client < 0) {
      close(md_client);
      throw std::runtime_error("OE accept failed");
    }

    close(md_client);
    close(oe_client);
  }
};