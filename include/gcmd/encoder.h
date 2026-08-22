#include "protocol.h"
#include <cstddef>
#include <cstring>
#include <stdint.h>

namespace gcmd_encoder {

using bytes_written = std::size_t;

template <typename T>
bytes_written encoding(const frame_header &fh, const T &qb, std::byte *out) {
  std::memcpy(out, &fh, sizeof(fh));
  std::memcpy(out + sizeof(fh), &qb, sizeof(qb));
  return sizeof(fh) + sizeof(qb);
}
}; // namespace gcmd_encoder