#include "category_tree_index.h"

#include <absl/log/check.h>
#include <absl/log/log.h>
#include <algorithm>
#include <array>
#include <fmt/core.h>
#include <iterator>
#include <queue>
#include <rocksdb/filter_policy.h>
#include <rocksdb/slice_transform.h>
#include <rocksdb/table.h>
#include <stdexcept>
#include <zstd.h>

namespace net_zelcon::wikidice {

CategoryTreeIndex::CategoryTreeIndex(const std::filesystem::path db_path) {
    std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
    rocksdb::ColumnFamilyOptions cf_options;
    cf_options.write_buffer_size = 128 * (1 << 20);
    // enable compression
    cf_options.compression = rocksdb::kZSTD;
    cf_options.bottommost_compression = rocksdb::kZSTD;
    cf_options.compression_opts.level = ZSTD_maxCLevel();
    cf_options.compression_opts.max_dict_bytes = 8192;
    cf_options.compression_opts.zstd_max_train_bytes = 8192;
    // enable Bloom filter for efficient prefix seek
    rocksdb::BlockBasedTableOptions table_options;
    table_options.filter_policy.reset(
        rocksdb::NewRibbonFilterPolicy(BLOOM_FILTER_BITS_PER_KEY));
    cf_options.table_factory.reset(
        rocksdb::NewBlockBasedTableFactory(table_options));
    cf_options.prefix_extractor.reset(
        rocksdb::NewCappedPrefixTransform(PREFIX_CAP_LEN));
    // setup column families
    column_families.emplace_back(rocksdb::kDefaultColumnFamilyName,
                                 rocksdb::ColumnFamilyOptions{});
    column_families.emplace_back("categorylinks", cf_options);
    // setup database
    rocksdb::DBOptions db_options;
    db_options.create_if_missing = true;
    db_options.create_missing_column_families = true;
    db_options.error_if_exists = false;
    rocksdb::Status status =
        rocksdb::DB::Open(db_options, db_path.string(), column_families,
                          &column_family_handles_, &db_);
    if (!status.ok()) {
        LOG(FATAL) << "Failed to open db at " << db_path.string() << ": "
                   << status.ToString();
    }
    CHECK(column_family_handles_.size() == 2);
    CHECK(column_family_handles_[1]->GetName() == "categorylinks");
    categorylinks_cf_ = column_family_handles_[1];
}

auto CategoryTreeIndex::get(std::string_view category_name)
    -> std::optional<CategoryLinkRecord> {
    // 1. Retrieve from RocksDB database
    rocksdb::Slice key{category_name.data(), category_name.length()};
    rocksdb::PinnableSlice value;
    rocksdb::ReadOptions read_options;
    const auto status = db_->Get(read_options, categorylinks_cf_, key, &value);
    if (!status.ok()) {
        if (status.IsNotFound()) {
            return std::nullopt;
        }
        // TODO: make this recoverable
        LOG(FATAL) << "Failed to get category_name: " << category_name
                   << " status: " << status.ToString();
    }
    // 2. Deserialize
    const auto o = msgpack::unpack(zone_, value.data(), value.size());
    CHECK(!o.is_nil());
    return o.as<CategoryLinkRecord>();
}

auto CategoryTreeIndexWriter::set(std::string_view category_name,
                                  const CategoryLinkRecord &record) -> void {
    // 1. Serialize
    msgpack::sbuffer buf;
    msgpack::pack(buf, record);
    rocksdb::Slice value{buf.data(), buf.size()};
    // 2. Add to RocksDB database
    rocksdb::WriteOptions write_options;
    const auto status = db_->Put(write_options, category_name, value);
    CHECK(status.ok()) << "Write of record failed: " << status.ToString();
}

void CategoryTreeIndexWriter::import_categorylinks_row(
    const CategoryLinksRow &row) {
    const auto category_row = category_table_->find(row.category_name);
    if (!category_row) {
        LOG(WARNING)
            << "category_name: " << row.category_name
            << " not found in `category` table, yet this category_name is a "
               "category according to the `categorylinks` table";
        return;
    }
    switch (row.page_type) {
    case CategoryLinkType::FILE:
        // ignore; we are not indexing files
        break;
    case CategoryLinkType::PAGE:
        add_page(row.category_name, row.page_id);
        break;
    case CategoryLinkType::SUBCAT:
        add_subcategory(row.category_name, category_row->category_name);
        break;
    }
}

void CategoryTreeIndexWriter::add_subcategory(
    const std::string_view category_name,
    const std::string_view subcategory_name) {
    auto record = get(category_name);
    if (!record)
        record.emplace();
    record->subcategories_mut().emplace_back(subcategory_name);
    set(category_name, record.value());
}

void CategoryTreeIndexWriter::add_page(const std::string_view category_name,
                                       const std::uint64_t page_id) {
    auto record = get(category_name);
    if (!record)
        record.emplace();
    record->pages_mut().push_back(page_id);
    set(category_name, record.value());
}

auto CategoryTreeIndex::lookup_pages(std::string_view category_name)
    -> std::vector<std::uint64_t> {
    auto record = get(category_name);
    if (record)
        return record->pages();
    else
        return {};
}

auto CategoryTreeIndex::lookup_subcats(std::string_view category_name)
    -> std::vector<std::string> {
    auto record = get(category_name);
    if (record)
        return record->subcategories();
    else
        return {};
}

auto CategoryTreeIndex::lookup_weight(std::string_view category_name)
    -> std::optional<std::uint64_t> {
    auto record = get(category_name);
    if (record)
        return record->weight();
    else
        return std::nullopt;
}

void CategoryTreeIndexWriter::set_weight(const std::string_view category_name,
                                         const std::uint64_t weight) {
    auto record = get(category_name);
    if (record) {
        record->weight(weight);
        set(category_name, record.value());
    } else {
        LOG(WARNING) << "category_name: " << category_name
                     << " not found in `categorylinks` column family";
        CategoryLinkRecord new_record{};
        new_record.weight(weight);
        set(category_name, new_record);
    }
}

auto CategoryTreeIndexReader::pick(std::string_view category_name,
                                   absl::BitGenRef random_generator)
    -> std::optional<std::uint64_t> {
    const auto weight = lookup_weight(category_name);
    if (!weight) {
        return std::nullopt;
    }
    const auto picked =
        absl::Uniform<std::uint64_t>(random_generator, 0UL, weight.value());
    return at_index(category_name, picked);
}

auto CategoryTreeIndex::at_index(std::string_view category_name,
                                 std::uint64_t index) -> std::uint64_t {
    auto record = get(category_name);
    if (!record)
        LOG(WARNING) << "category_name: " << category_name
                     << " not found in `categorylinks` column family";
    const auto &pages = record->pages();
    if (index < pages.size()) {
        return pages[index];
    }
    index -= pages.size();
    for (const auto &subcat : record->subcategories()) {
        const auto weight = lookup_weight(subcat);
        if (!weight.has_value()) {
            LOG(WARNING) << "category_name: " << subcat
                         << " not found in `categorylinks` column family";
            continue;
        }
        if (index < weight.value()) {
            return at_index(subcat, index);
        }
        index -= weight.value();
    }
    LOG(WARNING) << "index: " << index
                 << " out of range for category_name: " << category_name;
    return 0;
}

// Run second-pass to build the "weights" column family, which consists of the
// recursive sum of all the pages under a category.
void CategoryTreeIndexWriter::build_weights() {
    rocksdb::ReadOptions read_options;
    read_options.adaptive_readahead = true;
    read_options.total_order_seek = true; // ignore prefix Bloom filter in read
    rocksdb::Iterator *it = db_->NewIterator(read_options, categorylinks_cf_);
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string_view category_name{it->key().data(), it->key().size()};
        const std::uint64_t weight = compute_weight(category_name);
        set_weight(category_name, weight);
    }
    CHECK(it->status().ok()) << it->status().ToString();
}

auto CategoryTreeIndexWriter::compute_weight(
    std::string_view category_name, float32_t max_depth) -> std::uint64_t {
    uint64_t weight = 0;
    if (!get(category_name)) {
        LOG(WARNING) << "category name: " << std::quoted(category_name)
                     << "not found in `categorylinks` column family";
        return weight;
    }
    std::vector<std::string> categories_visited;
    std::queue<std::string> categories_to_visit;
    categories_to_visit.emplace(category_name);
    float32_t depth = 0.;
    while (!categories_to_visit.empty() && depth <= max_depth) {
        const auto top = categories_to_visit.front();
        categories_to_visit.pop();
        if (std::ranges::find(categories_visited, top) !=
            categories_visited.end())
            continue; // this prevents a cycle
        categories_visited.push_back(top);
        // add number of pages in this category to "weight"
        weight += lookup_pages(top).size();
        // enqueue the subcategories of this category
        for (const auto &subcat : lookup_subcats(top))
            categories_to_visit.push(subcat);
        // increment depth
        depth += 1.0f;
    }
    return weight;
}

auto CategoryTreeIndexWriter::run_second_pass() -> void {
    build_weights();
    // make sure compression is complete
    rocksdb::FlushOptions flush_options;
    flush_options.wait = true;
    db_->Flush(flush_options);
}

auto CategoryTreeIndexReader::search_categories(
    std::string_view category_name_prefix) -> std::vector<std::string> {
    std::vector<std::string> autocompletions{};
    static constexpr size_t MAX_CATEGORY_NAME_PREFIX_LEN = 1'000;
    if (category_name_prefix.length() > MAX_CATEGORY_NAME_PREFIX_LEN)
        return autocompletions;
    rocksdb::ReadOptions read_options;
    read_options.auto_prefix_mode = true;
    rocksdb::Iterator *it = db_->NewIterator(read_options, categorylinks_cf_);
    for (it->Seek(category_name_prefix);
         it->Valid() && it->key().starts_with(category_name_prefix);
         it->Next()) {
        autocompletions.emplace_back(it->value().data(), it->value().size());
    }
    return autocompletions;
}

} // namespace net_zelcon::wikidice
