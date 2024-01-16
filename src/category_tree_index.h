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
#include <msgpack.hpp>
#include <rocksdb/db.h>
#include <rocksdb/merge_operator.h>
#include <string>
#include <string_view>
#include <vector>

#include "category_link_type.h"
#include "category_table.h"
#include "wiki_page_table.h"

namespace net_zelcon::wikidice {

class CategoryLinkRecord {
  public:
    CategoryLinkRecord() {}
    auto pages() const noexcept -> std::vector<PageId> { return pages_; }
    auto subcategories() const noexcept -> std::vector<CategoryId> {
        return subcategories_;
    }
    auto weight() const noexcept -> uint64_t { return weight_; }
    auto pages_mut() noexcept -> std::vector<PageId> & { return pages_; }
    auto subcategories_mut() noexcept -> std::vector<CategoryId> & {
        return subcategories_;
    }
    auto weight_mut() noexcept -> uint64_t & { return weight_; }
    friend bool operator==(const CategoryLinkRecord &lhs,
                           const CategoryLinkRecord &rhs) {
        return lhs.pages_ == rhs.pages_ &&
               lhs.subcategories_ == rhs.subcategories_ &&
               lhs.weight_ == rhs.weight_;
    }
    void pages(std::vector<PageId> &&pages) noexcept {
        pages_ = std::move(pages);
    }
    void subcategories(std::vector<CategoryId> &&subcategories) noexcept {
        subcategories_ = std::move(subcategories);
    }
    void weight(const uint64_t weight) noexcept { weight_ = weight; }

    /**
     * @brief Merge another CategoryLinkRecord into this one.
     * @details This is used by the RocksDB merge operator.
     */
    auto operator+=(const CategoryLinkRecord &other) -> void {
        pages_.insert(pages_.end(), other.pages_.begin(), other.pages_.end());
        subcategories_.insert(subcategories_.end(),
                              other.subcategories_.begin(),
                              other.subcategories_.end());
        weight_ += other.weight_;
    }

  private:
    std::vector<PageId> pages_;
    std::vector<CategoryId> subcategories_;
    uint64_t weight_ = 0ULL;
};

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

    auto
    get(std::string_view category_name) -> std::optional<CategoryLinkRecord>;

    auto to_string(const CategoryLinkRecord &) -> std::string;

    virtual auto count_rows() -> uint64_t;

    void run_compaction();

  protected:
    auto
    lookup_pages(std::string_view category_name) -> std::vector<std::uint64_t>;

    auto
    lookup_subcats(std::string_view category_name) -> std::vector<std::string>;

    auto lookup_weight(std::string_view category_name)
        -> std::optional<std::uint64_t>;

    auto at_index(std::string_view category_name,
                  std::uint64_t index) -> std::uint64_t;

    virtual auto
    category_name_of(CategoryId category_id) -> std::optional<std::string>;

    virtual auto
    categorylinks_cf_options() const -> rocksdb::ColumnFamilyOptions;

    virtual auto db_options() const -> rocksdb::DBOptions;

    static constexpr size_t PREFIX_CAP_LEN = 8;
    static constexpr size_t BLOOM_FILTER_BITS_PER_KEY = 10;

    /**
     * @brief Map a list of category IDs to their names.
     */
    auto map_categories(absl::Span<const CategoryId>)
        -> std::vector<std::string>;

    auto category_id_to_name_cf_options() const -> rocksdb::ColumnFamilyOptions;
};

class CategoryTreeIndexWriter : public CategoryTreeIndex {
  public:
    void import_categorylinks_row(const CategoryLinksRow &);

    void import_category_row(const CategoryRow &);

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

    auto category_name_of(CategoryId category_id)
        -> std::optional<std::string> final;

  private:
    void set(std::string_view category_name, const CategoryLinkRecord &);

    void add_subcategory(const std::string_view category_name,
                         const CategoryId subcategory_id);

    void add_page(const std::string_view category_name, const PageId page_id);

    void set_weight(const std::string_view category_name,
                    const std::uint64_t weight);

    void build_weights();

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
    auto
    compute_weight(std::string_view category_name,
                   float max_depth = std::numeric_limits<float>::infinity())
        -> std::uint64_t;

    auto page_id_to_category_id(PageId page_id) -> std::optional<CategoryId>;

  private:
    std::shared_ptr<CategoryTable> category_table_;
    std::shared_ptr<WikiPageTable> wiki_page_table_;
    uint32_t n_threads_;
    std::atomic<uint64_t> categorylinks_count_;
};

class CategoryTreeIndexReader : public CategoryTreeIndex {
  public:
    auto pick(std::string_view category_name,
              absl::BitGenRef random_generator) -> std::optional<PageId>;

    auto search_categories(std::string_view category_name_prefix)
        -> std::vector<std::string>;

    auto for_each(
        std::function<bool(std::string_view, const CategoryLinkRecord &)>)
        -> void;

    explicit CategoryTreeIndexReader(const std::filesystem::path db_path);
};

} // namespace net_zelcon::wikidice

// Serialization and deserialization logic for the primary column type.
namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
    namespace adaptor {

    template <> struct convert<net_zelcon::wikidice::CategoryLinkRecord> {
        msgpack::object const &
        operator()(const msgpack::object &o,
                   net_zelcon::wikidice::CategoryLinkRecord &v) const {
            if (o.type != msgpack::type::ARRAY)
                throw msgpack::type_error();
            if (o.via.array.size != 3)
                throw msgpack::type_error();
            v.pages(o.via.array.ptr[0].as<std::vector<uint64_t>>());
            v.subcategories(o.via.array.ptr[1].as<std::vector<uint64_t>>());
            v.weight(o.via.array.ptr[2].as<uint64_t>());
            return o;
        }
    };

    template <> struct pack<net_zelcon::wikidice::CategoryLinkRecord> {
        template <typename Stream>
        packer<Stream> &
        operator()(msgpack::packer<Stream> &o,
                   net_zelcon::wikidice::CategoryLinkRecord const &v) const {
            o.pack_array(3);
            o.pack(v.pages());
            o.pack(v.subcategories());
            o.pack(v.weight());
            return o;
        }
    };

    template <>
    struct object_with_zone<net_zelcon::wikidice::CategoryLinkRecord> {
        void
        operator()(msgpack::object::with_zone &o,
                   net_zelcon::wikidice::CategoryLinkRecord const &v) const {
            o.type = type::ARRAY;
            o.via.array.size = 3;
            o.via.array.ptr =
                static_cast<msgpack::object *>(o.zone.allocate_align(
                    sizeof(msgpack::object) * o.via.array.size,
                    MSGPACK_ZONE_ALIGNOF(msgpack::object)));
            o.via.array.ptr[0] = msgpack::object{v.pages(), o.zone};
            o.via.array.ptr[1] = msgpack::object{v.subcategories(), o.zone};
            o.via.array.ptr[2] = msgpack::object{v.weight(), o.zone};
        }
    };

    } // namespace adaptor
} // MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
} // namespace msgpack
