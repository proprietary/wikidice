#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/random/bit_gen_ref.h>
#include <absl/random/random.h>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <msgpack.hpp>
#include <rocksdb/db.h>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "category_link_type.h"
#include "category_table.h"

namespace net_zelcon::wikidice {

class CategoryTreeIndex {
  private:
    rocksdb::DB *db_;
    std::vector<rocksdb::ColumnFamilyHandle *> column_family_handles_;
    rocksdb::ColumnFamilyHandle *subcategories_cf_;
    rocksdb::ColumnFamilyHandle *pages_cf_;
    rocksdb::ColumnFamilyHandle *weight_cf_;
    std::shared_ptr<CategoryTable> category_table_;
    msgpack::sbuffer sbuf_;

  public:
    CategoryTreeIndex(const std::filesystem::path db_path,
                      std::shared_ptr<CategoryTable> category_table);
    ~CategoryTreeIndex() {
        for (auto cf : column_family_handles_) {
            delete cf;
        }
        delete db_;
    }
    CategoryTreeIndex(const CategoryTreeIndex &other) = delete;
    CategoryTreeIndex &operator=(const CategoryTreeIndex &other) = delete;
    CategoryTreeIndex(CategoryTreeIndex &&other) {
        category_table_ = std::move(other.category_table_);
        sbuf_ = std::move(other.sbuf_);
        db_ = other.db_;
        column_family_handles_ = other.column_family_handles_;
        subcategories_cf_ = other.subcategories_cf_;
        pages_cf_ = other.pages_cf_;
        weight_cf_ = other.weight_cf_;
        other.db_ = nullptr;
        other.column_family_handles_.clear();
        other.subcategories_cf_ = nullptr;
        other.pages_cf_ = nullptr;
        other.weight_cf_ = nullptr;
    }
    CategoryTreeIndex &operator=(CategoryTreeIndex &&other) {
        category_table_ = std::move(other.category_table_);
        sbuf_ = std::move(other.sbuf_);
        db_ = other.db_;
        column_family_handles_ = other.column_family_handles_;
        subcategories_cf_ = other.subcategories_cf_;
        pages_cf_ = other.pages_cf_;
        weight_cf_ = other.weight_cf_;
        other.db_ = nullptr;
        other.column_family_handles_.clear();
        other.subcategories_cf_ = nullptr;
        other.pages_cf_ = nullptr;
        other.weight_cf_ = nullptr;
        return *this;
    }

    auto db() const noexcept -> rocksdb::DB * { return db_; }

    void import_categorylinks_row(const CategoryLinksRow &);

    void build_weights();

    auto pick(std::string_view category_name,
              absl::BitGenRef random_generator) -> std::optional<std::uint64_t>;

  private:
    auto
    lookup_pages(std::string_view category_name) -> std::vector<std::uint64_t>;

    auto
    lookup_subcats(std::string_view category_name) -> std::vector<std::string>;

    auto lookup_weight(std::string_view category_name)
        -> std::optional<std::uint64_t>;

    void add_subcategory(const std::string_view category_name,
                         const std::string_view subcategory_name);

    void add_page(const std::string_view category_name,
                  const std::uint64_t page_id);

    void set_category_members(const std::string_view category_name,
                              const std::vector<std::uint64_t> &page_ids);

    void set_weight(const std::string_view category_name,
                    const std::uint64_t weight);

    void set_subcategories(const std::string_view category_name,
                           const std::vector<std::string> &subcategories);

    auto at_index(std::string_view category_name,
                  std::uint64_t index) -> std::uint64_t;

    auto compute_weight(std::string_view category_name) -> std::uint64_t;
};

} // namespace net_zelcon::wikidice