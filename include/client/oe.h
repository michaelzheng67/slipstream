#include "codec/parser.h"
#include "row.h"
#include "row_reader.h"
#include <chrono>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

class order_entry_client {
  std::ifstream _file;
  std::vector<std::string> _header;
  parser _parser;

public:
  order_entry_client(const std::string_view fp)
      : _file(std::string(fp)), _parser(65536) {
    if (!_file.is_open()) {
      throw std::runtime_error("failed to open data file");
    }

    std::string line;

    while (std::getline(_file, line)) {
      if (line.front() == '#') {
        continue;
      }

      // first one that is not going to be a header
      _header = csv_row::split_csv_line(line);
      break;
    }
  }

  void replay(std::function<void(const row &)> &on_order) {
    std::vector<std::string> res;
    std::string line;
    auto start_time = std::chrono::steady_clock::now();
    bool first_order = true;
    uint64_t first_ts_ns = 0;

    while (auto r = csv_row::next_row(_file)) {
      if (r->type != row_type::trade) {
        continue;
      }

      uint64_t ts_ns = csv_row::parse_timestamp_ns(r->timestamp);

      if (first_order) {
        first_ts_ns = ts_ns;
        start_time = std::chrono::steady_clock::now();
        first_order = false;
      }

      uint64_t offset_ns = ts_ns - first_ts_ns;
      std::this_thread::sleep_until(start_time +
                                    std::chrono::nanoseconds(offset_ns));
      on_order(*r);
    }
  }

  void feed(const std::byte *data, std::size_t len,
            const std::function<void(const exec_report &)> &on_report) {
    auto messages = _parser.feed(data, len);

    for (const auto &msg : messages) {
      if (std::holds_alternative<exec_report>(msg)) {
        on_report(std::get<exec_report>(msg));
      }
    }
  }
};