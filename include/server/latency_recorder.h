#pragma once

#include <cstdint>
#include <vector>

class latency_recorder {
  std::vector<uint64_t> _samples;
  uint64_t _dropped{0};

public:
  explicit latency_recorder(std::size_t capacity) {
    _samples.reserve(capacity);
  }
  void record(uint64_t ns) {
    if (_samples.size() < _samples.capacity()) {
      _samples.push_back(ns);
    } else {
      ++_dropped;
    }
  }
  void print() const;
};