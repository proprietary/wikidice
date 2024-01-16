#include "category_table.h"
#include <algorithm>

namespace net_zelcon::wikidice {

void CategoryTable::add_category(const CategoryRow &category_row) {
    by_category_id_.emplace(category_row.category_id, category_row);
    by_category_name_.emplace(category_row.category_name, category_row);
}

auto CategoryTable::find(const CategoryId cat_id) const
    -> std::optional<CategoryRow> {
    const auto it = by_category_id_.find(cat_id);
    if (it == by_category_id_.end()) {
        return std::nullopt;
    }
    return it->second;
}

auto CategoryTable::find(const std::string_view category_name) const
    -> std::optional<CategoryRow> {
    const auto it = by_category_name_.find(category_name);
    if (it == by_category_name_.end()) {
        return std::nullopt;
    }
    return it->second;
}

auto CategoryTable::for_each(
    std::function<void(const CategoryRow &)> callback) const -> void {
    std::for_each(by_category_id_.begin(), by_category_id_.end(),
                  [&callback](const auto &pair) { callback(pair.second); });
}

} // namespace net_zelcon::wikidice