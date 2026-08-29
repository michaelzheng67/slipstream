#include "row.h"
#include <optional>
#include <sstream>
#include <vector>

namespace csv_row {

inline std::vector<std::string> split_csv_line(const std::string_view line) {
  std::vector<std::string> ret;
  std::stringstream ss{std::string(line)};
  std::string cell;

  while (std::getline(ss, cell, ',')) {
    ret.push_back(cell);
  }

  return ret;
}

inline uint64_t parse_timestamp_ns(const std::string &ts) {
  int h, m, s, ms;

  std::sscanf(ts.c_str(), "%d:%d:%d.%d", &h, &m, &s, &ms);
  uint64_t total_ms = ((h * 60ULL * 60ULL) + (m * 60ULL) + s) * 1000ULL + ms;
  return total_ms * 1'000'000ULL;
}

inline row parse_row(const std::vector<std::string> &fields) {
  row r;
  r.timestamp = fields[0];
  r.type = string_to_row_type[fields[1]];
  r.symbol = fields[2];
  r.bid_price = fields[3].empty() ? std::nullopt
                                  : std::optional<double>(std::stod(fields[3]));
  r.bid_qty = fields[4].empty() ? std::nullopt
                                : std::optional<uint32_t>(static_cast<uint32_t>(
                                      std::stoul(fields[4])));
  r.ask_price = fields[5].empty() ? std::nullopt
                                  : std::optional<double>(std::stod(fields[5]));
  r.ask_qty = fields[6].empty() ? std::nullopt
                                : std::optional<uint32_t>(static_cast<uint32_t>(
                                      std::stoul(fields[6])));
  r.price = fields[7].empty() ? std::nullopt
                              : std::optional<double>(std::stod(fields[7]));
  r.qty = fields[8].empty() ? std::nullopt
                            : std::optional<uint32_t>(
                                  static_cast<uint32_t>(std::stoul(fields[8])));
  return r;
}

inline std::optional<row> next_row(std::istream &in) {
  std::string line;
  if (!std::getline(in, line)) {
    return std::nullopt;
  }

  return parse_row(split_csv_line(line));
}
} // namespace csv_row