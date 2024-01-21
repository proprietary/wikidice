#include "common.h"
#include "entities.h"
#include "msgpack_serde_logic.h"
#include <stdexcept>
#include <string_view>
#include <algorithm>
#include <absl/log/check.h>
#include <msgpack.hpp>

namespace net_zelcon::wikidice::entities {

namespace {
class ByteBuffer {
    std::vector<uint8_t>& data_;
public:
    ByteBuffer() = delete;
    explicit ByteBuffer(std::vector<uint8_t>& data) : data_(data) {}
    void write(const char* data, std::size_t size) {
        DCHECK(data != nullptr);
        this->data_.reserve(this->data_.size() + size);
        for (std::size_t i = 0; i < size; i++) {
            this->data_.push_back(data[i]);
        }
    }
};
} // namespace

/**
 * Merges two vectors of CategoryWeight objects.
 *
 * @param lhs The first vector to be merged.
 * @param rhs The second vector to be merged.
 */
void merge(std::vector<CategoryWeight>& lhs, std::vector<CategoryWeight>& rhs) {
    if (rhs.empty())
        return;
    const auto comparison = [](const auto& a, const auto& b) { return a.depth < b.depth; };
    const auto equality = [](const auto& a, const auto& b) { return a.depth == b.depth; };
    // ensure uniqueness
    auto last = std::unique(rhs.begin(), rhs.end(), equality);
    rhs.erase(last, rhs.end());
    last = std::unique(lhs.begin(), lhs.end(), equality);
    lhs.erase(last, lhs.end());
    // merging algorithm requires sortedness
    std::sort(lhs.begin(), lhs.end(), comparison);
    std::sort(rhs.begin(), rhs.end(), comparison);
    if (rhs.size() > lhs.size())
        lhs.reserve(rhs.size());
    size_t i = 0, j = 0;
    while (i < lhs.size() && j < rhs.size()) {
        if (lhs[i].depth == rhs[j].depth) {
            lhs[i].weight += rhs[j].weight;
            i++;
            j++;
        } else if (lhs[i].depth < rhs[j].depth) {
            i++;
        } else if (lhs[i].depth > rhs[j].depth) {
            lhs.insert(lhs.begin() + i, rhs[j]);
            j++;
        }
    }
    if (j < rhs.size()) {
        lhs.insert(lhs.end(), rhs.begin() + j, rhs.end());
    }
    DCHECK(std::is_sorted(lhs.begin(), lhs.end(), comparison)) << "not sorted: " << to_string(lhs.begin(), lhs.end());
    DCHECK_EQ(lhs.size(), static_cast<size_t>(std::ranges::unique(lhs, equality).end() - lhs.begin()));
}

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

auto CategoryLinkRecord::operator+=(CategoryLinkRecord &&other) -> void {
    pages_.insert(pages_.end(), other.pages_.begin(), other.pages_.end());
    subcategories_.insert(subcategories_.end(), other.subcategories_.begin(),
                          other.subcategories_.end());
    merge(weights_, other.weights_);
}

auto serialize(std::stringstream& dst, const CategoryLinkRecord& src) -> void {
    msgpack::pack(dst, src);
}

auto serialize(std::vector<uint8_t>& dst, const CategoryLinkRecord& src) -> void {
    ByteBuffer b{dst};
    msgpack::pack(b, src);
}

auto deserialize(CategoryLinkRecord& dst, const std::span<const uint8_t>& src) -> void {
    msgpack::object_handle oh = msgpack::unpack(reinterpret_cast<const char*>(src.data()), src.size());
    msgpack::object obj = oh.get();
    obj.convert(dst);
}

auto to_string(const CategoryWeight& src) -> std::string {
    std::stringstream ss;
    ss << "{depth: " << static_cast<uint32_t>(src.depth)
       << ", weight: " << src.weight << "}";
    return ss.str();

}

} // namespace net_zelcon::wikidice::entities