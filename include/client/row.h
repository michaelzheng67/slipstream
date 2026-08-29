#pragma once

#include <optional>
#include <string>
#include <unordered_map>

enum class row_type { quote, trade };

inline std::unordered_map<std::string, row_type> string_to_row_type{
    {"Q", row_type::quote},
    {"T", row_type::trade},
};

struct row {
  std::string timestamp;
  row_type type;
  std::string symbol;
  std::optional<double> bid_price;
  std::optional<uint32_t> bid_qty;
  std::optional<double> ask_price;
  std::optional<uint32_t> ask_qty;

  std::optional<double> price;
  std::optional<uint32_t> qty;
};