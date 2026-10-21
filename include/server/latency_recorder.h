#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <print>
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
  void print() const {
    if (_samples.empty()) {
      std::println("no samples");
      return;
    }

    auto sorted = _samples;
    std::sort(sorted.begin(), sorted.end());
    auto pct = [&](double p) {
      auto n = sorted.size();
      auto rank =
          static_cast<std::size_t>(std::ceil(p * static_cast<double>(n)));
      return sorted[rank - 1];
    };
    std::println("=== tick-to-order latency (ns) ===");
    std::println("samples: {}  dropped: {}", sorted.size(), _dropped);
    std::println("p50: {}  p99: {}  p99.9: {}  max: {}", pct(0.50), pct(0.99),
                 pct(0.999), sorted.back());
  }
};