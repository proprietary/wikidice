#pragma once

#include "rocksdb/db.h"
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <absl/container/flat_hash_map.h>

#include "category_link_type.h"

namespace net_zelcon::wikidice {

class CategoryTreeIndex {
  private:
    rocksdb::DB *db_;
    std::vector<rocksdb::ColumnFamilyHandle *> column_family_handles_;
    rocksdb::ColumnFamilyHandle* subcategories_cf_;
    rocksdb::ColumnFamilyHandle* pages_cf_;
    rocksdb::ColumnFamilyHandle* weight_cf_;
    absl::flat_hash_map<std::uint64_t, CategoryRow> category_table_;

  public:
    CategoryTreeIndex(const std::filesystem::path db_path);
    ~CategoryTreeIndex() {
        for (auto cf : column_family_handles_) {
            delete cf;
        }
        delete db_;
    }
    CategoryTreeIndex(const CategoryTreeIndex & other) = delete;
    CategoryTreeIndex &operator=(const CategoryTreeIndex & other) = delete;
    CategoryTreeIndex(CategoryTreeIndex&& other) {
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
    CategoryTreeIndex &operator=(CategoryTreeIndex&& other) {
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

    auto db() const noexcept -> rocksdb::DB* {
        return db_;
    }

    void setup_column_families();

    void import_row(const std::uint64_t page_id, const std::string_view category_name, const CategoryLinkType category_link_type);

    void import_categorylinks_row(const CategoryLinksRow&);

    auto lookup_pages(std::string_view category_name) -> std::vector<std::uint64_t>;
    auto lookup_subcats(std::string_view category_name) -> std::vector<std::string>;
    auto lookup_weight(std::string_view category_name) -> std::uint64_t;

private:

  void add_subcategory(rocksdb::WriteBatch&, const std::string_view category_name, const std::string_view subcategory_name);

  void add_page(rocksdb::WriteBatch&, const std::string_view category_name, const std::uint64_t page_id);

  void add_to_weight(rocksdb::WriteBatch&, const std::string_view category_name, const std::uint64_t weight_to_add = 1UL);

  void perform_second_pass();

  auto ancestral_categories_of(const std::uint64_t page_id) -> std::vector<std::string>;
};

} // namespace net_zelcon::wikidice