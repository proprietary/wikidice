#pragma once

#include "common.h"

#include <concepts>
#include <cstdint>
#include <iterator>
#include <span>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <vector>

namespace net_zelcon::wikidice::entities {

auto from(std::string_view sv) -> CategoryLinkType;

auto to_string(CategoryLinkType) -> std::string;

auto to_string(const CategoryWeight &) -> std::string;

auto serialize(std::stringstream &, const CategoryLinkRecord &src) -> void;

auto serialize(std::vector<uint8_t>& dst, const CategoryLinkRecord& src) -> void;

auto deserialize(CategoryLinkRecord &dst,
                 const std::span<const uint8_t> &src) -> void;

template <std::forward_iterator Iterator>
auto to_string(Iterator begin, Iterator end) -> std::string {
    static_assert(
        std::is_same_v<typename std::iterator_traits<Iterator>::value_type,
                       CategoryWeight>,
        "Iterator must be of type CategoryWeight");
    std::stringstream ss;
    ss << "[";
    for (auto it = begin; it != end; ++it) {
        ss << to_string(*it);
        if (std::next(it) != end) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}

void merge(std::vector<CategoryWeight>& lhs, std::vector<CategoryWeight>& rhs);

} // namespace net_zelcon::wikidice::entities