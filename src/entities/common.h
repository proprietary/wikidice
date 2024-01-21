#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <optional>

namespace net_zelcon::wikidice::entities {

using CategoryId = uint64_t;
using PageId = uint64_t;

enum class CategoryLinkType {
    PAGE = 0,
    SUBCAT = 1,
    FILE = 2,
};

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

struct CategoryWeight {
    uint8_t depth;
    uint64_t weight;
};

inline bool operator==(const CategoryWeight& lhs, const CategoryWeight& rhs) {
    return lhs.weight == rhs.weight && lhs.depth == rhs.depth;
}

class CategoryLinkRecord {
  public:
    CategoryLinkRecord() {}

    auto pages() const noexcept -> const std::vector<PageId>& { return pages_; }
    auto pages_mut() noexcept -> std::vector<PageId> & { return pages_; }

    auto subcategories() const noexcept -> const std::vector<CategoryId>& {
        return subcategories_;
    }
    auto subcategories_mut() noexcept -> std::vector<CategoryId> & {
        return subcategories_;
    }

    auto weights() const noexcept -> const std::vector<CategoryWeight>& { return weights_; }
    auto weights_mut() noexcept -> std::vector<CategoryWeight>& { return weights_; }

    auto weight_at_depth(uint8_t depth) const noexcept -> uint64_t {
        const auto len = weights_.size();
        if (len == 0) {
            return 0; // errored
        }
        size_t l = 0;
        size_t r = len - 1;
        while (l < r) {
            const auto m = (l + r) / 2;
            if (weights_[m].depth == depth) {
                return weights_[m].weight;
            } else if (weights_[m].depth < depth) {
                l = m + 1;
            } else if (weights_[m].depth > depth) {
                r = m - 1;
            }
        }
        return weights_.back().weight;
    }

    friend bool operator==(const CategoryLinkRecord &lhs,
                           const CategoryLinkRecord &rhs) {
        return lhs.pages_ == rhs.pages_ &&
               lhs.subcategories_ == rhs.subcategories_ &&
               lhs.weights_ == rhs.weights_;
    }

    /**
     * @brief Merge another CategoryLinkRecord into this one.
     * @details This is used by the RocksDB merge operator.
     */
    auto operator+=(CategoryLinkRecord &&other) -> void;

  private:
    std::vector<PageId> pages_;
    std::vector<CategoryId> subcategories_;
    std::vector<CategoryWeight> weights_;
};


} // namespace net_zelcon::wikidice::entities
