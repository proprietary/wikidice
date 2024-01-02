#pragma once

#include "category_link_type.h"
#include <absl/container/flat_hash_map.h>
#include <functional>
#include <string>

namespace net_zelcon::wikidice {

class CategoryTable {
  public:
    void add_category(const CategoryRow &category_row);
    auto find(const std::uint64_t cat_id) const -> std::optional<CategoryRow>;
    auto find(const std::string_view category_name) const
        -> std::optional<CategoryRow>;
    void for_each(std::function<void(const CategoryRow &)> callback) const;

  private:
    absl::flat_hash_map<std::uint64_t, CategoryRow> by_category_id_;
    absl::flat_hash_map<std::string, CategoryRow> by_category_name_;
};

} // namespace net_zelcon::wikidice