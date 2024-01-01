#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <memory>

#include "category_tree_index.h"

namespace net_zelcon::wikidice {

class Session {
public:
    auto autocomplete_category_name(std::string_view) -> std::vector<std::string>;
    auto pick_random_article(std::string_view category_name) -> uint64_t;
private:
    std::shared_ptr<CategoryTreeIndex> index_;
    absl::BitGen rng_;
};

} // namespace net_zelcon::wikidice