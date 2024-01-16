#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace net_zelcon::wikidice {

using CategoryId = uint64_t;
using PageId = uint64_t;

enum class CategoryLinkType {
    PAGE = 0,
    SUBCAT = 1,
    FILE = 2,
};

auto from(std::string_view sv) -> CategoryLinkType;

auto to_string(CategoryLinkType) -> std::string;

struct CategoryLinksRow {
    std::string category_name;
    PageId page_id;
    CategoryLinkType page_type;
};

struct PageTableRow {
    PageId page_id;
    std::string page_title;
    bool is_redirect;
};

struct CategoryRow {
    CategoryId category_id;
    std::string category_name;
    std::uint32_t page_count;
    std::uint32_t subcategory_count;
};

} // namespace net_zelcon::wikidice