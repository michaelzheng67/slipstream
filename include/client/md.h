#include "row.h"
#include <chrono>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

class market_data_client {
  std::ifstream _file;
  std::vector<std::string> _header;

  static std::vector<std::string> split_csv_line(const std::string_view line) {
    std::vector<std::string> ret;
    std::stringstream ss{std::string(line)};
    std::string cell;

    while (std::getline(ss, cell, ',')) {
      ret.push_back(cell);
    }

    return ret;
  }

  static uint64_t parse_timestamp_ns(const std::string &ts) {
    int h, m, s, ms;

    std::sscanf(ts.c_str(), "%d:%d:%d.%d", &h, &m, &s, &ms);
    uint64_t total_ms = ((h * 60ULL * 60ULL) + (m * 60ULL) + s) * 1000ULL + ms;
    return total_ms * 1'000'000ULL;
  }

public:
  market_data_client(const std::string_view fp) : _file(std::string(fp)) {
    if (!_file.is_open()) {
      throw std::runtime_error("failed to open data file");
    }

    std::string line;

    while (std::getline(_file, line)) {
      if (line.front() == '#') {
        continue;
      }

      // first one that is not going to be a header
      _header = split_csv_line(line);
      break;
    }
  }

  void replay(std::function<void(const row &)> &on_quote) {
    std::vector<std::string> res;
    std::string line;
    auto start_time = std::chrono::steady_clock::now();
    bool first_quote = true;
    uint64_t first_ts_ns = 0;

    while (std::getline(_file, line)) {
      res = split_csv_line(line);

      row r;
      r.timestamp = res[0];
      r.type = string_to_row_type[res[1]];
      r.symbol = res[2];
      r.bid_price = res[3].empty() ? std::nullopt
                                   : std::optional<double>(std::stod(res[3]));
      r.bid_qty = res[4].empty()
                      ? std::nullopt
                      : std::optional<uint32_t>(
                            static_cast<uint32_t>(std::stoul(res[4])));
      r.ask_price = res[5].empty() ? std::nullopt
                                   : std::optional<double>(std::stod(res[5]));
      r.ask_qty = res[6].empty()
                      ? std::nullopt
                      : std::optional<uint32_t>(
                            static_cast<uint32_t>(std::stoul(res[6])));
      r.price = res[7].empty() ? std::nullopt
                               : std::optional<double>(std::stod(res[7]));
      r.qty = res[8].empty() ? std::nullopt
                             : std::optional<uint32_t>(
                                   static_cast<uint32_t>(std::stoul(res[8])));

      if (r.type != row_type::quote) {
        continue;
      }

      uint64_t ts_ns = parse_timestamp_ns(r.timestamp);

      if (first_quote) {
        first_ts_ns = ts_ns;
        start_time = std::chrono::steady_clock::now();
        first_quote = false;
      }

      uint64_t offset_ns = ts_ns - first_ts_ns;
      std::this_thread::sleep_until(start_time +
                                    std::chrono::nanoseconds(offset_ns));
      on_quote(r);
    }
  }
};