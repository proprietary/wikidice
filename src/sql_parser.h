#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/log/check.h>
#include <absl/log/log.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fmt/core.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "category_link_type.h"

namespace net_zelcon::wikidice {

class SQLParser {
  protected:
    std::istream &stream_;
    std::string table_name_;

  public:
    SQLParser(std::istream &stream, std::string_view table_name);

    auto next() -> std::optional<std::vector<std::string>>;
};

class CategoryLinksParser : public SQLParser {
  public:
    CategoryLinksParser(std::istream &stream)
        : SQLParser(stream, "categorylinks") {}

    auto next() -> std::optional<CategoryLinksRow>;
};

class CategoryParser : public SQLParser {
  public:
    CategoryParser(std::istream &stream) : SQLParser(stream, "category") {}

    auto next() -> std::optional<CategoryRow>;
};

auto read_category_table(std::istream &stream)
    -> absl::flat_hash_map<std::uint64_t, CategoryRow>;

} // namespace net_zelcon::wikidice