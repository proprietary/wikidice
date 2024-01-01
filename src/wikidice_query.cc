#include "wikidice_query.h"
#include <fmt/core.h>
#include <fstream>

namespace net_zelcon::wikidice {

auto Session::pick_random_article(std::string_view category_name)
    -> std::pair<uint64_t, bool> {
    const auto article = index_->pick(category_name, rng_);
    if (article) {
        return std::make_pair(*article, true);
    } else {
        return std::make_pair(0ULL, false);
    }
}

auto Session::autocomplete_category_name(std::string_view category_name)
    -> std::vector<std::string> {
    return index_->search_categories(category_name);
}

Session::Session(const std::filesystem::path db_path) {
    // check path
    if (!std::filesystem::exists(db_path) ||
        !std::filesystem::is_directory(db_path.parent_path()))
        throw std::invalid_argument{fmt::format(
            "{} is not a directory",
            std::filesystem::absolute(db_path.parent_path()).string())};
    // open database, assuming it is already built
    index_ = std::make_shared<CategoryTreeIndexReader>(db_path);
}

} // namespace net_zelcon::wikidice