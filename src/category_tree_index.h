#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/random/bit_gen_ref.h>
#include <absl/random/random.h>
#include <absl/types/span.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <limits>
#include <memory>
#include <rocksdb/db.h>
#include <rocksdb/merge_operator.h>
#include <rocksdb/write_batch.h>
#include <string>
#include <string_view>
#include <vector>

#include "category_table.h"
#include "entities/entities.h"
#include "wiki_page_table.h"

namespace net_zelcon::wikidice {

class CategoryLinkRecordMergeOperator
    : public rocksdb::AssociativeMergeOperator {
  public:
    bool Merge(const rocksdb::Slice &key, const rocksdb::Slice *existing_value,
               const rocksdb::Slice &value, std::string *new_value,
               rocksdb::Logger *logger) const override;

    const char *Name() const override {
        return "CategoryLinkRecordMergeOperator";
    }
};

class CategoryTreeIndex {
  protected:
    rocksdb::DB *db_;
    std::vector<rocksdb::ColumnFamilyHandle *> column_family_handles_;
    rocksdb::ColumnFamilyHandle *categorylinks_cf_;
    rocksdb::ColumnFamilyHandle *category_id_to_name_cf_;

    CategoryTreeIndex() {}

  public:
    explicit CategoryTreeIndex(const std::filesystem::path db_path);
    virtual ~CategoryTreeIndex() {
        for (auto cf : column_family_handles_)
            if (cf != nullptr)
                delete cf;

        if (db_ != nullptr)
            delete db_;
    }
    CategoryTreeIndex(const CategoryTreeIndex &other) = delete;
    CategoryTreeIndex &operator=(const CategoryTreeIndex &other) = delete;
    CategoryTreeIndex(CategoryTreeIndex &&other) {
        db_ = other.db_;
        other.db_ = nullptr;
        column_family_handles_ = std::move(other.column_family_handles_);
        categorylinks_cf_ = other.categorylinks_cf_;
        other.categorylinks_cf_ = nullptr;
        category_id_to_name_cf_ = other.category_id_to_name_cf_;
        other.category_id_to_name_cf_ = nullptr;
    }
    virtual CategoryTreeIndex &operator=(CategoryTreeIndex &&other) {
        if (db_ != nullptr)
            delete db_;
        db_ = other.db_;
        other.db_ = nullptr;
        column_family_handles_ = std::move(other.column_family_handles_);
        categorylinks_cf_ = other.categorylinks_cf_;
        other.categorylinks_cf_ = nullptr;
        category_id_to_name_cf_ = other.category_id_to_name_cf_;
        other.category_id_to_name_cf_ = nullptr;
        return *this;
    }

    auto db() const noexcept -> rocksdb::DB * { return db_; }

    auto get(std::string_view category_name)
        -> std::optional<entities::CategoryLinkRecord>;

    auto to_string(const entities::CategoryLinkRecord &) -> std::string;

    virtual auto count_rows() -> uint64_t;

    void run_compaction();

    /**
     * @brief Compute the weight of a category, which is the number of "leaf"
     * pages underneath the category, up to a specified depth.
     * @param category_name The name of the category.
     * @param max_depth The maximum depth (recursive, nested subcategories) to
     * search for pages.
     * @details This is a breadth-first search of the category tree, up to a
     * specified depth. The weight of a category is the number of pages in the
     * category, plus the number of pages in all subcategories, plus the number
     * of pages in all sub-subcategories, etc. up to the specified depth.
     */
    auto compute_weight(std::string_view category_name,
                        uint8_t max_depth) -> std::uint64_t;

  protected:
    auto
    lookup_pages(std::string_view category_name) -> std::vector<std::uint64_t>;

    auto
    lookup_subcats(std::string_view category_name) -> std::vector<std::string>;

    auto at_index(std::string_view category_name, std::uint64_t index,
                  uint8_t depth) -> std::uint64_t;

    virtual auto category_name_of(entities::CategoryId category_id)
        -> std::optional<std::string>;

    virtual auto
    categorylinks_cf_options() const -> rocksdb::ColumnFamilyOptions;

    virtual auto db_options() const -> rocksdb::DBOptions;

    static constexpr size_t PREFIX_CAP_LEN = 8;
    static constexpr size_t BLOOM_FILTER_BITS_PER_KEY = 10;

    /**
     * @brief Map a list of category IDs to their names.
     */
    auto map_categories(absl::Span<const entities::CategoryId>)
        -> std::vector<std::string>;

    auto category_id_to_name_cf_options() const -> rocksdb::ColumnFamilyOptions;
};

class CategoryTreeIndexWriter : public CategoryTreeIndex {
  public:
    void import_categorylinks_row(const entities::CategoryLinksRow &);

    void
    import_categorylinks_rows(const std::vector<entities::CategoryLinksRow> &);

    void import_categorylinks_rows(
        const std::vector<const entities::CategoryLinksRow *> &);

    void import_category_row(const entities::CategoryRow &);

    void run_second_pass();

    explicit CategoryTreeIndexWriter(
        const std::filesystem::path db_path,
        std::shared_ptr<CategoryTable> category_table,
        std::shared_ptr<WikiPageTable> wiki_page_table, uint32_t n_threads);

    CategoryTreeIndexWriter(CategoryTreeIndexWriter &&other) {
        db_ = other.db_;
        other.db_ = nullptr;
        column_family_handles_ = std::move(other.column_family_handles_);
        categorylinks_cf_ = other.categorylinks_cf_;
        other.categorylinks_cf_ = nullptr;
        category_id_to_name_cf_ = other.category_id_to_name_cf_;
        other.category_id_to_name_cf_ = nullptr;
        category_table_ = std::move(other.category_table_);
        // categorylinks_count_ is atomic, so no need to copy it.
        categorylinks_count_.store(other.categorylinks_count_.load());
        other.categorylinks_count_.store(0);
    }
    CategoryTreeIndexWriter &operator=(CategoryTreeIndexWriter &&other) {
        category_table_ = std::move(other.category_table_);
        n_threads_ = other.n_threads_;
        // categorylinks_count_ is atomic, so no need to copy it.
        categorylinks_count_.store(other.categorylinks_count_.load());
        other.categorylinks_count_.store(0);
        return *this;
    }

    auto count_rows() -> uint64_t override;

  protected:
    auto
    categorylinks_cf_options() const -> rocksdb::ColumnFamilyOptions override;

    auto db_options() const -> rocksdb::DBOptions override;

    auto category_name_of(entities::CategoryId category_id)
        -> std::optional<std::string> final;

  private:
    void set(std::string_view category_name,
             const entities::CategoryLinkRecord &);

    void set_weight(std::string_view category_name,
                    const entities::CategoryWeight &);

    void add_subcategory(rocksdb::WriteBatch &,
                         const std::string_view category_name,
                         const entities::CategoryId subcategory_id);

    void add_page(rocksdb::WriteBatch &, const std::string_view category_name,
                  const entities::PageId page_id);

    void build_weights(const uint8_t depth_begin, const uint8_t depth_end);

    auto page_id_to_category_id(entities::PageId page_id)
        -> std::optional<entities::CategoryId>;

    void prune_dangling_subcategories();

  private:
    std::shared_ptr<CategoryTable> category_table_;
    std::shared_ptr<WikiPageTable> wiki_page_table_;
    uint32_t n_threads_;
    std::atomic<uint64_t> categorylinks_count_;

    static constexpr uint8_t DEPTH_BEGIN = 0;
    static constexpr uint8_t DEPTH_END = 100;
};

class CategoryTreeIndexReader : public CategoryTreeIndex {
  public:
    auto pick_at_depth(std::string_view category_name, uint8_t depth,
                       absl::BitGenRef) -> std::optional<entities::PageId>;

    auto search_categories(std::string_view category_name_prefix, size_t requested_count = 10)
        -> std::vector<std::string>;

    auto for_each(std::function<bool(std::string_view,
                                     const entities::CategoryLinkRecord &)>)
        -> void;

    explicit CategoryTreeIndexReader(const std::filesystem::path db_path);
};

} // namespace net_zelcon::wikidice
