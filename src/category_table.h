#pragma once

#include "entities/entities.h"
#include <absl/container/flat_hash_map.h>
#include <functional>
#include <string>

namespace net_zelcon::wikidice {

class CategoryTable {
  public:
    void add_category(const entities::CategoryRow &category_row);
    auto find(const entities::CategoryId cat_id) const -> std::optional<entities::CategoryRow>;
    auto find(const std::string_view category_name) const
        -> std::optional<entities::CategoryRow>;
    void for_each(std::function<void(const entities::CategoryRow &)> callback) const;

  private:
    absl::flat_hash_map<std::uint64_t, entities::CategoryRow> by_category_id_;
    absl::flat_hash_map<std::string, entities::CategoryRow> by_category_name_;
};

} // namespace net_zelcon::wikidice