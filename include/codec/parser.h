#include "decoder.h"
#include <cstddef>
#include <memory>
#include <vector>

class parser {
  std::unique_ptr<std::byte[]> _buf;
  std::size_t _capacity;
  std::size_t _size{0};

public:
  parser(std::size_t capacity)
      : _buf(std::make_unique<std::byte[]>(capacity)), _capacity(capacity) {}

  std::vector<decoder::msg_body> feed(std::byte *new_bytes,
                                      std::size_t num_bytes) {

    if (_size + num_bytes >= _capacity) {
      throw std::out_of_range("stream parser buffer overflow");
    }

    std::vector<decoder::msg_body> ret;

    std::memcpy(_buf.get() + _size, new_bytes, num_bytes);
    _size += num_bytes;

    while (_size >= sizeof(frame_header)) {
      frame_header header;
      std::memcpy(&header, _buf.get(), sizeof(frame_header));

      std::size_t total_size = sizeof(frame_header) + header.body_len;

      if (_size < total_size) {
        break;
      }

      ret.push_back(decoder::decoding(_buf.get(), total_size));
      std::size_t remaining = _size - total_size;

      if (remaining > 0) {
        std::memmove(_buf.get(), _buf.get() + total_size, remaining);
      }

      _size = remaining;
    }
    return ret;
  }
};