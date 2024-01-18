#include "wikidice_query.h"
#include <fmt/core.h>
#include <fmt/ranges.h>
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

auto Session::take(uint64_t n) -> std::vector<std::string> {
    std::vector<std::string> dst;
    index_->for_each([&dst, &n](std::string_view category_name,
                                const CategoryLinkRecord &record) -> bool {
        if (n-- == 0)
            return false;
        dst.push_back(fmt::format("category name: {}, subcats={}",
                                  category_name, record.subcategories()));
        return true;
    });
    return dst;
}

auto Session::get_entry(std::string_view category_name) -> std::string {
    auto entry = index_->get(category_name);
    if (!entry)
        return {};
    auto s = index_->to_string(entry.value());
    return fmt::format("Category {}: {}", category_name, s);
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

Session::Session(std::string_view db_path)
    : Session{std::filesystem::path{db_path}} {}

} // namespace net_zelcon::wikidice