#pragma once

#include <cstddef>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>

// send() may write fewer bytes than asked; loop until the whole buffer is out
inline void send_all(int fd, const std::byte *data, std::size_t len) {
  std::size_t sent = 0;
  while (sent < len) {
    ssize_t n = send(fd, data + sent, len - sent, 0);
    if (n <= 0)
      throw std::runtime_error("send failed");
    sent += static_cast<std::size_t>(n);
  }
}
