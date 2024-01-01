#include "category_link_type.h"

namespace net_zelcon::wikidice {

auto from(std::string_view sv) -> CategoryLinkType {
    using std::literals::string_view_literals::operator""sv;
    if (sv == "page"sv) {
        return CategoryLinkType::PAGE;
    } else if (sv == "subcat"sv) {
        return CategoryLinkType::SUBCAT;
    } else if (sv == "file"sv) {
        return CategoryLinkType::FILE;
    } else {
        throw std::invalid_argument("invalid category link type");
    }
}

auto to_string(CategoryLinkType type) -> std::string {
    switch (type) {
    case CategoryLinkType::PAGE:
        return "page";
    case CategoryLinkType::SUBCAT:
        return "subcat";
    case CategoryLinkType::FILE:
        return "file";
    }
    throw std::invalid_argument("invalid category link type");
}

} // namespace net_zelcon::wikidice