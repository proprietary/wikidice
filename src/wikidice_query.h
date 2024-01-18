#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "category_tree_index.h"

namespace net_zelcon::wikidice {

class Session {
  public:
    auto autocomplete_category_name(std::string_view)
        -> std::vector<std::string>;
    auto pick_random_article(std::string_view category_name)
        -> std::pair<uint64_t, bool>;
    auto take(uint64_t n) -> std::vector<std::string>;
    auto get_entry(std::string_view category_name) -> std::string;
    explicit Session(const std::filesystem::path);
    explicit Session(std::string_view);

  private:
    std::shared_ptr<CategoryTreeIndexReader> index_;
    absl::BitGen rng_;
};

} // namespace net_zelcon::wikidice