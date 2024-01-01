#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace net_zelcon::wikidice {

enum class CategoryLinkType {
    PAGE = 0,
    SUBCAT = 1,
    FILE = 2,
};

auto from(std::string_view sv) -> CategoryLinkType;

auto to_string(CategoryLinkType) -> std::string;

struct CategoryLinksRow {
    std::string category_name;
    std::uint64_t page_id;
    CategoryLinkType page_type;
};

struct CategoryRow {
    std::uint64_t category_id;
    std::string category_name;
    std::uint32_t page_count;
    std::uint32_t subcategory_count;
};

} // namespace net_zelcon::wikidice